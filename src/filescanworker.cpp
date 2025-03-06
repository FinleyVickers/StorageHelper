#include "filescanworker.h"
#include "fileutils.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QThread>
#include <QtConcurrent>
#include <algorithm>
#include <deque>
#include <mutex>
#include <atomic>

// Work-stealing queue for better load balancing
class WorkQueue {
public:
    void push(const QString& dir) {
        std::lock_guard<std::mutex> lock(mutex);
        dirs.push_back(dir);
    }

    bool pop(QString& dir) {
        std::lock_guard<std::mutex> lock(mutex);
        if (dirs.empty()) return false;
        dir = std::move(dirs.front());
        dirs.pop_front();
        return true;
    }

    bool steal(QString& dir) {
        std::lock_guard<std::mutex> lock(mutex);
        if (dirs.empty()) return false;
        dir = std::move(dirs.back());
        dirs.pop_back();
        return true;
    }

private:
    std::deque<QString> dirs;
    std::mutex mutex;
};

FileScanWorker::FileScanWorker(QObject *parent)
    : QObject(parent), shouldStop(false), minimumSize(0) {}

void FileScanWorker::startScan(const QString& directory, qint64 minSize) {
    shouldStop = false;
    minimumSize = minSize;

    // Create thread pool for parallel scanning
    QThreadPool threadPool;
    const int maxThreads = std::max(1, QThread::idealThreadCount() - 1);
    threadPool.setMaxThreadCount(maxThreads);

    // Create work queues for each thread
    std::vector<std::unique_ptr<WorkQueue>> workQueues(maxThreads);
    for (int i = 0; i < maxThreads; ++i) {
        workQueues[i] = std::make_unique<WorkQueue>();
    }

    // Initialize the first queue with the root directory
    workQueues[0]->push(directory);

    // Shared data structures
    QList<FileInfo> results;
    std::mutex resultsMutex;
    std::atomic<qint64> totalProcessedSize{0};
    std::atomic<int> activeThreads{maxThreads};

    // Create worker functions for parallel processing
    auto scanFunction = [this, &workQueues, &results, &resultsMutex, &totalProcessedSize, 
                        &activeThreads, directory, maxThreads](int threadId) {
        QList<FileInfo> threadResults;
        threadResults.reserve(1000); // Pre-allocate space for batch processing

        // Local cache for file type lookups
        QHash<QString, QString> fileTypeCache;

        while (!shouldStop) {
            QString currentDir;
            bool hasWork = false;

            // Try to get work from own queue
            hasWork = workQueues[threadId]->pop(currentDir);

            // If no work in own queue, try to steal from others
            if (!hasWork) {
                for (int i = 0; i < maxThreads && !hasWork; ++i) {
                    if (i != threadId) {
                        hasWork = workQueues[i]->steal(currentDir);
                    }
                }
            }

            if (!hasWork) {
                if (--activeThreads == 0) break; // No more work available
                std::this_thread::yield();
                ++activeThreads;
                continue;
            }

            // Use recursive iterator to process entire subtree
            QDirIterator it(currentDir, 
                          QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);

            // Batch process files
            QVector<QPair<QString, QFileInfo>> batch;
            batch.reserve(100);

            while (it.hasNext() && !shouldStop) {
                QString filePath = it.next();
                QFileInfo fileInfo = it.fileInfo();

                if (fileInfo.isDir()) {
                    // Only queue directories that might contain large files
                    if (fileInfo.size() >= minimumSize) {
                        workQueues[threadId]->push(filePath);
                    }
                } else {
                    batch.append({filePath, fileInfo});
                    
                    // Process batch when full
                    if (batch.size() >= 100) {
                        processBatch(batch, threadResults, fileTypeCache, totalProcessedSize);
                        batch.clear();
                        batch.reserve(100);
                    }
                }
            }

            // Process remaining files in batch
            if (!batch.isEmpty()) {
                processBatch(batch, threadResults, fileTypeCache, totalProcessedSize);
            }

            // Submit results in batches
            if (threadResults.size() >= 1000) {
                std::lock_guard<std::mutex> lock(resultsMutex);
                results.append(threadResults);
                threadResults.clear();
                threadResults.reserve(1000);
            }

            // Update progress based on processed data size
            qint64 processed = totalProcessedSize.load();
            emit scanProgress(static_cast<int>((processed >> 20) & 0x7FFFFFFF)); // Convert to MB for progress
        }

        // Submit remaining results
        if (!threadResults.isEmpty()) {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results.append(threadResults);
        }
    };

    // Start parallel scanning
    QList<QFuture<void>> futures;
    for (int i = 0; i < maxThreads; ++i) {
        futures.append(QtConcurrent::run(&threadPool, [scanFunction, i]() { scanFunction(i); }));
    }

    // Wait for all threads to complete
    for (auto& future : futures) {
        future.waitForFinished();
    }

    if (!shouldStop) {
        // Sort results by size using parallel sort
        QtConcurrent::run(&threadPool, [&results]() {
            std::sort(results.begin(), results.end(),
                     [](const FileInfo& a, const FileInfo& b) { return a.size > b.size; });
        }).waitForFinished();

        emit scanComplete(results);
    }
}

void FileScanWorker::processBatch(const QVector<QPair<QString, QFileInfo>>& batch,
                                QList<FileInfo>& results,
                                QHash<QString, QString>& fileTypeCache,
                                std::atomic<qint64>& totalProcessedSize) {
    for (const auto& [filePath, fileInfo] : batch) {
        qint64 size = fileInfo.size();
        totalProcessedSize += size;

        if (size >= minimumSize) {
            FileInfo info;
            info.path = filePath;
            info.name = fileInfo.fileName();
            info.size = size;
            info.lastModified = fileInfo.lastModified();
            info.lastAccessed = fileInfo.lastRead();
            
            // Use cached file type if available
            QString ext = fileInfo.suffix().toLower();
            auto it = fileTypeCache.find(ext);
            if (it != fileTypeCache.end()) {
                info.fileType = it.value();
            } else {
                info.fileType = FileUtils::getFileType(filePath);
                fileTypeCache.insert(ext, info.fileType);
            }
            
            info.isDirectory = false;
            results.append(info);
        }
    }
}

void FileScanWorker::stop() {
    shouldStop = true;
} 
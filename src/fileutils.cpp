#include "fileutils.h"
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDebug>
#include <QDirIterator>
#include <QHash>
#include <mutex>

namespace FileUtils {

QString formatSize(qint64 bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = bytes;

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }

    return QString("%1 %2").arg(size, 0, 'f', 2).arg(units[unitIndex]);
}

// Cache for file extensions to MIME types
static QHash<QString, QString> mimeTypeCache;
static QMimeDatabase mimeDb;
static std::mutex mimeTypeMutex;

QString getFileType(const QString& path) {
    QFileInfo fileInfo(path);
    QString ext = fileInfo.suffix().toLower();
    
    if (ext.isEmpty()) {
        return "unknown";
    }

    // Check cache first
    {
        std::lock_guard<std::mutex> lock(mimeTypeMutex);
        auto it = mimeTypeCache.find(ext);
        if (it != mimeTypeCache.end()) {
            return it.value();
        }
    }

    // Not in cache, look it up
    QString mimeType = mimeDb.mimeTypeForFile(path).name();
    
    // Add to cache
    {
        std::lock_guard<std::mutex> lock(mimeTypeMutex);
        mimeTypeCache.insert(ext, mimeType);
    }

    return mimeType;
}

bool safeDelete(const QString& path) {
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) return false;

    if (fileInfo.isFile()) {
        return QFile::remove(path);
    } else if (fileInfo.isDir()) {
        QDir dir(path);
        return dir.removeRecursively();
    }
    return false;
}

qint64 calculateDirectorySize(const QString& path) {
    qint64 size = 0;
    QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                   QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        if (fileInfo.isFile()) {
            size += fileInfo.size();
        }
    }
    return size;
}

QString getFileIcon(const QString& path) {
    QFileInfo fileInfo(path);
    if (fileInfo.isDir()) {
        return "folder";
    }
    
    // Return appropriate icon based on mime type
    QString mimeType = getFileType(path);
    if (mimeType.startsWith("image/")) return "image";
    if (mimeType.startsWith("video/")) return "video";
    if (mimeType.startsWith("audio/")) return "audio";
    if (mimeType.startsWith("text/")) return "text";
    return "file";
}

bool isUselessFile(const QString& path, const QString& fileType) {
    // Define patterns for potentially useless files
    static const QStringList uselessPatterns = {
        ".DS_Store",
        "Thumbs.db",
        "desktop.ini",
        ".tmp",
        ".temp",
        ".cache",
        ".log"
    };

    QFileInfo fileInfo(path);
    QString fileName = fileInfo.fileName().toLower();

    // Check against known useless file patterns
    for (const QString& pattern : uselessPatterns) {
        if (fileName.endsWith(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }

    // Check for temporary or cache files
    if (fileType.contains("cache") || fileType.contains("temporary")) {
        return true;
    }

    return false;
}

} 
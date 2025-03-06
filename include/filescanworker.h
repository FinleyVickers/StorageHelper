#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QThread>
#include <QFileInfo>
#include <QVector>
#include <QPair>
#include <QHash>
#include <queue>
#include <mutex>
#include <atomic>

struct FileInfo {
    QString path;
    QString name;
    qint64 size;
    QDateTime lastModified;
    QDateTime lastAccessed;
    QString fileType;
    bool isDirectory;

    bool operator<(const FileInfo& other) const {
        return size > other.size; // Sort by size descending
    }
};

class FileScanWorker : public QObject {
    Q_OBJECT

public:
    explicit FileScanWorker(QObject *parent = nullptr);

public slots:
    void startScan(const QString& directory, qint64 minSize = 0);
    void stop();

signals:
    void scanProgress(int percentage);
    void scanComplete(const QList<FileInfo>& files);
    void error(const QString& message);

private:
    void processBatch(const QVector<QPair<QString, QFileInfo>>& batch,
                     QList<FileInfo>& results,
                     QHash<QString, QString>& fileTypeCache,
                     std::atomic<qint64>& totalProcessedSize);

    bool shouldStop;
    qint64 minimumSize;
}; 
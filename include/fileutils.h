#pragma once

#include <QString>
#include <QFileInfo>
#include "filescanworker.h"

namespace FileUtils {
    QString formatSize(qint64 bytes);
    QString getFileType(const QString& path);
    bool safeDelete(const QString& path);
    qint64 calculateDirectorySize(const QString& path);
    QString getFileIcon(const QString& path);
    bool isUselessFile(const QString& path, const QString& fileType);
} 
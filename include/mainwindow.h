#pragma once

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTreeView>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include "filescanworker.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void handleSelectDirectory();
    void handleStartScan();
    void handleScanProgress(int progress);
    void handleScanComplete(const QList<FileInfo>& files);
    void handleDeleteSelected();
    void handleOpenFileLocation();
    void handleFilterChanged();
    void handleError(const QString& message);

private:
    void setupUi();
    void setupConnections();
    void updateFileList(const QList<FileInfo>& files);
    void updateStatusBar();
    QString formatSize(qint64 size) const;

    Ui::MainWindow *ui;
    QFileSystemModel *fileSystemModel;
    FileScanWorker *scanWorker;
    QString currentDirectory;
    QList<FileInfo> currentFiles;

    // UI Elements
    QTreeView *fileTreeView;
    QPushButton *selectDirButton;
    QPushButton *startScanButton;
    QPushButton *deleteButton;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QComboBox *sizeFilterCombo;
    QSpinBox *minSizeSpinBox;
    QComboBox *fileTypeFilter;
}; 
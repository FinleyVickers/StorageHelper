#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fileutils.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QDesktopServices>
#include <QUrl>
#include <QThread>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // Create left side widget for file list
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    // Move existing widgets to layout
    layout->addWidget(ui->selectDirButton);
    layout->addWidget(ui->sizeFilterCombo);
    layout->addWidget(ui->minSizeSpinBox);
    layout->addWidget(ui->fileTypeFilter);
    layout->addWidget(ui->startScanButton);
    layout->addWidget(ui->fileTreeView);
    layout->addWidget(ui->progressBar);
    layout->addWidget(ui->deleteButton);
    layout->addWidget(ui->openLocationButton);
    layout->addWidget(ui->statusLabel);

    setCentralWidget(centralWidget);

    setupUi();

    // Create worker thread
    QThread* workerThread = new QThread(this);
    scanWorker = new FileScanWorker;
    scanWorker->moveToThread(workerThread);
    
    // Connect signals and slots
    setupConnections();
    
    // Start the worker thread
    workerThread->start();
}

MainWindow::~MainWindow() {
    if (scanWorker) {
        scanWorker->stop();
        scanWorker->thread()->quit();
        scanWorker->thread()->wait();
        delete scanWorker;
    }
    delete ui;
}

void MainWindow::setupUi() {
    // Setup the tree view model
    QStandardItemModel* model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({"Name", "Size", "Type", "Last Modified", "Path"});
    ui->fileTreeView->setModel(model);

    // Hide the size spinbox initially
    ui->minSizeSpinBox->setVisible(false);

    // Set initial button states
    ui->deleteButton->setEnabled(false);
    ui->openLocationButton->setEnabled(false);
    ui->startScanButton->setEnabled(false);

    // Set column widths
    ui->fileTreeView->setColumnWidth(0, 200);  // Name
    ui->fileTreeView->setColumnWidth(1, 100);  // Size
    ui->fileTreeView->setColumnWidth(2, 100);  // Type
    ui->fileTreeView->setColumnWidth(3, 150);  // Last Modified

    // Set minimum size for the window
    setMinimumSize(1200, 800);
}

void MainWindow::setupConnections() {
    connect(ui->selectDirButton, &QPushButton::clicked, this, &MainWindow::handleSelectDirectory);
    connect(ui->startScanButton, &QPushButton::clicked, this, &MainWindow::handleStartScan);
    connect(ui->deleteButton, &QPushButton::clicked, this, &MainWindow::handleDeleteSelected);
    connect(ui->openLocationButton, &QPushButton::clicked, this, &MainWindow::handleOpenFileLocation);
    connect(ui->sizeFilterCombo, &QComboBox::currentTextChanged, this, &MainWindow::handleFilterChanged);
    connect(ui->fileTypeFilter, &QComboBox::currentTextChanged, this, &MainWindow::handleFilterChanged);
    
    connect(scanWorker, &FileScanWorker::scanProgress, this, &MainWindow::handleScanProgress);
    connect(scanWorker, &FileScanWorker::scanComplete, this, &MainWindow::handleScanComplete);
    connect(scanWorker, &FileScanWorker::error, this, &MainWindow::handleError);

    connect(ui->fileTreeView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &selected, const QItemSelection &deselected) {
        bool hasSelection = !ui->fileTreeView->selectionModel()->selectedRows().isEmpty();
        ui->deleteButton->setEnabled(hasSelection);
        ui->openLocationButton->setEnabled(hasSelection);
    });

    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::handleSelectDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory",
                                                  QDir::homePath(),
                                                  QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        currentDirectory = dir;
        ui->startScanButton->setEnabled(true);
        ui->statusLabel->setText("Ready to scan: " + dir);
    }
}

void MainWindow::handleStartScan() {
    if (currentDirectory.isEmpty()) return;

    // Clear previous results
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->fileTreeView->model());
    model->removeRows(0, model->rowCount());

    // Disable UI elements during scan
    ui->startScanButton->setEnabled(false);
    ui->selectDirButton->setEnabled(false);
    ui->progressBar->setValue(0);
    ui->statusLabel->setText("Scanning...");

    // Start the scan
    qint64 minSize = 0;
    if (ui->sizeFilterCombo->currentText() == "Larger than...") {
        minSize = ui->minSizeSpinBox->value() * 1024 * 1024; // Convert MB to bytes
    }

    QMetaObject::invokeMethod(scanWorker, "startScan",
                             Q_ARG(QString, currentDirectory),
                             Q_ARG(qint64, minSize));
}

void MainWindow::handleScanProgress(int progress) {
    ui->progressBar->setValue(progress);
}

void MainWindow::handleScanComplete(const QList<FileInfo>& files) {
    currentFiles = files;
    updateFileList(files);
    
    // Re-enable UI elements
    ui->startScanButton->setEnabled(true);
    ui->selectDirButton->setEnabled(true);
    ui->statusLabel->setText(QString("Found %1 files").arg(files.size()));
}

void MainWindow::handleDeleteSelected() {
    QModelIndexList selected = ui->fileTreeView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Delete",
        QString("Are you sure you want to delete %1 file(s)?").arg(selected.count()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->fileTreeView->model());
        QList<QPersistentModelIndex> persistentIndices;
        qint64 totalFreed = 0;

        // Convert to persistent indices as we'll be modifying the model
        for (const QModelIndex& index : selected) {
            persistentIndices << QPersistentModelIndex(index);
            QString filePath = model->data(model->index(index.row(), 4)).toString();
            QString sizeStr = model->data(model->index(index.row(), 1)).toString();
            
            if (FileUtils::safeDelete(filePath)) {
                // Approximate size calculation from the displayed string
                // This is not exact but good enough for the status message
                double size = sizeStr.split(" ")[0].toDouble();
                QString unit = sizeStr.split(" ")[1];
                if (unit == "KB") size *= 1024;
                else if (unit == "MB") size *= 1024 * 1024;
                else if (unit == "GB") size *= 1024 * 1024 * 1024;
                totalFreed += static_cast<qint64>(size);
            }
        }

        // Remove the rows from the model
        for (const QPersistentModelIndex& index : persistentIndices) {
            if (index.isValid()) {
                model->removeRow(index.row());
            }
        }

        ui->statusLabel->setText(QString("Freed %1").arg(FileUtils::formatSize(totalFreed)));
    }
}

void MainWindow::handleOpenFileLocation() {
    QModelIndexList selected = ui->fileTreeView->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QString filePath = ui->fileTreeView->model()->data(
        ui->fileTreeView->model()->index(selected.first().row(), 4)).toString();
    
    QFileInfo fileInfo(filePath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.dir().path()));
}

void MainWindow::handleFilterChanged() {
    ui->minSizeSpinBox->setVisible(ui->sizeFilterCombo->currentText() == "Larger than...");
    
    // If we have results, reapply the filter
    if (ui->fileTreeView->model()->rowCount() > 0) {
        handleStartScan();
    }
}

void MainWindow::handleError(const QString& message) {
    QMessageBox::warning(this, "Error", message);
    ui->startScanButton->setEnabled(true);
    ui->selectDirButton->setEnabled(true);
    ui->statusLabel->setText("Error occurred during scan");
}

void MainWindow::updateFileList(const QList<FileInfo>& files) {
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->fileTreeView->model());
    
    // Disable sorting temporarily for better performance
    ui->fileTreeView->setSortingEnabled(false);
    
    // Clear the model
    model->removeRows(0, model->rowCount());

    // Pre-allocate the rows
    QList<QList<QStandardItem*>> allRows;
    allRows.reserve(files.size());

    // Create all items without adding them to the model yet
    for (const FileInfo& file : files) {
        QList<QStandardItem*> row;
        row.reserve(5);  // We know we'll have 5 columns
        
        // Name
        row.append(new QStandardItem(file.name));
        
        // Size - store raw size as user data for proper sorting
        QStandardItem* sizeItem = new QStandardItem(FileUtils::formatSize(file.size));
        sizeItem->setData(file.size, Qt::UserRole);
        row.append(sizeItem);
        
        // Type
        row.append(new QStandardItem(file.isDirectory ? "Directory" : file.fileType));
        
        // Last Modified - store raw date as user data for proper sorting
        QStandardItem* dateItem = new QStandardItem(file.lastModified.toString("yyyy-MM-dd hh:mm:ss"));
        dateItem->setData(file.lastModified, Qt::UserRole);
        row.append(dateItem);
        
        // Path
        row.append(new QStandardItem(file.path));

        allRows.append(row);
    }

    // Add all rows at once
    model->insertRows(0, allRows.size());
    for (int i = 0; i < allRows.size(); ++i) {
        for (int j = 0; j < allRows[i].size(); ++j) {
            model->setItem(i, j, allRows[i][j]);
        }
    }

    // Re-enable sorting and sort by size
    ui->fileTreeView->setSortingEnabled(true);
    
    // Set sort role to use the raw values stored in UserRole
    model->setSortRole(Qt::UserRole);
    ui->fileTreeView->sortByColumn(1, Qt::DescendingOrder);
} 
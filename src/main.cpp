#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Storage Helper");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("StorageHelper");
    
    MainWindow window;
    window.resize(1024, 768);
    window.show();
    
    return app.exec();
} 
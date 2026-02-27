#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("LogViewer");
    QCoreApplication::setOrganizationDomain("local");
    QCoreApplication::setApplicationName("LogViewer");

    MainWindow w;
    w.show();

    return a.exec();
}

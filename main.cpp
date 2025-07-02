#include <QApplication>
#include <QStyleFactory>
#include <QImageReader>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create(QStringLiteral(u"Fusion")));

    QCoreApplication::setOrganizationName(QStringLiteral("ImgView"));
    QCoreApplication::setApplicationName(QStringLiteral("ImgView"));
    QImageReader::setAllocationLimit(1024 * 1024 * 1024);

    QStringList files = QCoreApplication::arguments();
    files.pop_front();
    MainWindow mainwindow(files);
    mainwindow.show();

    return app.exec();
}

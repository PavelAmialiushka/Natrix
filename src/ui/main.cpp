#include "mainWindow.h"
#include <QApplication>

#include "version.h"

#include <QDir>
#include <QTimer>

int main(int argc, char *argv[])
{
    QFileInfo exefile(argv[0]);
    QDir      dir = exefile.absoluteDir();

    QString prepend;
    if(dir.exists())
        prepend = dir.absolutePath() + "/";

    QStringList paths;
    paths.append(prepend + ".");
    paths.append(prepend + "imageformats");
    paths.append(prepend + "platforms");
    paths.append(prepend + "sqldrivers");

    // если работаем из/под рабочей директории, то исльзуем пути QT
    if(!dir.exists("platforms"))
        paths = QCoreApplication::libraryPaths();

    QCoreApplication::setLibraryPaths(paths);

    QApplication a(argc, argv);

    QString version = VER_PRODUCTVERSION_STR;
    a.setApplicationVersion(version);

    MainWindow w;

    if(!w.readCommandLine())
        return 0;

    return a.exec();
}

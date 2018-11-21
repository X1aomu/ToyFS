#include "gui/mainwindow.h"
#include <QApplication>

#include "disk.h"
#include "filesystem.h"
#include <iostream>
using namespace std;
int main(int argc, char *argv[])
{
    cerr << "Create disk: " << Disk::CreateDisk("disk.dat") << endl;

    Disk d("disk.dat");
    cerr << "Valid: " << d.isValid() << endl;

    FileSystem fs(d);
    cerr << "Init FS: " << fs.initFileSystem() << endl;;

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-file-manager"), app.windowIcon()));

    MainWindow w;
    w.show();


    return app.exec();
}

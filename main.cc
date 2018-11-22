#include "gui/mainwindow.h"
#include <QApplication>

#include "disk.h"
#include "filesystem.h"
#include <iostream>
#include <cassert>
using namespace std;
int main(int argc, char *argv[])
{
    assert(Disk::CreateDisk("test.disk"));

    Disk d("test.disk");
    assert(d.isValid());

    FileSystem fs(d);
    assert(fs.initFileSystem());

    assert(fs.exist("/"));
    assert(fs.rootEntry()->name() == "/");
    assert(fs.rootEntry()->parent() == fs.rootEntry());
    assert(fs.getEntry("/") == fs.rootEntry());

    assert(fs.exist("/d1") == false);

    assert(fs.createDir("/d1"));
    assert(fs.createDir("/d1") == false);
    assert(fs.createFile("/f1", FileSystem::ReadOnly) == false);
    assert(fs.createFile("/f1", FileSystem::Directory) == false);
    assert(fs.createFile("/f1", FileSystem::File));
    assert(fs.createFile("/f1", FileSystem::File) == false);
    assert(fs.createFile("/f2", FileSystem::File));
    assert(fs.createFile("/f3", FileSystem::File));
    assert(fs.createFile("/f4", FileSystem::File | FileSystem::System));
    assert(fs.createDir("/d2"));
    assert(fs.createDir("/d3"));
    assert(fs.createDir("/d4"));
    assert(fs.createDir("/d5") == false);
    assert(fs.createFile("/f5", FileSystem::File) == false);

    assert(fs.createFile("/d1/f5", FileSystem::File));
    assert(fs.createFile("/d1/f6", FileSystem::File));
    assert(fs.createFile("/f1/f7", FileSystem::File) == false); // 父目录不存在(不是目录) 这里巨坑
    assert(fs.createFile("/d/f7", FileSystem::File) == false); // 父目录不存在
    assert(fs.createFile("/d1/f777", FileSystem::File));
    assert(fs.createFile("/d1/f8888", FileSystem::File) == false); // 文件名长度过长


    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-file-manager"), app.windowIcon()));

    MainWindow w;
    w.show();


    return app.exec();
}

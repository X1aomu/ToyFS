#include "gui/mainwindow.h"
#include <QApplication>

#include "disk.h"
#include "filesystem.h"
#include <iostream>
#include <cassert>
#include <algorithm>
using namespace std;
int main(int argc, char *argv[])
{
    assert(Disk::CreateDisk("test.disk"));

    Disk d("test.disk");
    assert(d.isValid());

    FileSystem fs(d);
    assert(fs.initFileSystem());

    // prerequisites
    assert(fs.exist("/"));
    assert(fs.rootEntry()->name() == "/");
    assert(fs.rootEntry()->fullpath() == "/");
    assert(fs.rootEntry()->isDir() == true);
    assert(fs.rootEntry()->isSystem() == true);
    assert(fs.rootEntry()->parent() == fs.rootEntry());
    assert(fs.rootEntry()->getChildren().empty());
    assert(fs.getEntry("/") == fs.rootEntry());
    assert(fs.exist("/d1") == false);

    // create dirs and files
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
    assert(fs.createDir("/d5") == false); // 前面目录项已满
    assert(fs.createFile("/f5", FileSystem::File) == false);
    assert(fs.createFile("/d1/f5", FileSystem::File));
    assert(fs.createFile("/d1/f6", FileSystem::File));
    assert(fs.createFile("/f1/f7", FileSystem::File) == false); // 父目录不存在(不是目录) 这里巨坑
    assert(fs.createFile("/d/f7", FileSystem::File) == false); // 父目录不存在
    assert(fs.createFile("/d1/f777", FileSystem::File));
    assert(fs.createFile("/d1/f8888", FileSystem::File) == false); // 文件名长度过长

    // test Entry
    assert(fs.rootEntry()->fullpath() == "/");
    auto f2Entry = fs.getEntry("/f2");
    assert(f2Entry->fullpath() == "/f2");
    assert(fs.getEntry("/d1/f5")->fullpath() == "/d1/f5");

    // open and close
    // five files was opened before
    list<string> openedFileList = fs.getOpenedFileList();
    assert(std::find(openedFileList.begin(), openedFileList.end(), "/f1") != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), "/f2") != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), "/f3") != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), "/f4") != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), "/d1/f5") != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), "/f5") != openedFileList.end() == false);
    assert(std::find(openedFileList.begin(), openedFileList.end(), "/d1/f6") != openedFileList.end() == false);
    assert(fs.openFile("/d1/f6", FileSystem::ReadOnly) == false); // should not be able to open more file
    assert(fs.openFile("/f1", FileSystem::Read | FileSystem::Write)); // reopen
    assert(fs.openFile("/d1/f5", FileSystem::Read | FileSystem::Write)); // reopen
    assert(fs.closeFile("/f1"));
    assert(fs.closeFile("/f2"));
    assert(fs.closeFile("/f3"));
    assert(fs.closeFile("/f4"));
    assert(fs.closeFile("/d1/f5"));
    assert(fs.setFileAttributes("/d1/f6", (fs.getEntry("/d1/f6"))->attributes() | FileSystem::ReadOnly));
    assert(fs.openFile("/d1/f6", FileSystem::Read | FileSystem::Write) == false); // 不能以写方式打开只读文件
    assert(fs.openFile("/d1/f6", FileSystem::Read));
    assert(fs.closeFile("/d1/f6"));

    // set file attribute
    fs.openFile("/f1", FileSystem::Read);
    assert(fs.setFileAttributes("/f1", FileSystem::File | FileSystem::ReadOnly) == false); // 文件已打开，不能改属性
    fs.closeFile("/f1");
    assert(fs.setFileAttributes("/f1", FileSystem::File | FileSystem::ReadOnly));
    assert(fs.setFileAttributes(f2Entry->fullpath(), f2Entry->attributes() | FileSystem::ReadOnly));
    assert(fs.setFileAttributes("/f1", FileSystem::Directory) == false);
    assert(fs.setFileAttributes("/f2", 0) == false); // 不能删掉文件属性
    assert(fs.setFileAttributes("/f1", FileSystem::Directory | FileSystem::File));

    // delete entry
    assert(fs.deleteEntry("/d4"));
    assert(fs.deleteEntry("/d3"));
    assert(fs.deleteEntry("/d2"));
    assert(fs.deleteEntry("/d1") == false); // /d1 不空
    assert(fs.deleteEntry("/d1/f5"));
    assert(fs.deleteEntry("/d1/f6"));
    assert(fs.deleteEntry("/d1/f777"));
    assert(fs.deleteEntry("/d1/f") == false); // 不存在
    assert(fs.deleteEntry("/d1"));
    assert(fs.deleteEntry("/f1"));
    assert(fs.deleteEntry("/f2"));
    assert(fs.deleteEntry("/f3"));
    assert(fs.deleteEntry("/f4"));
    assert(fs.deleteEntry("/f5") == false); // 不存在
    assert(fs.deleteEntry("f5") == false); // 不存在

    // at last, everything is gone
    assert(fs.rootEntry()->getChildren().empty());

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-file-manager"), app.windowIcon()));

    MainWindow w;
    w.show();


    return app.exec();
}

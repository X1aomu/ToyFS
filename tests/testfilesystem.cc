#include "disk.h"
#include "filesystem.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;

int main()
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

    string d1 = "/d1";
    string d2 = "/d2";
    string d3 = "/d3";
    string d4 = "/d4";
    string f1 = "/f1";
    string f2 = "/f2";
    string f3 = "/f3";
    string f4 = "/f4";
    string f5 = "/d1/f5";
    string f6 = "/d1/f6";
    string f7 = "/d1/f777";

    // create dirs and files
    assert(fs.createDir(d1));
    assert(fs.createDir(d1) == false);                        // 不能创建同名目录
    assert(fs.createFile(f1, FileSystem::ReadOnly) == false); // 不能创建只读文件
    assert(fs.createFile(f1, FileSystem::Directory) == false);
    assert(fs.createFile(f1, FileSystem::File));
    assert(fs.createFile(f1, FileSystem::File) == false); // 不能创建同名文件
    assert(fs.createFile(f2, FileSystem::File));
    assert(fs.createFile(f3, FileSystem::File));
    assert(fs.createFile(f4, FileSystem::File | FileSystem::System));
    assert(fs.createDir(d2));
    assert(fs.createDir(d3));
    assert(fs.createDir(d4));
    assert(fs.createDir("/d5") == false); // 前面目录项已满
    assert(fs.createFile("/f5", FileSystem::File) == false);
    assert(fs.createFile(f5, FileSystem::File));
    assert(fs.createFile(f6, FileSystem::File));
    assert(fs.createFile("/f1/f7", FileSystem::File) == false); // 父目录不存在(不是目录) 这里巨坑
    assert(fs.createFile("/d/f7", FileSystem::File) == false);  // 父目录不存在
    assert(fs.createFile(f7, FileSystem::File));
    assert(fs.createFile("/d1/f8888", FileSystem::File) == false); // 文件名长度过长

    // test Entry
    assert(fs.rootEntry()->fullpath() == "/");
    auto f2Entry = fs.getEntry(f2);
    assert(f2Entry->fullpath() == f2);
    assert(fs.getEntry(f5)->fullpath() == f5);

    // open and close
    // five files was opened before
    vector<string> openedFileList = fs.getOpenedFiles();
    assert(std::find(openedFileList.begin(), openedFileList.end(), f1) != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), f2) != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), f3) != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), f4) != openedFileList.end());
    assert(std::find(openedFileList.begin(), openedFileList.end(), f5) != openedFileList.end());
    assert((std::find(openedFileList.begin(), openedFileList.end(), "/f5") != openedFileList.end()) == false);
    assert((std::find(openedFileList.begin(), openedFileList.end(), f6) != openedFileList.end()) == false);
    assert(fs.openFile(f6, FileSystem::ReadOnly) == false);        // should not be able to open more file
    assert(fs.openFile(f1, FileSystem::Read | FileSystem::Write)); // reopen
    assert(fs.openFile(f5, FileSystem::Read | FileSystem::Write)); // reopen
    assert(fs.closeFile(f1));
    assert(fs.closeFile(f2));
    assert(fs.closeFile(f3));
    assert(fs.closeFile(f4));
    assert(fs.closeFile(f5));
    assert(fs.setFileAttributes(f6, (fs.getEntry(f6))->attributes() | FileSystem::ReadOnly));
    assert(fs.openFile(f6, FileSystem::Read | FileSystem::Write) == false); // 不能以写方式打开只读文件
    assert(fs.openFile(f6, FileSystem::Read));
    assert(fs.closeFile(f6));

    // set file attribute
    fs.openFile(f1, FileSystem::Read);
    assert(fs.setFileAttributes(f1, FileSystem::File | FileSystem::ReadOnly) == false); // 文件已打开，不能改属性
    fs.closeFile(f1);
    assert(fs.setFileAttributes(f1, FileSystem::File | FileSystem::ReadOnly));
    assert(fs.setFileAttributes(f2Entry->fullpath(), f2Entry->attributes() | FileSystem::ReadOnly));
    assert(fs.setFileAttributes(f1, FileSystem::Directory) == false);
    assert(fs.setFileAttributes(f2, 0) == false);                               // 不能删掉文件属性
    assert(fs.setFileAttributes(f1, FileSystem::Directory | FileSystem::File)); // 目录属性应该被忽略掉

    // read and write
    char* datain = new char[1024];
    char* dataout = new char[1024];
    for (int i = 0; i != 1024; ++i)
    {
        dataout[i] = 'X';
    }
    assert(fs.readFile(f2, datain, 100) == 0);
    assert(fs.closeFile(f2));                      // 文件被打开了，但是没有内容，所以能够正常关闭
    assert(fs.writeFile(f2, dataout, 1) == false); // 只读文件，不可写
    assert(fs.isOpened(f2) == false);              // 上个操作失败，文件不应处于打开状态

    assert(fs.writeFile(f3, dataout, 63)); // 写入 63 字节，总大小刚好不超过一个块
    assert(fs.getEntry(f3)->size() == 64);
    assert(fs.readFile(f3, datain, 32) == 32);
    assert(fs.readFile(f3, datain, 32) == 31); // 已读到文件尾
    assert(fs.readFile(f3, datain, 1) == 0);   // 已读到文件尾
    assert(fs.writeFile(f3, dataout, 1));      // 继续追加数据
    assert(fs.getEntry(f3)->size() == 64 * 2); // 大小应为两个块大小
    assert(fs.writeFile(f3, dataout, 64));     // 继续追加数据
    assert(fs.getEntry(f3)->size() == 64 * 3); // 大小应为三个块大小
    assert(fs.closeFile(f3));
    assert(fs.readFile(f3, datain, 64 * 2) == 64 * 2); // 关闭文件后重新读取（重置读取指针）
    assert(fs.readFile(f3, datain, 1) == 0);           // 已读到文件尾
    assert(fs.closeFile(f3));

    assert(fs.openFile(f2, FileSystem::Read));
    assert(fs.writeFile(f2, dataout, 1) == false); // 不能写入不以写方式打开的文件
    assert(fs.closeFile(f2));
    assert(fs.openFile(f4, FileSystem::Write));
    assert(fs.readFile(f4, dataout, 1) == false); // 不能读取不以读方式打开的文件
    assert(fs.closeFile(f4));

    delete[] datain;
    delete[] dataout;

    // delete entry
    assert(fs.deleteEntry(d4));
    assert(fs.deleteEntry(d3));
    assert(fs.deleteEntry(d2));
    assert(fs.deleteEntry(d1) == false); // /d1 不空
    assert(fs.deleteEntry(f5));
    assert(fs.deleteEntry(f6));
    assert(fs.deleteEntry(f7));
    assert(fs.deleteEntry("/d1/f") == false); // 不存在
    assert(fs.deleteEntry(d1));
    assert(fs.deleteEntry(f1));
    assert(fs.deleteEntry(f2));
    assert(fs.deleteEntry(f3));
    assert(fs.deleteEntry(f4));
    assert(fs.deleteEntry("/f5") == false); // 不存在
    assert(fs.deleteEntry("f5") == false);  // 不存在

    // at last, everything is gone
    assert(fs.rootEntry()->getChildren().empty());

    cout << "All tests pass!" << endl;

    return 0;
}

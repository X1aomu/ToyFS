//===-- filesystem.h - FileSystem class definition ------------------------===//
//
// The Toy FAT FileSystem
//
//===----------------------------------------------------------------------===//
///
/// \file
/// File description
///
//===----------------------------------------------------------------------===//
#ifndef TOYFS_FILESYSTEM_H_
#define TOYFS_FILESYSTEM_H_

#include "disk.h"

#include <string>
#include <vector>
#include <memory>

class Entry;

class FileSystem
{
public:
    static const int kBlockSize = 64; // 块大小，本程序为了简便等于磁盘扇区大小
    static const int kFatEntrySize = 1; // 每个 FAT 条目的大小（字节）
    static const int kMaxOpenedFiles = 5;

    enum Attribute {
        ReadOnly = 1,
        System = 2,
        File = 4,
        Directory = 8
    };
    using Attributes = int;

    enum OpenMode {
        Read = 1,
        Write = 2
    };
    using OpenModes = int;

    // constructors & destructor
    explicit FileSystem(Disk& disk);
    ~FileSystem();
    // keep from copying
    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    bool initFileSystem();

    std::shared_ptr<Entry> rootEntry();

    bool exist(const std::string& path);
    std::shared_ptr<Entry> getEntry(const std::string& path);

    bool createFile(const std::string& fullPath, Attributes attributes);
    bool openFile(const std::string& fullPath, OpenModes openModes);
    std::unique_ptr<std::string> readFile(const std::string& fullPath, int length);
    int writeFile(const std::string& fullPath, char* buffer, int length);
    bool closeFile(const std::string& fullPath);

    bool createDir(const std::string& fullPath);
    bool removeDir(const std::string& fullPath);
    std::shared_ptr<Entry> listEntries(const std::string& fullpath);

private:
    struct OpenedFile {
        std::string path;
        Attributes attributes;
        int blockNumber;
        int length;
        OpenModes modes;
        int g; // get pointer
        int p; // put pointer
    };

    const int kFatSize; // FAT 大小
    const int kNumOfFatBlocks; // FAT 占用的块数
    const int kRootBlockNumber; // 根目录起始块地址

    Disk& m_disk;
    char* fat;
    char* buffer;
    std::shared_ptr<Entry> m_rootEntry;
    std::array<OpenedFile, kMaxOpenedFiles> m_openedFiles; // true 表示这个位置的文件描述符已被点用

    bool loadFat();
    bool saveFat();
};

class Entry : public std::enable_shared_from_this<Entry>
{
public:
    bool isPathValid();
    bool isDir();
    bool isReadOnly();
    bool isSystem();

    std::string name();
    std::string fullpath();

    std::weak_ptr<Entry> self();
    std::weak_ptr<Entry> parent();
    std::vector<std::weak_ptr<Entry>> children();

    std::weak_ptr<Entry> addChild(const std::string& name);

private:
    std::string m_name;
    FileSystem::Attributes m_attrbutes;
    int m_blockStart;
    int m_numBlock;
};

#endif // TOYFS_FILESYSTEM_H_

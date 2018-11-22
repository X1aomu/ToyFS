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
    static const int kRawFileNameLength = 5;

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
    // 文件描述符
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
    std::array<OpenedFile, kMaxOpenedFiles> m_openedFiles;
    std::array<bool, kMaxOpenedFiles> m_openedFilesStat; // true 表示这个位置的文件描述符已被点用

    bool loadFat();
    bool saveFat();

    static bool checkName(const std::string& name) { return name.length() > 0 && name.length() < kRawFileNameLength; }

    friend class Entry;
};

class Entry : public std::enable_shared_from_this<Entry>
{
public:
    Entry(Disk& disk) : m_disk(disk) {}

    //bool isPathValid();
    bool isDir() { return m_attrbutes & FileSystem::Directory; }
    bool isReadOnly() { return m_attrbutes & FileSystem::ReadOnly; }
    bool isSystem() { return m_attrbutes & FileSystem::System; }

    std::string name() { return m_name; }
    std::string fullpath();
    std::shared_ptr<Entry> self() { return shared_from_this(); }
    std::shared_ptr<Entry> parent() { return m_parent; }
    /**
     * @brief getChildren
     * @return 如果是目录则为子项目列表，否刚返回空列表。
     */
    std::vector<std::shared_ptr<Entry>> getChildren();
    /**
     * @brief findChild
     * @param name 子项名。
     * @return 如果子项存在，返回子项，否则返回自身。
     */
    std::shared_ptr<Entry> findChild(const std::string& name);
//    std::weak_ptr<Entry> addChild(const std::string& name);

private:
    Disk& m_disk;
    std::string m_name;
    FileSystem::Attributes m_attrbutes;
    int m_blockStart;
    int m_numBlock;
    std::shared_ptr<Entry> m_parent;

    friend class FileSystem;
};

inline std::string Entry::fullpath()
{
    if (parent() == self())
    {
        return name();
    }
    return parent()->fullpath() + "/" + name();
}

#endif // TOYFS_FILESYSTEM_H_

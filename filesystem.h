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
#include <list>
#include <mutex>
#include <unordered_map>
#include <type_traits>

class Entry;

class FileSystem
{
public:
    static const int kBlockSize = 64; // 块大小，本程序为了简便等于磁盘扇区大小
    static const int kEntrySize = 8; // 目录项大小
    static const int kMaxChildEntries = 8; // 一个目录最大的目录项数
    static const int kMaxOpenedFiles = 5;
    static const int kRawFileNameLength = 5;
    static const int END_OF_FILE = '#';
    static_assert (kEntrySize * kMaxChildEntries == kBlockSize, "Mismatch constants.");

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
    std::shared_ptr<Entry> getEntry(const std::string& fullPath);
    bool exist(const std::string& fullPath);

    bool createDir(const std::string& fullPath);
    bool createFile(const std::string& fullPath, Attributes attributes);
    bool openFile(const std::string& fullPath, OpenModes openModes);
    bool closeFile(const std::string& fullPath);
    bool isOpened(const std::string& fullPath);
    std::vector<std::string> getOpenedFiles();
    std::unique_ptr<std::string> readFile(const std::string& fullPath, int length);
    int readFile(const std::string& fullPath, char* buf_out, int length);
    bool writeFile(const std::string& fullPath, char* buf_in, int length);
    bool setFileAttributes(const std::string& fullPath, Attributes attributes);

    bool deleteEntry(const std::string& fullPath);
    bool deleteEntry(std::shared_ptr<Entry> entry);

    //std::shared_ptr<Entry> listEntries(const std::string& fullpath);

    bool sync();

private:
    // 文件描述符
    struct OpenedFile {
        std::string fullPath;
        Attributes attributes;
        int blockNumber;
        int numOfBlocks;
        OpenModes modes;
        int g; // get pointer
        int p; // put pointer
    };

    static const int kEntryAttributesIndex = 5;
    static const int kEntryBlockStartIndex = 6;
    static const int kEntryNumOfBlocksIndex = 7;

    const int kFatSize; // FAT 大小
    const int kNumOfFatBlocks; // FAT 占用的块数
    const int kRootBlockNumber; // 根目录起始块地址

    Disk& m_disk;
    char* m_fat;
    char* m_buffer;
    std::shared_ptr<Entry> m_rootEntry;
    std::unordered_map<std::string, std::shared_ptr<OpenedFile>> m_openedFiles;

    // 互斥锁
    // 注意：如需占用多个锁，请按顺序加锁
    std::mutex m_mutex1Fat;
    std::mutex m_mutex2Buffer;

    // FAT 相关函数
    bool loadFat();
    bool saveFat();
    /**
     * @brief nextAvailableBlock
     * @return 如果有可用块则为可用块号，否则为 -1。
     */
    int nextAvailableBlock();
    /**
     * @brief findNextNBlock
     * @param firstBlock
     * @param n
     * @return firstBlock 之后 n 块的块号，如果结果不存在，返回 -1。
     */
    int findNextNBlock(int firstBlock, int n);

    // 实用函数
    static std::string getNameFromEntryPointer(char* p);
    static char* findChildEntryPointer(char* parentEntryPointer, const std::string& childName);
    static bool checkName(const std::string& name);
    static std::list<std::string> splitPath(const std::string& fullpath);

    friend class Entry;
};

class Entry : public std::enable_shared_from_this<Entry>
{
public:
    Entry(Disk& disk) : m_disk(disk) {}

    //bool isPathValid();
    bool isDir() { return m_attributes & FileSystem::Directory; }
    bool isReadOnly() { return m_attributes & FileSystem::ReadOnly; }
    bool isSystem() { return m_attributes & FileSystem::System; }
    FileSystem::Attributes attributes() { return m_attributes; }

    std::string name() { return m_name; }
    std::string fullpath();
    std::shared_ptr<Entry> self() { return shared_from_this(); }
    std::shared_ptr<Entry> parent() { return m_parent; }
    int size() { return m_numBlock * FileSystem::kBlockSize; }

    /**
     * @brief getChildren
     * @return 如果是目录则为子项目列表，否刚返回空列表。
     */
    std::vector<std::shared_ptr<Entry>> getChildren();
    /**
     * @brief findChild
     * @param name 子项名。
     * @return 如果子项存在，返回子项，否则返回 null。
     */
    std::shared_ptr<Entry> findChild(const std::string& name);
//    std::weak_ptr<Entry> addChild(const std::string& name);

private:
    Disk& m_disk;
    std::string m_name;
    FileSystem::Attributes m_attributes;
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
    auto fullPath = parent()->fullpath() + "/" + name();
    return fullPath.substr(fullPath.find_first_not_of('/') - 1);
}

#endif // TOYFS_FILESYSTEM_H_

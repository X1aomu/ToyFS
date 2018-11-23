#include "filesystem.h"

#include "disk.h"

#include <algorithm>
#include <iterator>
#include <iostream>
#include <sstream>
#include <list>
#include <mutex>
#include <cassert>

FileSystem::FileSystem(Disk &disk)
    : kFatSize(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize),
      kNumOfFatBlocks(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize / kBlockSize),
      kRootBlockNumber(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize / kBlockSize),
      m_disk(disk)
{
    assert(kFatSize / kBlockSize == kNumOfFatBlocks);
    assert(kRootBlockNumber == kNumOfFatBlocks);

    // FAT
    m_fat = new char[kFatSize];
    std::for_each(m_fat, m_fat + kFatSize, [](char &e){
        e = 0;
    });

    // buffer
    m_buffer = new char[Disk::kSectorSize];

    // load FAT
    bool succeeded = loadFat();
    if (!succeeded)
    {
        std::cerr << "Fatal: cannot load FAT from disk." << std::endl;
    }

    // root entry
    m_rootEntry = std::make_shared<Entry>(m_disk);
    m_rootEntry->m_name = "/";
    m_rootEntry->m_attributes = FileSystem::Directory | FileSystem::System;
    m_rootEntry->m_blockStart = kRootBlockNumber;
    m_rootEntry->m_numBlock = 0;
    m_rootEntry->m_parent = m_rootEntry->self();
}

FileSystem::~FileSystem()
{
    delete[] m_fat;
    delete[] m_buffer;
}

bool FileSystem::initFileSystem()
{
    bool succeeded;
    // init fat
    for (int i = 0; i != kNumOfFatBlocks; ++i)
    {
        m_fat[i] = -1;
    }
    m_fat[23] = m_fat[49] = -2; // 表示有两个坏块
    m_fat[kRootBlockNumber] = -1; // 根目录块已占用
    // 保存 FAT
    succeeded = saveFat();
    if (!succeeded) return false;

    std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer);

    // init root directory
    for (int i = 0; i != kMaxChildEntries; ++i)
    {
        m_buffer[kEntrySize * i] = '$'; // 所有的目录项都为空
    }
    // 写入根目录
    succeeded = m_disk.write(m_buffer, kRootBlockNumber);
    if (!succeeded) return false;

    if (!sync()) return false; // 更改持久化

    return true;
}

std::shared_ptr<Entry> FileSystem::rootEntry()
{
    return m_rootEntry;
}

std::shared_ptr<Entry> FileSystem::getEntry(const std::string &fullPath)
{
    if (fullPath[0] != '/') return nullptr; // 不是绝对路径

    auto names = splitPath(fullPath);

    auto targetEntry = m_rootEntry;
    for (const auto& name : names)
    {
        targetEntry = targetEntry->findChild(name);
        if (targetEntry == nullptr) break;
    }

    return targetEntry;
}

bool FileSystem::exist(const std::string &fullPath)
{
    return getEntry(fullPath) != nullptr;
}

bool FileSystem::createDir(const std::string &fullPath)
{
    if (exist(fullPath)) return false; // 目标已存在
    std::string parentPath = fullPath.substr(0, fullPath.find_last_of('/'));
    if (parentPath == "") // 父目录是根目录
    {
        parentPath = "/";
    }
    std::string dirName = fullPath.substr(fullPath.find_last_of('/') + 1);
    if (!checkName(dirName)) return false; // 名称不合法
    if (!exist(parentPath)) return false; // 父目录不存在
    auto parent = getEntry(parentPath);
    if (parent->getChildren().size() == kMaxChildEntries) return false; // 父目录子项数超限制

    std::lock_guard<std::mutex> fatLock(m_mutex1Fat);
    std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer);

    int blockNumber;
    if ((blockNumber = nextAvailableBlock()) < 0) return false; // 没有足够的块可供分配

    // 填充目录项
    for (int i = 0; i != kMaxChildEntries; ++i)
    {
        m_buffer[kEntrySize * i] = '$'; // 所有的目录项都为空
    }
    if (!m_disk.write(m_buffer, blockNumber)) return false;

    // 修改父目录项

    if (!m_disk.read(m_buffer, parent->m_blockStart)) return false;
    char* entryPointer = findChildEntryPointer(m_buffer, ""); // 一个空目录项指针
    // 填充目录名
    for (size_t i = 0; i != dirName.length(); ++i)
    {
        entryPointer[i] = dirName[i];
    }
    entryPointer[dirName.length()] = '$'; // 设置文件名结束标志
    // 填充其余信息
    entryPointer[kEntryAttributesIndex] = FileSystem::Directory;
    entryPointer[kEntryBlockStartIndex] = blockNumber;
    entryPointer[kEntryNumOfBlocksIndex] = 0;
    // 写入磁盘
    if (!m_disk.write(m_buffer, parent->m_blockStart)) return false;

    // 修改 FAT
    m_fat[blockNumber] = -1;
    if (!saveFat()) return false;

    if (!sync()) return false; // 更改持久化

    return true;
    // todo 适应可变 sector size
}

bool FileSystem::createFile(const std::string &fullPath, FileSystem::Attributes attributes)
{
    if (exist(fullPath)) return false; // 目标已存在
    std::string parentPath = fullPath.substr(0, fullPath.find_last_of('/'));
    if (parentPath == "") // 父目录是根目录
    {
        parentPath = "/";
    }
    std::string fileName = fullPath.substr(fullPath.find_last_of('/') + 1);
    if (!checkName(fileName)) return false; // 文件名不合法
    if (!exist(parentPath)) return false; // 父目录不存在
    auto parent = getEntry(parentPath);
    if (!parent->isDir()) return false; // 父目录不存在（不是目录），巨坑！！！
    if (parent->getChildren().size() == kMaxChildEntries) return false; // 父目录子项数超限制
    if (!(attributes & FileSystem::File)) return false; // 不是文件（属性错误）
    if ((attributes & FileSystem::ReadOnly)) return false; // 不允许为只读
    if ((attributes & FileSystem::Directory)) return false; // 不允许为目录

    {
        std::lock_guard<std::mutex> fatLock(m_mutex1Fat);
        std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer);

        int blockNumber;
        if ((blockNumber = nextAvailableBlock()) < 0) return false; // 没有足够的块可供分配

        // 填充文件内容
        m_buffer[0] = END_OF_FILE;
        if (!m_disk.write(m_buffer, blockNumber)) return false;

        // 修改父目录项

        if (!m_disk.read(m_buffer, parent->m_blockStart)) return false;
        char* entryPointer = findChildEntryPointer(m_buffer, ""); // 一个空目录项指针
        // 填充文件名
        for (size_t i = 0; i != fileName.length(); ++i)
        {
            entryPointer[i] = fileName[i];
        }
        entryPointer[fileName.length()] = '$'; // 设置文件名结束标志
        // 填充其余信息
        entryPointer[kEntryAttributesIndex] = attributes;
        entryPointer[kEntryBlockStartIndex] = blockNumber;
        entryPointer[kEntryNumOfBlocksIndex] = 1;
        // 写入磁盘
        if (!m_disk.write(m_buffer, parent->m_blockStart)) return false;

        // 修改 FAT
        m_fat[blockNumber] = -1;
        if (!saveFat()) return false;
    } // 释放锁

    if (!sync()) return false; // 更改持久化

    // 顺便打开文件，是否成功不打紧
    openFile(fullPath, Read | Write);

    return true;
    // todo 适应可变 sector size
}

bool FileSystem::openFile(const std::string &fullPath, FileSystem::OpenModes openModes)
{
    if (isOpened(fullPath)) return true; // 已经打开
    if (m_openedFiles.size() == kMaxOpenedFiles) return false; // 打开文件数量超限制
    if (!exist(fullPath)) return false; // 文件不存在
    auto fileEntry = getEntry(fullPath);
    if ((fileEntry->m_attributes & ReadOnly) && (openModes & Write)) return false; // 不能以写方式打开只读文件

    // 获取信息
    int blockStart = fileEntry->m_blockStart;
    int numOfBlock = fileEntry->m_numBlock;

    std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer);

    if (!m_disk.read(m_buffer, findNextNBlock(blockStart, numOfBlock - 1))) return false;
    int tailLength = 0;
    for ( ; m_buffer[tailLength] != END_OF_FILE; ++tailLength)
    {
        /* empty */
    }
    int length = kBlockSize * (numOfBlock - 1) + tailLength;

    // 加入打开列表
    std::shared_ptr<OpenedFile> of = std::make_shared<OpenedFile>();
    of->fullPath = fullPath;
    of->attributes = fileEntry->m_attributes;
    of->blockNumber = blockStart;
    of->numOfBlocks = numOfBlock;
    of->modes = openModes;
    of->g = 0;
    of->p = length;
    m_openedFiles.insert({fullPath, of});

    return true;
}

bool FileSystem::closeFile(const std::string &fullPath)
{
    // 等待缓存锁，即等待所有读写操作完成
    std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer); // lock buffer

    if (!sync()) return false; // 更改持久化

    auto iter = m_openedFiles.find(fullPath);
    if (iter == m_openedFiles.end()) return false;
    m_openedFiles.erase(iter);

    return true;
}

std::list<std::string> FileSystem::getOpenedFileList()
{
    std::list<std::string> ret;
    for(auto const& e : m_openedFiles)
    {
        ret.push_back((e.second)->fullPath);
    }
    return ret;
}

int FileSystem::readFile(const std::string &fullPath, char* buf_out, int length)
{
    if (!isOpened(fullPath)) // 文件没有打开
    {
        if (!openFile(fullPath, Read)) return 0; // 以读方式打开文件失败
    }
    std::shared_ptr<OpenedFile> fd = (m_openedFiles.find(fullPath))->second;
    if (!(fd->modes & Read)) return 0; // 没有以读的方式打开文件

    int rBlockNumber; // 当前读取的块号
    int rp; // 当前读取的块内指针

    // 初始化刚开始的读取块号和块内指针
    rBlockNumber = findNextNBlock(fd->blockNumber, fd->g / kBlockSize);
    rp = fd->g % kBlockSize;

    int wp = 0; // write pointer on buffer

    while (wp < length)
    {
        std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer);
        if (!m_disk.read(m_buffer, rBlockNumber)) break;
        while (wp < length && rp < kBlockSize)
        {
            if (m_buffer[rp] == END_OF_FILE)
            {
                goto read_end;
            }
            buf_out[wp++] = m_buffer[rp++];
            ++fd->g;
        }
        rBlockNumber = findNextNBlock(rBlockNumber, 1); // 找到下一文件块序号
        rp = 0; // 重置 rp，从下一文件块的头部开始
    }
    read_end:
    return wp;
}

bool FileSystem::writeFile(const std::string &fullPath, char *buffer, int length)
{
    if (!isOpened(fullPath)) // 文件没有打开
    {
        if (!openFile(fullPath, Read | Write)) return false; // 以写方式打开文件失败
    }
    std::shared_ptr<OpenedFile> fd = (m_openedFiles.find(fullPath))->second;
    if (!(fd->modes & Write)) return false; // 文件不是以写方式打开的

    int wBlockNumber; // 当前写入的块号
    int wp; // 当前写入的块内指针

    // 初始化刚开始的读取块号和块内指针
    wBlockNumber = findNextNBlock(fd->blockNumber, fd->p / kBlockSize);
    wp = fd->p % kBlockSize;

    // 给被写入数据未尾追加 END_OF_FILE
    std::string buf_in(buffer, buffer + length);
    ++length;
    buf_in.push_back(END_OF_FILE);

    int rp = 0; // read pointer on buffer

    while (rp < length)
    {
        std::lock_guard<std::mutex> lock1(m_mutex1Fat);
        std::lock_guard<std::mutex> lock2(m_mutex2Buffer);
        if (!m_disk.read(m_buffer, wBlockNumber)) break; // 先读入缓存
        while (rp < length && wp < kBlockSize)
        {
            m_buffer[wp++] = buf_in[rp++];
            ++fd->p;
        }
        if (!m_disk.write(m_buffer, wBlockNumber)) break; // 缓存满，写入磁盘

        // 已经写完一个块，如果还有数据要写则分配新块
        if (rp == length) break; // 没有更多数据，结束
        int previousNumber = wBlockNumber;
        wBlockNumber = nextAvailableBlock(); // 找到下一文件块序号
        if (wBlockNumber == -1) return false; // 没有新块可供分配

        // 修改 FAT
        m_fat[previousNumber] = wBlockNumber;
        m_fat[wBlockNumber] = -1;
        saveFat(); // 保存 FAT

        // 修改对应父目录项内记录的文件大小
        auto fileEntry = getEntry(fullPath);
        auto parentEntry = fileEntry->parent();
        if (!m_disk.read(m_buffer, parentEntry->m_blockStart)) break;
        char* fileEntryPointer = findChildEntryPointer(m_buffer, fileEntry->name());
        ++fileEntryPointer[kEntryNumOfBlocksIndex];
        if (!m_disk.write(m_buffer, parentEntry->m_blockStart)) break;

        wp = 0; // 重置 rp，从下一文件块的头部开始
    }

    --fd->p; // 前面记多了一次 END_OF_FILE 的写入

    return true;
}

bool FileSystem::setFileAttributes(const std::string &fullPath, FileSystem::Attributes attributes)
{
    if (!exist(fullPath)) return  false;
    auto entry = getEntry(fullPath);
    if (entry->isDir()) return false; // 不能为目录设置属性
    if (isOpened(fullPath)) return false; // 文件已被打开，不能改属性

    attributes &= ReadOnly | System | File; // 只保留有效的属性

    if (!(attributes & File)) return false; // 新属性中不能没有文件属性

    std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer); // lock buffer

    auto parentEntry = entry->parent();
    if (!m_disk.read(m_buffer, parentEntry->m_blockStart)) return false;
    char* fileEntryPointer = findChildEntryPointer(m_buffer, entry->name());
    fileEntryPointer[kEntryAttributesIndex] = attributes;
    if (!m_disk.write(m_buffer, parentEntry->m_blockStart)) return false;

    if (!sync()) return false;

    return true;
}

bool FileSystem::deleteEntry(const std::string &fullPath)
{
    if (!exist(fullPath)) return false;

    auto entry = getEntry(fullPath);
    if (entry->isDir()) // 是目录
    {
        if (entry->getChildren().size() != 0)
        {
            return false; // 不能删除非空目录
        }
    }
    else // 是文件
    {
        if (isOpened(fullPath)) return false; // 不能删除已打开文件
    }

    std::lock_guard<std::mutex> fatLock(m_mutex1Fat);
    std::lock_guard<std::mutex> bufferLock(m_mutex2Buffer);

    // 删除目录项
    auto parentEntry = entry->parent();
    if (!m_disk.read(m_buffer, parentEntry->m_blockStart)) return false;
    char* fileEntryPointer = findChildEntryPointer(m_buffer, entry->name());
    fileEntryPointer[0] = '$'; // 设该目录项为空目录项
    if (!m_disk.write(m_buffer, parentEntry->m_blockStart)) return false;

    // 释放 FAT
    int blockNumber = entry->m_blockStart;
    do
    {
        int next = m_fat[blockNumber];
        m_fat[blockNumber] = 0;
        blockNumber = next;
    } while (m_fat[blockNumber] != -1);
    if (!saveFat()) return false;

    if (!sync()) return false;

    return true;
}

bool FileSystem::sync()
{
    return m_disk.sync();
}

bool FileSystem::loadFat()
{
    return m_disk.read(m_fat, 0) && m_disk.read(m_fat + Disk::kSectorSize, 1);
}

bool FileSystem::saveFat()
{
    return m_disk.write(m_fat, 0) && m_disk.write(m_fat + Disk::kSectorSize, 1) && sync();
}

int FileSystem::nextAvailableBlock()
{
    for(int i = 0; i != kFatSize; ++i)
    {
        if (m_fat[i] == 0)
        {
            return i;
        }
    }
    return -1;
}

int FileSystem::findNextNBlock(int firstBlock, int n)
{
    int nextNBlock = firstBlock;
    for (int i = 0; i < n; ++i)
    {
        nextNBlock = m_fat[nextNBlock];
        if (nextNBlock == -1)
            break;
    }
    return nextNBlock;
}

bool FileSystem::isOpened(const std::string &fullPath)
{
    return m_openedFiles.find(fullPath) != std::end(m_openedFiles);
}

std::string FileSystem::getNameFromEntryPointer(char *p)
{
    char* end = p;
    for ( ; *end != '$'; ++end) {}
    return std::string(p, end);
}

char* FileSystem::findChildEntryPointer(char *parentEntryPointer, const std::string &childName)
{
    for (int i = 0; i != kMaxChildEntries; ++i)
    {
        char* childEntryPointer = parentEntryPointer + kEntrySize * i;
        auto name = getNameFromEntryPointer(childEntryPointer);
        if (name == childName) // 找到对应的目录项
        {
            return childEntryPointer;
        }
    }
    return nullptr;
}

bool FileSystem::checkName(const std::string &name)
{
    return name.length() > 0 && name.length() < kRawFileNameLength
            && name.find_first_of('$') == std::string::npos;
}

std::list<std::string> FileSystem::splitPath(const std::string &fullPath)
{
    std::list<std::string> names;
    std::string name;
    std::istringstream iss(fullPath);
    while (getline(iss, name, '/'))
    {
        names.push_back(name);
    }
    if (names.front().length() == 0)
    {
        names.pop_front(); // 移除没有名字的根目录
    }
    return names;
}

std::vector<std::shared_ptr<Entry> > Entry::getChildren()
{
    std::vector<std::shared_ptr<Entry>> ret;
    if (!isDir()) return ret; // 非目录则返回空列表

    // 申请缓存空间
    char* buffer = new char[Disk::kSectorSize];
    m_disk.read(buffer, m_blockStart);

    for (int i = 0; i != FileSystem::kMaxChildEntries; ++i)
    {
        char* entryPointer = buffer + FileSystem::kEntrySize * i;
        // find name
        std::string name = FileSystem::getNameFromEntryPointer(entryPointer);
        if (!FileSystem::checkName(name)) // 名字无效，这个目录项为空
            continue;                     // 继续查找下一目录项
        // 找到目录项，生成 Entry
        std::shared_ptr<Entry> entry(new Entry(m_disk));
        entry->m_parent = self();
        entry->m_name = name;
        entry->m_attributes = entryPointer[FileSystem::kEntryAttributesIndex];
        entry->m_blockStart = entryPointer[FileSystem::kEntryBlockStartIndex];
        entry->m_numBlock = entryPointer[FileSystem::kEntryNumOfBlocksIndex];
        // 加入返回结果集
        ret.push_back(entry);
    }

    // 释放资源
    delete[] buffer;

    return ret;
}

std::shared_ptr<Entry> Entry::findChild(const std::string &name)
{
    auto children = getChildren();
    for (const auto& entry : children)
    {
        if (entry->name() == name)
        {
            return entry;
        }
    }
    return nullptr;
}

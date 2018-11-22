#include "filesystem.h"

#include "disk.h"

#include <algorithm>
#include <iterator>
#include <iostream>
#include <sstream>
#include <list>

FileSystem::FileSystem(Disk &disk)
    : kFatSize(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize),
      kNumOfFatBlocks(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize / kBlockSize),
      kRootBlockNumber(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize / kBlockSize),
      m_disk(disk)
{
    // FAT
    fat = new char[kFatSize];
    std::for_each(fat, fat + kFatSize, [](char &e){
        e = 0;
    });

    // buffer
    buffer = new char[Disk::kSectorSize];

    // load FAT
    bool succeeded = loadFat();
    if (!succeeded)
    {
        std::cerr << "Fatal: cannot load FAT from disk." << std::endl;
    }

    // root entry
    m_rootEntry = std::make_shared<Entry>(m_disk);
    m_rootEntry->m_name = "/";
    m_rootEntry->m_attrbutes = FileSystem::Directory | FileSystem::System;
    m_rootEntry->m_blockStart = kRootBlockNumber;
    m_rootEntry->m_numBlock = 0;
    m_rootEntry->m_parent = m_rootEntry->self();

    // opend files
    std::for_each(std::begin(m_openedFilesStat), std::end(m_openedFilesStat), [](bool& e){
        e = false;
    });
}

FileSystem::~FileSystem()
{
    delete[] fat;
    delete[] buffer;
}

bool FileSystem::initFileSystem()
{
    bool succeeded;
    // init fat
    for (int i = 0; i != kNumOfFatBlocks; ++i)
    {
        fat[i] = -1;
    }
    fat[23] = fat[49] = -2; // 表示有两个坏块
    fat[kRootBlockNumber] = -1; // 根目录块已占用
    // 保存 FAT
    succeeded = saveFat();
    if (!succeeded) return false;

    // init root directory
    for (int i = 0; i != 8; ++i)
    {
        buffer[i * 8] = '$'; // 所有的目录项都为空
    }
    // 写入根目录
    succeeded = m_disk.write(buffer, kRootBlockNumber);
    if (!succeeded) return false;

    return true;
}

std::shared_ptr<Entry> FileSystem::rootEntry()
{
    return m_rootEntry;
}

std::shared_ptr<Entry> FileSystem::getEntry(const std::string &path)
{
    if (path[0] != '/') return nullptr; // 不是绝对路径

    std::list<std::string> names;
    std::string name;
    std::istringstream iss(path);
    while (getline(iss, name, '/'))
    {
        names.push_back(name);
    }
    names.pop_front(); // 移除没有名字的根目录

    auto targetEntry = m_rootEntry;
    for (const auto& n : names)
    {
        targetEntry = targetEntry->findChild(n);
        if (targetEntry == nullptr) break;
    }

    return targetEntry;
}

bool FileSystem::loadFat()
{
    return m_disk.read(fat, 0) && m_disk.read(fat + Disk::kSectorSize, 1);
}

bool FileSystem::saveFat()
{
    return m_disk.write(fat, 0) && m_disk.write(fat + Disk::kSectorSize, 1);
}

std::vector<std::shared_ptr<Entry> > Entry::getChildren()
{
    std::vector<std::shared_ptr<Entry>> ret;
    if (!isDir()) return ret; // 非目录则返回空列表

    // 申请缓存空间
    char* buffer = new char[Disk::kSectorSize];
    m_disk.read(buffer, m_blockStart);

    for (int p = 0; p != 8; ++p)
    {
        char* rawData = buffer + p;
        // find name
        std::string name;
        for (int i = 0; i != FileSystem::kRawFileNameLength; ++i)
        {
            char c = rawData[i];
            if (std::isprint(c) && c != '$')
            {
                name.push_back(c);
            }
        }
        if (!FileSystem::checkName(name)) // 名字无效，这个目录项为空
            continue;                     // 继续查找下一目录项
        // 找到目录项，生成 Entry
        std::shared_ptr<Entry> entry(new Entry(m_disk));
        entry->m_parent = self();
        entry->m_name = name;
        entry->m_attrbutes = rawData[5];
        entry->m_blockStart = rawData[6];
        entry->m_numBlock = rawData[7];
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
    return self();
}

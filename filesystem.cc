#include "filesystem.h"

#include "disk.h"

#include <algorithm>
#include <iterator>
#include <iostream>

FileSystem::FileSystem(Disk &disk)
    : kFatSize(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize),
      kNumOfFatBlocks(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize / kBlockSize),
      kRootBlockNumber(Disk::kNumOfSector * Disk::kSectorSize / kBlockSize / kBlockSize),
      m_disk(disk)
{
    // m_fat
    fat = new char[kFatSize];
    std::for_each(fat, fat + kFatSize, [](char &e){
        e = 0;
    });
    // m_buffer
    buffer = new char[Disk::kSectorSize];
    // load FAT
    bool succeeded = loadFat();
    if (!succeeded)
    {
        std::cerr << "Fatal: cannot load FAT from disk." << std::endl;
    }
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
    succeeded = m_disk.write(buffer, kRootBlockNumber, kBlockSize) == kBlockSize;
    if (!succeeded) return false;

    return true;
}

std::vector<std::string> FileSystem::ls(const std::string &dir)
{

}

bool FileSystem::loadFat()
{
    return m_disk.read(fat, 0, kFatSize) == kFatSize;
}

bool FileSystem::saveFat()
{
    return m_disk.write(fat, 0, kFatSize) == kFatSize;
}

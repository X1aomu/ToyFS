#include "disk.h"

#include <fstream>
#include <iostream>
#include <limits>

bool Disk::CreateDisk(const std::__cxx11::string &filePath)
{
    std::ofstream newFile(filePath);
    for (int i = 0; i != kNumOfSector * kSectorSize; ++i)
    {
        newFile.write("", 1); // one byte a time
    }

    return newFile.good();
}

Disk::Disk(const std::string &diskFile)
{
    m_ioFile.open(diskFile, std::ios::in | std::ios::out | std::ios::binary);
}

Disk::~Disk()
{
    m_ioFile.close();
}

bool Disk::isValid()
{
    if (m_ioFile.good())
    {
        auto currentPos = m_ioFile.tellg();

        m_ioFile.seekg(0, std::ios::beg);
        m_ioFile.ignore(std::numeric_limits<std::streamsize>::max());
        auto fileSize = m_ioFile.gcount();
        m_ioFile.clear();

        m_ioFile.seekg(currentPos);

        return fileSize == kNumOfSector * kSectorSize;
    }
    else
    {
        return false;
    }
}

bool Disk::read(char* buf, int sector)
{
    m_ioFile.seekg(kSectorSize * sector, std::ios::beg);
    m_ioFile.read(buf, kSectorSize);

    return m_ioFile.gcount() == kSectorSize;
}

bool Disk::write(char *buf, int sector)
{
    m_ioFile.seekp(kSectorSize * sector, std::ios::beg);
    auto posBefore = m_ioFile.tellp();
    m_ioFile.write(buf, kSectorSize);
    auto posAfter = m_ioFile.tellp();

    return posAfter - posBefore == kSectorSize;
}

bool Disk::sync()
{
    return m_ioFile.sync() == 0; // NOTE: 这个地方似乎不支持用 clang 编译，clang-7.0.0 on Archlinux x64
}

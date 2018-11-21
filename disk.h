#ifndef TOYFS_FAKEDISK_H_
#define TOYFS_FAKEDISK_H_

#include <string>
#include <fstream>

class Disk
{
public:
    // static attributes
    static const int kNumOfSector = 128;
    static const int kSectorSize = 64; // in byte

    // static functions
    /**
     * @brief CreateDisk Create a fake disk.
     *
     * @param filePath File path.
     * @return true if succeed.
     */
    static bool CreateDisk(const std::string& filePath);

    // constructors & destructor
    explicit Disk(const std::string& diskFile);
    ~Disk();
    // keep from copying
    Disk(const Disk&) = delete;
    Disk& operator=(const Disk&) = delete;

    /**
     * @brief isValid Whether is this disk valid or not.
     *
     * @return true if is valid.
     */
    bool isValid();

    /**
     * @brief read Read data from disk.
     *
     * @param buf Buffer to store data.
     * @param sector Start sector.
     * @param length Number of bytes to read.
     * @return Number of bytes read.
     */
    long read(char* buf, int sector, int length);
    /**
     * @brief write Write data to disk.
     *
     * @param buf Buffer to read data.
     * @param sector Start sector.
     * @param length Number of bytes to write.
     * @return Number of bytes written.
     */
    long write(char* buf, int sector, int length);

private:
    std::fstream m_ioFile;
};

#endif // TOYFS_FAKEDISK_H_

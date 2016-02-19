#define LOG_TAG "BLOB_FILE"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "tvconfig/tvconfig.h"
#include "CBlobDeviceFile.h"
#include "CTvLog.h"

CBlobDeviceFile::CBlobDeviceFile()
{
    m_dev_total_size = 4 * 1024;
    m_dev_fd = -1;
    mDataBuf = new unsigned char[m_dev_total_size];

    const char *device_path = config_get_str(CFG_SECTION_SETTING, "device_path", "/param/default_data");
    const char *device_size = config_get_str(CFG_SECTION_SETTING, "device_size", "0x1000");
    strcpy(m_dev_path, device_path);
}

CBlobDeviceFile::~CBlobDeviceFile()
{
    if (mDataBuf != NULL) {
        delete mDataBuf;
        mDataBuf = NULL;
    }
    CloseDevice();
}

int CBlobDeviceFile::WriteBytes(int offset, int size, unsigned char *buf)
{

    lseek(m_dev_fd, offset, SEEK_SET);
    write(m_dev_fd, buf, size);
    //not need
    //fsync(device_fd);
    return 0;
}
int CBlobDeviceFile::ReadBytes(int offset, int size, unsigned char *buf)
{
    lseek(m_dev_fd, offset, SEEK_SET);
    read(m_dev_fd, buf, size);
    return 0;
}
int CBlobDeviceFile::EraseAllData()
{
    return 0;
}
int CBlobDeviceFile::InitCheck()
{
    return 0;
}

int CBlobDeviceFile::OpenDevice()
{
    if (strlen(m_dev_path) <= 0) return -1;

    m_dev_fd = open(m_dev_path, O_RDWR | O_SYNC | O_CREAT, S_IRUSR | S_IWUSR);

    if (m_dev_fd < 0) {
        LOGE("%s, Open device file \"%s\" error: %s.\n", "TV", m_dev_path, strerror(errno));
        return -1;
    }

    return 0;
}

int CBlobDeviceFile::CloseDevice()
{
    if (m_dev_fd >= 0) {
        close(m_dev_fd);
        m_dev_fd = -1;
    }
    return 0;
}

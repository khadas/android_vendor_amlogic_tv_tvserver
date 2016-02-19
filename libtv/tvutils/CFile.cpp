#define LOG_TAG "CFILE"

#include "CFile.h"
#include "CTvLog.h"

#include <stdlib.h>

CFile::CFile()
{
    mPath[0] = '\0';
    mFd = -1;
}

CFile::~CFile()
{
    closeFile();
}

CFile::CFile(const char *path)
{
    strcpy(mPath, path);
    mFd = -1;
}

int CFile::openFile(const char *path)
{
    LOGD("openFile = %s", path);

    if (mFd < 0) {
        if (path == NULL) {
            if (strlen(mPath) <= 0)
                return -1;

            mFd = open(mPath, O_RDWR);//读写模式打开
            if (mFd < 0) LOGD("open file(--%s) fail", mPath);
        } else {
            mFd = open(path, O_RDWR);//读写模式打开
            LOGD("open file(%s fd=%d) ", path, mFd);
            strcpy(mPath, path);
        }
    }

    return mFd;
}

int CFile::closeFile()
{
    if (mFd > 0) {
        close(mFd);
        mFd = -1;
    }
    return 0;
}

int CFile::writeFile(const unsigned char *pData, int uLen)
{
    int ret = -1;
    if (mFd > 0)
        ret = write(mFd, pData, uLen);

    return ret;
}

int CFile::readFile(void *pBuf, int uLen)
{
    int ret = 0;
    if (mFd > 0) {
        ret = read(mFd, pBuf, uLen);
    }
    return ret;
}

int CFile::copyTo(const char *dstPath)
{
    if (strlen(mPath) <= 0)
        return -1;
    int dstFd;
    if (mFd == -1) {
        if ((mFd = open(mPath, O_RDONLY)) == -1) {
            LOGE("Open %s Error:%s/n", mPath, strerror(errno));
            return -1;
        }
    }

    if ((dstFd = open(dstPath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
        LOGE("Open %s Error:%s/n", dstPath, strerror(errno));
    }

    int bytes_read, bytes_write;
    char buffer[BUFFER_SIZE];
    char *ptr;
    int ret = 0;
    while ((bytes_read = read(mFd, buffer, BUFFER_SIZE))) {
        /* 一个致命的错误发生了 */
        if ((bytes_read == -1) && (errno != EINTR)) {
            ret = -1;
            break;
        } else if (bytes_read > 0) {
            ptr = buffer;
            while ((bytes_write = write(dstFd, ptr, bytes_read))) {
                /* 一个致命错误发生了 */
                if ((bytes_write == -1) && (errno != EINTR)) {
                    ret = -1;
                    break;
                }
                /* 写完了所有读的字节 */
                else if (bytes_write == bytes_read) {
                    ret = 0;
                    break;
                }
                /* 只写了一部分,继续写 */
                else if (bytes_write > 0) {
                    ptr += bytes_write;
                    bytes_read -= bytes_write;
                }
            }
            /* 写的时候发生的致命错误 */
            if (bytes_write == -1) {
                ret = -1;
                break;
            }
        }
    }
    fsync(dstFd);
    close(dstFd);
    return ret;
}


int CFile::delFile(const char *path)
{
    if (strlen(path) <= 0) return -1;
    if (unlink(path) != 0) {
        LOGD("delete file(%s) err=%s", path, strerror(errno));
        return -1;
    }
    return 0;
}

int CFile::delFile()
{
    if (strlen(mPath) <= 0) return -1;
    if (unlink(mPath) != 0) {
        LOGD("delete file(%s) err=%s", mPath, strerror(errno));
        return -1;
    }
    return 0;
}


int  CFile::getFileAttrValue(const char *path)
{
    int value;

    int fd = open(path, O_RDONLY);
    if (fd <= 0) {
        LOGE("open  (%s)ERROR!!error = -%s- \n", path, strerror ( errno ));
    }
    char s[8];
    read(fd, s, sizeof(s));
    close(fd);
    value = atoi(s);
    return value;
}

int  CFile::setFileAttrValue(const char *path, int value)
{
    FILE *fp = fopen ( path, "w" );

    if ( fp == NULL ) {
        LOGW ( "Open %s error(%s)!\n", path, strerror ( errno ) );
        return -1;
    }
    fprintf ( fp, "%d", value );
    fclose ( fp );
    return 0;
}

int CFile::getFileAttrStr(const char *path __unused, char *str __unused)
{
    return 0;
}

int CFile::setFileAttrStr(const char *path, const char *str)
{
    FILE *fp = fopen ( path, "w" );

    if ( fp == NULL ) {
        LOGW ( "Open %s error(%s)!\n", path, strerror ( errno ) );
        return -1;
    }
    fprintf ( fp, "%s", str );
    fclose ( fp );
    return 0;
}

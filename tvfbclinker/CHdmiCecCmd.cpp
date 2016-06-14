#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CHdmiCec"

#include "CHdmiCecCmd.h"
CHdmiCec::CHdmiCec()
{
}

CHdmiCec::~CHdmiCec()
{
}

int CHdmiCec::readFile(unsigned char *pBuf __unused, unsigned int uLen __unused)
{
    int ret = 0;
    //ret = read(mFd, pBuf, uLen);
    return ret;
}


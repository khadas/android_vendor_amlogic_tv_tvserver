#define LOG_TAG "CSerialCommunication"

#include <CTvLog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/log.h>
#include "CSerialCommunication.h"

CSerialCommunication::CSerialCommunication()
{
    int i = 0;

    mpObserver = NULL;
    mState = STATE_STOPED;

    mFileName[0] = 0;
    for (i = 0; i < CC_MAX_SERIAL_RD_BUF_LEN; i++) {
        mDataBuf[i] = 0;
    }
}

CSerialCommunication::~CSerialCommunication()
{
    mSerialPort.CloseDevice();
}

int CSerialCommunication::OpenModule(int serial_dev_id)
{
    int tmp_ret = 0;

    if (mSerialPort.OpenDevice(serial_dev_id) < 0) {
        return -1;
    }

    return mSerialPort.setup_serial();
}

int CSerialCommunication::CloseModule()
{
    stop();
    mSerialPort.CloseDevice();
    return 0;
}

int CSerialCommunication::start()
{
    if (mState == STATE_STOPED) {
        this->run();
    }

    return 0;
}

int CSerialCommunication::stop()
{
    requestExit();
    mState = STATE_STOPED;

    return 0;
}

int CSerialCommunication::sendData(int data_len, unsigned char data_buf[])
{
    mSerialPort.writeFile(data_buf, data_len);
    return 0;
}

bool CSerialCommunication::threadLoop()
{
    int rd_len = 0;
    int thread_dly_tm = 100 * 1000;

    if (mpObserver == NULL) {
        return false;
    }

    if (mSerialPort.getFd() < 0) {
        return false;
    }

    if (mSerialPort.getDevId() < 0) {
        return false;
    }

    LOGD("%s, entering...\n", "TV");

    prctl(PR_SET_NAME, (unsigned long)"CSerialCommunication thread loop");

    mState = STATE_RUNNING;

    while (!exitPending()) { //requietexit() or requietexitWait() not call
        memset(mDataBuf, 0, CC_MAX_SERIAL_RD_BUF_LEN);
        rd_len = mSerialPort.readFile(mDataBuf, CC_MAX_SERIAL_RD_BUF_LEN);
        if (rd_len <= 0) {
            usleep(thread_dly_tm);
            continue;
        }

        LOGD("%s, rd_len = %d.\n", __FUNCTION__, rd_len);

        if (rd_len > CC_MAX_SERIAL_RD_BUF_LEN) {
            rd_len = CC_MAX_SERIAL_RD_BUF_LEN;
        }

        LOGD("%s, send data to java.", __FUNCTION__);
        //send data to java
        mpObserver->onSerialCommunication(mSerialPort.getDevId(), rd_len, mDataBuf);

        usleep(thread_dly_tm);
    }

    mState = STATE_STOPED;

    LOGD("%s, exiting...\n", "TV");
    //return true, run again, return false,not run.
    return false;
}

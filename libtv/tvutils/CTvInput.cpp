#define LOG_TAG "CTvInput"

#include "CTvInput.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <CTvLog.h>
#include <utils/Timers.h>

#define SEND_KEY_ACTION_UP              0x00
#define SEND_KEY_ACTION_DOWN            0x01

CTvInput::CTvInput()
{
    mKeyEventFd_IR = open(KEY_EVENT_0, O_RDWR);
    mKeyEventFd = open(KEY_EVENT_1, O_RDWR);
    if (mKeyEventFd < 0 || mKeyEventFd_IR < 0) {
        LOGD( "could not open /dev/input/event1\n ");
        return;
    }
    mRepeatKeyCode = -1;
    mRepeatKeydisTime = -1;
    mWhenTimeRepeatKeyStartToSend = -1;
    mTimeoutCount = 0;
}

CTvInput::~CTvInput()
{
    close(mKeyEventFd);
    close(mKeyEventFd_IR);
}

void CTvInput::sendkeyEvent(const int &type, const int &code, const int &value)
{
    int ret;
    struct input_event event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code ;
    event.value = value;
    ret = write(mKeyEventFd, &event, sizeof(event));
    if (ret < (int)sizeof(event)) {
        LOGD("sendkeyEvent :write event failed, %s\n", strerror(errno));
        return;
    }
}

void CTvInput::sendIRkeyEvent(const int &type, const int &code, const int &value)
{
    int ret;
    struct input_event event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code ;
    event.value = value;
    ret = write(mKeyEventFd_IR, &event, sizeof(event));
    if (ret < (int)sizeof(event)) {
        LOGD("sendIRkeyEvent :write event failed, %s\n", strerror(errno));
        return;
    }
}

void CTvInput::sendkeyCode(const int &code)
{
    sendkeyEvent(EV_KEY , code, SEND_KEY_ACTION_DOWN); //down
    sendkeyEvent(EV_SYN, 0, 0); //clear
    sendkeyEvent(EV_KEY , code, SEND_KEY_ACTION_UP); //up
    sendkeyEvent(EV_SYN, 0, 0); //clear
}

void CTvInput::sendkeyCode_Up(const int &code)
{
    sendkeyEvent(EV_KEY , code, SEND_KEY_ACTION_UP); //up
    sendkeyEvent(EV_SYN, 0, 0); //clear
}

void CTvInput::sendkeyCode_Down(const int &code)
{
    sendkeyEvent(EV_KEY , code, SEND_KEY_ACTION_DOWN); //down
    sendkeyEvent(EV_SYN, 0, 0); //clear
}

void CTvInput::sendIRkeyCode(const int &code)
{
    sendIRkeyEvent(EV_KEY , code, SEND_KEY_ACTION_DOWN); //down
    sendIRkeyEvent(EV_SYN, 0, 0); //clear
    sendIRkeyEvent(EV_KEY , code, SEND_KEY_ACTION_UP); //up
    sendIRkeyEvent(EV_SYN, 0, 0); //clear
}

void CTvInput::sendIRkeyCode_Up(const int &code)
{
    sendIRkeyEvent(EV_KEY , code, SEND_KEY_ACTION_UP); //up
    sendIRkeyEvent(EV_SYN, 0, 0); //clear
}

void CTvInput::sendIRkeyCode_Down(const int &code)
{
    sendIRkeyEvent(EV_KEY , code, SEND_KEY_ACTION_DOWN); //down
    sendIRkeyEvent(EV_SYN, 0, 0); //clear
}

nsecs_t CTvInput::getNowMs()
{
    return systemTime(SYSTEM_TIME_MONOTONIC) / 1000000;
}

void CTvInput::sendKeyRepeatStart(const int &code, int disTime, int repeatDisTime)
{
    CMutex::Autolock _l(mLock);
    mTimeoutCount = 0;
    mRepeatKeyCode = code;
    mRepeatKeydisTime = disTime;
    mWhenTimeRepeatKeyStartToSend = disTime + getNowMs();
    mRepeatKeyRepeateDisTime = repeatDisTime;
    LOGD("sendKeyRepeatStart when = %lld", mWhenTimeRepeatKeyStartToSend);
    mSendKeyCondition.signal();
}

void CTvInput::sendKeyRepeatStop()
{
    CMutex::Autolock _l(mLock);
    mRepeatKeyCode = -1;
    mRepeatKeydisTime = -1;
}

bool CTvInput::threadLoop()
{
    int sleeptime = 100;//ms

    while (!exitPending()) { //requietexit() or requietexitWait() not call
        LOGD("threadLoop0 when = %lld", mWhenTimeRepeatKeyStartToSend);
        while (mRepeatKeyCode < 0 || mRepeatKeydisTime < 0) { //msg queue is empty
            mLock.lock();
            mSendKeyCondition.wait(mLock);//first unlock,when return,lock again,so need,call unlock
            mLock.unlock();
        }
        LOGD("threadLoop1 when = %lld", mWhenTimeRepeatKeyStartToSend);
        int disToSend = -1;
        do {
            disToSend = mWhenTimeRepeatKeyStartToSend - getNowMs();
            LOGD("dis when = %d", disToSend);
            if (disToSend <= 0) break;

            mLock.lock();
            mSendKeyCondition.waitRelative(mLock, disToSend);//first unlock,when return,lock again,so need,call unlock
            mLock.unlock();
        } while (disToSend > 0); //
        LOGD("threadLoop2 when = %lld ", mWhenTimeRepeatKeyStartToSend);
        if (mTimeoutCount < 30) {
            LOGD("mTimeoutCount = %d \n", mTimeoutCount);

            mTimeoutCount++;
            if (mRepeatKeyCode == 955) {
                sendkeyCode_Down(mRepeatKeyCode);
            } else {
                sendkeyCode(mRepeatKeyCode);
            }
        } else {
            LOGD("mTimeoutCount = %d,so stop sending long press msg.\n", mTimeoutCount);
        }
        usleep(mRepeatKeyRepeateDisTime * 1000);
    }
    //exit
    //return true, run again, return false,not run.
    return false;
}
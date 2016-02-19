#ifndef _TV_INPUT_H
#define _TV_INPUT_H
#include <linux/input.h>
#
#include "CThread.h"
static const   char *KEY_EVENT_0 = "/dev/input/event0";
#ifdef ADC_USE_KEY_EVENT0
static const   char *KEY_EVENT_1 = "/dev/input/event0";
#else
static const   char *KEY_EVENT_1 = "/dev/input/event1";
#endif
static const   char *KEY_EVENT_2 = "/dev/input/event2";
class CTvInput: public CThread {
public:
    CTvInput();
    ~CTvInput();
    void sendkeyEvent(const int &type, const int &code, const int &value);
    void sendIRkeyEvent(const int &type, const int &code, const int &value);
    void sendkeyCode(const int &code);
    void sendkeyCode_Up(const int &code);
    void sendkeyCode_Down(const int &code);
    void sendIRkeyCode(const int &code);
    void sendIRkeyCode_Up(const int &code);
    void sendIRkeyCode_Down(const int &code);
    void sendKeyRepeatStart(const int &code, int disTime, int repeatDisTime);//ms
    void sendKeyRepeatStop();
    nsecs_t getNowMs();

private:
    bool  threadLoop();

    int mKeyEventFd;
    int mKeyEventFd_IR;
    CCondition mSendKeyCondition;
    CMutex   mLock;
    int mRepeatKeyCode;
    int mRepeatKeydisTime;
    int mRepeatKeyRepeateDisTime;
    int mTimeoutCount;
    nsecs_t mWhenTimeRepeatKeyStartToSend;
};
#endif

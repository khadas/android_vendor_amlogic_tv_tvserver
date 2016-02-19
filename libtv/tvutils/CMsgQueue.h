/*
 * showboz
 * 单(线程)消费者
 * 一个简单的MSG handler
*/
#include "CThread.h"
#include <utils/Vector.h>
using namespace android;
#if !defined(_C_MSG_QUEUE_H)
#define _C_MSG_QUEUE_H

class CMessage {
public:
    CMessage();
    ~CMessage();
    nsecs_t mDelayMs;//delay times , MS
    nsecs_t mWhenMs;//when, the msg will handle
    int mType;
    void *mpData;
    unsigned char mpPara[3072];
};

class CMsgQueueThread: public CThread {
public:
    CMsgQueueThread();
    virtual ~CMsgQueueThread();
    int startMsgQueue();
    void sendMsg(CMessage &msg);
    void removeMsg(CMessage &msg);
    void clearMsg();
private:
    bool  threadLoop();
    nsecs_t getNowMs();//get system time , MS
    virtual void handleMessage(CMessage &msg) = 0;

    //
    Vector<CMessage> m_v_msg;
    CCondition mGetMsgCondition;
    CMutex   mLockQueue;
};

/*class CHandler
{
    pubulic:
    CHandler(CMsgQueueThread& msgQueue);
    ~CHandler();
    void sendMsg();
    void removeMsg();
    private:
    virtual void handleMessage(CMessage &msg);
};*/

#endif

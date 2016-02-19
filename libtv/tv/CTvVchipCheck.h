//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvDimension.h
//  @ Date : 2013-11
//  @ Author :
//
//

#if !defined(_CTVVCHIPCHECK_H)
#define _CTVVCHIPCHECK_H
#include <utils/Vector.h>
#include "CTvDatabase.h"
#include <utils/String8.h>
#include <stdlib.h>
#include "CTvDimension.h"
#include "CTvProgram.h"
#include "CTvTime.h"
#include "CTvEvent.h"
#include "CTvLog.h"
#include "tvutils/CThread.h"
// TV ATSC rating dimension
class CTvVchipCheck: public CThread {
public:
    CTvVchipCheck();
    ~CTvVchipCheck();
    bool CheckProgramBlock(int id);
    static void *VchipCheckingThread ( void *arg );
    int startVChipCheck();
    int stopVChipCheck();
    int pauseVChipCheck();
    int resumeVChipCheck();
    int requestAndWaitPauseVChipCheck();
private:
    bool  threadLoop();
    mutable CMutex           mLock;
    CCondition       mDetectPauseCondition;
    CCondition       mRequestPauseCondition;
    volatile bool m_request_pause_detect;
    enum DetectState {
        STATE_STOPED = 0,
        STATE_RUNNING,
        STATE_PAUSE
    };
    int mDetectState;
};
#endif  //_CTVDIMENSION_H

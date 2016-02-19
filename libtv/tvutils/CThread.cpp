/*
reference android api, just linux pthread
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>

#include "CThread.h"
#include <CTvLog.h>

#include <sys/prctl.h>

CThread::CThread()
    :   mThreadId(-1),
        mLock("Thread::mLock"),
        mExitPending(false), mRunning(false)
{
}

CThread::~CThread()
{
}

int CThread::readyToRun()
{
    return 0;
}


int CThread::run(const char *name __unused, int priority __unused, int stack)
{
    CMutex::Autolock _l(mLock);

    if (mRunning) {
        // thread already started
        return -1;
    }

    mStatus = 0;
    mExitPending = false;
    mThreadId = -1;
    mRunning = true;


    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (stack) {
        pthread_attr_setstacksize(&attr, stack);
    }

    int result = pthread_create(&mThreadId, &attr,
                                _threadLoop, this);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        LOGE("Thread create fail ret = %d (errno=%d)\n",
             result, errno);

        mStatus = -1;   // something happened!
        mRunning = false;
        mThreadId = -1;
        return -1;
    }
    return 0;
}

void *CThread::_threadLoop(void *user)
{
    CThread *const self = static_cast<CThread *>(user);

    bool first = true;
    do {
        bool result;
        if (first) {
            first = false;
            self->mStatus = self->readyToRun();
            result = (self->mStatus == 0);

            if (result && !self->exitPending()) {
                result = self->threadLoop();
            }
        } else {
            result = self->threadLoop();
        }


        CMutex::Autolock _l(self->mLock);
        if (result == false || self->mExitPending) {
            self->mExitPending = true;
            self->mRunning = false;
            // clear thread ID so that requestExitAndWait() does not exit if
            // called by a new thread using the same thread ID as this one.
            self->mThreadId = -1;
            // note that interested observers blocked in requestExitAndWait are
            // awoken by broadcast, but blocked on mLock until break exits scope
            self->mThreadExitedCondition.broadcast();
            break;
        }
    } while (true);

    return NULL;
}

void CThread::requestExit()
{
    CMutex::Autolock _l(mLock);
    mExitPending = true;
}

int CThread::requestExitAndWait()
{
    CMutex::Autolock _l(mLock);
    if (mThreadId == pthread_self()) {
        ALOGW(
            "Thread (this=%p): don't call waitForExit() from this "
            "Thread object's thread. It's a guaranteed deadlock!",
            this);
        return -1;
    }

    mExitPending = true;

    while (mRunning == true) {
        mThreadExitedCondition.wait(mLock);
    }
    // This next line is probably not needed any more, but is being left for
    // historical reference. Note that each interested party will clear flag.
    mExitPending = false;

    return mStatus;
}

int CThread::join()
{
    CMutex::Autolock _l(mLock);
    if (mThreadId == pthread_self()) {
        ALOGW(
            "Thread (this=%p): don't call join() from this "
            "Thread object's thread. It's a guaranteed deadlock!",
            this);

        return -1;
    }

    while (mRunning == true) {
        mThreadExitedCondition.wait(mLock);
    }

    return mStatus;
}

bool CThread::exitPending() const
{
    CMutex::Autolock _l(mLock);
    return mExitPending;
}

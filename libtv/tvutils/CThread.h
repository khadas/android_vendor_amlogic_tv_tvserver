/*
reference android api, just linux pthread
*/

#ifndef _TV_UTILS_THREAD_H
#define _TV_UTILS_THREAD_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include "CCondition.h"
#include "CMutex.h"

class CThread {
public:
    CThread();
    virtual ~CThread();

    // Start the thread in threadLoop() which needs to be implemented.
    virtual int run(const char *name = 0,
                    int priority = 0,
                    int stack = 0);
    //request thread object to exit, asynchronous to exit.
    virtual void        requestExit();

    // Good place to do one-time initializations
    virtual int    readyToRun();

    // Call requestExit() and wait until this object's thread exits.
    // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
    // this function from this object's thread. Will return WOULD_BLOCK in
    // that case.
    int    requestExitAndWait();

    // Wait until this object's thread exits. Returns immediately if not yet running.
    // Do not call from this object's thread; will return WOULD_BLOCK in that case.
    int    join();
protected:
    // exitPending() returns true if requestExit() has been called.
    bool        exitPending() const;

private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool        threadLoop() = 0;

private:
    static  void    *_threadLoop(void *user);
    pthread_t mThreadId;
    mutable CMutex           mLock;
    CCondition       mThreadExitedCondition;
    int        mStatus;
    //note that all accesses of mExitPending and mRunning need to hold mLock
    volatile bool           mExitPending;
    volatile bool           mRunning;
};
#endif

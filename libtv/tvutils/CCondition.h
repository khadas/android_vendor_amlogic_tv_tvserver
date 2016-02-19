/*
reference android api, just linux pthread
*/

#ifndef _TV_UTILS_CONDITION_H
#define _TV_UTILS_CONDITION_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include "CMutex.h"

typedef long long nsecs_t;

class CCondition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    CCondition();
    CCondition(int type);
    ~CCondition();
    // Wait on the condition variable.  Lock the mutex before calling.
    int wait(CMutex &mutex);
    // same with relative timeout
    int waitRelative(CMutex &mutex, long sec);
    // Signal the condition variable, allowing one thread to continue.
    void signal();
    // Signal the condition variable, allowing all threads to continue.
    void broadcast();

private:
    pthread_cond_t mCond;
};



// ---------------------------------------------------------------------------

inline CCondition::CCondition()
{
    pthread_cond_init(&mCond, NULL);
}
inline CCondition::CCondition(int type)
{
    if (type == SHARED) {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&mCond, &attr);
        pthread_condattr_destroy(&attr);
    } else {
        pthread_cond_init(&mCond, NULL);
    }
}
inline CCondition::~CCondition()
{
    pthread_cond_destroy(&mCond);
}
inline int CCondition::wait(CMutex &mutex)
{
    return -pthread_cond_wait(&mCond, &mutex.mMutex);
}
inline int CCondition::waitRelative(CMutex &mutex, long msec)
{
    struct timespec ts;
    long _nanoSec = 1000000000;
    int _sec = msec / 1000;
    long _nsec = (msec - 1000 * _sec) * 1000000;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += _sec;
    ts.tv_nsec += _nsec;
    if (ts.tv_nsec > _nanoSec) {
        ts.tv_nsec %= _nanoSec;
        ts.tv_sec++;
    }
    return -pthread_cond_timedwait(&mCond, &mutex.mMutex, &ts);
}
inline void CCondition::signal()
{
    pthread_cond_signal(&mCond);
}
inline void CCondition::broadcast()
{
    pthread_cond_broadcast(&mCond);
}
#endif

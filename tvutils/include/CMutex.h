/*
reference android api, just linux pthread
*/

#ifndef _TV_UTILS_MUTEX_H
#define _TV_UTILS_MUTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

class CCondition;

/*
 * Simple mutex class.  The implementation is system-dependent.
 *
 * The mutex must be unlocked by the thread that locked it.  They are not
 * recursive, i.e. the same thread can't lock it multiple times.
 */
class CMutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    CMutex();
    CMutex(const char *name);
    CMutex(int type, const char *name = NULL);
    ~CMutex();

    // lock or unlock the mutex
    int    lock();
    void   unlock();

    // lock if possible; returns 0 on success, error otherwise
    int    tryLock();

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class Autolock {
    public:
        inline Autolock(CMutex &mutex) : mLock(mutex)
        {
            mLock.lock();
        }
        inline Autolock(CMutex *mutex) : mLock(*mutex)
        {
            mLock.lock();
        }
        inline ~Autolock()
        {
            mLock.unlock();
        }
    private:
        CMutex &mLock;
    };

private:
    friend class CCondition;

    // A mutex cannot be copied
    CMutex(const CMutex &);
    CMutex      &operator = (const CMutex &);

    pthread_mutex_t mMutex;
};

inline CMutex::CMutex()
{
    pthread_mutex_init(&mMutex, NULL);
}

inline CMutex::CMutex(const char *name __unused)
{
    pthread_mutex_init(&mMutex, NULL);
}

inline CMutex::CMutex(int type, const char *name __unused)
{
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}

inline CMutex::~CMutex()
{
    pthread_mutex_destroy(&mMutex);
}

inline int CMutex::lock()
{
    return -pthread_mutex_lock(&mMutex);
}

inline void CMutex::unlock()
{
    pthread_mutex_unlock(&mMutex);
}

inline int CMutex::tryLock()
{
    return -pthread_mutex_trylock(&mMutex);
}

//typedef CMutex::Autolock AutoMutex;

#endif

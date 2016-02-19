#ifndef ANDROID_SERVERS_TV_TVSERVICE_H
#define ANDROID_SERVERS_TV_TVSERVICE_H

#include <ITvService.h>
//#include <include/Tv.h>
#include <utils/threads.h>
#include <utils/Vector.h>
#include <stdint.h>
#include <tv/CTv.h>
#include "tv/CTvScreenCapture.h"

using namespace android;

#define DEBUG_HEAP_LEAKS 0
#define RESOURCE_NUMS 5

class TvService: public BnTvService , public CTv::TvIObserver, public CTvScreenCapture::TvIObserver {
public:
    class Client: public BnTv {
    public:
        Client(const sp<TvService> &tvService, const sp<ITvClient> &tvClient, pid_t clientPid, CTv *pTv);
        Client();
        virtual ~Client();
        virtual void disconnect();
        virtual status_t connect(const sp<ITvClient> &client);
        virtual status_t lock();
        virtual status_t unlock();
        virtual status_t processCmd(const Parcel &p, Parcel *r);
        virtual status_t createSubtitle(const sp<IMemory> &share_mem);
        virtual status_t createVideoFrame(const sp<IMemory> &share_mem, int iSourceMode, int iCapVideoLayerOnly);

        // our client...
        const sp<ITvClient> &getTvClient() const
        {
            return mTvClient;
        }

        int notifyCallback(const int &msgtype, const Parcel &p);
        String16 mStrName;
        static void notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2, void *user);
    private:
        friend class CTv;
        friend class TvService;
        status_t checkPid();
        static sp<Client> getClientFromCookie(void *user);

        mutable Mutex mLock;
        mutable Condition mReady;
        sp<TvService> mTvService;
        sp<ITvClient> mTvClient;
        pid_t mClientPid;
        int mGetNotifyFlag;
        int mCurProgIndex;
        CTv *mpTv;
        bool mIsStartTv;
        sp<IMemory> mSubBmpBuf;
    };//end client

    static void instantiate();
    static TvService *mTvService;
    static TvService *getIntance()
    {
        if (mTvService == NULL)
            mTvService = new TvService();
        return mTvService;
    }
    virtual sp<ITv> connect(const sp<ITvClient> &tvClient);

    virtual void onTvEvent(const CTvEv &ev);
    void removeClient(const sp<ITvClient> &tvClient);

    Client *mpStartTvClient;
    wp<Client> mpScannerClient;
    wp<Client> mpSubClient;
    Vector< wp<Client> > m_v_Clients;

private:
    TvService();
    virtual ~TvService();
    virtual status_t onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags);
    volatile int32_t mUsers;
    virtual void incUsers();
    virtual void decUsers();
    mutable Mutex mServiceLock;
    static CTv *mpTv;
    CTvScreenCapture mCapVidFrame;

};

#endif

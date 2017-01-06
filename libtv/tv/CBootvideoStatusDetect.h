#ifndef C_BOOT_VIDEO_STATUS_DETECT_H
#define C_BOOT_VIDEO_STATUS_DETECT_H
#include "../../tvutils/include/CThread.h"

class CBootvideoStatusDetect: public CThread {
public:
    CBootvideoStatusDetect();
    ~CBootvideoStatusDetect();
    int startDetect();
    int stopDetect();
    bool isBootvideoStopped();

    class IBootvideoStatusObserver {
    public:
        IBootvideoStatusObserver() {};
        virtual ~IBootvideoStatusObserver() {};
        virtual void onBootvideoRunning() {};
        virtual void onBootvideoStopped() {};
    };
    void setObserver (IBootvideoStatusObserver *pOb)
    {
        mpObserver = pOb;
    };

private:
    bool threadLoop();

    IBootvideoStatusObserver *mpObserver;
    bool mIsRunning;
};
#endif


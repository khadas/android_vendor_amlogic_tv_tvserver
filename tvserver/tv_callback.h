#ifndef TV_CALLBACK
#define TV_CALLBACK
#include "tvapi/android/tv/CTv.h"
class TvCallback : public CTv::TvIObserver {
public:
    TvCallback(void *data)
    {
        mPri = data;
    }
    ~TvCallback()
    {
    }
    void onTvEvent (int32_t msgType, const Parcel &p);
private:
    void *mPri;
};
#endif

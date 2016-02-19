//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvRegion.h
//  @ Date : 2013-11
//  @ Author :
//
//


#if !defined(_CTVREGION_H)
#define _CTVREGION_H

#include "CTvDatabase.h"
#include "CTvChannel.h"
#include <utils/String8.h>
#include <utils/Vector.h>
//  TV 区域
//  可以获得每个区域的频率列表等信息
using namespace android;
class CTvRegion {
public:
    int id;
    String8 name;
    String8 country;
    CTvRegion(CTvDatabase db);
    CTvRegion();
    ~CTvRegion();
    static CTvRegion selectByID();
    static int getChannelListByName(char *name, Vector<sp<CTvChannel> > &vcp);
    static int getChannelListByNameAndFreqRange(char *name, int beginFreq, int endFreq, Vector<sp<CTvChannel> > &vcp);
    static int getLogicNumByNameAndfreq(char *name, int freq);
    void selectByCountry();
    Vector<String8> getAllCountry();
    // 取得该Region的信号源频率参数列表
    // @return 返回信号源频率参数列表
    CTvChannel getChannels();
};

#endif  //_CTVREGION_H

//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvRegion.cpp
//  @ Date : 2013-11
//  @ Author :
//
//
#define LOG_TAG "CTvRegion"

#include "CTvRegion.h"
#include "CTvDatabase.h"

CTvRegion::CTvRegion(CTvDatabase db __unused)
{
}

CTvRegion::CTvRegion()
{
}

CTvRegion::~CTvRegion()
{
}

CTvRegion CTvRegion::selectByID()
{
    CTvRegion r;
    return r;
}

int CTvRegion::getChannelListByName(char *name, Vector<sp<CTvChannel> > &vcp)
{
    if (name == NULL)
        return -1;

    String8 cmd;
    cmd = String8("select * from region_table where name = ") + String8("\'") + name + String8("\'");

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);
    int col, size = 0;
    int id;
    int mode;
    int frequency = 0;
    int bandwidth;
    int modulation;
    int symbolRate;
    int ofdmMode;
    int channelNum = 0;

    if (c.moveToFirst()) {
        do {
            col = c.getColumnIndex("db_id");
            id = c.getInt(col);
            col = c.getColumnIndex("fe_type");
            mode = c.getInt(col);
            col = c.getColumnIndex("frequency");
            frequency = c.getInt(col);
            col = c.getColumnIndex("modulation");
            modulation = c.getInt(col);
            col = c.getColumnIndex("bandwidth");
            bandwidth = c.getInt(col);
            col = c.getColumnIndex("symbol_rate");
            symbolRate = c.getInt(col);
            col = c.getColumnIndex("ofdm_mode");
            ofdmMode = c.getInt(col);
            col = c.getColumnIndex("logical_channel_num");
            channelNum = c.getInt(col);
            vcp.add(new CTvChannel(id, mode, frequency, bandwidth, modulation, symbolRate, ofdmMode, channelNum));
            size++;
        } while (c.moveToNext());
    }
    c.close();

    return size;
}

int CTvRegion::getChannelListByNameAndFreqRange(char *name, int beginFreq, int endFreq, Vector<sp<CTvChannel> > &vcp)
{
    if (name == NULL)
        return -1;
    int ret = 0;
    String8 cmd;
    cmd = String8("select * from region_table where name = ") + String8("\'") + name + String8("\'")
          + String8(" and frequency >= ") + String8::format("%d", beginFreq) + String8(" and frequency <= ")
          + String8::format("%d", endFreq);

    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);
    int col, size = 0;
    int id;
    int mode;
    int frequency = 0;
    int bandwidth;
    int modulation;
    int symbolRate;
    int ofdmMode;
    int channelNum = 0;

    do {
        if (c.moveToFirst()) {
            do {
                col = c.getColumnIndex("db_id");
                id = c.getInt(col);
                col = c.getColumnIndex("fe_type");
                mode = c.getInt(col);
                col = c.getColumnIndex("frequency");
                frequency = c.getInt(col);
                col = c.getColumnIndex("modulation");
                modulation = c.getInt(col);
                col = c.getColumnIndex("bandwidth");
                bandwidth = c.getInt(col);
                col = c.getColumnIndex("symbol_rate");
                symbolRate = c.getInt(col);
                col = c.getColumnIndex("ofdm_mode");
                ofdmMode = c.getInt(col);
                col = c.getColumnIndex("logical_channel_num");
                channelNum = c.getInt(col);
                vcp.add(new CTvChannel(id, mode, frequency, bandwidth, modulation, symbolRate, ofdmMode, channelNum));
                size++;
            } while (c.moveToNext());
        } else {
            ret = -1;
            break;
        }
    } while (false);

    c.close();
    return ret;
}

void CTvRegion::selectByCountry()
{
}

Vector<String8> CTvRegion::getAllCountry()
{
    Vector<String8> vStr;
    return vStr;
}

CTvChannel CTvRegion::getChannels()
{
    CTvChannel p;
    return p;
}

int CTvRegion::getLogicNumByNameAndfreq(char *name, int freq)
{
    int ret = 0;
    int col = 0;

    if (name == NULL)
        return -1;

    String8 cmd;
    cmd = String8("select * from region_table where name = ") + String8("\'") + name + String8("\'");
    cmd += String8(" and frequency = ") + String8::format("%d", freq);


    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);
    if (c.moveToFirst()) {
        col = c.getColumnIndex("logical_channel_num");
        ret = c.getInt(col);
    }
    c.close();

    return ret;
}


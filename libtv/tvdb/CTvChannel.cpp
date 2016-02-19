//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvChannel.cpp
//  @ Date : 2013-11
//  @ Author :
//
//
#define LOG_TAG "CTvChannel"

#include "CTvChannel.h"

void CTvChannel::createFromCursor(CTvDatabase::Cursor &c)
{
    int col;
    int src, freq, mod, symb, bw, satid, satpolar;

    col = c.getColumnIndex("db_id");
    this->id = c.getInt(col);

    col = c.getColumnIndex("ts_id");
    this->dvbTSID = c.getInt(col);

    col = c.getColumnIndex("src");
    src = c.getInt(col);

    col = c.getColumnIndex("freq");
    freq = c.getInt(col);

    if (src == MODE_QAM) {
        col = c.getColumnIndex("mod");
        mod = c.getInt(col);

        col = c.getColumnIndex("symb");
        symb = c.getInt(col);

        frequency  = freq;
        modulation = mod;
        symbolRate = symb;
        mode = MODE_QAM;

    } else if (src == MODE_OFDM) {
        col = c.getColumnIndex("bw");
        bw = c.getInt(col);

        frequency = freq;
        bandwidth = bw;
        mode = MODE_OFDM;

    } else if (src == MODE_ATSC) {
        col = c.getColumnIndex("mod");
        mod = c.getInt(col);

        frequency  = freq;
        modulation = mod;
        mode = MODE_ATSC;
    } else if (src == MODE_ANALOG) {
        col = c.getColumnIndex("std");
        int std = c.getInt(col);
        col = c.getColumnIndex("aud_mode");
        int aud_mode = c.getInt(col);
        col = c.getColumnIndex("flags");
        int afc_flag = c.getInt(col);

        frequency = freq;
        audio     = aud_mode;
        standard  = std;
        afc_data  = afc_flag;
        mode = MODE_ANALOG;
    } else if (src == MODE_QPSK) {
        col = c.getColumnIndex("symb");
        symb = c.getInt(col);

        col = c.getColumnIndex("db_sat_para_id");
        satid = c.getInt(col);

        col = c.getColumnIndex("polar");
        satpolar = c.getInt(col);

        frequency  = freq;
        symbolRate = symb;
        sat_id = satid;
        sat_polarisation = satpolar;
        mode = MODE_QPSK;

        /*new tv_satparams*/
        //showboz
        //TVSatellite sat = TVSatellite.tvSatelliteSelect(sat_id);
        //tv_satparams = sat.getParams();
    } else if (src == MODE_DTMB) {
        col = c.getColumnIndex("bw");
        bw = c.getInt(col);

        frequency = freq;
        bandwidth = bw;
        mode = MODE_DTMB;
    }

    this->fendID = 0;
}

CTvChannel::CTvChannel()
{
}

CTvChannel::CTvChannel(int dbID, int mode, int freq, int bw, int mod, int symb, int ofdm __unused, int channelNum)
{
    //other member not init,just a paras
    if (mode == MODE_QAM) {
        id = dbID;
        frequency = freq;
        modulation = mod;
        symbolRate = symb;
        mode = MODE_QAM;
    } else if (mode == MODE_OFDM) {
        id = dbID;
        frequency = freq;
        bandwidth = bw;
        mode = MODE_OFDM;
    } else if (mode == MODE_ATSC) {
        id = dbID;
        frequency = freq;
        modulation = mod;
        logicalChannelNum = channelNum;
        mod = MODE_ATSC;
    } else if (mode == MODE_ANALOG) {
        id = dbID;
        frequency = freq;
        audio     = 0;
        standard  = 0;
        afc_data  = 0;
        mode = MODE_ANALOG;
    } else if (mode == MODE_DTMB) {
        id = dbID;
        frequency = freq;
        bandwidth = bw;
        mode = MODE_DTMB;
    }
}

CTvChannel::~CTvChannel()
{
}

Vector<CTvChannel> CTvChannel::tvChannelList(int sat_id __unused)
{
    Vector<CTvChannel> v_channel;
    return v_channel;
}

int CTvChannel::selectByID(int cid, CTvChannel &channel)
{
    String8 cmd = String8("select * from ts_table where ts_table.db_id = ") + String8::format("%d", cid);
    CTvDatabase::Cursor c;
    CTvDatabase::GetTvDb()->select(cmd, c);

    if (c.moveToFirst()) {
        channel.createFromCursor(c);
    } else {
        c.close();
        return -1;
    }
    c.close();

    return 0;
}

int CTvChannel::SelectByFreq(int freq, CTvChannel &channel)
{
    String8 cmd ;
    CTvDatabase::Cursor c;
    int iOutRet = 0;

    do {
        cmd = String8("select * from ts_table where ts_table.freq = ") + String8::format("%d", freq);
        CTvDatabase::GetTvDb()->select(cmd, c);

        if (c.moveToFirst()) {
            channel.createFromCursor(c);
        } else {
            iOutRet = -1;
            break;
        }

        cmd = String8("delete  from ts_table where ts_table.freq = ") + String8::format("%d", freq);
        CTvDatabase::GetTvDb()->exeSql(cmd.string());
    } while (false);

    c.close();
    return iOutRet;
}

int CTvChannel::DeleteBetweenFreq(int beginFreq, int endFreq)
{
    String8 cmd ;
    CTvDatabase::Cursor c;
    int iOutRet = 0;
    CTvChannel channel;

    do {
        cmd = String8("select * from ts_table where ts_table.freq >= ") + String8::format("%d", beginFreq)
              + String8("and ts_table.freq <= ") + String8::format("%d", endFreq);
        CTvDatabase::GetTvDb()->select(cmd, c);
        if  (c.moveToFirst()) {
            do {
                channel.createFromCursor(c);
                cmd = String8("delete  from ts_table where ts_table.freq = ") + String8::format("%d", channel.frequency);
                CTvDatabase::GetTvDb()->exeSql(cmd.string());
            } while (c.moveToNext());
        } else {
            iOutRet = -1;
            break;
        }

    } while (false);

    c.close();
    return iOutRet;
}

int CTvChannel::CleanAllChannelBySrc(int src)
{
    String8 cmd = String8("delete from ts_table where src = ") + String8::format("%d", src);
    CTvDatabase::GetTvDb()->exeSql(cmd.string());
    return 0;
}

int CTvChannel::getChannelListBySrc(int src, Vector< sp<CTvChannel> > &v_channel)
{
    CTvDatabase::Cursor c;
    CTvChannel *channel;
    do {
        String8 cmd = String8("select * from ts_table where src = ") + String8::format("%d", src);
        CTvDatabase::GetTvDb()->select(cmd, c);
        if  (c.moveToFirst()) {
            do {
                channel = new CTvChannel();
                channel->createFromCursor(c);
                v_channel.add(channel);
            } while (c.moveToNext());
        } else {
            break;
        }
    } while (false);

    return 0;
}

int CTvChannel::updateByID(int progID, int std, int freq, int fineFreq)
{

    String8 cmd = String8("update ts_table set std = ") + String8::format("%d", std) +
                  String8(", freq = ") + String8::format("%d", freq) +
                  String8(", flags = ") + String8::format("%d", fineFreq) +
                  String8(" where ts_table.db_id = ") + String8::format("%d", progID);
    LOGD("%s, cmd = %s\n", "TV", cmd.string());
    CTvDatabase::GetTvDb()->exeSql(cmd.string());

    return 0;
}

void CTvChannel::tvChannelDel()
{
}

void CTvChannel::tvChannelDelBySatID(int id __unused)
{
}

int CTvChannel::getDVBTSID()
{
    return dvbTSID;
}

void CTvChannel::getDVBOrigNetID()
{
}

void CTvChannel::getFrontendID()
{
}

void CTvChannel::getTSSourceID()
{
}

void CTvChannel::isDVBCMode()
{
}

void CTvChannel::setFrequency()
{
}

void CTvChannel::setSymbolRate()
{
}

void CTvChannel::setPolarisation()
{
}

void CTvChannel::setATVAudio()
{
}

void CTvChannel::setATVVideoFormat()
{
}

void CTvChannel::setATVAudioFormat()
{
}

void CTvChannel::setATVFreq()
{
}

void CTvChannel::setATVAfcData()
{
}


//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvDatabase.cpp
//  @ Date : 2013-11
//  @ Author :
//
#define LOG_TAG "CTvDatabase"

#include <assert.h>
#include <tinyxml.h>
#include "CTvDatabase.h"
#include "tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"

const char *CTvDatabase::DEFAULT_DB_PATH = "/param/tv_default.xml";

const char *CTvDatabase::DB_VERSION_FIELD = "DATABASE_VERSION";
using namespace android;

CTvDatabase *CTvDatabase::mpDb = NULL;

const char CTvDatabase::feTypes[][32] = {"dvbs", "dvbc", "dvbt", "atsc", "analog", "dtmb"};
const char CTvDatabase::srvTypes[][32] = {"other", "dtv", "radio", "atv", "other"};
const char CTvDatabase::vidFmts[][32] = {"mpeg12", "mpeg4", "h264", "mjpeg", "real", "jpeg", "vc1", "avs"};
const char CTvDatabase::audFmts[][32] = {"mpeg", "pcm_s16le", "aac", "ac3", "alaw", "mulaw", "dts", "pcm_s16be",
                                         "flac", "cook", "pcm_u8", "adpcm", "amr", "raac", "wma", "wma_pro",
                                         "pcm_bluray", "alac", "vorbis", "aac_latm", "ape", "eac3", "pcm_wifidisplay"
                                        };
const char CTvDatabase::mods[][32] = {"qpsk", "qam16", "qam32", "qam64", "qam128", "qam256", "qamauto", "vsb8", "vsb16", "psk8", "apsk16", "apsk32", "dqpsk"};
const char CTvDatabase::bandwidths[][32] = {"8", "7", "6", "auto", "5", "10", "1_712"};
const char CTvDatabase::lnbPowers[][32] = {"13v", "18V", "off", "13/18v"};
const char CTvDatabase::sig22K[][32] = {"on", "off", "auto"};
const char CTvDatabase::tonebursts[][32] = {"none", "bursta", "burstb"};
const char CTvDatabase::diseqc10s[][32] = {"lnb1", "lnb2", "lnb3", "lnb4", "none"};
const char CTvDatabase::diseqc11s[][32] = {"lnb1", "lnb2", "lnb3", "lnb4", "lnb5", "lnb6", "lnb7", "lnb8",
                                           "lnb9", "lnb10", "lnb11", "lnb12", "lnb13", "lnb14", "lnb15", "lnb16", "none"
                                          };
const char CTvDatabase::motors[][32] = {"none", "none", "none", "diseqc1.2", "diseqc1.3"};
const char CTvDatabase::ofdmModes[][32] = {"dvbt", "dvbt2"};
const char CTvDatabase::atvVideoStds[][32] = {"auto", "pal", "ntsc", "secam"};
const char CTvDatabase::atvAudioStds[][32] = {"dk", "i", "bg", "m", "l", "auto"};

CTvDatabase::CTvDatabase()
{
}

int CTvDatabase::isFreqListExist()
{
    String8 cmd = String8("select * from region_table");
    CTvDatabase::Cursor c;
    select(cmd, c);
    return c.moveToFirst();
}
int CTvDatabase::UnInitTvDb()
{
    AM_DB_UnSetup();
    closeDb();
    return 0;
}
int CTvDatabase::InitTvDb(const char *path)
{
    if (path != NULL) {
        if (Tv_Utils_IsFileExist(path) && config_get_int("TV", "tv_db_created", 0) == 1) { //exist or created
            LOGD("tv db file(%s) exist and created, open it", path);
            if (openDb(path)  < 0 ) {
                LOGD("db(%s) open fail", path);
                return -1;
            }
            //setup and path set
            AM_DB_Setup((char *)path, getHandle());
            if (isFreqListExist() == false) {
                importXmlToDB("/etc/tv_default.xml");
                LOGD("scan region table is NULL, so import freq XML again\n");
            }
        } else {
            if (Tv_Utils_IsFileExist(path)) { // if just exist, create flag not set, delete it
                LOGD("tv db file (%s) exist, but delete it", path);
                if (unlink(path) != 0) {
                    LOGD("delete tv db file(%s) err=%s", path, strerror(errno));
                }
            }
            LOGD("tv db file(%s) not exist, create it", path);
            //setup and path set
            sqlite3 *h = NULL;
            AM_DB_Setup((char *)path, h);
            //create db
            AM_DB_GetHandle(&h);
            //create table
            AM_DB_CreateTables(h);
            setHandle(h);
            //clear db
            ClearDbTable();
            //insert 256 ATV Program
            //load init date
            importXmlToDB("/etc/tv_default.xml");
            config_set_int("TV", "tv_db_created", 1);
        }

    }
    return 0;
}
//CTvDatabase::CTvDatabase(char* path, sqlite3 * h)
//{
/*if(path != NULL && h != NULL)
{//setup and path set
    AM_DB_Setup((char*)path, h);
    mHandle = h;
}else
{
    mHandle = NULL;
}*/
//}

CTvDatabase::~CTvDatabase()
{
    AM_DB_UnSetup();
}

int CTvDatabase::getChannelParaList(char *path, Vector<sp<ChannelPara> > &vcp)
{
    //?????o?????aXML????????￡?ˉ1è±????
    TiXmlDocument myDocument(path);
    bool ret = myDocument.LoadFile();
    //è?·?????1????′?
    TiXmlElement *RootElement = myDocument.RootElement();
    //dvbc
    TiXmlElement *channel_list_element = RootElement->FirstChildElement("channel_list");
    for (TiXmlElement *channel_entry = channel_list_element->FirstChildElement("channel_entry") ; channel_entry != NULL; channel_entry = channel_entry->NextSiblingElement("channel_entry")) {
        sp<ChannelPara> pCp = new ChannelPara();
        channel_entry->Attribute("frequency", &(pCp->freq));
        channel_entry->Attribute("modulation", &(pCp->modulation));
        channel_entry->Attribute("symbol_rate", &(pCp->symbol_rate));
        vcp.push_back(pCp);
    }
    return vcp.size();
}

int CTvDatabase::ClearDbTable()
{
    LOGD("Clearing database ...");
    exeSql("delete from net_table");
    exeSql("delete from ts_table");
    exeSql("delete from srv_table");
    exeSql("delete from evt_table");
    exeSql("delete from booking_table");
    exeSql("delete from grp_table");
    exeSql("delete from grp_map_table");
    exeSql("delete from dimension_table");
    exeSql("delete from sat_para_table");
    exeSql("delete from region_table");
    return 0;
}

int CTvDatabase::clearDbAllProgramInfoTable()
{
    LOGD("Clearing clearDbAllProgramInfoTable ...");
    exeSql("delete from net_table");
    exeSql("delete from ts_table");
    exeSql("delete from srv_table");
    exeSql("delete from evt_table");
    exeSql("delete from booking_table");
    exeSql("delete from grp_table");
    exeSql("delete from grp_map_table");
    exeSql("delete from dimension_table");
    exeSql("delete from sat_para_table");
    return 0;
}

//showboz now just channellist
int CTvDatabase::importXmlToDB(const char *xmlPath)
{
    //delete region table before importing xml
    exeSql("delete from region_table");

    //?????o?????aXML????????￡?ˉ1è±????
    TiXmlDocument myDocument(xmlPath);
    bool ret = myDocument.LoadFile();

    //è?·?????1????′?
    TiXmlElement *RootElement = myDocument.RootElement();
    beginTransaction();//-----------------------------------------------
    //list-->entry
    for (TiXmlElement *channel_list_element = RootElement->FirstChildElement("channel_list"); channel_list_element != NULL; channel_list_element = channel_list_element->NextSiblingElement("channel_list")) {
        //LOGD("showboz-----channel_list =%d", channel_list_element);
        const char *channel_name = channel_list_element->Attribute("name");
        const char *channel_fe_type = channel_list_element->Attribute("fe_type");
        //LOGD("showboz-----channel_list name = %s type=%s", channel_name, channel_fe_type);

        for (TiXmlElement *channel_entry = channel_list_element->FirstChildElement("channel_entry") ; channel_entry != NULL; channel_entry = channel_entry->NextSiblingElement("channel_entry")) {
            int freq, symb, channelNum;
            String8 cmd = String8("insert into region_table(name,fe_type,frequency,symbol_rate,modulation,bandwidth,ofdm_mode,logical_channel_num)");
            cmd += String8("values('") + channel_name + String8("',") + String8::format("%d", StringToIndex(feTypes, channel_fe_type)) + String8(",");
            channel_entry->Attribute("frequency", &freq);
            cmd += String8::format("%d", freq) + String8(",");
            channel_entry->Attribute("symbol_rate", &symb);
            cmd += String8::format("%d", symb) + String8(",");
            //LOGD("showboz---------m=%s,b=%s,o=%s", channel_entry->Attribute("modulation"), channel_entry->Attribute("bandwidth"), channel_entry->Attribute("ofdm_mode"));
            cmd += String8::format("%d", StringToIndex(mods, channel_entry->Attribute("modulation"))) + String8(",");
            cmd += String8::format("%d", StringToIndex(bandwidths, channel_entry->Attribute("bandwidth"))) + String8(",");
            cmd += String8::format("%d", StringToIndex(ofdmModes, channel_entry->Attribute("ofdm_mode"))) + String8(",");
            channel_entry->Attribute("logical_channel_num", &channelNum);
            cmd += String8::format("%d", channelNum) + String8(")");
            exeSql(cmd.string());
        }
    }

    commitTransaction();//------------------------------------------------------
    return 0;
}

bool CTvDatabase::isAtv256ProgInsertForSkyworth()
{
    String8 select_ts_atvcount = String8("select * from ts_table where src = 4");
    Cursor c;
    select(select_ts_atvcount, c);
    return c.getCount() < 256 ? false : true;
}

int CTvDatabase::insert256AtvProgForSkyworth()
{
    beginTransaction();
    for (int i = 0; i < 256; i++) {
        String8 insert_ts = String8("insert into ts_table(db_id, src, db_net_id, ts_id, freq, symb, mod, bw, snr, ber, strength, db_sat_para_id, polar, std, aud_mode, flags, dvbt_flag) values (");
        insert_ts += String8::format("'%d'", i);
        insert_ts += String8(", '4', '-1', '-1', '44250000', '0', '0', '0', '0', '0', '0', '-1', '-1', '-1', '1', '0', '0')");
        exeSql(insert_ts.string());
        String8 insert_srv = String8("insert into srv_table(db_id, src, db_net_id, db_ts_id, name, service_id, service_type, eit_schedule_flag, eit_pf_flag, running_status, free_ca_mode, volume, aud_track, pmt_pid, vid_pid, vid_fmt, scrambled_flag, current_aud, aud_pids, aud_fmts, aud_langs, aud_types, current_sub, sub_pids, sub_types, sub_composition_page_ids, sub_ancillary_page_ids, sub_langs, current_ttx, ttx_pids, ttx_types, ttx_magazine_nos, ttx_page_nos, ttx_langs, chan_num, skip, lock, favor, lcn, sd_lcn, hd_lcn, default_chan_num, chan_order, lcn_order, service_id_order, hd_sd_order, db_sat_para_id, dvbt2_plp_id, major_chan_num, minor_chan_num, access_controlled, hidden, hide_guide, source_id, sdt_ver) values (");
        insert_srv += String8::format("'%d'", i);
        insert_srv += String8(" , '4', '-1', ");
        insert_srv += String8::format("'%d'", i);
        insert_srv += String8(", 'xxxATV Program', '-1', '3', '-1', '-1', '-1', '-1', '50', '1', '-1', '-1', '-1', '0', '-1', '-1', '-1', 'Audio1', '0', '-1', ' ', ' ', ' ', ' ', ' ', '-1', ' ', ' ', ' ', ' ', ' ', '-1', '1', '0', '0', '-1', '-1', '-1', '-1', ");
        insert_srv += String8::format("'%d'", i);
        insert_srv += String8(" , '0', '0', '0', '-1', '255', '0', '0', '0', '0', '0', '0', '255') ");
        exeSql(insert_srv.string());
    }
    commitTransaction();
    return 0;
}

void CTvDatabase::deleteTvDb()
{
    if (mpDb != NULL) {
        delete mpDb;
        mpDb = NULL;
    }
}
CTvDatabase *CTvDatabase::GetTvDb()
{
    if (mpDb == NULL) {
        mpDb = new CTvDatabase();
    }
    return mpDb;
}

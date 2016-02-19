/*
 * CPQdb.cpp
 *
 *  d on: 2011-12-13
 *      Author: casey
 *
 * change 2015.04
 * showbo
 */

#define LOG_TAG "CPQdb"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "CPQdb.h"
#include "../tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"

#if (PQ_NO_ANDROID == 0)
#include <android/log.h>
#include "CTvLog.h"
#endif

#if (PQ_NO_ANDROID == 1)
using namespace std;
#endif

//#define CPQDB_DEBUG

#define BRIGHTNESS_ID       0
#define CONTRAST_ID         1
#define SATURATION_ID       1
#define HUE_ID              0
#define SHARPNESS_ID        1

//static char *str_buf[256];

#define ID_FIELD            "TableID"
#define NR_LEVEL_NAME       "NRLevel"
#define CM_LEVEL_NAME       "CMLevel"
#define LEVEL_NAME          "Level"
#define ADC_SETTING         "ADC_Settings"
#define CVD2_SETTING        "CVD2_Settings"
#define PORT                "Port"
#define FORMAT              "Format"

#define CC_LINE_MAX_LEN             (1024)

#define LOG_TAG "CPQdb"
//#define CPQDB_DEBUG


/*int get_pq_from_file(const char *file_name, const char *screen_size, char *key_str_buf[]) {
 FILE *fp = NULL;
 int flag = -1;
 char *str = NULL;
 char r_line_buf[CC_LINE_MAX_LEN];

 if (file_name == NULL || screen_size == NULL || key_str_buf == NULL) {
 return -1;
 }

 fp = fopen(file_name, "r");
 if (fp == NULL) {
 LOGE("%s: open file \"%s\" error(%s).\n", "TV", file_name, strerror(errno));
 return -1;
 }

 while (fgets(r_line_buf, CC_LINE_MAX_LEN, fp)) {
 LOGD("%s, fgets_buf = %s\n","TV", r_line_buf);
 if (NULL != strstr(r_line_buf, screen_size)) {
 LOGD("%s, screen_size_buf = %s\n","TV", r_line_buf);
 str = (char*)malloc(strlen(r_line_buf));
 memset(str, 0, strlen(str));
 if (NULL != str) {
 strcpy(str, r_line_buf);
 key_str_buf[0] = str;
 LOGD("%s, key_str_buf[0] = %s\n","TV", key_str_buf[0]);
 }
 flag = 0;
 break;
 }
 }

 if (flag != 0)
 key_str_buf[0] = NULL;

 fclose(fp);
 fp = NULL;

 return flag;
 }*/

/*int getFileSizeSystemCall(char * strFileName)
 {
 struct stat temp;
 stat(strFileName, &temp);
 return temp.st_size;
 }*/

CPqData::CPqData()
{
    int i = 0, j = 0;

    for (i = 0; i < 15; i++) {
        pq_bri_data[i].TotalNode = 0;
        pq_bri_data[i].NodeValue = 0;
        pq_bri_data[i].IndexValue = 0;
        pq_bri_data[i].RegValue = 0;
        pq_bri_data[i].step = 0;

        pq_con_data[i].TotalNode = 0;
        pq_con_data[i].NodeValue = 0;
        pq_con_data[i].IndexValue = 0;
        pq_con_data[i].RegValue = 0;
        pq_con_data[i].step = 0;

        pq_sat_data[i].TotalNode = 0;
        pq_sat_data[i].NodeValue = 0;
        pq_sat_data[i].IndexValue = 0;
        pq_sat_data[i].RegValue = 0;
        pq_sat_data[i].step = 0;

        pq_hue_data[i].TotalNode = 0;
        pq_hue_data[i].NodeValue = 0;
        pq_hue_data[i].IndexValue = 0;
        pq_hue_data[i].RegValue = 0;
        pq_hue_data[i].step = 0;
    }

    for (i = 0; i < 10; i++) {
        pq_sharpness_reg_data[i].length = 0;
        pq_sharpness_reg_data_1[i].length = 0;
        for (j = 0; j < 50; j++) {
            pq_sharpness_reg_data[i].reg_data[j].TotalNode = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].TotalNode = 0;
            pq_sharpness_reg_data[i].reg_data[j].Value.type = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].Value.type = 0;
            pq_sharpness_reg_data[i].reg_data[j].Value.addr = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].Value.addr = 0;
            pq_sharpness_reg_data[i].reg_data[j].Value.mask = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].Value.mask = 0;
            pq_sharpness_reg_data[i].reg_data[j].Value.val = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].Value.val = 0;
            pq_sharpness_reg_data[i].reg_data[j].NodeValue = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].NodeValue = 0;
            pq_sharpness_reg_data[i].reg_data[j].IndexValue = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].IndexValue = 0;
            pq_sharpness_reg_data[i].reg_data[j].step = 0;
            pq_sharpness_reg_data_1[i].reg_data[j].step = 0;
        }
    }
}

CPqData::~CPqData()
{
}

int CPqData::openPqDB(const char *db_path)
{
    int rval;
    project_info_t tmp_info;
    char SYSTEM_PQ[128] = { 0 };
    char tmp_buf[128] = { 0 };

    strcpy(PQ_DB_PATH, db_path);
    strcpy(SYSTEM_PQ, "/system/etc/pq.db");

    if (GetProjectInfo(&tmp_info) == 0) {
        strcpy(tmp_buf, "/system/etc/");
        strcat(tmp_buf, tmp_info.panel_type);
        strcat(tmp_buf, ".db");
        if (access(tmp_buf, 0) == 0) {
            strncpy(SYSTEM_PQ, tmp_buf, sizeof(SYSTEM_PQ) - 1);
        }
    }

    LOGD("openPqDB path = %s   = %s", db_path, PQ_DB_PATH);
    if (access(PQ_DB_PATH, 0) < 0) {
        CFile file(SYSTEM_PQ);
        if (file.copyTo(PQ_DB_PATH) != 0) {
            LOGE("%s, copy file \"%s\" to \"%s\" error", __FUNCTION__, SYSTEM_PQ, PQ_DB_PATH);
            return -1;
        } else {
            LOGD("%s, copy file \"%s\" to \"%s\" ok", __FUNCTION__, SYSTEM_PQ, PQ_DB_PATH);
        }
    }

    closeDb();
    rval = openDb(PQ_DB_PATH);

    char *err;
    bool isOk = integrityCheck();
    if (!isOk) {
        CFile file(SYSTEM_PQ);
        if (file.copyTo(PQ_DB_PATH) != 0) {
            LOGE("%s, copy file = %s , error", __FUNCTION__, SYSTEM_PQ);
            return -1;
        } else {
            closeDb();
            rval = openDb(PQ_DB_PATH);
            LOGD("%s, copy file = %s , ok", __FUNCTION__, SYSTEM_PQ);
        }
    }
    return rval;
}
int CPqData::reopenDB()
{
    int  rval = openDb(PQ_DB_PATH);
    return rval;
}

int CPqData::getRegValues(const char *table_name, tvin_port_t source, tvin_sig_fmt_t signal,
                          is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt __unused, am_regs_t *regs)
{
    CSqlite::Cursor c_tablelist, c_reg_list;
    int index_am_reg = 0;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    if (table_name == NULL || !strlen(table_name)) {
        LOGE("%s, table_name is null\n", __FUNCTION__);
        return index_am_reg;
    }

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select TableName from %s where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", table_name, source, signal, mode);
    this->select(sqlmaster, c_tablelist);
    if (c_tablelist.moveToFirst()) { //for table list
        do {
            getSqlParams(__FUNCTION__, sqlmaster,
                         "select RegType, RegAddr, RegMask, RegValue from %s;",
                         c_tablelist.getString(0).string());
            this->select(sqlmaster, c_reg_list);

            if (c_reg_list.moveToFirst()) { //reg list for each table
                int index_type = 0;//c_reg_list.getColumnIndex("RegType");
                int index_addr = 1;//c_reg_list.getColumnIndex("RegAddr");
                int index_mask = 2;//c_reg_list.getColumnIndex("RegMask");
                int index_val = 3;//c_reg_list.getColumnIndex("RegValue");
                do {
                    regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
                    regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
                    regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
                    regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
                    index_am_reg++;
                } while (c_reg_list.moveToNext());
            }
        } while (c_tablelist.moveToNext());
    }
    //
    regs->length = index_am_reg;
    LOGD("%s, length = %d", __FUNCTION__, regs->length);
    return index_am_reg;
}

int CPqData::getRegValuesByValue(const char *name, const char *f_name, const char *f2_name,
                                 const int val, const int val2, am_regs_t *regs)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256];
    int rval = -1;
    //first  get table name
    if ((strlen(f2_name) == 0) && (val2 == 0))
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                     val);
    else
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);

    rval = this->select(sqlmaster, c_reg_list);
    int count = c_reg_list.getCount();
    if (count > REGS_MAX_NUMBER) {
        LOGD("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        return -1;
    }
    int index_am_reg = 0;
    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;//c_reg_list.getColumnIndex("RegType");
        int index_addr = 1;//c_reg_list.getColumnIndex("RegAddr");
        int index_mask = 2;//c_reg_list.getColumnIndex("RegMask");
        int index_val = 3;//c_reg_list.getColumnIndex("RegValue");
        do {
            regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
            regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
            regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
            regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            index_am_reg++;
        } while (c_reg_list.moveToNext());
    } else
        rval = -1;
    //
    regs->length = index_am_reg;
    LOGD("%s, length = %d", __FUNCTION__, regs->length);
    return rval;
}

int CPqData::getRegValuesByValue_long(const char *name, const char *f_name, const char *f2_name,
                                      const int val, const int val2, am_regs_t *regs, am_regs_t *regs_1)
{
    CSqlite::Cursor c_reg_list;
    char sqlmaster[256];
    int rval = -1;
    //first  get table name
    if ((strlen(f2_name) == 0) && (val2 == 0))
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d;", name, f_name,
                     val);
    else
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select RegType, RegAddr, RegMask, RegValue from %s where %s = %d and %s = %d;",
                     name, f_name, val, f2_name, val2);

    rval = this->select(sqlmaster, c_reg_list);

    int index_am_reg = 0;
    int count = c_reg_list.getCount();
    if (count > 1024) {
        LOGD("%s, regs is too more, in pq.db count = %d", __FUNCTION__, count);
        return -1;
    }
    if (c_reg_list.moveToFirst()) { //reg list for each table
        int index_type = 0;//c_reg_list.getColumnIndex("RegType");
        int index_addr = 1;//c_reg_list.getColumnIndex("RegAddr");
        int index_mask = 2;//c_reg_list.getColumnIndex("RegMask");
        int index_val = 3;//c_reg_list.getColumnIndex("RegValue");
        do {
            if (index_am_reg < 512) {
                regs->am_reg[index_am_reg].type = c_reg_list.getUInt(index_type);
                regs->am_reg[index_am_reg].addr = c_reg_list.getUInt(index_addr);
                regs->am_reg[index_am_reg].mask = c_reg_list.getUInt(index_mask);
                regs->am_reg[index_am_reg].val = c_reg_list.getUInt(index_val);
            } else if (index_am_reg >= 512 && index_am_reg < 1024) {
                regs_1->am_reg[index_am_reg - 512].type = c_reg_list.getUInt(index_type);
                regs_1->am_reg[index_am_reg - 512].addr = c_reg_list.getUInt(index_addr);
                regs_1->am_reg[index_am_reg - 512].mask = c_reg_list.getUInt(index_mask);
                regs_1->am_reg[index_am_reg - 512].val = c_reg_list.getUInt(index_val);
            } else {
            }
            index_am_reg++;
        } while (c_reg_list.moveToNext());
    } else
        rval = -1;
    //
    if (count < 512) {
        regs->length = index_am_reg;
    } else if (index_am_reg >= 512 && index_am_reg < 1024) {
        regs->length = 512;
        regs_1->length = index_am_reg - 512;
    }
    LOGD("%s, length = %d", __FUNCTION__, index_am_reg);
    return rval;
}

int CPqData::PQ_GetBaseColorParams(vpp_color_basemode_t basemode, tvin_port_t source_port,
                                   tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                                   tvin_trans_fmt_t trans_fmt __unused, am_regs_t *regs)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;
    getSqlParams(__FUNCTION__, sqlmaster,
                 "select TableName from GeneralColormanagementTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, sig_fmt, mode);

    this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        rval = getRegValuesByValue(c.getString(index_TableName), CM_LEVEL_NAME, "", (int) basemode,
                                   0, regs);
    }
    return rval;
}

int CPqData::PQ_GetCM2Params(vpp_color_management2_t basemode, tvin_port_t source_port,
                             tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                             tvin_trans_fmt_t trans_fmt __unused, am_regs_t *regs)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;
    if (source_port >= TVIN_PORT_CVBS0 && source_port <= TVIN_PORT_CVBS7) {
        if (source_port != TVIN_PORT_CVBS3) {
            source_port = TVIN_PORT_CVBS1;
        }
    } else if (source_port >= TVIN_PORT_HDMI0 && source_port <= TVIN_PORT_HDMI7) {
        source_port = TVIN_PORT_HDMI0;
    } else if (source_port > TVIN_PORT_DTV || source_port < TVIN_PORT_NULL) {
        source_port = TVIN_PORT_MPEG0;
    }

    if (sig_fmt >= TVIN_SIG_FMT_MAX) {
        sig_fmt = TVIN_SIG_FMT_NULL;
    }

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralCM2Table where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, sig_fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        rval = getRegValuesByValue(c.getString(index_TableName), CM_LEVEL_NAME, "", (int) basemode,
                                   0, regs);
    }
    return rval;
}

int CPqData::PQ_GetNR2Params(vpp_noise_reduction2_mode_t nr_mode, tvin_port_t source_port,
                             tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                             tvin_trans_fmt_t trans_fmt __unused, am_regs_t *regs)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;

    if (source_port >= TVIN_PORT_CVBS0 && source_port <= TVIN_PORT_CVBS7) {
        if (source_port != TVIN_PORT_CVBS3) {
            source_port = TVIN_PORT_CVBS1;
        }
    } else if (source_port >= TVIN_PORT_HDMI0 && source_port <= TVIN_PORT_HDMI7) {
        source_port = TVIN_PORT_HDMI0;
    } else if (source_port > TVIN_PORT_DTV || source_port < TVIN_PORT_NULL) {
        source_port = TVIN_PORT_MPEG0;
    }

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralNR2Table where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, sig_fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        rval = getRegValuesByValue(c.getString(index_TableName), LEVEL_NAME, "", (int) nr_mode, 0,
                                   regs);
    }
    return rval;
}

int CPqData::PQ_GetXVYCCParams(vpp_xvycc_mode_t xvycc_mode, tvin_port_t source_port,
                               tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                               tvin_trans_fmt_t trans_fmt __unused, am_regs_t *regs,
                               am_regs_t *regs_1)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;

    if (source_port >= TVIN_PORT_CVBS0 && source_port <= TVIN_PORT_CVBS7) {
        if (source_port != TVIN_PORT_CVBS3) {
            source_port = TVIN_PORT_CVBS1;
        }
    } else if (source_port >= TVIN_PORT_HDMI0 && source_port <= TVIN_PORT_HDMI7) {
        source_port = TVIN_PORT_HDMI0;
    } else if (source_port > TVIN_PORT_DTV || source_port < TVIN_PORT_NULL) {
        source_port = TVIN_PORT_MPEG0;
    }

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralXVYCCTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, sig_fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        rval = getRegValuesByValue_long(c.getString(index_TableName), LEVEL_NAME, "",
                                        (int) xvycc_mode, 0, regs, regs_1);
    }
    return rval;
}

int CPqData::PQ_GetMCDIParams(vpp_mcdi_mode_t mcdi_mode, tvin_port_t source_port,
                              tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                              tvin_trans_fmt_t trans_fmt __unused, am_regs_t *regs)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;

    if (source_port >= TVIN_PORT_CVBS0 && source_port <= TVIN_PORT_CVBS7) {
        if (source_port != TVIN_PORT_CVBS3) {
            source_port = TVIN_PORT_CVBS1;
        }
    } else if (source_port >= TVIN_PORT_HDMI0 && source_port <= TVIN_PORT_HDMI7) {
        source_port = TVIN_PORT_HDMI0;
    } else if (source_port > TVIN_PORT_DTV || source_port < TVIN_PORT_NULL) {
        source_port = TVIN_PORT_MPEG0;
    }

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralMCDITable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, sig_fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        rval = getRegValuesByValue(c.getString(index_TableName), LEVEL_NAME, "", (int) mcdi_mode,
                                   0, regs);
    }
    return rval;
}

int CPqData::PQ_GetDeblockParams(vpp_deblock_mode_t deb_mode, tvin_port_t source_port,
                                 tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                                 tvin_trans_fmt_t trans_fmt __unused, am_regs_t *regs)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralDeblockTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, sig_fmt, is2dOr3d);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        rval = getRegValuesByValue(c.getString(0), LEVEL_NAME, "", (int)deb_mode, 0, regs);
    }
    return rval;
}

int CPqData::PQ_GetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode,
        tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt,
        tcon_rgb_ogo_t *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];

    int rval = -1;

    //default
    params->en = 1;
    params->r_pre_offset = 0;
    params->g_pre_offset = 0;
    params->b_pre_offset = 0;
    params->r_gain = 1024;
    params->g_gain = 1024;
    params->b_gain = 1024;
    params->r_post_offset = 0;
    params->g_post_offset = 0;
    params->b_post_offset = 0;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralWhiteBalanceTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d;", source_port, sig_fmt, trans_fmt);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "select Enable, R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset  from %s where "
            "Level = %d and def = 0;", c.getString(index_TableName).string(),
            (int) Tempmode);

        rval = this->select(sqlmaster, c);

        if (c.moveToFirst()) {
            params->en = c.getInt(0);//index 0
            params->r_pre_offset = c.getInt(1);
            params->g_pre_offset = c.getInt(2);
            params->b_pre_offset = c.getInt(3);
            params->r_gain = c.getInt(4);
            params->g_gain = c.getInt(5);
            params->b_gain = c.getInt(6);
            params->r_post_offset = c.getInt(7);
            params->g_post_offset = c.getInt(8);
            params->b_post_offset = c.getInt(9);//index 9
        }
    }
    return rval;
}
int CPqData::PQ_SetColorTemperatureParams(vpp_color_temperature_mode_t Tempmode,
        tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, tvin_trans_fmt_t trans_fmt,
        tcon_rgb_ogo_t params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    char sql[512];

    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralWhiteBalanceTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d;", source_port, sig_fmt, trans_fmt);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        getSqlParams(
            __FUNCTION__,
            sql,
            "update %s set Enable = %d, "
            "R_Pre_Offset = %d, G_Pre_Offset = %d, B_Pre_Offset = %d, R_Gain = %d, G_Gain = %d, B_Gain = %d, "
            "R_Post_Offset = %d, G_Post_Offset = %d, B_Post_Offset = %d  where Level = %d and def = 0;",
            c.getString(index_TableName).string(), params.en, params.r_pre_offset,
            params.g_pre_offset, params.b_pre_offset, params.r_gain, params.g_gain,
            params.b_gain, params.r_post_offset, params.g_post_offset, params.b_post_offset,
            Tempmode);

        rval = this->exeSql(sql);
    }
    return rval;
}

int CPqData::PQ_ResetAllColorTemperatureParams(void)
{
    CSqlite::Cursor c;
    char sqlmaster[512];

    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select distinct TableName from GeneralWhiteBalanceTable ;");

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        do { //delete
            getSqlParams(
                __FUNCTION__,
                sqlmaster,
                "delete from %s where def = 0;"
                "insert into %s( Level , Enable , R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset, def ) "
                "select Level, Enable, R_Pre_Offset, G_Pre_Offset, B_Pre_Offset, R_Gain, G_Gain, B_Gain, R_Post_Offset, G_Post_Offset, B_Post_Offset, 0 from %s where def = 1;",
                c.getString(index_TableName).string(), c.getString(index_TableName).string(),
                c.getString(index_TableName).string());
            rval = this->exeSql(sqlmaster);
        } while (c.moveToNext());
    }
    return rval;
}

int CPqData::PQ_GetDNLPParams(tvin_port_t source_port, tvin_sig_fmt_t fmt, is_3d_type_t is2dOr3d,
                              tvin_trans_fmt_t trans_fmt __unused, ve_dnlp_t *params,
                              ve_dnlp_table_t *newParams, int *dnlpFlag)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;
    //default
    params->en = 0;
    params->rt = VE_DNLP_RT_0S;
    params->rl = VE_DNLP_RL_01;
    params->black = VE_DNLP_EXT_00;
    params->white = VE_DNLP_EXT_00;

    mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);//(status << 16)|trans_fmt;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralDNLPTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        getSqlParams(__FUNCTION__, sqlmaster, "select value  from %s ",
                     c.getString(index_TableName).string());

        rval = this->select(sqlmaster, c);
        int count = c.getCount();
        if (count == 1) {
            int val;
            c.moveToFirst();
            val = c.getInt(0);//index 0
            *dnlpFlag = 0;
            params->en = (val & 0xf0000000) >> 28;
            params->rt = (ve_dnlp_rt_e)((val & 0x0f000000) >> 24);
            params->rl = (ve_dnlp_rl_e)((val & 0x00ff0000) >> 16);
            params->black = (ve_dnlp_ext_e)((val & 0x0000ff00) >> 8);
            params->white = (ve_dnlp_ext_e)((val & 0x000000ff) >> 0);
        } else if (count > 1) {
            c.moveToFirst();
            c.moveToNext();
            *dnlpFlag = 1;
            newParams->en = c.getInt(0);
            c.moveToNext();
            newParams->method = c.getInt(0);
            c.moveToNext();
            newParams->cliprate = c.getInt(0);
            c.moveToNext();
            newParams->lowrange = c.getInt(0);
            c.moveToNext();
            newParams->hghrange = c.getInt(0);
            c.moveToNext();
            newParams->lowalpha = c.getInt(0);
            c.moveToNext();
            newParams->midalpha = c.getInt(0);
            c.moveToNext();
            newParams->hghalpha = c.getInt(0);
            c.moveToNext();
            newParams->adj_level = c.getInt(0);
            c.moveToNext();
            newParams->new_adj_level = c.getInt(0);
            c.moveToNext();
            newParams->new_mvreflsh = c.getInt(0);
            c.moveToNext();
            newParams->new_gmma_rate = c.getInt(0);
            c.moveToNext();
            newParams->new_lowalpha = c.getInt(0);
            c.moveToNext();
            newParams->new_hghalpha = c.getInt(0);
            c.moveToNext();
            newParams->new_sbgnbnd = c.getInt(0);
            c.moveToNext();
            newParams->new_sendbnd = c.getInt(0);
            c.moveToNext();
            newParams->new_clashBgn = c.getInt(0);
            c.moveToNext();
            newParams->new_clashEnd = c.getInt(0);
            c.moveToNext();
            newParams->new_cliprate = c.getInt(0);
            c.moveToNext();
            newParams->new_mtdbld_rate = c.getInt(0);
            c.moveToNext();
            newParams->new_blkgma_rate = c.getInt(0);
        }
    }
    return rval;
}

int CPqData::PQ_SetNoLineAllBrightnessParams(tv_source_input_type_t source_type, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_BRIGHTNESS, source_type, osd0, osd25, osd50, osd75, osd100);
}

int CPqData::PQ_GetNoLineAllBrightnessParams(tv_source_input_type_t source_type, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval;
    rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_BRIGHTNESS, source_type, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        LOGE("PQ_GetNoLineAllBrightnessParams Error %d\n", rval);
    }
    return rval;

}

int CPqData::PQ_GetBrightnessParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt __unused,
                                    is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                    int level, int *params)
{
    int val;

    GetNonlinearMapping(TVPQ_DATA_BRIGHTNESS, source_port, level, &val);
    *params = CaculateLevelParam(pq_bri_data, bri_nodes, val);
    return 0;

}

int CPqData::PQ_SetBrightnessParams(tvin_port_t source_port __unused, tvin_sig_fmt_t sig_fmt __unused,
                                    is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                    int level __unused, int params __unused)
{
    return 0;
}

int CPqData::PQ_SetNoLineAllContrastParams(tv_source_input_type_t source_type, int osd0, int osd25,
        int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_CONTRAST, source_type, osd0, osd25, osd50, osd75, osd100);
}

int CPqData::PQ_GetNoLineAllContrastParams(tv_source_input_type_t source_type, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval;
    rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_CONTRAST, source_type, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        LOGE("PQ_GetNoLineAllContrastParams Error %d\n", rval);
    }
    return rval;
}

int CPqData::PQ_GetContrastParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt __unused,
                                  is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                  int level, int *params)
{
    int val;

    GetNonlinearMapping(TVPQ_DATA_CONTRAST, source_port, level, &val);
    *params = CaculateLevelParam(pq_con_data, con_nodes, val);
    return 0;
}

int CPqData::PQ_SetContrastParams(tvin_port_t source_port __unused, tvin_sig_fmt_t sig_fmt __unused,
                                  is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                  int level __unused, int params __unused)
{
    return 0;
}

int CPqData::PQ_SetNoLineAllSaturationParams(tv_source_input_type_t source_type, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_SATURATION, source_type, osd0, osd25, osd50, osd75, osd100);
}

int CPqData::PQ_GetNoLineAllSaturationParams(tv_source_input_type_t source_type, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval;
    rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_SATURATION, source_type, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        LOGE("PQ_GetNoLineAllSaturationParams Error %d\n", rval);
    }
    return rval;
}

int CPqData::PQ_GetSaturationParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt __unused,
                                    is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                    int level, int *params)
{
    int val;

    GetNonlinearMapping(TVPQ_DATA_SATURATION, source_port, level, &val);
    *params = CaculateLevelParam(pq_sat_data, sat_nodes, val);
    return 0;
}

int CPqData::PQ_SetSaturationParams(tvin_port_t source_port __unused, tvin_sig_fmt_t sig_fmt __unused,
                                    is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                    int level __unused, int params __unused)
{
    return 0;
}

int CPqData::PQ_SetNoLineAllHueParams(tv_source_input_type_t source_type, int osd0, int osd25,
                                      int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_HUE, source_type, osd0, osd25, osd50, osd75, osd100);
}

int CPqData::PQ_GetNoLineAllHueParams(tv_source_input_type_t source_type, int *osd0, int *osd25,
                                      int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval;
    rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_HUE, source_type, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        LOGE("PQ_GetNoLineAllHueParams Error %d\n", rval);
    }
    return rval;
}

int CPqData::PQ_GetHueParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt __unused,
                             is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                             int level, int *params)
{
    int val;

    GetNonlinearMapping(TVPQ_DATA_HUE, source_port, level, &val);
    *params = CaculateLevelParam(pq_hue_data, hue_nodes, val);
    return 0;
}

int CPqData::PQ_SetHueParams(tvin_port_t source_port __unused, tvin_sig_fmt_t sig_fmt __unused,
                             is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                             int level __unused, int params __unused)
{
    return 0;
}

int CPqData::PQ_SetNoLineAllSharpnessParams(tv_source_input_type_t source_type, int osd0,
        int osd25, int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_type, osd0, osd25, osd50, osd75, osd100);
}

int CPqData::PQ_GetNoLineAllSharpnessParams(tv_source_input_type_t source_type, int *osd0,
        int *osd25, int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval;
    rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_SHARPNESS, source_type, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        LOGE("PQ_GetNoLineAllSharpnessParams Error %d\n", rval);
    }
    return rval;
}

int CPqData::PQ_GetSharpnessParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt __unused,
                                   is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                   int level, am_regs_t *regs, am_regs_t *regs_l)
{
    int val;

    GetNonlinearMapping(TVPQ_DATA_SHARPNESS, source_port, level, &val);
    LOGD("val = %d\n", val);
    *regs = CaculateLevelRegsParam(pq_sharpness_reg_data, val, 0);
    if (sha_diff_flag == 6) {
        *regs_l = CaculateLevelRegsParam(pq_sharpness_reg_data_1, val, 1);
    }
    return 0;
}

int CPqData::PQ_GetPLLParams(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, am_regs_t *regs)
{
    int ret = -1;
    int i = 0;

    ret = getRegValuesByValue(ADC_SETTING, PORT, FORMAT, source_port, sig_fmt, regs);
#ifdef  CPQDB_DEBUG
    if (ret == 0) {
        for (i = 0; i < regs->length; i++) {
            LOGD("%s, =================================================", "TV");
            LOGD("%s, regData.am_reg[%d].type = %d", "TV", i, regs->am_reg[i].type);
            LOGD("%s, regData.am_reg[%d].addr = %d", "TV", i, regs->am_reg[i].addr);
            LOGD("%s, regData.am_reg[%d].mask = %d", "TV", i, regs->am_reg[i].mask);
            LOGD("%s, regData.am_reg[%d].val  = %d", "TV", i, regs->am_reg[i].val);
        }
    }
#endif
    if (regs->am_reg[0].val == 0 && regs->am_reg[1].val == 0 && regs->am_reg[2].val == 0
            && regs->am_reg[3].val == 0) {
        LOGE("%s,db's value is all zeros, that's not OK!!!\n", "TV");
        return -1;
    }
    return ret;
}

int CPqData::PQ_GetCVD2Params(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, am_regs_t *regs)
{
    int ret = -1;
    int i = 0;
    ret = getRegValuesByValue(CVD2_SETTING, PORT, FORMAT, source_port, sig_fmt, regs);
#ifdef  CPQDB_DEBUG
    if (ret == 0) {
        for (i = 0; i < regs->length; i++) {
            LOGD("%s, =================================================", "TV");
            LOGD("%s, regData.am_reg[%d].type = %d", "TV", i, regs->am_reg[i].type);
            LOGD("%s, regData.am_reg[%d].addr = %d", "TV", i, regs->am_reg[i].addr);
            LOGD("%s, regData.am_reg[%d].mask = %d", "TV", i, regs->am_reg[i].mask);
            LOGD("%s, regData.am_reg[%d].val  = %d", "TV", i, regs->am_reg[i].val);
        }
    }
#endif
    if (regs->am_reg[0].val == 0 && regs->am_reg[1].val == 0 && regs->am_reg[2].val == 0
            && regs->am_reg[3].val == 0) {
        LOGE("%s,db's value is all zeros, that's not OK!!!\n", "TV");
        return -1;
    }

    return ret;
}

int CPqData::PQ_SetSharpnessParams(tvin_port_t source_port __unused, tvin_sig_fmt_t sig_fmt __unused,
                                   is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                   int level __unused, am_regs_t regs __unused)
{
    return 0;
}

int CPqData::PQ_SetNoLineAllVolumeParams(tv_source_input_type_t source_type, int osd0, int osd25,
        int osd50, int osd75, int osd100)
{
    return SetNonlinearMapping(TVPQ_DATA_VOLUME, source_type, osd0, osd25, osd50, osd75, osd100);
}

int CPqData::PQ_GetNoLineAllVolumeParams(tv_source_input_type_t source_type, int *osd0, int *osd25,
        int *osd50, int *osd75, int *osd100)
{
    int osdvalue[5] = { 0 };
    int rval;
    rval = GetNonlinearMappingByOSDFac(TVPQ_DATA_VOLUME, source_type, osdvalue);
    *osd0 = osdvalue[0];
    *osd25 = osdvalue[1];
    *osd50 = osdvalue[2];
    *osd75 = osdvalue[3];
    *osd100 = osdvalue[4];
    if (rval) {
        LOGE("PQ_GetNoLineAllSharpnessParams Error %d\n", rval);
    }
    return rval;
}

int CPqData::PQ_SetVolumeParams(tvin_port_t source_port __unused, tvin_sig_fmt_t sig_fmt __unused,
                                is_3d_type_t is2dOr3d __unused, tvin_trans_fmt_t trans_fmt __unused,
                                int level __unused, int params __unused)
{
    return 0;
}

int CPqData::PQ_ResetAllNoLineParams(void)
{
    int rval;
    char sqlmaster[256];
    char *err = NULL;

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from NonlinearMapping; "
        "insert into NonlinearMapping(TVIN_PORT, Item_ID, Level, Value) select TVIN_PORT, Item_ID, Level, Value from NonlinearMapping_Default;");

    rval = this->exeSql(sqlmaster);
    return rval;
}

int CPqData::PQ_GetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode,
                                        tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                                        tvin_trans_fmt_t trans_fmt __unused, int *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralNoiseReductionTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_port, sig_fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        getSqlParams(__FUNCTION__, sqlmaster, "select NRValue from %s where NRLevel = %d;",
                     c.getString(index_TableName).string(), (int) nr_mode);

        rval = this->select(sqlmaster, c);

        if (c.moveToFirst()) {
            *params = c.getInt(0);
        }
    }
    return rval;
}

int CPqData::PQ_SetNoiseReductionParams(vpp_noise_reduction_mode_t nr_mode __unused,
                                        tvin_port_t source_port __unused, tvin_sig_fmt_t sig_fmt __unused,
                                        is_3d_type_t is2dOr3d __unused,
                                        tvin_trans_fmt_t trans_fmt __unused, int *params __unused)
{
    return 0;
}

int CPqData::PQ_GetOverscanParams(tv_source_input_type_t source_type, tvin_sig_fmt_t fmt,
                                  is_3d_type_t is2dOr3d,
                                  tvin_trans_fmt_t trans_fmt __unused, vpp_display_mode_t dmode __unused,
                                  tvin_cutwin_t *cutwin_t)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;

    cutwin_t->hs = 0;
    cutwin_t->he = 0;
    cutwin_t->vs = 0;
    cutwin_t->ve = 0;

    getSqlParams(__FUNCTION__, sqlmaster, "select Hs, He, Vs, Ve from OVERSCAN where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d and "
                 "TVIN_TRANS_FMT = %d ;", source_type, fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        cutwin_t->hs = c.getInt(0);
        cutwin_t->he = c.getInt(1);
        cutwin_t->vs = c.getInt(2);
        cutwin_t->ve = c.getInt(3);
    }
    return rval;
}
int CPqData::PQ_SetOverscanParams(tv_source_input_type_t source_type, tvin_sig_fmt_t fmt,
                                  is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt __unused, tvin_cutwin_t cutwin_t)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int mode = is2dOr3d;//Check2Dor3D(status, trans_fmt);
    int rval = -1;

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "select * from OVERSCAN where TVIN_PORT = %d and TVIN_SIG_FMT = %d and TVIN_TRANS_FMT = %d;",
        source_type, fmt, mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "update OVERSCAN set Hs = %d, He = %d, Vs = %d, Ve = %d where TVIN_PORT = %d and TVIN_SIG_FMT = %d and TVIN_TRANS_FMT = %d;",
            cutwin_t.hs, cutwin_t.he, cutwin_t.vs, cutwin_t.ve, source_type, fmt, mode);
    } else {
        getSqlParams(
            __FUNCTION__,
            sqlmaster,
            "Insert into OVERSCAN(TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, Hs, He, Vs, Ve) values(%d, %d, %d ,%d ,%d, %d, %d);",
            source_type, fmt, mode, cutwin_t.hs, cutwin_t.he, cutwin_t.vs, cutwin_t.ve);
    }
    rval = this->exeSql(sqlmaster);

    return rval;
}

int CPqData::PQ_ResetAllOverscanParams(void)
{
    int rval;
    char sqlmaster[256];

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from OVERSCAN; insert into OVERSCAN(TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, hs, he, vs, ve) select TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, hs, he, vs, ve from OVERSCAN_default;");
    rval = this->exeSql(sqlmaster);

    return rval;
}

int CPqData::PQ_GetPQModeParams(tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode,
                                vpp_pq_para_t *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];

    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster,
                 "select Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR from Picture_Mode where "
                 "TVIN_PORT = %d and "
                 "Mode = %d ;", source_type, pq_mode);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        params->brightness = c.getInt(0);
        params->contrast = c.getInt(1);
        params->saturation = c.getInt(2);
        params->hue = c.getInt(3);
        params->sharpness = c.getInt(4);
        params->backlight = c.getInt(5);
        params->nr = c.getInt(6);
    }
    return rval;
}

int CPqData::PQ_SetPQModeParams(tv_source_input_type_t source_type, vpp_picture_mode_t pq_mode,
                                vpp_pq_para_t *params)
{
    int rval;
    char sql[256];

    getSqlParams(
        __FUNCTION__,
        sql,
        "update Picture_Mode set Brightness = %d, Contrast = %d, Saturation = %d, Hue = %d, Sharpness = %d, Backlight = %d, NR= %d "
        " where TVIN_PORT = %d and Mode = %d;", params->brightness, params->contrast,
        params->saturation, params->hue, params->sharpness, params->backlight, params->nr,
        source_type, pq_mode);
    rval = this->exeSql(sql);
    return 0;
}

int CPqData::PQ_SetPQModeParamsByName(const char *name, tv_source_input_type_t source_type,
                                      vpp_picture_mode_t pq_mode, vpp_pq_para_t *params)
{
    int rval;
    char sql[256];

    getSqlParams(__FUNCTION__, sql,
                 "insert into %s(TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR)"
                 " values(%d,%d,%d,%d,%d,%d,%d,%d,%d);", name, source_type, pq_mode,
                 params->brightness, params->contrast, params->saturation, params->hue,
                 params->sharpness, params->backlight, params->nr);

    rval = this->exeSql(sql);
    return 0;
}

int CPqData::PQ_ResetAllPQModeParams(void)
{
    int rval;
    char sqlmaster[256];

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "delete from picture_mode; insert into picture_mode(TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR) select TVIN_PORT, Mode, Brightness, Contrast, Saturation, Hue, Sharpness, Backlight, NR from picture_mode_default;");

    rval = this->exeSql(sqlmaster);
    return rval;
}

int CPqData::PQ_GetGammaSpecialTable(int gammaValue, const char *f_name,
                                     tcon_gamma_table_t *gamma_value)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select %s from GAMMA_%d", f_name, gammaValue);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        int index = 0;
        do {
            gamma_value->data[index] = c.getInt(0);
            index++;
        } while (c.moveToNext());
    }
    return rval;
}

int CPqData::PQ_GetGammaTableR(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt,
                               tcon_gamma_table_t *gamma_r)
{
    return PQ_GetGammaTable(panel_id, source_port, fmt, "Red", gamma_r);
}

int CPqData::PQ_GetGammaTableG(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt,
                               tcon_gamma_table_t *gamma_g)
{
    return PQ_GetGammaTable(panel_id, source_port, fmt, "Green", gamma_g);
}

int CPqData::PQ_GetGammaTableB(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt,
                               tcon_gamma_table_t *gamma_b)
{
    return PQ_GetGammaTable(panel_id, source_port, fmt, "Blue", gamma_b);
}

int CPqData::PQ_GetGammaTable(int panel_id, tvin_port_t source_port, tvin_sig_fmt_t fmt,
                              const char *f_name, tcon_gamma_table_t *val)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    panel_id = panel_id;

    getSqlParams(__FUNCTION__, sqlmaster, "select TableName from GeneralGammaTable where "
                 "TVIN_PORT = %d and "
                 "TVIN_SIG_FMT = %d;", source_port, fmt);

    rval = this->select(sqlmaster, c);
    if (c.moveToFirst()) {
        int index_TableName = 0;//c.getColumnIndex("TableName");
        getSqlParams(__FUNCTION__, sqlmaster, "select %s from %s;", f_name,
                     c.getString(index_TableName).string());

        rval = this->select(sqlmaster, c);
        if (c.moveToFirst()) {
            int index = 0;
            do {
                val->data[index] = c.getInt(0);
                index++;
            } while (c.moveToNext());
        }
    }
    return rval;
}

int CPqData::PQ_GetVGAAjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t *adjparam)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    adjparam->clk_step = 0;
    adjparam->phase = 0;
    adjparam->hpos_step = 0;
    adjparam->vpos_step = 0;
    adjparam->vga_in_clean = 0;

    getSqlParams(
        __FUNCTION__,
        sqlmaster,
        "select Clk, Phase, HPos, VPos, Vga_in_clean from VGA_AutoParams where TVIN_SIG_FMT = %d",
        vga_fmt);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        adjparam->clk_step = c.getInt(0);
        adjparam->phase = c.getInt(1);
        adjparam->hpos_step = c.getInt(2);
        adjparam->vpos_step = c.getInt(3);
        adjparam->vga_in_clean = c.getInt(4);
    }
    return rval;
}

int CPqData::PQ_SetVGAAjustPara(tvin_sig_fmt_t vga_fmt, tvafe_vga_parm_t adjparam)
{
    CSqlite::Cursor c;
    char sql[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sql, "select * from VGA_AutoParams where TVIN_SIG_FMT = %d;",
                 vga_fmt);

    rval = this->select(sql, c);

    if (c.moveToFirst()) {
        getSqlParams(
            __FUNCTION__,
            sql,
            "Insert into VGA_AutoParams(TVIN_SIG_FMT, Clk, Phase, HPos, VPos, Vga_in_clean) values(%d, %d, %d ,%d ,%d, %d);",
            vga_fmt, adjparam.clk_step, adjparam.phase, adjparam.hpos_step, adjparam.vpos_step,
            adjparam.vga_in_clean);
    } else {
        getSqlParams(
            __FUNCTION__,
            sql,
            "update VGA_AutoParams set Clk = %d, Phase = %d, HPos = %d, VPos = %d, Vga_in_clean = %d where TVIN_SIG_FMT = %d;",
            adjparam.clk_step, adjparam.phase, adjparam.hpos_step, adjparam.vpos_step,
            adjparam.vga_in_clean, vga_fmt);
    }
    rval = this->exeSql(sql);
    return rval;
}

int CPqData::CaculateLevelParam(tvpq_data_t *pq_data, int nodes, int level)
{
    int i;

    for (i = 0; i < nodes; i++) {
        if (level < pq_data[i].IndexValue) {
            break;
        }
    }

    if (i == 0) {
        return pq_data[i].RegValue;
    } else if (i == nodes) {
        return pq_data[i - 1].RegValue;
    } else {
        return pq_data[i - 1].RegValue + (level - pq_data[i - 1].IndexValue) * pq_data[i - 1].step;
    }
}

am_regs_t CPqData::CaculateLevelRegsParam(tvpq_sharpness_regs_t *pq_regs, int level, int flag)
{
    am_regs_t regs;
    int i;
    int *pq_nodes = NULL;
    if (flag && sha_diff_flag == 6) {
        pq_nodes = &sha_nodes_1;
    } else {
        pq_nodes = &sha_nodes;
    }

    for (i = 0; i < *pq_nodes; i++) {
        if (level < pq_regs[i].reg_data[0].IndexValue) {
            break;
        }
    }

    if (i == 0) {
        regs.length = pq_regs[i].length;
        for (int j = 0; j < pq_regs[i].length; j++) {
            regs.am_reg[j].type = pq_regs[i].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i].reg_data[j].Value.val;
        }

    } else if (i == *pq_nodes) {
        regs.length = pq_regs[i - 1].length;
        for (int j = 0; j < pq_regs[i - 1].length; j++) {
            regs.am_reg[j].type = pq_regs[i - 1].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i - 1].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i - 1].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i - 1].reg_data[j].Value.val;
        }
    } else {
        regs.length = pq_regs[i - 1].length;
        for (int j = 0; j < pq_regs[i - 1].length; j++) {
            regs.am_reg[j].type = pq_regs[i - 1].reg_data[j].Value.type;
            regs.am_reg[j].addr = pq_regs[i - 1].reg_data[j].Value.addr;
            regs.am_reg[j].mask = pq_regs[i - 1].reg_data[j].Value.mask;
            regs.am_reg[j].val = pq_regs[i - 1].reg_data[j].Value.val + (level
                                 - pq_regs[i - 1].reg_data[j].IndexValue) * pq_regs[i - 1].reg_data[j].step;
        }
    }

    return regs;
}

int CPqData::GetNonlinearMapping(tvpq_data_type_t data_type, tvin_port_t source_port, int level,
                                 int *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    tv_source_input_type_t type;
    //showboz
    type = CTvin::Tvin_SourcePortToSourceInputType(source_port);//???

    getSqlParams(__FUNCTION__, sqlmaster, "select Value from NonlinearMapping where "
                 "TVIN_PORT = %d and "
                 "Item_ID = %d and "
                 "Level = %d ;", type, data_type, level);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        *params = c.getInt(0);
    }
    return rval;
}

int CPqData::GetNonlinearMappingByOSDFac(tvpq_data_type_t data_type,
        tv_source_input_type_t source_type, int *params)
{
    CSqlite::Cursor c;
    char sqlmaster[256];
    int rval = -1;

    getSqlParams(__FUNCTION__, sqlmaster, "select Value from NonlinearMapping where "
                 "TVIN_PORT = %d and "
                 "Item_ID = %d and ("
                 "Level = 0 or Level = 25 or Level = 50 or Level = 75 or Level = 100);", source_type,
                 data_type);

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        params[0] = c.getInt(0);
        params[1] = c.getInt(1);
        params[2] = c.getInt(2);
        params[3] = c.getInt(3);
        params[4] = c.getInt(4);
    }
    return rval;
}

int CPqData::SetNonlinearMapping(tvpq_data_type_t data_type, tv_source_input_type_t source_type,
                                 int osd0, int osd25, int osd50, int osd75, int osd100)
{

    int rval;
    char *err = NULL;
    int osdvalue[101];
    double step[4];
    char sql[256];

    step[0] = (osd25 - osd0) / 25.0;
    step[1] = (osd50 - osd25) / 25.0;
    step[2] = (osd75 - osd50) / 25.0;
    step[3] = (osd100 - osd75) / 25.0;

    for (int i = 0; i <= 100; i++) {
        if (i == 0) {
            osdvalue[i] = osd0;
        } else if ((i > 0) && (i <= 25)) {
            osdvalue[i] = osd0 + (int) (i * step[0]);
        } else if ((i > 25) && (i <= 50)) {
            osdvalue[i] = osd25 + (int) ((i - 25) * step[1]);
        } else if ((i > 50) && (i <= 75)) {
            osdvalue[i] = osd50 + (int) ((i - 50) * step[2]);
        } else if ((i > 75) && (i <= 100)) {
            osdvalue[i] = osd75 + (int) ((i - 75) * step[3]);
        }
        getSqlParams(
            __FUNCTION__,
            sql,
            "update NonLinearMapping set Value = %d where TVIN_PORT = %d and Item_ID = %d and Level = %d ;",
            osdvalue[i], source_type, data_type, i);
        if (!(this->exeSql(sql))) {
            return -1;
        }
    }
    return 0;
}

int CPqData::SetNonlinearMappingByName(const char *name, tvpq_data_type_t data_type,
                                       tv_source_input_type_t source_type, int osd0, int osd25, int osd50, int osd75, int osd100)
{
    int rval;
    char *err = NULL;
    int osdvalue[101];
    double step[4];
    char sql[256];

    step[0] = (osd25 - osd0) / 25.0;
    step[1] = (osd50 - osd25) / 25.0;
    step[2] = (osd75 - osd50) / 25.0;
    step[3] = (osd100 - osd75) / 25.0;

    for (int i = 0; i <= 100; i++) {
        if (i == 0) {
            osdvalue[i] = osd0;
        } else if ((i > 0) && (i <= 25)) {
            osdvalue[i] = osd0 + (int) (i * step[0]);
        } else if ((i > 25) && (i <= 50)) {
            osdvalue[i] = osd25 + (int) ((i - 25) * step[1]);
        } else if ((i > 50) && (i <= 75)) {
            osdvalue[i] = osd50 + (int) ((i - 50) * step[2]);
        } else if ((i > 75) && (i <= 100)) {
            osdvalue[i] = osd75 + (int) ((i - 75) * step[3]);
        }
        memset(sql, '\0', 256);
        getSqlParams(__FUNCTION__, sql,
                     "insert into %s(TVIN_PORT, Item_ID, Level, Value) values(%d,%d,%d,%d);", name,
                     source_type, data_type, i, osdvalue[i]);
        if (!(this->exeSql(sql))) {
            return -1;
        }
    }
    return 0;
}

int CPqData::getSharpnessFlag()
{
    return sha_diff_flag;
}

int CPqData::loadSharpnessData(char *sqlmaster, char *table_name)
{
    LOGD("%s, table_name: %s, sqlmaster: %s\n", __FUNCTION__, table_name, sqlmaster);
    CSqlite::Cursor c;
    int rval;
    int *pq_nodes = NULL;
    int temp_flag = 0;

    if (!strncmp("Sharpness_0", table_name, 11)) {//for gxtvbb:Sharpness_0_***
        sha_diff_flag = sha_diff_flag | 2;
        pq_nodes = &sha_nodes;
        temp_flag = 2;
    } else if (!strncmp("Sharpness_1", table_name, 11)) {//for gxtvbb:Sharpness_1_***
        sha_diff_flag = sha_diff_flag | 4;
        pq_nodes = &sha_nodes_1;
        temp_flag = 4;
    } else {//for g9:Sharpness_G9_***
        sha_diff_flag = 1;
        pq_nodes = &sha_nodes;
        temp_flag = 1;
    }
    LOGD("%s, sha_diff_flag: %d, temp_flag: %d\n", __FUNCTION__, sha_diff_flag, temp_flag);

    rval = this->select(sqlmaster, c);
    int length = 0;
    int index = 0;
    if ((temp_flag == 1) || (temp_flag == 2)) {
        if (c.moveToFirst()) {
            *pq_nodes = c.getInt(0);//TotalNode?
            length = c.getCount() / (*pq_nodes);
            for (int i = 0; i < *pq_nodes; i++) {
                pq_sharpness_reg_data[i].length = length;
            }
            do {
                pq_sharpness_reg_data[index / length].reg_data[index % length].TotalNode
                    = c.getInt(0);
                pq_sharpness_reg_data[index / length].reg_data[index % length].NodeValue
                    = c.getInt(1);
                pq_sharpness_reg_data[index / length].reg_data[index % length].Value.type
                    = c.getUInt(2);
                pq_sharpness_reg_data[index / length].reg_data[index % length].Value.addr
                    = c.getUInt(3);
                pq_sharpness_reg_data[index / length].reg_data[index % length].Value.mask
                    = c.getUInt(4);
                pq_sharpness_reg_data[index / length].reg_data[index % length].IndexValue
                    = c.getInt(5);
                pq_sharpness_reg_data[index / length].reg_data[index % length].Value.val
                    = c.getUInt(6);
                pq_sharpness_reg_data[index / length].reg_data[index % length].step = c.getF(7);
                index++;
            } while (c.moveToNext());
        }
    } else if (temp_flag == 4) {
        if (c.moveToFirst()) {
            *pq_nodes = c.getInt(0);//TotalNode?
            length = c.getCount() / (*pq_nodes);
            for (int i = 0; i < *pq_nodes; i++) {
                pq_sharpness_reg_data_1[i].length = length;
            }
            do {
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].TotalNode
                    = c.getInt(0);
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].NodeValue
                    = c.getInt(1);
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].Value.type
                    = c.getUInt(2);
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].Value.addr
                    = c.getUInt(3);
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].Value.mask
                    = c.getUInt(4);
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].IndexValue
                    = c.getInt(5);
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].Value.val
                    = c.getUInt(6);
                pq_sharpness_reg_data_1[index / length].reg_data[index % length].step = c.getF(7);
                index++;
            } while (c.moveToNext());
        }
    }
    return rval;
}

int CPqData::LoadPQData(tvpq_data_type_t data_type, tvin_port_t source_port,
                        tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d, tvin_trans_fmt_t trans_fmt, int flag)
{
    CSqlite::Cursor c;
    int rval;
    char sqlmaster[256];
    char table_name[128];
    int mode = is2dOr3d;
    tvpq_data_t *pq_data = NULL;
    int *pq_nodes = NULL;

    trans_fmt = trans_fmt;
    switch (data_type) {
    case TVPQ_DATA_BRIGHTNESS:
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select TableName from GeneralContrastBrightnessTable where "
                     "TVIN_PORT = %d and "
                     "TVIN_SIG_FMT = %d and "
                     "TVIN_TRANS_FMT = %d and "
                     "%s = %d;", source_port, sig_fmt, mode, ID_FIELD, BRIGHTNESS_ID);
        pq_data = pq_bri_data;
        pq_nodes = &bri_nodes;
        break;
    case TVPQ_DATA_CONTRAST:
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select TableName from GeneralContrastBrightnessTable where "
                     "TVIN_PORT = %d and "
                     "TVIN_SIG_FMT = %d and "
                     "TVIN_TRANS_FMT = %d and "
                     "%s = %d;", source_port, sig_fmt, mode, ID_FIELD, CONTRAST_ID);
        pq_data = pq_con_data;
        pq_nodes = &con_nodes;
        break;
    case TVPQ_DATA_HUE:
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select TableName from GeneralSaturationHueTable where "
                     "TVIN_PORT = %d and "
                     "TVIN_SIG_FMT = %d and "
                     "TVIN_TRANS_FMT = %d and "
                     "%s = %d;", source_port, sig_fmt, mode, ID_FIELD, HUE_ID);
        pq_data = pq_hue_data;
        pq_nodes = &hue_nodes;
        break;
    case TVPQ_DATA_SATURATION:
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select TableName from GeneralSaturationHueTable where "
                     "TVIN_PORT = %d and "
                     "TVIN_SIG_FMT = %d and "
                     "TVIN_TRANS_FMT = %d and "
                     "%s = %d;", source_port, sig_fmt, mode, ID_FIELD, SATURATION_ID);
        pq_data = pq_sat_data;
        pq_nodes = &sat_nodes;
        break;
    case TVPQ_DATA_SHARPNESS:
        //sprintf(sqlmaster, "select TableName from GeneralSharpnessTable where "
        getSqlParams(__FUNCTION__, sqlmaster,
                     "select TableName from GeneralSharpnessG9Table where "
                     "TVIN_PORT = %d and "
                     "TVIN_SIG_FMT = %d and "
                     "TVIN_TRANS_FMT = %d and "
                     "%s = %d;", source_port, sig_fmt, mode, ID_FIELD, SHARPNESS_ID);
        pq_data = NULL;
        pq_nodes = &sha_nodes;
        break;
    default:
        break;
    }

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        switch (data_type) {
        case TVPQ_DATA_BRIGHTNESS:
        case TVPQ_DATA_CONTRAST:
        case TVPQ_DATA_HUE:
        case TVPQ_DATA_SATURATION:
            getSqlParams(
                __FUNCTION__,
                sqlmaster,
                "select TotalNode, NodeNumber, IndexValue, RegValue, StepUp from %s order by NodeNumber asc;",
                c.getString(0).string());
            break;
        case TVPQ_DATA_SHARPNESS:
            do {
                getSqlParams(__FUNCTION__, sqlmaster,
                             "select TotalNode, NodeNumber, RegType, RegAddr, RegMask,"
                             "IndexValue, RegValue, StepUp from %s order by NodeNumber asc;",
                             c.getString(0).string());
                c.getString(table_name, 0);
                rval = loadSharpnessData(sqlmaster, table_name);
            } while (c.moveToNext());
            return rval;
        default:
            break;
        }
    }

    rval = this->select(sqlmaster, c);

    if (c.moveToFirst()) {
        if (data_type != TVPQ_DATA_SHARPNESS) {
            int index = 0;
            do {
                pq_data[index].TotalNode = c.getInt(0);
                pq_data[index].NodeValue = c.getInt(1);
                pq_data[index].IndexValue = c.getInt(2);
                pq_data[index].RegValue = c.getInt(3);
                pq_data[index].step = c.getF(4);
                if (flag == 1) {
                    if (data_type == TVPQ_DATA_BRIGHTNESS) {
                        LOGD("%s, bri pq_data[%d].RegValue = %d\n", "TV", index,
                             pq_data[index].RegValue);
                        pq_data[index].RegValue -= 64;
                        LOGD("%s, bri pq_data[%d].RegValue + 64 = %d\n", "TV", index,
                             pq_data[index].RegValue);
                    } else if (data_type == TVPQ_DATA_CONTRAST) {
                        LOGD("%s, con pq_data[%d].RegValue = %d\n", "TV", index,
                             pq_data[index].RegValue);
                        pq_data[index].RegValue += 64;
                        LOGD("%s, con pq_data[%d].RegValue + 64 = %d\n", "TV", index,
                             pq_data[index].RegValue);
                    }
                }
                index++;
            } while (c.moveToNext());
            *pq_nodes = index;
        }
    }

    return rval;
}

int CPqData::LoadAllPQData(tvin_port_t source_port, tvin_sig_fmt_t sig_fmt, is_3d_type_t is2dOr3d,
                           tvin_trans_fmt_t trans_fmt, int flag)
{
    int rval = -1;
    rval = LoadPQData(TVPQ_DATA_BRIGHTNESS, source_port, sig_fmt, is2dOr3d, trans_fmt, flag);
    if (rval) {
        return rval;
    }
    rval = LoadPQData(TVPQ_DATA_CONTRAST, source_port, sig_fmt, is2dOr3d, trans_fmt, flag);
    if (rval) {
        return rval;
    }
    rval = LoadPQData(TVPQ_DATA_SATURATION, source_port, sig_fmt, is2dOr3d, trans_fmt, -1);
    if (rval) {
        return rval;
    }
    rval = LoadPQData(TVPQ_DATA_HUE, source_port, sig_fmt, is2dOr3d, trans_fmt, -1);
    if (rval) {
        return rval;
    }
    rval = LoadPQData(TVPQ_DATA_SHARPNESS, source_port, sig_fmt, is2dOr3d, trans_fmt, -1);
    if (rval) {
        return rval;
    }
    return rval;
}

typedef enum initial_type_e {
    TYPE_PMode = 0,
    TYPE_PMode_Default,
    TYPE_Nonlinear,
    TYPE_NonLinear_Default,
    TYPE_VGA_AUTO,
    TYPE_OVERSCAN,
} initial_type_t;

const char *Pmode_name[6] = { "Picture_Mode", "Picture_Mode_Default", "NonlinearMapping",
                              "NonlinearMapping_Default", "VGA_AutoParams", "OVERSCAN"
                            };

void CPqData::initialTable(int type)
{
    vpp_pq_para_t pmode_default;

    pmode_default.backlight = 100;
    pmode_default.brightness = 50;
    pmode_default.contrast = 50;
    pmode_default.hue = 50;
    pmode_default.nr = 0;
    pmode_default.saturation = 50;
    pmode_default.sharpness = 50;

    switch (type) {
    case TYPE_PMode:
    case TYPE_PMode_Default:
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 4; j++) {
                PQ_SetPQModeParamsByName(Pmode_name[type], (tv_source_input_type_t) i,
                                         (vpp_picture_mode_t) j, &pmode_default);
            }
        }
        break;
    case TYPE_Nonlinear:
    case TYPE_NonLinear_Default:
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 5; j++) {
                SetNonlinearMappingByName(Pmode_name[type], (tvpq_data_type_t) j,
                                          (tv_source_input_type_t) i, 0, (int) 255 / 4.0, (int) 255 * 2 / 4.0,
                                          (int) 255 * 3 / 4.0, 255);
            }
        }
        break;
    case TYPE_VGA_AUTO:
        break;
    case TYPE_OVERSCAN:
        break;
    }
}

/*
 void CPqData::transferFixTable()
 {
 char sql[256];
 char *err = NULL;
 char **rsltReg = NULL;
 int rval, rows, columns;
 unsigned int tvin, sig, trans;
 char tablename[256];

 sprintf(sql, "Select TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, TableName from GeneralSharpnessTable where TableID = 0;");

 rval = sqlite3_get_table(DB, sql, &rsltReg, &rows, &columns, &err);

 if (rval) {
 if (rsltReg != NULL) {
 sqlite3_free_table(rsltReg);
 rsltReg = NULL;
 }
 if (err != NULL) {
 sqlite3_free(err);
 err = NULL;
 }
 return;
 } else {
 for (int i = 1; i <= rows; i++) {
 memset(sql, '\0', 256);

 sscanf(rsltReg[i * columns], "%u", &tvin);
 sscanf(rsltReg[i * columns + 1], "%u", &sig);
 sscanf(rsltReg[i * columns + 2], "%u", &trans);
 sscanf(rsltReg[i * columns + 3], "%s", tablename);

 sprintf(sql, "insert into GeneralCommonTable(TVIN_PORT, TVIN_SIG_FMT, TVIN_TRANS_FMT, TableID, TableName) values(%d,%d,%d,0,\"%s\");",
 tvin, sig, trans, tablename);
 rval = sqlite3_exec(DB, sql, NULL, NULL, &err);
 if (rval) {
 if (err != NULL) {
 sqlite3_free(err);
 err = NULL;
 }
 return;
 }
 if (err != NULL) {
 sqlite3_free(err);
 err = NULL;
 }
 }
 memset(sql, '\0', 256);
 sprintf(sql,  "delete from GeneralSharpnessTable where TableID = 0;");
 rval = sqlite3_exec(DB, sql, NULL, NULL, &err);
 if (rval) {
 if (err != NULL) {
 sqlite3_free(err);
 err = NULL;
 }
 return;
 }
 if (err != NULL) {
 sqlite3_free(err);
 err = NULL;
 }
 }
 }
 */

int CPqData::replacePqDb(const char *newFilePath)
{

    CFile::delFile( PQ_DB_PATH);
    if (newFilePath == NULL) {
        return 0;
    } else {
        LOGD("path = %s", newFilePath);
        CFile file(newFilePath);
        int ret = file.copyTo(PQ_DB_PATH);
        system("sync");
        return ret;
    }
}

int CPqData::PQ_GetPhaseArray(am_phase_t *am_phase)
{
    CSqlite::Cursor c;
    int iOutRet = 0;
    char sqlmaster[256];
    getSqlParams(__FUNCTION__, sqlmaster, "select Phase from Phase order by Format ASC; ");

    this->select(sqlmaster, c);
    int nums = 0;
    am_phase->length = c.getCount();
    if (c.moveToFirst()) {
        do {
            am_phase->phase[nums] = c.getInt(0);//col index is 0[Phase]
            nums++;
        } while (c.moveToNext());
    }
    //
    return nums;
}


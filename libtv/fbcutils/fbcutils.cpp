/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */

#define LOG_TAG "tvserver"
#define LOG_TV_TAG "fbcutils"

#include "fbcutils.h"
#include <tvconfig.h>
#include <tvutils.h>

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CTvLog.h>


using namespace android;

static int gFBCPrjInfoRDPass = 0;
static char gFBCPrjInfoBuf[1024] = {0};


int rebootSystemByEdidInfo()
{
    int ret = -1;
    int fd = -1;
    int edid_info_len = 256;
    unsigned char fbc_edid_info[edid_info_len];
    int env_different_as_cur = 0;
    char outputmode_prop_value[256];
    char lcd_reverse_prop_value[256];

    LOGD("get edid info from fbc!");
    memset(outputmode_prop_value, '\0', 256);
    memset(lcd_reverse_prop_value, '\0', 256);
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode_prop_value, "null" );
    getBootEnv(UBOOTENV_LCD_REVERSE, lcd_reverse_prop_value, "null" );

    fd = open("/sys/class/amhdmitx/amhdmitx0/edid_info", O_RDWR);
    if (fd < 0) {
        LOGE("open edid node error");
        return -1;
    }
    ret = read(fd, fbc_edid_info, edid_info_len);
    if (ret < 0) {
        LOGE("read edid node error");
        return -1;
    }

    if ((0xfb == (unsigned char)fbc_edid_info[250]) && (0x0c == (unsigned char)fbc_edid_info[251])) {
        LOGD("RX is FBC!");
        // set outputmode env
        ret = 0;//is Fbc
        switch (fbc_edid_info[252] & 0x0f) {
        case 0x0:
            if (0 != strcmp(outputmode_prop_value, "1080p")
                && 0 != strcmp(outputmode_prop_value, "1080p50hz")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_OUTPUTMODE, "1080p");
            }
            break;
        case 0x1:
            if (0 != strcmp(outputmode_prop_value, "4k2k60hz420") &&
                    0 != strcmp(outputmode_prop_value, "4k2k50hz420")
               ) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_OUTPUTMODE, "4k2k60hz420");
            }
            break;
        case 0x2:
            if (0 != strcmp(outputmode_prop_value, "1366*768")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_OUTPUTMODE, "1366*768");
            }
            break;
        default:
            break;
        }

        // set RX 3D Info
        //switch((fbc_edid_info[252]>>4)&0x0f)

        // set lcd_reverse env
        switch (fbc_edid_info[253]) {
        case 0x0:
            if (0 != strcmp(lcd_reverse_prop_value, "0")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_LCD_REVERSE, "0");
            }
            break;
        case 0x1:
            if (0 != strcmp(lcd_reverse_prop_value, "1")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                setBootEnv(UBOOTENV_LCD_REVERSE, "1");
            }
            break;
        default:
            break;
        }
    }
    close(fd);
    fd = -1;
    //ret = -1;
    if (1 == env_different_as_cur) {
        LOGD("env change , reboot system");
        system("reboot");
    }
    return ret;
}

int rebootSystemByUartPanelInfo(CFbcCommunication *fbc)
{
    int ret = -1;
    char outputmode_prop_value[256] = {0};
    char lcd_reverse_prop_value[256] = {0};
    int env_different_as_cur = 0;
    int panel_reverse = -1;
    int panel_outputmode = -1;

    char panel_model[64] = {0};

    if (fbc == NULL) {
        LOGE("there is no fbc!!!");
        return -1;
    }

    fbc->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, panel_model);
    if (0 == panel_model[0]) {
        LOGD("device is not fbc");
        return -1;
    }
    LOGD("device is fbc, get panel info from fbc!");
    getBootEnv(UBOOTENV_OUTPUTMODE, outputmode_prop_value, "null" );
    getBootEnv(UBOOTENV_LCD_REVERSE, lcd_reverse_prop_value, "null" );

    fbc->cfbc_Get_FBC_PANEL_REVERSE(COMM_DEV_SERIAL, &panel_reverse);
    fbc->cfbc_Get_FBC_PANEL_OUTPUT(COMM_DEV_SERIAL, &panel_outputmode);
    LOGD("panel_reverse = %d, panel_outputmode = %d", panel_reverse, panel_outputmode);
    LOGD("panel_output prop = %s, panel reverse prop = %s", outputmode_prop_value, lcd_reverse_prop_value);
    switch (panel_outputmode) {
    case T_1080P50HZ:
        if ( NULL ==strstr (outputmode_prop_value, "1080p") ) {
            LOGD("panel_output changed to 1080p50hz");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "1080p50hz");
            setBootEnv(UBOOTENV_HDMIMODE, "1080p50hz");
        }
        break;
    case T_2160P50HZ420:
        if ( (NULL == strstr(outputmode_prop_value, "2160p")) || (NULL == strstr(outputmode_prop_value, "420")) ) {
            LOGD("panel_output changed to 2160p50hz420");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz420");
            setBootEnv(UBOOTENV_HDMIMODE, "2160p50hz420");
        }
        break;
    case T_1080P50HZ44410BIT:
        if ( (NULL == strstr(outputmode_prop_value, "1080p")) || (NULL == strstr(outputmode_prop_value, "44410bit")) ) {
            LOGD("panel_output changed to 1080p50hz44410bit");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "1080p50hz44410bit");
            setBootEnv(UBOOTENV_HDMIMODE, "1080p50hz44410bit");
        }
        break;
    case T_2160P50HZ42010BIT:
        if ( (NULL == strstr(outputmode_prop_value, "2160p")) || (NULL == strstr(outputmode_prop_value, "42010bit")) ) {
            LOGD("panel_output changed to 2160p50hz42010bit");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz42010bit");
            setBootEnv(UBOOTENV_HDMIMODE, "2160p50hz42010bit");
        }
        break;
    case T_2160P50HZ42210BIT:
        if ( (NULL == strstr(outputmode_prop_value, "2160p")) || (NULL == strstr(outputmode_prop_value, "42210bit")) ) {
            LOGD("panel_output changed to 2160p50hz42210bit");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz42210bit");
            setBootEnv(UBOOTENV_HDMIMODE, "2160p50hz42210bit");
        }
        break;
    case T_2160P50HZ444:
        if ( (NULL == strstr(outputmode_prop_value, "2160p")) || (NULL == strstr(outputmode_prop_value, "444")) ) {
            LOGD("panel_output changed to 2160p50hz444");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_OUTPUTMODE, "2160p50hz444");
            setBootEnv(UBOOTENV_HDMIMODE, "2160p50hz444");
        }
        break;
    default:
        break;
    }

    // set RX 3D Info
    //switch((fbc_edid_info[252]>>4)&0x0f)

    // set lcd_reverse env
    switch (panel_reverse) {
    case 0x0:
        if (0 != strcmp(lcd_reverse_prop_value, "0")) {
            LOGD("panel_reverse changed to 0");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_LCD_REVERSE, "0");
        }
        break;
    case 0x1:
        if (0 != strcmp(lcd_reverse_prop_value, "1")) {
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            setBootEnv(UBOOTENV_LCD_REVERSE, "1");
        }
        break;
    default:
        break;
    }

    ret = -1;
    if (1 == env_different_as_cur) {
        LOGD("env change , reboot system");
        system("reboot");
    }
    return 0;
}

int GetPlatformProjectInfoSrc()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, CFG_PROJECT_INFO, "null");
    if (strcmp(config_value, "null") == 0 || strcmp(config_value, "prop") == 0) {
        return 0;
    } else if (strcmp(config_value, "emmckey") == 0) {
        return 1;
    } else if (strcmp(config_value, "fbc_ver") == 0) {
        return 2;
    }

    return 0;
}

unsigned int CalCRC32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len)
{
    static const unsigned int s_crc32[16] = {
        0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
        0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8,
        0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};
    unsigned int crcu32 = crc;
    //if (buf_len < 0)
    //    return 0;
    if (!ptr) return 0;
    crcu32 = ~crcu32;
    while (buf_len--) {
        unsigned char b = *ptr++;
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }
    return ~crcu32;
}

static int isProjectInfoValid(char *data_str, int chksum_head_len, int ver_len)
{
    int tmp_len = 0, tmp_ver = 0;
    char *endp = NULL;
    unsigned long src_chksum = 0, cal_chksum = 0;
    char tmp_buf[129] = { 0 };

    if (data_str != NULL) {
        tmp_len = strlen(data_str);
        if (tmp_len > chksum_head_len + ver_len) {
            cal_chksum = CalCRC32(0, (unsigned char *)(data_str + chksum_head_len), tmp_len - chksum_head_len);
            memcpy(tmp_buf, data_str, chksum_head_len);
            tmp_buf[chksum_head_len] = 0;
            src_chksum = strtoul(tmp_buf, &endp, 16);
            if (cal_chksum == src_chksum) {
                memcpy(tmp_buf, data_str + chksum_head_len, ver_len);
                if ((tmp_buf[0] == 'v' || tmp_buf[0] == 'V') && isxdigit(tmp_buf[1]) && isxdigit(tmp_buf[2]) && isxdigit(tmp_buf[3])) {
                    tmp_ver = strtoul(tmp_buf + 1, &endp, 16);
                    if (tmp_ver <= 0) {
                        LOGD("%s, project_info data version error!!!", __FUNCTION__);
                        return -1;
                    }
                } else {
                    LOGD("%s, project_info data version error!!!", __FUNCTION__);
                    return -1;
                }

                return tmp_ver;
            } else {
                LOGD("%s, cal_chksum = %x", __FUNCTION__, (unsigned int)cal_chksum);
                LOGD("%s, src_chksum = %x", __FUNCTION__, (unsigned int)src_chksum);
            }
        }

        LOGD("%s, project_info data error!!!", __FUNCTION__);
        return -1;
    }

    LOGD("%s, project_info data is NULL!!!", __FUNCTION__);
    return -1;
}

static int GetProjectInfoOriData(char data_str[], CFbcCommunication *fbcIns)
{
    int src_type = GetPlatformProjectInfoSrc();

    if (src_type == 0) {
        //memset(data_str, '\0', sizeof(data_str));//sizeof pointer has issue
        getBootEnv(UBOOTENV_PROJECT_INFO, data_str, (char *)"null");
        if (strcmp(data_str, "null") == 0) {
            LOGE("%s, get project info data error!!!", __FUNCTION__);
            return -1;
        }

        return 0;
    } else if (src_type == 1) {
        return -1;
    } else if (src_type == 2) {
        int i = 0, tmp_len = 0, tmp_val = 0, item_cnt = 0;
        int tmp_rd_fail_flag = 0;
        unsigned int cal_chksum = 0;
        char sw_version[64];
        char build_time[64];
        char git_version[64];
        char git_branch[64];
        char build_name[64];
        char tmp_buf[512] = {0};

        if (fbcIns != NULL) {
            if (gFBCPrjInfoRDPass == 0) {
                memset((void *)gFBCPrjInfoBuf, 0, sizeof(gFBCPrjInfoBuf));
            }

            if (gFBCPrjInfoRDPass == 1) {
                strcpy(data_str, gFBCPrjInfoBuf);
                LOGD("%s, rd once just return, data_str = %s", __FUNCTION__, data_str);
                return 0;
            }

            if (fbcIns->cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_SERIAL, sw_version, build_time, git_version, git_branch, build_name) == 0) {
                if (sw_version[0] == '1' || sw_version[0] == '2') {
                    strcpy(build_name, "2");

                    strcpy(tmp_buf, "v001,fbc_");
                    strcat(tmp_buf, build_name);
                    strcat(tmp_buf, ",4k2k60hz420,no_rev,");
                    strcat(tmp_buf, "HV550QU2-305");
                    strcat(tmp_buf, ",8o8w,0,0");
                    cal_chksum = CalCRC32(0, (unsigned char *)tmp_buf, strlen(tmp_buf));
                    sprintf(data_str, "%08x,%s", cal_chksum, tmp_buf);
                    LOGD("%s, data_str = %s", __FUNCTION__, data_str);
                } else {
                    tmp_val = 0;
                    if (fbcIns->cfbc_Get_FBC_project_id(COMM_DEV_SERIAL, &tmp_val) == 0) {
                        sprintf(build_name, "fbc_%d", tmp_val);
                    } else {
                        tmp_rd_fail_flag = 1;
                        strcpy(build_name, "fbc_0");
                        LOGD("%s, get project id from fbc error!!!", __FUNCTION__);
                    }

                    strcpy(tmp_buf, "v001,");
                    strcat(tmp_buf, build_name);
                    strcat(tmp_buf, ",4k2k60hz420,no_rev,");

                    memset(git_branch, 0, sizeof(git_branch));
                    if (fbcIns->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, git_branch) == 0) {
                        strcat(tmp_buf, git_branch);
                    } else {
                        tmp_rd_fail_flag = 1;
                        strcat(tmp_buf, build_name);
                        LOGD("%s, get panel info from fbc error!!!", __FUNCTION__);
                    }

                    strcat(tmp_buf, ",8o8w,0,0");
                    cal_chksum = CalCRC32(0, (unsigned char *)tmp_buf, strlen(tmp_buf));
                    sprintf(data_str, "%08x,%s", cal_chksum, tmp_buf);
                    LOGD("%s, data_str = %s", __FUNCTION__, data_str);

                    if (tmp_rd_fail_flag == 0) {
                        gFBCPrjInfoRDPass = 1;
                        strcpy(gFBCPrjInfoBuf, data_str);
                    }
                }

                return 0;
            }

            return -1;
        }
    }

    return -1;
}

static int handleProjectInfoByVersion(int ver, int item_ind, char *item_str, project_info_t *proj_info_ptr)
{
    if (ver == 1) {
        if (item_ind == 0) {
            strncpy(proj_info_ptr->version, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 1) {
            strncpy(proj_info_ptr->panel_type, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 2) {
            strncpy(proj_info_ptr->panel_outputmode, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 3) {
            strncpy(proj_info_ptr->panel_rev, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 4) {
            strncpy(proj_info_ptr->panel_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 5) {
            strncpy(proj_info_ptr->amp_curve_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }
    } else {
        if (item_ind == 0) {
            strncpy(proj_info_ptr->version, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 1) {
            strncpy(proj_info_ptr->panel_type, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 2) {
            strncpy(proj_info_ptr->panel_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }  else if (item_ind == 3) {
            strncpy(proj_info_ptr->panel_outputmode, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 4) {
            strncpy(proj_info_ptr->panel_rev, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 5) {
            strncpy(proj_info_ptr->amp_curve_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }
    }

    return 0;
}

int GetProjectInfo(project_info_t *proj_info_ptr, CFbcCommunication *fbcIns)
{
    int i = 0, tmp_ret = 0, tmp_val = 0, tmp_len = 0;
    int item_cnt = 0, handle_prj_info_data_flag = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    char tmp_buf[1024] = { 0 };
    char data_str[1024] = { 0 };

    if (GetProjectInfoOriData(data_str, fbcIns) < 0) {
        return -1;
    }

    memset((void *)proj_info_ptr->version, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_type, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_outputmode, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_rev, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_name, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->amp_curve_name, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);

    //check project info data is valid
    handle_prj_info_data_flag = isProjectInfoValid(data_str, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN);

    //handle project info data
    if (handle_prj_info_data_flag > 0) {
        item_cnt = 0;
        memset((void *)tmp_buf, 0, sizeof(tmp_buf));
        strncpy(tmp_buf, data_str + CC_HEAD_CHKSUM_LEN, sizeof(tmp_buf) - 1);
        token = strtok(tmp_buf, strDelimit);
        while (token != NULL) {
            handleProjectInfoByVersion(handle_prj_info_data_flag, item_cnt, token, proj_info_ptr);

            token = strtok(NULL, strDelimit);
            item_cnt += 1;
        }

        return 0;
    }

    return -1;
}

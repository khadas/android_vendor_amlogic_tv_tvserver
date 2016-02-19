#define LOG_TAG "CTvin"

#include "CTvin.h"
#include <CTvLog.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <cutils/log.h>
#include <sys/mman.h>
#include "../vpp/CPQdb.h"
#include "CAv.h"
#include "../tvsetting/CTvSetting.h"
#include "../tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"
#include "CFbcCommunication.h"

#define AFE_DEV_PATH        "/dev/tvafe0"
#define HDMIRX_KSV_PATH     "/dev/hdmirx0"


#define CC_SEL_VDIN_DEV   (0)

#define MODE_3D_DISABLE         0x00000000
#define MODE_3D_ENABLE          0x00000001
#define MODE_AUTO               0x00000002
#define MODE_2D_TO_3D           0x00000004
#define MODE_LR                 0x00000008
#define MODE_BT                 0x00000010
#define MODE_LR_SWITCH          0x00000020
#define MODE_FIELD_DEPTH        0x00000040
//#define MODE_3D_TO_2D_L         0x00000080 redefine
//#define MODE_3D_TO_2D_R         0x00000100 redefine
#define LR_FORMAT_INDICATOR     0x00000200
#define BT_FORMAT_INDICATOR     0x00000400

/* ADC calibration pattern & format define */
/* default 100% 8 color-bar */
//#define VGA_SOURCE_RGB444
#define VGA_H_ACTIVE    (1024)
#define VGA_V_ACTIVE    (768)
#define COMP_H_ACTIVE   (1280)
#define COMP_V_ACTIVE   (720)
#define CVBS_H_ACTIVE   (720)
#define CVBS_V_ACTIVE   (480)

int CTvin::mSourceInputToPortMap[SOURCE_MAX];
CTvin *CTvin::mInstance;

CTvin *CTvin::getInstance()
{
    if (NULL == mInstance) mInstance = new CTvin();
    return mInstance;
}

CTvin::CTvin()
{
    int i = 0;

    m_vdin_dev_fd = -1;
    afe_dev_fd = -1;

    m_tvin_param.index = 0;
    m_is_decoder_start = false;
    gExistD2D3 = -1;
    gVideoPath[0] = '\0';
    m_pathid = TV_PATH_DECODER_3D_AMVIDEO;

    for (i = 0; i < SOURCE_MAX; i++) {
        mSourceInputToPortMap[i] = TVIN_PORT_NULL;
    }

    mSourceInputToPortMap[SOURCE_TV] = TVIN_PORT_CVBS3;
    mSourceInputToPortMap[SOURCE_AV1] = TVIN_PORT_CVBS1;
    mSourceInputToPortMap[SOURCE_AV2] = TVIN_PORT_CVBS2;
    mSourceInputToPortMap[SOURCE_YPBPR1] = TVIN_PORT_COMP0;
    mSourceInputToPortMap[SOURCE_YPBPR2] = TVIN_PORT_COMP1;
    mSourceInputToPortMap[SOURCE_HDMI1] = TVIN_PORT_HDMI0;
    mSourceInputToPortMap[SOURCE_HDMI2] = TVIN_PORT_HDMI2;
    mSourceInputToPortMap[SOURCE_HDMI3] = TVIN_PORT_HDMI1;
    mSourceInputToPortMap[SOURCE_VGA] = TVIN_PORT_VGA0;
    mSourceInputToPortMap[SOURCE_MPEG] = TVIN_PORT_MPEG0;
    mSourceInputToPortMap[SOURCE_DTV] = TVIN_PORT_DTV;
    mSourceInputToPortMap[SOURCE_IPTV] = TVIN_PORT_BT656;
}

CTvin::~CTvin()
{
}

int CTvin::OpenTvin()
{
    const char *config_value;
    config_value = config_get_str ( CFG_SECTION_TV, "tvin.manual.set.tvpath", "null" );
    strcpy ( config_tv_path, config_value );
    memset ( config_default_path, 0x0, 64 );
    config_value = config_get_str ( CFG_SECTION_TV, "tvin.manual.set.defaultpath", "null" );
    strcpy ( config_default_path, config_value );
    return 0;
}

int CTvin::IsFileExist ( const char *file_name )
{
    struct stat tmp_st;

    if ( stat ( file_name, &tmp_st ) < 0 ) {
        return 0;
    }

    return 1;
}

char *CTvin::DelSub ( char *str, char *sub )
{
    char *psrc = str, *pdest = str, *psub = NULL, *p = NULL;

    if ( ( str == NULL ) || ( sub == NULL ) ) {
        return NULL;
    }

    while ( *psrc ) {
        p = psrc;
        psub = sub;

        while ( *p && *p == *psub ) {
            p++;
            psub++;
        }

        if ( *psub == 0 ) {
            psrc = p;
        } else {
            *pdest++ = *psrc++;
        }
    }

    *pdest = 0;

    return str;
}

int CTvin::setMpeg2Vdin(int enable)
{
    /* let output data loop to vdin */
    FILE *fp = fopen ( "/sys/module/di/parameters/mpeg2vdin_en", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/mpeg2vdin_en error\n" );
        return -1;
    }
    fprintf ( fp, "%d", enable );
    fclose ( fp );
    return 0;
}

char *CTvin::VDIN_CheckVideoPath ( const char *videopath )
{
    strncpy ( gVideoPath, videopath, sizeof ( gVideoPath ) );

    if ( strstr ( videopath, "d2d3 " ) != NULL ) {
        if ( gExistD2D3 == -1 ) {
            gExistD2D3 = IsFileExist ( "/sys/class/d2d3/d2d3" );
        }

        if ( gExistD2D3 == 0 ) {
            DelSub ( gVideoPath, (char *)"d2d3 " );
        }
    }

    LOGW ( "%s, video path before check [%s]\n", CFG_SECTION_TV, videopath );
    LOGW ( "%s, video path after check [%s]\n", CFG_SECTION_TV, gVideoPath );

    return gVideoPath;
}

int CTvin::VDIN_AddPath ( const char *videopath )
{
    FILE *fp = NULL;
    int ret = -1;
    char *tmp_video_path = NULL;

    tmp_video_path = VDIN_CheckVideoPath ( videopath );

    fp = fopen ( "/sys/class/vfm/map", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/vfm/map error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = fprintf ( fp, "%s", tmp_video_path );

    if ( ret < 0 ) {
        LOGW ( "Add VideoPath error(%s)!\n", strerror ( errno ) );
    }

    fclose ( fp );
    fp = NULL;

    return ret;
}

int CTvin::VDIN_RmDefPath ( void )
{
    int fd = -1, ret;
    char str[] = "rm default";


    fd = open ( "/sys/class/vfm/map", O_RDWR );

    if ( fd < 0 ) {
        LOGW ( "Open /sys/class/vfm/map error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = write ( fd, str, sizeof ( str ) );

    if ( ret < 0 ) {
        LOGW ( "Rm default path error(%s)!\n", strerror ( errno ) );
    }

    close ( fd );
    fd = -1;

    return ret;
}

int CTvin::VDIN_RmTvPath ( void )
{
    int fd, ret;
    char str[] = "rm tvpath";


    fd = open ( "/sys/class/vfm/map", O_RDWR );

    if ( fd < 0 ) {
        LOGW ( "Open /sys/class/vfm/map error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = write ( fd, str, sizeof ( str ) );

    if ( ret <= 0 ) {
        LOGW ( "Rm tv path error(%s)!\n", strerror ( errno ) );
    }

    close ( fd );
    fd = -1;
    return ret;
}

int CTvin::VDIN_AddVideoPath ( int selPath )
{
    int ret = -1;

    switch ( selPath ) {
    case TV_PATH_VDIN_AMVIDEO:
        ret = VDIN_AddPath ( "add tvpath vdin0 amvideo" );
        break;

    case TV_PATH_VDIN_DEINTERLACE_AMVIDEO:
        ret = VDIN_AddPath ( "add tvpath vdin0 deinterlace amvideo" );
        break;

    case TV_PATH_VDIN_3D_AMVIDEO:
        ret = VDIN_AddPath ( "add tvpath vdin0 ppmgr amvideo" );
        break;

    case TV_PATH_VDIN_NEW3D_AMVIDEO:
        ret = VDIN_AddPath ( "add tvpath vdin0 deinterlace ppmgr d2d3 amvideo" );
        break;

    case TV_PATH_VDIN_NEW3D_WITHOUTPPMGR_AMVIDEO:
        ret = VDIN_AddPath ( "add tvpath vdin0 deinterlace d2d3 amvideo" );
        break;

    case TV_PATH_DECODER_3D_AMVIDEO:
        ret = VDIN_AddPath ( "add default decoder ppmgr deinterlace amvideo" );
        break;

    case TV_PATH_DECODER_AMVIDEO:
        ret = VDIN_AddPath ( "add default decoder deinterlace amvideo" );
        break;

    case TV_PATH_VDIN_FREESCALE_AMVIDEO:
        ret = VDIN_AddPath ( "add previewpath vdin0 freescale amvideo" );
        break;

    case TV_PATH_DECODER_NEW3D_AMVIDEO:
        ret = VDIN_AddPath ( "add default decoder deinterlace ppmgr d2d3 amvideo" );
        break;

    case TV_PATH_DECODER_NEW3D_WITHOUTPPMGR_AMVIDEO:
        ret = VDIN_AddPath ( "add default decoder deinterlace d2d3 amvideo" );
        break;
    }

    return ret;
}

int CTvin::VDIN_RmPreviewPath ( void )
{
    int fd, ret;
    char str[] = "rm previewpath";


    fd = open ( "/sys/class/vfm/map", O_RDWR );

    if ( fd < 0 ) {
        LOGW ( "Open /sys/class/vfm/map error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = write ( fd, str, sizeof ( str ) );

    if ( ret < 0 ) {
        LOGW ( "Rm tv path error(%s)!\n", strerror ( errno ) );
    }

    close ( fd );
    fd = -1;

    return ret;
}

int CTvin::VDIN_OpenModule()
{
    char file_name[64];
    sprintf ( file_name, "/dev/vdin%d", CC_SEL_VDIN_DEV );
    m_vdin_dev_fd = open ( file_name, O_RDWR );

    if ( m_vdin_dev_fd < 0 ) {
        LOGW ( "Open %s error(%s)!\n", file_name, strerror ( errno ) );
        return -1;
    }

    memset ( &gTvinVDINParam, 0, sizeof ( gTvinVDINParam ) );
    memset ( &gTvinVDINSignalInfo, 0, sizeof ( gTvinVDINSignalInfo ) );

    LOGD ( "Open vdin module vdin_dev_fd = [%d]", m_vdin_dev_fd );

    return m_vdin_dev_fd;
}

int CTvin::VDIN_GetVdinFd()
{
    return m_vdin_dev_fd;
}

int CTvin::VDIN_CloseModule()
{
    if ( m_vdin_dev_fd != -1 ) {
        close ( m_vdin_dev_fd );
        m_vdin_dev_fd = -1;
    }

    return 0;
}

int CTvin::VDIN_DeviceIOCtl ( int request, ... )
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;

    if ( m_vdin_dev_fd >= 0 ) {
        va_start ( ap, request );
        arg = va_arg ( ap, void * );
        va_end ( ap );

        tmp_ret = ioctl ( m_vdin_dev_fd, request, arg );
        return tmp_ret;
    }

    return -1;
}

int CTvin::VDIN_OpenPort ( tvin_port_t port )
{
    int rt = -1;
    struct tvin_parm_s vdinParam;
    vdinParam.port = port;
    vdinParam.index = 0;
    rt = VDIN_DeviceIOCtl ( TVIN_IOC_OPEN, &vdinParam );

    if ( rt < 0 ) {
        LOGW ( "Vdin open port, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_ClosePort()
{
    int rt = -1;

    rt = VDIN_DeviceIOCtl ( TVIN_IOC_CLOSE );

    if ( rt < 0 ) {
        LOGW ( "Vdin close port, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_StartDec ( const struct tvin_parm_s *vdinParam )
{
    int rt = -1;

    if ( vdinParam == NULL ) {
        return -1;
    }

    rt = VDIN_DeviceIOCtl ( TVIN_IOC_START_DEC, vdinParam );

    LOGD ( "VDIN_StartDec:\n" );
    LOGD ( "index = [%d]\n", vdinParam->index );
    LOGD ( "port = [0x%x]\n", ( unsigned int ) vdinParam->port );
    LOGD ( "format = [0x%x]\n", ( unsigned int ) ( vdinParam->info.fmt ) );

    if ( rt < 0 ) {
        LOGW ( "Vdin start decode, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_StopDec()
{
    int rt = -1;

    rt = VDIN_DeviceIOCtl ( TVIN_IOC_STOP_DEC );

    if ( rt < 0 ) {
        LOGW ( "Vdin stop decode, error(%s)", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_GetSignalInfo ( struct tvin_info_s *SignalInfo )
{
    int rt = -1;

    rt = VDIN_DeviceIOCtl ( TVIN_IOC_G_SIG_INFO, SignalInfo );

    if ( rt < 0 ) {
        LOGW ( "Vdin get signal info, error(%s), ret = %d.\n", strerror ( errno ), rt );
        //system("reboot");
    }

    return rt;
}

int CTvin::VDIN_SetVdinParam ( const struct tvin_parm_s *vdinParam )
{
    int rt = -1, i = 0;

    rt = VDIN_DeviceIOCtl ( TVIN_IOC_S_PARM, vdinParam );

    if ( rt < 0 ) {
        LOGW ( "Vdin set signal param, error(%s)\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_GetVdinParam ( const struct tvin_parm_s *vdinParam )
{
    int rt = -1, i = 0;

    rt = VDIN_DeviceIOCtl ( TVIN_IOC_G_PARM, vdinParam );

    if ( rt < 0 ) {
        LOGW ( "Vdin get signal param, error(%s)\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::VDIN_OnoffVScaler ( int isOn )
{
    FILE *fp = NULL;

    if ( isOn == 1 ) {
        isOn = 1;
    } else {
        isOn = 0;
    }

    fp = fopen ( "/sys/class/video/vscaler", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/video/vscaler error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", ( int ) isOn );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_GetDisplayVFreq (void)
{
    int fd = -1;
    char buf[32];

    fd = open("/sys/class/display/mode", O_RDWR);
    if (fd < 0) {
        LOGW("Open /sys/class/display/mode error(%s)!\n", strerror(errno));
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    read(fd, buf, sizeof(buf));
    close(fd);
    LOGD( "%s, VDIN_GetDisplayVFreq is %s\n", __FUNCTION__, buf);
    if (strstr(buf, "50hz") != NULL) {
        return 50;
    } else if (strstr(buf, "60hz") != 0) {
        return 60;
    } else {
        return 50;
    }
}

int CTvin::VDIN_SetDisplayVFreq ( int freq, int display_resolution , bool isFbc)
{
    FILE *fp = NULL;
    const char *config_value = NULL;
    static int display_mode_type = -1;
    CFbcCommunication *pFBC = NULL;
    static int last_freq = 50;

    last_freq = VDIN_GetDisplayVFreq();
    if (last_freq == freq) {
        LOGD ( "%s, same freq, last_freq = %d, freq = %d.", __FUNCTION__, last_freq, freq);
        return 0;
    } else {
        LOGD ( "%s, from last_freq[%d[ to new_freq[%d].", __FUNCTION__, last_freq, freq);
        last_freq = freq;
    }

    if (display_mode_type == -1) {
        config_value = config_get_str ( CFG_SECTION_TV, "tvin.display.mode.type", "null" );
        if (strcasecmp(config_value, "null") == 0 || strcasecmp(config_value, "hdmi_out") == 0) {
            display_mode_type = 0;
        } else if (strcasecmp(config_value, "lvds_out") == 0) {
            display_mode_type = 1;
        } else {
            display_mode_type = 0;
        }
    }

    fp = fopen ( "/sys/class/display/mode", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/display/mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    if ( isFbc ) {
        pFBC = GetSingletonFBC();
        if (pFBC != NULL) {
            pFBC->cfbc_Set_VMute (COMM_DEV_SERIAL, 2);
            usleep ( 300000 );
        }
    }

    switch ( display_resolution ) {
    case VPP_DISPLAY_RESOLUTION_1366X768:
        if ( freq == 50 ) {
            fprintf ( fp, "%s", "768p50hz" );
        } else {
            fprintf ( fp, "%s", "768p60hz" );
        }
        break;
    case VPP_DISPLAY_RESOLUTION_3840X2160:
        if ( freq == 50 ) {
            if (isFbc) {
                fprintf ( fp, "%s", "2160p50hz420" );
            } else {
                fprintf ( fp, "%s", "2160p50hz" );
            }
        } else {
            if (isFbc) {
                fprintf ( fp, "%s", "2160p60hz420" );
            } else {
                fprintf ( fp, "%s", "2160p60hz" );
            }
        }
        break;
    case VPP_DISPLAY_RESOLUTION_1920X1080:
    default:
        if ( freq == 50 ) {
            fprintf ( fp, "%s", "1080p50hz" );
        } else {
            fprintf ( fp, "%s", "1080p60hz" );
        }
        break;
    }

    fclose ( fp );
    fp = NULL;

    return 0;
}

void CTvin::Tvin_SetDepthOf2Dto3D ( int value )
{
    //value = -16~16
    int tmp_value = DepthTable_2DTO3D[value + 16];
    LOGD ( "%s, value: %d", __FUNCTION__,  value );
    VDIN_Set2D3DDepth ( value );
}


int CTvin::VDIN_Set2D3DDepth ( int count )
{
    FILE *fp = NULL;


    fp = fopen ( "/sys/module/d2d3/parameters/depth", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/d2d3/parameters/depth ERROR(%s)!!\n", strerror ( errno ) );
        return -1;
    }

    if ( count >= -127 && count <= 127 ) {
#ifdef LOGD_3D_FUNCTION
        LOGD ( "set depth value (%d).\n", count );
#endif
    } else {
        count = 8 * 12;
        LOGE ( "set depth value ERROR!! set default depth.\n" );
    }

    fprintf ( fp, "%d", count );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_Set2Dto3D ( int on_off )
{
    struct tvin_parm_s VdinParam;
    VDIN_GetVdinParam ( &VdinParam );
    VdinParam.flag &= ( ~TVIN_PARM_FLAG_2D_TO_3D );
    VdinParam.flag |= ( on_off ) ? ( TVIN_PARM_FLAG_2D_TO_3D ) : ( 0 );
    return VDIN_SetVdinParam ( &VdinParam );
}

int CTvin::VDIN_Get_avg_luma(void)
{
    unsigned int lum_sum, pixel_sum, luma_avg;
    struct tvin_parm_s vdinParam;

    if ( 0 == VDIN_GetVdinParam( &vdinParam )) {
        lum_sum = vdinParam.luma_sum;
        pixel_sum = vdinParam.pixel_sum * 2;
        if (pixel_sum != 0 && m_is_decoder_start) {
            luma_avg = lum_sum / pixel_sum;
        } else {
            luma_avg = 116;
        }
    } else {
        return -1;
    }
    LOGD ( "VDIN_get_avg_lut lum_sum =%d,pixel_sum=%d,lut_avg=%d\n", lum_sum, pixel_sum, luma_avg);
    return luma_avg;
}

int CTvin::VDIN_GetHistgram ( int *hisgram )
{
    int i = 0;
    struct tvin_parm_s vdinParam;

    if ( NULL == hisgram ) {
        return -1;
    }

    if ( 0 == VDIN_GetVdinParam ( &vdinParam ) ) {
        for ( i = 0; i < CC_HIST_GRAM_BUF_SIZE; i++ ) {
            hisgram[i] = ( int ) vdinParam.histgram[i];
        }
    } else {
        return -1;
    }

    return 0;
}

int CTvin::VDIN_SetMVCViewMode ( int mode )
{
    FILE *fp = NULL;


    fp = fopen ( "/sys/module/amvdec_h264mvc/parameters/view_mode", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/amvdec_h264mvc/parameters/view_mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", ( int ) mode );

    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_GetMVCViewMode ( void )
{
    FILE *fp = NULL;
    int ret = 0;
    int mode = 0;


    fp = fopen ( "/sys/module/amvdec_h264mvc/parameters/view_mode", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/amvdec_h264mvc/parameters/view_mode ERROR(%s)!!\n", strerror ( errno ) );
        return -1;
    }

    ret = fread ( &mode, 1, 1, fp );
    LOGD ( "fread /sys/module/amvdec_h264mvc/parameters/view_mode = [%d]", mode );

    fclose ( fp );
    fp = NULL;

    return mode;
}

int CTvin::VDIN_SetDIBuffMgrMode ( int mgr_mode )
{
    FILE *fp = NULL;


    fp = fopen ( "sys/module/di/parameters/buf_mgr_mode", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/buf_mgr_mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", mgr_mode );

    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDICFG ( int cfg )
{
    FILE *fp = NULL;


    fp = fopen ( "sys/class/deinterlace/di0/config", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/deinterlace/di0/config error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    if ( 0 == cfg ) {
        fprintf ( fp, "%s", "disable" );
    } else {
        fprintf ( fp, "%s", "enable" );
    }

    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDI3DDetc ( int enable )
{
    FILE *fp = NULL;


    fp = fopen ( "/sys/module/di/parameters/det3d_en", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/det3d_en error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );


    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_Get3DDetc ( void )
{
    int fd = -1;
    int ret = -1;
    char buf[10];

    fd = open ( "/sys/module/di/parameters/det3d_en", O_RDWR );
    if ( fd < 0 ) {
        LOGW ( "Open /sys/module/di/parameters/det3d_en error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = read ( fd, buf, sizeof ( buf ) );
    close ( fd );
    fd = -1;

    if ( strcmp ( "enable", buf ) == 0 ) {
        return 1;
    } else {
        return 0;
    }
}


int CTvin::VDIN_GetVscalerStatus ( void )
{
    int fd = -1;
    int ret = -1;
    char buf[7];

    fd = open ( "/sys/class/video/vscaler", O_RDWR );
    if ( fd < 0 ) {
        LOGW ( "Open /sys/class/video/vscaler error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    ret = read ( fd, buf, sizeof ( buf ) );
    close ( fd );
    fd = -1;

    sscanf ( buf, "%d", &ret );

    ret = ( ( ret & 0x40000 ) == 0 ) ? 1 : 0;
    if ( ret == 1 ) {
        sleep ( 1 );
    }

    return ret;
}

int CTvin::VDIN_TurnOnBlackBarDetect ( int isEnable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/tvin_vdin/parameters/black_bar_enable", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/tvin_vdin/parameters/black_bar_enable error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", isEnable );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_LoadHdcpKey ( unsigned char *hdcpkey_buff __unused)
{
    unsigned char testHdcp[368] = { 0x53, 0x4B, 0x59, 0x01, 0x00, 0x10, 0x0D, 0x15, 0x3A, 0x8E, 0x99, 0xEE, 0x2A, 0x55, 0x58, 0xEE, 0xED, 0x4B, 0xBE, 0x00, 0x74, 0xA9, 0x00, 0x10, 0x0A, 0x21, 0xE3,
                                    0x30, 0x66, 0x34, 0xCE, 0x9C, 0xC7, 0x8B, 0x51, 0x27, 0xF9, 0x0B, 0xAD, 0x09, 0x5F, 0x4D, 0xC2, 0xCA, 0xA2, 0x13, 0x06, 0x18, 0x8D, 0x34, 0x82, 0x46, 0x2D, 0xC9, 0x4B, 0xB0, 0x1C, 0xDE,
                                    0x3D, 0x49, 0x39, 0x58, 0xEF, 0x2B, 0x68, 0x39, 0x71, 0xC9, 0x4D, 0x25, 0xE9, 0x75, 0x4D, 0xAC, 0x62, 0xF5, 0xF5, 0x87, 0xA0, 0xB2, 0x4A, 0x60, 0xD3, 0xF1, 0x09, 0x3A, 0xB2, 0x3E, 0x19,
                                    0x4F, 0x3B, 0x1B, 0x2F, 0x85, 0x14, 0x28, 0x44, 0xFC, 0x69, 0x6F, 0x50, 0x42, 0x81, 0xBF, 0x7C, 0x2B, 0x3A, 0x17, 0x2C, 0x15, 0xE4, 0x93, 0x77, 0x74, 0xE8, 0x1F, 0x1C, 0x38, 0x54, 0x49,
                                    0x10, 0x64, 0x5B, 0x7D, 0x90, 0x3D, 0xA0, 0xE1, 0x8B, 0x67, 0x5C, 0x19, 0xE6, 0xCA, 0x9D, 0xE9, 0x68, 0x5A, 0xB5, 0x62, 0xDF, 0xA1, 0x28, 0xBC, 0x68, 0x82, 0x9A, 0x22, 0xC4, 0xDC, 0x48,
                                    0x85, 0x0F, 0xF1, 0x3E, 0x05, 0xDD, 0x1B, 0x2D, 0xF5, 0x49, 0x3A, 0x15, 0x29, 0xE7, 0xB6, 0x0B, 0x2A, 0x40, 0xE3, 0xB0, 0x89, 0xD5, 0x75, 0x84, 0x2E, 0x76, 0xE7, 0xBC, 0x63, 0x67, 0xE3,
                                    0x57, 0x67, 0x86, 0x81, 0xF4, 0xD7, 0xEA, 0x4D, 0x89, 0x8E, 0x37, 0x95, 0x59, 0x1C, 0x8A, 0xCD, 0x79, 0xF8, 0x4F, 0x82, 0xF2, 0x6C, 0x7E, 0x7F, 0x79, 0x8A, 0x6B, 0x90, 0xC0, 0xAF, 0x4C,
                                    0x8D, 0x43, 0x47, 0x1F, 0x9A, 0xF1, 0xBB, 0x88, 0x64, 0x49, 0x14, 0x50, 0xD1, 0xC3, 0xDF, 0xA6, 0x87, 0xA0, 0x15, 0x98, 0x51, 0x81, 0xF5, 0x97, 0x55, 0x10, 0x4A, 0x99, 0x30, 0x54, 0xA4,
                                    0xFC, 0xDA, 0x0E, 0xAC, 0x6A, 0xFA, 0x90, 0xEE, 0x12, 0x70, 0x69, 0x74, 0x63, 0x46, 0x63, 0xFB, 0xE6, 0x1F, 0x72, 0xEC, 0x43, 0x5D, 0x50, 0xFF, 0x03, 0x4F, 0x05, 0x33, 0x88, 0x36, 0x93,
                                    0xE4, 0x72, 0xD5, 0xCC, 0x34, 0x52, 0x96, 0x15, 0xCE, 0xD0, 0x32, 0x52, 0x41, 0x4F, 0xBC, 0x2D, 0xDF, 0xC5, 0xD6, 0x7F, 0xD5, 0x74, 0xCE, 0x51, 0xDC, 0x10, 0x5E, 0xF7, 0xAA, 0x4A, 0x2D,
                                    0x20, 0x9A, 0x17, 0xDD, 0x30, 0x89, 0x71, 0x82, 0x36, 0x50, 0x09, 0x1F, 0x7C, 0xF3, 0x12, 0xE9, 0x43, 0x10, 0x5F, 0x51, 0xBF, 0xB8, 0x45, 0xA8, 0x5A, 0x8D, 0x3F, 0x77, 0xE5, 0x96, 0x73,
                                    0x68, 0xAB, 0x73, 0xE5, 0x4C, 0xFB, 0xE5, 0x98, 0xB9, 0xAE, 0x74, 0xEB, 0x51, 0xDB, 0x91, 0x07, 0x7B, 0x66, 0x02, 0x9B, 0x79, 0x03, 0xC5, 0x34, 0x1C, 0x58, 0x13, 0x31, 0xD2, 0x4A, 0xEC
                                  };
    int ret = -1;
    int fd = -1;

    fd = open ( "/sys/class/hdmirx/hdmirx0/edid", O_RDWR );
    if ( fd < 0 ) {
        LOGW ( "Open hdmi hdcp key error(%s)!!\n", strerror ( errno ) );
        return -1;
    }

    ret = write ( fd, testHdcp, 368 );
    if ( ret < 0 ) {
        LOGD ( "Write hdmi hdcp key error(%s)!!\n", strerror ( errno ) );
    }

    close ( fd );
    fd = -1;

    return ret;
}

int CTvin::VDIN_KeepLastFrame ( int enable __unused)
{
    return 0;
    /*
    FILE *fp = NULL;
    fp = fopen ( "/sys/module/amvideo/parameters/keep_old_frame", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/amvideo/parameters/keep_old_frame error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );

    fclose ( fp );
    fp = NULL;

    return 0;
    */
}

int CTvin::VDIN_SetVideoFreeze ( int enable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/class/vdin/vdin0/attr", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/vdin/vdin0/attr error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    if ( enable == 1 ) {
        fprintf ( fp, "freeze" );
    } else {
        fprintf ( fp, "unfreeze" );
    }

    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDIBypasshd ( int enable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/bypass_hd", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/bypass_hd error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDIBypassAll ( int enable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/bypass_all", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/bypass_all error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );
    fclose ( fp );
    fp = NULL;
    return 0;
}

int CTvin::VDIN_SetDIBypass_Get_Buf_Threshold ( int enable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/bypass_get_buf_threshold", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/bypass_get_buf_threshold error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );
    fclose ( fp );
    fp = NULL;

    return 0;

}

int CTvin::VDIN_SetDIBypassProg ( int enable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/bypass_prog", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/bypass_prog error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDIBypassDynamic ( int flag )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/bypass_dynamic", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/bypass_dynamic error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", flag );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDIDet3DMode ( int value )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/det3d_mode", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/det3d_mode error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", value );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDIBypass3D ( int enable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/bypass_3d", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/bypass_3d error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_SetDIBypassPost ( int enable )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/bypass_post", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/bypass_post error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::set3D_FL_Frame(int value)
{
    FILE *fp = NULL;
    fp = fopen ( "/sys/module/amvideo/parameters/pause_one_3d_fl_frame", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/amvideo/parameters/pause_one_3d_fl_frame error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    fprintf ( fp, "%d", value );
    fclose ( fp );
    fp = NULL;
    return 0;
}

int CTvin::setLatchFlag(int value)
{
    int fd = -1;
    int nodeVal = 0;
    char s[10];
    FILE *fp = NULL;
    memset(s, 0, sizeof(s));
    read(fd, s, sizeof(s));
    close(fd);
    nodeVal = atoi(s);
    fp = fopen ( "/sys/module/am_vecm/parameters/vecm_latch_flag", "w" );
    if ( fp == NULL ) {
        LOGW ( "/sys/module/am_vecm/parameters/vecm_latch_flag error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    fprintf ( fp, "%d", value | nodeVal);
    fclose ( fp );
    fp = NULL;
    LOGD ( "read vecm_latch_flag = (%s)====(0x%x), then set vecm_latch_flag | 0x%x = 0x%x.\n", s, nodeVal, value, value | nodeVal);
    return 0;
}

int CTvin::VDIN_SetDIProg_Proc_Config ( int value )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/prog_proc_config", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/prog_proc_config error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", value );
    fclose ( fp );
    fp = NULL;

    return 0;
}

#if(1)
int CTvin::VDIN_SetDIInput2Pre ( int value )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/module/di/parameters/input2pre", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/di/parameters/input2pre error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", value );
    fclose ( fp );
    fp = NULL;

    return 0;

}
#endif
int CTvin::VDIN_SetVdinFlag ( int flag )
{
    FILE *fp = NULL;
    int freq = 1200000;

    fp = fopen ( "/sys/class/vdin/memp", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/vdin/memp error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", flag );
    fclose ( fp );
    fp = NULL;

    return 0;
}

int CTvin::VDIN_EnableRDMA ( int enable )
{
    FILE *fp = NULL;
    fp = fopen ( "/sys/module/rdma/parameters/enable", "w" );

    if ( fp == NULL ) {
        LOGW ( "Open /sys/module/rdma/parameters/enable error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    fprintf ( fp, "%d", enable );
    fclose ( fp );
    fp = NULL;
    return 0;
}

// AFE
int CTvin::AFE_OpenModule ( void )
{
    if ( afe_dev_fd < 0 ) {
        afe_dev_fd = open ( AFE_DEV_PATH, O_RDWR );

        if ( afe_dev_fd < 0 ) {
            LOGW ( "Open tvafe module, error(%s).\n", strerror ( errno ) );
            return -1;
        }
    }

    return afe_dev_fd;
}

void CTvin::AFE_CloseModule ( void )
{
    if ( afe_dev_fd >= 0 ) {
        close ( afe_dev_fd );
        afe_dev_fd = -1;
    }

    return;
}

int CTvin::AFE_DeviceIOCtl ( int request, ... )
{
    int tmp_ret = -1;
    va_list ap;
    void *arg;

    if ( afe_dev_fd >= 0 ) {
        va_start ( ap, request );
        arg = va_arg ( ap, void * );
        va_end ( ap );

        tmp_ret = ioctl ( afe_dev_fd, request, arg );

        return tmp_ret;
    }

    return -1;
}

int CTvin::AFE_GetDeviceFileHandle()
{
    return afe_dev_fd;
}

int CTvin::AFE_SetVGAEdid ( const unsigned char *ediddata )
{
    int rt = -1;
    struct tvafe_vga_edid_s vgaEdid;

#ifdef NO_IC_TEST

    for ( int i = 0; i < 256; i++ ) {
        test_edid[i] = ediddata[i];
    }

#endif

    for ( int i = 0; i < 256; i++ ) {
        vgaEdid.value[i] = ediddata[i];
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_EDID, &vgaEdid );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetVGAEdid, error(%s).!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_GetVGAEdid ( unsigned char *ediddata )
{
    int rt = -1;
    struct tvafe_vga_edid_s vgaEdid;

#ifdef NO_IC_TEST

    for ( int i = 0; i < 256; i++ ) {
        ediddata[i] = test_edid[i];
    }

    LOGD ( "AFE_GetVGAEdid:\n" );
    LOGD ( "===================================================\n" );

    for ( int i = 0; i < 256; i++ ) {
        LOGD ( "vag edid[%d] = [0x%x].\n", i, ediddata[i] );
    }

    LOGD ( "===================================================\n" );
#endif

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_VGA_EDID, &vgaEdid );

    for ( int i = 0; i < 256; i++ ) {
        ediddata[i] = vgaEdid.value[i];
    }

    if ( rt < 0 ) {
        LOGW ( "AFE_GetVGAEdid, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_SetADCTimingAdjust ( const struct tvafe_vga_parm_s *timingadj )
{
    int rt = -1;

    if ( timingadj == NULL ) {
        return rt;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_PARM, timingadj );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetADCTimingAdjust, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_GetADCCurrentTimingAdjust ( struct tvafe_vga_parm_s *timingadj )
{
    int rt = -1;

    if ( timingadj == NULL ) {
        return rt;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_VGA_PARM, timingadj );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetADCCurrentTimingAdjust, error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    return 0;
}

int CTvin::AFE_VGAAutoAdjust ( struct tvafe_vga_parm_s *timingadj )
{
    enum tvafe_cmd_status_e CMDStatus = TVAFE_CMD_STATUS_PROCESSING;
    struct tvin_parm_s tvin_para;
    int rt = -1, i = 0;

    if ( timingadj == NULL ) {
        return -1;
    }

    for ( i = 0, CMDStatus == TVAFE_CMD_STATUS_PROCESSING; i < 50; i++ ) {
        rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, &CMDStatus );

        if ( rt < 0 ) {
            LOGD ( "get afe CMD status, error(%s), fd(%d), return(%d).\n", strerror ( errno ), AFE_GetDeviceFileHandle(), rt );
        }

        if ( ( CMDStatus == TVAFE_CMD_STATUS_IDLE ) || ( CMDStatus == TVAFE_CMD_STATUS_SUCCESSFUL ) ) {
            break;
        }

        usleep ( 10 * 1000 );
    }

    if ( ( CMDStatus == TVAFE_CMD_STATUS_PROCESSING ) || ( CMDStatus == TVAFE_CMD_STATUS_FAILED ) ) {
        return -1;
    }

    for ( i = 0; i < 100; i++ ) {
        rt = VDIN_DeviceIOCtl ( TVIN_IOC_G_PARM, &tvin_para );

        if ( tvin_para.info.status == TVIN_SIG_STATUS_STABLE ) {
            break;
        }

        usleep ( 10 * 1000 );
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_AUTO );

    if ( rt < 0 ) {
        timingadj->clk_step = 0;
        timingadj->phase = 0;
        timingadj->hpos_step = 0;
        timingadj->vpos_step = 0;
        AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_PARM, timingadj );
        return rt;
    } else {
        ;//AFE_DeviceIOCtl(TVIN_IOC_G_AFE_VGA_PARM, timingadj);
    }

    for ( i = 0; i < 10; i++ ) {
        sleep ( 1 );

        rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, &CMDStatus );

        if ( rt < 0 ) {
            return rt;
        } else {
            if ( CMDStatus == TVAFE_CMD_STATUS_SUCCESSFUL ) {
                usleep ( 100 * 1000 );
                AFE_GetADCCurrentTimingAdjust ( timingadj );
                LOGD ( "===================================================\n" );
                LOGW ( "AFE_VGAAutoAdjust, successfull!\n" );
                return 0;
            }
        }
    }

    return -1;
}

int CTvin::AFE_SetVGAAutoAjust ( void )
{
    int rt = -1;
    tvafe_vga_parm_t timingadj;
    tvafe_cmd_status_t Status;
    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, &Status );

    if ( ( Status == TVAFE_CMD_STATUS_IDLE ) || ( Status == TVAFE_CMD_STATUS_SUCCESSFUL ) ) {
        ;
    } else {
        LOGW ( "AFE_SetVGAAutoAjust, TVIN_IOC_G_AFE_CMD_STATUS failed!\n" );
        return -1;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_AUTO );

    if ( rt < 0 ) {
        timingadj.clk_step = 0;
        timingadj.phase = 0;
        timingadj.hpos_step = 0;
        timingadj.vpos_step = 0;
        AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_VGA_PARM, &timingadj );
        return rt;
    }

    return 0;
}

int CTvin::AFE_GetVGAAutoAdjustCMDStatus ( tvafe_cmd_status_t *Status )
{
    int rt = -1;

    if ( Status == NULL ) {
        return rt;
    }

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CMD_STATUS, Status );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetVGAAutoAdjustStatus, get status, error(%s) fd(%d) return(%d)\n", strerror ( errno ), AFE_GetDeviceFileHandle(), rt );
        return rt;
    }

    return 0;
}

int CTvin::AFE_GetAdcCal ( struct tvafe_adc_cal_s *adccalvalue )
{
    int rt = -1;

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_ADC_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetADCGainOffset, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_SetAdcCal ( struct tvafe_adc_cal_s *adccalvalue )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_ADC_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetAdcCal, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_GetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue )
{
    int rt = -1;

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_ADC_COMP_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetYPbPrADCGainOffset, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::AFE_SetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_ADC_COMP_CAL, adccalvalue );

    if ( rt < 0 ) {
        LOGW ( "AFE_SetYPbPrADCGainOffset, error(%s)!", strerror ( errno ) );
    }

    return rt;
}
int CTvin::AFE_GetYPbPrWSSinfo ( struct tvafe_comp_wss_s *wssinfo )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_COMP_WSS, wssinfo );

    if ( rt < 0 ) {
        LOGW ( "AFE_GetYPbPrWSSinfo, error(%s)!", strerror ( errno ) );
    }

    return rt;
}

#define RGB444      3
#define YCBCR422    2
#define YCBCR444    3
#define Y422_POS    0
#define CB422_POS   1
#define CR422_POS   3
#define Y444_POS    0
#define CB444_POS   1
#define CR444_POS   2
#define R444_POS    0
#define G444_POS    1
#define B444_POS    2

//=========== VGA =====================
#define VGA_BUF_WID         (VGA_H_ACTIVE)

#ifdef PATTERN_7_COLOR_BAR
#define VGA_BAR_WID     (VGA_H_ACTIVE/7)
#define VGA_H_CUT_WID   (10)
#else
#define VGA_BAR_WID     (VGA_H_ACTIVE/8)
#define VGA_H_CUT_WID   (10)
#endif

#define VGA_V_CUT_WID       (40)
#define VGA_V_CAL_WID       (200+VGA_V_CUT_WID)

#define VGA_WHITE_HS        (VGA_BAR_WID*0+VGA_H_CUT_WID)
#define VGA_WHITE_HE        (VGA_BAR_WID*1-VGA_H_CUT_WID-1)
#define VGA_WHITE_VS        (VGA_V_CUT_WID)
#define VGA_WHITE_VE        (VGA_V_CAL_WID-1)
#define VGA_WHITE_SIZE      ((VGA_WHITE_HE-VGA_WHITE_HS+1)*(VGA_WHITE_VE-VGA_WHITE_VS+1))
#ifdef PATTERN_7_COLOR_BAR
#define VGA_BLACK_HS    (VGA_BAR_WID*6+VGA_H_CUT_WID)
#define VGA_BLACK_HE    (VGA_BAR_WID*7-VGA_H_CUT_WID-1)
#define VGA_BLACK_VS        (768-140)
#define VGA_BLACK_VE    (768-40-1)
#define VGA_BLACK_SIZE  ((VGA_BLACK_HE-VGA_BLACK_HS+1)*(VGA_BLACK_VE-VGA_BLACK_VS+1))
#else
#define VGA_BLACK_HS    (VGA_BAR_WID*7+VGA_H_CUT_WID)
#define VGA_BLACK_HE    (VGA_BAR_WID*8-VGA_H_CUT_WID-1)
#define VGA_BLACK_VS    (VGA_V_CUT_WID)
#define VGA_BLACK_VE    (VGA_V_CAL_WID-1)
#define VGA_BLACK_SIZE  ((VGA_BLACK_HE-VGA_BLACK_HS+1)*(VGA_BLACK_VE-VGA_BLACK_VS+1))
#endif

//=========== YPBPR =====================
#define COMP_BUF_WID        (COMP_H_ACTIVE)

#define COMP_BAR_WID        (COMP_H_ACTIVE/8)
#define COMP_H_CUT_WID      (20)
#define COMP_V_CUT_WID      (100)
#define COMP_V_CAL_WID      (200+COMP_V_CUT_WID)

#define COMP_WHITE_HS       (COMP_BAR_WID*0+COMP_H_CUT_WID)
#define COMP_WHITE_HE       (COMP_BAR_WID*1-COMP_H_CUT_WID-1)
#define COMP_WHITE_VS       (COMP_V_CUT_WID)
#define COMP_WHITE_VE       (COMP_V_CAL_WID-1)
#define COMP_WHITE_SIZE     ((COMP_WHITE_HE-COMP_WHITE_HS+1)*(COMP_WHITE_VE-COMP_WHITE_VS+1))
#define CB_WHITE_SIZE       ((COMP_WHITE_HE-COMP_WHITE_HS+1)*(COMP_WHITE_VE-COMP_WHITE_VS+1)/2)
#define CR_WHITE_SIZE       ((COMP_WHITE_HE-COMP_WHITE_HS+1)*(COMP_WHITE_VE-COMP_WHITE_VS+1)/2)

#define COMP_YELLOW_HS      (COMP_BAR_WID*1+COMP_H_CUT_WID)
#define COMP_YELLOW_HE      (COMP_BAR_WID*2-COMP_H_CUT_WID-1)
#define COMP_YELLOW_VS      (COMP_V_CUT_WID)
#define COMP_YELLOW_VE      (COMP_V_CAL_WID-1)
#define COMP_YELLOW_SIZE    ((COMP_YELLOW_HE-COMP_YELLOW_HS+1)*(COMP_YELLOW_VE-COMP_YELLOW_VS+1)/2)

#define COMP_CYAN_HS        (COMP_BAR_WID*2+COMP_H_CUT_WID)
#define COMP_CYAN_HE        (COMP_BAR_WID*3-COMP_H_CUT_WID-1)
#define COMP_CYAN_VS        (COMP_V_CUT_WID)
#define COMP_CYAN_VE        (COMP_V_CAL_WID-1)
#define COMP_CYAN_SIZE      ((COMP_CYAN_HE-COMP_CYAN_HS+1)*(COMP_CYAN_VE-COMP_CYAN_VS+1)/2)

#define COMP_RED_HS         (COMP_BAR_WID*5+COMP_H_CUT_WID)
#define COMP_RED_HE         (COMP_BAR_WID*6-COMP_H_CUT_WID-1)
#define COMP_RED_VS         (COMP_V_CUT_WID)
#define COMP_RED_VE         (COMP_V_CAL_WID-1)
#define COMP_RED_SIZE       ((COMP_RED_HE-COMP_RED_HS+1)*(COMP_RED_VE-COMP_RED_VS+1)/2)

#define COMP_BLUE_HS        (COMP_BAR_WID*6+COMP_H_CUT_WID)
#define COMP_BLUE_HE        (COMP_BAR_WID*7-COMP_H_CUT_WID-1)
#define COMP_BLUE_VS        (COMP_V_CUT_WID)
#define COMP_BLUE_VE        (COMP_V_CAL_WID-1)
#define COMP_BLUE_SIZE      ((COMP_BLUE_HE-COMP_BLUE_HS+1)*(COMP_BLUE_VE-COMP_BLUE_VS+1)/2)

#define COMP_BLACK_HS       (COMP_BAR_WID*7+COMP_H_CUT_WID)
#define COMP_BLACK_HE       (COMP_BAR_WID*8-COMP_H_CUT_WID-1)
#define COMP_BLACK_VS       (COMP_V_CUT_WID)
#define COMP_BLACK_VE       (COMP_V_CAL_WID-1)
#define COMP_BLACK_SIZE     ((COMP_BLACK_HE-COMP_BLACK_HS+1)*(COMP_BLACK_VE-COMP_BLACK_VS+1))
#define CB_BLACK_SIZE       ((COMP_BLACK_HE-COMP_BLACK_HS+1)*(COMP_BLACK_VE-COMP_BLACK_VS+1)/2)
#define CR_BLACK_SIZE       ((COMP_BLACK_HE-COMP_BLACK_HS+1)*(COMP_BLACK_VE-COMP_BLACK_VS+1)/2)

//=========== CVBS =====================
#define CVBS_BUF_WID        (CVBS_H_ACTIVE)
#define CVBS_BAR_WID        (CVBS_H_ACTIVE/8)
#define CVBS_H_CUT_WID      (20)

#define CVBS_V_CUT_WID      (40)
#define CVBS_V_CAL_WID      (140+CVBS_V_CUT_WID)

#define CVBS_WHITE_HS       (CVBS_BAR_WID*0+CVBS_H_CUT_WID)
#define CVBS_WHITE_HE       (CVBS_BAR_WID*1-CVBS_H_CUT_WID-1)
#define CVBS_WHITE_VS       (CVBS_V_CUT_WID)
#define CVBS_WHITE_VE       (CVBS_V_CAL_WID-1)
#define CVBS_WHITE_SIZE     ((CVBS_WHITE_HE-CVBS_WHITE_HS+1)*(CVBS_WHITE_VE-CVBS_WHITE_VS+1))

#define CVBS_BLACK_HS       (CVBS_BAR_WID*7+CVBS_H_CUT_WID)
#define CVBS_BLACK_HE       (CVBS_BAR_WID*8-CVBS_H_CUT_WID-1)
#define CVBS_BLACK_VS       (CVBS_V_CUT_WID)
#define CVBS_BLACK_VE       (CVBS_V_CAL_WID-1)
#define CVBS_BLACK_SIZE     ((CVBS_BLACK_HE-CVBS_BLACK_HS+1)*(CVBS_BLACK_VE-CVBS_BLACK_VS+1))

#define COMP_CAP_SIZE       (COMP_H_ACTIVE*COMP_V_ACTIVE*YCBCR422)
#ifdef VGA_SOURCE_RGB444
#define VGA_CAP_SIZE    (VGA_H_ACTIVE*VGA_V_ACTIVE*RGB444)
#else
#define VGA_CAP_SIZE    (VGA_H_ACTIVE*VGA_V_ACTIVE*YCBCR444)
#endif
#define CVBS_CAP_SIZE       (CVBS_H_ACTIVE*CVBS_V_ACTIVE)

#define PRE_0       -16
#define PRE_1       -128
#define PRE_2       -128
#define COEF_00     1.164
#define COEF_01     0
#define COEF_02     1.793
#define COEF_10     1.164
#define COEF_11     -0.213
#define COEF_12     -0.534
#define COEF_20     1.164
#define COEF_21     2.115
#define COEF_22     0
#define POST_0      0
#define POST_1      0
#define POST_2      0

unsigned int CTvin::data_limit ( float data )
{
    if ( data < 0 ) {
        return ( 0 );
    } else if ( data > 255 ) {
        return ( 255 );
    } else {
        return ( ( unsigned int ) data );
    }
}

void CTvin::matrix_convert_yuv709_to_rgb ( unsigned int y, unsigned int u, unsigned int v, unsigned int *r, unsigned int *g, unsigned int *b )
{
    *r = data_limit ( ( ( float ) y + PRE_0 ) * COEF_00 + ( ( float ) u + PRE_1 ) * COEF_01 + ( ( float ) v + PRE_2 ) * COEF_02 + POST_0 + 0.5 );
    *g = data_limit ( ( ( float ) y + PRE_0 ) * COEF_10 + ( ( float ) u + PRE_1 ) * COEF_11 + ( ( float ) v + PRE_2 ) * COEF_12 + POST_1 + 0.5 );
    *b = data_limit ( ( ( float ) y + PRE_0 ) * COEF_20 + ( ( float ) u + PRE_1 ) * COEF_21 + ( ( float ) v + PRE_2 ) * COEF_22 + POST_2 + 0.5 );
}

void CTvin::re_order ( unsigned int *a, unsigned int *b )
{
    unsigned int c = 0;

    if ( *a > *b ) {
        c = *a;
        *a = *b;
        *b = c;
    }
}

char *CTvin::get_cap_addr ( enum adc_cal_type_e calType )
{
    int n;
    char *dp;

    for ( n = 0; n < 0x00ff; n++ ) {
        if ( VDIN_DeviceIOCtl ( TVIN_IOC_G_SIG_INFO, &gTvinAFESignalInfo ) < 0 ) {
            LOGW ( "get_cap_addr, get signal info, error(%s),fd(%d).\n", strerror ( errno ), m_vdin_dev_fd );
            return NULL;
        } else {
            if ( gTvinAFESignalInfo.status == TVIN_SIG_STATUS_STABLE ) {
                gTvinAFEParam.info.fmt = gTvinAFESignalInfo.fmt;
                break;
            }
        }
    }

    if ( gTvinAFESignalInfo.status != TVIN_SIG_STATUS_STABLE ) {
        LOGD ( "get_cap_addr, signal isn't stable, out of calibration!\n" );
        return NULL;
    } else {
        if ( VDIN_DeviceIOCtl ( TVIN_IOC_STOP_DEC ) < 0 ) {
            LOGW ( "get_cap_addr, stop vdin, error (%s).\n", strerror ( errno ) );
            return NULL;
        }

        usleep ( 1000 );

        if ( calType == CAL_YPBPR ) {
            dp = ( char * ) mmap ( NULL, COMP_CAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_vdin_dev_fd, 0 );
        } else {
            dp = ( char * ) mmap ( NULL, VGA_CAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_vdin_dev_fd, 0 );
        }

        if ( dp == NULL ) {
            LOGE ( "get_cap_addr, mmap failed!\n" );
        }

        return dp;
    }

    return NULL;
}

inline unsigned char CTvin::get_mem_data ( char *dp, unsigned int addr )
{
    return ( * ( dp + ( addr ^ 7 ) ) );
}

int CTvin::get_frame_average ( enum adc_cal_type_e calType, struct adc_cal_s *mem_data )
{
    unsigned int y = 0, cb = 0, cr = 0;
    unsigned int r = 0, g = 0, b = 0;
    unsigned long n;
    unsigned int i = 0, j = 0;
    char *dp = get_cap_addr ( calType );

    if ( calType == CAL_YPBPR ) {
        for ( j = COMP_WHITE_VS; j <= COMP_WHITE_VE; j++ ) {
            for ( i = COMP_WHITE_HS; i <= COMP_WHITE_HE; i++ ) {
                mem_data->g_y_max += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_max /= COMP_WHITE_SIZE;

        for ( j = COMP_WHITE_VS; j <= COMP_WHITE_VE; j++ ) {
            for ( i = COMP_WHITE_HS; i <= COMP_WHITE_HE; ) {
                mem_data->cb_white += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS ) );
                mem_data->cr_white += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->cb_white /= CB_WHITE_SIZE;
        mem_data->cr_white /= CR_WHITE_SIZE;

        for ( j = COMP_RED_VS; j <= COMP_RED_VE; j++ ) {
            for ( i = COMP_RED_HS; i <= COMP_RED_HE; ) {
                mem_data->rcr_max += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->rcr_max /= COMP_RED_SIZE;

        for ( j = COMP_BLUE_VS; j <= COMP_BLUE_VE; j++ ) {
            for ( i = COMP_BLUE_HS; i <= COMP_BLUE_HE; ) {
                mem_data->bcb_max += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS ) );
                i = i + 2;
            }
        }

        mem_data->bcb_max /= COMP_BLUE_SIZE;

        for ( j = COMP_BLACK_VS; j <= COMP_BLACK_VE; j++ ) {
            for ( i = COMP_BLACK_HS; i <= COMP_BLACK_HE; i++ ) {
                mem_data->g_y_min += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_min /= COMP_BLACK_SIZE;

        for ( j = COMP_BLACK_VS; j <= COMP_BLACK_VE; j++ ) {
            for ( i = COMP_BLACK_HS; i <= COMP_BLACK_HE; ) {
                mem_data->cb_black += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS ) );
                mem_data->cr_black += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->cb_black /= CB_BLACK_SIZE;
        mem_data->cr_black /= CR_BLACK_SIZE;

        /*
         for (j=COMP_BLACK_VS; j<=COMP_BLACK_VE; j++) {
         for (i=COMP_BLACK_HS; i<=COMP_BLACK_HE;) {
         //mem_data->cb_black += get_mem_data(dp, ((COMP_BUF_WID*j+i)*YCBCR422+CB422_POS));
         mem_data->cr_black += get_mem_data(dp, ((COMP_BUF_WID*j+i)*YCBCR422+CR422_POS));
         i = i+2;
         }
         }
         mem_data->cr_black /= CR_BLACK_SIZE;
         */
        for ( j = COMP_CYAN_VS; j <= COMP_CYAN_VE; j++ ) {
            for ( i = COMP_CYAN_HS; i <= COMP_CYAN_HE; ) {
                mem_data->rcr_min += get_mem_data ( dp, ( ( COMP_BUF_WID * j + i ) * YCBCR422 + CR422_POS ) );
                i = i + 2;
            }
        }

        mem_data->rcr_min /= COMP_CYAN_SIZE;

        for ( j = COMP_YELLOW_VS; j <= COMP_YELLOW_VE; j++ ) {
            for ( i = COMP_YELLOW_HS; i <= COMP_YELLOW_HE; ) {
                mem_data->bcb_min += get_mem_data ( dp, ( COMP_BUF_WID * j + i ) * YCBCR422 + CB422_POS );
                i = i + 2;
            }
        }

        mem_data->bcb_min /= COMP_YELLOW_SIZE;

    } else if ( calType == CAL_VGA ) {
        for ( j = VGA_WHITE_VS; j <= VGA_WHITE_VE; j++ ) {
            for ( i = VGA_WHITE_HS; i <= VGA_WHITE_HE; i++ ) {
#ifdef VGA_SOURCE_RGB444
                r = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + R444_POS ) );
                g = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + G444_POS ) );
                b = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + B444_POS ) );
#else
                y = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + Y444_POS ) );
                cb = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CB444_POS ) );
                cr = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CR444_POS ) );
                matrix_convert_yuv709_to_rgb ( y, cb, cr, &r, &g, &b );
#endif
                mem_data->rcr_max = mem_data->rcr_max + r;
                mem_data->g_y_max = mem_data->g_y_max + g;
                mem_data->bcb_max = mem_data->bcb_max + b;
            }
        }

        mem_data->rcr_max = mem_data->rcr_max / VGA_WHITE_SIZE;
        mem_data->g_y_max = mem_data->g_y_max / VGA_WHITE_SIZE;
        mem_data->bcb_max = mem_data->bcb_max / VGA_WHITE_SIZE;

        for ( j = VGA_BLACK_VS; j <= VGA_BLACK_VE; j++ ) {
            for ( i = VGA_BLACK_HS; i <= VGA_BLACK_HE; i++ ) {
#ifdef VGA_SOURCE_RGB444
                r = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + R444_POS ) );
                g = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + G444_POS ) );
                b = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * RGB444 + B444_POS ) );
#else
                y = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + Y444_POS ) );
                cb = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CB444_POS ) );
                cr = get_mem_data ( dp, ( ( VGA_BUF_WID * j + i ) * YCBCR444 + CR444_POS ) );
                matrix_convert_yuv709_to_rgb ( y, cb, cr, &r, &g, &b );
#endif
                mem_data->rcr_min = mem_data->rcr_min + r;
                mem_data->g_y_min = mem_data->g_y_min + g;
                mem_data->bcb_min = mem_data->bcb_min + b;
            }
        }

        mem_data->rcr_min = mem_data->rcr_min / VGA_BLACK_SIZE;
        mem_data->g_y_min = mem_data->g_y_min / VGA_BLACK_SIZE;
        mem_data->bcb_min = mem_data->bcb_min / VGA_BLACK_SIZE;

    } else { //CVBS
        for ( j = CVBS_WHITE_VS; j <= CVBS_WHITE_VE; j++ ) {
            for ( i = CVBS_WHITE_HS; i <= CVBS_WHITE_HE; i++ ) {
                mem_data->g_y_max += mem_data->g_y_max + get_mem_data ( dp, ( ( CVBS_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_max /= COMP_WHITE_SIZE;

        for ( j = CVBS_BLACK_VS; j <= CVBS_BLACK_VE; j++ ) {
            for ( i = CVBS_BLACK_HS; i <= CVBS_BLACK_HE; i++ ) {
                mem_data->g_y_min += mem_data->g_y_min + get_mem_data ( dp, ( ( CVBS_BUF_WID * j + i ) * YCBCR422 ) );
            }
        }

        mem_data->g_y_min /= CVBS_BLACK_SIZE;
    }

    if ( calType == CAL_YPBPR ) {
        munmap ( dp, COMP_CAP_SIZE );
    } else if ( calType == CAL_VGA ) {
        munmap ( dp, VGA_CAP_SIZE );
    } else {
        munmap ( dp, CVBS_CAP_SIZE );
    }

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_START_DEC, &gTvinAFEParam ) < 0 ) {
        LOGW ( "get_frame_average, get vdin signal info, error(%s),fd(%d).\n", strerror ( errno ), m_vdin_dev_fd );
        return 0;
    }

    return 0;
}

#define ADC_CAL_FRAME_QTY_ORDER 2 //NOTE:  MUST >=2!!
struct adc_cal_s CTvin::get_n_frame_average ( enum adc_cal_type_e calType )
{
    struct adc_cal_s mem_data = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned int rcrmax[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int rcrmin[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int g_ymax[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int g_ymin[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int bcbmax[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int bcbmin[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int cbwhite[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int crwhite[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int cbblack[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int crblack[1 << ADC_CAL_FRAME_QTY_ORDER];
    unsigned int i = 0, j = 0;

    for ( i = 0; i < ( 1 << ADC_CAL_FRAME_QTY_ORDER ); i++ ) {
        get_frame_average ( calType, &mem_data );
        rcrmax[i] = mem_data.rcr_max;
        rcrmin[i] = mem_data.rcr_min;
        g_ymax[i] = mem_data.g_y_max;
        g_ymin[i] = mem_data.g_y_min;
        bcbmax[i] = mem_data.bcb_max;
        bcbmin[i] = mem_data.bcb_min;
        cbwhite[i] = mem_data.cb_white;
        crwhite[i] = mem_data.cr_white;
        cbblack[i] = mem_data.cb_black;
        crblack[i] = mem_data.cr_black;
    }

    for ( i = 0; i < ( 1 << ADC_CAL_FRAME_QTY_ORDER ) - 1; i++ ) {
        for ( j = 1; j < ( 1 << ADC_CAL_FRAME_QTY_ORDER ); j++ ) {
            re_order ( & ( rcrmax[i] ), & ( rcrmax[j] ) );
            re_order ( & ( rcrmin[i] ), & ( rcrmin[j] ) );
            re_order ( & ( g_ymax[i] ), & ( g_ymax[j] ) );
            re_order ( & ( g_ymin[i] ), & ( g_ymin[j] ) );
            re_order ( & ( bcbmax[i] ), & ( bcbmax[j] ) );
            re_order ( & ( bcbmin[i] ), & ( bcbmin[j] ) );
            re_order ( & ( cbwhite[i] ), & ( cbwhite[j] ) );
            re_order ( & ( crwhite[i] ), & ( crwhite[j] ) );
            re_order ( & ( cbblack[i] ), & ( cbblack[j] ) );
            re_order ( & ( crblack[i] ), & ( crblack[j] ) );
        }
    }

    memset ( &mem_data, 0, sizeof ( mem_data ) );

    for ( i = 0; i < ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 1 ) ); i++ ) { //(1<<(ADC_CAL_FRAME_QTY_ORDER-1))
        mem_data.rcr_max += rcrmax[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.rcr_min += rcrmin[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.g_y_max += g_ymax[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.g_y_min += g_ymin[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.bcb_max += bcbmax[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.bcb_min += bcbmin[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cb_white += cbwhite[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cr_white += crwhite[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cb_black += cbblack[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
        mem_data.cr_black += crblack[i + ( 1 << ( ADC_CAL_FRAME_QTY_ORDER - 2 ) )];
    }


    mem_data.rcr_max >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.rcr_min >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.g_y_max >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.g_y_min >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.bcb_max >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.bcb_min >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cb_white >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cr_white >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cb_black >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );
    mem_data.cr_black >>= ( ADC_CAL_FRAME_QTY_ORDER - 1 );

    return mem_data;
}

int CTvin::AFE_GetMemData ( int typeSel, struct adc_cal_s *mem_data )
{
    int rt = -1;

    if ( m_vdin_dev_fd < 0 || mem_data == NULL ) {
        LOGW ( "AFE_GetMemData, didn't open vdin fd, return!\n" );
        return -1;
    }

    memset ( &gTvinAFEParam, 0, sizeof ( gTvinAFEParam ) );
    memset ( &gTvinAFESignalInfo, 0, sizeof ( gTvinAFESignalInfo ) );

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_G_PARM, &gTvinAFEParam ) < 0 ) {
        LOGW ( "AFE_GetMemData, get vdin param, error(%s), fd(%d)!\n", strerror ( errno ), m_vdin_dev_fd );
        return -1;
    }

    gTvinAFEParam.flag = gTvinAFEParam.flag | TVIN_PARM_FLAG_CAP;

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_S_PARM, &gTvinAFEParam ) < 0 ) {
        LOGW ( "AFE_GetMemData, set vdin param error(%s)!\n", strerror ( errno ) );
        return -1;
    }

    if ( typeSel == 0 ) {
        get_frame_average ( CAL_YPBPR, mem_data );
    } else if ( typeSel == 1 ) {
        get_frame_average ( CAL_VGA, mem_data );
    } else {
        *mem_data = get_n_frame_average ( CAL_CVBS );
    }

    gTvinAFEParam.flag &= 0x11111110;

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_S_PARM, &gTvinAFEParam ) < 0 ) {
        LOGW ( "AFE_GetMemData, set vdin param error(%s)\n", strerror ( errno ) );
        return -1;
    }

    LOGD ( "AFE_GetMemData, MAX ======> :\n Y(White)->%d \n Cb(Blue)->%d \n Cr(Red)->%d\n", mem_data->g_y_max, mem_data->bcb_max, mem_data->rcr_max );
    LOGD ( "AFE_GetMemData, MIN ======>:\n Y(Black)->%d \n Cb(Yellow)->%d \n Cr(Cyan)->%d\n Cb(White) ->%d\n Cb(Black)->%d\n Cr(Black)->%d\n", mem_data->g_y_min, mem_data->bcb_min, mem_data->rcr_min,
           mem_data->cb_white, mem_data->cb_black, mem_data->cr_black );
    return 0;
}

int CTvin::AFE_GetCVBSLockStatus ( enum tvafe_cvbs_video_e *cvbs_lock_status )
{
    int rt = -1;

    rt = AFE_DeviceIOCtl ( TVIN_IOC_G_AFE_CVBS_LOCK, cvbs_lock_status );

    if ( rt < 0 ) {
        LOGD ( "AFE_GetCVBSLockStatus, error: return(%d), error(%s)!\n", rt, strerror ( errno ) );
    } else {
        LOGD ( "AFE_GetCVBSLockStatus, value=%d.\n", *cvbs_lock_status );
    }

    return *cvbs_lock_status;
}

int CTvin::AFE_SetCVBSStd ( tvin_sig_fmt_t fmt )
{
    int rt = -1;

    LOGD ( "AFE_SetCVBSStd, sig_fmt = %d\n", fmt );
    rt = AFE_DeviceIOCtl ( TVIN_IOC_S_AFE_CVBS_STD, &fmt );

    if ( rt < 0 ) {
        LOGD ( "AFE_SetCVBSStd, error: return(%d), error(%s)!\n", rt, strerror ( errno ) );
    }

    return rt;
}

int CTvin::TvinApi_SetStartDropFrameCn ( int count )
{
    int ret = -1;
    char set_str[4];

    memset ( set_str, 0, 4 );
    sprintf ( set_str, "%d", count );
    return SetFileAttrValue ( "/sys/module/di/parameters/start_frame_drop_count", set_str );
}

int CTvin::TvinApi_SetVdinHVScale ( int vdinx, int hscale, int vscale )
{
    int ret = -1;
    char set_str[32];

    memset ( set_str, 0, 32 );
    sprintf ( set_str, "%s %d %d", "hvscaler", hscale, vscale );

    if ( vdinx == 0 ) {
        ret = SetFileAttrValue ( "/sys/class/vdin/vdin0/attr", set_str );
    } else {
        ret = SetFileAttrValue ( "/sys/class/vdin/vdin1/attr", set_str );
    }

    return ret;
}
int CTvin::TvinApi_SetCompPhase ( am_phase_t &am_phase )
{
    int ret = -1, fd = -1;
    unsigned int i = 0;
    int idx = 0, len = 0;
    char str1[200], str2[100];

    LOGD ( "enter,TvinApi_SetCompPhase" );

    fd = open ( "/sys/module/tvin_afe/parameters/comp_phase", O_RDWR );
    if ( fd < 0 ) {
        LOGW ( "Open vdin_comp_phase_op_mutex error(%s)!!\n", strerror ( errno ) );
        return -1;
    }

    for ( i = 0; i < am_phase.length; i++ ) {
        sprintf ( &str1[idx], "%d,", am_phase.phase[i] );
        sprintf ( str2, "%d,", am_phase.phase[i] );
        int len = strlen ( str2 );
        idx = idx + len;
    }

    LOGD ( "##########str1 = %s\n", str1 );

    ret = write ( fd, str1, strlen ( str1 ) );
    if ( ret < 0 ) {
        LOGD ( "Write vdin_comp_phase_op_mutex error(%s)!!\n", strerror ( errno ) );
    }

    LOGD ( "write ok!!!" );
    close ( fd );
    fd = -1;

    return ret;
}

tvin_trans_fmt CTvin::TvinApi_Get3DDectMode()
{
    int fd;
    int ret;
    char det_3d[10];
    int det_3dmode = 8;
    //LOGW("det_3dmode %d\n", det_3dmode);

    fd = open ( "/sys/module/di/parameters/det3d_mode", O_RDWR );
    if ( fd < 0 ) {
        LOGW ( "/sys/module/di/parameters/det3d_mode error(%s)!!\n", strerror ( errno ) );

        return  TVIN_TFMT_3D_MAX;
    }

    ret = read ( fd, det_3d, 10 );
    if ( ret < 0 ) {
        LOGW ( "/sys/module/di/parameters/det3d_mode error(%s)!!\n", strerror ( errno ) );
    }

    det_3dmode = atoi ( det_3d );
    close ( fd );
    fd = -1;

    return (tvin_trans_fmt)det_3dmode;
}

int CTvin::TvinApi_SetCompPhaseEnable ( int enable )
{
    int ret = -1;
    if ( enable == 1 ) {
        ret = SetFileAttrValue ( "/sys/module/tvin_afe/parameters/enable_dphase", "Y" );
        LOGD ( "%s, enable TvinApi_SetCompPhase.", CFG_SECTION_TV );
    }

    return ret;
}

int CTvin::VDIN_GetPortConnect ( int port )
{
    int status = 0;

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_CALLMASTER_SET, &port ) < 0 ) {
        LOGW ( "TVIN_IOC_CALLMASTER_SET error(%s) port %d\n", strerror ( errno ), port );
        return 0;
    }

    if ( VDIN_DeviceIOCtl ( TVIN_IOC_CALLMASTER_GET, &status ) < 0 ) {
        LOGW ( "TVIN_IOC_CALLMASTER_GET error(%s)\n", strerror ( errno ) );
        return 0;
    }

    //LOGD("%s, port:%x,status:%d", CFG_SECTION_TV,port,status);
    return status;
}

int CTvin::VDIN_OpenHDMIPinMuxOn ( bool flag )
{
    FILE *fp = NULL;

    fp = fopen ( "/sys/class/hdmirx/hdmirx0/debug", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/hdmirx/hdmirx0/debug(%s)!\n", strerror ( errno ) );
        return -1;
    }

    if ( flag ) {
        fprintf ( fp, "%s", "pinmux_on" );
    } else {
        fprintf ( fp, "%s", "pinmux_off" );

    }

    fclose ( fp );
    fp = NULL;
    return 1;
}

int CTvin::VDIN_GetHdmiHdcpKeyKsvInfo(struct _hdcp_ksv *msg)
{
    int m_ksv_dev_fd = -1;

    if (msg == NULL) {
        LOGE("%s, msg is NULL\n", __FUNCTION__);
    }

    //PrintMessage(__FUNCTION__, 0, msg);

    m_ksv_dev_fd = open(HDMIRX_KSV_PATH, O_RDWR);
    if (m_ksv_dev_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, HDMIRX_KSV_PATH, strerror ( errno ));
        return -1;
    }
    LOGD("# call ioctl with HDMI_IOC_HDCP_SENT_KSV #");
    ioctl(m_ksv_dev_fd, HDMI_IOC_HDCP_KSV, msg);

    close(m_ksv_dev_fd);
    m_ksv_dev_fd = -1;

    LOGD("msg->bksv0 is %x, msg->bksv1 is %x", msg->bksv0, msg->bksv1);

    return 0;
}


int CTvin::get_hdmi_ksv_info(int source_input, int data_buf[])
{
    if (source_input != SOURCE_HDMI1 && source_input != SOURCE_HDMI2 && source_input != SOURCE_HDMI3) {
        return -1;
    }

    struct _hdcp_ksv msg;
    int ret = -1;
    ret = VDIN_GetHdmiHdcpKeyKsvInfo(&msg);
    memset((void *)data_buf, 0, 2);
    data_buf[0] = msg.bksv0;
    data_buf[1] = msg.bksv1;
    return ret;
}


int CTvin::TVAFE_EnablePlugInDetect ( bool flag )
{
    FILE *fp = NULL;
    int status = 1;

    fp = fopen ( "/sys/class/tvafe/tvafe0/debug", "w" );
    if ( fp == NULL ) {
        LOGW ( "Open /sys/class/tvafe/tvafe0/debug (%s)!\n", strerror ( errno ) );
        return -1;
    }

    if ( flag ) {
        fprintf ( fp, "%s", "tvafe_enable" );
    } else {
        fprintf ( fp, "%s", "tvafe_down" );

    }

    fclose ( fp );
    fp = NULL;
    return status;
}

int CTvin::TvinApi_GetHDMIAudioStatus ( void )
{
    int fd;
    int val = 0;
    char bcmd[16];
    fd = open ( "/sys/module/tvin_hdmirx/parameters/auds_rcv_sts", O_RDONLY );

    if ( fd >= 0 ) {
        read ( fd, bcmd, sizeof ( bcmd ) );
        val = strtol ( bcmd, NULL, 10 );
        close ( fd );
    } else {
        LOGE ( "open /sys/module/tvin_hdmirx/parameters/auds_rcv_sts ERROR(%s)!!\n",  strerror ( errno ) );
        return -1;
    }

    return val;
}

int CTvin::TvinApi_LoadPLLValues ( am_regs_t regs )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_LOAD_REG, &regs );

    if ( rt < 0 ) {
        LOGE ( "TvinApi_LoadPLLValues, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::TvinApi_LoadCVD2Values ( am_regs_t regs )
{
    int rt = AFE_DeviceIOCtl ( TVIN_IOC_LOAD_REG, &regs );

    if ( rt < 0 ) {
        LOGE ( "TvinApi_LoadCVD2Values, error(%s)!\n", strerror ( errno ) );
    }

    return rt;
}

int CTvin::TvinApi_GetFbSize ( unsigned int *fb_width, unsigned int *fb_height )
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int xres = 0, yres = 0, bits_per_pixel = 0;

    fbfd = open ( "/dev/graphics/fb0", O_RDWR );
    if ( !fbfd ) {
        return -1;
    }

    if ( ioctl ( fbfd, FBIOGET_FSCREENINFO, &finfo ) ) {
        goto fail_close_fb;
    }

    if ( ioctl ( fbfd, FBIOGET_VSCREENINFO, &vinfo ) ) {
        goto fail_close_fb;
    }

    *fb_width = vinfo.xres;
    *fb_height = vinfo.yres;
    return 1;

fail_close_fb:
    close ( fbfd );;
    return -1;
}

tv_source_input_type_t CTvin::Tvin_SourceInputToSourceInputType ( tv_source_input_t source_input )
{
    if (source_input == SOURCE_TV) {
        return SOURCE_TYPE_TV;
    } else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
        return SOURCE_TYPE_AV;
    } else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2) {
        return SOURCE_TYPE_COMPONENT;
    } else if (source_input == SOURCE_VGA) {
        return SOURCE_TYPE_VGA;
    } else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
        return SOURCE_TYPE_HDMI;
    } else if (source_input == SOURCE_DTV) {
        return SOURCE_TYPE_DTV;
    } else if (source_input == SOURCE_IPTV) {
        return SOURCE_TYPE_IPTV;
    } else if (source_input == SOURCE_MPEG) {
        return SOURCE_TYPE_MPEG;
    }

    return SOURCE_TYPE_MPEG;
}

tv_source_input_type_t CTvin::Tvin_SourcePortToSourceInputType ( tvin_port_t source_port )
{
    tv_source_input_t source_input = Tvin_PortToSourceInput(source_port);
    return Tvin_SourceInputToSourceInputType(source_input);
}

tvin_port_t CTvin::Tvin_GetSourcePortBySourceType ( tv_source_input_type_t source_type )
{
    tvin_port_t source_port;

    switch ( source_type ) {
    case SOURCE_TYPE_TV:
        source_port = Tvin_GetSourcePortBySourceInput(SOURCE_TV);
        break;
    case SOURCE_TYPE_AV:
        source_port = Tvin_GetSourcePortBySourceInput(SOURCE_AV1);
        break;
    case SOURCE_TYPE_COMPONENT:
        source_port = Tvin_GetSourcePortBySourceInput(SOURCE_YPBPR1);
        break;
    case SOURCE_TYPE_VGA:
        source_port = Tvin_GetSourcePortBySourceInput(SOURCE_VGA);
        break;
    case SOURCE_TYPE_HDMI:
        source_port = TVIN_PORT_HDMI0;
        break;
    case SOURCE_TYPE_IPTV:
        source_port = Tvin_GetSourcePortBySourceInput(SOURCE_IPTV);
        break;
    case SOURCE_TYPE_DTV:
        source_port = Tvin_GetSourcePortBySourceInput(SOURCE_DTV);
        break;
    case SOURCE_TYPE_MPEG:
    default:
        source_port = Tvin_GetSourcePortBySourceInput(SOURCE_MPEG);
        break;
    }

    return source_port;
}

tvin_port_t CTvin::Tvin_GetSourcePortBySourceInput ( tv_source_input_t source_input )
{
    tvin_port_t source_port = TVIN_PORT_NULL;

    if ( source_input < SOURCE_TV || source_input >= SOURCE_MAX ) {
        source_port = TVIN_PORT_NULL;
    } else {
        source_port = ( tvin_port_t ) mSourceInputToPortMap[ ( int ) source_input];
    }

    return source_port;
}

tv_source_input_t CTvin::Tvin_PortToSourceInput ( tvin_port_t port )
{
    int i;

    for ( i = SOURCE_TV; i < SOURCE_MAX; i++ ) {
        if ( mSourceInputToPortMap[i] == (int)port ) {
            break;
        }
    }

    if ( i == SOURCE_MAX ) {
        return SOURCE_MAX;
    } else {
        return tv_source_input_t ( i );
    }
}

unsigned int CTvin::Tvin_TransPortStringToValue(const char *port_str)
{
    if (strcasecmp(port_str, "TVIN_PORT_CVBS0") == 0) {
        return TVIN_PORT_CVBS0;
    } else if (strcasecmp(port_str, "TVIN_PORT_CVBS1") == 0) {
        return TVIN_PORT_CVBS1;
    } else if (strcasecmp(port_str, "TVIN_PORT_CVBS2") == 0) {
        return TVIN_PORT_CVBS2;
    } else if (strcasecmp(port_str, "TVIN_PORT_CVBS3") == 0) {
        return TVIN_PORT_CVBS3;
    } else if (strcasecmp(port_str, "TVIN_PORT_COMP0") == 0) {
        return TVIN_PORT_COMP0;
    } else if (strcasecmp(port_str, "TVIN_PORT_COMP1") == 0) {
        return TVIN_PORT_COMP1;
    } else if (strcasecmp(port_str, "TVIN_PORT_VGA0") == 0) {
        return TVIN_PORT_VGA0;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI0") == 0) {
        return TVIN_PORT_HDMI0;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI1") == 0) {
        return TVIN_PORT_HDMI1;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI2") == 0) {
        return TVIN_PORT_HDMI2;
    } else if (strcasecmp(port_str, "TVIN_PORT_HDMI3") == 0) {
        return TVIN_PORT_HDMI3;
    }

    return TVIN_PORT_MPEG0;
}

void CTvin::Tvin_LoadSourceInputToPortMap()
{
    const char *config_value = NULL;

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.atv", "TVIN_PORT_CVBS3");
    mSourceInputToPortMap[SOURCE_TV] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.av1", "TVIN_PORT_CVBS1");
    mSourceInputToPortMap[SOURCE_AV1] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.av2", "TVIN_PORT_CVBS2");
    mSourceInputToPortMap[SOURCE_AV2] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.ypbpr1", "TVIN_PORT_COMP0");
    mSourceInputToPortMap[SOURCE_YPBPR1] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.ypbpr2", "TVIN_PORT_COMP1");
    mSourceInputToPortMap[SOURCE_YPBPR2] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.hdmi1", "TVIN_PORT_HDMI0");
    mSourceInputToPortMap[SOURCE_HDMI1] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.hdmi2", "TVIN_PORT_HDMI2");
    mSourceInputToPortMap[SOURCE_HDMI2] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.hdmi3", "TVIN_PORT_HDMI1");
    mSourceInputToPortMap[SOURCE_HDMI3] = Tvin_TransPortStringToValue(config_value);

    config_value = config_get_str(CFG_SECTION_SRC_INPUT, "ro.tv.tvinchannel.vga", "TVIN_PORT_VGA0");
    mSourceInputToPortMap[SOURCE_VGA] = Tvin_TransPortStringToValue(config_value);

    mSourceInputToPortMap[SOURCE_MPEG] = TVIN_PORT_MPEG0;
    mSourceInputToPortMap[SOURCE_DTV] = TVIN_PORT_DTV;
    mSourceInputToPortMap[SOURCE_IPTV] = TVIN_PORT_BT656;
}

int CTvin::Tvin_GetSourcePortByCECPhysicalAddress(int physical_addr)
{
    if (physical_addr == 0x1000) {
        return TVIN_PORT_HDMI0;
    } else if (physical_addr == 0x2000) {
        return TVIN_PORT_HDMI1;
    } else if (physical_addr == 0x3000) {
        return TVIN_PORT_HDMI2;
    }

    return TVIN_PORT_MAX;
}

tv_audio_channel_t CTvin::Tvin_GetInputSourceAudioChannelIndex ( tv_source_input_t source_input )
{
    int aud_chan = TV_AUDIO_LINE_IN_0;
    const char *config_value = NULL;

    if ( source_input == SOURCE_TV ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.atv", "2");
    } else if ( source_input == SOURCE_AV1 ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.av1", "1");
    } else if ( source_input == SOURCE_AV2 ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.av2", "3");
    } else if ( source_input == SOURCE_YPBPR1 ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.comp1", "0");
    } else if ( source_input == SOURCE_YPBPR2 ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.comp2", "0");
    } else if ( source_input == SOURCE_HDMI1 ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.hdmi1", "0");
    } else if ( source_input == SOURCE_HDMI2 ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.hdmi2", "0");
    } else if ( source_input == SOURCE_HDMI3 ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.hdmi3", "0");
    } else if ( source_input == SOURCE_VGA ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.vga", "0");
    } else if ( source_input == SOURCE_MPEG ) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.chan.mpeg", "0");
    }

    if (config_value != NULL) {
        aud_chan = strtol (config_value, NULL, 10);
    }

    return ( tv_audio_channel_t ) aud_chan;
}

tv_audio_in_source_type_t CTvin::Tvin_GetAudioInSourceType ( tv_source_input_t source_input )
{
    const char *config_value = NULL;

    if (source_input == SOURCE_TV) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.aud.insource.atv", "TV_AUDIO_IN_SOURCE_TYPE_LINEIN");
        if (strcasecmp(config_value, "TV_AUDIO_IN_SOURCE_TYPE_ATV") == 0) {
            return TV_AUDIO_IN_SOURCE_TYPE_ATV;
        }
    } else if (source_input == SOURCE_AV1 || source_input == SOURCE_AV2) {
        return TV_AUDIO_IN_SOURCE_TYPE_LINEIN;
    } else if (source_input == SOURCE_YPBPR1 || source_input == SOURCE_YPBPR2 || source_input == SOURCE_VGA) {
        return TV_AUDIO_IN_SOURCE_TYPE_LINEIN;
    } else if (source_input == SOURCE_HDMI1 || source_input == SOURCE_HDMI2 || source_input == SOURCE_HDMI3) {
        return TV_AUDIO_IN_SOURCE_TYPE_HDMI;
    }

    return TV_AUDIO_IN_SOURCE_TYPE_LINEIN;
}

int CTvin::isVgaFmtInHdmi ( tvin_sig_fmt_t fmt )
{
    if ( fmt == TVIN_SIG_FMT_HDMI_640X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_800X600_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1024X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X400_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X800_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X960_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X1024_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1360X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1366X768_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1600X1200_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1200_00HZ ) {
        LOGD ( "%s, HDMI source : VGA format.", CFG_SECTION_TV );
        return 1;
    }

    return -1;
}

int CTvin::isSDFmtInHdmi ( tvin_sig_fmt_t fmt )
{
    if ( fmt == TVIN_SIG_FMT_HDMI_640X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X240P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X240P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X480P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576P_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X480P_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_200HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_200HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X480P_240HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_240HZ
            || fmt == TVIN_SIG_FMT_HDMI_800X600_00HZ
            || fmt == TVIN_SIG_FMT_HDMI_720X400_00HZ ) {
        LOGD ( "%s, SD format.", CFG_SECTION_TV );
        return true;
    } else {
        LOGD ( "%s, HD format.", CFG_SECTION_TV );
        return false;
    }
}

bool CTvin::Tvin_is50HzFrameRateFmt ( tvin_sig_fmt_t fmt )
{
    /** component **/
    if ( fmt == TVIN_SIG_FMT_COMP_576P_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_576I_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_720P_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080P_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_A
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_B
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_C
            || fmt == TVIN_SIG_FMT_COMP_1080P_24HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080P_25HZ_D000
            /** hdmi **/
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1280X720P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X288P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080P_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B
            || fmt == TVIN_SIG_FMT_HDMI_1280X720P_50HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_720X576P_50HZ_FRAME_PACKING
            /** cvbs **/
            || fmt == TVIN_SIG_FMT_CVBS_PAL_I
            || fmt == TVIN_SIG_FMT_CVBS_PAL_M
            || fmt == TVIN_SIG_FMT_CVBS_PAL_CN
            || fmt == TVIN_SIG_FMT_CVBS_SECAM ) {
        LOGD ( "%s, Frame rate == 50Hz.", CFG_SECTION_TV );
        return true;
    } else {
        LOGD ( "%s, Frame rate != 50Hz.", CFG_SECTION_TV );
        return false;
    }
}

bool CTvin::Tvin_IsDeinterlaceFmt ( tvin_sig_fmt_t fmt )
{
    if ( fmt == TVIN_SIG_FMT_COMP_480I_59HZ_D940
            || fmt == TVIN_SIG_FMT_COMP_576I_50HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080I_47HZ_D952
            || fmt == TVIN_SIG_FMT_COMP_1080I_48HZ_D000
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_A
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_B
            || fmt == TVIN_SIG_FMT_COMP_1080I_50HZ_D000_C
            || fmt == TVIN_SIG_FMT_COMP_1080I_60HZ_D000
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_240HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_200HZ
            || fmt == TVIN_SIG_FMT_HDMI_1440X576I_50HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_100HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_120HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_FRAME_PACKING
            || fmt == TVIN_SIG_FMT_HDMI_2880X480I_60HZ
            || fmt == TVIN_SIG_FMT_HDMI_2880X576I_50HZ
            || fmt == TVIN_SIG_FMT_CVBS_NTSC_M
            || fmt == TVIN_SIG_FMT_CVBS_NTSC_443
            || fmt == TVIN_SIG_FMT_CVBS_PAL_60
            || fmt == TVIN_SIG_FMT_CVBS_PAL_CN
            || fmt == TVIN_SIG_FMT_CVBS_PAL_I
            || fmt == TVIN_SIG_FMT_CVBS_PAL_M
            || fmt == TVIN_SIG_FMT_CVBS_SECAM ) {
        LOGD ( "%s, Interlace format.", CFG_SECTION_TV );
        return true;
    } else {
        LOGD ( "%s, Progressive format.", CFG_SECTION_TV );
        return false;
    }
}

int CTvin::Tvin_StartDecoder ( tvin_info_t &info )
{
    if ( m_is_decoder_start == false ) {
        m_tvin_param.info = info;

        if ( VDIN_StartDec ( &m_tvin_param ) >= 0 ) {
            LOGD ( "StartDecoder succeed." );
            m_is_decoder_start = true;
        } else {
            LOGW ( "StartDecoder failed." );
            return -1;
        }
    } else {
        return -2;
    }

    return 0;
}

int CTvin::SwitchPort (tvin_port_t source_port )
{
    int ret = 0;
    LOGD ("%s, source_port = %x", __FUNCTION__,  source_port);
    ret = Tvin_StopDecoder();
    if ( 0 == ret || 1 == ret ) {
        if ( 1 == ret ) { //decode not started
            //mVpp.Tvin_SetVideoScreenColorType ( TV_SIGNAL_BLUE_PATTERN );
        }
        VDIN_ClosePort();
    } else {
        LOGW ( "%s,stop decoder failed.", __FUNCTION__);
        return -1;
    }
    // Open Port
    if ( VDIN_OpenPort ( source_port ) < 0 ) {
        LOGD ( "%s, OpenPort failed,  source_port =%x ", __FUNCTION__,  source_port );
    }
    m_tvin_param.port = source_port;
    return 0;
}

int CTvin::Tvin_StopDecoder()
{
    if ( m_is_decoder_start == true ) {
        if ( VDIN_StopDec() >= 0 ) {
            LOGD ( "StopDecoder ok!" );
            m_is_decoder_start = false;
            return 0;
        } else {
            LOGD ( "StopDecoder Failed!" );
        }
    } else {
        return 1;
    }

    return -1;
}

int CTvin::init_vdin ( void )
{
    VDIN_OpenModule();
    return 0;
}

int CTvin::uninit_vdin ( void )
{
    VDIN_ClosePort();
    VDIN_CloseModule();
    return 0;
}

int CTvin::Tvin_AddPath ( tvin_path_id_t pathid )
{
    int ret = -1;
    int i = 0, dly = 10;
    tv_path_type_t pathtype;

    if ( pathid >= TV_PATH_VDIN_AMVIDEO && pathid < TV_PATH_DECODER_3D_AMVIDEO ) {
        pathtype = TV_PATH_TYPE_TVIN;
    } else {
        pathtype = TV_PATH_TYPE_DEFAULT;
    }

    if ( pathid >= TV_PATH_VDIN_AMVIDEO && pathid < TV_PATH_DECODER_3D_AMVIDEO ) {
        if ( m_pathid == pathid ) {
            LOGW ( "%s, no need to add the same tvin path.\n", CFG_SECTION_TV );
            return 0;
        }

        for ( i = 0; i < 50; i++ ) {
            ret = VDIN_RmTvPath();

            if ( ret > 0 ) {
                LOGD ( "%s, remove tvin path ok, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                break;
            } else {
                LOGW ( "%s, remove tvin path faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                usleep ( dly * 1000 );
            }
        }
    } else {
        for ( i = 0; i < 50; i++ ) {
            ret = VDIN_RmDefPath();

            if ( ret > 0 ) {
                LOGD ( "%s, remove default path ok, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                break;
            } else {
                LOGW ( "%s, remove default path faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                usleep ( dly * 1000 );
            }
        }
    }

    for ( i = 0; i < 50; i++ ) {
        if ( pathid >= TV_PATH_VDIN_AMVIDEO && pathid < TV_PATH_DECODER_3D_AMVIDEO ) {
            if ( strcmp ( config_tv_path, "null" ) == 0 ) {
                ret = VDIN_AddVideoPath ( pathid );
            } else {
                ret = VDIN_AddPath ( config_tv_path );
            }
        } else {
            if ( strcmp ( config_default_path, "null" ) == 0 ) {
                ret = VDIN_AddVideoPath ( pathid );
            } else {
                ret = VDIN_AddPath ( config_default_path );
            }
        }

        if ( ret >= 0 ) {
            LOGD ( "%s, add pathid[%d] ok, %d ms gone.\n", CFG_SECTION_TV, pathid, i );
            break;
        } else {
            LOGW ( "%s, add pathid[%d] faild, %d ms gone.\n", CFG_SECTION_TV, pathid, i );
            usleep ( dly * 1000 );
        }
    }

    if ( pathid >= TV_PATH_VDIN_AMVIDEO && pathid < TV_PATH_MAX ) {
        m_pathid = pathid;
    }

    return ret;
}


int CTvin::Tvin_RemovePath ( tv_path_type_t pathtype )
{
    int ret = -1;
    int i = 0, dly = 10;

    for ( i = 0; i < 500; i++ ) {
        ret = Tvin_CheckPathActive ( pathtype, 0 );

        if ( ret == TV_PATH_STATUS_INACTIVE ) {
            LOGD ( "%s, check path is inactive, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
            break;
        } else if ( ret == TV_PATH_STATUS_INACTIVE || ret == TV_PATH_STATUS_ERROR ) {
            usleep ( dly * 1000 );
        } else {
            break;
        }
    }

    if ( i == 500 ) {
        LOGE ( "%s, check path active faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
    }

    if ( pathtype == TV_PATH_TYPE_DEFAULT ) {
        for ( i = 0; i < 50; i++ ) {
            ret = VDIN_RmDefPath();

            if ( ret > 0 ) {
                LOGD ( "%s, remove default path ok, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                break;
            } else {
                LOGW ( "%s, remove default path faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                usleep ( dly * 1000 );
            }
        }
    } else if ( pathtype == TV_PATH_TYPE_TVIN ) {
        for ( i = 0; i < 50; i++ ) {
            ret = VDIN_RmTvPath();

            if ( ret > 0 ) {
                LOGD ( "%s, remove tvin path ok, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                break;
            } else {
                LOGW ( "%s, remove tvin path faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                usleep ( dly * 1000 );
            }
        }

        m_pathid = TV_PATH_DECODER_3D_AMVIDEO;
    } else if ( pathtype == TV_PATH_TYPE_TVIN_PREVIEW ) {
        for ( i = 0; i < 50; i++ ) {
            ret = VDIN_RmPreviewPath();

            if ( ret > 0 ) {
                LOGD ( "%s, remove preview path ok, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                break;
            } else {
                LOGW ( "%s, remove preview path faild, %d ms gone.\n", CFG_SECTION_TV, ( dly * i ) );
                usleep ( dly * 1000 );
            }
        }

        m_pathid = TV_PATH_DECODER_NEW3D_WITHOUTPPMGR_AMVIDEO;
    } else {
        ret = -1;
    }

    return ret;
}


int CTvin::Tvin_CheckPathActive ( tv_path_type_t path_type, int isCheckD2D3 )
{
    FILE *f;
    char path[256];
    char decoder_str[20] = "default {";
    char tvin_str[20] = "tvpath {";
    char decoder_active_str[20] = "decoder(1)";
    char tvin_active_str[20] = "vdin0(1)";
    char di_active_str[20] = "deinterlace(1)";
    char d2d3_active_str[20] = "d2d3(1)";
    char decoder_inactive_str[20] = "decoder(0)";
    char tvin_inactive_str[20] = "vdin0(0)";
    char di_inactive_str[20] = "deinterlace(0)";
    char d2d3_inactive_str[20] = "d2d3(0)";

    char *str_find = NULL;
    char *active_str = NULL;
    char *inactive_str = NULL;
    int mount_freq;
    int match;
    int is_active = TV_PATH_STATUS_INACTIVE;

    memset ( path, 0, 255 );

    f = fopen ( "/sys/class/vfm/map", "r" );
    if ( !f ) {
        LOGE ( "%s, can not open /sys/class/vfm/map!\n", CFG_SECTION_TV );
        return TV_PATH_STATUS_NO_DEV;
    }

    while ( fgets ( path, 254, f ) ) {
        if ( path_type == TV_PATH_TYPE_DEFAULT ) {
            str_find = strstr ( path, decoder_str );
            active_str = decoder_active_str;
            inactive_str = decoder_inactive_str;
        } else if ( path_type == TV_PATH_TYPE_TVIN || path_type == TV_PATH_TYPE_TVIN_PREVIEW ) {
            str_find = strstr ( path, tvin_str );
            active_str = tvin_active_str;
            inactive_str = tvin_inactive_str;
        } else {
            LOGW ( "%s, there is no %d path_type.\n", CFG_SECTION_TV, path_type );
            break;
        }

        if ( str_find ) {
            if ( isCheckD2D3 == 0 ) {
                if ( strstr ( str_find, active_str ) && strstr ( str_find, di_active_str ) ) {
                    is_active = TV_PATH_STATUS_ACTIVE;
                    //LOGD("%s, %s is active.\n", CFG_SECTION_TV, path);
                } else if ( strstr ( str_find, inactive_str )
                            && ( strstr ( str_find, di_inactive_str ) || ( !strstr ( str_find, di_inactive_str ) ) )
                          ) {
                    is_active = TV_PATH_STATUS_INACTIVE;
                    //LOGD("%s, %s is inactive.\n", CFG_SECTION_TV, path);
                } else {
                    is_active = TV_PATH_STATUS_ERROR;
                    LOGE ( "%s, %s is error!\n", CFG_SECTION_TV, path );
                }

                break;
            } else {
                if ( strstr ( str_find, active_str ) && strstr ( str_find, di_active_str ) && strstr ( str_find, d2d3_active_str ) ) {
                    is_active = TV_PATH_STATUS_ACTIVE;
                    //LOGD("%s, %s is active.\n", CFG_SECTION_TV, path);
                } else if ( strstr ( str_find, inactive_str )
                            && ( strstr ( str_find, di_inactive_str ) || ( !strstr ( str_find, di_inactive_str ) ) )
                            && ( strstr ( str_find, d2d3_inactive_str ) || ( !strstr ( str_find, d2d3_inactive_str ) ) )
                          ) {
                    is_active = TV_PATH_STATUS_INACTIVE;
                    //LOGD("%s, %s is inactive.\n", CFG_SECTION_TV, path);
                } else {
                    is_active = TV_PATH_STATUS_ERROR;
                    LOGE ( "%s, %s is error!\n", CFG_SECTION_TV, path );
                }

                break;
            }
        }
    }

    fclose ( f );

    return is_active;
}


int CTvin::Tv_init_afe ( void )
{
    AFE_OpenModule();
    return 0;
}

int CTvin::Tv_uninit_afe ( void )
{
    AFE_CloseModule();
    return 0;
}

int CTvin::get_hdmi_sampling_rate()
{
    int fd;
    int val = 0;
    char bcmd[16];
    fd = open ( "/sys/module/tvin_hdmirx/parameters/audio_sample_rate", O_RDONLY );
    if ( fd >= 0 ) {
        read ( fd, bcmd, sizeof ( bcmd ) );
        val = strtol ( bcmd, NULL, 10 );
        close ( fd );
    }

    return val;
}

//**************************************************************************
CTvin::CTvinSigDetect::CTvinSigDetect ()
{
    mDetectState = STATE_STOPED;
    mpObserver = NULL;
    initSigState();
}

CTvin::CTvinSigDetect::~CTvinSigDetect()
{
}

int CTvin::CTvinSigDetect::startDetect(bool bPause)
{
    LOGD ( "startDetect()" );

    if ( mDetectState == STATE_RUNNING || mDetectState == STATE_PAUSE ) {
        return mDetectState;
    }

    m_request_pause_detect = bPause;
    initSigState();
    this->run();
    return mDetectState;
}

int CTvin::CTvinSigDetect::initSigState()
{
    m_cur_sig_info.trans_fmt = TVIN_TFMT_2D;
    m_cur_sig_info.fmt = TVIN_SIG_FMT_NULL;
    m_cur_sig_info.status = TVIN_SIG_STATUS_NULL;
    m_cur_sig_info.reserved = 0;
    m_pre_sig_info = m_cur_sig_info;
    mKeepNosigTime = 0;
    m_is_nosig_checktimes_once_valid = false;
    mResumeLaterTime = 0;
    return 0;
}

int CTvin::CTvinSigDetect::stopDetect()
{
    CMutex::Autolock _l ( mLock );
    LOGD ( "stopDetect()" );
    requestExit();
    return 0;
}

int CTvin::CTvinSigDetect::pauseDetect()
{
    CMutex::Autolock _l ( mLock );
    LOGD ( "pauseDetect()" );
    m_request_pause_detect = true;
    return 0;
}

int CTvin::CTvinSigDetect::requestAndWaitPauseDetect()
{
    CMutex::Autolock _l ( mLock );
    LOGD ( "requestAndWaitPauseDetect()" );
    m_request_pause_detect = true;

    if ( mDetectState == STATE_RUNNING ) {
        mRequestPauseCondition.wait ( mLock );
    }

    return 0;
}

int CTvin::CTvinSigDetect::resumeDetect(int later)//ms
{
    CMutex::Autolock _l ( mLock );
    LOGD ( "resumeDetect()" );
    mResumeLaterTime = later;
    m_request_pause_detect = false;
    mDetectPauseCondition.signal();
    return 0;
}

void CTvin::CTvinSigDetect::setVdinNoSigCheckKeepTimes(int times, bool isOnce)
{
    LOGD("setVdinNoSigCheckKeepTimes mKeepNosigTime = %d, times = %d", mKeepNosigTime, times);
    mKeepNosigTime = times;
    m_is_nosig_checktimes_once_valid = isOnce;
}

int CTvin::CTvinSigDetect::Tv_TvinSigDetect ( int &sleeptime )
{
    CTvin::getInstance()->VDIN_GetSignalInfo ( &m_cur_sig_info ); //get info
    //set no sig check times
    static long long sNosigKeepTime = 0;
    //LOGD("stime=%d status=%d, fmt = %d sNosigKeepTime = %d, mKeepNosigTime = %d", sleeptime, m_cur_sig_info.status,m_cur_sig_info.fmt, sNosigKeepTime, mKeepNosigTime);
    if ( m_cur_sig_info.status == TVIN_SIG_STATUS_NOSIG  || m_cur_sig_info.status == TVIN_SIG_STATUS_NULL ) {
        sNosigKeepTime += sleeptime;
        if ( sNosigKeepTime > mKeepNosigTime ) { //real no sig
            //cur is no sig
            if ( m_is_nosig_checktimes_once_valid ) { //just once change,is nosig, and default it
                m_is_nosig_checktimes_once_valid = false;
                mKeepNosigTime = 0;
            }
        } else {//not
            m_cur_sig_info.status = m_pre_sig_info.status;
        }
    } else {
        sNosigKeepTime = 0;
        if ( m_is_nosig_checktimes_once_valid ) { //just once change,not nosig,default is
            m_is_nosig_checktimes_once_valid = false;
            mKeepNosigTime = 0;
        }
    }

    //if state change
    if ( m_cur_sig_info.status != m_pre_sig_info.status ) {
        sleeptime = 20;

        if ( m_cur_sig_info.status == TVIN_SIG_STATUS_STABLE ) { // to stable
            mpObserver->onSigToStable();
        } else if ( m_pre_sig_info.status == TVIN_SIG_STATUS_STABLE && m_cur_sig_info.status == TVIN_SIG_STATUS_UNSTABLE ) { //stable to unstable
            //mVpp.Tvin_SetVideoScreenColorType ( TV_SIGNAL_BLACK_PATTERN );
            mpObserver->onSigStableToUnstable();
        } else if ( m_pre_sig_info.status == TVIN_SIG_STATUS_STABLE && m_cur_sig_info.status == TVIN_SIG_STATUS_NOTSUP ) {
            mpObserver->onSigStableToUnSupport();
        } else if ( m_pre_sig_info.status == TVIN_SIG_STATUS_STABLE && m_cur_sig_info.status == TVIN_SIG_STATUS_NOSIG ) {
            mpObserver->onSigStableToNoSig();
        } else if ( m_pre_sig_info.status == TVIN_SIG_STATUS_UNSTABLE && m_cur_sig_info.status == TVIN_SIG_STATUS_NOTSUP ) {
            mpObserver->onSigUnStableToUnSupport();
        } else if ( m_pre_sig_info.status == TVIN_SIG_STATUS_UNSTABLE && m_cur_sig_info.status == TVIN_SIG_STATUS_NOSIG ) {
            mpObserver->onSigUnStableToNoSig();
        } else if ( m_pre_sig_info.status == TVIN_SIG_STATUS_NULL && m_cur_sig_info.status == TVIN_SIG_STATUS_NOSIG ) {
            mpObserver->onSigNullToNoSig();
        } else if ( m_pre_sig_info.status == TVIN_SIG_STATUS_NOSIG && m_cur_sig_info.status == TVIN_SIG_STATUS_UNSTABLE ) {
            mpObserver->onSigNoSigToUnstable();
        }
    } else { //state not change
        sleeptime = 20;

        switch ( m_cur_sig_info.status ) {
        case TVIN_SIG_STATUS_STABLE:
            mpObserver->onSigStillStable();
            if ( m_cur_sig_info.trans_fmt != m_pre_sig_info.trans_fmt ) {
                mpObserver->onStableTransFmtChange();
            }
            if (m_cur_sig_info.fmt != m_pre_sig_info.fmt) {
                mpObserver->onStableSigFmtChange();
            }
            break;

        case TVIN_SIG_STATUS_NOTSUP:
            mpObserver->onSigStillNoSupport();
            break;

        case TVIN_SIG_STATUS_UNSTABLE:
            mpObserver->onSigStillUnstable();
            break;

        case TVIN_SIG_STATUS_NOSIG:
            mpObserver->onSigStillNosig();
            break;

        case TVIN_SIG_STATUS_NULL:
        default:
            mpObserver->onSigStillNull();
            break;
        }
    }

    m_pre_sig_info = m_cur_sig_info;//backup info
    return sleeptime;
}

bool CTvin::CTvinSigDetect::threadLoop()
{
    //enter onStart()
    if ( mpObserver == NULL ) {
        return false;
    }

    int sleeptime = 200;//ms
    mDetectState = STATE_RUNNING;
    mpObserver->onSigDetectEnter();

    while ( !exitPending() ) { //requietexit() or requietexitWait() not call
        while ( m_request_pause_detect ) {
            mLock.lock();
            mRequestPauseCondition.broadcast();
            mDetectState = STATE_PAUSE;
            mDetectPauseCondition.wait ( mLock ); //first unlock,when return,lock again,so need,call unlock
            mDetectState = STATE_RUNNING;
            mLock.unlock();
            //
            while (!m_request_pause_detect && mResumeLaterTime > 0) {
                //LOGD("mResumeLaterTime = %d", mResumeLaterTime);
                usleep(10 * 1000);
                mResumeLaterTime -= 10;
            }
        }

        mResumeLaterTime = 0;
        mpObserver->onSigDetectLoop();
        Tv_TvinSigDetect ( sleeptime );
        //可以优化
        if ( !m_request_pause_detect ) {
            usleep ( sleeptime * 1000 );
        }
    }

    mDetectState = STATE_STOPED;
    mRequestPauseCondition.broadcast();
    //exit
    //return true, run again, return false,not run.
    return false;
}

v4l2_std_id CTvin::CvbsFtmToV4l2ColorStd(tvin_sig_fmt_t fmt)
{
    v4l2_std_id v4l2_std;
    if (fmt == TVIN_SIG_FMT_CVBS_NTSC_M ||  fmt == TVIN_SIG_FMT_CVBS_NTSC_443) {
        v4l2_std = V4L2_COLOR_STD_NTSC;
    } else if (fmt >= TVIN_SIG_FMT_CVBS_PAL_I && fmt <= TVIN_SIG_FMT_CVBS_PAL_CN) {
        v4l2_std = V4L2_COLOR_STD_PAL;
    } else if (fmt == TVIN_SIG_FMT_CVBS_SECAM) {
        v4l2_std = V4L2_COLOR_STD_SECAM;
    } else {
        v4l2_std = V4L2_COLOR_STD_PAL;
    }
    return v4l2_std;
}

int CTvin::CvbsFtmToColorStdEnum(tvin_sig_fmt_t fmt)
{
    v4l2_std_id v4l2_std;
    if (fmt == TVIN_SIG_FMT_CVBS_NTSC_M ||  fmt == TVIN_SIG_FMT_CVBS_NTSC_443) {
        v4l2_std = CC_ATV_VIDEO_STD_NTSC;
    } else if (fmt >= TVIN_SIG_FMT_CVBS_PAL_I && fmt <= TVIN_SIG_FMT_CVBS_PAL_CN) {
        v4l2_std = CC_ATV_VIDEO_STD_PAL;
    } else if (fmt == TVIN_SIG_FMT_CVBS_SECAM) {
        v4l2_std = CC_ATV_VIDEO_STD_SECAM;
    } else {
        v4l2_std = CC_ATV_VIDEO_STD_PAL;
    }
    return v4l2_std;
}

int CTvin::GetITContent()
{
    FILE *fp = NULL;
    int value = 0;
    fp = fopen("/sys/module/tvin_hdmirx/parameters/it_content", "r");
    if (fp == NULL) {
        LOGE ( "Open /sys/module/tvin_hdmirx/parameters/it_content error(%s)!\n", strerror ( errno ) );
        return -1;
    }
    fscanf(fp, "%d", &value );
    fclose(fp);
    fp = NULL;
    return value;
}
//**************************************************************************

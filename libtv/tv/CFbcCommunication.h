#ifndef C_FBC_COMMUNICATION_H
#define C_FBC_COMMUNICATION_H
#include "tvutils/CHdmiCecCmd.h"
#include "tvutils/CSerialPort.h"
#include "tvutils/zepoll.h"
#include "tvutils/CThread.h"
#include <sys/time.h>
#include "tvutils//CTvInput.h"
#include "../tvutils/CMsgQueue.h"
typedef enum COMM_DEV_TYPE_NO {
    COMM_DEV_CEC = 0,
    COMM_DEV_SERIAL = 1,
} COMM_DEV_TYPE_E;
typedef enum vpu_modules_e {
    VPU_MODULE_NULL = 0,
    VPU_MODULE_VPU,        //vpu uint
    VPU_MODULE_TIMGEN,
    VPU_MODULE_PATGEN,
    VPU_MODULE_GAMMA,
    VPU_MODULE_WB,        //WhiteBalance
    VPU_MODULE_BC,        //Brightness&Contrast
    VPU_MODULE_BCRGB,    //RGB Brightness&Contrast
    VPU_MODULE_CM2,
    VPU_MODULE_CSC1,
    VPU_MODULE_DNLP,
    VPU_MODULE_CSC0,
    VPU_MODULE_OSD,
    VPU_MODULE_BLEND,
    VPU_MODULE_DEMURE,    //15
    VPU_MODULE_OUTPUT,    //LVDS/VX1 output
    VPU_MODULE_OSDDEC,    //OSD decoder
    VPU_MODULE_MAX,
} vpu_modules_t;

typedef enum fbc_command_e {
    VPU_CMD_NULL = 0,
    FBC_REBOOT_UPGRADE = 0x1,
    FBC_USER_SETTING_DEFAULT = 0x02,
    FBC_USER_SETTING_SET,
    FBC_GET_HDCP_KEY,
    FBC_PANEL_POWER,
    FBC_SUSPEND_POWER,
    //TOP CMD num:6
    VPU_CMD_INIT = 0x8, //parameter num 0
    VPU_CMD_ENABLE,     //parameter num 1;bit0~6:module;bit7:enable(1) or disable(0)
    VPU_CMD_BYPASS,     //parameter num 1;bit0~6:module;bit7:bypass(1) or not(0)
    VPU_CMD_OUTPUT_MUX, //parameter num 1;1:lvds;2:vx1;3:minilvds
    VPU_CMD_TIMING,     //parameter num 1;reference vpu_timing_t
    VPU_CMD_SOURCE,     //parameter num 1;reference vpu_source_t
    VPU_CMD_GAMMA_MOD,  //parameter num 1;reference vpu_gammamod_t
    VPU_CMD_D2D3 = 0xf,     //0xf:D2D3
    //

    CMD_BRIDGE_SW_VER = 0x10,
    CMD_DEVICE_ID,
    CMD_CLIENT_TYPE,
    CMD_DEVICE_NUM,
    CMD_ACTIVE_KEY,
    CMD_ACTIVE_STATUS,
    CMD_PANEL_INFO,
    CMD_LVDS_SSG_SET,

    CMD_DBG_REGISTER_ACCESS = 0x18,
    CMD_DBG_MEMORY_ACCESS,
    CMD_DBG_SPI_ACCESS,
    CMD_DBG_VPU_MEMORY_ACCESS,
    CMD_DBG_MEMORY_TRANSFER,
    CMD_INPUT_DOWN,
    CMD_INPUT_UP,
    CMD_FBC_MAIN_CODE_VERSION,

    //0x1f reserved
    //PQ+debug CMD num:32
    VPU_CMD_NATURE_LIGHT_EN = 0x20, //0 or 1;on or off  ????
    VPU_CMD_BACKLIGHT_EN,       //0 or 1;on or off
    VPU_CMD_BRIGHTNESS, //parameter num 2;parameter1:distinguish two modules;parameter2:ui value
    VPU_CMD_CONTRAST,   //parameter num 2;parameter1:distinguish two modules;parameter2:ui value
    VPU_CMD_BACKLIGHT,  //parameter num 1;
    VPU_CMD_RES1,       //reserved
    VPU_CMD_SATURATION, //parameter num 1;
    VPU_CMD_DYNAMIC_CONTRAST,   //0 or 1;??
    VPU_CMD_PICTURE_MODE,   //??
    VPU_CMD_PATTERN_EN, //0 or 1;on or off
    VPU_CMD_PATTEN_SEL, //0x2a parameter num 1;PATTEN SELECT
    VPU_CMD_RES2,
    VPU_CMD_RES3,
    VPU_CMD_RES4,
    VPU_CMD_RES5,
    VPU_CMD_USER_VMUTE = 0x2e,
    VPU_CMD_USER_GAMMA ,
    //0x30:sound_mode_def
    VPU_CMD_COLOR_TEMPERATURE_DEF = 0x31,   //def:factory setting
    VPU_CMD_BRIGHTNESS_DEF,
    VPU_CMD_CONTRAST_DEF,
    VPU_CMD_COLOR_DEF,
    VPU_CMD_HUE_DEF,
    VPU_CMD_BACKLIGHT_DEF,
    VPU_CMD_RES7,
    VPU_CMD_AUTO_LUMA_EN = 0x38,//0 or 1;on or off;appoint to backlight?
    VPU_CMD_HIST,       //parameter num 0;read hist info
    VPU_CMD_BLEND,      //parameter num ?;
    VPU_CMD_DEMURA,     //parameter num ?;
    VPU_CMD_CSC,        //parameter num ?;
    VPU_CMD_CM2,        //parameter num 1;index
    VPU_CMD_GAMMA,      //parameter num 1;index
    VPU_CMD_SRCIF,
    //WB CMD num:10
    VPU_CMD_RED_GAIN_DEF = 0x40,
    VPU_CMD_GREEN_GAIN_DEF,
    VPU_CMD_BLUE_GAIN_DEF,
    VPU_CMD_PRE_RED_OFFSET_DEF,
    VPU_CMD_PRE_GREEN_OFFSET_DEF,
    VPU_CMD_PRE_BLUE_OFFSET_DEF,
    VPU_CMD_POST_RED_OFFSET_DEF,
    VPU_CMD_POST_GREEN_OFFSET_DEF,
    VPU_CMD_POST_BLUE_OFFSET_DEF,
    VPU_CMD_RES9,
    VPU_CMD_WB = 0x4a,
    //DNLP PARM
    VPU_CMD_DNLP_PARM,
    VPU_CMD_WB_VALUE,
    VPU_CMD_GRAY_PATTERN,
    VPU_CMD_BURN,
    CMD_HDMI_STAT,
    VPU_CMD_READ = 0x80,
    //VPU_CMD_HUE_ADJUST,   //parameter num 1;
    //VPU_CMD_WB,       //parameter num 3;one parameter include two items so that six items can all be included
    VPU_CMD_MAX = 50,//temp define 50       //

    //audio cmd
    AUDIO_CMD_SET_SOURCE = 0x50,
    AUDIO_CMD_SET_MASTER_VOLUME,
    AUDIO_CMD_SET_CHANNEL_VOLUME,
    AUDIO_CMD_SET_SUBCHANNEL_VOLUME,
    AUDIO_CMD_SET_MASTER_VOLUME_GAIN,
    AUDIO_CMD_SET_CHANNEL_VOLUME_INDEX,
    AUDIO_CMD_SET_VOLUME_BAR,
    AUDIO_CMD_SET_MUTE,
    AUDIO_CMD_SET_EQ_MODE,
    AUDIO_CMD_SET_BALANCE,
    AUDIO_CMD_GET_SOURCE,
    AUDIO_CMD_GET_MASTER_VOLUME,
    AUDIO_CMD_GET_CHANNEL_VOLUME,
    AUDIO_CMD_GET_SUBCHANNEL_VOLUME,
    AUDIO_CMD_GET_MASTER_VOLUME_GAIN,
    AUDIO_CMD_GET_CHANNEL_VOLUME_INDEX,
    AUDIO_CMD_GET_VOLUME_BAR,
    AUDIO_CMD_GET_MUTE,
    AUDIO_CMD_GET_EQ_MODE,
    AUDIO_CMD_GET_BALANCE,

    VPU_CMD_SET_ELEC_MODE = 0x64,
    CMD_SET_LED_MODE   = 0x65,

    CMD_SET_FACTORY_SN = 0x66,
    CMD_GET_FACTORY_SN,
    CMD_COMMUNICATION_TEST,
    CMD_CLR_SETTINGS_DEFAULT,
    CMD_BLUETOOTH_I2S_STATUS = 0x6a,
    CMD_PANEL_ON_OFF = 0x6b,

    CMD_HDMI_REG   = 0x70,
    CMD_SET_PROJECT_SELECT = 0x71,
    CMD_GET_PROJECT_SELECT = 0x72,
    CMD_SET_LOCKN_DISABLE = 0x73, //0x73
    CMD_SET_SPLIT_SCREEN_DEMO = 0X74,
    CMD_SET_UBOOT_STAGE = 0x7b,

    CMD_SET_AUTO_BACKLIGHT_ONFF = 0x85,
    CMD_GET_AUTO_BACKLIGHT_ONFF = 0x86,
} fbc_command_t;

typedef struct REQUEST_REPLY_CMD {
    CCondition       WaitReplyCondition;
    int WaitDevNo;
    int WaitCmd;
    int WaitTimeOut;
    unsigned char *replyData;
    int reDataLen;
} REQUEST_REPLY_S;
class CFbcCommunication: public CThread {
public:
    static const unsigned char CEC_CMD_WIFI = 0x01;
    static const unsigned char CEC_CMD_ETHERNET = 0x02;
    static const unsigned char CEC_CMD_3D_TIMING = 0x03;
    static const unsigned char CEC_CMD_PANEL_REVERSE = 0x04;
    static const unsigned char CEC_CMD_VPU_INIT = 0x08;
    static const unsigned char CEC_CMD_VPU_ENABLE = 0x09;
    static const unsigned char CEC_CMD_VPU_BYPASS = 0x0a;
    static const unsigned char CEC_CMD_VPU_OUTPUT_MUX = 0x0b;
    static const unsigned char CEC_CMD_VPU_TIMING = 0x0c;
    static const unsigned char CEC_CMD_VPU_SOURCE = 0x0d;
    static const unsigned char CEC_CMD_BRIDGE_SW_VER = 0x10;
    static const unsigned char CEC_CMD_DEVICE_ID = 0x11;
    static const unsigned char CEC_CMD_CLIENT_TYPE = 0x12;
    static const unsigned char CEC_CMD_DEVICE_NUM = 0x13;
    static const unsigned char CEC_CMD_ACTIVE_KEY = 0x14;
    static const unsigned char CEC_CMD_ACTIVE_STATUS = 0x15;


    static const unsigned char CEC_CMD_DBG_REGISTER_ACCESS = 0x18;
    static const unsigned char CEC_CMD_DBG_MEMORY_ACCESS = 0x19;
    static const unsigned char CEC_CMD_DBG_SPI_ACCESS = 0x1a;
    static const unsigned char CEC_CMD_DBG_VPU_MEMORY_ACCESS = 0x1b;
    static const unsigned char CEC_CMD_DBG_MEMORY_TRANSFER = 0x1c;
    static const unsigned char CEC_CMD_INPUT_KEY_DOWN = 0x1d;
    static const unsigned char CEC_CMD_INPUT_KEY_UP = 0x1e;
    static const unsigned char CEC_CMD_REBOOT = 0x1f;
    static const unsigned char CEC_CMD_NATURE_LIGHT_ONOFF = 0x20;
    static const unsigned char CEC_CMD_USR_BACKLIGHT_ONOFF = 0x21;
    static const unsigned char CEC_CMD_USR_BRIGHTNESS = 0x22;
    static const unsigned char CEC_CMD_USR_CONTRAST = 0x23;
    static const unsigned char CEC_CMD_USR_BACKLIGHT = 0x24;
    static const unsigned char CEC_CMD_USR_SATURATION = 0x26;
    static const unsigned char CEC_CMD_USR_DYNAMIC_CONTRAST = 0x27;
    static const unsigned char CEC_CMD_USR_PICTURE_MODE = 0x28;
    static const unsigned char CEC_CMD_TEST_PATTERN_ONOFF = 0x29;
    static const unsigned char CEC_CMD_TEST_PATTERN_SELECT = 0x2a;
    static const unsigned char CEC_CMD_USER_GAMMA = 0x2f;
    static const unsigned char CEC_CMD_DEF_SOUND_MODE = 0x30;
    static const unsigned char CEC_CMD_DEF_COLOR_TEMPERATURE = 0x31;
    static const unsigned char CEC_CMD_DEF_BRIGHTNESS = 0x32;
    static const unsigned char CEC_CMD_DEF_CONTRAST = 0x33;
    static const unsigned char CEC_CMD_DEF_COLOR = 0x34;
    static const unsigned char CEC_CMD_DEF_BACKLIGHT = 0x36;
    static const unsigned char CEC_CMD_AUTO_LUMA_ONOFF = 0x38;
    static const unsigned char CEC_CMD_HISTOGRAM = 0x39;
    static const unsigned char CEC_CMD_BLEND = 0x3a;
    static const unsigned char CEC_CMD_DEMULA = 0x3b;
    static const unsigned char CEC_CMD_COLOR_SPACE_CONVERSION = 0x3c;
    static const unsigned char CEC_CMD_CM2 = 0x3d;
    static const unsigned char CEC_CMD_DEF_RED_GAIN = 0x40;
    static const unsigned char CEC_CMD_DEF_GREEN_GAIN = 0x41;
    static const unsigned char CEC_CMD_DEF_BLUE_GAIN = 0x42;
    static const unsigned char CEC_CMD_DEF_RED_OFFSET = 0x43;
    static const unsigned char CEC_CMD_DEF_GREEN_OFFSET = 0x44;
    static const unsigned char CEC_CMD_DEF_BLUE_OFFSET = 0x45;
    static const unsigned char CEC_CMD_DEF_PRE_RED_OFFSET = 0x46;
    static const unsigned char CEC_CMD_DEF_PRE_GREEN_OFFSET = 0x47;
    static const unsigned char CEC_CMD_DEF_PRE_BLUE_OFFSET = 0x48;
    static const unsigned char CEC_CMD_WHITEBALANCE = 0x4a;
    static const unsigned char CEC_CMD_SET_SW_VERSION = 0x57;
    static const unsigned char CEC_CMD_3D = 0xd0;

    friend class CTvMsgQueue;
    class CFbcMsgQueue: public CMsgQueueThread {
    public:
        static const int TV_MSG_COMMON = 0;
        static const int TV_MSG_SEND_KEY = 1;
    private:
        virtual void handleMessage ( CMessage &msg );
    };
    CFbcCommunication();
    ~CFbcCommunication();
    int start();
    //---------------------------------------------

    //---------------------------------------------
    void testUart();
    int handleCmd(COMM_DEV_TYPE_E fromDev, int cmd[], int *pRetValue);
    int sendDataOneway(int devno, unsigned char *pData, int dataLen, int flags);
    int sendDataAndWaitReply(int devno, int waitForDevno, int waitForCmd, unsigned char *pData, int dataLen, int timeout, unsigned char *pReData, int *reDataLen, int flags);
    int closeAll();
    int SetUpgradeFlag(int flag);
    int uartReadStream(unsigned char *retData, int rd_max_len, int timeout);
    unsigned int Calcrc32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len);
    int cfbc_Set_Gain_Red(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Gain_Red(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Gain_Green(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Gain_Green(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Gain_Blue(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Gain_Blue(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Offset_Red(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Offset_Red(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Offset_Green(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Offset_Green(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Offset_Blue(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Offset_Blue(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_WB_Initial(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_WB_Initial(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Test_Pattern(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Test_Pattern(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Picture_Mode(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Picture_Mode(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Contrast(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Contrast(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Brightness(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Brightness(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Saturation(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Saturation(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_HueColorTint(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_HueColorTint(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Backlight(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Backlight(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Source(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Source(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Mute(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Mute(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Volume_Bar(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Volume_Bar(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Balance(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Balance(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Master_Volume(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Master_Volume(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_CM(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_CM(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_DNLP(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_DNLP(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Gamma(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_Gamma(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_TYPE_E fromDev, int onOff);
    int cfbc_TestPattern_Select(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_WhiteBalance_SetGrayPattern(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_Get_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode, unsigned char *r_gain, unsigned char *g_gain, unsigned char *b_gain, unsigned char *r_offset, unsigned char *g_offset, unsigned char *b_offset);
    int cfbc_Set_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode, unsigned char r_gain, unsigned char g_gain, unsigned char b_gain, unsigned char r_offset, unsigned char g_offset, unsigned char b_offset);
    int cfbc_Set_backlight_onoff(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_backlight_onoff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_LVDS_SSG_Set(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_AUTO_ELEC_MODE(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_TYPE_E fromDev, char sw_ver[], char build_time[], char git_ver[], char git_branch[], char build_name[]);
    int cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_TYPE_E fromDev, char panel_model[]);
    int cfbc_Set_FBC_panel_power_switch(COMM_DEV_TYPE_E fromDev, int switch_val);
    int cfbc_Set_FBC_suspend(COMM_DEV_TYPE_E fromDev, int switch_val);
    int cfbc_FBC_Send_Key_To_Fbc(COMM_DEV_TYPE_E fromDev, int keycode, int param);
    int cfbc_Get_FBC_PANEL_REVERSE(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Get_FBC_PANEL_OUTPUT(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_FBC_project_id(COMM_DEV_TYPE_E fromDev, int prj_id);
    int cfbc_Get_FBC_project_id(COMM_DEV_TYPE_E fromDev, int *prj_id);
    int cfbc_SendRebootToUpgradeCmd(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_FBC_User_Setting_Default(COMM_DEV_TYPE_E fromDev, int param);
    int cfbc_Set_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, const char *pSNval) ;
    int cfbc_Get_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, char FactorySN[]);
    int cfbc_Set_FBC_Audio_Source(COMM_DEV_TYPE_E fromDev, int value);

    int cfbc_Set_Thermal_state(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_LightSensor_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_LightSensor_N310(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Dream_Panel_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Dream_Panel_N310(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_MULT_PQ_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_MULT_PQ_N310(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_MEMC_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_MEMC_N310(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_LockN_state(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_VMute(COMM_DEV_TYPE_E fromDev, unsigned char value);
    int cfbc_SET_SPLIT_SCREEN_DEMO(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Set_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int *value);
    int cfbc_Set_Led_onoff(COMM_DEV_TYPE_E fromDev, int val_1, int val_2, int val_3);
    int cfbc_Set_AP_STANDBY_N310(COMM_DEV_TYPE_E fromDev, int value);
    int cfbc_Get_AP_STANDBY_N310(COMM_DEV_TYPE_E fromDev, int *value);
    virtual int cfbc_Set_Fbc_Uboot_Stage(COMM_DEV_TYPE_E fromDev, int value);

private:
    //now,just one item in list,haha...
    void showTime(struct timeval *_time);
    long getTime(void);
    int rmFromRequestList();
    int addToRequestList();
    void initCrc32Table();
    void sendAckCmd(bool isOk);
    unsigned int GetCrc32(unsigned char *InStr, unsigned int len);
    int processData(COMM_DEV_TYPE_E fromDev, unsigned char *PData, int dataLen);
    int uartReadData(unsigned char *retData, int *retLen);
    int Fbc_Set_Value_INT8(COMM_DEV_TYPE_E toDev, int cmd_type, int value);
    int Fbc_Get_Value_INT8(COMM_DEV_TYPE_E fromDev, int cmd_type, int *value);
    int Fbc_Set_Value_INT32(COMM_DEV_TYPE_E toDev, int cmd_type, int value);
    int Fbc_Get_BatchValue(COMM_DEV_TYPE_E toDev, unsigned char *cmd_buf, int count);
    int Fbc_Set_BatchValue(COMM_DEV_TYPE_E fromDev, unsigned char *cmd_buf, int count);
    int cfbc_Set_Get_BatchValue(COMM_DEV_TYPE_E fromDev, unsigned char *cmd_buf, int count, int isSet);
    bool        threadLoop();

    int mUpgradeFlag;
    CHdmiCec mHdmiCec;
    CSerialPort mSerialPort;
    Epoll       mEpoll;
    mutable CMutex           mLock;
    REQUEST_REPLY_S mReplyList;
    //list
    epoll_event m_event;
    unsigned char *mpRevDataBuf;
    unsigned int mCrc32Table[256];
    bool mbSendKeyCode;
    CTvInput mTvInput;
    CFbcMsgQueue mFbcMsgQueue;
    int mbDownHaveSend;

    int mbFbcKeyEnterDown;
    nsecs_t mFbcEnterKeyDownTime;
};

extern CFbcCommunication *GetSingletonFBC();

#endif

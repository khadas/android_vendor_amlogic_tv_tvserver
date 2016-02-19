#ifndef __TV_MISC_H__
#define __TV_MISC_H__

#include "../tv/CFbcCommunication.h"
#define CC_MIN_ADC_CHANNEL_VAL              (0)
#define CC_MAX_ADC_CHANNEL_VAL              (5)

#define CC_I2C_BUS_ON                       (0)
#define CC_I2C_BUS_OFF                      (1)

#define CPQDB_DB_SIZE_ID       "tv.db.size.id"

struct erase_info_user {
    uint32_t start;
    uint32_t length;
};

extern int *GetFileAttrIntValueStr(const char *fp);
extern int ReadADCSpecialChannelValue(int adc_channel_num);
extern int Tv_MiscRegs(const char *cmd);

extern int TvMisc_SetLVDSSSC(int val);
extern int TvMisc_SetUserCounterTimeOut(int timeout);
extern int TvMisc_SetUserCounter(int count);
extern int TvMisc_SetUserPetResetEnable(int enable);
extern int TvMisc_SetSystemPetResetEnable(int enable);
extern int TvMisc_SetSystemPetEnable(int enable);
extern int TvMisc_SetSystemPetCounter(int count);
extern void TvMisc_EnableWDT(bool kernelpet_disable, unsigned int userpet_enable, unsigned int kernelpet_timeout, unsigned int userpet_timeout, unsigned int userpet_reset);
extern void TvMisc_DisableWDT(unsigned int userpet_enable);
extern int I2C_WriteNbyte(int i2c_no, int dev_addr, int slave_addr, int len, unsigned char data_buf[]);
extern int I2C_ReadNbyte(int i2c_no, int dev_addr, int slave_addr, int len, unsigned char data_buf[]);
extern int GetTvDBDefineSize();
extern int SetFileAttrValue(const char *fp, const char value[]);
extern int GetFileAttrIntValue(const char *fp);

extern int Get_Fixed_NonStandard(void);
extern int Set_Fixed_NonStandard(int value);

extern int get_hardware_name(char *hardware);

extern int TvMisc_DeleteDirFiles(const char *strPath, int flag);

extern int cfg_get_one_item(const char *key_str, const char *strDelimit, int item_index, char cfg_str[]);
extern int Tv_Utils_CheckFs(void);
extern int Tv_Utils_SetFileAttrStr(const char *file_path, char val_str_buf[]);
extern int Tv_Utils_GetFileAttrStr(const char *file_path, int buf_size, char val_str_buf[]);
extern bool Tv_Utils_IsFileExist(const char *file_name);
extern void monitor_info_name_init ( unsigned char *edidbuf );
extern void monitor_info_set_name ( unsigned char *edidbuf );
extern void monitor_info_set_imagesize ( unsigned char *edidbuf );
extern void monitor_info_edid_checksum ( unsigned char *edidbuf );
extern int reboot_sys_by_fbc_edid_info();
extern int reboot_sys_by_fbc_uart_panel_info(CFbcCommunication *fbc = NULL);
extern int GetPlatformHaveDDFlag();

#define CC_PROJECT_INFO_ITEM_MAX_LEN  (64)

typedef struct project_info_s {
    char version[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_type[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_outputmode[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_rev[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_name[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char amp_curve_name[CC_PROJECT_INFO_ITEM_MAX_LEN];
} project_info_t;

extern unsigned int CalCRC32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len);
extern int GetProjectInfo(project_info_t *proj_info_ptr, CFbcCommunication *fbcIns = NULL);
extern int getBootEnv(const char *key, char *value, char *def_val);
extern void setBootEnv(const char *key, const char *value);

//extern void SSMRewriteEdidInfo ( unsigned char *edidbuf );
//extern int HandleEdid ( int op_type, int op_direct, unsigned char edid_buf[], unsigned char def_buf[] );
//extern int  SSMSetHdmiEdid();
#define AML_DBG_REGS_IOC_MAGIC 'R'

typedef struct aml_debug_reg_s {
    unsigned int addr;
    unsigned int val;
    unsigned char mode;
} aml_debug_reg_t;

typedef struct aml_debug_bit_s {
    unsigned int addr;
    unsigned int val;
    unsigned int start;
    unsigned int len;
} aml_debug_bit_t;

template<typename T1, typename T2>
int ArrayCopy(T1 dst_buf[], T2 src_buf[], int copy_size)
{
    int i = 0;

    for (i = 0; i < copy_size; i++) {
        dst_buf[i] = src_buf[i];
    }

    return 0;
};

#endif  //__TV_MISC_H__

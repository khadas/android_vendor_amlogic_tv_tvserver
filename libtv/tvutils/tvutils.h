#ifndef __TV_MISC_H__
#define __TV_MISC_H__

#include "../tv/CFbcCommunication.h"
#define CC_MIN_ADC_CHANNEL_VAL              (0)
#define CC_MAX_ADC_CHANNEL_VAL              (5)
#define SYS_STR_LEN                         1024

int tvReadSysfs(const char *path, char *value);
int tvWriteSysfs(const char *path, const char *value);

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
extern int GetTvDBDefineSize();

extern int Set_Fixed_NonStandard(int value);


extern int TvMisc_DeleteDirFiles(const char *strPath, int flag);

extern bool Tv_Utils_IsFileExist(const char *file_name);
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
extern int getBootEnv(const char *key, char *value, const char *def_val);
extern void setBootEnv(const char *key, const char *value);
extern int readSysfs(const char *path, char *value);
extern void writeSysfs(const char *path, const char *value);

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

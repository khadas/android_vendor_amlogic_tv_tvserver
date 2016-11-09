#ifndef _TV_UTILS_H_
#define _TV_UTILS_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <map>

#define SYS_STR_LEN                         1024

int tvReadSysfs(const char *path, char *value);
int tvWriteSysfs(const char *path, const char *value);
int tvWriteSysfs(const char *path, int value, int base=10);


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

extern bool isFileExist(const char *file_name);
extern int GetPlatformHaveDDFlag();
extern int getBootEnv(const char *key, char *value, const char *def_val);
extern void setBootEnv(const char *key, const char *value);
extern int readSysfs(const char *path, char *value);
extern void writeSysfs(const char *path, const char *value);
extern int GetFileAttrIntValue(const char *fp, int flag = O_RDWR);

template<typename T1, typename T2>
int ArrayCopy(T1 dst_buf[], T2 src_buf[], int copy_size)
{
    int i = 0;

    for (i = 0; i < copy_size; i++) {
        dst_buf[i] = src_buf[i];
    }

    return 0;
};


typedef std::map<std::string, std::string> STR_MAP;

extern void jsonToMap(STR_MAP &m, const std::string &j);
extern void mapToJson(std::string &j, const STR_MAP &m);
extern void mapToJson(std::string &j, const STR_MAP &m, const std::string &k);
extern void mapToJsonAppend(std::string &j, const STR_MAP &m, const std::string &k);

class Paras {
protected:
    STR_MAP mparas;

public:
    Paras() {}
    Paras(const Paras &paras):mparas(paras.mparas) {}
    Paras(const char *paras) { jsonToMap(mparas, std::string(paras)); }
    Paras(const STR_MAP &paras):mparas(paras) {}

    void clear() { mparas.clear(); }

    int getInt (const char *key, int def) const;
    void setInt(const char *key, int v);

    const std::string toString() { std::string s; mapToJson(s, mparas); return s; }

    Paras operator + (const Paras &p);
    Paras& operator = (const Paras &p) { mparas = p.mparas; return *this; };
};


#endif  //__TV_MISC_H__

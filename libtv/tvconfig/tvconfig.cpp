#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "tvconfig.h"

#define LOG_TAG "TVCONFIG"
#include "CTvLog.h"
#include "CIniFile.h"
static const char *TV_SECTION = "TV";
//INI_CONFIG* mpConfig = NULL;
static char mpFilePath[256] = {0};

static CIniFile *pIniFile = NULL;
int tv_config_load(const char *file_name)
{
    if (pIniFile != NULL)
        delete pIniFile;

    pIniFile = new CIniFile();
    pIniFile->LoadFromFile(file_name);
    strcpy(mpFilePath, file_name);
    return 0;
}

int tv_config_unload()
{
    if (pIniFile != NULL)
        delete pIniFile;
    return 0;
}


int config_set_str(const char *section, const char *key, const char *value)
{
    return pIniFile->SetString(section, key, value);
}

const char *config_get_str(const char *section,  const char *key, const char *def_value)
{
    return pIniFile->GetString(section, key, def_value);
}

int config_get_int(const char *section, const char *key, const int def_value)
{
    return pIniFile->GetInt(section, key, def_value);
}

int config_set_int(const char *section, const char *key, const int value)
{
    pIniFile->SetInt(section, key, value);
    return 0;
}

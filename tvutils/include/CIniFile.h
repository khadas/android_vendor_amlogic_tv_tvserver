/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef INI_FILE_H_
#define INI_FILE_H_

static const int MAX_INI_FILE_LINE_LEN = 512;

typedef enum _LINE_TYPE {
    LINE_TYPE_SECTION = 0,
    LINE_TYPE_KEY,
    LINE_TYPE_COMMENT,
} LINE_TYPE;

//基于链表结构

//行链表,去掉多余的空格和回车,换行后.
typedef struct _LINE {
    LINE_TYPE type;//类型
    char  Text[MAX_INI_FILE_LINE_LEN];//内容
    int LineLen;
    char *pKeyStart;
    char *pKeyEnd;
    char *pValueStart;
    char *pValueEnd;
    struct _LINE *pNext;
} LINE;
//段链表
typedef struct _SECTION {
    LINE *pLine;
    struct _SECTION *pNext;
} SECTION;


class CIniFile {
public:
    //const int MAX_SECTION_NAME_LEN = 16;
    //const int MAX_KEY_NAME_LEN = 64;
    //const int MAX_VALUE_LEN = 512;

    CIniFile();
    ~CIniFile();
    int LoadFromFile(const char *filename);

    int SaveToFile(const char *filename = NULL);

    int SetString(const char *section, const char *key, const char *value);
    int SetInt(const char *section, const char *key, int value);

    const char *GetString(const char *section, const char *key, const char *def_value);
    int GetInt(const char *section, const char *key, int def_value);
    float GetFloat(const char *section, const char *key, float def_value);
    int SetFloat(const char *section, const char *key, float value);

private:
    LINE_TYPE getLineType(char *Str);
    //去掉串里面的,空格,回车,换行,s指向转换处理后的串的开头
    void allTrim(char *Str);

    SECTION *getSection(const char *section);
    LINE *getKeyLineAtSec(SECTION *pSec, const char *key);

    void FreeAllMem();
    int InsertSection(SECTION *pSec);
    int InsertKeyLine(SECTION *pSec, LINE *line);
    char mpFileName[256];
    FILE *m_pIniFile;
    LINE *mpFirstLine;
    SECTION *mpFirstSection;
};
#endif //end of INI_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "CIniFile.h"

CIniFile::CIniFile()
{
    mpFirstSection = NULL;
    mpFileName[0] = '\0';
    m_pIniFile = NULL;
    mpFirstLine = NULL;
}

CIniFile::~CIniFile()
{
    LOGD("CIniFile::~CIniFile()");
    FreeAllMem();
}

int CIniFile::LoadFromFile(const char *filename)
{
    char   lineStr[MAX_INI_FILE_LINE_LEN];
    char   *pStr;
    LINE *pCurLINE = NULL;
    SECTION *pCurSection = NULL;

    FreeAllMem();

    int Len;
    if (filename == NULL) {
        return -1;
    }

    strcpy(mpFileName, filename);
    LOGD("LoadFromFile 2name = %s", mpFileName);
    if ((m_pIniFile = fopen (mpFileName, "r")) == NULL) {
        return -1;
    }

    while (fgets (lineStr, MAX_INI_FILE_LINE_LEN, m_pIniFile) != NULL) {
        //去掉多余字符
        allTrim(lineStr);

        LINE *pLINE = new LINE();
        pLINE->pKeyStart = pLINE->Text;
        pLINE->pKeyEnd = pLINE->Text;
        pLINE->pValueStart = pLINE->Text;
        pLINE->pValueEnd = pLINE->Text;
        pLINE->pNext = NULL;
        pLINE->type = getLineType(lineStr);
        //LOGD("getline=%s len=%d type=%d", lineStr, strlen(lineStr), pLINE->type);
        strcpy(pLINE->Text, lineStr);
        pLINE->LineLen = strlen(pLINE->Text);

        //head
        if (mpFirstLine == NULL) {
            mpFirstLine = pLINE;
        } else {
            pCurLINE->pNext = pLINE;
        }

        pCurLINE = pLINE;

        switch (pCurLINE->type) {
        case LINE_TYPE_SECTION: {
            SECTION *pSec = new SECTION();
            pSec->pLine = pLINE;
            pSec->pNext = NULL;
            if (mpFirstSection == NULL) { //first section
                mpFirstSection = pSec;
            } else {
                pCurSection->pNext = pSec;
            }
            pCurSection = pSec;
            break;
        }
        case LINE_TYPE_KEY: {
            char *pM = strchr(pCurLINE->Text, '=');
            pCurLINE->pKeyStart = pCurLINE->Text;
            pCurLINE->pKeyEnd = pM - 1;
            pCurLINE->pValueStart = pM + 1;
            pCurLINE->pValueEnd = pCurLINE->Text + pCurLINE->LineLen - 1;
            break;
        }
        case LINE_TYPE_COMMENT: {
            break;
        }
        default: {
            break;
        }
        }
    }

    fclose (m_pIniFile);
    m_pIniFile = NULL;

    return 0;
}

void CIniFile::printAll()
{
    //line
    for (LINE *pline = mpFirstLine; pline != NULL; pline = pline->pNext) {
        LOGD("line = %s type = %d", pline->Text, pline->type);
    }

    //section
    for (SECTION *psec = mpFirstSection; psec != NULL; psec = psec->pNext) {
        LOGD("sec = %s", psec->pLine->Text);
    }
    return;
}

int CIniFile::LoadFromString(const char *str __unused)
{
    return 0;
}

int CIniFile::SaveToFile(const char *filename)
{
    const char *file = NULL;
    if (m_pIniFile != NULL) {
        fclose (m_pIniFile);
    }

    if (filename == NULL) {
        if (strlen(mpFileName) == 0) {
            LOGD("error save file is null");
            return -1;
        } else {
            file = mpFileName;
        }
    } else {
        file = filename;
    }
    //LOGD("Save to file name = %s", file);

    if ((m_pIniFile = fopen (file, "wb")) == NULL) {
        LOGD("Save to file open error = %s", file);
        return -1;
    }

    LINE *pCurLine = NULL;
    for (pCurLine = mpFirstLine; pCurLine != NULL; pCurLine = pCurLine->pNext) {
        fprintf (m_pIniFile, "%s\r\n", pCurLine->Text);
    }

    fflush(m_pIniFile);
    fsync(fileno(m_pIniFile));

    fclose(m_pIniFile);
    m_pIniFile = NULL;
    return 0;
}

//暂不插入操作
int CIniFile::SetString(const char *section, const char *key, const char *value)
{
    SECTION *pNewSec = NULL;
    LINE *pNewSecLine = NULL;
    LINE *pNewKeyLine = NULL;

    SECTION *pSec = getSection(section);
    if (pSec == NULL) {
        pNewSec = new SECTION();
        pNewSecLine = new LINE();
        pNewKeyLine = new LINE();

        pNewKeyLine->type = LINE_TYPE_KEY;
        pNewSecLine->type = LINE_TYPE_SECTION;


        sprintf(pNewSecLine->Text, "[%s]", section);
        pNewSec->pLine = pNewSecLine;

        InsertSection(pNewSec);

        int keylen = strlen(key);
        sprintf(pNewKeyLine->Text, "%s=%s", key, value);
        pNewKeyLine->LineLen = strlen(pNewKeyLine->Text);
        pNewKeyLine->pKeyStart = pNewKeyLine->Text;
        pNewKeyLine->pKeyEnd = pNewKeyLine->pKeyStart + keylen - 1;
        pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen + 1;
        pNewKeyLine->pValueEnd = pNewKeyLine->Text + pNewKeyLine->LineLen - 1;

        InsertKeyLine(pNewSec, pNewKeyLine);

    } else { //find section
        LINE *pLine = getKeyLineAtSec(pSec, key);
        if (pLine == NULL) { //, not find key
            pNewKeyLine = new LINE();
            pNewKeyLine->type = LINE_TYPE_KEY;

            int keylen = strlen(key);
            sprintf(pNewKeyLine->Text, "%s=%s", key, value);
            pNewKeyLine->LineLen = strlen(pNewKeyLine->Text);
            pNewKeyLine->pKeyStart = pNewKeyLine->Text;
            pNewKeyLine->pKeyEnd = pNewKeyLine->pKeyStart + keylen - 1;
            pNewKeyLine->pValueStart = pNewKeyLine->pKeyStart + keylen + 1;
            pNewKeyLine->pValueEnd = pNewKeyLine->Text + pNewKeyLine->LineLen - 1;

            InsertKeyLine(pSec, pNewKeyLine);
        } else { //all find, change it
            sprintf(pLine->Text, "%s=%s", key, value);
            pLine->LineLen = strlen(pLine->Text);
            pLine->pValueEnd = pLine->Text + pLine->LineLen - 1;
        }
    }

    //save
    SaveToFile(NULL);
    return 0;
}
int CIniFile::SetInt(const char *section, const char *key, int value)
{
    char tmp[64];
    sprintf(tmp, "%d", value);
    SetString(section, key, tmp);
    return 0;
}
const char *CIniFile::GetString(const char *section, const char *key, const char *def_value)
{
    SECTION *pSec = getSection(section);
    if (pSec == NULL) return def_value;
    LINE *pLine = getKeyLineAtSec(pSec, key);
    if (pLine == NULL) return def_value;

    return pLine->pValueStart;
}
int CIniFile::GetInt(const char *section, const char *key, int def_value)
{
    const char *num = GetString(section, key, NULL);
    if (num != NULL) {
        return atoi(num);
    }
    return def_value;
}


LINE_TYPE CIniFile::getLineType(char *Str)
{
    LINE_TYPE type = LINE_TYPE_COMMENT;
    //只要有#,就是注释
    if (strchr(Str, '#')  != NULL) {
        type = LINE_TYPE_COMMENT;
    } else if ( (strstr (Str, "[") != NULL) && (strstr (Str, "]") != NULL) ) { /* Is Section */
        type = LINE_TYPE_SECTION;
    } else {
        if (strstr (Str, "=") != NULL) {
            type = LINE_TYPE_KEY;
        } else {
            type = LINE_TYPE_COMMENT;
        }
    }
    return type;
}

void CIniFile::FreeAllMem()
{
    //line
    LINE *pCurLine = NULL;
    LINE *pNextLine = NULL;
    for (pCurLine = mpFirstLine; pCurLine != NULL;) {
        pNextLine = pCurLine->pNext;
        delete pCurLine;
        pCurLine = pNextLine;
    }
    mpFirstLine = NULL;
    //section
    SECTION *pCurSec = NULL;
    SECTION *pNextSec = NULL;
    for (pCurSec = mpFirstSection; pCurSec != NULL;) {
        pNextSec = pCurSec->pNext;
        delete pCurSec;
        pCurSec = pNextSec;
    }
    mpFirstSection = NULL;
}

int CIniFile::InsertSection(SECTION *pSec)
{
    //insert it to sections list ,as first section
    pSec->pNext = mpFirstSection;
    mpFirstSection = pSec;
    //insert it to lines list, at first
    pSec->pLine->pNext = mpFirstLine;
    mpFirstLine = pSec->pLine;
    return 0;
}
int CIniFile::InsertKeyLine(SECTION *pSec, LINE *line)
{
    LINE *line1 = pSec->pLine;
    LINE *line2 = line1->pNext;
    line1->pNext = line;
    line->pNext = line2;
    return 0;
}
SECTION *CIniFile::getSection(const char *section)
{
    //section
    for (SECTION *psec = mpFirstSection; psec != NULL; psec = psec->pNext) {
        if (strncmp((psec->pLine->Text) + 1, section, strlen(section)) == 0)
            return psec;
    }
    return NULL;
}
LINE *CIniFile::getKeyLineAtSec(SECTION *pSec, const char *key)
{
    //line
    for (LINE *pline = pSec->pLine->pNext; (pline != NULL && pline->type != LINE_TYPE_SECTION); pline = pline->pNext) {
        if (pline->type == LINE_TYPE_KEY) {
            if (strncmp(pline->Text, key, strlen(key)) == 0)
                return pline;
        }
    }
    return NULL;
}
//去掉串里面的,空格,回车,换行,s指向转换处理后的串的开头
void CIniFile::allTrim(char *Str)
{
    //去掉换行
    char *pStr;
    pStr = strchr (Str, '\n');
    if (pStr != NULL) {
        *pStr = 0;
    }
    //去掉尾部回车
    int Len = strlen(Str);
    if ( Len > 0 ) {
        if ( Str[Len - 1] == '\r' ) {
            Str[Len - 1] = '\0';
        }
    }
    //去掉空格
    pStr = Str;
    while (*pStr != '\0') { //没到尾部
        if (*pStr == ' ') { //遇到空格
            char *pTmp = pStr;//从空格处开始
            while (*pTmp != '\0') {
                *pTmp = *(pTmp + 1);//前移,包括移最后结束符
                pTmp++;
            }
        } else {
            pStr++;
        }
    }
    return;
}



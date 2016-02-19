//
//
//  amlogic 2015
//
//  @ Project : tv
//  @ File Name : CSqlite
//  @ Date : 2015-5
//  @ Author :
//
//
#define LOG_TAG "CSqlite"

#include "CSqlite.h"

using namespace android;

CSqlite::CSqlite()
{
    mHandle = NULL;
}

CSqlite::~CSqlite()
{
    if (mHandle != NULL) {
        sqlite3_close(mHandle);
        mHandle = NULL;
    }
}

//完整性检测，检测数据库是否被破坏
bool CSqlite::integrityCheck()
{
    char *err;
    int rval = sqlite3_exec(mHandle, "PRAGMA integrity_check;", sqlite3_exec_callback, NULL, &err);
    if (rval != SQLITE_OK) {
        LOGD(" val = %d  msg = %s!\n", rval, sqlite3_errmsg(mHandle));
        return false;
    } else {
        return true;
    }
    return true;
}

int CSqlite::sqlite3_exec_callback(void *data __unused, int nColumn, char **colValues, char **colNames __unused)
{
    LOGD("sqlite3_exec_callback--- nums = %d", nColumn);
    for (int i = 0; i < nColumn; i++) {
        LOGD("%s\t", colValues[i]);
    }
    LOGD("\n");

    return 0;
}

int CSqlite::openDb(const char *path)
{
    if (sqlite3_open(path, &mHandle) != SQLITE_OK) {
        LOGD("open db(%s) error", path);
        mHandle = NULL;
        return -1;
    }
    return 0;
}

int CSqlite::closeDb()
{
    int rval = 0;
    if (mHandle != NULL) {
        rval = sqlite3_close(mHandle);
        mHandle = NULL;
    }
    return rval;
}

void CSqlite::setHandle(sqlite3 *h)
{
    mHandle = h;
}

sqlite3 *CSqlite::getHandle()
{
    return mHandle;
}

int CSqlite::select(const char *sql, CSqlite::Cursor &c)
{
    int col, row;
    char **pResult = NULL;
    char *errmsg;
    assert(mHandle && sql);

    if (strncmp(sql, "select", 6))
        return -1;
    //LOGD("sql=%s", sql);
    if (sqlite3_get_table(mHandle, sql, &pResult, &row, &col, &errmsg) != SQLITE_OK) {
        LOGD("errmsg=%s", errmsg);
        if (pResult != NULL)
            sqlite3_free_table(pResult);
        return -1;
    }

    //LOGD("row=%d, col=%d", row, col);
    c.Init(pResult, row, col);
    return 0;
}

void CSqlite::insert()
{
}

bool CSqlite::exeSql(const char *sql)
{
    char *errmsg;
    if (sql == NULL) return false;
    if (sqlite3_exec(mHandle, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        LOGD("exeSql=: %s error=%s", sql, errmsg ? errmsg : "Unknown");
        if (errmsg)
            sqlite3_free(errmsg);
        return false;
    }
    LOGD("sql=%s", sql);
    return true;
}

bool CSqlite::beginTransaction()
{
    return exeSql("begin;");
}

bool CSqlite::commitTransaction()
{
    return exeSql("commit;");
}

bool CSqlite::rollbackTransaction()
{
    return exeSql("rollback;");
}

void CSqlite::del()
{
}

void CSqlite::update()
{
}

void CSqlite::xxtable()
{
}


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
#if !defined(_CSQLITE_H_)
#define _CSQLITE_H_
#include <unistd.h>
#include <stdlib.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/RefBase.h>
#include "CTvLog.h"
#include <sqlite3.h>
using namespace android;
class CSqlite {
public:
    class Cursor {
    public:
        void Init(char **data, int cow, int col)
        {
            mData = data;
            mCurRowIndex = 0;
            mRowNums = cow;
            mColNums = col;
            mIsClosed = false;
        }
        Cursor()
        {
            mData = NULL;
            mCurRowIndex = 0;
            mRowNums = 0;
            mColNums = 0;
            mIsClosed = false;
        }

        /*Cursor(Cursor& c)
        {
            data = c.data;
            mCurRowIndex = 0;
            mRowNums = c.mRowNums;
            mColNums = c.mColNums;
            mIsClosed = false;
        }

        Cursor& operator = (const Cursor& c)
        {
            data = c.data;
            mCurRowIndex = 0;
            mRowNums = c.mRowNums;
            mColNums = c.mColNums;
            mIsClosed = false;
            return *this;
        }*/
        ~Cursor()
        {
            close();
        }
        //Row nums
        int getCount()
        {
            return mRowNums;
        }

        int getPosition();

        bool move(int offset);

        bool moveToPosition(int position);

        bool moveToFirst()
        {
            //LOGD("moveToFirst mRowNums = %d", mRowNums);
            if (mRowNums <= 0) return false;
            mCurRowIndex = 0;
            return true;
        }

        bool moveToLast();

        bool moveToNext()
        {
            if (mCurRowIndex >= mRowNums - 1)return false;
            mCurRowIndex++;
            return true;
        }

        bool moveToPrevious();

        int getColumnIndex(const char *columnName)
        {
            int index = 0;
            for (int i = 0; i < mColNums; i++) {
                if (strcmp(columnName, mData[i]) == 0)
                    return index;
                index++;
            }

            return -1;
        }

        //String getColumnName(int columnIndex);
        //String[] getColumnNames();
        int getColumnCount();
        //字符串长度问题,弃用
        int getString(char *str, int columnIndex)
        {
            if (columnIndex >= mColNums || str == NULL) return -1;
            strcpy(str, mData[mColNums * (mCurRowIndex + 1) + columnIndex]);
            return 0;
        }
        //不限长度,依赖于String8
        String8 getString(int columnIndex)
        {
            if (columnIndex >= mColNums) return String8("");
            return String8(mData[mColNums * (mCurRowIndex + 1) + columnIndex]);
        }

        int getInt(int columnIndex)
        {
            //if(columnIndex >= mColNums || str == NULL) return -1;
            return atoi(mData[mColNums * (mCurRowIndex + 1) + columnIndex]);
        }
        unsigned long int getUInt(int columnIndex)
        {
            return strtoul(mData[mColNums * (mCurRowIndex + 1) + columnIndex], NULL, 10);
        }
        double getF(int columnIndex)
        {
            return atof(mData[mColNums * (mCurRowIndex + 1) + columnIndex]);
        }
        int getType(int columnIndex);
        void close()
        {
            if (mData != NULL)
                sqlite3_free_table(mData);

            mData = NULL;
            mCurRowIndex = 0;
            mRowNums = 0;
            mIsClosed = true;
        }
        bool isClosed()
        {
            return mIsClosed;
        }
    private:
        char **mData;
        int mCurRowIndex;
        int mRowNums;
        int mColNums;
        bool mIsClosed;
    };
public:
    CSqlite();
    virtual ~CSqlite();
    int openDb(const char *path);
    int closeDb();
    void setHandle(sqlite3 *h);
    sqlite3 *getHandle();
    bool integrityCheck();
    int select(const char *sql, Cursor &);
    bool exeSql(const char *sql);
    void insert();
    void del();
    void update();
    void xxtable();
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    void dbsync()
    {
        sync();
    };
private:
    static int  sqlite3_exec_callback(void *data, int nColumn, char **colValues, char **colNames);
    sqlite3 *mHandle;
};
#endif //CSQLITE

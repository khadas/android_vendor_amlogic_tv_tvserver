/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
*/

#ifndef _C_TV_TIME_H_
#define _C_TV_TIME_H_

#include <utils/Timers.h>
#include <time.h>
#include <sys/times.h>
#include <sys/sysinfo.h>

/**
 *TV时间管理
 */
class CTvTime {
public:
    CTvTime() {
        mDiff = 0;
        mHZ = sysconf( _SC_CLK_TCK );
    }

    long getSysUTCTime();
    void setTime(long t);
    long getTime();

    /**
     *取得TDT/STT与系统时间的差值
     *@return 返回差值时间
     */
    long getDiffTime() {
        return mDiff;
    }

    /**
     *设置TDT/STT与系统时间的差值
     */
    void setDiffTime(long diff) {
        mDiff = diff;
    }

private:
    long mDiff;
    unsigned long mHZ;
};
#endif/*_C_TV_TIME_H_*/


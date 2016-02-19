#define LOG_TAG "CUpgradeFBC"

#include <CTvLog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>

#include "CUpgradeFBC.h"

CUpgradeFBC::CUpgradeFBC()
{
    mUpgradeMode = CC_UPGRADE_MODE_MAIN;
    mFileName[0] = 0;

    mOPTotalSize = 0;
    mBinFileSize = 0;
    mBinFileBuf = NULL;
    mUpgradeBlockSize = 0x10000;

    mpObserver = NULL;
    mState = STATE_STOPED;
    mCfbcIns = GetSingletonFBC();
    mCfbcIns->SetUpgradeFlag(0);
}

CUpgradeFBC::~CUpgradeFBC()
{
    if (mBinFileBuf != NULL) {
        delete mBinFileBuf;
        mBinFileBuf = NULL;
    }
}

int CUpgradeFBC::start()
{
    if (mState == STATE_STOPED || mState == STATE_ABORT || mState == STATE_FINISHED) {
        mCfbcIns->SetUpgradeFlag(1);

        this->run();
    }

    return 0;
}

int CUpgradeFBC::stop()
{
    requestExit();
    mState = STATE_STOPED;

    return 0;
}

int CUpgradeFBC::GetUpgradeFBCProgress()
{
    return 0;
}

int CUpgradeFBC::SetUpgradeFileName(char *file_name)
{
    if (file_name == NULL) {
        return -1;
    }

    strcpy(mFileName, file_name);

    return 0;
}

int CUpgradeFBC::SetUpgradeFileSize(int file_size)
{
    mBinFileSize = file_size;
    return 0;
}

int CUpgradeFBC::SetUpgradeBlockSize(int block_size)
{
    mUpgradeBlockSize = block_size;
    return 0;
}

int CUpgradeFBC::SetUpgradeMode(int mode)
{
    int tmp_val = 0;

    tmp_val = mUpgradeMode;
    mUpgradeMode = mode;

    return tmp_val;
}

int CUpgradeFBC::AddCRCToDataBuf(unsigned char data_buf[], int data_len)
{
    unsigned int tmp_crc = 0;

    tmp_crc = mCfbcIns->Calcrc32(0, data_buf, data_len);
    data_buf[data_len + 0] = (tmp_crc >> 0) & 0xFF;
    data_buf[data_len + 1] = (tmp_crc >> 8) & 0xFF;
    data_buf[data_len + 2] = (tmp_crc >> 16) & 0xFF;
    data_buf[data_len + 3] = (tmp_crc >> 24) & 0xFF;

    return 0;
}

bool CUpgradeFBC::threadLoop()
{
    int file_handle = -1;
    int i = 0, tmp_flag = 0, cmd_len = 0, tmp_prog = 0, total_item = 0;
    int start_off = 0, end_off = 0, cur_off = 0, old_off = 0, rw_size = 0;
    int upgrade_version = 0, upgrade_flag = 0, upgrade_err_code = 0, upgrade_try_cnt = 0;
    int upgrade_pq_wb_flag = 0;
    unsigned char tmp_buf[128] = {0};

    if (mpObserver == NULL) {
        return false;
    }

    LOGD("%s, entering...\n", "TV");

    prctl(PR_SET_NAME, (unsigned long)"CUpgradeFBC thread loop");

    mState = STATE_RUNNING;

    LOGD("%s, upgrade mode = %d\n", __FUNCTION__, mUpgradeMode);
    if (mUpgradeMode != CC_UPGRADE_MODE_BOOT_MAIN && mUpgradeMode != CC_UPGRADE_MODE_BOOT &&
            mUpgradeMode != CC_UPGRADE_MODE_MAIN && mUpgradeMode != CC_UPGRADE_MODE_COMPACT_BOOT &&
            mUpgradeMode != CC_UPGRADE_MODE_ALL && mUpgradeMode != CC_UPGRADE_MODE_MAIN_PQ_WB &&
            mUpgradeMode != CC_UPGRADE_MODE_ALL_PQ_WB && mUpgradeMode != CC_UPGRADE_MODE_MAIN_WB &&
            mUpgradeMode != CC_UPGRADE_MODE_ALL_WB && mUpgradeMode != CC_UPGRADE_MODE_MAIN_PQ &&
            mUpgradeMode != CC_UPGRADE_MODE_ALL_PQ && mUpgradeMode != CC_UPGRADE_MODE_PQ_WB_ONLY &&
            mUpgradeMode != CC_UPGRADE_MODE_WB_ONLY && mUpgradeMode != CC_UPGRADE_MODE_PQ_ONLY &&
            mUpgradeMode != CC_UPGRADE_MODE_CUR_PQ_BIN && mUpgradeMode != CC_UPGRADE_MODE_BURN &&
            mUpgradeMode != CC_UPGRADE_MODE_DUMMY) {
        mState = STATE_ABORT;
        upgrade_err_code = ERR_NOT_SUPPORT_UPGRADE_MDOE;
        mpObserver->onUpgradeStatus(mState, upgrade_err_code);
        mCfbcIns->SetUpgradeFlag(0);

        return false;
    }

    if (mUpgradeBlockSize % 0x1000 != 0) {
        mState = STATE_ABORT;
        upgrade_err_code = ERR_NOT_CORRECT_UPGRADE_BLKSIZE;
        mpObserver->onUpgradeStatus(mState, upgrade_err_code);
        mCfbcIns->SetUpgradeFlag(0);

        return false;
    }

    struct stat tmp_st;
    stat(mFileName, &tmp_st);
    if (tmp_st.st_size == CC_FBC_V01_FILE_SIZE) {
        upgrade_version = CC_FBC_V01_00_VAL;
        mOPTotalSize = CC_UPGRADE_V01_ALL_LENGTH;
        mBinFileSize = CC_FBC_V01_FILE_SIZE;
    } else if (tmp_st.st_size == CC_FBC_V02_FILE_SIZE) {
        upgrade_version = CC_FBC_V02_00_VAL;
        mOPTotalSize = CC_UPGRADE_V02_ALL_LENGTH;
        mBinFileSize = CC_FBC_V02_FILE_SIZE;
    } else if (tmp_st.st_size == CC_FBC_V02_CUR_PQ_BIN_FILE_SIZE) {
        upgrade_version = CC_FBC_V02_01_VAL;
        mOPTotalSize = CC_UPGRADE_V02_ALL_LENGTH;
        mBinFileSize = CC_FBC_V02_CUR_PQ_BIN_FILE_SIZE;
    } else if (tmp_st.st_size == CC_FBC_V03_FILE_SIZE) {
        upgrade_version = CC_FBC_V03_00_VAL;
        mOPTotalSize = CC_UPGRADE_V03_ALL_LENGTH;
        mBinFileSize = CC_FBC_V03_FILE_SIZE;
    } else if (tmp_st.st_size == CC_FBC_V03_CUR_PQ_BIN_FILE_SIZE) {
        upgrade_version = CC_FBC_V03_01_VAL;
        mOPTotalSize = CC_UPGRADE_V03_ALL_LENGTH;
        mBinFileSize = CC_FBC_V03_CUR_PQ_BIN_FILE_SIZE;
    } else {
        upgrade_version = 0;
        mOPTotalSize = 0;
        mBinFileSize = 0;
        mState = STATE_ABORT;
        upgrade_err_code = ERR_BIN_FILE_SIZE;
        mpObserver->onUpgradeStatus(mState, upgrade_err_code);
        mCfbcIns->SetUpgradeFlag(0);
        return false;
    }

    //open upgrade source file and read it to temp buffer.
    file_handle = open(mFileName, O_RDONLY);
    if (file_handle < 0) {
        LOGE("%s, Can't Open file %s\n", __FUNCTION__, mFileName);
        mState = STATE_ABORT;
        upgrade_err_code = ERR_OPEN_BIN_FILE;
        mpObserver->onUpgradeStatus(mState, upgrade_err_code);
        mCfbcIns->SetUpgradeFlag(0);
        return false;
    }

    lseek(file_handle, 0, SEEK_SET);

    mBinFileBuf = new unsigned char[mOPTotalSize];

    memset(mBinFileBuf, 0, mOPTotalSize);
    rw_size = read(file_handle, mBinFileBuf, mBinFileSize);
    if (rw_size != mBinFileSize || rw_size <= 0) {
        LOGE("%s, read file %s error(%d, %d)\n", __FUNCTION__, mFileName, mBinFileSize, rw_size);
        mState = STATE_ABORT;
        upgrade_err_code = ERR_READ_BIN_FILE;
        mpObserver->onUpgradeStatus(mState, upgrade_err_code);
        mCfbcIns->SetUpgradeFlag(0);

        if (mBinFileBuf != NULL) {
            delete mBinFileBuf;
            mBinFileBuf = NULL;
        }
        return false;
    }

    close(file_handle);
    file_handle = -1;

    if (upgrade_version == CC_FBC_V02_00_VAL) {
        memcpy((void *)(mBinFileBuf + CC_UPGRADE_V02_BOOT_BAK_OFFSET), (void *)(mBinFileBuf + CC_UPGRADE_V02_BOOT_OFFSET), CC_UPGRADE_V02_BOOT_LEN);
        memcpy((void *)(mBinFileBuf + CC_UPGRADE_V02_MAIN_BAK_OFFSET), (void *)(mBinFileBuf + CC_UPGRADE_V02_MAIN_OFFSET), CC_UPGRADE_V02_MAIN_LEN);
    } else if (upgrade_version == CC_FBC_V02_01_VAL) {
        memcpy((void *)(mBinFileBuf + CC_UPGRADE_V02_CUR_PQ_OFFSET), (void *)(mBinFileBuf + 0), CC_FBC_V02_CUR_PQ_BIN_FILE_SIZE);
    } else if (upgrade_version == CC_FBC_V03_01_VAL) {
        memcpy((void *)(mBinFileBuf + CC_UPGRADE_V03_CUR_PQ_OFFSET), (void *)(mBinFileBuf + 0), CC_FBC_V03_CUR_PQ_BIN_FILE_SIZE);
    }

    //calculate start addr
    if (upgrade_version == CC_FBC_V01_00_VAL) {
        start_off = CC_UPGRADE_V01_MAIN_OFFSET;
        end_off = 0;
        if (mUpgradeMode == CC_UPGRADE_MODE_ALL || mUpgradeMode == CC_UPGRADE_MODE_BOOT_MAIN) {
            start_off = CC_UPGRADE_V01_BOOT_OFFSET;
            end_off = CC_UPGRADE_V01_BOOT_OFFSET + CC_UPGRADE_V01_ALL_LENGTH;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_BOOT) {
            start_off = CC_UPGRADE_V01_BOOT_OFFSET;
            end_off = CC_UPGRADE_V01_BOOT_OFFSET + CC_UPGRADE_V01_BOOT_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_MAIN) {
            start_off = CC_UPGRADE_V01_MAIN_OFFSET;
            end_off = CC_UPGRADE_V01_MAIN_OFFSET + CC_UPGRADE_V01_MAIN_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        }
    } else if (upgrade_version == CC_FBC_V02_00_VAL) {
        start_off = CC_UPGRADE_V02_MAIN_OFFSET;
        end_off = 0;
        if (mUpgradeMode == CC_UPGRADE_MODE_ALL || mUpgradeMode == CC_UPGRADE_MODE_ALL_PQ_WB ||
                mUpgradeMode == CC_UPGRADE_MODE_ALL_WB || mUpgradeMode == CC_UPGRADE_MODE_ALL_PQ) {
            start_off = CC_UPGRADE_V02_COMPACT_BOOT_OFFSET;
            end_off = CC_UPGRADE_V02_COMPACT_BOOT_OFFSET + CC_UPGRADE_V02_ALL_LENGTH;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_BOOT) {
            start_off = CC_UPGRADE_V02_BOOT_OFFSET;
            end_off = CC_UPGRADE_V02_BOOT_OFFSET + CC_UPGRADE_V02_BOOT_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_MAIN || mUpgradeMode == CC_UPGRADE_MODE_MAIN_PQ_WB ||
                   mUpgradeMode == CC_UPGRADE_MODE_MAIN_WB || mUpgradeMode == CC_UPGRADE_MODE_MAIN_PQ ) {
            start_off = CC_UPGRADE_V02_MAIN_OFFSET;
            end_off = CC_UPGRADE_V02_MAIN_OFFSET + CC_UPGRADE_V02_MAIN_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_COMPACT_BOOT) {
            start_off = CC_UPGRADE_V02_COMPACT_BOOT_OFFSET;
            end_off = CC_UPGRADE_V02_COMPACT_BOOT_OFFSET + CC_UPGRADE_V02_COMPACT_BOOT_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_BOOT_MAIN) {
            start_off = CC_UPGRADE_V02_BOOT_OFFSET;
            end_off = CC_UPGRADE_V02_BOOT_OFFSET + CC_UPGRADE_V02_BOOT_LEN + CC_UPGRADE_V02_MAIN_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        }
    } else if (upgrade_version == CC_FBC_V02_01_VAL) {
        start_off = CC_UPGRADE_V02_CUR_PQ_OFFSET;
        end_off = 0;
        if (mUpgradeMode == CC_UPGRADE_MODE_CUR_PQ_BIN) {
            start_off = CC_UPGRADE_V02_CUR_PQ_OFFSET;
            end_off = CC_UPGRADE_V02_CUR_PQ_OFFSET + CC_FBC_V02_CUR_PQ_BIN_FILE_SIZE;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        }
    } else if (upgrade_version == CC_FBC_V03_00_VAL) {
        start_off = CC_UPGRADE_V03_MAIN_OFFSET;
        end_off = 0;
        if (mUpgradeMode == CC_UPGRADE_MODE_ALL || mUpgradeMode == CC_UPGRADE_MODE_ALL_PQ_WB ||
                mUpgradeMode == CC_UPGRADE_MODE_ALL_WB || mUpgradeMode == CC_UPGRADE_MODE_ALL_PQ) {
            start_off = CC_UPGRADE_V03_COMPACT_BOOT_OFFSET;
            end_off = CC_UPGRADE_V03_COMPACT_BOOT_OFFSET + CC_UPGRADE_V03_ALL_LENGTH;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_BOOT) {
            start_off = CC_UPGRADE_V03_BOOT_OFFSET;
            end_off = CC_UPGRADE_V03_BOOT_OFFSET + CC_UPGRADE_V03_BOOT_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_MAIN || mUpgradeMode == CC_UPGRADE_MODE_MAIN_PQ_WB ||
                   mUpgradeMode == CC_UPGRADE_MODE_MAIN_WB || mUpgradeMode == CC_UPGRADE_MODE_MAIN_PQ ) {
            start_off = CC_UPGRADE_V03_MAIN_OFFSET;
            end_off = CC_UPGRADE_V03_MAIN_OFFSET + CC_UPGRADE_V03_MAIN_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_COMPACT_BOOT) {
            start_off = CC_UPGRADE_V03_COMPACT_BOOT_OFFSET;
            end_off = CC_UPGRADE_V03_COMPACT_BOOT_OFFSET + CC_UPGRADE_V03_COMPACT_BOOT_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_BOOT_MAIN) {
            start_off = CC_UPGRADE_V03_BOOT_OFFSET;
            end_off = CC_UPGRADE_V03_BOOT_OFFSET + CC_UPGRADE_V03_BOOT_LEN + CC_UPGRADE_V03_MAIN_LEN;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_BURN) {
            start_off = CC_UPGRADE_V03_COMPACT_BOOT_OFFSET;
            end_off = CC_UPGRADE_V03_COMPACT_BOOT_OFFSET + CC_FBC_V03_FILE_SIZE;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        }
    } else if (upgrade_version == CC_FBC_V03_01_VAL) {
        start_off = CC_UPGRADE_V03_CUR_PQ_OFFSET;
        end_off = 0;
        if (mUpgradeMode == CC_UPGRADE_MODE_CUR_PQ_BIN) {
            start_off = CC_UPGRADE_V03_CUR_PQ_OFFSET;
            end_off = CC_UPGRADE_V03_CUR_PQ_OFFSET + CC_FBC_V03_CUR_PQ_BIN_FILE_SIZE;
            total_item = (end_off - start_off) / mUpgradeBlockSize + 2;
        }
    }

    //let's try set default pq & wb
    if (upgrade_version == CC_FBC_V02_00_VAL || upgrade_version == CC_FBC_V03_00_VAL) {
        if (mUpgradeMode == CC_UPGRADE_MODE_ALL_PQ_WB || mUpgradeMode == CC_UPGRADE_MODE_MAIN_PQ_WB ||
                mUpgradeMode == CC_UPGRADE_MODE_PQ_WB_ONLY) {
            mDataBuf[6] = 3;
            upgrade_pq_wb_flag = 1;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_MAIN_WB || mUpgradeMode == CC_UPGRADE_MODE_ALL_WB ||
                   mUpgradeMode == CC_UPGRADE_MODE_WB_ONLY) {
            mDataBuf[6] = 2;
            upgrade_pq_wb_flag = 1;
        } else if (mUpgradeMode == CC_UPGRADE_MODE_MAIN_PQ || mUpgradeMode == CC_UPGRADE_MODE_ALL_PQ ||
                   mUpgradeMode == CC_UPGRADE_MODE_PQ_ONLY) {
            mDataBuf[6] = 1;
            upgrade_pq_wb_flag = 1;
        } else {
            upgrade_pq_wb_flag = 0;
        }

        if (upgrade_pq_wb_flag == 1) {
            cmd_len = 7;
            mDataBuf[0] = 0x5A;
            mDataBuf[1] = 0x5A;
            mDataBuf[2] = cmd_len + 4;
            mDataBuf[3] = 0x00;
            mDataBuf[4] = 0x00;
            mDataBuf[5] = CMD_CLR_SETTINGS_DEFAULT;

            AddCRCToDataBuf(mDataBuf, cmd_len);
            if (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, mDataBuf, cmd_len + 4, 0) <= 0) {
                mState = STATE_ABORT;
                upgrade_err_code = ERR_SERIAL_CONNECT;
                mpObserver->onUpgradeStatus(mState, upgrade_err_code);
                mCfbcIns->SetUpgradeFlag(0);

                if (mBinFileBuf != NULL) {
                    delete mBinFileBuf;
                    mBinFileBuf = NULL;
                }
                return false;
            }

            usleep(3000 * 1000);

            if (mUpgradeMode == CC_UPGRADE_MODE_PQ_WB_ONLY || mUpgradeMode == CC_UPGRADE_MODE_WB_ONLY ||
                    mUpgradeMode == CC_UPGRADE_MODE_PQ_ONLY) {
                system("reboot");
                return false;
            }
        }
    }

    //send upgrade command
    cmd_len = 10;
    mDataBuf[0] = 0x5A;
    mDataBuf[1] = 0x5A;
    mDataBuf[2] = cmd_len + 4;
    mDataBuf[3] = 0x00;
    mDataBuf[4] = 0x00;
    mDataBuf[5] = 0x01;
    mDataBuf[6] = 0x88;
    mDataBuf[7] = 0x88;
    mDataBuf[8] = 0x88;
    mDataBuf[9] = 0x88;
    AddCRCToDataBuf(mDataBuf, cmd_len);
    if (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, mDataBuf, cmd_len + 4, 0) <= 0) {
        mState = STATE_ABORT;
        upgrade_err_code = ERR_SERIAL_CONNECT;
        mpObserver->onUpgradeStatus(mState, upgrade_err_code);
        mCfbcIns->SetUpgradeFlag(0);

        if (mBinFileBuf != NULL) {
            delete mBinFileBuf;
            mBinFileBuf = NULL;
        }
        return false;
    }

    //waiting fbc restart
    usleep(5000 * 1000);

    if (mUpgradeMode == CC_UPGRADE_MODE_DUMMY) {
        //dummy test mode

        //wait 10 second
        usleep(10000 * 1000);

        //send reboot command to reboot fbc
        sprintf((char *)tmp_buf, "reboot\n");
        cmd_len = strlen((char *)tmp_buf);
        mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, tmp_buf, cmd_len, 0);
        usleep(400 * 1000);

        mpObserver->onUpgradeStatus(mState, 100);

        usleep(100 * 1000);
        mState = STATE_FINISHED;
        mpObserver->onUpgradeStatus(mState, 0);

        if (mBinFileBuf != NULL) {
            delete mBinFileBuf;
            mBinFileBuf = NULL;
        }
        return false;
    }

    tmp_prog += 1;
    mpObserver->onUpgradeStatus(mState, tmp_prog * 100 / total_item);

    cur_off = start_off;
    old_off = cur_off;

    upgrade_flag = 0;
    while (!exitPending()) { //requietexit() or requietexitWait() not call
        if (cur_off >= end_off) {
            upgrade_flag = 1;
            break;
        }

        //copy data from file temp buffer
        if (end_off - cur_off < mUpgradeBlockSize) {
            rw_size = end_off - cur_off;
        } else {
            rw_size = mUpgradeBlockSize;
        }

        memcpy(mDataBuf, mBinFileBuf + cur_off, rw_size);

        //send upgrade start addr and write size
        sprintf((char *)tmp_buf, "upgrade 0x%x 0x%x\n", cur_off, rw_size);
        LOGD("\n\n%s, %s\n", __FUNCTION__, tmp_buf);
        cmd_len = strlen((char *)tmp_buf);
        if (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, tmp_buf, cmd_len, 0) <= 0) {
            mState = STATE_ABORT;
            upgrade_err_code = ERR_SERIAL_CONNECT;
            upgrade_flag = 0;
            break;
        }
        usleep(500 * 1000);

        //send upgrade data
        if (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, mDataBuf, rw_size, 0) <= 0) {
            mState = STATE_ABORT;
            upgrade_err_code = ERR_SERIAL_CONNECT;
            upgrade_flag = 0;
            break;
        }

        //send upgrade data crc
        AddCRCToDataBuf(mDataBuf, rw_size);
        if (mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, mDataBuf + rw_size, 4, 0) <= 0) {
            mState = STATE_ABORT;
            upgrade_err_code = ERR_SERIAL_CONNECT;
            upgrade_flag = 0;
            break;
        }

        old_off = cur_off;
        cur_off += rw_size;

        //deal with fbc response
        tmp_flag = 0;
        memset(mDataBuf, 0, CC_UPGRADE_DATA_BUF_SIZE);
        rw_size = mCfbcIns->uartReadStream(mDataBuf, CC_UPGRADE_DATA_BUF_SIZE, 2000);
        for (i = 0; i < rw_size - 3; i++) {
            if ((0x5A == mDataBuf[i]) && (0x5A == mDataBuf[i + 1]) && (0x5A == mDataBuf[i + 2])) {
                LOGD("%s, fbc write data at 0x%x ok!\n", __FUNCTION__, old_off);
                tmp_flag = 1;
                break;
            }
        }

        if (tmp_flag == 0) {
            LOGE("%s, fbc write data at 0x%x error! rewrite!\n", __FUNCTION__, old_off);
            if (upgrade_try_cnt < 6) {
                cur_off = old_off;
                upgrade_try_cnt += 1;

                mpObserver->onUpgradeStatus(mState, ERR_DATA_CRC_ERROR);
            } else {
                LOGE("%s, we have rewrite more than %d times, abort.\n", __FUNCTION__, upgrade_try_cnt);
                mState = STATE_ABORT;
                upgrade_err_code = ERR_SERIAL_CONNECT;
                upgrade_flag = 0;
                break;
            }
        } else {
            tmp_prog += 1;
            upgrade_try_cnt = 0;
        }

        usleep(3000 * 1000);

        mpObserver->onUpgradeStatus(mState, tmp_prog * 100 / total_item);
    }

    if (mState == STATE_ABORT) {
        mpObserver->onUpgradeStatus(mState, upgrade_err_code);
    } else if (mState == STATE_RUNNING) {
        if (upgrade_flag == 1) {
            sprintf((char *)tmp_buf, "reboot\n");
            cmd_len = strlen((char *)tmp_buf);
            mCfbcIns->sendDataOneway(COMM_DEV_SERIAL, tmp_buf, cmd_len, 0);
            usleep(400 * 1000);

            tmp_prog += 1;
            mpObserver->onUpgradeStatus(mState, tmp_prog * 100 / total_item);

            usleep(100 * 1000);
            mState = STATE_FINISHED;
            mpObserver->onUpgradeStatus(mState, 0);
        }
    } else {
        if (upgrade_flag == 1) {
            tmp_prog += 1;
            mpObserver->onUpgradeStatus(mState, tmp_prog * 100 / total_item);
        }
    }

    mState = STATE_STOPED;

    mCfbcIns->SetUpgradeFlag(0);

    if (mBinFileBuf != NULL) {
        delete mBinFileBuf;
        mBinFileBuf = NULL;
    }

    LOGD("%s, exiting...\n", "TV");
    system("reboot");
    //return true, run again, return false,not run.
    return false;
}

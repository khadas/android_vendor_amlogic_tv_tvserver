#define LOG_TAG "FBC"

#include <time.h>
#include "CFbcCommunication.h"
#include  "CTvLog.h"
#include "../tvconfig/tvconfig.h"
#include "../tvutils/tvutils.h"

static CFbcCommunication *gSingletonFBC = NULL;
CFbcCommunication *GetSingletonFBC()
{
    if (gSingletonFBC == NULL) {
        gSingletonFBC = new CFbcCommunication();
        gSingletonFBC->start();
    }

    return gSingletonFBC;

}

CFbcCommunication::CFbcCommunication()
{
    initCrc32Table();

    m_event.data.fd = -1;
    m_event.events = EPOLLIN | EPOLLET;

    mpRevDataBuf = new unsigned char[512];

    mUpgradeFlag = 0;
    mbDownHaveSend = 0;//false
    //mFbcMsgQueue.startMsgQueue();
    mbFbcKeyEnterDown = 0;//false
    mFbcEnterKeyDownTime = -1;
}

CFbcCommunication::~CFbcCommunication()
{
    m_event.data.fd = mSerialPort.getFd();
    m_event.events = EPOLLIN | EPOLLET;
    mEpoll.del(mSerialPort.getFd(), &m_event);
    closeAll();
    delete[] mpRevDataBuf;
}

int CFbcCommunication::start()
{
#if 1
    //int serial_port = config_get_int("TV", "fbc.communication.serial", SERIAL_C);
    if (mSerialPort.OpenDevice(SERIAL_C) < 0) {
    } else {
#if 0
        LOGD("%s %d be called......\n", __FUNCTION__, __LINE__);
        mSerialPort.set_opt(115200, 8, 1, 'N', 5000, 1);
#else
        LOGD("%s %d be called......\n", __FUNCTION__, __LINE__);
        mSerialPort.setup_serial();
#endif
    }

    if (mEpoll.create() < 0) {
        return -1;
    }

    m_event.data.fd = mSerialPort.getFd();
    m_event.events = EPOLLIN | EPOLLET;
    mEpoll.add(mSerialPort.getFd(), &m_event);
#endif

#if 0
    mHdmiCec.openFile(CEC_PATH);
    m_event.data.fd = mHdmiCec.getFd();
    m_event.events = EPOLLIN | EPOLLET;
    mEpoll.add(mHdmiCec.getFd(), &m_event);
#endif

    //timeout for long
    mEpoll.setTimeout(3000);

    this->run();
    mTvInput.run();
    return 0;
}

void CFbcCommunication::testUart()
{
    unsigned char write_buf[64], read_buf[64];
    int idx = 0;
    memset(write_buf, 0, sizeof(write_buf));
    memset(read_buf, 0, sizeof(read_buf));

    write_buf[0] = 0x5a;
    write_buf[1] = 0x5a;
    write_buf[2] = 0xb;
    write_buf[3] = 0x0;
    write_buf[4] = 0x0;
    write_buf[5] = 0x40;
    write_buf[6] = 0x0;

    write_buf[7] = 0x2;
    write_buf[8] = 0x3c;
    write_buf[9] = 0x75;
    write_buf[10] = 0x30;
    //LOGD("to write ..........\n");
    mSerialPort.writeFile(write_buf, 11);
    sleep(1);
    //LOGD("to read........\n");
    mSerialPort.readFile(read_buf, 12);
    for (idx = 0; idx < 12; idx++)
        LOGD("the data is:0x%x\n", read_buf[idx]);
    LOGD("end....\n");
}

void CFbcCommunication::showTime(struct timeval *_time)
{
    struct timeval curTime;

    if (_time == NULL) {
        gettimeofday(&curTime, NULL);
    } else {
        curTime.tv_sec = _time->tv_sec;
        curTime.tv_usec = _time->tv_usec;
    }
    if (curTime.tv_usec > 100000) {
        LOGD("[%ld.%ld]", curTime.tv_sec, curTime.tv_usec);
    } else if (curTime.tv_usec > 10000) {
        LOGD("[%ld.0%ld]", curTime.tv_sec, curTime.tv_usec);
    } else if (curTime.tv_usec > 1000) {
        LOGD("[%ld.00%ld]", curTime.tv_sec, curTime.tv_usec);
    } else if (curTime.tv_usec > 100) {
        LOGD("[%ld.000%ld]", curTime.tv_sec, curTime.tv_usec);
    } else if (curTime.tv_usec > 10) {
        LOGD("[%ld.0000%ld]", curTime.tv_sec, curTime.tv_usec);
    } else if (curTime.tv_usec > 1) {
        LOGD("[%ld.00000%ld]", curTime.tv_sec, curTime.tv_usec);
    }
}
long CFbcCommunication::getTime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
void CFbcCommunication::initCrc32Table()
{
    //生成Crc32的查询表
    int i, j;
    unsigned int Crc;
    for (i = 0; i < 256; i++) {
        Crc = i;
        for (j = 0; j < 8; j++) {
            if (Crc & 1)
                Crc = (Crc >> 1) ^ 0xEDB88320;
            else
                Crc >>= 1;
        }
        mCrc32Table[i] = Crc;
    }
}

void CFbcCommunication::sendAckCmd(bool isOk)
{
    int crc32value = 0;
    unsigned char ackcmd[12];
    ackcmd[0] = 0x5A;
    ackcmd[1] = 0x5A;
    ackcmd[2] = 12;//little endian
    ackcmd[3] = 0x00;
    ackcmd[4] = 0x80;//ack flag
    ackcmd[5] = 0xff;//cmd id
    if (isOk) {
        ackcmd[6] = 0xfe;
        ackcmd[7] = 0x7f;
    } else {
        ackcmd[6] = 0x80;
        ackcmd[7] = 0x01;
    }
    //*((unsigned int*) (ackcmd + 8)) = GetCrc32(ackcmd, 8);
    crc32value = Calcrc32(0, ackcmd, 8);
    ackcmd[8] = (crc32value >> 0) & 0xFF;
    ackcmd[9] = (crc32value >> 8) & 0xFF;
    ackcmd[10] = (crc32value >> 16) & 0xFF;
    ackcmd[11] = (crc32value >> 24) & 0xFF;
    LOGD("to send ack and crc is:0x%x\n", crc32value);
    sendDataOneway(COMM_DEV_SERIAL, ackcmd, 12, 0x00000000);
}

/*  函数名：GetCrc32
*  函数原型：unsigned int GetCrc32(char* InStr,unsigned int len)
*      参数：InStr  ---指向需要计算CRC32值的字符? *          len  ---为InStr的长帿 *      返回值为计算出来的CRC32结果? */
unsigned int CFbcCommunication::GetCrc32(unsigned char *InStr, unsigned int len)
{
    //开始计算CRC32校验便
    unsigned int Crc = 0xffffffff;
    for (int i = 0; i < (int)len; i++) {
        Crc = (Crc >> 8) ^ mCrc32Table[(Crc & 0xFF) ^ InStr[i]];
    }

    Crc ^= 0xFFFFFFFF;
    return Crc;
}

unsigned int CFbcCommunication::Calcrc32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len)
{
    static const unsigned int s_crc32[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                                              0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
                                            };
    unsigned int crcu32 = crc;
    //if (buf_len < 0)
    //    return 0;
    if (!ptr) return 0;
    crcu32 = ~crcu32;
    while (buf_len--) {
        unsigned char b = *ptr++;
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }
    return ~crcu32;
}

int CFbcCommunication::sendDataOneway(int devno, unsigned char *pData, int dataLen, int flags __unused)
{
    int ret = -1;
    switch (devno) {
    case COMM_DEV_CEC: {
        ret = mHdmiCec.writeFile(pData, dataLen);
        break;
    }
    case COMM_DEV_SERIAL: {
        ret = mSerialPort.writeFile(pData, dataLen);
        break;
    }
    default:
        break;
    }
    return ret;
}

int CFbcCommunication::rmFromRequestList()
{
    return 0;
}

int CFbcCommunication::addToRequestList()
{
    return 0;
}

//timeout ms
int CFbcCommunication::sendDataAndWaitReply(int devno, int waitForDevno, int waitForCmd, unsigned char *pData, int dataLen, int timeout, unsigned char *pReData, int *reDataLen, int flags)
{
    int ret = sendDataOneway(devno, pData, dataLen, flags);
    if (ret < 0) return ret;

    mReplyList.WaitDevNo = waitForDevno;
    mReplyList.WaitCmd = waitForCmd;
    mReplyList.WaitTimeOut = timeout;
    mReplyList.reDataLen = 0;
    mReplyList.replyData = mpRevDataBuf;
    addToRequestList();
    LOGD("wait dev:%d, cmd:0x%x, timeout:%d", mReplyList.WaitDevNo, mReplyList.WaitCmd, mReplyList.WaitTimeOut);

    mLock.lock();
    ret = mReplyList.WaitReplyCondition.waitRelative(mLock, timeout);//wait reply
    LOGD("%s, %d, wait reply return = %d", __FUNCTION__, __LINE__, ret);
    mLock.unlock();

    //
    if (mReplyList.reDataLen > 0) { //data have come in
        *reDataLen = mReplyList.reDataLen;
        memcpy(pReData, mReplyList.replyData, mReplyList.reDataLen);
        mReplyList.reDataLen = 0;
        return *reDataLen;
    } else {
        //timeout,not to wait continue
        mReplyList.WaitDevNo = -1;
        mReplyList.WaitCmd = 0xff;
        return -1;//timeout but data not come.
    }
    return 0;
}

int CFbcCommunication::closeAll()
{
    mSerialPort.CloseDevice();
    return 0;
}

int CFbcCommunication::SetUpgradeFlag(int flag)
{
    mUpgradeFlag = flag;
    return 0;
}

/*
** cmd[0] cmd_type; cmd[1] cmd length; cmd[2] para1 cmd[3] para2 ...
*/
int CFbcCommunication::handleCmd(COMM_DEV_TYPE_E fromDev, int *pData, int *pRetValue)
{
    fbc_command_t cmd_type = VPU_CMD_NULL;
    int ret_value = 0;

    if ((fromDev != COMM_DEV_SERIAL && fromDev != COMM_DEV_CEC) || pData == NULL || pRetValue == NULL) {
        //LOGD("para error and returned!");
        return -1;
    }

    cmd_type = (fbc_command_t)pData[0];
    LOGD("the cmd type is:0x%02x\n", cmd_type);
    switch (cmd_type) {
    case VPU_CMD_RED_GAIN_DEF:
        cfbc_Set_Gain_Red(fromDev, pData[2]);
        break;

    /*
    //case value '192' not in enumerated type 'fbc_command_t
    case (VPU_CMD_RED_GAIN_DEF|0x80):
        cfbc_Get_Gain_Red(fromDev, &ret_value);
        pRetValue[0] = VPU_CMD_RED_GAIN_DEF | 0x80;
        pRetValue[1] = 3;
        pRetValue[2] = ret_value;
        break;*/

    case VPU_CMD_PATTEN_SEL:
        cfbc_Set_Test_Pattern(fromDev, pData[2]);
        break;

    case VPU_CMD_BACKLIGHT_DEF:
        cfbc_Set_Backlight(fromDev, pData[2]);
        break;

    /*
    //case value '170' not in enumerated type 'fbc_command_t
    case (VPU_CMD_PATTEN_SEL|0x80):
        cfbc_Get_Test_Pattern(fromDev, &ret_value);
        pRetValue[0] = VPU_CMD_RED_GAIN_DEF | 0x80;
        pRetValue[1] = 3;
        pRetValue[2] = ret_value;
        break;*/

    default:
        return -1;
    }

    return 0;
}

int CFbcCommunication::uartReadStream(unsigned char *retData, int rd_max_len, int timeout)
{
    int readLen = 0, bufIndex = 0, haveRead = 0;
    clock_t start_tm = clock();

    do {
        readLen = mSerialPort.readFile(retData + bufIndex, rd_max_len - haveRead);
        haveRead += readLen;
        bufIndex += readLen;
        if (haveRead == rd_max_len) {
            return haveRead;
        }

        LOGD("readLen = %d, haveRead = %d\n", readLen, haveRead);

        if (((clock() - start_tm) / (CLOCKS_PER_SEC / 1000)) > timeout) {
            return haveRead;
        }
    } while (true);

    return haveRead;
}

int CFbcCommunication::uartReadData(unsigned char *retData, int *retLen)
{
    unsigned char tempBuf[512];
    int cmdLen = 0;
    int bufIndex = 0;
    int readLen = 0;

    if (retData == NULL) {
        LOGD("the retData is NULL\n");
        return 0;
    }

    //leader codes 2 byte
    memset(tempBuf, 0, sizeof(tempBuf));
    do {
        readLen = mSerialPort.readFile(tempBuf  + 0, 1);
        if (tempBuf[0] == 0x5A) {
            bufIndex = 1;
            readLen = mSerialPort.readFile(tempBuf  + 1, 1);
            if (tempBuf[1] == 0x5A) {
                bufIndex = 2;
                LOGD("leading code coming...\n");
                break;
            } else {
                continue;
            }
        } else {
            continue;
        }
    } while (true);

    //data len 2 byte
    int needRead = 2, haveRead = 0;
    do {
        readLen = mSerialPort.readFile(tempBuf  + bufIndex,  needRead - haveRead);
        haveRead += readLen;
        bufIndex += readLen;
        if (haveRead == needRead) {
            break;
        }
    } while (true);

    //little endian
    cmdLen = (tempBuf[3] << 8) + tempBuf[2];
    //cmd data cmdLen - 2  -2
    needRead = cmdLen - 4, haveRead = 0;
    LOGD("cmdLen is:%d\n", cmdLen);

    do {
        readLen = mSerialPort.readFile(tempBuf  + bufIndex,  needRead - haveRead);
        haveRead += readLen;
        bufIndex += readLen;
        if (readLen > 0) {
            LOGD("data readLen is:%d\n", readLen);
        }
        if (haveRead == needRead) {
            break;
        }
    } while (true);

    unsigned int crc = 0;
    if (cmdLen > 4) {
        crc = Calcrc32(0, tempBuf, cmdLen - 4);//not include crc 4byte
    }
    unsigned int bufCrc = tempBuf[cmdLen - 4] |
                          tempBuf[cmdLen - 3] << 8 |
                          tempBuf[cmdLen - 2] << 16 |
                          tempBuf[cmdLen - 1] << 24;
    int idx = 0;

    if (crc == bufCrc) {
        memcpy(retData, tempBuf, cmdLen % 512);
        *retLen = cmdLen;
        return cmdLen;
    } else {
        return -1;
    }
}

int CFbcCommunication::processData(COMM_DEV_TYPE_E fromDev, unsigned char *pData, int dataLen)
{
    switch (fromDev) {
    case COMM_DEV_CEC: {
        if (mReplyList.WaitDevNo == fromDev && mReplyList.WaitCmd == pData[1]) {
            mReplyList.reDataLen = dataLen;
            memcpy(mReplyList.replyData, pData, dataLen);
            mReplyList.WaitReplyCondition.signal();
        } else if (0) {
        }
        break;
    }
    case COMM_DEV_SERIAL: {
        LOGD("to signal wait dataLen:0x%x, cmdId:0x%x\n", dataLen, pData[5]);
        if (mReplyList.WaitDevNo == fromDev && mReplyList.WaitCmd == pData[5]) {
            mReplyList.reDataLen = dataLen;
            memcpy(mReplyList.replyData, pData, dataLen);
            mReplyList.WaitReplyCondition.signal();
        } else {
            unsigned char cmd = pData[5];
            //just test
            const char *value;
            if (!mbSendKeyCode) {
                value = config_get_str(CFG_SECTION_FBCUART, "fbc_key_event_handle", "null");
                if ( strcmp ( value, "true" ) == 0 )
                    mbSendKeyCode = true;
                else mbSendKeyCode = false;
            }
#if(0)
            switch (cmd) {
            case 0x14:
                if (mbSendKeyCode ) {
                    if (pData[6] >= 12 && pData[6] <= 16 ) { //left ---enter
                        unsigned char key = pData[6]  ;
                        LOGD("  key:0x%x\n", key);

                        //16    key 28       DPAD_CENTER
                        //12    key 103      DPAD_UP
                        //13    key 108      DPAD_DOWN
                        //14    key 105      DPAD_LEFT
                        //15    key 106      DPAD_RIGHT
                        int checkKey = 0;
                        if (key == 16)
                            checkKey = 28 ;
                        else  if (key == 12)
                            checkKey = 103 ;
                        else  if (key == 13)
                            checkKey = 108 ;
                        else  if (key == 14)
                            checkKey = 105 ;
                        else  if (key == 15)
                            checkKey = 106 ;
                        mTvInput.sendkeyCode(checkKey);
                    }
                }
                break;

            default:
                break;
            }
#else
            static long curTime, lastTime;
            int tmp;
            static int st = 0, st_key_up = 0;
            static int st_key_down = 0;
            static int st_key_left = 0;
            static int st_key_right = 0;
            static int checkKey = 0, last_checkKey = 0;


            switch (pData[6]) {                     //different key
            case 12:                                //DPAD_UP
                st_key_up = pData[5];
                if (st_key_up == 0x1D) {        //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 103;
                    mTvInput.sendkeyCode(checkKey);
                } else if (st_key_up == 0x1E) {     //CMD_INPUT_UP
                    //checkKey = 103;
                    //mTvInput.sendkeyCode_Up(checkKey);
                }
                break;

            case 13:                                //DPAD_DOWN
                st_key_down = pData[5];
                if (st_key_down == 0x1D) {          //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 108;
                    mTvInput.sendkeyCode(checkKey);
                } else if (st_key_down == 0x1E) {   //CMD_INPUT_UP
                    //checkKey = 108;
                    //mTvInput.sendkeyCode_Up(checkKey);
                }
                break;

            case 14:                                //DPAD_LEFT
                st = pData[5];
                if (st == 0x1D) {           //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 105;
                    lastTime = 0;
                    mbFbcKeyEnterDown = 1;//true
                    mFbcEnterKeyDownTime = mTvInput.getNowMs();
                    mTvInput.sendKeyRepeatStart(15, 1200, 1500);//code 4, dis 3, repeatTime 2
                } else if (st == 0x1E) {    //CMD_INPUT_UP
                    checkKey = 105;
                    if (mbFbcKeyEnterDown == 1) {
                        mbFbcKeyEnterDown = 0;//false
                        mTvInput.sendKeyRepeatStop();
                        int disFbcEnterKeyDown = mTvInput.getNowMs() - mFbcEnterKeyDownTime;
                        LOGD("disFbcEnterKeyDown = %d", disFbcEnterKeyDown);
                        if (disFbcEnterKeyDown > 1200) { //long down
                        } else {
                            mTvInput.sendkeyCode(105);
                        }
                    }
                    //checkKey = 105;
                    //mTvInput.sendkeyCode_Up(checkKey);
                }
                break;

            case 15:                                //DPAD_RIGHT
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 106;
                    mTvInput.sendkeyCode(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    //checkKey = 106;
                    //mTvInput.sendkeyCode_Up(checkKey);
                }
                break;

            case 16:                                //DPAD_ENTER
                st = pData[5];
                if (st == 0x1D) {           //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 28;
                    lastTime = 0;
                    mbFbcKeyEnterDown = 1;//true
                    mFbcEnterKeyDownTime = mTvInput.getNowMs();
                    mTvInput.sendKeyRepeatStart(158, 1200, 1500);//code 4, dis 3, repeatTime 2
                } else if (st == 0x1E) {    //CMD_INPUT_UP
                    checkKey = 28;
                    if (mbFbcKeyEnterDown == 1) {
                        mbFbcKeyEnterDown = 0;//false
                        mTvInput.sendKeyRepeatStop();
                        int disFbcEnterKeyDown = mTvInput.getNowMs() - mFbcEnterKeyDownTime;
                        LOGD("disFbcEnterKeyDown = %d", disFbcEnterKeyDown);
                        if (disFbcEnterKeyDown > 1200) { //long down
                        } else {
                            mTvInput.sendkeyCode(28);
                        }
                    }
                }
                break;

            case 20:                                //7key power
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 116;
                    //mTvInput.sendIRkeyCode_Down(checkKey);
                    mTvInput.sendIRkeyCode(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    checkKey = 116;
                    //mTvInput.sendIRkeyCode_Up(checkKey);
                }
                break;

            case 26:                                //7key source
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 466;
                    mTvInput.sendIRkeyCode_Down(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    checkKey = 466;
                    mTvInput.sendIRkeyCode_Up(checkKey);
                }
                break;

            case 27:                                //7key menu
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 139;
                    mTvInput.sendkeyCode(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    //checkKey = 139;
                    //mTvInput.sendkeyCode_Up(checkKey);
                }
                break;

            case 40:                                //7key vol -
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 114;
                    mTvInput.sendIRkeyCode_Down(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    checkKey = 114;
                    mTvInput.sendIRkeyCode_Up(checkKey);
                }
                break;

            case 41:                                //7key vol +
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 115;
                    mTvInput.sendIRkeyCode_Down(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    checkKey = 115;
                    mTvInput.sendIRkeyCode_Up(checkKey);
                }
                break;
            case 201:                                //SARADC_DPAD_UP
                st_key_up = pData[5];
                if (st_key_up == 0x1D) {        //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 950;
                    mTvInput.sendkeyCode_Down(checkKey);
                } else if (st_key_up == 0x1E) {     //CMD_INPUT_UP
                    mTvInput.sendkeyCode_Up(checkKey);
                }
                break;

            case 202:                                //SARADC_DOWN
                st_key_down = pData[5];
                if (st_key_down == 0x1D) {          //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 951;
                    mTvInput.sendkeyCode_Down(checkKey);
                } else if (st_key_down == 0x1E) {   //CMD_INPUT_UP
                    mTvInput.sendkeyCode_Up(checkKey);
                }
                break;

            case 203:                                //SARADC_LEFT
                st = pData[5];
                if (st == 0x1D) {           //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 952;
                    lastTime = 0;
                    mbFbcKeyEnterDown = 1;//true
                    mFbcEnterKeyDownTime = mTvInput.getNowMs();
                    mTvInput.sendkeyCode_Down(checkKey);
                    mTvInput.sendKeyRepeatStart(955, 1200, 1500);//code 4, dis 3, repeatTime 2
                } else if (st == 0x1E) {    //CMD_INPUT_UP
                    checkKey = 952;
                    if (mbFbcKeyEnterDown == 1) {
                        mbFbcKeyEnterDown = 0;//false
                        mTvInput.sendKeyRepeatStop();
                        int disFbcEnterKeyDown = mTvInput.getNowMs() - mFbcEnterKeyDownTime;
                        LOGD("disFbcEnterKeyDown = %d", disFbcEnterKeyDown);
                        if (disFbcEnterKeyDown > 1200) { //long down
                            mTvInput.sendkeyCode_Up(955);
                            mTvInput.sendkeyCode_Up(952);
                        } else {
                            mTvInput.sendkeyCode_Up(checkKey);
                        }
                    }
                }
                break;


            case 204:                                //SARADC_RIGHT
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 953;
                    mTvInput.sendkeyCode_Down(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    mTvInput.sendkeyCode_Up(checkKey);

                }
                break;

            case 205:                                //SARADC_ENTER
                st_key_right = pData[5];
                if (st_key_right == 0x1D) {         //CMD_INPUT_DOWN
                    last_checkKey = checkKey;
                    checkKey = 954;
                    mTvInput.sendkeyCode_Down(checkKey);
                } else if (st_key_right == 0x1E) {      //CMD_INPUT_UP
                    mTvInput.sendkeyCode_Up(checkKey);
                }

                break;
            }
#endif
        }
        break;
    }
    default: {
        break;
    }
    }
    return 0;
}
bool CFbcCommunication::threadLoop()
{
    unsigned char readFrameBuf[512];
    while (!exitPending()) { //requietexit() or requietexitWait() not call
        while (mUpgradeFlag == 1) {
            usleep(1000 * 1000);
        }

        int num = mEpoll.wait();

        while (mUpgradeFlag == 1) {
            usleep(1000 * 1000);
        }

        for (int i = 0; i < num; ++i) {
            int fd = (mEpoll)[i].data.fd;
            /**
             * EPOLLIN event
             */
            if ((mEpoll)[i].events & EPOLLIN) {
                if (fd == mHdmiCec.getFd()) { //ce-----------------------------c
                    int bufIndex = 0;
                    int needRead = 4;
                    int haveRead = 0;
                    int idx = 0, readLen = 0;
                    do {
                        readLen = mHdmiCec.readFile(readFrameBuf + bufIndex,  needRead - haveRead);
                        haveRead += readLen;
                        bufIndex += readLen;
                        //if(haveRead == needRead) break;
                    } while (0);

                    if (readLen > 0) {
                        processData(COMM_DEV_CEC, readFrameBuf, readLen);
                    } else {
                    }
                } else if (fd == mSerialPort.getFd()) {
                    //seria---------------------------l
                    int cmdLen = 0, idx = 0;
                    LOGD("serial data come");
                    memset(readFrameBuf, 0, 512);
                    int ret = uartReadData(readFrameBuf, &cmdLen);

                    if (ret == -1) { //data error
                        sendAckCmd(false);
                    } else if (readFrameBuf[4] == 0x80) { //ack
                        LOGD("is ack come");
#ifdef TV_RESEND_UMUTE_TO_FBC
                        if (((readFrameBuf[7] << 8) | readFrameBuf[6]) == 0x8001 &&
                                readFrameBuf[5] == AUDIO_CMD_SET_MUTE) {
                            LOGD("resend unmute to 101 avoid 101 receiving unmute timeout\n");
                            Fbc_Set_Value_INT8(COMM_DEV_SERIAL, AUDIO_CMD_SET_MUTE, 1);
                        }
#endif
                    } else { //not ack
                        sendAckCmd(true);
                        processData(COMM_DEV_SERIAL, readFrameBuf, cmdLen);
                    }
                }
            }

            /**
             * EPOLLOUT event
             */
            if ((mEpoll)[i].events & EPOLLOUT) {

            }
        }
    }
    //exit
    //return true, run again, return false,not run.
    LOGD("thread exited..........\n");
    return false;
}

int CFbcCommunication::Fbc_Set_Value_INT8(COMM_DEV_TYPE_E toDev, int cmd_type, int value)
{
    if (mUpgradeFlag == 1) {
        return 0;
    }
    LOGD("%s  cmd =0x%x, value=%d", __FUNCTION__, cmd_type, value);
    if (toDev == COMM_DEV_CEC) {
        unsigned char cmd[16], rxbuf[16];
        int rxlen = 0, idx = 0;
        memset(cmd, 0, 16);
        memset(rxbuf, 0, 16);
        cmd[0] = 0x40;
        cmd[1] = cmd_type;
        cmd[2] = value;
        sendDataOneway(COMM_DEV_CEC, cmd, 4, 0);
    } else if (toDev == COMM_DEV_SERIAL) {
        int crc32value = 0;
        unsigned char write_buf[512];
        unsigned char rxbuf[512];
        int rxlen = 0, idx = 0;

        //leading code
        write_buf[0] = 0x5a;
        write_buf[1] = 0x5a;
        //package length from begin to end
        write_buf[2] = 0xb;
        write_buf[3] = 0x0;
        //Ack byte
        write_buf[4] = 0x0;
        //cmd ID
        write_buf[5] = cmd_type;
        //parameter
        write_buf[6] = value;
        //crc32 little Endian
        crc32value = Calcrc32(0, write_buf, 7);
        write_buf[7] = (crc32value >> 0) & 0xFF;
        write_buf[8] = (crc32value >> 8) & 0xFF;
        write_buf[9] = (crc32value >> 16) & 0xFF;
        write_buf[10] = (crc32value >> 24) & 0xFF;
        sendDataOneway(COMM_DEV_SERIAL, write_buf, write_buf[2], 0);
    }
    return 0;
}
int CFbcCommunication::Fbc_Set_Value_INT32(COMM_DEV_TYPE_E toDev, int cmd_type, int value)
{
    if (mUpgradeFlag == 1) {
        return 0;
    }

    if (toDev == COMM_DEV_SERIAL) {
        int crc32value = 0;
        unsigned char write_buf[512];
        unsigned char rxbuf[512];
        int rxlen = 0, idx = 0;

        //leading code
        write_buf[0] = 0x5a;
        write_buf[1] = 0x5a;
        //package length from begin to end
        write_buf[2] = 14;
        write_buf[3] = 0x0;
        //Ack byte
        write_buf[4] = 0x0;
        //cmd ID
        write_buf[5] = cmd_type;
        //parameter
        write_buf[6] = (value >> 0) & 0xFF;
        write_buf[7] = (value >> 8) & 0xFF;
        write_buf[8] = (value >> 16) & 0xFF;
        write_buf[9] = (value >> 24) & 0xFF;
        //crc32 little Endian
        crc32value = Calcrc32(0, write_buf, 10);
        write_buf[10] = (crc32value >> 0) & 0xFF;
        write_buf[11] = (crc32value >> 8) & 0xFF;
        write_buf[12] = (crc32value >> 16) & 0xFF;
        write_buf[13] = (crc32value >> 24) & 0xFF;

        return sendDataOneway(COMM_DEV_SERIAL, write_buf, write_buf[2], 0);
    }
    return -1;
}
int CFbcCommunication::Fbc_Get_Value_INT8(COMM_DEV_TYPE_E fromDev, int cmd_type, int *value)
{
    if (mUpgradeFlag == 1) {
        return 0;
    }

    LOGD("%s  cmd =0x%x", __FUNCTION__, cmd_type);

    if (fromDev == COMM_DEV_CEC) {
        unsigned char cmd[16], rxbuf[16];
        int rxlen = 0, idx = 0;
        memset(cmd, 0, 16);
        memset(rxbuf, 0, 16);
        cmd[0] = 0x40;
        cmd[1] = cmd_type;
        sendDataAndWaitReply(COMM_DEV_CEC, COMM_DEV_CEC, cmd[1], cmd, 4, 0, rxbuf, &rxlen, 0);
        *value =  rxbuf[6];
    } else if (fromDev == COMM_DEV_SERIAL) {
        int crc32value = 0, idx = 0, rxlen = 0;
        unsigned char write_buf[16];
        unsigned char rxbuf[16];
        memset(rxbuf, 0, 16);

        //leading code
        idx = 0;
        write_buf[idx++] = 0x5a;
        write_buf[idx++] = 0x5a;
        //package length from begin to end
        write_buf[idx++] = 0xa;
        write_buf[idx++] = 0x0;
        //Ack byte
        write_buf[idx++] = 0x0;
        //cmd ID
        write_buf[idx++] = cmd_type;
        //crc32 little Endian
        crc32value = Calcrc32(0, write_buf, idx);
        write_buf[idx++] = (crc32value >> 0) & 0xFF;
        write_buf[idx++] = (crc32value >> 8) & 0xFF;
        write_buf[idx++] = (crc32value >> 16) & 0xFF;
        write_buf[idx++] = (crc32value >> 24) & 0xFF;

        sendDataAndWaitReply(COMM_DEV_SERIAL, COMM_DEV_SERIAL, write_buf[5], write_buf, write_buf[2], 2000, rxbuf, &rxlen, 0);
        *value =  rxbuf[6];
    }
    return 0;
}

int CFbcCommunication::Fbc_Set_BatchValue(COMM_DEV_TYPE_E toDev, unsigned char *cmd_buf, int count)
{
    if (mUpgradeFlag == 1) {
        return 0;
    }

    if ( 512 <= count) {
        return -1;
    }

    if (toDev == COMM_DEV_CEC) {
        unsigned char cmd[512], rxbuf[512];
        int rxlen = 0, idx = 0;
        memset(cmd, 0, 512);
        memset(rxbuf, 0, 512);
        cmd[0] = 0x40;
        for (idx = 0; idx < count; idx++) {
            cmd[idx + 1] = cmd_buf[idx];
        }
        sendDataOneway(COMM_DEV_CEC, cmd, count + 1, 0);
    } else if (toDev == COMM_DEV_SERIAL) {
        int crc32value = 0;
        unsigned char write_buf[512];
        unsigned char rxbuf[512];
        int rxlen = 0, idx = 0;

        //leading code
        write_buf[0] = 0x5a;
        write_buf[1] = 0x5a;
        //package length from begin to end
        write_buf[2] = (count + 9) & 0xFF;
        write_buf[3] = ((count + 9) >> 8) & 0xFF;
        //Ack byte : 0x80-> ack package;0x00->normal package;
        write_buf[4] = 0x00;

        for (idx = 0; idx < count; idx++) {
            write_buf[idx + 5] = cmd_buf[idx];
        }
        //crc32 little Endian
        crc32value = Calcrc32(0, write_buf, count + 5);
        write_buf[count + 5] = (crc32value >> 0) & 0xFF;
        write_buf[count + 6] = (crc32value >> 8) & 0xFF;
        write_buf[count + 7] = (crc32value >> 16) & 0xFF;
        write_buf[count + 8] = (crc32value >> 24) & 0xFF;
        sendDataOneway(COMM_DEV_SERIAL, write_buf, count + 9, 0);
    }
    return 0;
}

int CFbcCommunication::Fbc_Get_BatchValue(COMM_DEV_TYPE_E fromDev, unsigned char *cmd_buf, int count)
{
    if (mUpgradeFlag == 1) {
        return 0;
    }

    if ( 512 <= count) {
        return -1;
    }
    int ret = 0;
    // TODO: read value
    if (fromDev == COMM_DEV_CEC) {
        unsigned char cmd[512], rxbuf[512];
        int rxlen = 0, idx = 0;
        memset(cmd, 0, 512);
        memset(rxbuf, 0, 512);
        cmd[0] = 0x40;
        cmd[1] = cmd_buf[0];
        sendDataAndWaitReply(COMM_DEV_CEC, COMM_DEV_CEC, cmd[1], cmd, count + 1, 0, rxbuf, &rxlen, 0);

        if (rxlen > 2) {
            for (idx = 0; idx < rxlen; idx++) {
                cmd_buf[idx] = cmd[idx + 1];
            }
        }
    } else if (fromDev == COMM_DEV_SERIAL) {
        int crc32value = 0, idx = 0, rxlen = 0;
        unsigned char write_buf[512];
        unsigned char rxbuf[512];
        memset(write_buf, 0, 512);
        memset(rxbuf, 0, 512);

        //leading code
        write_buf[0] = 0x5a;
        write_buf[1] = 0x5a;
        //package length from begin to end
        write_buf[2] = (count + 9) & 0xFF;
        write_buf[3] = ((count + 9) >> 8) & 0xFF;
        //Ack byte
        write_buf[4] = 0x00;
        //cmd ID
        for (idx = 0; idx < count; idx++) {
            write_buf[idx + 5] = cmd_buf[idx];
        }
        //crc32 little Endian
        crc32value = Calcrc32(0, write_buf, count + 5);
        write_buf[count + 5] = (crc32value >> 0) & 0xFF;
        write_buf[count + 6] = (crc32value >> 8) & 0xFF;
        write_buf[count + 7] = (crc32value >> 16) & 0xFF;
        write_buf[count + 8] = (crc32value >> 24) & 0xFF;
        sendDataAndWaitReply(COMM_DEV_SERIAL, COMM_DEV_SERIAL, write_buf[5], write_buf, write_buf[2], 2000, rxbuf, &rxlen, 0);

        if (rxlen > 9) {
            for (idx = 0; idx < (rxlen - 9); idx++) {
                cmd_buf[idx] = rxbuf[idx + 5];
            }
        }
        ret = rxlen;
    }

    return ret;
}

int CFbcCommunication::cfbc_Set_Gain_Red(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_RED_GAIN_DEF, value);
}

int CFbcCommunication::cfbc_Get_Gain_Red(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_RED_GAIN_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Gain_Green(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_GREEN_GAIN_DEF, value);
}

int CFbcCommunication::cfbc_Get_Gain_Green(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_GREEN_GAIN_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Gain_Blue(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_BLUE_GAIN_DEF, value);
}

int CFbcCommunication::cfbc_Get_Gain_Blue(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_BLUE_GAIN_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Offset_Red(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_PRE_RED_OFFSET_DEF, value);
}

int CFbcCommunication::cfbc_Get_Offset_Red(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_PRE_RED_OFFSET_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Offset_Green(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_PRE_GREEN_OFFSET_DEF, value);
}

int CFbcCommunication::cfbc_Get_Offset_Green(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_PRE_GREEN_OFFSET_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Offset_Blue(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_PRE_BLUE_OFFSET_DEF, value);
}

int CFbcCommunication::cfbc_Get_Offset_Blue(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_PRE_BLUE_OFFSET_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_WB_Initial(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_WB, value);
}

int CFbcCommunication::cfbc_Get_WB_Initial(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_WB | 0x80, value);
}

int CFbcCommunication::cfbc_Set_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int value)
{
    int fbcValue = value;
    switch (value) {
    case 0:     //standard
        fbcValue = 1;
        break;
    case 1:     //warm
        fbcValue = 2;
        break;
    case 2:    //cold
        fbcValue = 0;
        break;
    default:
        break;
    }
    LOGD("before set fbcValue = %d", fbcValue);
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_COLOR_TEMPERATURE_DEF, fbcValue);
}

int CFbcCommunication::cfbc_Get_ColorTemp_Mode(COMM_DEV_TYPE_E fromDev, int *value)
{
    Fbc_Get_Value_INT8(fromDev, VPU_CMD_COLOR_TEMPERATURE_DEF | 0x80, value);

    switch (*value) {
    case 0:     //cold
        *value = 2;
        break;
    case 1:     //standard
        *value = 0;
        break;
    case 2:    //warm
        *value = 1;
        break;
    default:
        break;
    }

    return 0;
}

int CFbcCommunication::cfbc_Set_Test_Pattern(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_PATTEN_SEL, value);
}

int CFbcCommunication::cfbc_Get_Test_Pattern(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_PATTEN_SEL | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Picture_Mode(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_PICTURE_MODE, value);
}

int CFbcCommunication::cfbc_Get_Picture_Mode(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_PICTURE_MODE | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Contrast(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_CONTRAST_DEF, value);
}

int CFbcCommunication::cfbc_Get_Contrast(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_CONTRAST_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Brightness(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_BRIGHTNESS_DEF, value);
}

int CFbcCommunication::cfbc_Get_Brightness(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_BRIGHTNESS_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Saturation(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_COLOR_DEF, value);
}

int CFbcCommunication::cfbc_Get_Saturation(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_COLOR_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_HueColorTint(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_HUE_DEF, value);
}

int CFbcCommunication::cfbc_Get_HueColorTint(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_HUE_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Backlight(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_BACKLIGHT_DEF, value);
}

int CFbcCommunication::cfbc_Get_Backlight(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_BACKLIGHT_DEF | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Source(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_SOURCE, value);
}

int CFbcCommunication::cfbc_Get_Source(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_SOURCE | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Mute(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("cfbc_Set_Mute = %d", value);
    return Fbc_Set_Value_INT8(fromDev, AUDIO_CMD_SET_MUTE, value);
}
int CFbcCommunication::cfbc_Set_FBC_Audio_Source(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, AUDIO_CMD_SET_SOURCE, value);
}

int CFbcCommunication::cfbc_Get_Mute(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, AUDIO_CMD_GET_MUTE, value);
}

int CFbcCommunication::cfbc_Set_Volume_Bar(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, AUDIO_CMD_SET_VOLUME_BAR, value);
}

int CFbcCommunication::cfbc_Get_Volume_Bar(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, AUDIO_CMD_GET_VOLUME_BAR, value);
}

int CFbcCommunication::cfbc_Set_Balance(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, AUDIO_CMD_SET_BALANCE, value);
}

int CFbcCommunication::cfbc_Get_Balance(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, AUDIO_CMD_GET_BALANCE, value);
}

int CFbcCommunication::cfbc_Set_Master_Volume(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, AUDIO_CMD_SET_MASTER_VOLUME, value);
}

int CFbcCommunication::cfbc_Get_Master_Volume(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, AUDIO_CMD_GET_MASTER_VOLUME, value);
}

int CFbcCommunication::cfbc_Set_CM(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE;
    cmd[1] = VPU_MODULE_CM2;
    cmd[2] = value;//value:0~1
    return Fbc_Set_BatchValue(fromDev, cmd, 3);
}

int CFbcCommunication::cfbc_Get_CM(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE | 0x80;
    cmd[1] = VPU_MODULE_CM2;

    Fbc_Get_BatchValue(fromDev, cmd, 2);
    *value = cmd[2];

    return 0;
}
int CFbcCommunication::cfbc_Set_DNLP(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE;
    cmd[1] = VPU_MODULE_DNLP;
    cmd[2] = value;//value:0~1

    return Fbc_Set_BatchValue(fromDev, cmd, 3);
}

int CFbcCommunication::cfbc_Get_DNLP(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE | 0x80;
    cmd[1] = VPU_MODULE_DNLP;

    Fbc_Get_BatchValue(fromDev, cmd, 2);
    *value = cmd[2];

    return 0;
}

int CFbcCommunication::cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_TYPE_E fromDev, char sw_ver[], char build_time[], char git_ver[], char git_branch[], char build_name[])
{
    int rx_len = 0, tmp_ind = 0;
    unsigned char cmd[512];

    if (sw_ver == NULL || build_time == NULL || git_ver == NULL || git_branch == NULL || build_name == NULL) {
        return -1;
    }

    memset(cmd, 0, 512);
    cmd[0] = CMD_FBC_MAIN_CODE_VERSION;
    rx_len = Fbc_Get_BatchValue(fromDev, cmd, 1);

    sw_ver[0] = 0;
    build_time[0] = 0;
    git_ver[0] = 0;
    git_branch[0] = 0;
    build_name[0] = 0;

    if (rx_len <= 0) {
        return -1;
    }

    if (cmd[0] != CMD_FBC_MAIN_CODE_VERSION || cmd[1] != 0x88 || cmd[2] != 0x99) {
        return -1;
    }

    tmp_ind = 3;

    strcpy(sw_ver, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(sw_ver);
    tmp_ind += 1;

    strcpy(build_time, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(build_time);
    tmp_ind += 1;

    strcpy(git_ver, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(git_ver);
    tmp_ind += 1;

    strcpy(git_branch, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(git_branch);
    tmp_ind += 1;

    strcpy(build_name, (char *)(cmd + tmp_ind));
    tmp_ind += strlen(build_name);
    tmp_ind += 1;
    LOGD("sw_ver=%s, buildt=%s, gitv=%s, gitb=%s,bn=%s", sw_ver, build_time, git_ver, git_branch, build_name);
    return 0;
}

int CFbcCommunication::cfbc_Set_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, const char *pSNval)
{
    unsigned char cmd[512];
    int len = strlen(pSNval);

    memset(cmd, 0, 512);

    cmd[0] = CMD_SET_FACTORY_SN;

    memcpy(cmd + 1, pSNval, len);

    LOGD("cmd : %s\n", cmd);

    return Fbc_Set_BatchValue(fromDev, cmd, len + 1);
}

int CFbcCommunication::cfbc_Get_FBC_Factory_SN(COMM_DEV_TYPE_E fromDev, char FactorySN[])
{
    int rx_len = 0;
    unsigned char cmd[512];
    memset(cmd, 0, 512);
    cmd[0] = CMD_GET_FACTORY_SN;
    rx_len = Fbc_Get_BatchValue(fromDev, cmd, 1);
    if (rx_len <= 0) {
        return -1;
    }
    strncpy(FactorySN, (char *)(cmd + 1), rx_len);

    LOGD("panelModel=%s", FactorySN);
    return 0;
}

int CFbcCommunication::cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_TYPE_E fromDev, char panel_model[])
{
    int rx_len = 0;
    unsigned char cmd[512];
    memset(cmd, 0, 512);
    cmd[0] = CMD_DEVICE_ID;
    rx_len = Fbc_Get_BatchValue(fromDev, cmd, 1);
    if (rx_len <= 0) {
        return -1;
    }
    strcpy(panel_model, (char *)(cmd + 1));

    LOGD("panelModel=%s", panel_model);
    return 0;
}

int CFbcCommunication::cfbc_Set_FBC_panel_power_switch(COMM_DEV_TYPE_E fromDev, int switch_val)
{
    unsigned char cmd[512];

    memset(cmd, 0, 512);

    cmd[0] = FBC_PANEL_POWER;
    cmd[1] = switch_val; //0 is fbc panel power off, 1 is panel power on.

    return Fbc_Set_BatchValue(fromDev, cmd, 2);
}

int CFbcCommunication::cfbc_Set_FBC_suspend(COMM_DEV_TYPE_E fromDev, int switch_val)
{
    unsigned char cmd[512];

    memset(cmd, 0, 512);

    cmd[0] = FBC_SUSPEND_POWER;
    cmd[1] = switch_val; //0

    return Fbc_Set_BatchValue(fromDev, cmd, 2);
}
int CFbcCommunication::cfbc_Set_FBC_User_Setting_Default(COMM_DEV_TYPE_E fromDev, int param)
{
    unsigned char cmd[512];

    memset(cmd, 0, 512);

    cmd[0] = FBC_USER_SETTING_DEFAULT;
    cmd[1] = param; //0

    return Fbc_Set_BatchValue(fromDev, cmd, 2);
}

int CFbcCommunication::cfbc_SendRebootToUpgradeCmd(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT32(fromDev, FBC_REBOOT_UPGRADE, value);
}

int CFbcCommunication::cfbc_FBC_Send_Key_To_Fbc(COMM_DEV_TYPE_E fromDev, int keycode __unused, int param __unused)
{
    unsigned char cmd[512];

    memset(cmd, 0, 512);

    cmd[0] = CMD_ACTIVE_KEY;
    cmd[1] = keycode; //0

    return Fbc_Set_BatchValue(fromDev, cmd, 2);
}

int CFbcCommunication::cfbc_Get_FBC_PANEL_REVERSE(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = CMD_PANEL_INFO;
    //0://panel reverse
    //1://panel output_mode
    //2://panel byte num
    cmd[1] = 0;

    Fbc_Get_BatchValue(fromDev, cmd, 2);
    //cmd[0] cmdid-PANEL-INFO
    //cmd[1] param[0] - panel reverse
    *value = cmd[2];

    return 0;
}

int CFbcCommunication::cfbc_Get_FBC_PANEL_OUTPUT(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = CMD_PANEL_INFO;
    //0://panel reverse
    //1://panel output_mode
    //2://panel byte num
    cmd[1] = 1;

    Fbc_Get_BatchValue(fromDev, cmd, 2);
    //cmd[0] cmdid-PANEL-INFO
    //cmd[1] param[0] - panel reverse
    *value = cmd[2];

    return 0;
}

int CFbcCommunication::cfbc_Set_FBC_project_id(COMM_DEV_TYPE_E fromDev, int prj_id)
{
    unsigned char cmd[512];

    memset(cmd, 0, 512);

    cmd[0] = CMD_SET_PROJECT_SELECT;
    cmd[1] = prj_id;

    return Fbc_Set_BatchValue(fromDev, cmd, 2);
}

int CFbcCommunication::cfbc_Get_FBC_project_id(COMM_DEV_TYPE_E fromDev, int *prj_id)
{
    return Fbc_Get_Value_INT8(fromDev, CMD_GET_PROJECT_SELECT, prj_id);
}

int CFbcCommunication::cfbc_Set_Gamma(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE;
    cmd[1] = VPU_MODULE_GAMMA;
    cmd[2] = value;//value:0~1

    return Fbc_Set_BatchValue(fromDev, cmd, 3);
}

int CFbcCommunication::cfbc_Get_Gamma(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE | 0x80;
    cmd[1] = VPU_MODULE_GAMMA;

    Fbc_Get_BatchValue(fromDev, cmd, 2);
    *value = cmd[2];

    return 0;
}

int CFbcCommunication::cfbc_Set_VMute(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_USER_VMUTE;
    cmd[1] = value;
    LOGD("cfbc_Set_VMute=%d", cmd[1]);

    return Fbc_Set_BatchValue(fromDev, cmd, 2);
}
int CFbcCommunication::cfbc_Set_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE;
    cmd[1] = VPU_MODULE_WB;
    cmd[2] = value;//value:0~1

    return Fbc_Set_BatchValue(fromDev, cmd, 3);
}

int CFbcCommunication::cfbc_Get_WhiteBalance_OnOff(COMM_DEV_TYPE_E fromDev, int *value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_ENABLE | 0x80;
    cmd[1] = VPU_MODULE_WB;

    Fbc_Get_BatchValue(fromDev, cmd, 2);
    *value = cmd[2];

    return 0;
}

int CFbcCommunication::cfbc_Set_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode, unsigned char r_gain, unsigned char g_gain, unsigned char b_gain, unsigned char r_offset, unsigned char g_offset, unsigned char b_offset)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_WB_VALUE;
    cmd[1] = mode;
    cmd[2] = r_gain;
    cmd[3] = g_gain;
    cmd[4] = b_gain;
    cmd[5] = r_offset;
    cmd[6] = g_offset;
    cmd[7] = b_offset;

    return Fbc_Set_BatchValue(fromDev, cmd, 8);
}

int CFbcCommunication::cfbc_TestPattern_Select(COMM_DEV_TYPE_E fromDev, int value)
{
    int ret = -1;
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    LOGD("Call vpp 63 2 1\n");
    cmd[0] = VPU_CMD_SRCIF;
    cmd[1] = 2;
    cmd[2] = 1;

    ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//close csc0

    if (ret == 0) {
        LOGD("Call vpp 9 11 1\n");
        memset(cmd, 0, 512);
        cmd[0] = VPU_CMD_ENABLE;
        cmd[1] = 11;
        cmd[2] = 1;
        ret = Fbc_Set_BatchValue(fromDev, cmd, 3);
        if (ret == 0) {
            memset(cmd, 0, 512);
            LOGD("Call vpp 42 1-17\n");
            Fbc_Set_Value_INT8(fromDev, VPU_CMD_PATTEN_SEL, value);
        }
    }

    return ret;
}
int CFbcCommunication::cfbc_WhiteBalance_GrayPattern_OnOff(COMM_DEV_TYPE_E fromDev, int onOff)
{
    int ret = -1;
    unsigned char cmd[512];
    memset(cmd, 0, 512);
    if (onOff == 0) { //On
        LOGD("Call vpp 63 2 1");
        cmd[0] = VPU_CMD_SRCIF;
        cmd[1] = 2;
        cmd[2] = 1;
        ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//close csc0
        if (ret == 0) {
            LOGD("Call vpp 9 9 0");
            cmd[0] = VPU_CMD_ENABLE;
            cmd[1] = 9;
            cmd[2] = 0;
            ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//close csc1
            if (ret == 0) {
                LOGD("Call vpp 9 10 0");
                cmd[0] = VPU_CMD_ENABLE;
                cmd[1] = 10;
                cmd[2] = 0;
                ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//close dnlp
                if (ret == 0) {
                    LOGD("Call vpp 9 8 0");
                    cmd[0] = VPU_CMD_ENABLE;
                    cmd[1] = 8;
                    cmd[2] = 0;
                    ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//close cm
                }
            }
        }

    } else { //Off
        LOGD("Call vpp 63 2 2");
        cmd[0] = VPU_CMD_SRCIF;
        cmd[1] = 2;
        cmd[2] = 2;
        ret = Fbc_Set_BatchValue(fromDev, cmd, 3);
        if (ret == 0) {
            LOGD("Call vpp 9 9 1");
            cmd[0] = VPU_CMD_ENABLE;
            cmd[1] = 9;
            cmd[2] = 1;
            ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//open csc1
            if (ret == 0) {
                LOGD("Call vpp 9 10 1");
                cmd[0] = VPU_CMD_ENABLE;
                cmd[1] = 10;
                cmd[2] = 1;
                ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//open dnlp
                if (ret == 0) {
                    LOGD("Call vpp 9 8 1");
                    cmd[0] = VPU_CMD_ENABLE;
                    cmd[1] = 8;
                    cmd[2] = 1;
                    ret = Fbc_Set_BatchValue(fromDev, cmd, 3);//open cm
                }
            }
        }
    }
    return ret;
}

int CFbcCommunication::cfbc_WhiteBalance_SetGrayPattern(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    int ret = -1;
    unsigned char cmd[512];
    memset(cmd, 0, 512);
    cmd[0] = VPU_CMD_GRAY_PATTERN;
    cmd[1] = value;
    cmd[2] = value;
    cmd[3] = value;
    ret = Fbc_Set_BatchValue(fromDev, cmd, 4);
    return ret;
}

int CFbcCommunication::cfbc_Set_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, unsigned char value)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = CMD_SET_AUTO_BACKLIGHT_ONFF;
    cmd[1] = value;
    LOGD("cfbc_Set_naturelight_onoff\n");
    return Fbc_Set_BatchValue(fromDev, cmd, 2);
}

int CFbcCommunication::cfbc_Get_Auto_Backlight_OnOff(COMM_DEV_TYPE_E fromDev, int *value)
{
    LOGD("cfbc_get_naturelight_onoff\n");
    return Fbc_Get_Value_INT8(fromDev, CMD_GET_AUTO_BACKLIGHT_ONFF, value);
}
int CFbcCommunication::cfbc_Get_WB_Batch(COMM_DEV_TYPE_E fromDev, unsigned char mode, unsigned char *r_gain, unsigned char *g_gain, unsigned char *b_gain, unsigned char *r_offset, unsigned char *g_offset, unsigned char *b_offset)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = VPU_CMD_WB_VALUE | 0x80;
    cmd[1] = mode;

    Fbc_Get_BatchValue(fromDev, cmd, 2);
    //*mode = cmd[1];
    *r_gain = cmd[2];
    *g_gain = cmd[3];
    *b_gain = cmd[4];
    *r_offset = cmd[5];
    *g_offset = cmd[6];
    *b_offset = cmd[7];

    return 0;
}

int CFbcCommunication::cfbc_Set_backlight_onoff(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_BACKLIGHT_EN, value);
}

int CFbcCommunication::cfbc_Get_backlight_onoff(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, VPU_CMD_BACKLIGHT_EN, value);
}

int CFbcCommunication::cfbc_Set_LVDS_SSG_Set(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, CMD_LVDS_SSG_SET, value);
}

int CFbcCommunication::cfbc_Set_AUTO_ELEC_MODE(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s  cmd =0x%x, value=%d~~~~~~~~", __FUNCTION__, VPU_CMD_SET_ELEC_MODE, value);
    return Fbc_Set_Value_INT8(fromDev, VPU_CMD_SET_ELEC_MODE, value);
}

int CFbcCommunication::cfbc_Get_AUTO_ELEC_MODE(COMM_DEV_TYPE_E fromDev __unused, int *value __unused)
{
    return 0;
}

int CFbcCommunication::cfbc_Set_Thermal_state(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s  cmd =0x%x, data%d ~~~~~~~~~\n", __FUNCTION__, CMD_HDMI_STAT, value);
    //return Fbc_Set_Value_INT8(fromDev, CMD_HDMI_STAT, value);
    return Fbc_Set_Value_INT32(fromDev, CMD_HDMI_STAT, value);
}

int CFbcCommunication::cfbc_Set_LightSensor_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, 0x84, value);
    //    return Fbc_Set_Value_INT8(fromDev, CMD_LIGHT_SENSOR, value);
}

int CFbcCommunication::cfbc_Get_LightSensor_N310(COMM_DEV_TYPE_E fromDev __unused, int *value __unused)
{
    //    return Fbc_Get_Value_INT8(fromDev, CMD_LIGHT_SENSOR|0x80, value);
    return 0;
}

int CFbcCommunication::cfbc_Set_Dream_Panel_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, 0x83, value);
    //    return Fbc_Set_Value_INT8(fromDev, CMD_DREAM_PANEL, value);
}

int CFbcCommunication::cfbc_Get_Dream_Panel_N310(COMM_DEV_TYPE_E fromDev __unused, int *value __unused)
{
    //    return Fbc_Get_Value_INT8(fromDev, CMD_DREAM_PANEL|0x80, value);
    return 0;
}

int CFbcCommunication::cfbc_Set_MULT_PQ_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, 0x81, value);
    //    return Fbc_Set_Value_INT8(fromDev, CMD_MUTI_PQ, value);
}

int CFbcCommunication::cfbc_Get_MULT_PQ_N310(COMM_DEV_TYPE_E fromDev __unused, int *value __unused)
{
    //    return Fbc_Get_Value_INT8(fromDev, CMD_MUTI_PQ|0x80, value);
    return 0;
}

int CFbcCommunication::cfbc_Set_MEMC_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, 0x82, value);
    //    return Fbc_Set_Value_INT8(fromDev, CMD_MEMC, value);
}

int CFbcCommunication::cfbc_Get_MEMC_N310(COMM_DEV_TYPE_E fromDev __unused, int *value __unused)
{
    //    return Fbc_Get_Value_INT8(fromDev, CMD_MEMC|0x80, value);
    return 0;
}

int CFbcCommunication::cfbc_Set_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, CMD_BLUETOOTH_I2S_STATUS, value);
}

int CFbcCommunication::cfbc_Get_Bluetooth_IIS_onoff(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, CMD_BLUETOOTH_I2S_STATUS | 0x80, value);
}

int CFbcCommunication::cfbc_Set_Led_onoff(COMM_DEV_TYPE_E fromDev, int val_1, int val_2, int val_3)
{
    unsigned char cmd[512];
    memset(cmd, 0, 512);

    cmd[0] = CMD_SET_LED_MODE;
    cmd[1] = val_1;
    cmd[2] = val_2;
    cmd[3] = val_3;

    return Fbc_Set_BatchValue(fromDev, cmd, 4);
}
int CFbcCommunication::cfbc_Set_LockN_state(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s  cmd =0x%x, data%d ~~~~~~~~~\n", __FUNCTION__, CMD_SET_LOCKN_DISABLE, value);
    return Fbc_Set_Value_INT8(fromDev, CMD_SET_LOCKN_DISABLE, value);
}

int CFbcCommunication::cfbc_SET_SPLIT_SCREEN_DEMO(COMM_DEV_TYPE_E fromDev, int value)
{
    LOGD("%s,cmd[%#x], data[%d]\n", __FUNCTION__, CMD_SET_SPLIT_SCREEN_DEMO, value);
    return Fbc_Set_Value_INT8(fromDev, CMD_SET_SPLIT_SCREEN_DEMO, value);
}

int CFbcCommunication::cfbc_Set_AP_STANDBY_N310(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, CMD_PANEL_ON_OFF, value);
    //    return 0;
}

int CFbcCommunication::cfbc_Get_AP_STANDBY_N310(COMM_DEV_TYPE_E fromDev, int *value)
{
    return Fbc_Get_Value_INT8(fromDev, CMD_PANEL_ON_OFF | 0x80, value);
    //    return 0;
}
int CFbcCommunication::cfbc_Set_Fbc_Uboot_Stage(COMM_DEV_TYPE_E fromDev, int value)
{
    return Fbc_Set_Value_INT8(fromDev, CMD_SET_UBOOT_STAGE, value);
}

void CFbcCommunication::CFbcMsgQueue::handleMessage ( CMessage &msg )
{
    LOGD ( "%s, CFbcCommunication::CFbcMsgQueue::handleMessage type = %d", __FUNCTION__,  msg.mType );

    switch ( msg.mType ) {
    case TV_MSG_COMMON:
        break;

    case TV_MSG_SEND_KEY: {
        LOGD("CFbcMsgQueue msg type = %d", msg.mType);
        //CFbcCommunication *pFbc = ( CFbcCommunication * ) ( msg.mpData );
        //pFbc->mTvInput.sendkeyCode_Down(4);
        //pFbc->mbDownHaveSend = 1;//true
        break;
    }

    default:
        break;
    }
}

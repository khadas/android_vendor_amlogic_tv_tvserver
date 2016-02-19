
#include "CHDMIRxCEC.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <CTvLog.h>

#include "../tvconfig/tvconfig.h"

#define CC_ENABLE_PRINT_MESSAGE (0)

#define CS_HDMIRX_CEC_PATH    "/dev/hdmirx0"

CHDMIRxCEC::CHDMIRxCEC()
{
    int i = 0;

    for (i = 0; i < CC_REQUEST_LIST_SIZE; i++) {
        ClrReplyListItem(&mReplyList[i]);
        memset((void *) &mMsgBuf[i], 0, sizeof(struct _cec_msg));
    }

    mModuleEnableFlag = -1;
    mRequestPause = false;
    mState = STATE_STOPED;

    mpObserver = NULL;

    for (i = 0; i < CC_SOURCE_DEV_REFRESH_CNT; i++) {
        mSourceDevLogicAddrBuf[i] = -1;
        mSourceDevRefreshBuf[i] = -1;
    }

    mSourceDevLogicAddrBuf[0] = E_LA_TV;
    mSourceDevLogicAddrBuf[1] = E_LA_TV;
    mSourceDevLogicAddrBuf[2] = E_LA_TV;

    mSourceDevRefreshBuf[0] = E_LA_PLAYBACK1;
    mSourceDevRefreshBuf[1] = E_LA_PLAYBACK2;
    mSourceDevRefreshBuf[2] = E_LA_PLYBACK3;
}

CHDMIRxCEC::~CHDMIRxCEC()
{
}

int CHDMIRxCEC::start()
{
    CMutex::Autolock _l (mLock);

    if (GetModuleEnableFlag() == 0) {
        return -1;
    }

    if (mState == STATE_STOPED) {
        this->run();
    }

    return mState;
}

int CHDMIRxCEC::stop()
{
    CMutex::Autolock _l (mLock);

    if (GetModuleEnableFlag() == 0) {
        return -1;
    }

    if (mState == STATE_PAUSE) {
        resume();
    }

    requestExit();
    mState = STATE_STOPED;
    return mState;
}

int CHDMIRxCEC::pause()
{
    CMutex::Autolock _l (mLock);

    if (GetModuleEnableFlag() == 0) {
        return -1;
    }

    mRequestPause = true;
    return 0;
}

int CHDMIRxCEC::resume()
{
    CMutex::Autolock _l (mLock);

    if (GetModuleEnableFlag() == 0) {
        return -1;
    }

    ClearRxMessageBuffer();

    mRequestPause = false;
    mPauseCondition.signal();

    ClearRxMessageBuffer();
    return 0;
}

int CHDMIRxCEC::isAllowOperate(int source_input)
{
    if (GetModuleEnableFlag() == 0) {
        return -1;
    }

    int source_port = CTvin::Tvin_GetSourcePortBySourceInput((tv_source_input_t)source_input);
    if (CTvin::getInstance()->VDIN_GetPortConnect(source_port) == false) {
        return -1;
    }

    return 0;
}

int CHDMIRxCEC::GetModuleEnableFlag()
{
    const char *config_value = NULL;

    if (mModuleEnableFlag < 0) {
        config_value = config_get_str(CFG_SECTION_TV, "tvin.hdmirx.cec.enable", "null");
        if (strcasecmp(config_value, "null") == 0) {
            mModuleEnableFlag = 0;
        } else if (strcasecmp(config_value, "1") == 0 || strcasecmp(config_value, "true") == 0) {
            mModuleEnableFlag = 1;
        } else {
            mModuleEnableFlag = 0;
        }
    }

    return mModuleEnableFlag;
}

int CHDMIRxCEC::PrintMessage(const char *func_name, int data_type, struct _cec_msg *msg)
{
#ifdef CC_ENABLE_PRINT_MESSAGE
    if (msg == NULL) {
        LOGE("%s, msg is NULL\n", func_name);
        return -1;
    }

    LOGD("%s, msg_len = %d\n", func_name, msg->msg_len);
    LOGD("%s, msg addr 0x%02X\n", func_name, msg->addr);
    LOGD("%s, msg cmd 0x%02X\n", func_name, msg->cmd);
    for (int i = 0; i < msg->msg_len - 2; i++) {
        if (data_type == 0) {
            LOGD("%s, msg data[%d] = 0x%02X\n", func_name, i, msg->msg_data[i]);
        } else {
            LOGD("%s, msg data[%d] = %c\n", func_name, i, msg->msg_data[i]);
        }
    }
#endif
    return 0;
}

int CHDMIRxCEC::ClrReplyListItem(HDMIRxRequestReplyItem *reply_item)
{
    if (reply_item == NULL) {
        return -1;
    }

    reply_item->WaitCmd = 0;
    reply_item->WaitLogicAddr = 0;
    reply_item->WaitTimeOut = 0;
    reply_item->WaitFlag = 0;
    reply_item->DataFlag = 0;
    memset((void *) & (reply_item->msg), 0, sizeof(struct _cec_msg));

    return 0;
}

int CHDMIRxCEC::CopyMessageData(unsigned char data_buf[], unsigned char msg_data[], int msg_len)
{
    if (data_buf == NULL) {
        return 0;
    }

    memset((void *)data_buf, 0, CC_CEC_STREAM_SIZE);

    if (msg_len > (int)CC_CEC_STREAM_SIZE) {
        return 0;
    }

    if (msg_len <= 2) {
        return 0;
    }

    msg_len -= 2;

    memcpy(data_buf, msg_data, msg_len);

    return msg_len;
}

int CHDMIRxCEC::GetDeviceLogicAddr(int source_input)
{
    return mSourceDevLogicAddrBuf[source_input - SOURCE_HDMI1];
}

int CHDMIRxCEC::processRefreshSrcDevice(int source_input)
{
    int i = 0, physical_addr = 0;
    int source_port_1 = 0, source_port_2 = 0;

    if (source_input != SOURCE_HDMI1 && source_input != SOURCE_HDMI2 && source_input != SOURCE_HDMI3) {
        return -1;
    }

    mSourceDevLogicAddrBuf[0] = E_LA_TV;
    mSourceDevLogicAddrBuf[1] = E_LA_TV;
    mSourceDevLogicAddrBuf[2] = E_LA_TV;

    for (i = 0; i < CC_SOURCE_DEV_REFRESH_CNT; i++) {
        if (mSourceDevRefreshBuf[i] < 0) {
            continue;
        }

        physical_addr = 0;
        if (SendGivePhysicalAddressMessage(source_input, mSourceDevRefreshBuf[i], &physical_addr) > 0) {
            source_port_1 = CTvin::Tvin_GetSourcePortBySourceInput((tv_source_input_t)source_input);
            source_port_2 = CTvin::Tvin_GetSourcePortByCECPhysicalAddress(physical_addr);
            if (source_port_1 == source_port_2) {
                mSourceDevLogicAddrBuf[source_input - SOURCE_HDMI1] = mSourceDevRefreshBuf[i];
                break;
            }
        }
    }

    if (i == CC_SOURCE_DEV_REFRESH_CNT) {
        return -1;
    }

    return 0;
}

int CHDMIRxCEC::ClearRxMessageBuffer()
{
    int m_cec_dev_fd = -1;

    m_cec_dev_fd = open(CS_HDMIRX_CEC_PATH, O_RDWR);
    if (m_cec_dev_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, CS_HDMIRX_CEC_PATH, strerror ( errno ));
        return -1;
    }

    ioctl(m_cec_dev_fd, HDMI_IOC_CEC_CLEAR_BUFFER, NULL);

    close(m_cec_dev_fd);
    m_cec_dev_fd = -1;

    return 0;
}

int CHDMIRxCEC::GetMessage(struct _cec_msg msg_buf[])
{
    int i = 0;
    int m_cec_dev_fd = -1;
    int msg_cnt = 0;

    if (msg_buf == NULL) {
        LOGE("%s, msg_buf is NULL\n", __FUNCTION__);
    }

    m_cec_dev_fd = open(CS_HDMIRX_CEC_PATH, O_RDWR);
    if (m_cec_dev_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, CS_HDMIRX_CEC_PATH, strerror ( errno ));
        return 0;
    }

    ioctl(m_cec_dev_fd, HDMI_IOC_CEC_GET_MSG_CNT, &msg_cnt);
    if (msg_cnt > 0) {
        LOGD("%s, msg_cnt = %d\n", __FUNCTION__, msg_cnt);

        for (i = 0; i < msg_cnt; i++) {
            ioctl(m_cec_dev_fd, HDMI_IOC_CEC_GET_MSG, &msg_buf[i]);
            PrintMessage(__FUNCTION__, 0, &msg_buf[i]);
        }
    }

    close(m_cec_dev_fd);
    m_cec_dev_fd = -1;

    return msg_cnt;
}


int CHDMIRxCEC::SendMessage(struct _cec_msg *msg)
{
    int m_cec_dev_fd = -1;

    if (msg == NULL) {
        LOGE("%s, msg is NULL\n", __FUNCTION__);
    }

    PrintMessage(__FUNCTION__, 0, msg);

    m_cec_dev_fd = open(CS_HDMIRX_CEC_PATH, O_RDWR);
    if (m_cec_dev_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, CS_HDMIRX_CEC_PATH, strerror ( errno ));
        return -1;
    }

    ioctl(m_cec_dev_fd, HDMI_IOC_CEC_SENT_MSG, msg);

    close(m_cec_dev_fd);
    m_cec_dev_fd = -1;

    return 0;
}

int CHDMIRxCEC::sendMessageAndWaitReply(struct _cec_msg *msg, struct _cec_msg *reply_msg, int WaitCmd, int timeout)
{
    int tmp_ret = 0, tmp_ind = 0;

    tmp_ret = SendMessage(msg);
    if (tmp_ret < 0) {
        return tmp_ret;
    }

    reply_msg->msg_len = 0;

    HDMIRxRequestReplyItem reply_item;
    reply_item.WaitCmd = WaitCmd;
    reply_item.WaitLogicAddr = msg->addr;
    reply_item.WaitTimeOut = timeout;
    reply_item.WaitFlag = 1;
    reply_item.DataFlag = 0;

    tmp_ind = addToRequestList(&reply_item);

    mReplyLock[tmp_ind].lock();
    mReplyList[tmp_ind].WaitReplyCondition.waitRelative(mReplyLock[tmp_ind], timeout);//wait reply
    mReplyLock[tmp_ind].unlock();

    if (mReplyList[tmp_ind].DataFlag == 1) {
        PrintMessage(__FUNCTION__, 0, &mReplyList[tmp_ind].msg);
        *reply_msg = mReplyList[tmp_ind].msg;
    } else {
        rmFromRequestList(tmp_ind);
        return -1;
    }

    rmFromRequestList(tmp_ind);

    return 0;
}

int CHDMIRxCEC::SendCustomMessage(int source_input, unsigned char data_buf[])
{
    CECMsgStream msg_stream;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    memcpy((void *)msg_stream.buf, data_buf, CC_CEC_STREAM_SIZE);
    if (msg_stream.msg.msg_len > CC_CEC_STREAM_SIZE) {
        return -1;
    }

    return SendMessage(&msg_stream.msg);
}

int CHDMIRxCEC::SendCustomMessageAndWaitReply(int source_input, unsigned char data_buf[], unsigned char reply_buf[], int WaitCmd, int timeout)
{
    CECMsgStream msg_stream;
    struct _cec_msg reply_msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    memcpy((void *)msg_stream.buf, data_buf, CC_CEC_STREAM_SIZE);
    if (msg_stream.msg.msg_len > CC_CEC_STREAM_SIZE) {
        return -1;
    }

    if (sendMessageAndWaitReply(&msg_stream.msg, &reply_msg, WaitCmd, timeout) < 0) {
        return -1;
    }

    return CopyMessageData(reply_buf, reply_msg.msg_data, reply_msg.msg_len);
}

int CHDMIRxCEC::SendBoradcastStandbyMessage(int source_input)
{
    struct _cec_msg msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    msg.addr = GetDeviceLogicAddr(source_input);
    msg.cmd = E_MSG_STANDBY;
    msg.msg_data[0] = 0;
    msg.msg_len = 2;

    return SendMessage(&msg);
}

int CHDMIRxCEC::SendGiveCECVersionMessage(int source_input, unsigned char data_buf[])
{
    struct _cec_msg msg, reply_msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    if (data_buf == NULL) {
        return -1;
    }

    msg.addr = GetDeviceLogicAddr(source_input);
    msg.cmd = E_MSG_GET_CEC_VERSION;
    msg.msg_data[0] = 0;
    msg.msg_len = 2;

    if (sendMessageAndWaitReply(&msg, &reply_msg, E_MSG_CEC_VERSION, 2000) < 0) {
        return -1;
    }

    return CopyMessageData(data_buf, reply_msg.msg_data, reply_msg.msg_len);
}

int CHDMIRxCEC::SendGiveDeviceVendorIDMessage(int source_input, unsigned char data_buf[])
{
    struct _cec_msg msg, reply_msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    if (data_buf == NULL) {
        return -1;
    }

    msg.addr = GetDeviceLogicAddr(source_input);
    msg.cmd = E_MSG_GIVE_DEVICE_VENDOR_ID;
    msg.msg_data[0] = 0;
    msg.msg_len = 2;

    if (sendMessageAndWaitReply(&msg, &reply_msg, E_MSG_DEVICE_VENDOR_ID, 2000) < 0) {
        return -1;
    }

    return CopyMessageData(data_buf, reply_msg.msg_data, reply_msg.msg_len);
}

int CHDMIRxCEC::SendGiveOSDNameMessage(int source_input, unsigned char data_buf[])
{
    struct _cec_msg msg, reply_msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    if (data_buf == NULL) {
        return -1;
    }

    msg.addr = GetDeviceLogicAddr(source_input);
    msg.cmd = E_MSG_OSDNT_GIVE_OSD_NAME;
    msg.msg_data[0] = 0;
    msg.msg_len = 2;

    if (sendMessageAndWaitReply(&msg, &reply_msg, E_MSG_OSDNT_SET_OSD_NAME, 2000) < 0) {
        return -1;
    }

    return CopyMessageData(data_buf, reply_msg.msg_data, reply_msg.msg_len);
}

int CHDMIRxCEC::SendGivePhysicalAddressMessage(int source_input, int logic_addr, int *physical_addr)
{
    struct _cec_msg msg, reply_msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    if (physical_addr == NULL) {
        return -1;
    }

    msg.addr = logic_addr;
    msg.cmd = E_MSG_GIVE_PHYSICAL_ADDRESS;
    msg.msg_data[0] = 0;
    msg.msg_len = 2;

    if (sendMessageAndWaitReply(&msg, &reply_msg, E_MSG_REPORT_PHYSICAL_ADDRESS, 2000) < 0) {
        return -1;
    }

    if (reply_msg.msg_len == 5) {
        *physical_addr = 0;
        *physical_addr |= reply_msg.msg_data[0] << 8;
        *physical_addr |= reply_msg.msg_data[1];
        return reply_msg.msg_len;
    }

    return -1;
}

int CHDMIRxCEC::SendSetMenuLanguageMessage(int source_input, unsigned char data_buf[])
{
    struct _cec_msg msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    if (data_buf == NULL) {
        return -1;
    }

    msg.addr = GetDeviceLogicAddr(source_input);
    msg.cmd = E_MSG_SET_MENU_LANGUAGE;
    msg.msg_data[0] = data_buf[0];
    msg.msg_data[1] = data_buf[1];
    msg.msg_data[2] = data_buf[2];
    msg.msg_len = 5;

    return SendMessage(&msg);
}

int CHDMIRxCEC::SendVendorRemoteKeyDownMessage(int source_input, unsigned char key_val)
{
    struct _cec_msg msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    msg.addr = GetDeviceLogicAddr(source_input);
    msg.cmd = E_MSG_VENDOR_RC_BUT_DOWN;
    msg.msg_data[0] = key_val;
    msg.msg_len = 3;

    return SendMessage(&msg);
}

int CHDMIRxCEC::SendVendorRemoteKeyUpMessage(int source_input)
{
    struct _cec_msg msg;

    if (isAllowOperate(source_input) < 0) {
        return -1;
    }

    msg.addr = GetDeviceLogicAddr(source_input);
    msg.cmd = E_MSG_VENDOR_RC_BUT_UP;
    msg.msg_data[0] = 0;
    msg.msg_len = 2;

    return SendMessage(&msg);
}

int CHDMIRxCEC::rmFromRequestList(int index)
{
    mListLock.lock();

    ClrReplyListItem(&mReplyList[index]);

    mListLock.unlock();

    return 0;
}

int CHDMIRxCEC::addToRequestList(HDMIRxRequestReplyItem *reply_item)
{
    int i = 0;

    mListLock.lock();

    for (i = 0; i < CC_REQUEST_LIST_SIZE; i++) {
        if (mReplyList[i].WaitFlag == 0) {
            mReplyList[i] = *reply_item;
            mListLock.unlock();
            return i;
        }
    }

    mListLock.unlock();

    return 0;
}

int CHDMIRxCEC::processData(int msg_cnt)
{
    int i = 0, j = 0;
    CECMsgStream msg_stream;

    for (i = 0; i < msg_cnt; i++) {
        for (j = 0; j < CC_REQUEST_LIST_SIZE; j++) {
            if (mReplyList[j].WaitFlag) {
                if (mMsgBuf[i].cmd == mReplyList[j].WaitCmd && ((mMsgBuf[i].addr & 0xF0) >> 4) == mReplyList[j].WaitLogicAddr) {
                    mReplyList[j].DataFlag = 1;
                    mReplyList[j].msg = mMsgBuf[i];
                    PrintMessage(__FUNCTION__, 0, &mReplyList[j].msg);
                    mReplyList[j].WaitReplyCondition.signal();

                    return 0;
                }
            }
        }

        msg_stream.msg = mMsgBuf[i];
        if (mpObserver != NULL) {
            mpObserver->onHDMIRxCECMessage(msg_stream.msg.msg_len, msg_stream.buf);
        }
    }

    return 0;
}

bool CHDMIRxCEC::threadLoop()
{
    int msg_cnt = 0;
    int sleeptime = 200; //ms

    mState = STATE_RUNNING;

    while (!exitPending()) { //requietexit() or requietexitWait() not call
        while (mRequestPause) {
            mLock.lock();
            mState = STATE_PAUSE;
            mPauseCondition.wait(mLock); //first unlock,when return,lock again,so need,call unlock
            mState = STATE_RUNNING;
            mLock.unlock();
        }

        msg_cnt = GetMessage(mMsgBuf);

        processData(msg_cnt);

        if (!mRequestPause) {
            usleep(sleeptime * 1000);
        }
    }

    mState = STATE_STOPED;
    //exit
    //return true, run again, return false,not run.
    return false;
}

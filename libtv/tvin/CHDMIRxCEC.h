#ifndef _C_HDMI_RX_CEC_H_
#define _C_HDMI_RX_CEC_H_

#ifdef __cplusplus
//extern "C" {
#endif

#include "CTvin.h"
#include <pthread.h>
//#include <cm.h>
//#include <ve.h>
#include "../tvutils/CThread.h"
#include <hdmirx_cec.h>

class CHDMIRxCEC: public CThread {
public:
    CHDMIRxCEC();
    ~CHDMIRxCEC();

    class IHDMIRxCECObserver {
    public:
        IHDMIRxCECObserver() {};
        virtual ~IHDMIRxCECObserver() {};
        virtual void onHDMIRxCECMessage(int msg_len __unused, unsigned char msg_buf[] __unused) {
        };
    };

    void setObserver ( IHDMIRxCECObserver *pOb ) {
        mpObserver = pOb;
    };

    int start();
    int stop();
    int pause();
    int resume();
    int ClearRxMessageBuffer();
    int processRefreshSrcDevice(int source_input);
    int SendCustomMessage(int source_input, unsigned char data_buf[]);
    int SendCustomMessageAndWaitReply(int source_input, unsigned char data_buf[], unsigned char reply_buf[], int WaitCmd, int timeout);
    int SendBoradcastStandbyMessage(int source_input);
    int SendGiveCECVersionMessage(int source_input, unsigned char data_buf[]);
    int SendGiveDeviceVendorIDMessage(int source_input, unsigned char data_buf[]);
    int SendGiveOSDNameMessage(int source_input, unsigned char data_buf[]);
    int SendGivePhysicalAddressMessage(int source_input, int logic_addr, int *physical_addr);
    int SendSetMenuLanguageMessage(int source_input, unsigned char data_buf[]);
    int SendVendorRemoteKeyDownMessage(int source_input, unsigned char key_val);
    int SendVendorRemoteKeyUpMessage(int source_input);


private:
    int mModuleEnableFlag;
    int mRequestPause;
    int mState;
    IHDMIRxCECObserver *mpObserver;

    bool threadLoop();
    int processData(int msg_cnt);

    int isAllowOperate(int source_input);
    int GetModuleEnableFlag();
    int GetDeviceLogicAddr(int source_input);
    int SendMessage(struct _cec_msg *msg);
    int sendMessageAndWaitReply(struct _cec_msg *msg, struct _cec_msg *reply_msg, int WaitCmd, int timeout);
    int GetMessage(struct _cec_msg *msg_list);
    int rmFromRequestList(int index);
    int addToRequestList(HDMIRxRequestReplyItem *reply_item);
    int PrintMessage(const char *func_name, int data_type, struct _cec_msg *msg);
    int ClrReplyListItem(HDMIRxRequestReplyItem *reply_item);
    int CopyMessageData(unsigned char data_buf[], unsigned char msg_data[], int msg_len);

    int mSourceDevLogicAddrBuf[CC_SOURCE_DEV_REFRESH_CNT];
    int mSourceDevRefreshBuf[CC_SOURCE_DEV_REFRESH_CNT];

    CCondition mPauseCondition;
    mutable CMutex mLock;
    mutable CMutex mListLock;
    mutable CMutex mReplyLock[CC_REQUEST_LIST_SIZE];
    HDMIRxRequestReplyItem mReplyList[CC_REQUEST_LIST_SIZE];
    struct _cec_msg mMsgBuf[CC_REQUEST_LIST_SIZE];

    enum RefreshState {
        STATE_STOPED = 0,
        STATE_RUNNING,
        STATE_PAUSE,
        STATE_FINISHED,
    };
};

#ifdef __cplusplus
//}
#endif

#endif/*_C_HDMI_RX_CEC_H_*/


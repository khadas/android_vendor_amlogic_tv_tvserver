#ifndef __AUDIO_CUSTOMER_CTRL_H__
#define __AUDIO_CUSTOMER_CTRL_H__

typedef int (*TYPE_DacMute_FUN)(int mute_state);
typedef int (*TYPE_DacMainVolume_FUN)(int vol);
typedef int (*TYPE_DacBalance_FUN)(int balance_val);
typedef int (*TYPE_AudioSourceType_FUN)(int source_type);

class CAudioCustomerCtrl {
public:
    static const int MUTE = 0;
    static const int UNMUTE = 1;

    CAudioCustomerCtrl();
    ~CAudioCustomerCtrl();
    int LoadExternalDacLib(void);
    int UnLoadExternalDacLib(void);
    int SendCmdToOffBoardCustomerLibExternalDac(int, int);
    int SetMute(int mute);
    int SetVolumeBar(int vol);
    int SetBlance(int balance);
    int SetSource(int source);

private:
    void *gExternalDacLibHandler;
    TYPE_DacMute_FUN mpDacMuteFun;
    TYPE_DacMainVolume_FUN mpDacMainVolFun;
    TYPE_DacBalance_FUN mpDacBalanceFun;
    TYPE_AudioSourceType_FUN mpSetSourceTypeFun;
};
#endif

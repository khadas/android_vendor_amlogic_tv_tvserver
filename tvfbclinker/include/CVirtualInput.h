#ifndef _VIRTUAL_INPUT_H
#define _VIRTUAL_INPUT_H
#include <linux/input.h>
#include <linux/uinput.h>

#define DEV_UINPUT         "/dev/uinput"

class CVirtualInput {
public:
    CVirtualInput();
    ~CVirtualInput();
    void sendVirtualkeyEvent(__u16 key_code);

private:
    bool  threadLoop();
    int setup_uinput_device();
    int uinp_fd;
};
#endif

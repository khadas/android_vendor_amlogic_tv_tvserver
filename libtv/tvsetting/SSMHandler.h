

#ifndef __SSM_HANDLER_H__
#define __SSM_HANDLER_H__

#include <string>
#include <utils/threads.h>
#include "CBlobDeviceFile.h"
#include "SSMHeader.h"

typedef enum
{
    SSM_HEADER_INVALID = -1,
    SSM_HEADER_VALID = 0,
    SSM_HEADER_STRUCT_CHANGE = 1,
}SSM_status_t;

class SSMHandler
{
public:
    static CBlobDevice *mSSMHeaderFile;
    static SSMHandler* GetSingletonInstance();
    virtual ~SSMHandler();
    SSM_status_t SSMVerify();
    bool SSMResetX(unsigned int);
    bool SSMRecreateHeader();
    bool SSMRecovery();
    unsigned int SSMGetActualAddr(int);
    unsigned int SSMGetActualSize(int);
    const std::string& getCfgPath(bool isDefault) const;
private:
    int mFd;
    const std::string mCfgPath = "SSMHandler_Path";
    const std::string mDefCfgPath = "/param/SSMHandler";
    static SSMHandler *mSSMHandler;
    SSMHeader_section1_t mSSMHeader_section1;
    static android::Mutex sLock;

    bool Construct();
    explicit SSMHandler();
    SSM_status_t SSMSection1Verify();
    SSM_status_t  SSMSection2Verify(SSM_status_t);
    SSMHandler(const SSMHandler&);
    SSMHandler(SSMHandler&);
    SSMHandler& operator = (const SSMHandler&);
public:
    int VPPSSMRestoreDefault(int id = -1, bool resetAll = true);
    int AudioSSMRestoreDefaultSetting();
};


#endif

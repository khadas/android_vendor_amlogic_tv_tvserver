#include "CFile.h"

static  const char   *CEC_PATH = "/dev/aocec";
class CHdmiCec: public CFile {
public:
    CHdmiCec();
    ~CHdmiCec();
    int readFile(unsigned char *pBuf, unsigned int uLen);
};

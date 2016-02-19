#include <tv/CFbcCommunication.h>
#include  "tv/CTvLog.h"

void usage(char *processname)
{
    fprintf(stderr, "Usage: %s <cmd num> [arg1]... [argn]\n", processname);
    return;
}


int main(int argc __unused, char **argv __unused)
{
    int cmd,  go = 1;
    LOGD("---------------0------------------------");
    CFbcCommunication fbc;
    LOGD("---------------1-------------------------");
    fbc.run();
    LOGD("------------------2----------------------");

    while (go) {
        scanf("%d", &cmd);
        switch (cmd) {
        case 1:
            go = 0;
            fbc.closeAll();
            break;
        case 2:
            break;
        default:
            break;
        }
    }
}

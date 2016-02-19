#define LOG_TAG "CTvRecord"

#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <am_debug.h>
#include <am_dmx.h>
#include <am_av.h>

#include <am_misc.h>

#include <am_fend.h>
#include <am_dvr.h>
#include <errno.h>
#include "CTvProgram.h"
#include "../tvconfig/tvconfig.h"
#include "CTvRecord.h"

#define FEND_DEV_NO 0
#define DVR_DEV_NO 0
#define DVR_BUF_SIZE 1024*1024
#define DVR_DEV_COUNT      (2)

typedef struct {
    int id;
    char file_name[256];
    pthread_t thread;
    int running;
    int fd;
} DVRData;

static DVRData data_threads[DVR_DEV_COUNT];
int pvr_init = 0;
CTvRecord::CTvRecord()
{
    AM_DVR_OpenPara_t dpara;
    memset(&dpara, 0, sizeof(dpara));
    AM_DVR_Open(DVR_DEV_NO, &dpara);
    data_threads[DVR_DEV_NO].id = 0;
    data_threads[DVR_DEV_NO].fd = -1;
    data_threads[DVR_DEV_NO].running = 0;

    AM_DVR_SetSource(DVR_DEV_NO, AM_DVR_SRC_ASYNC_FIFO0);
    AM_DVR_SetBufferSize(DVR_DEV_NO, DVR_BUF_SIZE);

    memset(filename, 0, sizeof(filename));
    progid = 0xFFFF;
    vpid = 0x1fff;
    apid = 0x1fff;
}
CTvRecord::~CTvRecord()
{
    AM_DVR_Close(DVR_DEV_NO);

}
void CTvRecord::dvr_init(void)
{
    AM_DVR_OpenPara_t para;
    char buf[32];

    if (pvr_init)
        return;

    memset(&para, 0, sizeof(para));
    LOGD("%s,%d", "TV", __LINE__);

    AM_DVR_Open(DVR_DEV_NO, &para);
    AM_DVR_SetSource(DVR_DEV_NO, AM_DVR_SRC_ASYNC_FIFO0);
    AM_DVR_SetBufferSize(DVR_DEV_NO, DVR_BUF_SIZE);

    snprintf(buf, sizeof(buf), "%d", (512 * 1024));
    AM_FileEcho("/sys/class/dmx/asyncfifo_len", buf);

    pvr_init = 1;
}

char *CTvRecord::GetRecordFileName()
{
    return filename;
}
void CTvRecord::SetRecordFileName(char *name)
{
    strcpy(filename, name);
}
void CTvRecord::SetCurRecProgramId(int id)
{
    progid = id;
}

int CTvRecord::dvr_data_write(int fd, uint8_t *buf, int size)
{
    int ret;
    int left = size;
    uint8_t *p = buf;
    LOGD("%s,%d", "TV", __LINE__);

    while (left > 0) {
        ret = write(fd, p, left);
        if (ret == -1) {
            if (errno != EINTR) {
                LOGD("Write DVR data failed: %s", strerror(errno));
                break;
            }
            ret = 0;
        }

        left -= ret;
        p += ret;
    }

    return (size - left);
}
void *CTvRecord::dvr_data_thread(void *arg)
{
    DVRData *dd = (DVRData *)arg;
    int cnt;
    uint8_t buf[256 * 1024];

    LOGD("Data thread for DVR%d start ,record file will save to '%s'", dd->id, dd->file_name);
    LOGD("%s,%d", "TV", __LINE__);

    while (dd->running) {
        cnt = AM_DVR_Read(dd->id, buf, sizeof(buf), 1000);
        if (cnt <= 0) {
            LOGD("No data available from DVR%d", dd->id);
            usleep(200 * 1000);
            continue;
        }
        //AM_DEBUG(1, "read from DVR%d return %d bytes", dd->id, cnt);
        if (dd->fd != -1) {
            dvr_data_write(dd->fd, buf, cnt);
        }
    }

    if (dd->fd != -1) {
        close(dd->fd);
        dd->fd = -1;
    }
    LOGD("Data thread for DVR%d now exit", dd->id);

    return NULL;
}

void CTvRecord::start_data_thread(int dev_no)
{
    DVRData *dd = &data_threads[dev_no];

    if (dd->running)
        return;
    LOGD("%s,%d,dev=%d", "TV", __LINE__, dev_no);
    dd->fd = open(dd->file_name, O_TRUNC | O_WRONLY | O_CREAT, 0666);
    if (dd->fd == -1) {
        LOGD("Cannot open record file '%s' for DVR%d, %s", dd->file_name, dd->id, strerror(errno));
        return;
    }
    dd->running = 1;
    pthread_create(&dd->thread, NULL, dvr_data_thread, dd);
}
void CTvRecord::get_cur_program_pid(int progId)
{
    CTvProgram prog;
    int aindex;
    CTvProgram::Audio *pA;
    CTvProgram::Video *pV;
    int ret = CTvProgram::selectByID(progId, prog);
    if (ret != 0) return;

    LOGD("%s,%d", "TV", __LINE__);
    pV = prog.getVideo();
    if (pV != NULL) {
        setvpid(pV->getPID());
    }

    aindex = prog.getCurrAudioTrackIndex();
    if (-1 == aindex) { //db is default
        aindex = prog.getCurrentAudio(String8("eng"));
        if (aindex >= 0) {
            prog.setCurrAudioTrackIndex(progId, aindex);
        }
    }

    if (aindex >= 0) {
        pA = prog.getAudio(aindex);
        if (pA != NULL) {
            setapid(pA->getPID());
        }
    }

}
int CTvRecord::start_dvr()
{
    AM_DVR_StartRecPara_t spara;
    int pid_cnt;
    int pids[2];

    /**仅测试最多8个PID*/
    get_cur_program_pid(progid);
    pids[0] = getvpid();
    pids[1] = getapid();

    strcpy(data_threads[DVR_DEV_NO].file_name, GetRecordFileName());
    LOGD("%s,%d", "TV", __LINE__);
    //sprintf(data_threads[DVR_DEV_NO].file_name,"%s","/storage/external_storage/sda4/testdvr.ts");
    spara.pid_count = 2;
    memcpy(&spara.pids, pids, sizeof(pids));

    if (AM_DVR_StartRecord(DVR_DEV_NO, &spara) == AM_SUCCESS) {
        start_data_thread(DVR_DEV_NO);
    }

    return 0;
}
void CTvRecord::stop_data_thread(int dev_no)
{
    DVRData *dd = &data_threads[dev_no];
    LOGD("%s,%d", "TV", __LINE__);

    if (! dd->running)
        return;
    dd->running = 0;
    pthread_join(dd->thread, NULL);
    LOGD("Data thread for DVR%d has exit", dd->id);
}


void CTvRecord::StartRecord(int id)
{
    AM_DVR_OpenPara_t dpara;
    fe_status_t status;
    char buf[32];

    AM_FEND_GetStatus(FEND_DEV_NO, &status);

    if (status & FE_HAS_LOCK) {
        LOGD("locked\n");
    } else {
        LOGD("unlocked\n");
        return ;
    }
    SetCurRecProgramId(id);
    start_dvr();

    return;
}
void CTvRecord::StopRecord()
{
    int i = 0;
    LOGD("stop record for %d", DVR_DEV_NO);
    AM_DVR_StopRecord(DVR_DEV_NO);

    //for (i=0; i< DVR_DEV_COUNT; i++)
    {
        if (data_threads[DVR_DEV_NO].running)
            stop_data_thread(DVR_DEV_NO);
        //LOGD("Closing DMX%d...", i);
    }


}
void CTvRecord::SetRecCurTsOrCurProgram(int sel)
{
    int i = 0;
    char buf[50];
    memset(buf, 0, sizeof(buf));
    for (; i < 3; i++) {
        snprintf(buf, sizeof(buf), "/sys/class/stb/dvr%d_mode", i);
        if (sel)
            AM_FileEcho(buf, "ts");
        else
            AM_FileEcho(buf, "pid");
    }
}


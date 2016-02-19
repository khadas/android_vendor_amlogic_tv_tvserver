#define LOG_TAG "CSourceConnectDetect"

#include "CTvin.h"
#include <CTvLog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>

#include "../tvutils/tvutils.h"
#include "../tvconfig/tvconfig.h"

#include "CSourceConnectDetect.h"

CSourceConnectDetect::CSourceConnectDetect()
{
    mpObserver = NULL;
    if (mEpoll.create() < 0) {
        return;
    }
    //avin
    if (mAvinDetectFile.openFile(AVIN_DETECT_PATH) > 0) {
        m_event.data.fd = mAvinDetectFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mAvinDetectFile.getFd(), &m_event);
    }
    //HDMI
    if (mHdmiDetectFile.openFile(HDMI_DETECT_PATH) > 0) {
        m_event.data.fd = mHdmiDetectFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mHdmiDetectFile.getFd(), &m_event);
    }
    //vfame size change
    if (mVppPollFile.openFile(VPP_POLL_PATCH) > 0) {
        m_event.data.fd = mVppPollFile.getFd();
        m_event.events = EPOLLIN | EPOLLET;
        mEpoll.add(mVppPollFile.getFd(), &m_event);
    }
}

CSourceConnectDetect::~CSourceConnectDetect()
{
}

int CSourceConnectDetect::startDetect()
{
    this->run();

    return 0;
}

int CSourceConnectDetect::SourceInputMaptoChipHdmiPort(tv_source_input_t source_input)
{
    tvin_port_t source_port = TVIN_PORT_NULL;
    source_port = CTvin::getInstance()->Tvin_GetSourcePortBySourceInput(source_input);
    switch (source_port) {
    case TVIN_PORT_HDMI0:
        return HDMI_DETECT_STATUS_BIT_A;
        break;
    case TVIN_PORT_HDMI1:
        return HDMI_DETECT_STATUS_BIT_B;
        break;
    case TVIN_PORT_HDMI2:
        return HDMI_DETECT_STATUS_BIT_C;
        break;
    case TVIN_PORT_HDMI3:
        return HDMI_DETECT_STATUS_BIT_D;
        break;
    default:
        return HDMI_DETECT_STATUS_BIT_A;
        break;
    }

}

tv_source_input_t CSourceConnectDetect::ChipHdmiPortMaptoSourceInput(int port)
{
    switch (port) {
    case HDMI_DETECT_STATUS_BIT_A:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI0);
        break;
    case HDMI_DETECT_STATUS_BIT_B:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI1);
        break;
    case HDMI_DETECT_STATUS_BIT_C:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI2);
        break;
    case HDMI_DETECT_STATUS_BIT_D:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI3);
        break;
    default:
        return CTvin::getInstance()->Tvin_PortToSourceInput(TVIN_PORT_HDMI0);
        break;
    }
}

int CSourceConnectDetect::GetSourceConnectStatus(tv_source_input_t source_input)
{
    int PlugStatus = -1;
    int hdmi_status = 0;
    int source = -1;
    int HdmiDetectStatusBit = SourceInputMaptoChipHdmiPort((tv_source_input_t)source_input);
    switch (source_input) {
    case SOURCE_AV2:
    case SOURCE_AV1: {
        struct report_data_s status[2];
        mAvinDetectFile.readFile((void *)(&status), sizeof(struct report_data_s) * 2);
        for (int i = 0; i < 2; i++) {
            if (status[i].channel == AVIN_CHANNEL1) {
                source = SOURCE_AV1;
            } else if (status[i].channel == AVIN_CHANNEL2) {
                source = SOURCE_AV2;
            }

            if (source == source_input) {
                if (status[i].status == AVIN_STATUS_IN) {
                    PlugStatus = CC_SOURCE_PLUG_IN;
                } else {
                    PlugStatus = CC_SOURCE_PLUG_OUT;
                }
                break;
            }
            m_avin_status[i] = status[i];
        }//end for

        break;
    }
    case SOURCE_HDMI1:
    case SOURCE_HDMI2:
    case SOURCE_HDMI3: {
        mHdmiDetectFile.readFile((void *)(&hdmi_status), sizeof(int));
        if ((hdmi_status & HdmiDetectStatusBit) == HdmiDetectStatusBit) {
            PlugStatus = CC_SOURCE_PLUG_IN;
        } else {
            PlugStatus = CC_SOURCE_PLUG_OUT;
        }
        m_hdmi_status = hdmi_status;
        break;
    }
    default:
        LOGD("GetSourceConnectStatus not support  source!!!!!!!!!!!!!!!1");
        break;
    }

    return PlugStatus;
}

bool CSourceConnectDetect::threadLoop()
{
    if ( mpObserver == NULL ) {
        return false;
    }

    LOGD("%s, entering...\n", "TV");

    prctl(PR_SET_NAME, (unsigned long)"CSourceConnectDetect thread loop");
    //init status
    mHdmiDetectFile.readFile((void *)(&m_hdmi_status), sizeof(int));
    mAvinDetectFile.readFile((void *)(&m_avin_status), sizeof(struct report_data_s) * 2);
    LOGD("CSourceConnectDetect Loop, get init hdmi = 0x%x  avin[0].status = %d, avin[1].status = %d", m_hdmi_status, m_avin_status[0].status, m_avin_status[1].status);

    while (!exitPending()) { //requietexit() or requietexitWait() not call
        int num = mEpoll.wait();
        for (int i = 0; i < num; ++i) {
            int fd = (mEpoll)[i].data.fd;
            /**
             * EPOLLIN event
             */
            if ((mEpoll)[i].events & EPOLLIN) {
                if (fd == mAvinDetectFile.getFd()) {//avin
                    struct report_data_s status[2];
                    mAvinDetectFile.readFile((void *)(&status), sizeof(struct report_data_s) * 2);
                    for (int i = 0; i < 2; i++) {
                        int source = -1, plug = -1;
                        if (/*status[i].channel == m_avin_status[i].channel &&*/ status[i].status != m_avin_status[i].status) {
                            //LOGD("status[i].status != m_avin_status[i].status");
                            if (status[i].status == AVIN_STATUS_IN) {
                                plug = CC_SOURCE_PLUG_IN;
                            } else {
                                plug = CC_SOURCE_PLUG_OUT;
                            }

                            if (status[i].channel == AVIN_CHANNEL1) {
                                source = SOURCE_AV1;
                            } else if (status[i].channel == AVIN_CHANNEL2) {
                                source = SOURCE_AV2;
                            }

                            if (mpObserver != NULL) {
                                mpObserver->onSourceConnect(source, plug);
                            }
                        }//not equal
                        m_avin_status[i] = status[i];
                    }
                } else if (fd == mHdmiDetectFile.getFd()) { //hdmi
                    int hdmi_status = 0;
                    mHdmiDetectFile.readFile((void *)(&hdmi_status), sizeof(int));
                    int source = -1, plug = -1;
                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_A) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_A) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_A) == HDMI_DETECT_STATUS_BIT_A) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_A);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_A);;
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }

                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_B) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_B) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_B) == HDMI_DETECT_STATUS_BIT_B) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_B);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_B);;
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }

                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_C) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_C) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_C) == HDMI_DETECT_STATUS_BIT_C) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_C);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_C);;
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }

                    if ((hdmi_status & HDMI_DETECT_STATUS_BIT_D) != (m_hdmi_status  & HDMI_DETECT_STATUS_BIT_D) ) {
                        if ((hdmi_status & HDMI_DETECT_STATUS_BIT_D) == HDMI_DETECT_STATUS_BIT_D) {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_D);
                            plug = CC_SOURCE_PLUG_IN;
                        } else {
                            source = ChipHdmiPortMaptoSourceInput(HDMI_DETECT_STATUS_BIT_D);;
                            plug = CC_SOURCE_PLUG_OUT;
                        }
                        mpObserver->onSourceConnect(source, plug);
                    }
                    m_hdmi_status = hdmi_status;
                } else if (fd == mVppPollFile.getFd()) { //vframe size change
                    mpObserver->onVframeSizeChange();
                }
                /**
                 * EPOLLOUT event
                 */
                if ((mEpoll)[i].events & EPOLLOUT) {

                }
            }
        }
    }//exit

    LOGD("%s, exiting...\n", "CSourceConnectDetect");
    //return true, run again, return false,not run.
    return false;
}

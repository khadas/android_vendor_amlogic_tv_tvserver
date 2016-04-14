
#include "CHDMIRxManager.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <CTvLog.h>


CHDMIRxManager::CHDMIRxManager()
{

}

CHDMIRxManager::~CHDMIRxManager()
{
}

int CHDMIRxManager::HdmiRxEdidUpdate()
{
    int m_hdmi_fd = -1;

    m_hdmi_fd = open(CS_HDMIRX_DEV_PATH, O_RDWR);
    if (m_hdmi_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, CS_HDMIRX_DEV_PATH, strerror ( errno ));
        return -1;
    }

    ioctl(m_hdmi_fd, HDMI_IOC_EDID_UPDATE, NULL);

    close(m_hdmi_fd);
    m_hdmi_fd = -1;

    return 0;
}

int CHDMIRxManager::HdmiRxHdcpOnOff(tv_hdmi_hdcpkey_enable_t flag)
{
    int m_hdmi_fd = -1;

    m_hdmi_fd = open(CS_HDMIRX_DEV_PATH, O_RDWR);
    if (m_hdmi_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, CS_HDMIRX_DEV_PATH, strerror ( errno ));
        return -1;
    }

    if (hdcpkey_enable == flag) {
        ioctl(m_hdmi_fd, HDMI_IOC_HDCP_ON, NULL);
    }else if (hdcpkey_enable == flag) {
        ioctl(m_hdmi_fd, HDMI_IOC_HDCP_OFF, NULL);
    }else {
        return -1;
    }

    close(m_hdmi_fd);
    m_hdmi_fd = -1;

    return 0;
}

int CHDMIRxManager::GetHdmiHdcpKeyKsvInfo(struct _hdcp_ksv *msg)
{
    int m_hdmi_fd = -1;

    m_hdmi_fd = open(CS_HDMIRX_DEV_PATH, O_RDWR);
    if (m_hdmi_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, CS_HDMIRX_DEV_PATH, strerror ( errno ));
        return -1;
    }

    ioctl(m_hdmi_fd, HDMI_IOC_HDCP_KSV, msg);

    close(m_hdmi_fd);
    m_hdmi_fd = -1;

    return 0;
}

int CHDMIRxManager::SetHdmiColorRangeMode(tv_hdmi_color_range_t range_mode)
{
    FILE *fp = NULL;
    int value = 0;
    fp = fopen(HDMI_FORCE_COLOR_RANGE, "w");
    if (fp == NULL) {
        LOGE ( "Open %s (%s)!\n", HDMI_FORCE_COLOR_RANGE, strerror(errno));
        return -1;
    }
    fprintf(fp, "%d", (int)range_mode);
    fclose(fp);
    fp = NULL;
    return value;
}

tv_hdmi_color_range_t CHDMIRxManager::GetHdmiColorRangeMode()
{
    FILE *fp = NULL;
    int value = 0;
    fp = fopen(HDMI_FORCE_COLOR_RANGE, "r");
    if (fp == NULL) {
        LOGE ( "Open %s (%s)!\n", HDMI_FORCE_COLOR_RANGE, strerror(errno));
        return AUTO_RANGE;
    }
    fscanf(fp, "%d", &value);
    fclose(fp);
    fp = NULL;
    return (tv_hdmi_color_range_t)value;
}


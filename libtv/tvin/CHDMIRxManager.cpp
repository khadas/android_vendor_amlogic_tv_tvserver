#define LOG_TAG "tvserver"
#define LOG_TV_TAG "CHDMIRxManager"

#include "CHDMIRxManager.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <CTvLog.h>
#include <tvutils.h>


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
    ioctl(m_hdmi_fd, HDMI_IOC_EDID_UPDATE, 1);
    close(m_hdmi_fd);
    m_hdmi_fd = -1;
    return 0;
}

int CHDMIRxManager::HdmiRxHdcpVerSwitch(tv_hdmi_hdcp_version_t version)
{
    int m_hdmi_fd = -1;
    m_hdmi_fd = open(CS_HDMIRX_DEV_PATH, O_RDWR);
    if (m_hdmi_fd < 0) {
        LOGE("%s, Open file %s error: (%s)!\n", __FUNCTION__, CS_HDMIRX_DEV_PATH, strerror ( errno ));
        return -1;
    }
    if (HDMI_HDCP_VER_14 == version) {
        ioctl(m_hdmi_fd, HDMI_IOC_HDCP22_FORCE14, NULL);
    } else if (HDMI_HDCP_VER_22 == version) {
        ioctl(m_hdmi_fd, HDMI_IOC_HDCP22_AUTO, NULL);
    } else {
        close(m_hdmi_fd);
        m_hdmi_fd = -1;
        return -1;
    }
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

    ioctl(m_hdmi_fd, HDMI_IOC_HDCP_GET_KSV, msg);

    close(m_hdmi_fd);
    m_hdmi_fd = -1;

    return 0;
}

int CHDMIRxManager::SetHdmiColorRangeMode(tv_hdmi_color_range_t range_mode)
{
    char val[5] = {0};
    sprintf(val, "%d", range_mode);
    tvWriteSysfs(HDMI_FORCE_COLOR_RANGE, val);
    return 0;
}

tv_hdmi_color_range_t CHDMIRxManager::GetHdmiColorRangeMode()
{
    char value[5] = {0};
    tvReadSysfs(HDMI_FORCE_COLOR_RANGE, value);

    return (tv_hdmi_color_range_t)atoi(value);
}


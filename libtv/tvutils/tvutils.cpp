#define LOG_TAG "TVutils"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <android/log.h>
#include <cutils/android_reboot.h>
#include "../tvconfig/tvconfig.h"
#include "../tvsetting/CTvSetting.h"
#include <cutils/properties.h>
#include <dirent.h>
using namespace android;

#include "tvutils.h"
#include "CTvLog.h"

#define CS_I2C_1_DEV_PATH "/dev/i2c-1"
#define CS_I2C_2_DEV_PATH "/dev/i2c-2"

/* number of times a device address should be polled when not acknowledging */
#define I2C_RETRIES             0x0701

/* set timeout in units of 10 ms */
#define I2C_TIMEOUT             0x0702

/* NOTE: Slave address is 7 or 10 bits, but 10-bit addresses
 * are NOT supported! (due to code brokenness)
 */

/* Use this slave address */
#define I2C_SLAVE               0x0703

/* Use this slave address, even if it is already in use by a driver! */
#define I2C_SLAVE_FORCE         0x0706

#define I2C_TENBIT              0x0704  /* 0 for 7 bit addrs, != 0 for 10 bit */
#define I2C_FUNCS               0x0705  /* Get the adapter functionality mask */
#define I2C_RDWR                0x0707  /* Combined R/W transfer (one STOP only) */
#define I2C_PEC                 0x0708  /* != 0 to use PEC with SMBus */
#define I2C_SMBUS               0x0720  /* SMBus transfer */

struct i2c_msg {
    unsigned short addr; /* slave address */
    unsigned short flags;
#define I2C_M_TEN           0x0010  /* this is a ten bit chip address */
#define I2C_M_WR            0x0000  /* write data, from master to slave */
#define I2C_M_RD            0x0001  /* read data, from slave to master */
#define I2C_M_NOSTART       0x4000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_REV_DIR_ADDR  0x2000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_IGNORE_NAK    0x1000  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_NO_RD_ACK     0x0800  /* if I2C_FUNC_PROTOCOL_MANGLING */
#define I2C_M_RECV_LEN      0x0400  /* length will be first received byte */

    unsigned short len; /* msg length */
    unsigned char *buf; /* pointer to msg data */
};

struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;
    unsigned int nmsgs;
};

static volatile int mDreamPanelResumeLastBLFlag = 0;
static volatile int mDreamPanelDemoFlag = 0;

static pthread_mutex_t dream_panel_resume_last_bl_flag_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t dream_panel_demo_flag_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t file_attr_control_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t UserPet_ThreadId = 0;
static unsigned char is_turnon_user_pet_thread = false;
static unsigned char is_user_pet_thread_start = false;
static unsigned int user_counter = 0;
static unsigned int user_pet_terminal = 1;

static int iw_duty = 0;

static int Miscioctl(const char *file_path, int request, ...)
{
    int fd = -1, tmp_ret = -1;
    int bus_status = 0;
    va_list ap;
    void *arg;

    if (file_path == NULL) {
        LOGE("%s, file path is NULL!!!\n", CFG_SECTION_TV);
        return -1;
    }

    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        LOGE("%s, Open %s ERROR(%s)!!\n", CFG_SECTION_TV, file_path, strerror(errno));
        return -1;
    }

    va_start(ap, request);
    arg = va_arg(ap, void *);
    va_end(ap);

    tmp_ret = ioctl(fd, request, arg);

    close(fd);
    fd = -1;

    return tmp_ret;
}

int cfg_get_one_item(const char *key_str, const char *strDelimit, int item_index, char cfg_str[])
{
    int cfg_item_ind = 0;
    char *token = NULL;
    const char *config_value;
    char data_str[CC_CFG_VALUE_STR_MAX_LEN] = { 0 };

    if (key_str == NULL) {
        LOGE("%s, key_str's pointer is NULL.\n", CFG_SECTION_TV);
        return -1;
    }

    if (cfg_str == NULL) {
        LOGE("%s, cfg_str's pointer is NULL.\n", CFG_SECTION_TV);
        return -1;
    }

    if (item_index < 0) {
        LOGE("%s, item_index can't be less than 0.\n", CFG_SECTION_TV);
        return -1;
    }

    config_value = config_get_str(CFG_SECTION_TV, key_str, "null");
    if (strcasecmp(config_value, "null") == 0) {
        cfg_str[0] = '\0';
        LOGE("%s, can't get config \"%s\"!!!\n", CFG_SECTION_TV, key_str);
        return -1;
    }

    cfg_item_ind = 0;

    memset((void *)data_str, 0, sizeof(data_str));
    strncpy(data_str, config_value, sizeof(data_str) - 1);

    token = strtok(data_str, strDelimit);
    while (token != NULL) {
        if (cfg_item_ind == item_index) {
            strcpy(cfg_str, token);
            break;
        }

        token = strtok(NULL, strDelimit);
        cfg_item_ind += 1;
    }

    if (token == NULL) {
        cfg_str[0] = '\0';
        return -1;
    }

    return 0;
}

int cfg_split(char *line_data, char *strDelimit, int *item_cnt, char **item_bufs)
{
    int i = 0, tmp_cnt = 0;
    char *token = NULL;

    if (line_data == NULL) {
        LOGE("%s, line_data is NULL", CFG_SECTION_TV);
        return -1;
    }

    if (strDelimit == NULL) {
        LOGE("%s, strDelimit is NULL", CFG_SECTION_TV);
        return -1;
    }

    if (item_cnt == NULL) {
        LOGE("%s, item_cnt is NULL", CFG_SECTION_TV);
        return -1;
    }

    if (item_bufs == NULL) {
        LOGE("%s, item_bufs is NULL", CFG_SECTION_TV);
        return -1;
    }

    for (i = 0; i < *item_cnt; i++) {
        item_bufs[i] = NULL;
    }

    token = strtok(line_data, strDelimit);

    while (token != NULL) {
        item_bufs[tmp_cnt] = token;

        token = strtok(NULL, strDelimit);

        tmp_cnt += 1;
        if (tmp_cnt >= *item_cnt) {
            break;
        }
    }

    *item_cnt = tmp_cnt;

    return 0;
}

int ReadADCSpecialChannelValue(int adc_channel_num)
{
    FILE *fp = NULL;
    int rd_data = 0;
    char ch_sysfs_path[256] = { 0 };

    if (adc_channel_num < CC_MIN_ADC_CHANNEL_VAL || adc_channel_num > CC_MAX_ADC_CHANNEL_VAL) {
        LOGD("adc channel num must between %d and %d.", CC_MIN_ADC_CHANNEL_VAL, CC_MAX_ADC_CHANNEL_VAL);
        return 0;
    }

    sprintf(ch_sysfs_path, "/sys/class/saradc/saradc_ch%d", adc_channel_num);

    fp = fopen(ch_sysfs_path, "r");

    if (fp == NULL) {
        LOGE("open %s ERROR(%s)!!\n", ch_sysfs_path, strerror(errno));
        return 0;
    }

    fscanf(fp, "%d", &rd_data);

    fclose(fp);

    return rd_data;
}

int Tv_MiscRegs(const char *cmd)
{
    //#ifdef BRING_UP_DEBUG
    FILE *fp = NULL;
    fp = fopen("/sys/class/register/reg", "w");

    if (fp != NULL && cmd != NULL) {
        fprintf(fp, "%s", cmd);
    } else {
        LOGE("Open /sys/class/register/reg ERROR(%s)!!\n", strerror(errno));
        fclose(fp);
        return -1;
    }
    fclose(fp);
    //#endif

    return 0;
}

int TvMisc_SetLVDSSSC(int val)
{
    FILE *fp;

    fp = fopen("/sys/class/lcd/ss", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", val);
        fclose(fp);
    } else {
        LOGE("open /sys/class/lcd/ss ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
    return 0;
}

int TvMisc_SetUserCounterTimeOut(int timeout)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/user_pet_timeout", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", timeout);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/user_pet_timeout ERROR(%s)!!\n", strerror(errno));
        return -1;
    }
    return 0;
}

int TvMisc_SetUserCounter(int count)
{
    FILE *fp;

    fp = fopen("/sys/module/aml_wdt/parameters/user_pet", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", count);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/user_pet ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetUserPetResetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/module/aml_wdt/parameters/user_pet_reset_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/user_pet_reset_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetSystemPetResetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/reset_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/reset_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetSystemPetEnable(int enable)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/ping_enable", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", enable);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/ping_enable ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

int TvMisc_SetSystemPetCounterTimeOut(int timeout)
{
    FILE *fp;

    fp = fopen("/sys/devices/platform/aml_wdt/wdt_timeout", "w");

    if (fp != NULL) {
        fprintf(fp, "%d", timeout);
        fclose(fp);
    } else {
        LOGE("=OSD CPP=> open /sys/devices/platform/aml_wdt/wdt_timeout ERROR(%s)!!\n", strerror(errno));
        return -1;
    }

    fclose(fp);

    return 0;
}

#define MEMERASE _IOW('M', 2, struct erase_info_user)
static int memerase(int fd, struct erase_info_user *erase)
{
    return (ioctl (fd, MEMERASE, erase));
}

#define CS_ATV_SOCKET_FILE_NAME "/dev/socket/datv_sock"

static int setServer(const char *fileName __unused)
{
    int ret = -1, sock = -1;
    struct sockaddr_un srv_addr;

    sock = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("%s, socket create failed (errno = %d: %s).\n", CFG_SECTION_TV, errno, strerror(errno));
        return -1;
    }

    //set server addr_param
    srv_addr.sun_family = AF_UNIX;
    strncpy(srv_addr.sun_path, CS_ATV_SOCKET_FILE_NAME, sizeof(srv_addr.sun_path) - 1);
    unlink(CS_ATV_SOCKET_FILE_NAME);

    //bind sockfd & addr
    ret = bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    if (ret == -1) {
        LOGE("%s, cannot bind server socket.\n", CFG_SECTION_TV);
        close(sock);
        unlink(CS_ATV_SOCKET_FILE_NAME);
        return -1;
    }

    //listen sockfd
    ret = listen(sock, 1);
    if (ret == -1) {
        LOGE("%s, cannot listen the client connect request.\n", CFG_SECTION_TV);
        close(sock);
        unlink(CS_ATV_SOCKET_FILE_NAME);
        return -1;
    }

    return sock;
}

static int acceptMessage(int listen_fd)
{
    int ret, com_fd;
    socklen_t len;
    struct sockaddr_un clt_addr;

    //have connect request use accept
    len = sizeof(clt_addr);
    com_fd = accept(listen_fd, (struct sockaddr *) &clt_addr, &len);
    if (com_fd < 0) {
        LOGE("%s, cannot accept client connect request.\n", CFG_SECTION_TV);
        close(listen_fd);
        unlink(CS_ATV_SOCKET_FILE_NAME);
        return -1;
    }

    LOGD("%s, com_fd = %d\n", CFG_SECTION_TV, com_fd);

    return com_fd;
}

static int parse_socket_message(char *msg_str, int *para_cnt, int para_buf[])
{
    int para_count = 0, set_mode = 0;
    char *token = NULL;

    set_mode = -1;

    token = strtok(msg_str, ",");
    if (token != NULL) {
        if (strcasecmp(token, "quit") == 0) {
            set_mode = 0;
        } else if (strcasecmp(token, "SetAudioVolumeCompensationVal") == 0) {
            set_mode = 1;
        } else if (strcasecmp(token, "set3dmode") == 0) {
            set_mode = 2;
        } else if (strcasecmp(token, "setdisplaymode") == 0) {
            set_mode = 3;
        }
    }

    if (set_mode != 1 && set_mode != 2 && set_mode != 3) {
        return set_mode;
    }

    para_count = 0;

    token = strtok(NULL, ",");
    while (token != NULL) {
        para_buf[para_count] = strtol(token, NULL, 10);
        para_count += 1;

        token = strtok(NULL, ",");
    }

    *para_cnt = para_count;

    return set_mode;
}

/*static void* socket_thread_entry(void *arg)
{
    int ret = 0, listen_fd = -1, com_fd = -1, rd_len = 0;
    int para_count = 0, set_mode = 0;
    int tmp_val = 0;
    int para_buf[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };
    static char recv_buf[1024];

    listen_fd = setServer(CS_ATV_SOCKET_FILE_NAME);
    chmod(CS_ATV_SOCKET_FILE_NAME, 0666);
    prctl(PR_SET_NAME, (unsigned long) "datv.sock.thread");

    if (listen_fd < 0) {
        return NULL;
    }

    while (1) {
        com_fd = acceptMessage(listen_fd);

        if (com_fd >= 0) {
            //read message from client
            memset((void *) recv_buf, 0, sizeof(recv_buf));
            rd_len = read(com_fd, recv_buf, sizeof(recv_buf));
            LOGD("%s, message from client (%d)) : %s\n", CFG_SECTION_TV, rd_len, recv_buf);

            set_mode = parse_socket_message(recv_buf, &para_count, para_buf);
            if (set_mode == 0) {
                LOGD("%s, receive quit message, starting to quit.\n", CFG_SECTION_TV);
                sprintf(recv_buf, "%s", "quiting now...");
                write(com_fd, recv_buf, strlen(recv_buf) + 1);
                break;
            } else if (set_mode == 1) {
                ret = -1;

                if (para_count == 1) {
                    LOGD("%s, SetAudioVolumeCompensationVal value = %d\n", CFG_SECTION_TV, para_buf[0]);

                    ret = SetAudioVolumeCompensationVal(para_buf[0]);
                } else if (para_count == 2) {
                    LOGD("%s, SetAudioVolumeCompensationVal value = %d, type = %d\n", CFG_SECTION_TV, para_buf[0], para_buf[1]);

                    ret = SetAudioVolumeCompensationVal(para_buf[0]);

                    if (para_buf[1] == 1) {
                        tmp_val = GetAudioMasterVolume();
                        ret |= SetAudioMasterVolume(tmp_val);
                    }
                }

                sprintf(recv_buf, "%d", ret);
                write(com_fd, recv_buf, strlen(recv_buf) + 1);
            } else if (set_mode == 2) {
                ret = -1;

                if (para_count == 1) {
                    LOGE("%s, mode = %d ------->\n", CFG_SECTION_TV, para_buf[0]);

                    switch (para_buf[0]) {
                    case 4: //BT
                        ret = Tvin_Set3DFunction(MODE3D_BT);
                        break;
                    case 5: //LR
                        ret = Tvin_Set3DFunction(MODE3D_LR);
                        break;
                    case 8: //
                        ret = Tvin_Set3DFunction(MODE3D_DISABLE);
                        break;
                    case 0:
                        ret = Tvin_Set3DFunction(MODE3D_DISABLE);
                        break;
                    case 9: //
                        ret = Tvin_Set3DFunction(MODE3D_L_3D_TO_2D);
                        break;
                    }
                    if (ret == 0) {
                        LOGE("%s, sk_hdi_av_set_3d_mode return sucess.\n", CFG_SECTION_TV);
                    } else {
                        LOGE("%s, sk_hdi_av_set_3d_mode return error(%d).\n", CFG_SECTION_TV, ret);
                    }
                }

                sprintf(recv_buf, "%d", ret);
                write(com_fd, recv_buf, strlen(recv_buf) + 1);

            }

            close(com_fd);
            com_fd = -1;
        }
    }

    if (com_fd >= 0) {
        close(com_fd);
        com_fd = -1;
    }

    if (listen_fd >= 0) {
        close(listen_fd);
        listen_fd = -1;
    }

    unlink(CS_ATV_SOCKET_FILE_NAME);

    return NULL;
}*/

#if !defined(SUN_LEN)
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

static int connectToServer(char *file_name)
{
    int tmp_ret = 0, sock = -1;
    struct sockaddr_un addr;

    if (file_name == NULL) {
        LOGE("%s, file name is NULL\n", CFG_SECTION_TV);
        return -1;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("%s, socket create failed (errno = %d: %s)\n", CFG_SECTION_TV, errno, strerror(errno));
        return -1;
    }

    /* connect to socket; fails if file doesn't exist */
    strcpy(addr.sun_path, file_name); // max 108 bytes
    addr.sun_family = AF_UNIX;
    tmp_ret = connect(sock, (struct sockaddr *) &addr, SUN_LEN(&addr));
    if (tmp_ret < 0) {
        // ENOENT means socket file doesn't exist
        // ECONNREFUSED means socket exists but nobody is listening
        LOGE("%s, AF_UNIX connect failed for '%s': %s\n", CFG_SECTION_TV, file_name, strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}

static int realSendSocketMsg(char *file_name, char *msg_str, char recv_buf[])
{
    int sock = -1, rd_len = 0;
    char tmp_buf[1024];

    if (file_name == NULL) {
        LOGE("%s, file name is NULL\n", CFG_SECTION_TV);
        return -1;
    }

    if (msg_str == NULL) {
        LOGE("%s, msg string is NULL\n", CFG_SECTION_TV);
        return -1;
    }

    LOGD("%s, message to server (%d)) : %s\n", CFG_SECTION_TV, strlen(msg_str), msg_str);

    sock = connectToServer(file_name);

    if (sock >= 0) {
        write(sock, msg_str, strlen(msg_str) + 1);

        if (recv_buf == NULL) {
            memset((void *) tmp_buf, 0, sizeof(tmp_buf));
            rd_len = read(sock, tmp_buf, sizeof(tmp_buf));
            LOGD("%s, message from server (%d)) : %s\n", CFG_SECTION_TV, rd_len, tmp_buf);
        } else {
            rd_len = read(sock, recv_buf, 1024);
            LOGD("%s, message from server (%d)) : %s\n", CFG_SECTION_TV, rd_len, recv_buf);
        }

        close(sock);
        sock = -1;

        return 0;
    }

    return -1;
}

int I2C_WriteNbyte(int i2c_no, int dev_addr, int slave_addr, int len, unsigned char data_buf[])
{
    int tmp_ret = 0;
    struct i2c_rdwr_ioctl_data ctl_data;
    struct i2c_msg msg;
    unsigned char msg_buf[52];
    int device_fd = -1;

    memset((void *) msg_buf, 0, 52);

    msg_buf[0] = (unsigned char) (slave_addr >> 8);
    msg_buf[1] = (unsigned char) (slave_addr & 0x00ff);

    if (data_buf == NULL) {
        return -1;
    }

    if (len < 50) {
        memcpy((void *) &msg_buf[2], data_buf, len);
    } else {
        LOGE("I2C_WriteNbyte len(%d) > 50, error!\n", len);
        return -1;
    }

    msg.addr = dev_addr;
    msg.flags = I2C_M_WR;
    msg.len = 2 + len;
    msg.buf = msg_buf;
    ctl_data.nmsgs = 1;
    ctl_data.msgs = &msg;

    if (i2c_no == 1) {
        device_fd = open(CS_I2C_1_DEV_PATH, O_RDWR);
        if (device_fd < 0) {
            LOGE("%s, Open device file %s error: %s.\n", CFG_SECTION_TV, CS_I2C_1_DEV_PATH, strerror(errno));
            return -1;
        }
    } else if (i2c_no == 2) {
        device_fd = open(CS_I2C_2_DEV_PATH, O_RDWR);
        if (device_fd < 0) {
            LOGE("%s, Open device file %s error: %s.\n", CFG_SECTION_TV, CS_I2C_2_DEV_PATH, strerror(errno));
            return -1;
        }
    } else {
        LOGE("%s, invalid i2c no (%d).\n", CFG_SECTION_TV, i2c_no);
        return -1;
    }

    tmp_ret = ioctl(device_fd, I2C_RDWR, &ctl_data);

    usleep(10 * 1000);
    if (device_fd >= 0) {
        close(device_fd);
        device_fd = -1;
    }
    return tmp_ret;
}

int I2C_ReadNbyte(int i2c_no, int dev_addr, int slave_addr, int len, unsigned char data_buf[])
{
    int tmp_ret = 0;
    struct i2c_rdwr_ioctl_data ctl_data;
    struct i2c_msg msg;
    unsigned char msg_buf[52];
    int device_fd = -1;

    memset((void *) msg_buf, 0, 52);

    if (data_buf == NULL) {
        return -1;
    }

    if (len < 50) {
        memcpy((void *) &msg_buf[2], data_buf, len);
    } else {
        LOGE("I2C_WriteNbyte len(%d) > 50, error!\n", len);
        return -1;
    }

    msg_buf[0] = (unsigned char) (slave_addr >> 8);
    msg_buf[1] = (unsigned char) (slave_addr & 0x00ff);
    msg.addr = dev_addr;
    msg.flags = I2C_M_WR;
    msg.len = 2;
    msg.buf = msg_buf;
    ctl_data.nmsgs = 1;
    ctl_data.msgs = &msg;

    if (i2c_no == 1) {
        device_fd = open(CS_I2C_1_DEV_PATH, O_RDWR);
        if (device_fd < 0) {
            LOGE("%s, Open device file %s error: %s.\n", CFG_SECTION_TV, CS_I2C_1_DEV_PATH, strerror(errno));
            return -1;
        }
    } else if (i2c_no == 2) {
        device_fd = open(CS_I2C_2_DEV_PATH, O_RDWR);
        if (device_fd < 0) {
            LOGE("%s, Open device file %s error: %s.\n", CFG_SECTION_TV, CS_I2C_2_DEV_PATH, strerror(errno));
            return -1;
        }
    } else {
        LOGE("%s, invalid i2c no (%d).\n", CFG_SECTION_TV, i2c_no);
        return -1;
    }

    tmp_ret = ioctl(device_fd, I2C_RDWR, &ctl_data);

    msg.addr = dev_addr;
    msg.flags |= I2C_M_RD;
    msg.len = len;
    msg.buf = data_buf;
    ctl_data.nmsgs = 1;
    ctl_data.msgs = &msg;

    tmp_ret = ioctl(device_fd, I2C_RDWR, &ctl_data);

    usleep(10 * 1000);

    if (device_fd >= 0) {
        close(device_fd);
        device_fd = -1;
    }
    return tmp_ret;
}

int SetFileAttrValue(const char *fp, const char value[])
{
    int fd = -1, ret = -1;

    pthread_mutex_lock(&file_attr_control_flag_mutex);

    fd = open(fp, O_RDWR);

    if (fd < 0) {
        LOGE("open %s ERROR(%s)!!\n", fp, strerror(errno));
        pthread_mutex_unlock(&file_attr_control_flag_mutex);
        return -1;
    }

    ret = write(fd, value, strlen(value));
    close(fd);

    pthread_mutex_unlock(&file_attr_control_flag_mutex);
    return ret;
}

int GetFileAttrIntValue(const char *fp)
{
    int fd = -1, ret = -1;
    int temp = -1;
    char temp_str[32];

    memset(temp_str, 0, 32);

    fd = open(fp, O_RDWR);

    if (fd <= 0) {
        LOGE("open %s ERROR(%s)!!\n", fp, strerror(errno));
        return -1;
    }

    if (read(fd, temp_str, sizeof(temp_str)) > 0) {
        if (sscanf(temp_str, "%d", &temp) >= 0) {
            LOGD("%s -> get %s value =%d!\n", CFG_SECTION_TV, fp, temp);
            close(fd);
            return temp;
        } else {
            LOGE("%s -> get %s value error(%s)\n", CFG_SECTION_TV, fp, strerror(errno));
            close(fd);
            return -1;
        }
    }

    close(fd);
    return -1;
}

int *GetFileAttrIntValueStr(const char *fp)
{
    int fd = -1, ret = -1;
    static int temp[4];
    char temp_str[32];
    int i = 0;
    char *p = NULL;

    memset(temp_str, 0, 32);

    fd = open(fp, O_RDWR);

    if (fd <= 0) {
        LOGE("open %s ERROR(%s)!!\n", fp, strerror(errno));
        return NULL;
    }

    if (read(fd, temp_str, sizeof(temp_str)) > 0) {
        LOGD("%s,temp_str = %s\n", CFG_SECTION_TV, temp_str);
        p = strtok(temp_str, " ");
        while (p != NULL) {
            sscanf(p, "%d", &temp[i]);
            p = strtok(NULL, " ");
            i = i + 1;
        }
        close(fd);
        return temp;
    }

    close(fd);
    return NULL;
}

int Get_Fixed_NonStandard(void)
{
    return GetFileAttrIntValue("/sys/module/tvin_afe/parameters/force_nostd");
}

//0-turn off
//1-force non-standard
//2-force normal
int Set_Fixed_NonStandard(int value)
{
    int fd = -1, ret = -1;
    char set_vale[32];
    memset(set_vale, '\0', 32);

    sprintf(set_vale, "%d", value);

    fd = open("/sys/module/tvin_afe/parameters/force_nostd", O_RDWR);

    if (fd >= 0) {
        ret = write(fd, set_vale, strlen(set_vale));
    }

    if (ret <= 0) {
        LOGE("%s -> set /sys/module/tvin_afe/parameters/force_nostd error(%s)!\n", CFG_SECTION_TV, strerror(errno));
    }

    close(fd);

    return ret;
}

static void *UserPet_TreadRun(void *data __unused)
{
    while (is_turnon_user_pet_thread == true) {
        if (is_user_pet_thread_start == true) {
            usleep(1000 * 1000);
            if (++user_counter == 0xffffffff)
                user_counter = 1;
            TvMisc_SetUserCounter(user_counter);
        } else {
            usleep(10000 * 1000);
        }
    }
    if (user_pet_terminal == 1) {
        user_counter = 0;
    } else {
        user_counter = 1;
    }
    TvMisc_SetUserCounter(user_counter);
    return ((void *) 0);
}

static int UserPet_CreateThread(void)
{
    int ret = 0;
    pthread_attr_t attr;
    struct sched_param param;

    is_turnon_user_pet_thread = true;
    is_user_pet_thread_start = true;

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &param);
    ret = pthread_create(&UserPet_ThreadId, &attr, &UserPet_TreadRun, NULL);
    pthread_attr_destroy(&attr);
    return ret;
}

static void UserPet_KillThread(void)
{
    int i = 0, dly = 600;
    is_turnon_user_pet_thread = false;
    is_user_pet_thread_start = false;
    for (i = 0; i < 2; i++) {
        usleep(dly * 1000);
    }
    pthread_join(UserPet_ThreadId, NULL);
    UserPet_ThreadId = 0;
    LOGD("%s, done.", CFG_SECTION_TV);
}

void TvMisc_EnableWDT(bool kernelpet_disable, unsigned int userpet_enable, unsigned int kernelpet_timeout, unsigned int userpet_timeout, unsigned int userpet_reset)
{
    TvMisc_SetSystemPetCounterTimeOut(kernelpet_timeout);
    TvMisc_SetSystemPetEnable(1);
    if (kernelpet_disable) {
        TvMisc_SetSystemPetResetEnable(0);
    } else {
        TvMisc_SetSystemPetResetEnable(1);
    }
    if (userpet_enable) {
        TvMisc_SetUserCounterTimeOut(userpet_timeout);
        TvMisc_SetUserPetResetEnable(userpet_reset);
        UserPet_CreateThread();
    } else {
        TvMisc_SetUserCounter(0);
        TvMisc_SetUserPetResetEnable(0);
    }
}

void TvMisc_DisableWDT(unsigned int userpet_enable)
{
    if (userpet_enable) {
        user_pet_terminal = 0;
        UserPet_KillThread();
    }
}

static int get_hardware_info(char *hardware, unsigned int *revision)
{
    char data[1024];
    int fd, n;
    char *x, *hw, *rev;

    fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    n = read(fd, data, 1023);
    close(fd);
    if (n < 0) {
        return -1;
    }

    data[n] = 0;

    if (hardware != NULL) {
        hw = strstr(data, "\nHardware");

        if (hw) {
            x = strstr(hw, ": ");
            if (x) {
                x += 2;
                n = 0;
                while (*x && *x != '\n' && !isspace(*x)) {
                    hardware[n++] = tolower(*x);
                    x++;
                    if (n == 31) {
                        break;
                    }
                }

                hardware[n] = 0;
            }
        }
    }

    if (revision != NULL) {
        rev = strstr(data, "\nRevision");

        if (rev) {
            x = strstr(rev, ": ");
            if (x) {
                *revision = strtoul(x + 2, 0, 16);
            }
        }
    }

    return 0;
}

int get_hardware_name(char *hardware)
{
    int tmp_ret = 0;

    if (hardware == NULL) {
        return -1;
    }

    tmp_ret = get_hardware_info(hardware, NULL);
    if (tmp_ret < 0) {
        hardware[0] = '\0';
    }

    return 0;
}

/*---------------delete dir---------------*/
int TvMisc_DeleteDirFiles(const char *strPath, int flag)
{
    int status;
    char tmp[256];
    switch (flag) {
    case 0:
        sprintf(tmp, "rm -f %s", strPath);
        LOGE("%s", tmp);
        system(tmp);
        break;
    case 1:
        sprintf(tmp, "cd %s", strPath);
        LOGE("%s", tmp);
        status = system(tmp);
        if (status > 0 || status < 0)
            return -1;
        sprintf(tmp, "cd %s;rm -rf *", strPath);
        system(tmp);
        LOGE("%s", tmp);
        break;
    case 2:
        sprintf(tmp, "rm -rf %s", strPath);
        LOGE("%s", tmp);
        system(tmp);
        break;
    }
    return 0;
}
/*---------------delete dir  end-----------*/

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif


int Tv_Utils_CheckFs(void)
{
    FILE *f;
    char mount_dev[256];
    char mount_dir[256];
    char mount_type[256];
    char mount_opts[256];
    int mount_freq;
    int mount_passno;
    int match;
    int found_ro_fs = 0;
    int data_status = 0;
    int cache_status = 0;
    int atv_status = 0;
    int dtv_status = 0;
    int param_status = 0;
    int cool_reboot = 0;
    int recovery_reboot = 0;

    f = fopen("/proc/mounts", "r");
    if (! f) {
        /* If we can't read /proc/mounts, just give up */
        return 1;
    }

    do {
        match = fscanf(f, "%255s %255s %255s %255s %d %d\n",
                       mount_dev, mount_dir, mount_type,
                       mount_opts, &mount_freq, &mount_passno);
        mount_dev[255] = 0;
        mount_dir[255] = 0;
        mount_type[255] = 0;
        mount_opts[255] = 0;
        if ((match == 6) && (!strncmp(mount_dev, "/dev/block", 10))) {
            LOGD("%s, %s %s %s %s %d %d!", CFG_SECTION_TV, mount_dev, mount_dir, mount_type, mount_opts, mount_freq, mount_passno);
            if (!strncmp(mount_dir, "/param", 6)) {
                param_status |= 0x01;
            } else if (!strncmp(mount_dir, "/atv", 4)) {
                atv_status |= 0x01;
            } else if (!strncmp(mount_dir, "/dtv", 4)) {
                dtv_status |= 0x01;
            } else if (!strncmp(mount_dir, "/data", 5)) {
                data_status |= 0x01;
            } else if (!strncmp(mount_dir, "/cache", 6)) {
                cache_status |= 0x01;
            }
            if (strstr(mount_opts, "ro")) {
                found_ro_fs += 1;
                if (!strncmp(mount_dir, "/param", 6)) {
                    param_status |= 0x02;
                } else if (!strncmp(mount_dir, "/atv", 4)) {
                    atv_status |= 0x02;
                } else if (!strncmp(mount_dir, "/dtv", 4)) {
                    dtv_status |= 0x02;
                } else if (!strncmp(mount_dir, "/data", 5)) {
                    data_status |= 0x02;
                } else if (!strncmp(mount_dir, "/cache", 6)) {
                    cache_status |= 0x02;
                }
            }
        }
    } while (match != EOF);

    fclose(f);

    switch (param_status) {
    case 0x03:
        LOGW("%s, param partition is read-only!", CFG_SECTION_TV);
        break;
    case 0x00:
        LOGW("%s, param partition can not be mounted!", CFG_SECTION_TV);
        break;
    default:
        break;
    }
    switch (atv_status) {
    case 0x03:
        LOGW("%s, atv partition is read-only!", CFG_SECTION_TV);
        cool_reboot = 1;
        //android_reboot(ANDROID_RB_RESTART2, 0, "cool_reboot");
        break;
    case 0x00:
        LOGW("%s, atv partition can not be mounted!", CFG_SECTION_TV);
        recovery_reboot = 1;
    //android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
    default:
        break;
    }
    switch (dtv_status) {
    case 0x03:
        LOGW("%s, dtv partition is read-only!", CFG_SECTION_TV);
        //android_reboot(ANDROID_RB_RESTART2, 0, "cool_reboot");
        break;
    case 0x00:
        LOGW("%s, dtv partition can not be mounted!", CFG_SECTION_TV);
    //android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
    default:
        break;
    }
    switch (data_status) {
    case 0x03:
        LOGW("%s, data partition is read-only!", CFG_SECTION_TV);
        cool_reboot = 1;
        //android_reboot(ANDROID_RB_RESTART2, 0, "cool_reboot");
        break;
    case 0x00:
        LOGW("%s, data partition can not be mounted!", CFG_SECTION_TV);
        recovery_reboot = 1;
        //android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
        break;
    default:
        break;
    }
    switch (cache_status) {
    case 0x03:
        LOGW("%s, cache partition is read-only!", CFG_SECTION_TV);
        cool_reboot = 1;
        //android_reboot(ANDROID_RB_RESTART2, 0, "cool_reboot");
        break;
    case 0x00:
        LOGW("%s, cache partition can not be mounted!", CFG_SECTION_TV);
        recovery_reboot = 1;
        //android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
        break;
    default:
        break;
    }
    if (cool_reboot == 1) {
        android_reboot(ANDROID_RB_RESTART2, 0, "cool_reboot");
    }
    if (recovery_reboot == 1) {
        android_reboot(ANDROID_RB_RESTART2, 0, "recovery");
    }
    return found_ro_fs;
}


int Tv_Utils_SetFileAttrStr(const char *file_path, char val_str_buf[])
{
    FILE *tmpfp = NULL;

    tmpfp = fopen(file_path, "w");
    if (tmpfp == NULL) {
        LOGE("%s, write open file %s error(%s)!!!\n", CFG_SECTION_TV, file_path, strerror(errno));
        return -1;
    }

    fputs(val_str_buf, tmpfp);

    fclose(tmpfp);
    tmpfp = NULL;

    return 0;
}

int Tv_Utils_GetFileAttrStr(const char *file_path, int buf_size, char val_str_buf[])
{
    FILE *tmpfp = NULL;

    tmpfp = fopen(file_path, "r");
    if (tmpfp == NULL) {
        LOGE("%s, read open file %s error(%s)!!!\n", CFG_SECTION_TV, file_path, strerror(errno));
        val_str_buf[0] = '\0';
        return -1;
    }

    fgets(val_str_buf, buf_size, tmpfp);

    fclose(tmpfp);
    tmpfp = NULL;

    return 0;
}

//check file exist or not
bool Tv_Utils_IsFileExist(const char *file_name)
{
    struct stat tmp_st;

    return stat(file_name, &tmp_st) == 0;
}

#define CC_EDID_SIZE                                (256)
#define CS_VGA_EDID_BUF_DATA_CFG_NAME "ssm.vga.edidbuf.data"
/*
static unsigned char customer_edid_buf[CC_EDID_SIZE + 4];

static unsigned char mDefHDMIEdidBuf[CC_EDID_SIZE] = {
    //256 bytes
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4d, 0x79, 0x02, 0x2c, 0x01, 0x01, 0x01, 0x01, //0x00~0x0F
    0x01, 0x15, 0x01, 0x03, 0x80, 0x85, 0x4b, 0x78, 0x0a, 0x0d, 0xc9, 0xa0, 0x57, 0x47, 0x98, 0x27, //0x10~0x1F
    0x12, 0x48, 0x4c, 0x21, 0x08, 0x00, 0x81, 0x80, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, //0x20~0x2F
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c, //0x30~0x3F
    0x45, 0x00, 0x30, 0xeb, 0x52, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, //0x40~0x4F
    0x6e, 0x28, 0x55, 0x00, 0x30, 0xeb, 0x52, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x53, //0x50~0x5F
    0x6b, 0x79, 0x77, 0x6f, 0x72, 0x74, 0x68, 0x20, 0x54, 0x56, 0x0a, 0x20, 0x00, 0x00, 0x00, 0xfd, //0x60~0x6F
    0x00, 0x30, 0x3e, 0x0e, 0x46, 0x0f, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x3e, //0x70~0x7F
    0x02, 0x03, 0x38, 0xf0, 0x53, 0x1f, 0x10, 0x14, 0x05, 0x13, 0x04, 0x20, 0x22, 0x3c, 0x3e, 0x12, //0x80~0x8F
    0x16, 0x03, 0x07, 0x11, 0x15, 0x02, 0x06, 0x01, 0x23, 0x09, 0x07, 0x01, 0x83, 0x01, 0x00, 0x00, //0x90~0x9F
    0x78, 0x03, 0x0c, 0x00, 0x10, 0x00, 0x88, 0x3c, 0x2f, 0xd0, 0x8a, 0x01, 0x02, 0x03, 0x04, 0x01, //0xA0~0xAF
    0x40, 0x00, 0x7f, 0x20, 0x30, 0x70, 0x80, 0x90, 0x76, 0xe2, 0x00, 0xfb, 0x02, 0x3a, 0x80, 0xd0, //0xB0~0xBF
    0x72, 0x38, 0x2d, 0x40, 0x10, 0x2c, 0x45, 0x80, 0x30, 0xeb, 0x52, 0x00, 0x00, 0x1e, 0x01, 0x1d, //0xC0~0xCF
    0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28, 0x55, 0x40, 0x30, 0xeb, 0x52, 0x00, 0x00, 0x1e, //0xD0~0xDF
    0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c, 0x16, 0x20, 0x10, 0x2c, 0x25, 0x80, 0x30, 0xeb, 0x52, 0x00, //0xE0~0xEF
    0x00, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4, //0xF0~0xFF
};
*/
static unsigned char mVGAEdidDataBuf[CC_EDID_SIZE];
void  monitor_info_set_date ( unsigned char *edidbuf )
{
    char prop_value[PROPERTY_VALUE_MAX];
    char tmp[4];
    struct tm *p;
    int week = 0;

    memset ( prop_value, '\0', PROPERTY_VALUE_MAX );

    property_get ( "ro.build.date.utc", prop_value, "VERSION_ERROR" );

    time_t timep = atoi ( prop_value );

    p = localtime ( &timep );

    mktime ( p );

    strftime ( prop_value, PROPERTY_VALUE_MAX, "%W.", p );

    week = atoi ( prop_value );

    edidbuf[16] = week;
    edidbuf[17] = ( 1900 + p->tm_year ) - 1990;

    LOGD ( "###############%s##############", CFG_SECTION_TV );
    LOGD ( "Week number is %d", week );
    LOGD ( "%d %02d %02d", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday );
    LOGD ( "###############%s##############", CFG_SECTION_TV );
}


void monitor_info_set_imagesize ( unsigned char *edidbuf )
{
    //panel size info for edid:
    //39' : max_horizontal = 86; max_vertical = 48;
    //42' : max_horizontal = 93; max_vertical = 52;
    //47' : max_horizontal = 104; max_vertical = 60;
    //50' : max_horizontal = 110; max_vertical = 62;
    //55' : max_horizontal = 121; max_vertical = 71;
    int max_horizontal = 104; //47'
    int max_vertical = 60; //47'

    edidbuf[21] = max_horizontal;
    edidbuf[22] = max_vertical;

    LOGD ( "imagesize max_horizontal %d max_vertical %d", max_horizontal, max_vertical );
}



void monitor_info_name_init ( unsigned char *edidbuf )
{
    int i = 0;

    for ( i = 90; i < 108; i++ ) {
        edidbuf[i] = 0;
    }

    edidbuf[93] = 252;
}

void monitor_info_set_name ( unsigned char *edidbuf )
{
    int i = 0;
    int config_value_len;
    const char *config_value;
    unsigned char str_manufacturer_name[14];
    config_value = config_get_str ( CFG_SECTION_TV, "tvin.hdmiedid.name", "null" );

    if ( strcmp ( config_value, "null" ) != 0 ) {
        config_value_len = strlen ( config_value );

        if ( config_value_len < 13 ) {
            for ( i = 0; i < config_value_len; ++i ) {
                str_manufacturer_name[i] = config_value[i];
            }

            for ( i = config_value_len; i < 13; ++i ) {
                str_manufacturer_name[i] = ' ';
            }
        } else {
            for ( i = 0; i < 13; ++i ) {
                str_manufacturer_name[i] = config_value[i];
            }
        }

    }

    for ( i = 0; i < 13; i++ ) {
        edidbuf[95 + i] = str_manufacturer_name[i];
    }
}

void monitor_info_edid_checksum ( unsigned char *edidbuf )
{
    int sum = 0, i = 0;

    for ( i = 0; i < 127; i++ ) {
        sum += edidbuf[i];
    }

    sum = ( 256 - ( sum % 256 ) ) % 256;
    edidbuf[127] = sum;

    LOGD ( "checksum is 0x%x,so testBuf[127] = 0x%x", sum, edidbuf[127] );
}

int reboot_sys_by_fbc_edid_info()
{
    int ret = -1;
    int fd = -1;
    int edid_info_len = 256;
    unsigned char fbc_edid_info[edid_info_len];
    int env_different_as_cur = 0;
    char outputmode_prop_value[256];
    char lcd_reverse_prop_value[256];

    LOGD("get edid info from fbc!");
    memset(outputmode_prop_value, '\0', 256);
    memset(lcd_reverse_prop_value, '\0', 256);
    property_get(UBOOTENV_OUTPUTMODE, outputmode_prop_value, "null" );
    property_get("ubootenv.var.lcd_reverse", lcd_reverse_prop_value, "null" );

    fd = open("/sys/class/amhdmitx/amhdmitx0/edid_info", O_RDWR);
    if (fd < 0) {
        LOGW("open edid node error\n");
        return -1;
    }
    ret = read(fd, fbc_edid_info, edid_info_len);
    if (ret < 0) {
        LOGW("read edid node error\n");
        return -1;
    }

    if ((0xfb == fbc_edid_info[250]) && (0x0c == fbc_edid_info[251])) {
        LOGD("RX is FBC!");
        // set outputmode env
        ret = 0;//is Fbc
        switch (fbc_edid_info[252] & 0x0f) {
        case 0x0:
            if (0 != strcmp(outputmode_prop_value, "1080p") &&
                    0 != strcmp(outputmode_prop_value, "1080p50hz")
               ) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                property_set(UBOOTENV_OUTPUTMODE, "1080p");
            }
            break;
        case 0x1:
            if (0 != strcmp(outputmode_prop_value, "4k2k60hz420") &&
                    0 != strcmp(outputmode_prop_value, "4k2k50hz420")
               ) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                property_set(UBOOTENV_OUTPUTMODE, "4k2k60hz420");
            }
            break;
        case 0x2:
            if (0 != strcmp(outputmode_prop_value, "1366*768")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                property_set(UBOOTENV_OUTPUTMODE, "1366*768");
            }
            break;
        default:
            break;
        }

        // set RX 3D Info
        //switch((fbc_edid_info[252]>>4)&0x0f)

        // set lcd_reverse env
        switch (fbc_edid_info[253]) {
        case 0x0:
            if (0 != strcmp(lcd_reverse_prop_value, "0")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                property_set("ubootenv.var.lcd_reverse", "0");
            }
            break;
        case 0x1:
            if (0 != strcmp(lcd_reverse_prop_value, "1")) {
                if (0 == env_different_as_cur) {
                    env_different_as_cur = 1;
                }
                property_set("ubootenv.var.lcd_reverse", "1");
            }
            break;
        default:
            break;
        }
    }
    close(fd);
    fd = -1;
    //ret = -1;
    if (1 == env_different_as_cur) {
        LOGW("env change , reboot system\n");
        system("reboot");
    }
    return ret;
}

int reboot_sys_by_fbc_uart_panel_info(CFbcCommunication *fbc)
{
    int ret = -1;
    char outputmode_prop_value[256];
    char lcd_reverse_prop_value[256];
    int env_different_as_cur = 0;
    int panel_reverse = -1;
    int panel_outputmode = -1;


    char panel_model[64] = {0};

    if (fbc == NULL) {
        LOGE("there is no fbc!!!\n");
        return -1;
    }

    fbc->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, panel_model);
    if (0 == panel_model[0]) {
        LOGD("device is not fbc\n");
        return -1;
    }
    LOGD("device is fbc, get panel info from fbc!\n");
    memset(outputmode_prop_value, '\0', 256);
    memset(lcd_reverse_prop_value, '\0', 256);
    property_get(UBOOTENV_OUTPUTMODE, outputmode_prop_value, "null" );
    property_get("ubootenv.var.lcd_reverse", lcd_reverse_prop_value, "null" );

    fbc->cfbc_Get_FBC_PANEL_REVERSE(COMM_DEV_SERIAL, &panel_reverse);
    fbc->cfbc_Get_FBC_PANEL_OUTPUT(COMM_DEV_SERIAL, &panel_outputmode);
    LOGD("panel_reverse = %d, panel_outputmode = %d\n", panel_reverse, panel_outputmode);
    LOGD("panel_output prop = %s, panel reverse prop = %s\n", outputmode_prop_value, lcd_reverse_prop_value);
    switch (panel_outputmode) {
    case 0x0:
        if (0 != strcmp(outputmode_prop_value, "1080p") &&
                0 != strcmp(outputmode_prop_value, "1080p50hz")
           ) {
            LOGD("panel_output changed to 1080p\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            property_set(UBOOTENV_OUTPUTMODE, "1080p");
        }
        break;
    case 0x1:
        if (0 != strcmp(outputmode_prop_value, "4k2k60hz420") &&
                0 != strcmp(outputmode_prop_value, "4k2k50hz420")
           ) {
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            property_set(UBOOTENV_OUTPUTMODE, "4k2k60hz420");
        }
        break;
    case 0x2:
        if (0 != strcmp(outputmode_prop_value, "1366*768")) {
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            property_set(UBOOTENV_OUTPUTMODE, "1366*768");
        }
        break;
    default:
        break;
    }

    // set RX 3D Info
    //switch((fbc_edid_info[252]>>4)&0x0f)

    // set lcd_reverse env
    switch (panel_reverse) {
    case 0x0:
        if (0 != strcmp(lcd_reverse_prop_value, "0")) {
            LOGD("panel_reverse changed to 0\n");
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            property_set("ubootenv.var.lcd_reverse", "0");
        }
        break;
    case 0x1:
        if (0 != strcmp(lcd_reverse_prop_value, "1")) {
            if (0 == env_different_as_cur) {
                env_different_as_cur = 1;
            }
            property_set("ubootenv.var.lcd_reverse", "1");
        }
        break;
    default:
        break;
    }

    ret = -1;
    if (1 == env_different_as_cur) {
        LOGW("env change , reboot system\n");
        system("reboot");
    }
    return 0;
}

static pid_t pidof(const char *name)
{
    DIR *dir;
    struct dirent *ent;
    char *endptr;
    char tmp_buf[512];

    if (!(dir = opendir("/proc"))) {
        LOGE("%s, can't open /proc, %s", __FUNCTION__, strerror(errno));
        return -1;
    }

    while ((ent = readdir(dir)) != NULL) {
        /* if endptr is not a null character, the directory is not
         * entirely numeric, so ignore it */
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        /* try to open the cmdline file */
        snprintf(tmp_buf, sizeof(tmp_buf), "/proc/%ld/cmdline", lpid);
        FILE *fp = fopen(tmp_buf, "r");

        if (fp) {
            if (fgets(tmp_buf, sizeof(tmp_buf), fp) != NULL) {
                /* check the first token in the file, the program name */
                char *first = strtok(tmp_buf, " ");
                if (!strcmp(first, name)) {
                    fclose(fp);
                    closedir(dir);
                    return (pid_t) lpid;
                }
            }
            fclose(fp);
        }
    }

    closedir(dir);
    return -1;
}



int GetPlatformHaveDDFlag()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, "platform.havedd", "null");
    if (strcmp(config_value, "true") == 0 || strcmp(config_value, "1") == 0) {
        return 1;
    }

    return 0;
}

int GetPlatformProjectInfoSrc()
{
    const char *config_value;

    config_value = config_get_str(CFG_SECTION_TV, "platform.projectinfo.src", "null");
    if (strcmp(config_value, "null") == 0 || strcmp(config_value, "prop") == 0) {
        return 0;
    } else if (strcmp(config_value, "emmckey") == 0) {
        return 1;
    } else if (strcmp(config_value, "fbc_ver") == 0) {
        return 2;
    }

    return 0;
}

static unsigned int mCrc32Table[256];

static void initCrc32Table()
{
    int i, j;
    unsigned int Crc;
    for (i = 0; i < 256; i++) {
        Crc = i;
        for (j = 0; j < 8; j++) {
            if (Crc & 1)
                Crc = (Crc >> 1) ^ 0xEDB88320;
            else
                Crc >>= 1;
        }
        mCrc32Table[i] = Crc;
    }
}

unsigned int CalCRC32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len)
{
    static const unsigned int s_crc32[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
                                              0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
                                            };
    unsigned int crcu32 = crc;
    //if (buf_len < 0)
    //    return 0;
    if (!ptr) return 0;
    crcu32 = ~crcu32;
    while (buf_len--) {
        unsigned char b = *ptr++;
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)];
        crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)];
    }
    return ~crcu32;
}

#define CC_HEAD_CHKSUM_LEN     (9)
#define CC_VERSION_LEN         (5)

static int check_projectinfo_data_valid(char *data_str, int chksum_head_len, int ver_len)
{
    int tmp_len = 0, tmp_ver = 0;
    char *endp = NULL;
    unsigned long src_chksum = 0, cal_chksum = 0;
    char tmp_buf[129] = { 0 };

    if (data_str != NULL) {
        tmp_len = strlen(data_str);
        if (tmp_len > chksum_head_len + ver_len) {
            cal_chksum = CalCRC32(0, (unsigned char *)(data_str + chksum_head_len), tmp_len - chksum_head_len);
            memcpy(tmp_buf, data_str, chksum_head_len);
            tmp_buf[chksum_head_len] = 0;
            src_chksum = strtoul(tmp_buf, &endp, 16);
            if (cal_chksum == src_chksum) {
                memcpy(tmp_buf, data_str + chksum_head_len, ver_len);
                if ((tmp_buf[0] == 'v' || tmp_buf[0] == 'V') && isxdigit(tmp_buf[1]) && isxdigit(tmp_buf[2]) && isxdigit(tmp_buf[3])) {
                    tmp_ver = strtoul(tmp_buf + 1, &endp, 16);
                    if (tmp_ver <= 0) {
                        LOGD("%s, project_info data version error!!!\n", __FUNCTION__);
                        return -1;
                    }
                } else {
                    LOGD("%s, project_info data version error!!!\n", __FUNCTION__);
                    return -1;
                }

                return tmp_ver;
            } else {
                LOGD("%s, cal_chksum = %x\n", __FUNCTION__, (unsigned int)cal_chksum);
                LOGD("%s, src_chksum = %x\n", __FUNCTION__, (unsigned int)src_chksum);
            }
        }

        LOGD("%s, project_info data error!!!\n", __FUNCTION__);
        return -1;
    }

    LOGD("%s, project_info data is NULL!!!\n", __FUNCTION__);
    return -1;
}

static int gFBCPrjInfoRDPass = 0;
static char gFBCPrjInfoBuf[1024] = {0};

static int GetProjectInfoOriData(char data_str[], CFbcCommunication *fbcIns)
{
    int src_type = GetPlatformProjectInfoSrc();

    if (src_type == 0) {
        //memset(data_str, '\0', sizeof(data_str));//sizeof pointer has issue
        getBootEnv("ubootenv.var.project_info", data_str, (char *)"null");
        if (strcmp(data_str, "null") == 0) {
            LOGE("%s, get project info data error!!!\n", __FUNCTION__);
            return -1;
        }

        return 0;
    } else if (src_type == 1) {
        return -1;
    } else if (src_type == 2) {
        int i = 0, tmp_len = 0, tmp_val = 0, item_cnt = 0;
        int tmp_rd_fail_flag = 0;
        unsigned int cal_chksum = 0;
        char sw_version[64];
        char build_time[64];
        char git_version[64];
        char git_branch[64];
        char build_name[64];
        char tmp_buf[512] = {0};

        if (fbcIns != NULL) {
            if (gFBCPrjInfoRDPass == 0) {
                memset((void *)gFBCPrjInfoBuf, 0, sizeof(gFBCPrjInfoBuf));
            }

            if (gFBCPrjInfoRDPass == 1) {
                strcpy(data_str, gFBCPrjInfoBuf);
                LOGD("%s, rd once just return, data_str = %s\n", __FUNCTION__, data_str);
                return 0;
            }

            if (fbcIns->cfbc_Get_FBC_MAINCODE_Version(COMM_DEV_SERIAL, sw_version, build_time, git_version, git_branch, build_name) == 0) {
                if (sw_version[0] == '1' || sw_version[0] == '2') {
                    strcpy(build_name, "2");

                    strcpy(tmp_buf, "v001,fbc_");
                    strcat(tmp_buf, build_name);
                    strcat(tmp_buf, ",4k2k60hz420,no_rev,");
                    strcat(tmp_buf, "HV550QU2-305");
                    strcat(tmp_buf, ",8o8w,0,0");
                    cal_chksum = CalCRC32(0, (unsigned char *)tmp_buf, strlen(tmp_buf));
                    sprintf(data_str, "%08x,%s", cal_chksum, tmp_buf);
                    LOGD("%s, data_str = %s\n", __FUNCTION__, data_str);
                } else {
                    tmp_val = 0;
                    if (fbcIns->cfbc_Get_FBC_project_id(COMM_DEV_SERIAL, &tmp_val) == 0) {
                        sprintf(build_name, "fbc_%d", tmp_val);
                    } else {
                        tmp_rd_fail_flag = 1;
                        strcpy(build_name, "fbc_0");
                        LOGD("%s, get project id from fbc error!!!\n", __FUNCTION__);
                    }

                    strcpy(tmp_buf, "v001,");
                    strcat(tmp_buf, build_name);
                    strcat(tmp_buf, ",4k2k60hz420,no_rev,");

                    memset(git_branch, 0, sizeof(git_branch));
                    if (fbcIns->cfbc_Get_FBC_Get_PANel_INFO(COMM_DEV_SERIAL, git_branch) == 0) {
                        strcat(tmp_buf, git_branch);
                    } else {
                        tmp_rd_fail_flag = 1;
                        strcat(tmp_buf, build_name);
                        LOGD("%s, get panel info from fbc error!!!\n", __FUNCTION__);
                    }

                    strcat(tmp_buf, ",8o8w,0,0");
                    cal_chksum = CalCRC32(0, (unsigned char *)tmp_buf, strlen(tmp_buf));
                    sprintf(data_str, "%08x,%s", cal_chksum, tmp_buf);
                    LOGD("%s, data_str = %s\n", __FUNCTION__, data_str);

                    if (tmp_rd_fail_flag == 0) {
                        gFBCPrjInfoRDPass = 1;
                        strcpy(gFBCPrjInfoBuf, data_str);
                    }
                }

                return 0;
            }

            return -1;
        }
    }

    return -1;
}

static int handle_prj_info_by_ver(int ver, int item_ind, char *item_str, project_info_t *proj_info_ptr)
{
    if (ver == 1) {
        if (item_ind == 0) {
            strncpy(proj_info_ptr->version, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 1) {
            strncpy(proj_info_ptr->panel_type, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 2) {
            strncpy(proj_info_ptr->panel_outputmode, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 3) {
            strncpy(proj_info_ptr->panel_rev, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 4) {
            strncpy(proj_info_ptr->panel_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 5) {
            strncpy(proj_info_ptr->amp_curve_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }
    } else {
        if (item_ind == 0) {
            strncpy(proj_info_ptr->version, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 1) {
            strncpy(proj_info_ptr->panel_type, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 2) {
            strncpy(proj_info_ptr->panel_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }  else if (item_ind == 3) {
            strncpy(proj_info_ptr->panel_outputmode, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 4) {
            strncpy(proj_info_ptr->panel_rev, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        } else if (item_ind == 5) {
            strncpy(proj_info_ptr->amp_curve_name, item_str, CC_PROJECT_INFO_ITEM_MAX_LEN - 1);
        }
    }

    return 0;
}

int GetProjectInfo(project_info_t *proj_info_ptr, CFbcCommunication *fbcIns)
{
    int i = 0, tmp_ret = 0, tmp_val = 0, tmp_len = 0;
    int item_cnt = 0, handle_prj_info_data_flag = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    char tmp_buf[1024] = { 0 };
    char data_str[1024] = { 0 };

    if (GetProjectInfoOriData(data_str, fbcIns) < 0) {
        return -1;
    }

    memset((void *)proj_info_ptr->version, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_type, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_outputmode, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_rev, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->panel_name, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);
    memset((void *)proj_info_ptr->amp_curve_name, 0, CC_PROJECT_INFO_ITEM_MAX_LEN);

    //check project info data is valid
    handle_prj_info_data_flag = check_projectinfo_data_valid(data_str, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN);

    //handle project info data
    if (handle_prj_info_data_flag > 0) {
        item_cnt = 0;
        memset((void *)tmp_buf, 0, sizeof(tmp_buf));
        strncpy(tmp_buf, data_str + CC_HEAD_CHKSUM_LEN, sizeof(tmp_buf) - 1);
        token = strtok(tmp_buf, strDelimit);
        while (token != NULL) {
            handle_prj_info_by_ver(handle_prj_info_data_flag, item_cnt, token, proj_info_ptr);

            token = strtok(NULL, strDelimit);
            item_cnt += 1;
        }

        return 0;
    }

    return -1;
}

#if ANDROID_PLATFORM_SDK_VERSION == 19
int getBootEnv(const char *key, char *value, char *def_val)
{
    return property_get(key, value, def_val);
}

void setBootEnv(const char *key, const char *value)
{
    return property_set(key, value);
}
#endif

#if ANDROID_PLATFORM_SDK_VERSION >= 21

#include <binder/IServiceManager.h>
#include <utils/threads.h>
#include <systemcontrol/ISystemControlService.h>

class DeathNotifier: public IBinder::DeathRecipient {
public:
    DeathNotifier()
    {
    }

    void binderDied(const wp<IBinder> &who __unused)
    {
        ALOGW("system_control died!");
    }
};

static sp<ISystemControlService> amSystemControlService;
static sp<DeathNotifier> amDeathNotifier;
static Mutex            amLock;

static const sp<ISystemControlService> &getSystemControlService()
{
    Mutex::Autolock _l(amLock);
    if (amSystemControlService.get() == 0) {
        sp<IServiceManager> sm = defaultServiceManager();

        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("system_control"));
            if (binder != 0)
                break;
            ALOGW("SystemControlService not published, waiting...");
            usleep(500000); // 0.5 s
        } while(true);
        if (amDeathNotifier == NULL) {
            amDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(amDeathNotifier);
        amSystemControlService = interface_cast<ISystemControlService>(binder);
    }
    ALOGE_IF(amSystemControlService == 0, "no System Control Service!?");

    return amSystemControlService;
}

int getBootEnv(const char *key, char *value, char *def_val)
{
    const sp<ISystemControlService> &sws = getSystemControlService();
    if (sws != 0) {
        String16 v;
        if (sws->getBootEnv(String16(key), v)) {
            strcpy(value, String8(v).string());
            return 0;
        }
    }

    strcpy(value, def_val);
    return -1;
}

void setBootEnv(const char *key, const char *value)
{
    const sp<ISystemControlService> &sws = getSystemControlService();
    if (sws != 0) {
        sws->setBootEnv(String16(key), String16(value));
    }
}
#endif

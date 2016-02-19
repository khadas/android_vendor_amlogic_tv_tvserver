#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#include "tvconfig_core.h"
#include "tvconfig.h"

#define LOG_TAG "tvconfig_test"
#include "CTvLog.h"

static int GetAudioAmplifierBiquadsDataBuffer00(int *biquad_count, int *biquad_item_count, int biquad_data_buf[]);
static int GetAudioAmplifierBiquadsDataBuffer01(int *biquad_count, int *biquad_item_count, int biquad_data_buf[]);

static int GetTvAudioCardID(char tv_card_id_buf[]);
static int GetTvAudioCardName(char tv_card_name_buf[]);

static int ATVGetFacRestoreChanInfo(int *chan_cnt, int *item_cnt, int chan_data_buf[]);

static void PrintGPIOCfgData(int cfg_info_item_count, GPIOCFGInfo cfg_info_buf[]);
static int GetAudioAnalogAmplifierMuteGPIOCFG(char gpio_grp_str[], int *mute_gpio_addr, int *mute_on_val, int *mute_off_val);
static int GetAudioHeadSetMuteGPIOCFG(char gpio_grp_str[], int *mute_gpio_addr, int *mute_on_val, int *mute_off_val);
static int GetAudioAVOutMuteGPIOCFG(char gpio_grp_str[], int *mute_gpio_addr, int *mute_on_val, int *mute_off_val);
static int GetWriteProtectGPIOCFG(char gpio_grp_str[], int *protect_gpio_addr, int *protect_on_val, int *protect_off_val);

typedef struct tagTvServerInfo {
    int power_on_off_channel;
    int last_source_select;
    int system_language;
} TvServerInfo;

int main(int argc, char **argv)
{
    int i, j;
    char key_buf[CC_CFG_KEY_STR_MAX_LEN];
    char cfg_buf[CC_CFG_VALUE_STR_MAX_LEN];

    config_set_log_level (CC_LOG_LEVEL_ALL);

    config_init((char *) "tvconfig.conf");

    strcpy(key_buf, "media.amplayer.enable-acodecs");
    config_get(key_buf, cfg_buf, (char *) "");
    LOGD("file(%s)'s function(%s), key string \"%s\", value string \"%s\".\n", __FILE__, "TV", key_buf, cfg_buf);

    strcpy(key_buf, "media.amplayer.enable-acodecs");
    strcpy(cfg_buf, "shorthoho");
    config_set(key_buf, cfg_buf);

    strcpy(key_buf, "media.amplayer.enable-acodecs");
    config_get(key_buf, cfg_buf, (char *) "");
    LOGD("file(%s)'s function(%s), key string \"%s\", value string \"%s\".\n", __FILE__, "TV", key_buf, cfg_buf);

    strcpy(key_buf, "media.amplayer.enable-acodecs");
    config_get(key_buf, cfg_buf, (char *) "");
    LOGD("file(%s)'s function(%s), key string \"%s\", value string \"%s\".\n", __FILE__, "TV", key_buf, cfg_buf);

    strcpy(key_buf, "media.amplayer.enable-acodecs");
    strcpy(cfg_buf, "asf,wav,aac,mp3,m4a,ape,flac,alac,hohoho");
    config_set(key_buf, cfg_buf);

    strcpy(key_buf, "media.amplayer.enable-acodecs");
    config_get(key_buf, cfg_buf, (char *) "");
    LOGD("file(%s)'s function(%s), key string \"%s\", value string \"%s\".\n", __FILE__, "TV", key_buf, cfg_buf);

    strcpy(key_buf, "shoufu.zhao.test");
    config_get(key_buf, cfg_buf, (char *) "");
    LOGD("file(%s)'s function(%s), key string \"%s\", value string \"%s\".\n", __FILE__, "TV", key_buf, cfg_buf);

    strcpy(key_buf, "shoufu.zhao.test");
    strcpy(cfg_buf, "test hohoho");
    config_set(key_buf, cfg_buf);

    strcpy(key_buf, "shoufu.zhao.test");
    config_get(key_buf, cfg_buf, (char *) "");
    LOGD("file(%s)'s function(%s), key string \"%s\", value string \"%s\".\n", __FILE__, "TV", key_buf, cfg_buf);

    int biquad_count = 0, biquad_item_count = 0;
    int biquad_data_buf00[128] = { 0 };
    int biquad_data_buf01[128] = { 0 };

    GetAudioAmplifierBiquadsDataBuffer00(&biquad_count, &biquad_item_count, biquad_data_buf00);
    for (i = 0; i < biquad_count; i++) {
        for (j = 0; j < biquad_item_count; j++) {
            LOGD("0x%x\n", biquad_data_buf00[i * biquad_item_count + j]);
        }

        LOGD("\n");
    }

    GetAudioAmplifierBiquadsDataBuffer01(&biquad_count, &biquad_item_count, biquad_data_buf01);
    for (i = 0; i < biquad_count; i++) {
        for (j = 0; j < biquad_item_count; j++) {
            LOGD("0x%x\n", biquad_data_buf01[i * biquad_item_count + j]);
        }

        LOGD("\n");
    }

    char tv_card_id_buf[64] = { 0 };
    char tv_card_name_buf[64] = { 0 };

    GetTvAudioCardID(tv_card_id_buf);

    GetTvAudioCardName(tv_card_name_buf);

    printf("tvservice log cfg value = %d\n", config_log_cfg_get(CC_LOG_MODULE_TVSERVICE));
    printf("vpp log cfg value = %d\n", config_log_cfg_get(CC_LOG_MODULE_VPP));

    int chan_cnt = 0, item_cnt = 0;
    int chan_data_buf[256] = { 0 };

    ATVGetFacRestoreChanInfo(&chan_cnt, &item_cnt, chan_data_buf);

    for (i = 0; i < chan_cnt; i++) {
        for (j = 0; j < item_cnt; j++) {
            LOGD("%d\n", chan_data_buf[i * item_cnt + j]);
        }

        LOGD("\n");
    }

    char gpio_grp_str[32] = { 0 };
    int gpio_addr, gpio_on_val;

    int cfg_info_item_count = 0;
    GPIOCFGInfo cfg_info_buf[64];

    if (cfg_get_one_gpio_data("audio.avout.mute.gpio", gpio_grp_str, &gpio_addr, &gpio_on_val) == 0) {
        cfg_info_item_count = 1;

        strcpy(cfg_info_buf[0].gpio_grp_str, gpio_grp_str);
        cfg_info_buf[0].gpio_addr = gpio_addr;
        cfg_info_buf[0].gpio_val = gpio_on_val;

        PrintGPIOCfgData(cfg_info_item_count, &cfg_info_buf[0]);
    }

    if (cfg_get_gpio_data("audio.avout.mute.gpio", &cfg_info_item_count, cfg_info_buf) == 0) {
        PrintGPIOCfgData(cfg_info_item_count, cfg_info_buf);
    }

    cfg_info_item_count = 64;
    if (cfg_get_gpio_data("audio.initaudio.gpioctl", &cfg_info_item_count, cfg_info_buf) == 0) {
        PrintGPIOCfgData(cfg_info_item_count, cfg_info_buf);
    }

    char tmp_buf[32] = { 0 };
    int mute_gpio_addr, mute_on_gpio_val, mute_off_gpio_val;

    if (GetAudioAnalogAmplifierMuteGPIOCFG(gpio_grp_str, &mute_gpio_addr, &mute_on_gpio_val, &mute_off_gpio_val) == 0) {
        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_on_gpio_val);
        LOGD("%s, mute on write command %s\n", "GetAudioAnalogAmplifierMuteGPIOCFG", tmp_buf);

        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_off_gpio_val);
        LOGD("%s, mute off write command %s\n", "GetAudioAnalogAmplifierMuteGPIOCFG", tmp_buf);
    }

    if (GetAudioHeadSetMuteGPIOCFG(gpio_grp_str, &mute_gpio_addr, &mute_on_gpio_val, &mute_off_gpio_val) == 0) {
        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_on_gpio_val);
        LOGD("%s, mute on write command %s\n", "GetAudioHeadSetMuteGPIOCFG", tmp_buf);

        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_off_gpio_val);
        LOGD("%s, mute off write command %s\n", "GetAudioHeadSetMuteGPIOCFG", tmp_buf);
    }

    if (GetAudioAVOutMuteGPIOCFG(gpio_grp_str, &mute_gpio_addr, &mute_on_gpio_val, &mute_off_gpio_val) == 0) {
        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_on_gpio_val);
        LOGD("%s, mute on write command %s\n", "GetAudioAVOutMuteGPIOCFG", tmp_buf);

        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_off_gpio_val);
        LOGD("%s, mute off write command %s\n", "GetAudioAVOutMuteGPIOCFG", tmp_buf);
    }

    if (GetWriteProtectGPIOCFG(gpio_grp_str, &mute_gpio_addr, &mute_on_gpio_val, &mute_off_gpio_val) == 0) {
        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_on_gpio_val);
        LOGD("%s, protect on write command %s\n", "GetWriteProtectGPIOCFG", tmp_buf);

        sprintf(tmp_buf, "w %s %d %d", gpio_grp_str, mute_gpio_addr, mute_off_gpio_val);
        LOGD("%s, protect off write command %s\n", "GetWriteProtectGPIOCFG", tmp_buf);
    }

    tmpInfo.power_on_off_channel = 0;
    tmpInfo.last_source_select = 0;
    tmpInfo.system_language = 1;

    for (i = 0; i < 12; i++) {
        tmpInfo.last_source_select = i;
    }

    char item_buf[128] = { 0 };

    for (i = 0; i < 16; i++) {
        cfg_get_one_item("misc.lastselsrc.show.cfg", i, item_buf);
        LOGD("item %d = %s\n", i, item_buf);
    }

    config_uninit();

    LOGD("file(%s)'s function(%s) exiting.\n", __FILE__, "TV");
    return 0;
}

static int RealGetAudioAmplifierBiquadsDataBuffer(const char *key_str, int *biquad_count, int *biquad_item_count, int biquad_data_buf[])
{
    int cfg_item_count = 0, tmpTotalItemCount = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    char prop_value[CC_CFG_VALUE_STR_MAX_LEN];

    memset(prop_value, '\0', CC_CFG_VALUE_STR_MAX_LEN);

    config_get(key_str, prop_value, "null");
    if (strcasecmp(prop_value, "null") == 0) {
        LOGE("%s, can't get config \"%s\"!!!\n", "TV", key_str);
        *biquad_count = 0;
        *biquad_item_count = 0;
        return -1;
    }

    tmpTotalItemCount = 0;

    token = strtok(prop_value, strDelimit);
    while (token != NULL) {
        if (cfg_item_count == 0) {
            *biquad_count = strtoul(token, NULL, 16);
        } else if (cfg_item_count == 1) {
            *biquad_item_count = strtoul(token, NULL, 16);
        } else if (cfg_item_count >= 2) {
            biquad_data_buf[tmpTotalItemCount] = strtoul(token, NULL, 16);
            tmpTotalItemCount += 1;
        }

        token = strtok(NULL, strDelimit);
        cfg_item_count += 1;
    }

    if ((*biquad_count) * (*biquad_item_count) != tmpTotalItemCount) {
        LOGE("%s, get item count error!!! should be %d, real is %d.\n", "TV", (*biquad_count) * (*biquad_item_count), tmpTotalItemCount);
        *biquad_count = 0;
        *biquad_item_count = 0;
        return -1;
    }

    LOGD("%s, biquad count = %d, biquad item count = %d.\n", "TV", *biquad_count, *biquad_item_count);

    return 0;
}

static int GetAudioAmplifierBiquadsDataBuffer00(int *biquad_count, int *biquad_item_count, int biquad_data_buf[])
{
    return RealGetAudioAmplifierBiquadsDataBuffer("audio.amplifier.biquad00.data", biquad_count, biquad_item_count, biquad_data_buf);
}

static int GetAudioAmplifierBiquadsDataBuffer01(int *biquad_count, int *biquad_item_count, int biquad_data_buf[])
{
    return RealGetAudioAmplifierBiquadsDataBuffer("audio.amplifier.biquad01.data", biquad_count, biquad_item_count, biquad_data_buf);
}

static int GetTvAudioCardID(char tv_card_id_buf[])
{
    config_get("audio.tv.card.id", tv_card_id_buf, "null");
    LOGD("%s, get card id is \"%s\".\n", "TV", tv_card_id_buf);

    if (strcmp(tv_card_id_buf, "null") == 0) {
        strcpy(tv_card_id_buf, "hw:AMLM2");
        LOGD("%s, card id not config, we set to default \"%s\".\n", "TV", "hw:AMLM2");
    }

    return 0;
}

static int GetTvAudioCardName(char tv_card_name_buf[])
{
    config_get("audio.tv.card.name", tv_card_name_buf, "null");
    LOGD("%s, get card name is \"%s\".\n", "TV", tv_card_name_buf);

    if (strcmp(tv_card_name_buf, "null") == 0) {
        strcpy(tv_card_name_buf, "AML-M2");
        LOGD("%s, card name not config, we set to default \"%s\".\n", "TV", "AML-M2");
    }

    return 0;
}

static int ATVGetFacRestoreChanInfo(int *chan_cnt, int *item_cnt, int chan_data_buf[])
{
    int cfg_item_count = 0, tmpTotalItemCount = 0;
    char *token = NULL;
    const char *strDelimit = ",";
    const char *key_str = "atv.fac.defchaninfo";
    char prop_value[CC_CFG_VALUE_STR_MAX_LEN];

    memset(prop_value, '\0', CC_CFG_VALUE_STR_MAX_LEN);

    config_get(key_str, prop_value, "null");
    if (strcasecmp(prop_value, "null") == 0) {
        LOGE("%s, can't get config \"%s\"!!!\n", "TV", key_str);
        *chan_cnt = 0;
        *item_cnt = 0;
        return -1;
    }

    tmpTotalItemCount = 0;

    token = strtok(prop_value, strDelimit);
    while (token != NULL) {
        if (cfg_item_count == 0) {
            *chan_cnt = strtoul(token, NULL, 10);
        } else if (cfg_item_count == 1) {
            *item_cnt = strtoul(token, NULL, 10);
        } else if (cfg_item_count >= 2) {
            chan_data_buf[tmpTotalItemCount] = strtoul(token, NULL, 10);
            tmpTotalItemCount += 1;
        }

        token = strtok(NULL, strDelimit);
        cfg_item_count += 1;
    }

    if ((*chan_cnt) * (*item_cnt) != tmpTotalItemCount) {
        LOGE("%s, get item count error!!! should be %d, real is %d.\n", "TV", (*chan_cnt) * (*item_cnt), tmpTotalItemCount);
        *chan_cnt = 0;
        *item_cnt = 0;
        return -1;
    }

    LOGD("%s, channel count = %d, channel item count = %d.\n", "TV", *chan_cnt, *item_cnt);

    return 0;
}

static void PrintGPIOCfgData(int cfg_info_item_count, GPIOCFGInfo cfg_info_buf[])
{
    int i = 0;

    LOGD("%s, cfg_info_item_count = %d\n", "TV", cfg_info_item_count);

    for (i = 0; i < cfg_info_item_count; i++) {
        LOGD("%s, %s %d %d %d\n", "TV", cfg_info_buf[i].gpio_grp_str, cfg_info_buf[i].gpio_addr, cfg_info_buf[i].gpio_val, !cfg_info_buf[i].gpio_val);
    }

    LOGD("\n");
}

static int GetWriteProtectGPIOCFG(char gpio_grp_str[], int *protect_gpio_addr, int *protect_on_val, int *protect_off_val)
{
    if (cfg_get_one_gpio_data("ssm.writeprotect.gpio", gpio_grp_str, protect_gpio_addr, protect_on_val) < 0) {
        return -1;
    }

    *protect_off_val = !(*protect_on_val);

    return 0;
}

#define CC_AMPLIFIER_ANALOG_MUTE_GPIO_CFG_NAME  "audio.amp.analog.mute.gpio"
#define CC_HEADSET_MUTE_GPIO_CFG_NAME  "audio.headset.mute.gpio"
#define CC_AVOUT_MUTE_GPIO_CFG_NAME  "audio.avout.mute.gpio"

static int GetAudioAnalogAmplifierMuteGPIOCFG(char gpio_grp_str[], int *mute_gpio_addr, int *mute_on_val, int *mute_off_val)
{
    if (cfg_get_one_gpio_data(CC_AMPLIFIER_ANALOG_MUTE_GPIO_CFG_NAME, gpio_grp_str, mute_gpio_addr, mute_on_val) < 0) {
        return -1;
    }

    *mute_off_val = !(*mute_on_val);

    return 0;
}

static int GetAudioHeadSetMuteGPIOCFG(char gpio_grp_str[], int *mute_gpio_addr, int *mute_on_val, int *mute_off_val)
{
    if (cfg_get_one_gpio_data(CC_HEADSET_MUTE_GPIO_CFG_NAME, gpio_grp_str, mute_gpio_addr, mute_on_val) < 0) {
        return -1;
    }

    *mute_off_val = !(*mute_on_val);

    return 0;
}

static int GetAudioAVOutMuteGPIOCFG(char gpio_grp_str[], int *mute_gpio_addr, int *mute_on_val, int *mute_off_val)
{
    if (cfg_get_one_gpio_data(CC_AVOUT_MUTE_GPIO_CFG_NAME, gpio_grp_str, mute_gpio_addr, mute_on_val) < 0) {
        return -1;
    }

    //Some projects has been volume production. They may not use the newest code, so we do compatible for old config.
    //old config 69,0,1
    //new config x,69,0

    if (gpio_grp_str[0] != 'x' || gpio_grp_str[0] != 'b' || gpio_grp_str[0] != 'd') {
        *mute_gpio_addr = strtol(gpio_grp_str, NULL, 10);
        gpio_grp_str[0] = 'x';
        gpio_grp_str[1] = '\0';
        *mute_off_val = *mute_on_val;
        *mute_on_val = !(*mute_off_val);
    } else {
        *mute_off_val = !(*mute_on_val);
    }

    return 0;
}

#define PROPERTY_VALUE_MAX  (92)

typedef enum tvin_source_input_e {
    SOURCE_TV,
    SOURCE_AV1,
    SOURCE_AV2,
    SOURCE_YPBPR1,
    SOURCE_YPBPR2,
    SOURCE_HDMI1,
    SOURCE_HDMI2,
    SOURCE_HDMI3,
    SOURCE_VGA,
    SOURCE_MPEG,
    SOURCE_DTV,
    SOURCE_MAX,
} tv_source_input_t;

static int property_set(const char *key_value, char *prop_value)
{
    LOGD("%s, %s = %s\n", "TV", key_value, prop_value);
    return 0;
}

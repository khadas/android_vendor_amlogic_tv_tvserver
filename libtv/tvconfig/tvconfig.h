#ifndef __TVCONFIG_API_H__
#define __TVCONFIG_API_H__

#define CC_CFG_KEY_STR_MAX_LEN                  (128)
#define CC_CFG_VALUE_STR_MAX_LEN                (512)

//for tv config
#define CFG_SECTION_TV                          "TV"
#define CFG_SECTION_ATV                         "ATV"
#define CFG_SECTION_SRC_INPUT                   "SourceInputMap"
#define CFG_SECTION_SETTING                     "SETTING"
#define CFG_SECTION_FBCUART                     "FBCUART"

#define CFG_BLUE_SCREEN_COLOR                   "tvin.bluescreen.color"


#define CFG_SSM_HDMI_AV_DETECT                  "ssm.hdmi_av.hotplug.detect.en"
#define CFG_SSM_HDMI_EDID_EN                    "ssm.handle.hdmi.edid.en"


#define CFG_ATV_FREQ_LIST                       "atv.get.min.max.freq"


#define CFG_TVIN_KERNELPET_DISABLE              "tvin.kernelpet_disable"
#define CFG_TVIN_KERNELPET_TIMEROUT             "tvin.kernelpet.timeout"
#define CFG_TVIN_USERPET                        "tvin.userpet"
#define CFG_TVIN_USERPET_TIMEROUT               "tvin.userpet.timeout"
#define CFG_TVIN_USERPET_RESET                  "tvin.userpet.reset"

#define CFG_TVIN_DISPLAY_FREQ_AUTO              "tvin.autoset.displayfreq"
#define CFG_TVIN_DB_REG                         "tvin.db.reg.en"
#define CFG_TVIN_THERMAL_THRESHOLD_ENABLE       "tvin.thermal.threshold.enable"
#define CFG_TVIN_THERMAL_THRESHOLD_VALUE        "tvin.thermal.threshold.value"
#define CFG_TVIN_THERMAL_FBC_NORMAL_VALUE       "tvin.thermal.fbc.normal.value"
#define CFG_TVIN_THERMAL_FBC_COLD_VALUE         "tvin.thermal.fbc.cold.value"

#define CFG_AUDIO_SRS_SOURROUND_GAIN            "audio.srs.sourround.gain"
#define CFG_AUDIO_SRS_INPUT_GAIN                "audio.srs.input.gain"
#define CFG_AUDIO_SRS_OUTPUT_GAIN               "audio.srs.output.gain"
#define CFG_AUDIO_SRS_TRUBASS_GAIN              "audio.srs.trubass.gain"
#define CFG_AUDIO_SRS_TRUBASS_SPEAKERSIZE       "audio.srs.trubass.speakersize"
#define CFG_AUDIO_SRS_CLARITY_GAIN              "audio.srs.dialogclarity.gain"
#define CFG_AUDIO_SRS_DEFINITION_GAIN           "audio.srs.definition.gain"

#define CFG_AUDIO_SRS_SOURROUND_MASTER_GAIN "audio.srs.sourround.ampmaster.gain"
#define CFG_AUDIO_SRS_CLARITY_MASTER_GAIN       "audio.srs.dialogclarity.ampmaster.gain"
#define CFG_AUDIO_SRS_TRUBASS_MASTER_GAIN       "audio.srs.trubass.ampmaster.gain"
#define CFG_AUDIO_SRS_TRUBASS_CLARITY_MASTER_GAIN "audio.srs.trubass.dialogclarity.ampmaster.gain"


#define CFG_FBC_PANEL_INFO                      "fbc.get.panelinfo"
#define CFG_FBC_USED                            "platform.havefbc"

#define CFG_PQ_DB_PATH                          "pq_db_path"
#define DEF_VALUE_PQ_DB_PATH                    "/param/pq.db"

#define CFG_CAHNNEL_DB                          "tv.channel.db"
#define DEF_VALUE_CAHNNEL_DB                    "/param/dtv.db"

//for tv property
#define PROP_DEF_CAPTURE_NAME                   "snd.card.default.card.capture"
#define PROP_AUDIO_CARD_NUM                     "snd.card.totle.num"

#define UBOOTENV_OUTPUTMODE                     "ubootenv.var.outputmode"
#define UBOOTENV_CONSOLE                        "ubootenv.var.console"

//for tv sysfs
#define SYS_SPDIF_MODE_DEV_PATH                 "/sys/class/audiodsp/digital_raw"
#define SYS_VECM_DNLP_ADJ_LEVEL                 "/sys/module/am_vecm/parameters/dnlp_adj_level"

#define SYS_AUIDO_DSP_AC3_DRC                   "/sys/class/audiodsp/ac3_drc_control"
#define SYS_AUIDO_DIRECT_RIGHT_GAIN             "/sys/class/amaudio2/aml_direct_right_gain"
#define SYS_AUIDO_DIRECT_LEFT_GAIN              "/sys/class/amaudio2/aml_direct_left_gain"

#define SYS_VIDEO_FRAME_HEIGHT                  "/sys/class/video/frame_height"
#define SYS_VIDEO_SCALER_PATH_SEL               "/sys/class/video/video_scaler_path_sel"

#define SYS_DROILOGIC_DEBUG                     "/sys/class/amlogic/debug"
#define SYS_ATV_DEMOD_DEBUG                     "/sys/class/amlatvdemod/atvdemod_debug"

extern int tv_config_load(const char *file_name);
extern int tv_config_unload();
extern int tv_config_save();

//[TV] section

extern int config_set_str(const char *section, const char *key, const char *value);
extern const char *config_get_str(const char *section, const char *key, const char *def_value);
extern int config_get_int(const char *section, const char *key, const int def_value);
extern int config_set_int(const char *section, const char *key, const int value);


#endif //__TVCONFIG_API_H__

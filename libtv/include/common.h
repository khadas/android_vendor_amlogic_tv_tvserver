

#ifndef __COMMON_H__
#define __COMMON_H__

//CVpp.h
typedef enum vpp_picture_mode_e {
    VPP_PICTURE_MODE_STANDARD,
    VPP_PICTURE_MODE_BRIGHT,
    VPP_PICTURE_MODE_SOFT,
    VPP_PICTURE_MODE_USER,
    VPP_PICTURE_MODE_MOVIE,
    VPP_PICTURE_MODE_COLORFUL,
    VPP_PICTURE_MODE_MONITOR,
    VPP_PICTURE_MODE_GAME,
    VPP_PICTURE_MODE_SPORTS,
    VPP_PICTURE_MODE_SONY,
    VPP_PICTURE_MODE_SAMSUNG,
    VPP_PICTURE_MODE_SHARP,
    VPP_PICTURE_MODE_MAX,
} vpp_picture_mode_t;

//CTvin.h
typedef enum tv_source_input_e {
    SOURCE_INVALID = -1,
    SOURCE_TV = 0,
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
    SOURCE_SVIDEO,
    SOURCE_IPTV,
    SOURCE_DUMMY,
    SOURCE_SPDIF,
    SOURCE_ADTV,
    SOURCE_MAX,
} tv_source_input_t;


#endif
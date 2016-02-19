/*
 * TVIN Modules Exported Header File
 *
 * Author: Lin Xu <lin.xu@amlogic.com>
 *         Bobby Yang <bo.yang@amlogic.com>
 *
 * Copyright (C) 2010 Amlogic Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _CTVIN_H
#define _CTVIN_H


#include <pthread.h>
#include <cm.h>
#include <ve.h>
#include "../tvutils/CThread.h"
#include <hdmirx_cec.h>
#include "../tv/CFrontEnd.h"

#define DEPTH_LEVEL_2DTO3D 33
static const int DepthTable_2DTO3D[DEPTH_LEVEL_2DTO3D] = {
    -64, // -16
    -60, // -15
    -56, // -14
    -52, // -13
    -49, // -12
    -46, // -11
    -43, // -10
    -40, // -09
    -37, // -08
    -34, // -07
    -31, // -06
    -28, // -05
    -25, // -04
    -22, // -03
    -19, // -02
    -16, // -01
    -13, // 0
    3, // 1
    6, // 2
    9, // 3
    12, // 4
    15, // 5
    18, // 6
    21, // 7
    24, // 8
    28, // 9
    32, // 10
    36, // 11
    40, // 12
    44, // 13
    48, // 14
    52, // 15
    56, // 16
};

enum {
    MEMP_VDIN_WITHOUT_3D = 0,
    MEMP_VDIN_WITH_3D,
    MEMP_DCDR_WITHOUT_3D,
    MEMP_DCDR_WITH_3D,
    MEMP_ATV_WITHOUT_3D,
    MEMP_ATV_WITH_3D,
};

// ***************************************************************************
// *** TVIN general definition/enum/struct ***********************************
// ***************************************************************************
/* tvin input port select */
typedef enum tvin_port_e {
    TVIN_PORT_NULL    = 0x00000000,
    TVIN_PORT_MPEG0   = 0x00000100,
    TVIN_PORT_BT656   = 0x00000200,
    TVIN_PORT_BT601,
    TVIN_PORT_CAMERA,
    TVIN_PORT_VGA0    = 0x00000400,
    TVIN_PORT_VGA1,
    TVIN_PORT_VGA2,
    TVIN_PORT_VGA3,
    TVIN_PORT_VGA4,
    TVIN_PORT_VGA5,
    TVIN_PORT_VGA6,
    TVIN_PORT_VGA7,
    TVIN_PORT_COMP0   = 0x00000800,
    TVIN_PORT_COMP1,
    TVIN_PORT_COMP2,
    TVIN_PORT_COMP3,
    TVIN_PORT_COMP4,
    TVIN_PORT_COMP5,
    TVIN_PORT_COMP6,
    TVIN_PORT_COMP7,
    TVIN_PORT_CVBS0   = 0x00001000,
    TVIN_PORT_CVBS1,
    TVIN_PORT_CVBS2,
    TVIN_PORT_CVBS3,    //as atv demod to tvafe
    TVIN_PORT_CVBS4,
    TVIN_PORT_CVBS5,
    TVIN_PORT_CVBS6,
    TVIN_PORT_CVBS7,
    TVIN_PORT_SVIDEO0 = 0x00002000,
    TVIN_PORT_SVIDEO1,
    TVIN_PORT_SVIDEO2,
    TVIN_PORT_SVIDEO3,
    TVIN_PORT_SVIDEO4,
    TVIN_PORT_SVIDEO5,
    TVIN_PORT_SVIDEO6,
    TVIN_PORT_SVIDEO7,
    TVIN_PORT_HDMI0   = 0x00004000,
    TVIN_PORT_HDMI1,
    TVIN_PORT_HDMI2,
    TVIN_PORT_HDMI3,
    TVIN_PORT_HDMI4,
    TVIN_PORT_HDMI5,
    TVIN_PORT_HDMI6,
    TVIN_PORT_HDMI7,
    TVIN_PORT_DVIN0   = 0x00008000,
    TVIN_PORT_VIU     = 0x0000C000,
    TVIN_PORT_MIPI    = 0x00010000,
    TVIN_PORT_ISP     = 0x00020000,
    TVIN_PORT_DTV     = 0x00040000,
    TVIN_PORT_MAX     = 0x80000000,
} tvin_port_t;

const char *tvin_port_str(enum tvin_port_e port);


/* tvin signal format table */
typedef enum tvin_sig_fmt_e {
    TVIN_SIG_FMT_NULL = 0,
    //VGA Formats
    TVIN_SIG_FMT_VGA_512X384P_60HZ_D147             = 0x001,
    TVIN_SIG_FMT_VGA_560X384P_60HZ_D147             = 0x002,
    TVIN_SIG_FMT_VGA_640X200P_59HZ_D924             = 0x003,
    TVIN_SIG_FMT_VGA_640X350P_85HZ_D080             = 0x004,
    TVIN_SIG_FMT_VGA_640X400P_59HZ_D940             = 0x005,
    TVIN_SIG_FMT_VGA_640X400P_85HZ_D080             = 0x006,
    TVIN_SIG_FMT_VGA_640X400P_59HZ_D638             = 0x007,
    TVIN_SIG_FMT_VGA_640X400P_56HZ_D416             = 0x008,
    TVIN_SIG_FMT_VGA_640X480P_66HZ_D619             = 0x009,
    TVIN_SIG_FMT_VGA_640X480P_66HZ_D667             = 0x00a,
    TVIN_SIG_FMT_VGA_640X480P_59HZ_D940             = 0x00b,
    TVIN_SIG_FMT_VGA_640X480P_60HZ_D000             = 0x00c,
    TVIN_SIG_FMT_VGA_640X480P_72HZ_D809             = 0x00d,
    TVIN_SIG_FMT_VGA_640X480P_75HZ_D000_A           = 0x00e,
    TVIN_SIG_FMT_VGA_640X480P_85HZ_D008             = 0x00f,
    TVIN_SIG_FMT_VGA_640X480P_59HZ_D638             = 0x010,
    TVIN_SIG_FMT_VGA_640X480P_75HZ_D000_B           = 0x011,
    TVIN_SIG_FMT_VGA_640X870P_75HZ_D000             = 0x012,
    TVIN_SIG_FMT_VGA_720X350P_70HZ_D086             = 0x013,
    TVIN_SIG_FMT_VGA_720X400P_85HZ_D039             = 0x014,
    TVIN_SIG_FMT_VGA_720X400P_70HZ_D086             = 0x015,
    TVIN_SIG_FMT_VGA_720X400P_87HZ_D849             = 0x016,
    TVIN_SIG_FMT_VGA_720X400P_59HZ_D940             = 0x017,
    TVIN_SIG_FMT_VGA_720X480P_59HZ_D940             = 0x018,
    TVIN_SIG_FMT_VGA_768X480P_59HZ_D896             = 0x019,
    TVIN_SIG_FMT_VGA_800X600P_56HZ_D250             = 0x01a,
    TVIN_SIG_FMT_VGA_800X600P_60HZ_D000             = 0x01b,
    TVIN_SIG_FMT_VGA_800X600P_60HZ_D000_A           = 0x01c,
    TVIN_SIG_FMT_VGA_800X600P_60HZ_D317             = 0x01d,
    TVIN_SIG_FMT_VGA_800X600P_72HZ_D188             = 0x01e,
    TVIN_SIG_FMT_VGA_800X600P_75HZ_D000             = 0x01f,
    TVIN_SIG_FMT_VGA_800X600P_85HZ_D061             = 0x020,
    TVIN_SIG_FMT_VGA_832X624P_75HZ_D087             = 0x021,
    TVIN_SIG_FMT_VGA_848X480P_84HZ_D751             = 0x022,
    TVIN_SIG_FMT_VGA_960X600P_59HZ_D635             = 0x023,
    TVIN_SIG_FMT_VGA_1024X768P_59HZ_D278            = 0x024,
    TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000            = 0x025,
    TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_A          = 0x026,
    TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_B          = 0x027,
    TVIN_SIG_FMT_VGA_1024X768P_74HZ_D927            = 0x028,
    TVIN_SIG_FMT_VGA_1024X768P_60HZ_D004            = 0x029,
    TVIN_SIG_FMT_VGA_1024X768P_70HZ_D069            = 0x02a,
    TVIN_SIG_FMT_VGA_1024X768P_75HZ_D029            = 0x02b,
    TVIN_SIG_FMT_VGA_1024X768P_84HZ_D997            = 0x02c,
    TVIN_SIG_FMT_VGA_1024X768P_74HZ_D925            = 0x02d,
    TVIN_SIG_FMT_VGA_1024X768P_60HZ_D020            = 0x02e,
    TVIN_SIG_FMT_VGA_1024X768P_70HZ_D008            = 0x02f,
    TVIN_SIG_FMT_VGA_1024X768P_75HZ_D782            = 0x030,
    TVIN_SIG_FMT_VGA_1024X768P_77HZ_D069            = 0x031,
    TVIN_SIG_FMT_VGA_1024X768P_71HZ_D799            = 0x032,
    TVIN_SIG_FMT_VGA_1024X1024P_60HZ_D000           = 0x033,
    TVIN_SIG_FMT_VGA_1152X864P_60HZ_D000            = 0x034,
    TVIN_SIG_FMT_VGA_1152X864P_70HZ_D012            = 0x035,
    TVIN_SIG_FMT_VGA_1152X864P_75HZ_D000            = 0x036,
    TVIN_SIG_FMT_VGA_1152X864P_84HZ_D999            = 0x037,
    TVIN_SIG_FMT_VGA_1152X870P_75HZ_D062            = 0x038,
    TVIN_SIG_FMT_VGA_1152X900P_65HZ_D950            = 0x039,
    TVIN_SIG_FMT_VGA_1152X900P_66HZ_D004            = 0x03a,
    TVIN_SIG_FMT_VGA_1152X900P_76HZ_D047            = 0x03b,
    TVIN_SIG_FMT_VGA_1152X900P_76HZ_D149            = 0x03c,
    TVIN_SIG_FMT_VGA_1280X720P_59HZ_D855            = 0x03d,
    TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_A          = 0x03e,
    TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_B          = 0x03f,
    TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_C          = 0x040,
    TVIN_SIG_FMT_VGA_1280X720P_60HZ_D000_D          = 0x041,
    TVIN_SIG_FMT_VGA_1280X768P_59HZ_D870            = 0x042,
    TVIN_SIG_FMT_VGA_1280X768P_59HZ_D995            = 0x043,
    TVIN_SIG_FMT_VGA_1280X768P_60HZ_D100            = 0x044,
    TVIN_SIG_FMT_VGA_1280X768P_85HZ_D000            = 0x045,
    TVIN_SIG_FMT_VGA_1280X768P_74HZ_D893            = 0x046,
    TVIN_SIG_FMT_VGA_1280X768P_84HZ_D837            = 0x047,
    TVIN_SIG_FMT_VGA_1280X800P_59HZ_D810            = 0x048,
    TVIN_SIG_FMT_VGA_1280X800P_59HZ_D810_A          = 0x049,
    TVIN_SIG_FMT_VGA_1280X800P_60HZ_D000            = 0x04a,
    TVIN_SIG_FMT_VGA_1280X800P_85HZ_D000            = 0x04b,
    TVIN_SIG_FMT_VGA_1280X960P_60HZ_D000            = 0x04c,
    TVIN_SIG_FMT_VGA_1280X960P_60HZ_D000_A          = 0x04d,
    TVIN_SIG_FMT_VGA_1280X960P_75HZ_D000            = 0x04e,
    TVIN_SIG_FMT_VGA_1280X960P_85HZ_D002            = 0x04f,
    TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D020           = 0x050,
    TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D020_A         = 0x051,
    TVIN_SIG_FMT_VGA_1280X1024P_75HZ_D025           = 0x052,
    TVIN_SIG_FMT_VGA_1280X1024P_85HZ_D024           = 0x053,
    TVIN_SIG_FMT_VGA_1280X1024P_59HZ_D979           = 0x054,
    TVIN_SIG_FMT_VGA_1280X1024P_72HZ_D005           = 0x055,
    TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D002           = 0x056,
    TVIN_SIG_FMT_VGA_1280X1024P_67HZ_D003           = 0x057,
    TVIN_SIG_FMT_VGA_1280X1024P_74HZ_D112           = 0x058,
    TVIN_SIG_FMT_VGA_1280X1024P_76HZ_D179           = 0x059,
    TVIN_SIG_FMT_VGA_1280X1024P_66HZ_D718           = 0x05a,
    TVIN_SIG_FMT_VGA_1280X1024P_66HZ_D677           = 0x05b,
    TVIN_SIG_FMT_VGA_1280X1024P_76HZ_D107           = 0x05c,
    TVIN_SIG_FMT_VGA_1280X1024P_59HZ_D996           = 0x05d,
    TVIN_SIG_FMT_VGA_1280X1024P_60HZ_D000           = 0x05e,
    TVIN_SIG_FMT_VGA_1360X768P_59HZ_D799            = 0x05f,
    TVIN_SIG_FMT_VGA_1360X768P_60HZ_D015            = 0x060,
    TVIN_SIG_FMT_VGA_1360X768P_60HZ_D015_A          = 0x061,
    TVIN_SIG_FMT_VGA_1360X850P_60HZ_D000            = 0x062,
    TVIN_SIG_FMT_VGA_1360X1024P_60HZ_D000           = 0x063,
    TVIN_SIG_FMT_VGA_1366X768P_59HZ_D790            = 0x064,
    TVIN_SIG_FMT_VGA_1366X768P_60HZ_D000            = 0x065,
    TVIN_SIG_FMT_VGA_1400X1050P_59HZ_D978           = 0x066,
    TVIN_SIG_FMT_VGA_1440X900P_59HZ_D887            = 0x067,
    TVIN_SIG_FMT_VGA_1440X1080P_60HZ_D000           = 0x068,
    TVIN_SIG_FMT_VGA_1600X900P_60HZ_D000            = 0x069,
    TVIN_SIG_FMT_VGA_1600X1024P_60HZ_D000           = 0x06a,
    TVIN_SIG_FMT_VGA_1600X1200P_59HZ_D869           = 0x06b,
    TVIN_SIG_FMT_VGA_1600X1200P_60HZ_D000           = 0x06c,
    TVIN_SIG_FMT_VGA_1600X1200P_65HZ_D000           = 0x06d,
    TVIN_SIG_FMT_VGA_1600X1200P_70HZ_D000           = 0x06e,
    TVIN_SIG_FMT_VGA_1680X1050P_59HZ_D954           = 0x06f,
    TVIN_SIG_FMT_VGA_1680X1080P_60HZ_D000           = 0x070,
    TVIN_SIG_FMT_VGA_1920X1080P_49HZ_D929           = 0x071,
    TVIN_SIG_FMT_VGA_1920X1080P_59HZ_D963_A         = 0x072,
    TVIN_SIG_FMT_VGA_1920X1080P_59HZ_D963           = 0x073,
    TVIN_SIG_FMT_VGA_1920X1080P_60HZ_D000           = 0x074,
    TVIN_SIG_FMT_VGA_1920X1200P_59HZ_D950           = 0x075,
    TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_C          = 0x076,
    TVIN_SIG_FMT_VGA_1024X768P_60HZ_D000_D          = 0x077,
    TVIN_SIG_FMT_VGA_1920X1200P_59HZ_D988           = 0x078,
    TVIN_SIG_FMT_VGA_1400X900P_60HZ_D000            = 0x079,
    TVIN_SIG_FMT_VGA_1680X1050P_60HZ_D000           = 0x07a,
    TVIN_SIG_FMT_VGA_800X600P_60HZ_D062             = 0x07b,
    TVIN_SIG_FMT_VGA_800X600P_60HZ_317_B            = 0x07c,
    TVIN_SIG_FMT_VGA_RESERVE8                       = 0x07d,
    TVIN_SIG_FMT_VGA_RESERVE9                       = 0x07e,
    TVIN_SIG_FMT_VGA_RESERVE10                      = 0x07f,
    TVIN_SIG_FMT_VGA_RESERVE11                      = 0x080,
    TVIN_SIG_FMT_VGA_RESERVE12                      = 0x081,
    TVIN_SIG_FMT_VGA_MAX                            = 0x082,
    TVIN_SIG_FMT_VGA_THRESHOLD                      = 0x200,
    //Component Formats
    TVIN_SIG_FMT_COMP_480P_60HZ_D000                = 0x201,
    TVIN_SIG_FMT_COMP_480I_59HZ_D940                = 0x202,
    TVIN_SIG_FMT_COMP_576P_50HZ_D000                = 0x203,
    TVIN_SIG_FMT_COMP_576I_50HZ_D000                = 0x204,
    TVIN_SIG_FMT_COMP_720P_59HZ_D940                = 0x205,
    TVIN_SIG_FMT_COMP_720P_50HZ_D000                = 0x206,
    TVIN_SIG_FMT_COMP_1080P_23HZ_D976               = 0x207,
    TVIN_SIG_FMT_COMP_1080P_24HZ_D000               = 0x208,
    TVIN_SIG_FMT_COMP_1080P_25HZ_D000               = 0x209,
    TVIN_SIG_FMT_COMP_1080P_30HZ_D000               = 0x20a,
    TVIN_SIG_FMT_COMP_1080P_50HZ_D000               = 0x20b,
    TVIN_SIG_FMT_COMP_1080P_60HZ_D000               = 0x20c,
    TVIN_SIG_FMT_COMP_1080I_47HZ_D952               = 0x20d,
    TVIN_SIG_FMT_COMP_1080I_48HZ_D000               = 0x20e,
    TVIN_SIG_FMT_COMP_1080I_50HZ_D000_A             = 0x20f,
    TVIN_SIG_FMT_COMP_1080I_50HZ_D000_B             = 0x210,
    TVIN_SIG_FMT_COMP_1080I_50HZ_D000_C             = 0x211,
    TVIN_SIG_FMT_COMP_1080I_60HZ_D000               = 0x212,
    TVIN_SIG_FMT_COMP_MAX                           = 0x213,
    TVIN_SIG_FMT_COMP_THRESHOLD                     = 0x400,
    //HDMI Formats
    TVIN_SIG_FMT_HDMI_640X480P_60HZ                 = 0x401,
    TVIN_SIG_FMT_HDMI_720X480P_60HZ                 = 0x402,
    TVIN_SIG_FMT_HDMI_1280X720P_60HZ                = 0x403,
    TVIN_SIG_FMT_HDMI_1920X1080I_60HZ               = 0x404,
    TVIN_SIG_FMT_HDMI_1440X480I_60HZ                = 0x405,
    TVIN_SIG_FMT_HDMI_1440X240P_60HZ                = 0x406,
    TVIN_SIG_FMT_HDMI_2880X480I_60HZ                = 0x407,
    TVIN_SIG_FMT_HDMI_2880X240P_60HZ                = 0x408,
    TVIN_SIG_FMT_HDMI_1440X480P_60HZ                = 0x409,
    TVIN_SIG_FMT_HDMI_1920X1080P_60HZ               = 0x40a,
    TVIN_SIG_FMT_HDMI_720X576P_50HZ                 = 0x40b,
    TVIN_SIG_FMT_HDMI_1280X720P_50HZ                = 0x40c,
    TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_A             = 0x40d,
    TVIN_SIG_FMT_HDMI_1440X576I_50HZ                = 0x40e,
    TVIN_SIG_FMT_HDMI_1440X288P_50HZ                = 0x40f,
    TVIN_SIG_FMT_HDMI_2880X576I_50HZ                = 0x410,
    TVIN_SIG_FMT_HDMI_2880X288P_50HZ                = 0x411,
    TVIN_SIG_FMT_HDMI_1440X576P_50HZ                = 0x412,
    TVIN_SIG_FMT_HDMI_1920X1080P_50HZ               = 0x413,
    TVIN_SIG_FMT_HDMI_1920X1080P_24HZ               = 0x414,
    TVIN_SIG_FMT_HDMI_1920X1080P_25HZ               = 0x415,
    TVIN_SIG_FMT_HDMI_1920X1080P_30HZ               = 0x416,
    TVIN_SIG_FMT_HDMI_2880X480P_60HZ                = 0x417,
    TVIN_SIG_FMT_HDMI_2880X576P_60HZ                = 0x418,
    TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_B             = 0x419,
    TVIN_SIG_FMT_HDMI_1920X1080I_100HZ              = 0x41a,
    TVIN_SIG_FMT_HDMI_1280X720P_100HZ               = 0x41b,
    TVIN_SIG_FMT_HDMI_720X576P_100HZ                = 0x41c,
    TVIN_SIG_FMT_HDMI_1440X576I_100HZ               = 0x41d,
    TVIN_SIG_FMT_HDMI_1920X1080I_120HZ              = 0x41e,
    TVIN_SIG_FMT_HDMI_1280X720P_120HZ               = 0x41f,
    TVIN_SIG_FMT_HDMI_720X480P_120HZ                = 0x420,
    TVIN_SIG_FMT_HDMI_1440X480I_120HZ               = 0x421,
    TVIN_SIG_FMT_HDMI_720X576P_200HZ                = 0x422,
    TVIN_SIG_FMT_HDMI_1440X576I_200HZ               = 0x423,
    TVIN_SIG_FMT_HDMI_720X480P_240HZ                = 0x424,
    TVIN_SIG_FMT_HDMI_1440X480I_240HZ               = 0x425,
    TVIN_SIG_FMT_HDMI_1280X720P_24HZ                = 0x426,
    TVIN_SIG_FMT_HDMI_1280X720P_25HZ                = 0x427,
    TVIN_SIG_FMT_HDMI_1280X720P_30HZ                = 0x428,
    TVIN_SIG_FMT_HDMI_1920X1080P_120HZ              = 0x429,
    TVIN_SIG_FMT_HDMI_1920X1080P_100HZ              = 0x42a,
    TVIN_SIG_FMT_HDMI_1280X720P_60HZ_FRAME_PACKING  = 0x42b,
    TVIN_SIG_FMT_HDMI_1280X720P_50HZ_FRAME_PACKING  = 0x42c,
    TVIN_SIG_FMT_HDMI_1280X720P_24HZ_FRAME_PACKING  = 0x42d,
    TVIN_SIG_FMT_HDMI_1280X720P_30HZ_FRAME_PACKING  = 0x42e,
    TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_FRAME_PACKING = 0x42f,
    TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_FRAME_PACKING = 0x430,
    TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_FRAME_PACKING = 0x431,
    TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_FRAME_PACKING = 0x432,
    TVIN_SIG_FMT_HDMI_800X600_00HZ                  = 0x433,
    TVIN_SIG_FMT_HDMI_1024X768_00HZ                 = 0x434,
    TVIN_SIG_FMT_HDMI_720X400_00HZ                  = 0x435,
    TVIN_SIG_FMT_HDMI_1280X768_00HZ                 = 0x436,
    TVIN_SIG_FMT_HDMI_1280X800_00HZ                 = 0x437,
    TVIN_SIG_FMT_HDMI_1280X960_00HZ                 = 0x438,
    TVIN_SIG_FMT_HDMI_1280X1024_00HZ                = 0x439,
    TVIN_SIG_FMT_HDMI_1360X768_00HZ                 = 0x43a,
    TVIN_SIG_FMT_HDMI_1366X768_00HZ                 = 0x43b,
    TVIN_SIG_FMT_HDMI_1600X1200_00HZ                = 0x43c,
    TVIN_SIG_FMT_HDMI_1920X1200_00HZ                = 0x43d,
    TVIN_SIG_FMT_HDMI_1440X900_00HZ                 = 0x43e,
    TVIN_SIG_FMT_HDMI_1400X1050_00HZ                = 0x43f,
    TVIN_SIG_FMT_HDMI_1680X1050_00HZ                = 0x440,
    /* for alternative and 4k2k */
    TVIN_SIG_FMT_HDMI_1920X1080I_60HZ_ALTERNATIVE   = 0x441,
    TVIN_SIG_FMT_HDMI_1920X1080I_50HZ_ALTERNATIVE   = 0x442,
    TVIN_SIG_FMT_HDMI_1920X1080P_24HZ_ALTERNATIVE   = 0x443,
    TVIN_SIG_FMT_HDMI_1920X1080P_30HZ_ALTERNATIVE   = 0x444,
    TVIN_SIG_FMT_HDMI_3840_2160_00HZ                = 0x445,
    TVIN_SIG_FMT_HDMI_4096_2160_00HZ                = 0x446,
    TVIN_SIG_FMT_HDMI_RESERVE7                      = 0x447,
    TVIN_SIG_FMT_HDMI_RESERVE8                      = 0x448,
    TVIN_SIG_FMT_HDMI_RESERVE9                      = 0x449,
    TVIN_SIG_FMT_HDMI_RESERVE10                     = 0x44a,
    TVIN_SIG_FMT_HDMI_RESERVE11                     = 0x44b,
    TVIN_SIG_FMT_HDMI_720X480P_60HZ_FRAME_PACKING   = 0x44c,
    TVIN_SIG_FMT_HDMI_720X576P_50HZ_FRAME_PACKING   = 0x44d,
    TVIN_SIG_FMT_HDMI_MAX                           = 0x44e,
    TVIN_SIG_FMT_HDMI_THRESHOLD                     = 0x600,
    //Video Formats
    TVIN_SIG_FMT_CVBS_NTSC_M                        = 0x601,
    TVIN_SIG_FMT_CVBS_NTSC_443                      = 0x602,
    TVIN_SIG_FMT_CVBS_PAL_I                         = 0x603,
    TVIN_SIG_FMT_CVBS_PAL_M                         = 0x604,
    TVIN_SIG_FMT_CVBS_PAL_60                        = 0x605,
    TVIN_SIG_FMT_CVBS_PAL_CN                        = 0x606,
    TVIN_SIG_FMT_CVBS_SECAM                         = 0x607,
    TVIN_SIG_FMT_CVBS_MAX                           = 0x608,
    TVIN_SIG_FMT_CVBS_THRESHOLD                     = 0x800,
    //656 Formats
    TVIN_SIG_FMT_BT656IN_576I_50HZ                  = 0x801,
    TVIN_SIG_FMT_BT656IN_480I_60HZ                  = 0x802,
    //601 Formats
    TVIN_SIG_FMT_BT601IN_576I_50HZ                  = 0x803,
    TVIN_SIG_FMT_BT601IN_480I_60HZ                  = 0x804,
    //Camera Formats
    TVIN_SIG_FMT_CAMERA_640X480P_30HZ               = 0x805,
    TVIN_SIG_FMT_CAMERA_800X600P_30HZ               = 0x806,
    TVIN_SIG_FMT_CAMERA_1024X768P_30HZ              = 0x807,
    TVIN_SIG_FMT_CAMERA_1920X1080P_30HZ             = 0x808,
    TVIN_SIG_FMT_CAMERA_1280X720P_30HZ              = 0x809,
    TVIN_SIG_FMT_BT601_MAX                          = 0x80a,
    TVIN_SIG_FMT_BT601_THRESHOLD                    = 0xa00,
    TVIN_SIG_FMT_MAX,
} tvin_sig_fmt_t;

//tvin signal status
typedef enum tvin_sig_status_e {
    TVIN_SIG_STATUS_NULL = 0, // processing status from init to the finding of the 1st confirmed status
    TVIN_SIG_STATUS_NOSIG,    // no signal - physically no signal
    TVIN_SIG_STATUS_UNSTABLE, // unstable - physically bad signal
    TVIN_SIG_STATUS_NOTSUP,   // not supported - physically good signal & not supported
    TVIN_SIG_STATUS_STABLE,   // stable - physically good signal & supported
} tvin_sig_status_t;

const char *tvin_sig_status_str(enum tvin_sig_status_e status);

// tvin parameters
#define TVIN_PARM_FLAG_CAP      0x00000001 //tvin_parm_t.flag[ 0]: 1/enable or 0/disable frame capture function
#define TVIN_PARM_FLAG_CAL      0x00000002 //tvin_parm_t.flag[ 1]: 1/enable or 0/disable adc calibration
/*used for processing 3d in ppmgr set this flag to drop one field and send real height in vframe*/
#define TVIN_PARM_FLAG_2D_TO_3D 0x00000004 //tvin_parm_t.flag[ 2]: 1/enable or 0/disable 2D->3D mode

typedef enum tvin_trans_fmt {
    TVIN_TFMT_2D = 0,
    TVIN_TFMT_3D_LRH_OLOR,  // 1 Primary: Side-by-Side(Half) Odd/Left picture, Odd/Right p
    TVIN_TFMT_3D_LRH_OLER,  // 2 Primary: Side-by-Side(Half) Odd/Left picture, Even/Right picture
    TVIN_TFMT_3D_LRH_ELOR,  // 3 Primary: Side-by-Side(Half) Even/Left picture, Odd/Right picture
    TVIN_TFMT_3D_LRH_ELER,  // 4 Primary: Side-by-Side(Half) Even/Left picture, Even/Right picture
    TVIN_TFMT_3D_TB,   // 5 Primary: Top-and-Bottom
    TVIN_TFMT_3D_FP,   // 6 Primary: Frame Packing
    TVIN_TFMT_3D_FA,   // 7 Secondary: Field Alternative
    TVIN_TFMT_3D_LA,   // 8 Secondary: Line Alternative
    TVIN_TFMT_3D_LRF,  // 9 Secondary: Side-by-Side(Full)
    TVIN_TFMT_3D_LD,   // 10 Secondary: L+depth
    TVIN_TFMT_3D_LDGD, // 11 Secondary: L+depth+Graphics+Graphics-depth
    /* normal 3D format */
    TVIN_TFMT_3D_DET_TB,// 12
    TVIN_TFMT_3D_DET_LR,// 13
    TVIN_TFMT_3D_DET_INTERLACE,// 14
    TVIN_TFMT_3D_DET_CHESSBOARD,// 15
    TVIN_TFMT_3D_MAX,
} tvin_trans_fmt_t;

const char *tvin_trans_fmt_str(enum tvin_trans_fmt trans_fmt);

typedef enum tvin_color_fmt_e {
    TVIN_RGB444 = 0,
    TVIN_YUV422, // 1
    TVIN_YUV444, // 2
    TVIN_YUYV422,// 3
    TVIN_YVYU422,// 4
    TVIN_UYVY422,// 5
    TVIN_VYUY422,// 6
    TVIN_NV12,   // 7
    TVIN_NV21,   // 8
    TVIN_BGGR,   // 9  raw data
    TVIN_RGGB,   // 10 raw data
    TVIN_GBRG,   // 11 raw data
    TVIN_GRBG,   // 12 raw data
    TVIN_COLOR_FMT_MAX,
} tvin_color_fmt_t;

const char *tvin_color_fmt_str(enum tvin_color_fmt_e color_fmt);
typedef enum tvin_scan_mode_e {
    TVIN_SCAN_MODE_NULL = 0,
    TVIN_SCAN_MODE_PROGRESSIVE,
    TVIN_SCAN_MODE_INTERLACED,
} tvin_scan_mode_t;

typedef struct tvin_info_s {
    enum tvin_trans_fmt    trans_fmt;
    enum tvin_sig_fmt_e    fmt;
    volatile enum tvin_sig_status_e status;
    enum tvin_color_fmt_e  cfmt;
    unsigned int           fps;
    unsigned int           reserved;
} tvin_info_t;

typedef struct tvin_buf_info_s {
    unsigned int vf_size;
    unsigned int buf_count;
    unsigned int buf_width;
    unsigned int buf_height;
    unsigned int buf_size;
    unsigned int wr_list_size;
} tvin_buf_info_t;

typedef struct tvin_video_buf_s {
    unsigned int index;
    unsigned int reserved;
} tvin_video_buf_t;

// hs=he=vs=ve=0 is to disable Cut Window
typedef struct tvin_cutwin_s {
    unsigned short hs;
    unsigned short he;
    unsigned short vs;
    unsigned short ve;
} tvin_cutwin_t;

typedef struct tvin_parm_s {
    int                         index;    // index of frontend for vdin
    enum tvin_port_e            port;     // must set port in IOCTL
    struct tvin_info_s          info;
    unsigned int                hist_pow;
    unsigned int                luma_sum;
    unsigned int                pixel_sum;
    unsigned short              histgram[64];
    unsigned int                flag;
    unsigned short              dest_width;//for vdin horizontal scale down
    unsigned short              dest_height;//for vdin vertical scale down
    bool                h_reverse;//for vdin horizontal reverse
    bool                v_reverse;//for vdin vertical reverse
    unsigned int                reserved;
} tvin_parm_t;



// ***************************************************************************
// *** AFE module definition/enum/struct *************************************
// ***************************************************************************

typedef enum tvafe_cmd_status_e {
    TVAFE_CMD_STATUS_IDLE = 0,   // idle, be ready for TVIN_IOC_S_AFE_VGA_AUTO command
    TVAFE_CMD_STATUS_PROCESSING, // TVIN_IOC_S_AFE_VGA_AUTO command is in process
    TVAFE_CMD_STATUS_SUCCESSFUL, // TVIN_IOC_S_AFE_VGA_AUTO command is done with success
    TVAFE_CMD_STATUS_FAILED,     // TVIN_IOC_S_AFE_VGA_AUTO command is done with failure
    TVAFE_CMD_STATUS_TERMINATED, // TVIN_IOC_S_AFE_VGA_AUTO command is terminated by others related
} tvafe_cmd_status_t;

typedef struct tvafe_vga_edid_s {
    unsigned char value[256]; //256 byte EDID
} tvafe_vga_edid_t;

typedef struct tvafe_comp_wss_s {
    unsigned int wss1[5];
    unsigned int wss2[5];
} tvafe_comp_wss_t;

typedef struct tvafe_vga_parm_s {
    signed short clk_step;  // clock < 0, tune down clock freq
    // clock > 0, tune up clock freq
    unsigned short phase;     // phase is 0~31, it is absolute value
    signed short hpos_step; // hpos_step < 0, shift display to left
    // hpos_step > 0, shift display to right
    signed short vpos_step; // vpos_step < 0, shift display to top
    // vpos_step > 0, shift display to bottom
    unsigned int   vga_in_clean;  // flage for vga clean screen
} tvafe_vga_parm_t;

#define TVAFE_ADC_CAL_VALID 0x00000001
typedef struct tvafe_adc_cal_s {
    // ADC A
    unsigned short a_analog_clamp;    // 0x00~0x7f
    unsigned short a_analog_gain;     // 0x00~0xff, means 0dB~6dB
    unsigned short a_digital_offset1; // offset for fine-tuning
    // s11.0:   signed value, 11 integer bits,  0 fraction bits
    unsigned short a_digital_gain;    // 0~3.999
    // u2.10: unsigned value,  2 integer bits, 10 fraction bits
    unsigned short a_digital_offset2; // offset for format
    // s11.0:   signed value, 11 integer bits,  0 fraction bits
    // ADC B
    unsigned short b_analog_clamp;    // ditto to ADC A
    unsigned short b_analog_gain;
    unsigned short b_digital_offset1;
    unsigned short b_digital_gain;
    unsigned short b_digital_offset2;
    // ADC C
    unsigned short c_analog_clamp;    // ditto to ADC A
    unsigned short c_analog_gain;
    unsigned short c_digital_offset1;
    unsigned short c_digital_gain;
    unsigned short c_digital_offset2;
    // ADC D
    unsigned short d_analog_clamp;    // ditto to ADC A
    unsigned short d_analog_gain;
    unsigned short d_digital_offset1;
    unsigned short d_digital_gain;
    unsigned short d_digital_offset2;
    unsigned int   reserved;          // bit[ 0]: TVAFE_ADC_CAL_VALID
} tvafe_adc_cal_t;

typedef struct tvafe_adc_cal_clamp_s {
    short a_analog_clamp_diff;
    short b_analog_clamp_diff;
    short c_analog_clamp_diff;
} tvafe_adc_cal_clamp_t;

typedef struct tvafe_adc_comp_cal_s {
    struct tvafe_adc_cal_s comp_cal_val[3];
} tvafe_adc_comp_cal_t;

typedef enum tvafe_cvbs_video_e {
    TVAFE_CVBS_VIDEO_HV_UNLOCKED = 0,
    TVAFE_CVBS_VIDEO_H_LOCKED,
    TVAFE_CVBS_VIDEO_V_LOCKED,
    TVAFE_CVBS_VIDEO_HV_LOCKED,
} tvafe_cvbs_video_t;

// for pin selection
typedef enum tvafe_adc_pin_e {
    TVAFE_ADC_PIN_NULL = 0,
#if (MESON_CPU_TYPE == MESON_CPU_TYPE_MESONG9TV)
    TVAFE_CVBS_IN0      = 1,
    TVAFE_CVBS_IN1      = 2,
    TVAFE_CVBS_IN2      = 3,
    TVAFE_CVBS_IN3      = 4,//as atvdemod to tvafe
#else
    TVAFE_ADC_PIN_A_PGA_0   = 1,
    TVAFE_ADC_PIN_A_PGA_1   = 2,
    TVAFE_ADC_PIN_A_PGA_2   = 3,
    TVAFE_ADC_PIN_A_PGA_3   = 4,
    TVAFE_ADC_PIN_A_PGA_4   = 5,
    TVAFE_ADC_PIN_A_PGA_5   = 6,
    TVAFE_ADC_PIN_A_PGA_6   = 7,
    TVAFE_ADC_PIN_A_PGA_7   = 8,
    TVAFE_ADC_PIN_A_0   = 9,
    TVAFE_ADC_PIN_A_1   = 10,
    TVAFE_ADC_PIN_A_2   = 11,
    TVAFE_ADC_PIN_A_3   = 12,
    TVAFE_ADC_PIN_A_4   = 13,
    TVAFE_ADC_PIN_A_5   = 14,
    TVAFE_ADC_PIN_A_6   = 15,
    TVAFE_ADC_PIN_A_7   = 16,
    TVAFE_ADC_PIN_B_0   = 17,
    TVAFE_ADC_PIN_B_1   = 18,
    TVAFE_ADC_PIN_B_2   = 19,
    TVAFE_ADC_PIN_B_3   = 20,
    TVAFE_ADC_PIN_B_4   = 21,
    TVAFE_ADC_PIN_B_5   = 22,
    TVAFE_ADC_PIN_B_6   = 23,
    TVAFE_ADC_PIN_B_7   = 24,
    TVAFE_ADC_PIN_C_0   = 25,
    TVAFE_ADC_PIN_C_1   = 26,
    TVAFE_ADC_PIN_C_2   = 27,
    TVAFE_ADC_PIN_C_3   = 28,
    TVAFE_ADC_PIN_C_4   = 29,
    TVAFE_ADC_PIN_C_5   = 30,
    TVAFE_ADC_PIN_C_6   = 31,
    TVAFE_ADC_PIN_C_7   = 32,
    TVAFE_ADC_PIN_D_0   = 33,
    TVAFE_ADC_PIN_D_1   = 34,
    TVAFE_ADC_PIN_D_2   = 35,
    TVAFE_ADC_PIN_D_3   = 36,
    TVAFE_ADC_PIN_D_4   = 37,
    TVAFE_ADC_PIN_D_5   = 38,
    TVAFE_ADC_PIN_D_6   = 39,
    TVAFE_ADC_PIN_D_7   = 40,
    TVAFE_ADC_PIN_SOG_0 = 41,
    TVAFE_ADC_PIN_SOG_1 = 42,
    TVAFE_ADC_PIN_SOG_2 = 43,
    TVAFE_ADC_PIN_SOG_3 = 44,
    TVAFE_ADC_PIN_SOG_4 = 45,
    TVAFE_ADC_PIN_SOG_5 = 46,
    TVAFE_ADC_PIN_SOG_6 = 47,
    TVAFE_ADC_PIN_SOG_7 = 48,
#endif
    TVAFE_ADC_PIN_MAX,
} tvafe_adc_pin_t;

typedef enum tvafe_src_sig_e {
#if (MESON_CPU_TYPE == MESON_CPU_TYPE_MESONG9TV)
    CVBS_IN0 = 0,
    CVBS_IN1,
    CVBS_IN2,
    CVBS_IN3,
#else
    CVBS0_Y = 0,
    CVBS0_SOG,
    CVBS1_Y,
    CVBS1_SOG,
    CVBS2_Y,
    CVBS2_SOG,
    CVBS3_Y,
    CVBS3_SOG,
    CVBS4_Y,
    CVBS4_SOG,
    CVBS5_Y,
    CVBS5_SOG,
    CVBS6_Y,
    CVBS6_SOG,
    CVBS7_Y,
    CVBS7_SOG,
    S_VIDEO0_Y,
    S_VIDEO0_C,
    S_VIDEO0_SOG,
    S_VIDEO1_Y,
    S_VIDEO1_C,
    S_VIDEO1_SOG,
    S_VIDEO2_Y,
    S_VIDEO2_C,
    S_VIDEO2_SOG,
    S_VIDEO3_Y,
    S_VIDEO3_C,
    S_VIDEO3_SOG,
    S_VIDEO4_Y,
    S_VIDEO4_C,
    S_VIDEO4_SOG,
    S_VIDEO5_Y,
    S_VIDEO5_C,
    S_VIDEO5_SOG,
    S_VIDEO6_Y,
    S_VIDEO6_C,
    S_VIDEO6_SOG,
    S_VIDEO7_Y,
    S_VIDEO7_C,
    S_VIDEO7_SOG,
    VGA0_G,
    VGA0_B,
    VGA0_R,
    VGA0_SOG,
    VGA1_G,
    VGA1_B,
    VGA1_R,
    VGA1_SOG,
    VGA2_G,
    VGA2_B,
    VGA2_R,
    VGA2_SOG,
    VGA3_G,
    VGA3_B,
    VGA3_R,
    VGA3_SOG,
    VGA4_G,
    VGA4_B,
    VGA4_R,
    VGA4_SOG,
    VGA5_G,
    VGA5_B,
    VGA5_R,
    VGA5_SOG,
    VGA6_G,
    VGA6_B,
    VGA6_R,
    VGA6_SOG,
    VGA7_G,
    VGA7_B,
    VGA7_R,
    VGA7_SOG,
    COMP0_Y,
    COMP0_PB,
    COMP0_PR,
    COMP0_SOG,
    COMP1_Y,
    COMP1_PB,
    COMP1_PR,
    COMP1_SOG,
    COMP2_Y,
    COMP2_PB,
    COMP2_PR,
    COMP2_SOG,
    COMP3_Y,
    COMP3_PB,
    COMP3_PR,
    COMP3_SOG,
    COMP4_Y,
    COMP4_PB,
    COMP4_PR,
    COMP4_SOG,
    COMP5_Y,
    COMP5_PB,
    COMP5_PR,
    COMP5_SOG,
    COMP6_Y,
    COMP6_PB,
    COMP6_PR,
    COMP6_SOG,
    COMP7_Y,
    COMP7_PB,
    COMP7_PR,
    COMP7_SOG,
    SCART0_G,
    SCART0_B,
    SCART0_R,
    SCART0_CVBS,
    SCART1_G,
    SCART1_B,
    SCART1_R,
    SCART1_CVBS,
    SCART2_G,
    SCART2_B,
    SCART2_R,
    SCART2_CVBS,
    SCART3_G,
    SCART3_B,
    SCART3_R,
    SCART3_CVBS,
    SCART4_G,
    SCART4_B,
    SCART4_R,
    SCART4_CVBS,
    SCART5_G,
    SCART5_B,
    SCART5_R,
    SCART5_CVBS,
    SCART6_G,
    SCART6_B,
    SCART6_R,
    SCART6_CVBS,
    SCART7_G,
    SCART7_B,
    SCART7_R,
    SCART7_CVBS,
#endif
    TVAFE_SRC_SIG_MAX_NUM,
} tvafe_src_sig_t;

typedef struct tvafe_pin_mux_s {
    enum tvafe_adc_pin_e pin[TVAFE_SRC_SIG_MAX_NUM];
} tvafe_pin_mux_t;


// ***************************************************************************
// *** IOCTL command definition **********************************************
// ***************************************************************************

#define TVIN_IOC_MAGIC 'T'

//GENERAL
#define TVIN_IOC_OPEN               _IOW(TVIN_IOC_MAGIC, 0x01, struct tvin_parm_s)
#define TVIN_IOC_START_DEC          _IOW(TVIN_IOC_MAGIC, 0x02, struct tvin_parm_s)
#define TVIN_IOC_STOP_DEC           _IO( TVIN_IOC_MAGIC, 0x03)
#define TVIN_IOC_CLOSE              _IO( TVIN_IOC_MAGIC, 0x04)
#define TVIN_IOC_G_PARM             _IOR(TVIN_IOC_MAGIC, 0x05, struct tvin_parm_s)
#define TVIN_IOC_S_PARM             _IOW(TVIN_IOC_MAGIC, 0x06, struct tvin_parm_s)
#define TVIN_IOC_G_SIG_INFO         _IOR(TVIN_IOC_MAGIC, 0x07, struct tvin_info_s)
#define TVIN_IOC_G_BUF_INFO         _IOR(TVIN_IOC_MAGIC, 0x08, struct tvin_buf_info_s)
#define TVIN_IOC_START_GET_BUF      _IO( TVIN_IOC_MAGIC, 0x09)
#define TVIN_IOC_GET_BUF            _IOR(TVIN_IOC_MAGIC, 0x10, struct tvin_video_buf_s)
#define TVIN_IOC_PAUSE_DEC          _IO(TVIN_IOC_MAGIC, 0x41)
#define TVIN_IOC_RESUME_DEC         _IO(TVIN_IOC_MAGIC, 0x42)
#define TVIN_IOC_VF_REG             _IO(TVIN_IOC_MAGIC, 0x43)
#define TVIN_IOC_VF_UNREG           _IO(TVIN_IOC_MAGIC, 0x44)
#define TVIN_IOC_FREEZE_VF          _IO(TVIN_IOC_MAGIC, 0x45)
#define TVIN_IOC_UNFREEZE_VF        _IO(TVIN_IOC_MAGIC, 0x46)

//HDMI
#define HDMI_IOC_HDCP_KSV      _IOR(HDMI_IOC_MAGIC, 0x09, struct _hdcp_ksv)



//TVAFE
#define TVIN_IOC_S_AFE_ADC_CAL      _IOW(TVIN_IOC_MAGIC, 0x11, struct tvafe_adc_cal_s)
#define TVIN_IOC_G_AFE_ADC_CAL      _IOR(TVIN_IOC_MAGIC, 0x12, struct tvafe_adc_cal_s)
#define TVIN_IOC_G_AFE_COMP_WSS     _IOR(TVIN_IOC_MAGIC, 0x13, struct tvafe_comp_wss_s)
#define TVIN_IOC_S_AFE_VGA_EDID     _IOW(TVIN_IOC_MAGIC, 0x14, struct tvafe_vga_edid_s)
#define TVIN_IOC_G_AFE_VGA_EDID     _IOR(TVIN_IOC_MAGIC, 0x15, struct tvafe_vga_edid_s)
#define TVIN_IOC_S_AFE_VGA_PARM     _IOW(TVIN_IOC_MAGIC, 0x16, struct tvafe_vga_parm_s)
#define TVIN_IOC_G_AFE_VGA_PARM     _IOR(TVIN_IOC_MAGIC, 0x17, struct tvafe_vga_parm_s)
#define TVIN_IOC_S_AFE_VGA_AUTO     _IO( TVIN_IOC_MAGIC, 0x18)
#define TVIN_IOC_G_AFE_CMD_STATUS   _IOR(TVIN_IOC_MAGIC, 0x19, enum tvafe_cmd_status_e)
#define TVIN_IOC_G_AFE_CVBS_LOCK    _IOR(TVIN_IOC_MAGIC, 0x1a, enum tvafe_cvbs_video_e)
#define TVIN_IOC_S_AFE_CVBS_STD     _IOW(TVIN_IOC_MAGIC, 0x1b, enum tvin_sig_fmt_e)
#define TVIN_IOC_CALLMASTER_SET     _IOW(TVIN_IOC_MAGIC, 0x1c, enum tvin_port_e)
#define TVIN_IOC_CALLMASTER_GET     _IO( TVIN_IOC_MAGIC, 0x1d)
#define TVIN_IOC_S_AFE_ADC_COMP_CAL  _IOW(TVIN_IOC_MAGIC, 0x1e, struct tvafe_adc_comp_cal_s)
#define TVIN_IOC_G_AFE_ADC_COMP_CAL  _IOR(TVIN_IOC_MAGIC, 0x1f, struct tvafe_adc_comp_cal_s)
#define TVIN_IOC_LOAD_REG           _IOW(TVIN_IOC_MAGIC, 0x20, struct am_regs_s)
#define TVIN_IOC_S_AFE_ADC_DIFF     _IOW(TVIN_IOC_MAGIC, 0x21, struct tvafe_adc_cal_clamp_s)

// ***************************************************************************
// *** add more **********************************************
// ***************************************************************************

typedef enum tvin_path_id_e {
    TV_PATH_VDIN_AMVIDEO,
    TV_PATH_VDIN_DEINTERLACE_AMVIDEO,
    TV_PATH_VDIN_3D_AMVIDEO,
    TV_PATH_VDIN_NEW3D_AMVIDEO,
    TV_PATH_VDIN_NEW3D_WITHOUTPPMGR_AMVIDEO,
    TV_PATH_VDIN_FREESCALE_AMVIDEO,
    TV_PATH_DECODER_3D_AMVIDEO,
    TV_PATH_DECODER_AMVIDEO,
    TV_PATH_DECODER_NEW3D_AMVIDEO,
    TV_PATH_DECODER_NEW3D_WITHOUTPPMGR_AMVIDEO,
    TV_PATH_MAX,
} tvin_path_id_t;

#define CAMERA_IOC_MAGIC 'C'
#define CAMERA_IOC_START        _IOW(CAMERA_IOC_MAGIC, 0x01, struct camera_info_s)
#define CAMERA_IOC_STOP         _IO(CAMERA_IOC_MAGIC, 0x02)
#define CAMERA_IOC_SET_PARA     _IOW(CAMERA_IOC_MAGIC, 0x03, struct camera_info_s)
#define CAMERA_IOC_GET_PARA     _IOR(CAMERA_IOC_MAGIC, 0x04, struct camera_info_s)


#define CC_HIST_GRAM_BUF_SIZE   (64)
/*******************************extend define*******************************/

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
    SOURCE_MAX,
} tv_source_input_t;

typedef enum tv_source_input_type_e {
    SOURCE_TYPE_TV,
    SOURCE_TYPE_AV,
    SOURCE_TYPE_COMPONENT,
    SOURCE_TYPE_HDMI,
    SOURCE_TYPE_VGA,
    SOURCE_TYPE_MPEG,
    SOURCE_TYPE_DTV,
    SOURCE_TYPE_SVIDEO,
    SOURCE_TYPE_IPTV,
    SOURCE_TYPE_MAX,
} tv_source_input_type_t;

typedef enum adc_cal_type_e {
    CAL_YPBPR = 0,
    CAL_VGA,
    CAL_CVBS,
} adc_cal_type_t;

typedef enum signal_range_e {
    RANGE100 = 0,
    RANGE75,
} signal_range_t;

typedef struct adc_cal_s {
    unsigned int rcr_max;
    unsigned int rcr_min;
    unsigned int g_y_max;
    unsigned int g_y_min;
    unsigned int bcb_max;
    unsigned int bcb_min;
    unsigned int cr_white;
    unsigned int cb_white;
    unsigned int cr_black;
    unsigned int cb_black;
} adc_cal_t;

typedef struct tvin_window_pos_s {
    int x1;
    int y1;
    int x2;
    int y2;
} tvin_window_pos_t;


typedef enum tv_path_type_e {
    TV_PATH_TYPE_DEFAULT,
    TV_PATH_TYPE_TVIN,
    TV_PATH_TYPE_TVIN_PREVIEW,
    TV_PATH_TYPE_MAX,
} tv_path_type_t;

typedef enum tv_path_status_e {
    TV_PATH_STATUS_NO_DEV = -2,
    TV_PATH_STATUS_ERROR = -1,
    TV_PATH_STATUS_INACTIVE = 0,
    TV_PATH_STATUS_ACTIVE = 1,
    TV_PATH_STATUS_MAX,
} tv_path_status_t;

typedef enum tv_audio_channel_e {
    TV_AUDIO_LINE_IN_0,
    TV_AUDIO_LINE_IN_1,
    TV_AUDIO_LINE_IN_2,
    TV_AUDIO_LINE_IN_3,
    TV_AUDIO_LINE_IN_4,
    TV_AUDIO_LINE_IN_5,
    TV_AUDIO_LINE_IN_6,
    TV_AUDIO_LINE_IN_7,
    TV_AUDIO_LINE_IN_MAX,
} tv_audio_channel_t;

typedef enum tv_audio_in_source_type_e {
    TV_AUDIO_IN_SOURCE_TYPE_LINEIN,
    TV_AUDIO_IN_SOURCE_TYPE_ATV,
    TV_AUDIO_IN_SOURCE_TYPE_HDMI,
    TV_AUDIO_IN_SOURCE_TYPE_MAX,
} tv_audio_in_source_type_t;

#define CC_RESOLUTION_1366X768_W      (1366)
#define CC_RESOLUTION_1366X768_H      (768)
#define CC_RESOLUTION_1920X1080_W     (1920)
#define CC_RESOLUTION_1920X1080_H     (1080)
#define CC_RESOLUTION_3840X2160_W     (3840)
#define CC_RESOLUTION_3840X2160_H     (2160)

typedef enum tv_source_connect_detect_status_e {
    CC_SOURCE_PLUG_OUT = 0,
    CC_SOURCE_PLUG_IN = 1,
} tv_source_connect_detect_status_t;

//HDMI rx cec
typedef struct tagHDMIRxRequestReplyItem {
    CCondition WaitReplyCondition;
    int WaitCmd;
    int WaitLogicAddr;
    int WaitTimeOut;
    int WaitFlag;
    int DataFlag;
    struct _cec_msg msg;
} HDMIRxRequestReplyItem;

typedef struct _hdcp_ksv {
    int bksv0;
    int bksv1;
} _hdcp_ksv;


typedef struct am_phase_s {
    unsigned int   length; // Length of total
    unsigned int phase[TVIN_SIG_FMT_COMP_MAX - TVIN_SIG_FMT_VGA_THRESHOLD];
} am_phase_t;


#define CC_REQUEST_LIST_SIZE            (32)
#define CC_CEC_STREAM_SIZE              (sizeof(struct _cec_msg))
#define CC_SOURCE_DEV_REFRESH_CNT       (E_LA_MAX)

class CTvin {
public:
    CTvin();
    ~CTvin();
    int OpenTvin();
    int init_vdin();
    int uninit_vdin ( void );
    int Tv_init_afe ( void );
    int Tv_uninit_afe ( void );
    int Tvin_AddPath ( tvin_path_id_t pathid );
    int Tvin_RemovePath ( tv_path_type_t pathtype );
    int Tvin_CheckPathActive ( tv_path_type_t path_type, int isCheckD2D3 );
    int setMpeg2Vdin(int enable);
    //pre apis
    int AFE_DeviceIOCtl ( int request, ... );
    void TvinApi_CloseAFEModule ( void );
    int TvinApi_SetVdinHVScale ( int vdinx, int hscale, int vscale );
    int TvinApi_SetCompPhase ( am_phase_t &am_phase );
    int TvinApi_SetStartDropFrameCn ( int count );
    int TvinApi_SetCompPhaseEnable ( int enable );
    tvin_trans_fmt TvinApi_Get3DDectMode();
    int TvinApi_GetHDMIAudioStatus ( void );
    int TvinApi_LoadPLLValues ( am_regs_t regs );
    int TvinApi_LoadCVD2Values ( am_regs_t regs );
    int TvinApi_GetFbSize ( unsigned int *fb_width, unsigned int *fb_height );
    int Tvin_StartDecoder ( tvin_info_t &info );
    int Tvin_StopDecoder();
    int get_hdmi_sampling_rate();
    int get_hdmi_ksv_info(int source_input, int data_buf[]);
    int SwitchPort (tvin_port_t source_port );
    //
    void Tvin_SetDepthOf2Dto3D ( int value );
    int set3D_FL_Frame(int value);
    int setLatchFlag(int value);
    //
    int IsFileExist ( const char *file_name );
    char *DelSub ( char *str, char *sub );
    char *VDIN_CheckVideoPath ( const char *videopath );
    int VDIN_AddPath ( const char *videopath );
    int VDIN_RmDefPath ( void );
    int VDIN_RmTvPath ( void );
    int VDIN_AddVideoPath ( int selPath );
    int VDIN_RmPreviewPath ( void );
    int VDIN_GetVdinFd();

    int VDIN_OpenModule();
    int VDIN_CloseModule();
    int VDIN_DeviceIOCtl ( int request, ... );
    int VDIN_GetDeviceFileHandle();
    int VDIN_OpenPort ( tvin_port_t port );
    int VDIN_ClosePort();
    int VDIN_StartDec ( const struct tvin_parm_s *vdinParam );
    int VDIN_StopDec();
    int VDIN_GetSignalInfo ( struct tvin_info_s *SignalInfo );
    int VDIN_SetVdinParam ( const struct tvin_parm_s *vdinParam );
    int VDIN_GetVdinParam ( const struct tvin_parm_s *vdinParam );
    int VDIN_OnoffVScaler ( int isOn );
    int VDIN_GetDisplayVFreq ( void );
    int VDIN_SetDisplayVFreq ( int freq, int display_resolution , bool isFbc);

    int VDIN_Set2D3DDepth ( int count );

    int VDIN_Set2Dto3D ( int on_off );
    int VDIN_Set3DCmd ( int cmd );

    int VDIN_Get_avg_luma(void);
    int VDIN_GetHistgram ( int *hisgram );
    int VDIN_SetMVCViewMode ( int mode );
    int VDIN_GetMVCViewMode ( void );
    int VDIN_SetDIBuffMgrMode ( int mgr_mode );
    int VDIN_SetDICFG ( int cfg );
    int VDIN_SetDI3DDetc ( int enable );
    int VDIN_Get3DDetc ( void );
    int VDIN_GetVscalerStatus ( void );
    int VDIN_TurnOnBlackBarDetect ( int isEnable );
    int VDIN_LoadHdcpKey ( unsigned char *hdcpkey_buff );
    int VDIN_KeepLastFrame ( int enable );
    int VDIN_SetVideoFreeze ( int enable );
    int VDIN_SetDIBypasshd ( int enable );
    int VDIN_SetDIBypassAll ( int enable );
    int VDIN_SetDIBypass_Get_Buf_Threshold ( int enable );
    int VDIN_SetDIBypassProg ( int enable );
    int VDIN_SetDIBypassDynamic ( int flag );
    int VDIN_SetDIDet3DMode ( int value );
    int VDIN_SetDIBypass3D ( int enable );
    int VDIN_SetDIBypassPost ( int enable );
    int VDIN_SetDIProg_Proc_Config ( int value );
    int VDIN_SetDIInput2Pre ( int value );
    int VDIN_SetVdinFlag ( int flag );
    int VDIN_EnableRDMA ( int enable );
    int VDIN_GetHdmiHdcpKeyKsvInfo(struct _hdcp_ksv *msg);
    int AFE_OpenModule ( void );
    void AFE_CloseModule ( void );
    int AFE_GetDeviceFileHandle();
    int AFE_SetCVBSStd ( tvin_sig_fmt_t cvbs_fmt );
    int AFE_SetVGAEdid ( const unsigned char *ediddata );
    int AFE_GetVGAEdid ( unsigned char *ediddata );
    int AFE_SetADCTimingAdjust ( const struct tvafe_vga_parm_s *timingadj );
    int AFE_GetADCCurrentTimingAdjust ( struct tvafe_vga_parm_s *timingadj );
    int AFE_VGAAutoAdjust ( struct tvafe_vga_parm_s *timingadj );
    int AFE_SetVGAAutoAjust ( void );
    int AFE_GetVGAAutoAdjustCMDStatus ( tvafe_cmd_status_t *Status );
    int AFE_GetAdcCal ( struct tvafe_adc_cal_s *adccalvalue );
    int AFE_SetAdcCal ( struct tvafe_adc_cal_s *adccalvalue );
    int AFE_GetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue );
    int AFE_SetAdcCompCal ( struct tvafe_adc_comp_cal_s *adccalvalue );
    int AFE_GetYPbPrWSSinfo ( struct tvafe_comp_wss_s *wssinfo );
    unsigned int data_limit ( float data );
    void matrix_convert_yuv709_to_rgb ( unsigned int y, unsigned int u, unsigned int v, unsigned int *r, unsigned int *g, unsigned int *b );
    void re_order ( unsigned int *a, unsigned int *b );
    char *get_cap_addr ( enum adc_cal_type_e calType );
    inline unsigned char get_mem_data ( char *dp, unsigned int addr );
    int get_frame_average ( enum adc_cal_type_e calType, struct adc_cal_s *mem_data );
    struct adc_cal_s get_n_frame_average ( enum adc_cal_type_e calType ) ;
    int AFE_GetMemData ( int typeSel, struct adc_cal_s *mem_data );
    int AFE_GetCVBSLockStatus ( enum tvafe_cvbs_video_e *cvbs_lock_status );
    static int CvbsFtmToColorStdEnum(tvin_sig_fmt_t fmt);
    int VDIN_GetPortConnect ( int port );
    int VDIN_OpenHDMIPinMuxOn ( bool flag );
    int TVAFE_EnablePlugInDetect ( bool flag );
    int GetITContent();
    /*******************************************extend funs*********************/
    static tv_source_input_type_t Tvin_SourcePortToSourceInputType ( tvin_port_t source_port );
    static tv_source_input_type_t Tvin_SourceInputToSourceInputType ( tv_source_input_t source_input );
    static tvin_port_t Tvin_GetSourcePortBySourceType ( tv_source_input_type_t source_type );
    static tvin_port_t Tvin_GetSourcePortBySourceInput ( tv_source_input_t source_input );
    static unsigned int Tvin_TransPortStringToValue(const char *port_str);
    static void Tvin_LoadSourceInputToPortMap();
    static int Tvin_GetSourcePortByCECPhysicalAddress(int physical_addr);
    static tv_audio_channel_t Tvin_GetInputSourceAudioChannelIndex ( tv_source_input_t source_input );
    static tv_audio_in_source_type_t Tvin_GetAudioInSourceType ( tv_source_input_t source_input );
    static tv_source_input_t Tvin_PortToSourceInput ( tvin_port_t port );
    static int isVgaFmtInHdmi ( tvin_sig_fmt_t fmt );
    static int isSDFmtInHdmi ( tvin_sig_fmt_t fmt );
    static bool Tvin_is50HzFrameRateFmt ( tvin_sig_fmt_t fmt );
    static bool Tvin_IsDeinterlaceFmt ( tvin_sig_fmt_t fmt );
    static v4l2_std_id CvbsFtmToV4l2ColorStd(tvin_sig_fmt_t fmt);

    static CTvin *getInstance();

public:
    class CTvinSigDetect: public CThread {
    public:
        static const int VDIN_NOSIG_DEFAULT_CHECK_TIMES = 1;
        CTvinSigDetect ();
        ~CTvinSigDetect();
        int startDetect(bool bPause = true);
        int stopDetect();
        int pauseDetect();
        int resumeDetect(int later = 0);
        int initSigState();
        void setVdinNoSigCheckKeepTimes(int times, bool isOnce);//times is time, ms
        int requestAndWaitPauseDetect();
        //first pause detect? ok
        tvin_info_t &getCurSigInfo()
        {
            return m_cur_sig_info;
        }
        class ISigDetectObserver {
        public:
            ISigDetectObserver()
            {};
            virtual ~ISigDetectObserver()
            {};
            virtual void onSigToStable()
            {};
            virtual void onSigStableToUnstable() {};
            virtual void onSigStableToUnSupport() {};
            virtual void onSigStableToNoSig() {};
            virtual void onSigUnStableToUnSupport() {};
            virtual void onSigUnStableToNoSig() {};
            virtual void onSigNullToNoSig() {};
            virtual void onSigNoSigToUnstable() {};

            virtual void onSigStillStable() {};
            virtual void onSigStillUnstable() {};
            virtual void onSigStillNosig() {};
            virtual void onSigStillNoSupport() {};
            virtual void onSigStillNull() {};
            virtual void onStableSigFmtChange() {};
            virtual void onStableTransFmtChange() {};

            virtual void onSigDetectEnter() {};
            virtual void onSigDetectLoop() {};
        };
        void setObserver ( ISigDetectObserver *pOb )
        {
            mpObserver = pOb;
        };
    private:
        bool  threadLoop();
        int Tv_TvinSigDetect ( int &args );

        //member
        tvin_info_t m_cur_sig_info;
        tvin_info_t m_pre_sig_info;
        int mKeepNosigTime;
        bool m_is_nosig_checktimes_once_valid;
        mutable CMutex           mLock;
        CCondition       mDetectPauseCondition;
        CCondition       mRequestPauseCondition;
        volatile int m_sig_detect_status;
        volatile bool m_request_pause_detect;
        enum DetectState {
            STATE_STOPED = 0,
            STATE_RUNNING,
            STATE_PAUSE
        };
        int mDetectState;
        int mResumeLaterTime;
        ISigDetectObserver *mpObserver;
    };//

private:
    static CTvin *mInstance;
    int m_vdin_dev_fd;
    int afe_dev_fd;
    tvin_parm_t m_tvin_param;
    tvin_parm_t gTvinVDINParam;
    tvin_info_t gTvinVDINSignalInfo;
    tvin_parm_t gTvinAFEParam;
    tvin_info_t gTvinAFESignalInfo;
    static int mSourceInputToPortMap[SOURCE_MAX];
    int gExistD2D3;
    char gVideoPath[256];
    int m_is_decoder_start;

    enum tvin_path_id_e m_pathid;
    char config_tv_path[64];
    char config_default_path[64];
};
#endif

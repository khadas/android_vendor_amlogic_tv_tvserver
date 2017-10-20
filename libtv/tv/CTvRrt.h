/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <am_epg.h>
#include "CTvEv.h"
#include "CTvLog.h"

#if !defined(_CDTVRRT_H)
#define _CDTVRRT_H

#define TV_RRT_DEFINE_FILE_PATH  "/param/tv_rrt_define.xml"

typedef struct rrt_info
{
    INT16U rating_region;
    int  dimensions_id;
    int  rating_value_id;
    char rating_region_name[2048];
    char dimensions_name[2048];
    char abbrev_rating_value_text[2048];
    char rating_value_text[2048];
} rrt_info_t;

typedef struct rrt_select_info_s
{
    int  rating_region_name_count;
    char rating_region_name[2048];
    int  dimensions_name_count;
    char dimensions_name[2048];
    int  rating_value_text_count;
    char rating_value_text[2048];
} rrt_select_info_t;

class CTvRrt
{
public:
    static const int MODE_ADD            = 0;
    static const int MODE_REMOVE         = 1;
    static const int MODE_SET            = 2;

    static const int SCAN_PAT            = 0x01;
    static const int SCAN_PMT            = 0x02;
    static const int SCAN_CAT            = 0x04;
    static const int SCAN_SDT            = 0x08;
    static const int SCAN_NIT            = 0x10;
    static const int SCAN_TDT            = 0x20;
    static const int SCAN_EIT_PF_ACT     = 0x40;
    static const int SCAN_EIT_PF_OTH     = 0x80;
    static const int SCAN_EIT_SCHE_ACT   = 0x100;
    static const int SCAN_EIT_SCHE_OTH   = 0x200;
    static const int SCAN_MGT            = 0x400;
    static const int SCAN_VCT            = 0x800;
    static const int SCAN_STT            = 0x1000;
    static const int SCAN_RRT            = 0x2000;
    static const int SCAN_PSIP_EIT       = 0x4000;
    static const int SCAN_PSIP_ETT       = 0x8000;
    static const int SCAN_EIT_PF_ALL     = SCAN_EIT_PF_ACT | SCAN_EIT_PF_OTH;
    static const int SCAN_EIT_SCHE_ALL   = SCAN_EIT_SCHE_ACT | SCAN_EIT_SCHE_OTH;
    static const int SCAN_EIT_ALL        = SCAN_EIT_PF_ALL | SCAN_EIT_SCHE_ALL;
    static const int SCAN_ALL            = SCAN_PAT | SCAN_PMT | SCAN_CAT | SCAN_SDT |SCAN_NIT | SCAN_TDT | SCAN_EIT_ALL |
                                           SCAN_MGT | SCAN_VCT | SCAN_STT | SCAN_RRT | SCAN_PSIP_EIT | SCAN_PSIP_ETT;

    static const int INVALID_ID = -1;

    class RrtEvent : public CTvEv {
        public:
            RrtEvent(): CTvEv(CTvEv::TV_EVENT_RRT)
            {

            };
            ~RrtEvent()
            {
            };

            static const int EVENT_RRT_SCAN_START          = 1;
            static const int EVENT_RRT_SCAN_SCANING        = 2;
            static const int EVENT_RRT_SCAN_END            = 3;

            int satus;
        };

        class IObserver {
        public:
            IObserver() {};
            virtual ~IObserver() {};
            virtual void onEvent(const RrtEvent &ev) = 0;
        };

        int setObserver(IObserver *ob)
        {
            mpObserver = ob;
            return 0;
        }

public:
    static CTvRrt *getInstance();
    CTvRrt();
    ~CTvRrt();
    int StartRrtUpdate(void);
    int StopRrtUpdate(void);
    int GetRRTRating(int rating_region_id, int dimension_id, int value_id, rrt_select_info_t *ret);

private:
    int RrtCreate(int fend_id, int dmx_id, int src, char * textLangs);
    int RrtDestroy();
    int RrtChangeMode(int op, int mode);
    int RrtScanStart(void);
    int RrtScanStop(void);
    static void rrt_evt_callback(long dev_no, int event_type, void *param, void *user_data);
    static void rrt_table_callback(AM_EPG_Handle_t handle, int type, void * tables, void * user_data);
    void rrt_table_update(AM_EPG_Handle_t handle, int type, void * tables, void * user_data);
    void atsc_multiple_string_parser(atsc_multiple_string_t atsc_multiple_string, char *ret);

    static CTvRrt *mInstance;
    IObserver *mpObserver;
    RrtEvent mCurRrtEv;
public:
    rrt_section_info_t *mpNewRrt;
    int mRrtScanStatus;
    int mDmx_id ;
    AM_EPG_Handle_t mRrtScanHandle;
};


#endif //_CDTVRRT_H


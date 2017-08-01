/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _FBC_UTILS_H_
#define _FBC_UTILS_H_

#include "CFbcCommunication.h"
#include <stdio.h>

#define CC_HEAD_CHKSUM_LEN      (9)
#define CC_VERSION_LEN          (5)
#define CC_PROJECT_INFO_ITEM_MAX_LEN  (64)

typedef struct project_info_s {
    char version[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_type[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_outputmode[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_rev[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char panel_name[CC_PROJECT_INFO_ITEM_MAX_LEN];
    char amp_curve_name[CC_PROJECT_INFO_ITEM_MAX_LEN];
} project_info_t;

extern int rebootSystemByEdidInfo();
extern int rebootSystemByUartPanelInfo(CFbcCommunication *fbc = NULL);
extern unsigned int CalCRC32(unsigned int crc, const unsigned char *ptr, unsigned int buf_len);
extern int GetProjectInfo(project_info_t *proj_info_ptr, CFbcCommunication *fbcIns = NULL);

#endif

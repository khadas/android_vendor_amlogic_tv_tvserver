/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: c++ file
 */
#define LOG_TAG                 "tvserver"
#define LOG_TV_TAG              "CTvRrt"

#include <tinyxml.h>
#include "CTvRrt.h"


CTvRrt *CTvRrt::mInstance;
CTvRrt *CTvRrt::getInstance()
{
    LOGD("start rrt action!\n");
    if (NULL == mInstance) {
        mInstance = new CTvRrt();
    }

    return mInstance;
}

CTvRrt::CTvRrt()
{
    mRrtScanHandle      = NULL;
    mpNewRrt            = NULL;
    mRrtScanStatus      = INVALID_ID;
    mDmx_id             = INVALID_ID;

    int ret = RrtCreate(0, 2, 0, NULL);
    if (ret < 0) {
        LOGE("RrtCreate failed!\n");
    }
}

CTvRrt::~CTvRrt()
{
    int ret = RrtDestroy();
    if (ret < 0) {
        LOGE("RrtDestroy failed!\n");
    }

    if (mInstance != NULL) {
        delete mInstance;
        mInstance = NULL;
    }
}

int CTvRrt::StartRrtUpdate(void)
{
    int ret;
    ret = RrtScanStart();
    if (ret < 0) {
        LOGD("RrtScanStart failed!\n");
    }

    return ret;
}

int CTvRrt::StopRrtUpdate(void)
{
    int ret = -1;

    ret = RrtScanStop();

    if (ret < 0) {
        LOGD("RrtScanStop failed!\n");
    }

    return ret;
}

int CTvRrt::GetRRTRating(int rating_region_id, int dimension_id, int value_id, rrt_select_info_t *ret)
{
    LOGD("rating_region_id = %d, dimension_id = %d, value_id = %d\n",rating_region_id, dimension_id, value_id);

    //check rrt_define_file exist
    struct stat tmp_st;
    if (stat(TV_RRT_DEFINE_FILE_PATH, &tmp_st) != 0) {
        LOGD("file don't exist!\n");
        return -1;
    }

    TiXmlDocument *pRRTFile = new TiXmlDocument(TV_RRT_DEFINE_FILE_PATH);
    if (!pRRTFile->LoadFile()) {
        LOGD("load %s error!\n", TV_RRT_DEFINE_FILE_PATH);
        return -1;
    }

    memset(ret, 0, sizeof(rrt_select_info_t));
    TiXmlElement* pTmpElement = pRRTFile->RootElement()->FirstChildElement();
    if (pTmpElement != NULL) {
        do {
            if ((pTmpElement->FirstAttribute()->Next()->IntValue() ==rating_region_id) &&
                (pTmpElement->LastAttribute()->IntValue() == dimension_id )) {
                LOGD("selectChildElect: %s\n",pTmpElement->Value());
                LOGD("%s\n",pTmpElement->FirstAttribute()->Next()->Next()->Value());
                int RationSize = strlen(pTmpElement->FirstAttribute()->Next()->Next()->Value());
                LOGD("RationSize: %d\n",RationSize);
                ret->rating_region_name_count = RationSize;
                const char *rating_region_name = pTmpElement->FirstAttribute()->Next()->Next()->Value();
                memcpy(ret->rating_region_name, rating_region_name, RationSize+1);
                LOGD("%s\n",pTmpElement->FirstAttribute()->Value());
                int DimensionSize = strlen(pTmpElement->FirstAttribute()->Value());
                LOGD("DimensionSize: %d\n",DimensionSize);
                ret->dimensions_name_count = DimensionSize;
                memcpy(ret->dimensions_name, pTmpElement->FirstAttribute()->Value(), DimensionSize+1);

                TiXmlElement* pElement = NULL;
                for (pElement=pTmpElement->FirstChildElement();pElement;pElement = pElement->NextSiblingElement()) {
                    if (pElement->LastAttribute()->IntValue() == value_id ) {
                        int ValueSize = strlen(pElement->FirstAttribute()->Value());
                        LOGD("ValueSize: %d\n",ValueSize);
                        ret->rating_value_text_count = ValueSize;
                        LOGD("%s\n",pElement->FirstAttribute()->Value());
                        memcpy(ret->rating_value_text, pElement->FirstAttribute()->Value(), ValueSize+1);
                        return 0;
                    }
                }
            }
        } while(pTmpElement = pTmpElement->NextSiblingElement());
                LOGD("Don't find value !\n");
                return -1;
    } else {
        LOGD("XML file is NULL!\n");
        return -1;
    }
}

int CTvRrt::RrtCreate(int fend_id, int dmx_id, int src, char * textLangs)
{
    AM_EPG_CreatePara_t para;
    AM_ErrorCode_t  ret;
    AM_DMX_OpenPara_t dmx_para;
    mDmx_id = dmx_id;

    memset(&dmx_para, 0, sizeof(dmx_para));
    LOGD("Opening demux%d ...", dmx_id);
    ret = AM_DMX_Open(mDmx_id, &dmx_para);
    if (ret != AM_SUCCESS) {
        LOGD("AM_DMX_Open failed");
        return - 1;
    }

    para.fend_dev       = fend_id;
    para.dmx_dev        = dmx_id;
    para.source         = src;
    para.hdb            = NULL;
    snprintf(para.text_langs, sizeof(para.text_langs), "%s", textLangs);

    ret = AM_EPG_Create(&para, &mRrtScanHandle);

    if (ret != AM_SUCCESS) {
        LOGD("AM_EPG_Create failed");
        return - 1;
    }

    /*disable internal default table procedure*/
    AM_EPG_DisableDefProc(mRrtScanHandle, AM_TRUE);
    if (ret != AM_SUCCESS) {
        LOGD("AM_EPG_DisableDefProc failed");
        return - 1;
    }

    /*register eit events notifications*/
    AM_EVT_Subscribe((long)mRrtScanHandle, AM_EPG_EVT_NEW_RRT, rrt_evt_callback, NULL);
    AM_EPG_SetUserData(mRrtScanHandle, (void *)this);

    /*handle tables directly by user*/
    AM_EPG_SetTablesCallback(mRrtScanHandle, AM_EPG_TAB_RRT, rrt_table_callback, NULL);
    if (ret != AM_SUCCESS) {
        LOGD("AM_EPG_SetTablesCallback failed");
        return - 1;
    }

    return 0;
}

int CTvRrt::RrtDestroy()
{
    AM_ErrorCode_t  ret;

    if (mRrtScanHandle != NULL) {
        AM_EVT_Unsubscribe((long)mRrtScanHandle, AM_EPG_EVT_NEW_RRT, rrt_evt_callback, NULL);
        ret = AM_EPG_Destroy(mRrtScanHandle);
        if (ret != AM_SUCCESS) {
            LOGD("AM_EPG_Destroy failed");
            return - 1;
        }
    }

    if (mDmx_id != INVALID_ID) {
        ret = AM_DMX_Close(mDmx_id);

        if (ret != AM_SUCCESS) {
            LOGD("AM_DMX_Close failed");
            return - 1;
        }
    }

    return 0;
}

int CTvRrt::RrtChangeMode(int op, int mode)
{
    AM_ErrorCode_t  ret;

    ret = AM_EPG_ChangeMode(mRrtScanHandle, op, mode);
    if (ret != AM_SUCCESS) {
        LOGD("AM_EPG_ChangeMode failed");
        return - 1;
    }

    return 0;
}

int CTvRrt::RrtScanStart(void)
{
    int ret = -1;
    mRrtScanStatus = RrtEvent::EVENT_RRT_SCAN_START;
    ret = RrtChangeMode(MODE_ADD, SCAN_RRT);

    return ret;
}

int CTvRrt::RrtScanStop(void)
{
    int ret = -1;
    mRrtScanStatus = RrtEvent::EVENT_RRT_SCAN_END;
    ret = RrtChangeMode(MODE_REMOVE, SCAN_RRT);
    return ret;
}

void CTvRrt::rrt_evt_callback(long dev_no, int event_type, void *param, void *user_data __unused)
{
    CTvRrt *pRrt;

    AM_EPG_GetUserData((AM_EPG_Handle_t)dev_no, (void **)&pRrt);

    if ((pRrt == NULL) || (pRrt->mpObserver == NULL)) {
        LOGD("rrt_evt_callback: Rrt event is NULL!\n");
        return;
    }

    switch (event_type) {
    case AM_EPG_EVT_NEW_RRT: {
        pRrt->mCurRrtEv.satus = mInstance->mRrtScanStatus;
        pRrt->mpObserver->onEvent(pRrt->mCurRrtEv);
        break;
    }
    default:
        break;
    }
}

void CTvRrt::rrt_table_callback(AM_EPG_Handle_t handle, int type, void * tables, void * user_data)
{
    if (!tables) {
        LOGD("rrt_table_callback: tables is NULL!\n");
        return;
    }

    switch (type) {
    case AM_EPG_TAB_RRT: {
        mInstance->rrt_table_update(handle, type, tables, user_data);
        break;
    }
    default:
        break;
    }
}

TiXmlElement *GetElementPointerByName(TiXmlElement* pRootElement, char *ElementName)
{
    if (strcmp(ElementName, pRootElement->Value()) == 0) {
        return pRootElement;
    }

    TiXmlElement* pElement = NULL;
    TiXmlElement* pRetElement = NULL;
    for (pElement=pRootElement->FirstChildElement();pElement;pElement = pElement->NextSiblingElement()) {
         pRetElement = GetElementPointerByName(pElement, ElementName);
    }

    if (pRetElement != NULL) {
        LOGD("GetNodePointerByName: %s", pRetElement->Value());
        return pRetElement;
    } else {
        return NULL;
    }
}

TiXmlDocument *OpenXmlFile(void)
{
    // define TiXmlDocument
    TiXmlDocument *pRRTFile = new TiXmlDocument();
    pRRTFile->LoadFile();
    if (NULL == pRRTFile) {
        LOGD("create RRTFile error!\n");
        return NULL;
    }

    //add Declaration
    LOGD("start create Declaration!\n");
    TiXmlDeclaration *pNewDeclaration = new TiXmlDeclaration("1.0","utf-8","");
    if (NULL == pNewDeclaration) {
        LOGD("create Declaration error!\n");
        return NULL;
    }
    pRRTFile->LinkEndChild(pNewDeclaration);

    //add root element
    LOGD("start create RootElement!\n");
    TiXmlElement *pRootElement = new TiXmlElement("rating-system-definitions");
    if (NULL == pRootElement) {
        LOGD("create RootElement error!\n");
        return NULL;
    }
    pRRTFile->LinkEndChild(pRootElement);
    pRootElement->SetAttribute("xmlns:android", "http://schemas.android.com/apk/res/android");
    pRootElement->SetAttribute("android:versionCode", "2");

    return pRRTFile;
}

bool SaveDataToXml(TiXmlDocument *pRRTFile, rrt_info_t rrt_info)
{
    if (pRRTFile == NULL) {
        LOGE("xml file don't open!\n");
        return false;
    }

    TiXmlElement *pRootElement = pRRTFile->RootElement();
    if (pRootElement->FirstChildElement() == NULL) {
        TiXmlElement *pRatingSystemElement = new TiXmlElement("rating-system-definition");
        if (NULL == pRatingSystemElement) {
            LOGD("create pRatingSystemElement error!\n");
            return false;
        }
        pRootElement->LinkEndChild(pRatingSystemElement);
        pRatingSystemElement->SetAttribute("android:name", rrt_info.dimensions_name);
        pRatingSystemElement->SetAttribute("android:rating", rrt_info.rating_region);
        pRatingSystemElement->SetAttribute("android:country",rrt_info.rating_region_name);
        pRatingSystemElement->SetAttribute("android:dimension_id",rrt_info.dimensions_id);

        TiXmlElement *pNewElement = new TiXmlElement("rating-definition");
        if (NULL == pNewElement) {
            return false;
        }
        pRatingSystemElement->LinkEndChild(pNewElement);
        pNewElement->SetAttribute("android:title",rrt_info.abbrev_rating_value_text);
        pNewElement->SetAttribute("android:description",rrt_info.rating_value_text);
        pNewElement->SetAttribute("android:rating_id",rrt_info.rating_value_id);

    } else {
        TiXmlElement *pTmpElement = GetElementPointerByName(pRootElement, "rating-system-definition");
        if ((strcmp(pTmpElement->FirstAttribute()->Value(), rrt_info.dimensions_name) == 0) &&
            (strcmp(pTmpElement->FirstAttribute()->Next()->Next()->Value(), rrt_info.rating_region_name) == 0) &&
            (pTmpElement->LastAttribute()->IntValue() == rrt_info.dimensions_id)) {
            LOGD("add new rating-definition to rating-system-definition!\n");
            TiXmlElement *pNewElement = new TiXmlElement("rating-definition");
            if (NULL == pNewElement) {
                return false;
            }
            pTmpElement->LinkEndChild(pNewElement);
            pNewElement->SetAttribute("android:title",rrt_info.abbrev_rating_value_text);
            pNewElement->SetAttribute("android:description",rrt_info.rating_value_text);
            pNewElement->SetAttribute("android:rating_id",rrt_info.rating_value_id);
        } else {
            LOGD("create new rating-system-definition!\n");
            TiXmlElement *pRatingSystemElement = new TiXmlElement("rating-system-definition");
            if (NULL == pRatingSystemElement) {
                LOGD("create pRatingSystemElement error!\n");
                return false;
            }
            pRootElement->LinkEndChild(pRatingSystemElement);
            pRatingSystemElement->SetAttribute("android:name", rrt_info.dimensions_name);
            pRatingSystemElement->SetAttribute("android:rating", rrt_info.rating_region);
            pRatingSystemElement->SetAttribute("android:country",rrt_info.rating_region_name);
            pRatingSystemElement->SetAttribute("android:dimension_id",rrt_info.dimensions_id);

            TiXmlElement *pNewElement = new TiXmlElement("rating-definition");
            if (NULL == pNewElement) {
                return false;
            }
            pRatingSystemElement->LinkEndChild(pNewElement);
            pNewElement->SetAttribute("android:title",rrt_info.abbrev_rating_value_text);
            pNewElement->SetAttribute("android:description",rrt_info.rating_value_text);
            pNewElement->SetAttribute("android:rating_id",rrt_info.rating_value_id);
        }
    }

    if (!pRRTFile->SaveFile(TV_RRT_DEFINE_FILE_PATH)) {
        LOGD("save RRTFile error!\n");
        return false;
    }

    char sysCmd[1024];
    sprintf(sysCmd, "chmod 755 %s", TV_RRT_DEFINE_FILE_PATH);
    if (system(sysCmd)) {
       LOGE("exec cmd:%s fail\n", sysCmd);
    }

    return true;
}

void CTvRrt::rrt_table_update(AM_EPG_Handle_t handle, int type, void * tables, void * user_data)
{
    switch (type) {
    case AM_EPG_TAB_RRT: {
        mRrtScanStatus = RrtEvent::EVENT_RRT_SCAN_SCANING;
        mpNewRrt = (rrt_section_info_t *)tables;
        INT8U i, j;
        rrt_info_t rrt_info;
        memset(&rrt_info, 0, sizeof(rrt_info_t));
        #if 0
        //check rrt_define_file exist
        struct stat tmp_st;
        if (stat(TV_RRT_DEFINE_FILE_PATH, &tmp_st) == 0) {
            LOGD("file exist,delete it!\n");
            remove(TV_RRT_DEFINE_FILE_PATH);
        }
        #endif
        //open xml file
        TiXmlDocument *pRRTFile = OpenXmlFile();
        if (pRRTFile == NULL) {
            LOGD("open xml file failed!\n");
            return;
        }

        while (mpNewRrt != NULL) {
            LOGD("T [RRT:0x%02x][rr:0x%04x][dd:0x%04x] v[0x%x]\n", mpNewRrt->i_table_id, mpNewRrt->rating_region,
                    mpNewRrt->dimensions_defined, mpNewRrt->version_number);

            //save rating_region
            rrt_info.rating_region = mpNewRrt->rating_region;

            //parser rating_region_name
            atsc_multiple_string_parser(mpNewRrt->rating_region_name, rrt_info.rating_region_name);

            //parser dimensions_info
            rrt_dimensions_info  *dimensions_info = mpNewRrt->dimensions_info;

            while (dimensions_info != NULL) {
                //parser dimensions_name
                atsc_multiple_string_parser(dimensions_info->dimensions_name, rrt_info.dimensions_name);
                LOGD("graduated_scale[%d] values_defined[%d]\n", mpNewRrt->dimensions_info->graduated_scale,mpNewRrt->dimensions_info->values_defined);

                //paser and save data to xml
                for (j=1;j<dimensions_info->values_defined;j++) {
                    //save rating_id
                    rrt_info.rating_value_id = j;
                    atsc_multiple_string_parser(dimensions_info->rating_value[j].abbrev_rating_value_text, rrt_info.abbrev_rating_value_text);
                    atsc_multiple_string_parser(dimensions_info->rating_value[j].rating_value_text, rrt_info.rating_value_text);

                    bool ret = SaveDataToXml(pRRTFile, rrt_info);
                    if (ret) {
                        LOGD("save XML data to file success!\n");
                    } else {
                        LOGD("save XML data to file failed!\n");
                    }
                }
                //save dimensions_id
                rrt_info.dimensions_id ++ ;

                dimensions_info = dimensions_info->p_next;
            }
            mpNewRrt = mpNewRrt->p_next;
        }
        mRrtScanStatus = RrtEvent::EVENT_RRT_SCAN_END;
        break;
    }
    default:
        break;
    }
}

void CTvRrt::atsc_multiple_string_parser(atsc_multiple_string_t atsc_multiple_string, char *ret)
{
    int i;
    for (i=0;i<atsc_multiple_string.i_string_count;i++) {
        int size = strlen((char *)atsc_multiple_string.string[0].string);
        if (ret != NULL) {
            memcpy(ret, atsc_multiple_string.string[0].string, size+1);
        }
    }

    return;
}

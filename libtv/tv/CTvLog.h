//
//
//  amlogic 2013
//
//  @ Project : tv
//  @ File Name : CTvDatabase.h
//  @ Date : 2013-11
//  @ Author :
//
//
#include <utils/Log.h>
#if !defined(_CTVLOG_H)
#define _CTVLOG_H

#define LOGD(...)   __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define LOGE(...)   __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define LOGW(...)   __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)


//class CTvLog{
//  public:
//  inline static void LOGD();
//  inline static void LOGW();
//  inline static void LOGE();
//};
#endif //

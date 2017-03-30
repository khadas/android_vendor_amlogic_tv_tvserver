LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIB_TV_BINDER_PATH := $(LOCAL_PATH)/../../frameworks/libtvbinder

LOCAL_SRC_FILES:= \
    android_tvtest.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libtvbinder \
	libnativehelper \
	libandroid_runtime \
	liblog

LOCAL_C_INCLUDES += \
    bionic/libc/include \
    $(LIB_TV_BINDER_PATH)/include

LOCAL_MODULE:= tvtest

include $(BUILD_EXECUTABLE)


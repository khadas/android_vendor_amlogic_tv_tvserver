# Copyright (c) 2014 Amlogic, Inc. All rights reserved.
#
# This source code is subject to the terms and conditions defined in the
# file 'LICENSE' which is part of this source code package.
#
# Description: makefile

ifeq ($(USE_ANOTHER_FBC_LINKER),)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIB_TV_UTILS := $(LOCAL_PATH)/../tvutils

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  CFbcLinker.cpp \
  CFbcProtocol.cpp \
  CFbcUpgrade.cpp \
  CSerialPort.cpp \
  CHdmiCecCmd.cpp \
  CVirtualInput.cpp

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/include \
  $(LIB_TV_UTILS)/include

LOCAL_STATIC_LIBRARIES := \
  libtv_utils

LOCAL_SHARED_LIBRARIES := \
  libcutils \
  liblog \
  libutils

LOCAL_MODULE:= libtv_linker

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
endif

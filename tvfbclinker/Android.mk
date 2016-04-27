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
  libutils

LOCAL_MODULE:= libtv_linker

include $(BUILD_SHARED_LIBRARY)
endif

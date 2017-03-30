LOCAL_PATH:= $(call my-dir)


DVB_PATH := $(wildcard external/dvb)
LIB_TV_UTILS := $(LOCAL_PATH)/../tvutils
LIB_TV_BINDER := $(LOCAL_PATH)/../../frameworks/libtvbinder

ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard vendor/amlogic/external/dvb)
endif

ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard vendor/amlogic/dvb)
endif

LIB_ZVBI_PATH := $(wildcard external/libzvbi)
LIB_SQLITE_PATH := $(wildcard external/sqlite)


#tvserver
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
  main.cpp \
  MemoryLeakTrackUtil.cpp \
  TvService.cpp

LOCAL_SHARED_LIBRARIES += \
  libutils \
  libbinder \
  libcutils \
  libtvbinder \
  libtv

LOCAL_SHARED_LIBRARIES += \
  libam_mw \
  libam_adp \
  libam_ver

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/../libtv \
  $(LOCAL_PATH)/../libtv/tvdb \
  $(LOCAL_PATH)/../libtv/tv \
  $(LOCAL_PATH)/../libtv/include \
  $(LOCAL_PATH)/../libtv/gpio \
  $(LOCAL_PATH)/../tvfbclinker/include \
  $(LIB_TV_UTILS)/include \
  $(LIB_SQLITE_PATH)/dist

LOCAL_C_INCLUDES += \
  system/extras/ext4_utils \
  $(LIB_TV_BINDER)/include \
  system/media/audio_effects/include \
  hardware/amlogic/audio/libTVaudio

LOCAL_C_INCLUDES += \
  $(DVB_PATH)/include/am_adp \
  $(DVB_PATH)/include/am_mw \
  $(DVB_PATH)/include/am_ver \
  $(DVB_PATH)/android/ndk/include \
  hardware/amlogic/media/amadec/include \
  hardware/amlogic/media/amcodec/include

LOCAL_CFLAGS += -DTARGET_BOARD_$(strip $(TVAPI_TARGET_BOARD_VERSION))

LOCAL_MODULE:= tvserver

include $(BUILD_EXECUTABLE)

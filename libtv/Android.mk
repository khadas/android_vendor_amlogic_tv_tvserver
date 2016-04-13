LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#first delete prebuilt file
$(shell rm -r vendor/amlogic/prebuilt/tv)

DVB_PATH := $(wildcard external/dvb)

ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard vendor/amlogic/external/dvb)
endif

ifeq ($(DVB_PATH), )
  DVB_PATH := $(wildcard vendor/amlogic/dvb)
endif

AM_LIBPLAYER_PATH := $(wildcard vendor/amlogic/frameworks/av/LibPlayer)
LIB_ZVBI_PATH := $(wildcard external/libzvbi)
LIB_SQLITE_PATH := $(wildcard external/sqlite)

#support android and amaudio
BOARD_TV_AUDIO_TYPE := amaudio

#support builtin and external
BOARD_TV_AUDIO_AMAUDIO_LIB_TYPE := external


LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
  tv/CAutoPQparam.cpp \
  tv/AutoBackLight.cpp \
  tv/CTvLog.cpp \
  tv/CTvEv.cpp \
  tv/CTvEpg.cpp \
  tv/CTvRecord.cpp \
  tv/CTvSubtitle.cpp \
  tv/CTvScanner.cpp \
  tv/CTvTime.cpp \
  tv/CTv.cpp \
  tv/CTvBooking.cpp \
  tv/CFrontEnd.cpp \
  tv/CTvVchipCheck.cpp \
  tv/CFbcCommunication.cpp \
  tv/CTvScreenCapture.cpp \
  tv/CUpgradeFBC.cpp \
  tv/CAv.cpp \
  tv/CTvDmx.cpp \
  tv/CTvFactory.cpp \
  tvin/CTvin.cpp \
  tvin/CSourceConnectDetect.cpp \
  tvin/CHDMIRxManager.cpp \
  tvdb/CTvDimension.cpp \
  vpp/CVpp.cpp \
  vpp/pqdata.cpp \
  vpp/CPQdb.cpp \
  tvutils/tvutils.cpp \
  tvutils/CFile.cpp \
  tvutils/CThread.cpp \
  tvutils/CMsgQueue.cpp \
  tvutils/zepoll.cpp \
  tvutils/serial_base.cpp \
  tvutils/serial_operate.cpp \
  tvutils/CSerialCommunication.cpp \
  tvutils/CSerialPort.cpp \
  tvutils/CHdmiCecCmd.cpp \
  tvutils/CVirtualInput.cpp \
  tvutils/CSqlite.cpp \
  tvconfig/tvconfig.cpp \
  tvconfig/CIniFile.cpp \
  audio/CTvAudio.cpp \
  audio/audio_effect.cpp \
  audio/audio_alsa.cpp \
  audio/CAudioCustomerCtrl.cpp \
  tvsetting/audio_cfg.cpp \
  tvsetting/CBlobDevice.cpp \
  tvsetting/CBlobDeviceE2prom.cpp \
  tvsetting/CBlobDeviceFile.cpp \
  tvsetting/CTvSetting.cpp  \
  tvsetting/CTvSettingDeviceFactory.cpp  \
  tvsetting/TvKeyData.cpp \
  version/version.cpp \
  tvdb/CTvChannel.cpp \
  tvdb/CTvDatabase.cpp \
  tvdb/CTvEvent.cpp \
  tvdb/CTvGroup.cpp \
  tvdb/CTvProgram.cpp \
  tvdb/CTvRegion.cpp

LOCAL_SHARED_LIBRARIES := \
  libui \
  libutils \
  libcutils \
  libnetutils \
  libsqlite \
  libmedia \
  libtvbinder \
  libhardware_legacy \
  libdl \
  libskia \
  libtinyxml \
  libusbhost \
  libbinder \
  libstagefright

LOCAL_SHARED_LIBRARIES += \
  libzvbi \
  libntsc_decode \
  libam_mw \
  libam_adp \
  libam_ver \
  libsystemcontrolservice

ifeq ($(strip $(BOARD_TV_AUDIO_AMAUDIO_LIB_TYPE)), external)
  LOCAL_SHARED_LIBRARIES += libTVaudio
endif

ifeq ($(strip $(BOARD_ALSA_AUDIO)),tiny)
  LOCAL_SHARED_LIBRARIES += libtinyalsa
else
  LOCAL_SHARED_LIBRARIES += libasound
endif

LOCAL_STATIC_LIBRARIES += libz

LOCAL_CFLAGS := \
  -fPIC -fsigned-char -D_POSIX_SOURCE \
  -DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
  -DALSA_PLUGIN_DIR=\"/system/usr/lib/alsa-lib\" \
  -DALSA_DEVICE_DIRECTORY=\"/dev/snd/\"

LOCAL_CFLAGS += -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq ($(SOURCE_DEDTECT_ON),true)
  LOCAL_CFLAGS += -DSOURCE_DETECT_ENABLE
endif

ifeq ($(TARGET_SIMULATOR),true)
  LOCAL_CFLAGS += -DSINGLE_PROCESS
endif

ifeq ($(strip $(BOARD_ALSA_AUDIO)),tiny)
  LOCAL_CFLAGS += -DBOARD_ALSA_AUDIO_TINY
endif

ifeq ($(strip $(BOARD_TV_AUDIO_TYPE)),amaudio)
  LOCAL_CFLAGS += -DCC_TV_AUDIO_TYPE_AMAUDIO=1
endif

ifeq ($(strip $(BOARD_TV_AUDIO_TYPE)),android)
  LOCAL_SRC_FILES += audio/audio_android.cpp
  LOCAL_CFLAGS += -DCC_TV_AUDIO_TYPE_ANDROID=1
endif

ifneq ($(TARGET_BUILD_VARIANT), user)
	LOCAL_CFLAGS += -DTV_DEBUG_PQ_ENABLE
endif

LOCAL_C_INCLUDES += \
  bionic/libc/include \
  bionic/libc/private \
  system/extras/ext4_utils \
  vendor/amlogic/frameworks/libtvbinder/include \
  $(LIB_SQLITE_PATH)/dist \
  system/media/audio_effects/include \
  external/tinyalsa/include \
  frameworks/av/include/media/stagefright \
  frameworks/native/include/media/openmax

ifeq ($(strip $(BOARD_TV_AUDIO_AMAUDIO_LIB_TYPE)), external)
  LOCAL_C_INCLUDES += hardware/amlogic/audio/libTVaudio
endif

LOCAL_C_INCLUDES += external/libzvbi/src \
  bionic/libc/include \
  $(DVB_PATH)/android/ndk/include \
  external/tinyxml

ifeq ($(strip $(BOARD_ALSA_AUDIO)),tiny)
  LOCAL_C_INCLUDES += external/tinyalsa/include
else
  LOCAL_C_INCLUDES += external/alsa-lib/include
endif


LOCAL_C_INCLUDES += \
  $(DVB_PATH)/include/am_adp \
  $(DVB_PATH)/include/am_mw \
  $(DVB_PATH)/include/am_ver \
  $(DVB_PATH)/android/ndk/include \
  $(LOCAL_PATH)/dtv_play \
  $(LIB_ZVBI_PATH)/ntsc_decode/include \
  $(LIB_ZVBI_PATH)/ntsc_decode/include/ntsc_dmx \
  $(LIB_ZVBI_PATH)/src \
  $(AM_LIBPLAYER_PATH)/amadec/include \
  $(AM_LIBPLAYER_PATH)/amcodec/include \
  $(AM_LIBPLAYER_PATH)/amffmpeg \
  $(AM_LIBPLAYER_PATH)/amplayer \
  $(LOCAL_PATH)/tvdb \
  $(LOCAL_PATH)/tv \
  $(LOCAL_PATH)/include \
  $(LOCAL_PATH)/../build/include \
  $(TOP)/vendor/amlogic/frameworks/services \



LOCAL_LDLIBS  += -L$(SYSROOT)/usr/lib -llog

LOCAL_PRELINK_MODULE := false

# version
ifeq ($(strip $(BOARD_TVAPI_NO_VERSION)),)
  $(shell cd $(LOCAL_PATH);touch version/version.cpp)
  LIBTVSERVICE_GIT_VERSION="$(shell cd $(LOCAL_PATH);git log | grep commit -m 1 | cut -d' ' -f 2)"
  LIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM=$(shell cd $(LOCAL_PATH);git diff | grep +++ -c)
  LIBTVSERVICE_GIT_BRANCH="$(shell cd $(LOCAL_PATH);git branch | grep \* -m 1)"
  LIBTVSERVICE_LAST_CHANGED="$(shell cd $(LOCAL_PATH);git log | grep Date -m 1)"
  LIBTVSERVICE_BUILD_TIME=" $(shell date)"
  LIBTVSERVICE_BUILD_NAME=" $(shell echo ${LOGNAME})"

  LOCAL_CFLAGS+=-DHAVE_VERSION_INFO
  LOCAL_CFLAGS+=-DLIBTVSERVICE_GIT_VERSION=\"${LIBTVSERVICE_GIT_VERSION}${LIBTVSERVICE_GIT_DIRTY}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM=${LIBTVSERVICE_GIT_UNCOMMIT_FILE_NUM}
  LOCAL_CFLAGS+=-DLIBTVSERVICE_GIT_BRANCH=\"${LIBTVSERVICE_GIT_BRANCH}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_LAST_CHANGED=\"${LIBTVSERVICE_LAST_CHANGED}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_BUILD_TIME=\"${LIBTVSERVICE_BUILD_TIME}\"
  LOCAL_CFLAGS+=-DLIBTVSERVICE_BUILD_NAME=\"${LIBTVSERVICE_BUILD_NAME}\"
  LOCAL_CFLAGS+=-DTVAPI_BOARD_VERSION=\"$(TVAPI_TARGET_BOARD_VERSION)\"
endif

LOCAL_CFLAGS += -DTARGET_BOARD_$(strip $(TVAPI_TARGET_BOARD_VERSION))
LOCAL_MODULE:= libtv

include $(BUILD_SHARED_LIBRARY)

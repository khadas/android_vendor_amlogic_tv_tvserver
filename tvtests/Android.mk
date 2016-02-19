LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

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
    vendor/amlogic/frameworks/libtvbinder/include

LOCAL_MODULE:= tvtest

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    comm_test.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libtvbinder \
	libnativehelper \
	libandroid_runtime \
	liblog \
	libtv

LOCAL_C_INCLUDES += \
    bionic/libc/include \
	$(LOCAL_PATH)/../ \
	$(LOCAL_PATH)/../libtv\
	$(LOCAL_PATH)/../android

LOCAL_MODULE:= comm_test

include $(BUILD_EXECUTABLE)

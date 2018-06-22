LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libarpcap

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include \

LOCAL_SRC_FILES := \
	src/ScreenCapture.cpp

LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libui \
	libgui \

include $(BUILD_SHARED_LIBRARY)

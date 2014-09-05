LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	ddr_hotplug.c

common_CFLAGS := $(common_CCFLAGS) \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wbad-function-cast \
	-std=c99

LOCAL_CFLAGS := $(common_CFLAGS)
LOCAL_C_INCLUDES := \
	$(JNI_H_INCLUDE) \
	$(LOCAL_PATH)/../../ \
	$(LOCAL_PATH)/../../../plugins

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libcutils \
	liblog


LOCAL_PRELINK_MODULE := false
LOCAL_MODULE    := libddr_hotplug

include $(BUILD_SHARED_LIBRARY)

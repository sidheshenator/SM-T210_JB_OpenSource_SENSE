LOCAL_PATH:= $(call my-dir)
ANDOIRD_EVENT_RELAY_PATH:= $(LOCAL_PATH)/AndroidEvent

common_CFLAGS := $(common_CCFLAGS) \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wbad-function-cast \
	-std=c99

common_C_INCLUDES := \
	$(LOCAL_PATH)/../

# the socket server lib
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libsocketsvr

LOCAL_C_INCLUDES := \
	$(common_C_INCLUDES) \

LOCAL_SRC_FILES:= \
        ppd_socket_server.c

LOCAL_CFLAGS := $(common_CFLAGS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_LDFLAGS := -Wl,--allow-shlib-undefined

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

include $(ANDOIRD_EVENT_RELAY_PATH)/Android.mk

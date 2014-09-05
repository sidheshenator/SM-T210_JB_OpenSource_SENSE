LOCAL_PATH:= $(call my-dir)
REL_PATH := $(LOCAL_PATH)/release
res := $(shell ls $(REL_PATH) 2>/dev/null)


ifeq ($(res),)
#############################################
#>--Build the source file
#############################################

#LOCAL_PATH:= $(call my-dir)
PLUGIN_PATH:= $(LOCAL_PATH)/src/plugins
DELEGATE_PATH:= $(LOCAL_PATH)/src/delegates
LIB_PATH:= $(LOCAL_PATH)/src/lib
UTILS_PATH:= $(LOCAL_PATH)/utils

common_CFLAGS := -O2 -W -Wall -Wshadow -W -Wpointer-arith \
	-Wcast-qual -Wcast-align -Wwrite-strings \
	-Wconversion -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wredundant-decls \
	-Wnested-externs -Winline -Wundef -Wbad-function-cast \
	-Waggregate-return \
	-std=c99

common_CFLAGS += -D_POSIX_SOURCE -D_GNU_SOURCE

common_C_INCLUDES := \
	$(LOCAL_PATH)/src

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
	external/libxml2/include \
	external/icu4c/common \
	$(common_C_INCLUDES)

LOCAL_SRC_FILES:= \
	src/config_parser.c \
	src/ppd_log.c \
	src/daemon_utils.c \
	src/list.c \
	src/main.c \
	src/ppd_plugin_utils.c \
	src/ppd_delegate_utils.c \
	src/sock_utils.c

LOCAL_SHARED_LIBRARIES := libdl libcutils libxml2 libsocketsvr
LOCAL_CFLAGS := $(common_CFLAGS)
LOCAL_LDFLAGS := -Wl,--export-dynamic

LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := powerdaemon
include $(BUILD_EXECUTABLE)

include $(DELEGATE_PATH)/Android.mk
include $(PLUGIN_PATH)/Android.mk
include $(LIB_PATH)/Android.mk
include $(UTILS_PATH)/Android.mk

else
#############################################
#>--Build the release binary
#############################################
include $(REL_PATH)/Android.mk
endif

LOCAL_PATH:= $(call my-dir)

common_CFLAGS := -O2 -W -Wall -Wshadow -W -Wpointer-arith \
	-Wcast-qual -Wcast-align -Wwrite-strings \
	-Wconversion -Wstrict-prototypes -Wmissing-prototypes \
	-Wmissing-declarations -Wredundant-decls \
	-Wnested-externs -Winline -Wundef -Wbad-function-cast \
	-Waggregate-return \
	-std=c99

common_CFLAGS += -D_POSIX_SOURCE -D_GNU_SOURCE

common_C_INCLUDES := \
	$(LOCAL_PATH)/../src

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := \
	$(common_C_INCLUDES)

LOCAL_SRC_FILES:= \
	ppd_cmd.c

LOCAL_SHARED_LIBRARIES := liblog
LOCAL_CFLAGS := $(common_CFLAGS)
LOCAL_LDFLAGS := -Wl,--export-dynamic

LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := ppd_cmd
include $(BUILD_EXECUTABLE)

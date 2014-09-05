#
# config.
# -> mrvl_dut is copied to /system/bin
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	mrvl_dut.c

LOCAL_C_INCLUDES:=\
	external/bluetooth/bluez/lib \
	external/bluetooth/bluez/include

LOCAL_SHARED_LIBRARIES := \
	libbluetooth \
	libcutils

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=mrvl_dut

include $(BUILD_EXECUTABLE)
LOCAL_PATH:= $(call my-dir)
PLUGIN_THERMAL_JAVA_PATH:= $(LOCAL_PATH)/ThermalService
PLUGIN_DDR_HOTPLUG_JAVA_PATH:= $(LOCAL_PATH)/DDRHotplugService

common_CFLAGS := $(common_CCFLAGS) \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wbad-function-cast \
	-std=c99

common_C_INCLUDES := \
	$(LOCAL_PATH)/../

# the plugin of android event
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := plugin_android

LOCAL_C_INCLUDES := \
	$(common_C_INCLUDES) \
	external/libxml2/include \
	external/icu4c/common \

LOCAL_SRC_FILES:= \
        $(LOCAL_MODULE).c

LOCAL_CFLAGS := $(common_CFLAGS)

LOCAL_SHARED_LIBRARIES := libxml2
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_LDFLAGS := -Wl,--allow-shlib-undefined

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/PowerDaemon
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/PowerDaemon
include $(BUILD_SHARED_LIBRARY)

# the plugin of coda7542
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := plugin_coda

LOCAL_C_INCLUDES := \
	$(common_C_INCLUDES) \
	external/libxml2/include \
	external/icu4c/common \

LOCAL_SRC_FILES:= \
        $(LOCAL_MODULE).c

LOCAL_CFLAGS := $(common_CFLAGS)

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_LDFLAGS := -Wl,--allow-shlib-undefined

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/PowerDaemon
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/PowerDaemon
include $(BUILD_SHARED_LIBRARY)

# the plugin of thermal
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := plugin_thermal

LOCAL_C_INCLUDES := \
	$(common_C_INCLUDES) \
	external/libxml2/include \
	external/icu4c/common \
	$(TOPDIR)vendor/marvell/external/sysfsutils/sysfs

LOCAL_SRC_FILES:= \
        $(LOCAL_MODULE).c

LOCAL_CFLAGS := $(common_CFLAGS)

LOCAL_SHARED_LIBRARIES := libsysfs libcutils
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_LDFLAGS := -Wl,--allow-shlib-undefined

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/PowerDaemon
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/PowerDaemon
include $(BUILD_SHARED_LIBRARY)

# the plugin of ddr_hotplug
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := plugin_ddr_hotplug

LOCAL_C_INCLUDES := \
	$(common_C_INCLUDES) \
	external/libxml2/include \
	external/icu4c/common \
	$(TOPDIR)vendor/marvell/external/sysfsutils/sysfs

LOCAL_SRC_FILES:= \
        $(LOCAL_MODULE).c

LOCAL_CFLAGS := $(common_CFLAGS)

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_LDFLAGS := -Wl,--allow-shlib-undefined

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/PowerDaemon
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/PowerDaemon
include $(BUILD_SHARED_LIBRARY)

# the plugin of input event
# =====================================================
include $(CLEAR_VARS)
LOCAL_MODULE := plugin_input

LOCAL_C_INCLUDES := \
	$(common_C_INCLUDES) \
	external/libxml2/include \
	external/icu4c/common \
    frameworks/base/services/input

LOCAL_SRC_FILES:= \
        $(LOCAL_MODULE).cpp

LOCAL_CFLAGS := $(common_CCFLAGS)

LOCAL_SHARED_LIBRARIES := libinput libutils

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_LDFLAGS := -Wl,--allow-shlib-undefined

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/PowerDaemon
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/PowerDaemon
include $(BUILD_SHARED_LIBRARY)

include $(PLUGIN_THERMAL_JAVA_PATH)/Android.mk
include $(PLUGIN_DDR_HOTPLUG_JAVA_PATH)/Android.mk

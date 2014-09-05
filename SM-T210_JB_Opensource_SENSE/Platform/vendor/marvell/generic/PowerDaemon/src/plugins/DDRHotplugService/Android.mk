LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# Only compile source java files in this apk.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := DDRHotplugService
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)

include $(LOCAL_PATH)/jni/Android.mk

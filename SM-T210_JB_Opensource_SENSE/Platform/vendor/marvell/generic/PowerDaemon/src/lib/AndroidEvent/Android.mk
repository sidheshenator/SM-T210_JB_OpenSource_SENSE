LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_JNI_SHARED_LIBRARIES := libAndroidEvent

LOCAL_PACKAGE_NAME := EventRelay
LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

include $(BUILD_PACKAGE)

include $(LOCAL_PATH)/jni/Android.mk

LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := libdumpsys

LOCAL_SRC_FILES := Dumpsys.cpp

LOCAL_C_INCLUDES := $(ROOT_PATH)/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -std=c++23 -O2
LOCAL_CPPFLAGS += -Wpedantic -Wall -Wextra -Werror -Wformat -Wuninitialized

include $(BUILD_STATIC_LIBRARY)

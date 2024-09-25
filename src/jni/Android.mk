LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := encore-service
LOCAL_SRC_FILES := encore-service.c
LOCAL_LDFLAGS := -static
LOCAL_CFLAGS := -Wall -O2
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := vmtouch
LOCAL_SRC_FILES := vmtouch.c
LOCAL_LDFLAGS := -static
LOCAL_CFLAGS := -Wall -O2 -g -std=c99
include $(BUILD_EXECUTABLE)

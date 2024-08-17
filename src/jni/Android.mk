LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := encore-service
LOCAL_SRC_FILES := encore-service.c
LOCAL_LDFLAGS := -static

include $(BUILD_EXECUTABLE)

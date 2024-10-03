LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := encore-service
LOCAL_SRC_FILES := encore-service.c
LOCAL_LDFLAGS := -static
LOCAL_CFLAGS := -Wall -Wextra -Werror -pedantic-errors -Wpedantic -O2
include $(BUILD_EXECUTABLE)

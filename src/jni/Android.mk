LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := encored
LOCAL_SRC_FILES := encored.c
LOCAL_LDFLAGS := -static
LOCAL_CFLAGS := -Wall -Wextra -Werror -pedantic-errors -Wpedantic -O2
include $(BUILD_EXECUTABLE)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := encored
LOCAL_SRC_FILES := encored.c
LOCAL_LDFLAGS := -static
LOCAL_CFLAGS := -Wall -Wextra -Werror -pedantic-errors -Wpedantic -O2
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := vmtouch
LOCAL_SRC_FILES := vmtouch.c
LOCAL_LDFLAGS := -static
LOCAL_CFLAGS := -Wall -Wextra -Werror -pedantic-errors -Wpedantic -O2 -std=c99
include $(BUILD_EXECUTABLE)

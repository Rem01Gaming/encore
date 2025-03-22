LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := encored
LOCAL_SRC_FILES := \
    main.c \
    src/cmd_utils.c \
    src/encore_log.c \
    src/encore_profiler.c \
    src/file_utils.c \
    src/ksu_utils.c \
    src/process_utils.c \
    src/misc_utils.c \
    src/mlbb_handler.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_CFLAGS := -DNDEBUG -Wall -Wextra -Werror \
                -pedantic-errors -Wpedantic \
                -O2 -std=c23 -fPIC -flto

LOCAL_LDFLAGS := -static -flto
include $(BUILD_EXECUTABLE)

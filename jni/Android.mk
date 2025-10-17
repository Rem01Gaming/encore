LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := encored

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
    $(LOCAL_PATH)/external/rapidjson/include \
    $(LOCAL_PATH)/external/spdlog/include \
    $(LOCAL_PATH)/external/x-watcher/include

LOCAL_STATIC_LIBRARIES := libdumpsys libencorecfg libencoreutil

LOCAL_SRC_FILES := Main.cpp

LOCAL_CPPFLAGS += -fexceptions -std=c++23 -O2 -flto
LOCAL_CPPFLAGS += -Wpedantic -Wall -Wextra -Werror -Wformat -Wuninitialized

LOCAL_LDFLAGS += -flto

include $(BUILD_EXECUTABLE)

include $(LOCAL_PATH)/src/Android.mk

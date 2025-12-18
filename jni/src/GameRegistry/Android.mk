LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := GameRegistry

LOCAL_SRC_FILES := GameRegistry.cpp

LOCAL_C_INCLUDES := \
	$(ROOT_PATH)/include \
    $(ROOT_PATH)/external/rapidjson/include \
    $(ROOT_PATH)/external/spdlog/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -std=c++23 -O0
LOCAL_CPPFLAGS += -Wpedantic -Wall -Wextra -Werror -Wformat -Wuninitialized

include $(BUILD_STATIC_LIBRARY)

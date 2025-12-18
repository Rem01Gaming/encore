LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)

LOCAL_MODULE := EncoreConfig
LOCAL_SRC_FILES := InotifyHandler.cpp EncoreConfigStore.cpp

LOCAL_STATIC_LIBRARIES := GameRegistry

LOCAL_C_INCLUDES := \
	$(ROOT_PATH)/include \
    $(ROOT_PATH)/external/rapidjson/include \
    $(ROOT_PATH)/external/spdlog/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -std=c++23 -O0
LOCAL_CPPFLAGS += -Wpedantic -Wall -Wextra -Werror -Wformat -Wuninitialized

include $(BUILD_STATIC_LIBRARY)

LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)

LOCAL_MODULE := EncoreConfig

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_SRC_FILES := $(LOCAL_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_STATIC_LIBRARIES := SystemStatus InotifyWatcher GameRegistry DeviceInfo EncoreUtility

LOCAL_C_INCLUDES := \
	$(ROOT_PATH)/include \
    $(ROOT_PATH)/external/rapidjson/include \
    $(ROOT_PATH)/external/spdlog/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -std=c++23 -O0
LOCAL_CPPFLAGS += -Wpedantic -Wall -Wextra -Werror -Wformat -Wuninitialized

include $(BUILD_STATIC_LIBRARY)

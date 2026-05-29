LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := rapidjson
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/rapidjson/include
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := spdlog
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/spdlog/include
include $(BUILD_STATIC_LIBRARY)

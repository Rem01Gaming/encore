LOCAL_PATH := $(call my-dir)
ROOT_PATH := $(call my-dir)/../..

include $(CLEAR_VARS)
LOCAL_MODULE := EncoreUtility

LOCAL_C_INCLUDES := \
	$(ROOT_PATH)/include \
	$(ROOT_PATH)/external/rapidjson/include \
    $(ROOT_PATH)/external/spdlog/include

LOCAL_SRC_FILES := \
	FileUtility.cpp \
	MiscUtility.cpp \
	ProcessUtility.cpp \
	Profiler.cpp

LOCAL_STATIC_LIBRARIES := EncoreConfig

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_CPPFLAGS += -fexceptions -std=c++23 -O2
LOCAL_CPPFLAGS += -Wpedantic -Wall -Wextra -Werror -Wformat -Wuninitialized

include $(BUILD_STATIC_LIBRARY)

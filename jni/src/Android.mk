LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

SUBDIRS := $(wildcard $(LOCAL_PATH)/*/)
SUB_ANDROID_MKS := $(addsuffix Android.mk, $(SUBDIRS))
SUB_ANDROID_MKS := $(wildcard $(SUB_ANDROID_MKS))

$(foreach mk, $(SUB_ANDROID_MKS), $(eval include $(mk)))

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/SystemStatus/Android.mk \
	$(LOCAL_PATH)/InotifyWatcher/Android.mk \
	$(LOCAL_PATH)/LockFile/Android.mk \
	$(LOCAL_PATH)/PIDTracker/Android.mk \
	$(LOCAL_PATH)/EncoreCLI/Android.mk \
	$(LOCAL_PATH)/GameRegistry/Android.mk \
	$(LOCAL_PATH)/DeviceInfo/Android.mk \
	$(LOCAL_PATH)/EncoreConfig/Android.mk \
	$(LOCAL_PATH)/EncoreUtility/Android.mk

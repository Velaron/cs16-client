#hlsdk-2.3 client port for android
#Copyright (c) mittorn

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := client
LOCAL_CONLYFLAGS += -std=c99

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libclient_hardfp
endif

LOCAL_CFLAGS += -fsigned-char -DCLIENT_DLL=1 -DCLIENT_WEAPONS=1 -Wno-write-strings -DLINUX -D_LINUX -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -DCLIENT_WEAPONS -DCLIENT_DLL -Wl,--no-undefined
LOCAL_CPPFLAGS += -std=gnu++11

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,,$(shell find $(LOCAL_PATH) -name *.cpp)) \
	$(subst $(LOCAL_PATH)/,,$(shell find $(LOCAL_PATH)/../dlls/wpn_shared -name *.cpp)) \
	$(subst $(LOCAL_PATH)/,,$(shell find $(LOCAL_PATH)/../pm_shared -name *.cpp)) \
	../common/interface.cpp 

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/include/hud \
	$(LOCAL_PATH)/include/studio \
	$(LOCAL_PATH)/include/math \
	$(LOCAL_PATH)/../common \
	$(LOCAL_PATH)/../engine \
	$(LOCAL_PATH)/../game_shared \
	$(LOCAL_PATH)/../dlls \
	$(LOCAL_PATH)/../pm_shared \
	$(LOCAL_PATH)/../public

include $(BUILD_SHARED_LIBRARY)

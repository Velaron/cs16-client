#Xash3d mainui port for android
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libmenu_hardfp
endif



LOCAL_MODULE := menu
LOCAL_CPPFLAGS := -std=gnu++11 -DMAINUI_STUB

APP_PLATFORM := android-9

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
		    $(LOCAL_PATH)/.			    \
		    $(LOCAL_PATH)/../common \
	            $(LOCAL_PATH)/../pm_shared \
	            $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../engine/common \
		    $(LOCAL_PATH)/../utils/vgui/include \
		    $(LOCAL_PATH)/menus \
		    $(LOCAL_PATH)/controls

LOCAL_SRC_FILES := udll_int.cpp

include $(BUILD_SHARED_LIBRARY)

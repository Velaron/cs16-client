#hlsdk-2.3 client port for android
#Copyright (c) mittorn

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := client
APP_PLATFORM := android-8
LOCAL_CONLYFLAGS += -std=c99

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libclient_hardfp
endif

LOCAL_CFLAGS += -fsigned-char -DCLIENT_DLL=1 -DCLIENT_WEAPONS=1
LOCAL_CPPFLAGS += -std=c++0x
SRCS := \
	./cdll_int.cpp ./demo.cpp ./entity.cpp ./in_camera.cpp ./input.cpp ./rain.cpp \
	./tri.cpp ./util.cpp ./view.cpp ./input_xash3d.cpp ./vgui_parser.cpp \
	./unicode_strtools.cpp ./draw_util.cpp ./cvar_checker.cpp \
	../pm_shared/pm_debug.cpp ../pm_shared/pm_math.cpp ../pm_shared/pm_shared.cpp \
	../common/interface.cpp \
	./studio/GameStudioModelRenderer.cpp ./studio/StudioModelRenderer.cpp ./studio/studio_util.cpp

# I hate Make
SRCS += $(wildcard ../cl_dll/hud/*.cpp)
SRCS += $(wildcard ../cl_dll/cs_wpn/*.cpp)
SRCS += $(wildcard ../cl_dll/events/*.cpp)
SRCS += $(wildcard ../dlls/wpn_shared/*.cpp)

SRCS +=	../mainui/controls/Framework.cpp ../mainui/controls/BaseItem.cpp \
	../mainui/controls/Action.cpp ../mainui/controls/Bitmap.cpp \
	../mainui/controls/CheckBox.cpp ../mainui/controls/ItemsHolder.cpp \
	../mainui/controls/Field.cpp ../mainui/controls/PicButton.cpp \
	../mainui/controls/ScrollList.cpp ../mainui/controls/Slider.cpp \
	../mainui/controls/SpinControl.cpp ../mainui/controls/YesNoMessageBox.cpp \
	../mainui/controls/MessageBox.cpp ../mainui/controls/Editable.h \
	../mainui/controls/Switch.cpp ../mainui/controls/BaseWindow.cpp \
	../mainui/controls/ProgressBar.cpp ../mainui/controls/BackgroundBitmap.cpp \
	../mainui/menus/Audio.cpp ../mainui/menus/Configuration.cpp \
	../mainui/menus/Controls.cpp ../mainui/menus/CreateGame.cpp \
	../mainui/menus/Credits.cpp ../mainui/menus/FileDialog.cpp \
	../mainui/menus/GameOptions.cpp ../mainui/menus/Gamepad.cpp \
	../mainui/menus/Main.cpp ../mainui/menus/Multiplayer.cpp \
	../mainui/menus/PlayerSetup.cpp ../mainui/menus/ServerBrowser.cpp \
	../mainui/menus/TouchButtons.cpp ../mainui/menus/Touch.cpp \
	../mainui/menus/TouchEdit.cpp ../mainui/menus/TouchOptions.cpp \
	../mainui/menus/Video.cpp ../mainui/menus/ConnectionProgress.cpp \
	../mainui/menus/dynamic/ScriptMenu.cpp \
	../mainui/menus/client/BaseClientMenu.cpp \
	../mainui/menus/client/SpectatorMenu.cpp \
	../mainui/menus/client/JoinMenu.cpp \
	../mainui/menus/client/BuyMenu.cpp \
	../mainui/BaseMenu.cpp ../mainui/Btns.cpp ../mainui/MenuStrings.cpp \
	../mainui/CFGScript.cpp ../mainui/Utils.cpp ../mainui/Scissor.cpp \
	../mainui/udll_int.cpp


DEFINES = -Wno-write-strings -DLINUX -D_LINUX -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -DCLIENT_WEAPONS -DCLIENT_DLL -Wl,--no-undefined

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
	$(LOCAL_PATH)/../public \
	$(LOCAL_PATH)/../mainui \
	$(LOCAL_PATH)/../mainui/menus \
	$(LOCAL_PATH)/../mainui/controls \
	$(LOCAL_PATH)/../mainui/menus/dynamic \
	$(LOCAL_PATH)/../mainui/font/ \
	$(LOCAL_PATH)/../mainui/utl/
LOCAL_CFLAGS += $(DEFINES)

LOCAL_SRC_FILES := $(SRCS)

include $(BUILD_SHARED_LIBRARY)

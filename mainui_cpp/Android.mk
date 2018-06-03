#Xash3d mainui port for android
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
	LOCAL_MODULE_FILENAME = libmenu_hardfp
endif

LOCAL_MODULE := menu
LOCAL_CPPFLAGS := -std=gnu++11 -DMAINUI_USE_STB -DMAINUI_USE_CUSTOM_FONT_RENDER -DNO_STL -fno-rtti -fno-exceptions

LOCAL_C_INCLUDES := \
	$(SDL_PATH)/include				\
	$(LOCAL_PATH)/. 				\
	$(LOCAL_PATH)/../common 			\
	$(LOCAL_PATH)/../pm_shared 			\
	$(LOCAL_PATH)/../engine 			\
	$(LOCAL_PATH)/../engine/common 			\
	$(LOCAL_PATH)/../utils/vgui/include 		\
	$(LOCAL_PATH)/menus 				\
	$(LOCAL_PATH)/controls 				\
	$(LOCAL_PATH)/font 				\
	$(LOCAL_PATH)/utl 				\
	$(LOCAL_PATH)/model				\

LOCAL_SRC_FILES := \
	controls/Framework.cpp                          \
	controls/BaseItem.cpp                           \
	controls/Action.cpp                             \
	controls/Bitmap.cpp                             \
	controls/Editable.cpp                           \
	controls/CheckBox.cpp                           \
	controls/ItemsHolder.cpp                        \
	controls/Field.cpp                              \
	controls/PicButton.cpp                          \
	controls/Slider.cpp                             \
	controls/SpinControl.cpp                        \
	controls/YesNoMessageBox.cpp                    \
	controls/MessageBox.cpp                         \
	controls/Switch.cpp                             \
	controls/ProgressBar.cpp                        \
	controls/BaseWindow.cpp                         \
	controls/BackgroundBitmap.cpp                   \
	controls/Table.cpp                              \
	controls/PlayerModelView.cpp                    \
	menus/AdvancedControls.cpp                      \
	menus/Audio.cpp                                 \
	menus/Configuration.cpp                         \
	menus/ConnectionProgress.cpp                    \
	menus/ConnectionWarning.cpp                     \
	menus/Controls.cpp                              \
	menus/CreateGame.cpp                            \
	menus/Credits.cpp                               \
	menus/CustomGame.cpp                            \
	menus/FileDialog.cpp                            \
	menus/GameOptions.cpp                           \
	menus/Gamepad.cpp                               \
	menus/InputDevices.cpp                          \
	menus/LoadGame.cpp                              \
	menus/Main.cpp                                  \
	menus/Multiplayer.cpp                           \
	menus/NewGame.cpp                               \
	menus/PlayerIntroduceDialog.cpp                 \
	menus/PlayerSetup.cpp                           \
	menus/SaveLoad.cpp                              \
	menus/ServerBrowser.cpp                         \
	menus/TouchButtons.cpp                          \
	menus/Touch.cpp                                 \
	menus/TouchEdit.cpp                             \
	menus/TouchOptions.cpp                          \
	menus/Video.cpp                                 \
	menus/VideoModes.cpp                            \
	menus/VideoOptions.cpp                          \
	menus/dynamic/ScriptMenu.cpp                    \
	font/FontManager.cpp				\
	font/BaseFontBackend.cpp			\
	font/StbFont.cpp				\
	font/BitmapFont.cpp				\
	EventSystem.cpp                                 \
	BaseMenu.cpp                                    \
	Btns.cpp                                        \
	MenuStrings.cpp                                 \
	Utils.cpp                                       \
	Scissor.cpp                                     \
	udll_int.cpp                                    \
	CFGScript.cpp					\
	EngineCallback.cpp

include $(BUILD_SHARED_LIBRARY)

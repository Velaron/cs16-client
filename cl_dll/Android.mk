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
	./cdll_int.cpp \
	./demo.cpp \
	./entity.cpp \
	./in_camera.cpp \
	./input.cpp \
	./rain.cpp \
	./tri.cpp \
	./util.cpp \
	./view.cpp \
	./input_xash3d.cpp \
	./vgui_parser.cpp \
	./unicode_strtools.cpp \
	./draw_util.cpp \
	../pm_shared/pm_debug.cpp \
	../pm_shared/pm_math.cpp \
	../pm_shared/pm_shared.cpp \
	./studio/GameStudioModelRenderer.cpp \
	./studio/StudioModelRenderer.cpp \
	./studio/studio_util.cpp \
	./hud/ammo.cpp \
	./hud/ammo_secondary.cpp \
	./hud/ammohistory.cpp \
	./hud/battery.cpp \
	./hud/geiger.cpp \
	./hud/health.cpp \
	./hud/hud.cpp \
	./hud/hud_msg.cpp \
	./hud/hud_redraw.cpp \
	./hud/hud_spectator.cpp \
	./hud/hud_update.cpp \
	./hud/sniperscope.cpp \
	./hud/nvg.cpp \
	./hud/menu.cpp \
	./hud/message.cpp \
	./hud/status_icons.cpp \
	./hud/statusbar.cpp \
	./hud/text_message.cpp \
	./hud/train.cpp \
	./hud/scoreboard.cpp \
	./hud/MOTD.cpp \
	./hud/radio.cpp \
	./hud/radar.cpp \
	./hud/timer.cpp \
	./hud/money.cpp \
	./hud/flashlight.cpp \
	./hud/death.cpp \
	./hud/saytext.cpp \
	./hud/spectator_gui.cpp \
	./com_weapons.cpp \
	./cs_wpn/cs_baseentity.cpp \
	./cs_wpn/cs_weapons.cpp \
	../dlls/wpn_shared/wpn_ak47.cpp \
	../dlls/wpn_shared/wpn_aug.cpp \
	../dlls/wpn_shared/wpn_awp.cpp \
	../dlls/wpn_shared/wpn_c4.cpp \
	../dlls/wpn_shared/wpn_deagle.cpp \
	../dlls/wpn_shared/wpn_elite.cpp \
	../dlls/wpn_shared/wpn_famas.cpp \
	../dlls/wpn_shared/wpn_fiveseven.cpp \
	../dlls/wpn_shared/wpn_flashbang.cpp \
	../dlls/wpn_shared/wpn_g3sg1.cpp \
	../dlls/wpn_shared/wpn_galil.cpp \
	../dlls/wpn_shared/wpn_glock18.cpp \
	../dlls/wpn_shared/wpn_hegrenade.cpp \
	../dlls/wpn_shared/wpn_knife.cpp \
	../dlls/wpn_shared/wpn_m249.cpp \
	../dlls/wpn_shared/wpn_m3.cpp \
	../dlls/wpn_shared/wpn_m4a1.cpp \
	../dlls/wpn_shared/wpn_mac10.cpp \
	../dlls/wpn_shared/wpn_mp5navy.cpp \
	../dlls/wpn_shared/wpn_p228.cpp \
	../dlls/wpn_shared/wpn_p90.cpp \
	../dlls/wpn_shared/wpn_scout.cpp \
	../dlls/wpn_shared/wpn_sg550.cpp \
	../dlls/wpn_shared/wpn_sg552.cpp \
	../dlls/wpn_shared/wpn_smokegrenade.cpp \
	../dlls/wpn_shared/wpn_tmp.cpp \
	../dlls/wpn_shared/wpn_ump45.cpp \
	../dlls/wpn_shared/wpn_usp.cpp \
	../dlls/wpn_shared/wpn_xm1014.cpp \
	./events/ev_cs16.cpp \
	./events/event_ak47.cpp \
	./events/event_aug.cpp \
	./events/event_awp.cpp \
	./events/event_createexplo.cpp \
	./events/event_createsmoke.cpp \
	./events/event_deagle.cpp \
	./events/event_decal_reset.cpp \
	./events/event_elite_left.cpp \
	./events/event_elite_right.cpp \
	./events/event_famas.cpp \
	./events/event_fiveseven.cpp \
	./events/event_g3sg1.cpp \
	./events/event_galil.cpp \
	./events/event_glock18.cpp \
	./events/event_knife.cpp \
	./events/event_m249.cpp \
	./events/event_m3.cpp \
	./events/event_m4a1.cpp \
	./events/event_mac10.cpp \
	./events/event_mp5n.cpp \
	./events/event_p228.cpp \
	./events/event_p90.cpp \
	./events/event_scout.cpp \
	./events/event_sg550.cpp \
	./events/event_sg552.cpp \
	./events/event_tmp.cpp \
	./events/event_ump45.cpp \
	./events/event_usp.cpp \
	./events/event_vehicle.cpp \
	./events/event_xm1014.cpp \
	./events/hl_events.cpp \
	./ev_common.cpp

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
	$(LOCAL_PATH)/../public
LOCAL_CFLAGS += $(DEFINES)

LOCAL_SRC_FILES := $(SRCS)

include $(BUILD_SHARED_LIBRARY)

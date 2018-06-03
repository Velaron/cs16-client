#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 2.6.0)
project (CLDLL CXX)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter -Wl,--no-undefined)
set(CMAKE_CXX_STANDARD 11)

# force build 32bit libraries
if(CMAKE_SIZEOF_VOID_P MATCHES "8")
    if(APPLE)
        SET(CMAKE_OSX_ARCHITECTURES "i386")
    else()
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    endif()
endif()

# find_package(SDL2 REQUIRED)

set (CLDLL_LIBRARY client)

set (WEAPONEVENT_SRCS
	./events/ev_cs16.cpp
	./events/event_ak47.cpp
	./events/event_aug.cpp
	./events/event_awp.cpp
	./events/event_createexplo.cpp
	./events/event_createsmoke.cpp
	./events/event_deagle.cpp
	./events/event_decal_reset.cpp
	./events/event_elite_left.cpp
	./events/event_elite_right.cpp
	./events/event_famas.cpp
	./events/event_fiveseven.cpp
	./events/event_g3sg1.cpp
	./events/event_galil.cpp
	./events/event_glock18.cpp
	./events/event_knife.cpp
	./events/event_m249.cpp
	./events/event_m3.cpp
	./events/event_m4a1.cpp
	./events/event_mac10.cpp
	./events/event_mp5n.cpp
	./events/event_p228.cpp
	./events/event_p90.cpp
	./events/event_scout.cpp
	./events/event_sg550.cpp
	./events/event_sg552.cpp
	./events/event_tmp.cpp
	./events/event_ump45.cpp
	./events/event_usp.cpp
	./events/event_vehicle.cpp
	./events/event_xm1014.cpp
	./events/event_czerods.cpp
	./events/hl_events.cpp
	./events/ev_common.cpp
)

set (CLWEAPON_SRCS
	./cs_wpn/com_weapons.cpp
	./cs_wpn/cs_weapons.cpp
	../dlls/wpn_shared/wpn_ak47.cpp
	../dlls/wpn_shared/wpn_aug.cpp
	../dlls/wpn_shared/wpn_awp.cpp
	../dlls/wpn_shared/wpn_c4.cpp
	../dlls/wpn_shared/wpn_deagle.cpp
	../dlls/wpn_shared/wpn_elite.cpp
	../dlls/wpn_shared/wpn_famas.cpp
	../dlls/wpn_shared/wpn_fiveseven.cpp
	../dlls/wpn_shared/wpn_flashbang.cpp
	../dlls/wpn_shared/wpn_g3sg1.cpp
	../dlls/wpn_shared/wpn_galil.cpp
	../dlls/wpn_shared/wpn_glock18.cpp
	../dlls/wpn_shared/wpn_hegrenade.cpp
	../dlls/wpn_shared/wpn_knife.cpp
	../dlls/wpn_shared/wpn_m249.cpp
	../dlls/wpn_shared/wpn_m3.cpp
	../dlls/wpn_shared/wpn_m4a1.cpp
	../dlls/wpn_shared/wpn_mac10.cpp
	../dlls/wpn_shared/wpn_mp5navy.cpp
	../dlls/wpn_shared/wpn_p228.cpp
	../dlls/wpn_shared/wpn_p90.cpp
	../dlls/wpn_shared/wpn_scout.cpp
	../dlls/wpn_shared/wpn_sg550.cpp
	../dlls/wpn_shared/wpn_sg552.cpp
	../dlls/wpn_shared/wpn_smokegrenade.cpp
	../dlls/wpn_shared/wpn_tmp.cpp
	../dlls/wpn_shared/wpn_ump45.cpp
	../dlls/wpn_shared/wpn_usp.cpp
	../dlls/wpn_shared/wpn_xm1014.cpp
)

set (HUD_SRCS
	./hud/ammo.cpp
	./hud/ammo_secondary.cpp
	./hud/ammohistory.cpp
	./hud/battery.cpp
	./hud/geiger.cpp
	./hud/health.cpp
	./hud/hud.cpp
	./hud/hud_msg.cpp
	./hud/hud_redraw.cpp
	./hud/hud_spectator.cpp
	./hud/hud_update.cpp
	./hud/sniperscope.cpp
	./hud/nvg.cpp
	./hud/menu.cpp
	./hud/message.cpp
	./hud/status_icons.cpp
	./hud/statusbar.cpp
	./hud/text_message.cpp
	./hud/train.cpp
	./hud/scoreboard.cpp
	./hud/MOTD.cpp
	./hud/radio.cpp
	./hud/radar.cpp
	./hud/timer.cpp
	./hud/money.cpp
	./hud/flashlight.cpp
	./hud/death.cpp
	./hud/saytext.cpp
	./hud/spectator_gui.cpp
	./hud/scenario.cpp

	./include/hud/ammo.h
	./include/hud/ammohistory.h
	./include/hud/health.h
	./include/hud/hud.h
	./include/hud/hud_iface.h
	./include/hud/hud_spectator.h
	./include/hud/radar.h
)

set (STUDIORENDER_SRCS
	./studio/GameStudioModelRenderer.cpp
	./studio/StudioModelRenderer.cpp
	./studio/studio_util.cpp

	./include/studio/GameStudioModelRenderer.h
	./include/studio/StudioModelRenderer.h
	./include/studio/studio_util.h

)

set (PLAYERMOVE_SRCS
	../pm_shared/pm_debug.cpp
	../pm_shared/pm_math.cpp
	../pm_shared/pm_shared.cpp
)

set (CLDLL_SOURCES
	./cdll_int.cpp
    ./demo.cpp
    ./entity.cpp
    ./in_camera.cpp
    ./input.cpp
	./rain.cpp
    ./tri.cpp
	./util.cpp
	./view.cpp
	#./input_xash3d.cpp
	./input_sdl.cpp
    ./vgui_parser.cpp
    ./unicode_strtools.cpp
	./draw_util.cpp
	../common/interface.cpp
	./cvar_checker.cpp

	./include/camera.h
	./include/cl_dll.h
	./include/cl_util.h
	./include/com_weapons.h
	./include/csprite.h
	./include/demo.h
	./include/draw_util.h
	./include/eventscripts.h
	./include/events.h
	./include/ev_hldm.h
	./include/overview.h
	./include/parsemsg.h
	./include/rain.h
	./include/tf_defs.h
	./include/unicode_strtools.h
	./include/util_vector.h
	./include/vgui_parser.h
	./include/view.h
	./include/wrect.h
	./include/math/neon_mathfun.h
	./include/math/sse_mathfun.h
	./include/cvar_checker.h
	../public/cl_dll/IGameClientExports.h

	${PLAYERMOVE_SRCS}
	${HUD_SRCS}
	${WEAPONEVENT_SRCS}
	${CLWEAPON_SRCS}
	${STUDIORENDER_SRCS}
)

include_directories (
	include/
	include/hud/
	include/studio/
	include/math/
	../cl_dll/
	../common/
	../engine/
	../pm_shared/
	../dlls/
	../game_shared/
	../public/
	../mainui/
	../mainui/controls
	../mainui/menus
	/usr/include/SDL2 )
add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})
add_definitions( -D_CS16CLIENT_ENABLE_GSRC_SUPPORT
	-DLINUX -D_LINUX -DCLIENT_WEAPONS -DCLIENT_DLL
	-D_DEBUG  -D_CS16CLIENT_ALLOW_SPECIAL_SCRIPTING
	-Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp )
add_compile_options(-Wl,--no-undefined)

if( CS16CLIENT_PRINT_SOURCES )
	get_target_property(MY_PROJECT_SOURCES ${CLDLL_LIBRARY} SOURCES)
	foreach( i IN LISTS MY_PROJECT_SOURCES)
		message(${i}   \\)
	endforeach()
endif()

if( CS16CLIENT_ANALYZE )
	include(../cmake/PVS-Studio.cmake)
	pvs_studio_add_target(TARGET analyze ALL
		FORMAT tasklist
		LOG "${CMAKE_CURRENT_LIST_DIR}/cs16client.tasks"
		ANALYZE ${CLDLL_LIBRARY})
endif()

target_link_libraries( ${CLDLL_LIBRARY} ${CMAKE_DL_LIBS} -lSDL2 )


set_target_properties (${CLDLL_SHARED} PROPERTIES
	VERSION 1.6 SOVERSION 1.6
	POSITION_INDEPENDENT_CODE 1)

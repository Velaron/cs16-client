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
project (CLDLL)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLINUX -D_LINUX -DCLIENT_WEAPONS -DCLIENT_DLL")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -Wl,--no-undefined")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set (CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g -O3")

set (CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
set (CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
set (CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

set (CLDLL_LIBRARY client)

set (CLDLL_SOURCES ./hl/hl_baseentity.cpp
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
	./events/events.h
	./events/event_tmp.cpp
	./events/event_ump45.cpp
	./events/event_usp.cpp
	./events/event_vehicle.cpp
	./events/event_xm1014.cpp
	./events/hl_events.cpp
    ./hl/hl_objects.cpp
    ./hl/hl_weapons.cpp
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
    ../common/interface.cpp
    ./ammo.cpp
    ./ammo_secondary.cpp
    ./ammohistory.cpp
    ./battery.cpp
    ./cdll_int.cpp
    ./com_weapons.cpp
    ./death.cpp
    ./demo.cpp
    ./entity.cpp
    ./ev_common.cpp
    ./events.cpp
    ./flashlight.cpp
    ./GameStudioModelRenderer.cpp
    ./geiger.cpp
    ./health.cpp
    ./hud.cpp
    ./hud_msg.cpp
    ./hud_redraw.cpp
    ./hud_spectator.cpp
    ./hud_update.cpp
    ./sniperscope.cpp
    ./nvg.cpp
    ./in_camera.cpp
    ./input.cpp
    ./menu.cpp
    ./message.cpp
    ./overview.cpp
    ./parsemsg.cpp
    ../pm_shared/pm_debug.c
    ../pm_shared/pm_math.c
    ../pm_shared/pm_shared.c
    ./rain.cpp
    ./saytext.cpp
    ./status_icons.cpp
    ./statusbar.cpp
    ./studio_util.cpp
    ./StudioModelRenderer.cpp
    ./text_message.cpp
    ./train.cpp
    ./tri.cpp
    ./util.cpp
    ./view.cpp
    ./input_xash3d.cpp
    ./scoreboard.cpp
    ./MOTD.cpp
    ./radio.cpp
    ./timer.cpp
    ./money.cpp
    ./vgui_parser.cpp
)

include_directories (../cl_dll ../common ../engine ../pm_shared ../dlls ../game_shared ../public)
add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})
target_link_libraries( ${CLDLL_LIBRARY} ${CMAKE_DL_LIBS} )


set_target_properties (${CLDLL_SHARED} PROPERTIES
	VERSION 1.6 SOVERSION 1.6
	POSITION_INDEPENDENT_CODE 1)

cmake_minimum_required(VERSION 2.8.12)
project(client)

set(CLIENT_LIB client)

file(GLOB CS_CLIENT_SRC "*.cpp")
file(GLOB CS_EV_SRC "events/*.cpp")
file(GLOB CS_WPN_SRC "cs_wpn/*.cpp")
file(GLOB CS_WPNSH_SRC "../dlls/wpn_shared/*.cpp")
file(GLOB CS_HUD_SRC "hud/*.cpp")
file(GLOB CS_STUDIO_SRC "studio/*.cpp")
file(GLOB CS_PM_SRC "../pm_shared/*.cpp")

list(REMOVE_ITEM CS_CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/ev_hldm.cpp")
list(REMOVE_ITEM CS_CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/input_sdl.cpp")
list(REMOVE_ITEM CS_CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/inputw32.cpp")
list(REMOVE_ITEM CS_WPN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/cs_wpn/com_weapons.cpp")
list(APPEND CS_CLIENT_SRC ../common/interface.cpp)

list(APPEND CS_CLIENT_SRC ${CS_EV_SRC})
list(APPEND CS_CLIENT_SRC ${CS_WPN_SRC})
list(APPEND CS_CLIENT_SRC ${CS_WPNSH_SRC})
list(APPEND CS_CLIENT_SRC ${CS_HUD_SRC})
list(APPEND CS_CLIENT_SRC ${CS_STUDIO_SRC})
list(APPEND CS_CLIENT_SRC ${CS_PM_SRC})

include_directories(
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
	../public/cl_dll
	../3rdparty/mainui_cpp/
	../3rdparty/mainui_cpp/controls
	../3rdparty/mainui_cpp/menus
	../3rdparty/mainui_cpp/miniutl)

add_definitions(-DCLIENT_WEAPONS -DCLIENT_DLL)

if(NOT MSVC)
	add_definitions(-DLINUX -D_LINUX)
	add_definitions(-Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -fms-extensions)
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

add_library (${CLIENT_LIB} MODULE ${CS_CLIENT_SRC})

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND WIN32)
	set_target_properties(${CLIENT_LIB} PROPERTIES OUTPUT_NAME "client_amd64")
endif()

install(TARGETS ${CLIENT_LIB}
	DESTINATION "${GAME_DIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

# Install PDB file on Windows
if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	install(FILES $<TARGET_PDB_FILE:${CLIENT_LIB}>
		DESTINATION "${GAME_DIR}/${CLIENT_INSTALL_DIR}/" OPTIONAL)
endif()
cmake_minimum_required(VERSION 2.8.12)
project(cs16-client)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(LibraryNaming)

option(BUILD_CLIENT "Build client library." ON)
option(BUILD_MAINUI "Build menu library." ON)
option(BUILD_SERVER "Build server library." ON)

set(GAME_DIR "cstrike" CACHE STRING "Game directory name.")
set(CLIENT_INSTALL_DIR "cl_dlls" CACHE STRING "Client library location.")
set(SERVER_INSTALL_DIR "dlls" CACHE STRING "Server library location.")

# if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
# 	add_compile_options(/fsanitize=address)
# 	add_link_options(/fsanitize=address)
# endif()

if(VITA)
	message(STATUS "Building for PS Vita")
	add_compile_options(-fno-use-cxa-atexit)
endif()

if(BUILD_CLIENT)
	add_subdirectory(cl_dll)
endif()

if(BUILD_SERVER)
	add_subdirectory(3rdparty/yapb)
	add_subdirectory(3rdparty/ReGameDLL_CS)
endif()

if(BUILD_MAINUI)
#	set(CS16CLIENT ON)
	# set(XASH_SDK ../../)
	add_subdirectory(3rdparty/mainui_cpp)
endif()

set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)
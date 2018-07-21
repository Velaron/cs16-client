#
# Copyright (c) 2016-2017 Alibek Omarov
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

cmake_minimum_required(VERSION 3.0.0)
project (CS16CLIENT)

#--------------
# USER DEFINES \
################\

option(BUILD_CLIENT "Build client dll" ON)
option(BUILD_MAINUI "Build mainui dll" ON)
option(BUILD_SERVER "Build server dll" ON)
option(ALLOW64BUILD "Allow amd64 build" OFF)

#-----------------
# MAIN BUILD CODE \
###################\

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Build 32-bit Xash on 64-bit, because Xash3D not support this
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT ALLOW64BUILD )
	if(MSVC)
		error("UNDONE: set 32 build flags")
	else()
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
	endif()
endif()

if(BUILD_CLIENT)
	add_subdirectory(cl_dll)
endif()

if(BUILD_SERVER)
	add_subdirectory(3rdparty/ReGameDLL_CS/regamedll/dlls)
endif()

if(BUILD_MAINUI)
	add_subdirectory(mainui)
endif()

if(NOT BUILD_SERVER AND NOT BUILD_MAINUI)
	message(ERROR "Nothing to build")
endif()

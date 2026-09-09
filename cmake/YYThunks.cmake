set(YY_THUNKS_OBJ
	"${CMAKE_SOURCE_DIR}/3rdparty/yy-thunks/YY_Thunks_for_WinXP.obj"
	CACHE FILEPATH "Path to the Win32 YY-Thunks object")

set(YY_THUNKS_ENABLED FALSE)


if(ENABLE_YY_THUNKS AND WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND
	CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	if(NOT DEFINED ENV{YY_THUNKS_VERSION} OR "$ENV{YY_THUNKS_VERSION}" STREQUAL "")
		message(WARNING
			"YY-Thunks: YY_THUNKS_VERSION is not set, XP compatibility disabled")
	elseif(NOT DEFINED ENV{YY_THUNKS_OBJ_SHA256} OR "$ENV{YY_THUNKS_OBJ_SHA256}" STREQUAL "")
		message(WARNING
			"YY-Thunks: YY_THUNKS_OBJ_SHA256 is not set, XP compatibility disabled")
	else()
		set(YY_THUNKS_VERSION "$ENV{YY_THUNKS_VERSION}" CACHE STRING "YY-Thunks release version" FORCE)
		set(YY_THUNKS_EXPECTED_SHA256 "$ENV{YY_THUNKS_OBJ_SHA256}" CACHE STRING
			"SHA-256 of the YY-Thunks object" FORCE)

		if(NOT EXISTS "${YY_THUNKS_OBJ}")
			message(WARNING
				"YY-Thunks: ${YY_THUNKS_OBJ} not found, XP compatibility disabled")
		else()
			file(SHA256 "${YY_THUNKS_OBJ}" YY_THUNKS_ACTUAL_SHA256)
			if(NOT YY_THUNKS_ACTUAL_SHA256 STREQUAL YY_THUNKS_EXPECTED_SHA256)
				message(FATAL_ERROR
					"YY-Thunks checksum mismatch: expected ${YY_THUNKS_EXPECTED_SHA256}, "
					"got ${YY_THUNKS_ACTUAL_SHA256}")
			endif()

			set(YY_THUNKS_ENABLED TRUE)
			message(STATUS "YY-Thunks ${YY_THUNKS_VERSION} enabled for Win32 shared libraries")
		endif()
	endif()
endif()

function(yy_thunks_apply target)
	if(NOT YY_THUNKS_ENABLED)
		return()
	endif()

	if(NOT TARGET "${target}")
		message(FATAL_ERROR "Cannot apply YY-Thunks: target does not exist: ${target}")
	endif()

	get_target_property(target_type "${target}" TYPE)
	if(NOT target_type STREQUAL "SHARED_LIBRARY")
		message(FATAL_ERROR "YY-Thunks can only be applied to a shared library: ${target}")
	endif()

	set_property(TARGET "${target}" APPEND PROPERTY SOURCES "${YY_THUNKS_OBJ}")
	target_link_options("${target}" PRIVATE
		"/ENTRY:DllMainCRTStartupForYY_Thunks"
		"/alternatename:_YY_ThunksOriginalDllMainCRTStartup@12=__DllMainCRTStartup@12")
endfunction()
/*
dll_int.cpp - dll entry point
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef MAINUI_STUB
#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"

ui_enginefuncs_t EngFuncs::engfuncs;
ui_textfuncs_t	EngFuncs::textfuncs;
ui_globalvars_t	*gpGlobals;
CMenu gMenu;

static UI_FUNCTIONS gFunctionTable = 
{
	UI_VidInit,
	UI_Init,
	UI_Shutdown,
	UI_UpdateMenu,
	UI_KeyEvent,
	UI_MouseMove,
	UI_SetActiveMenu,
	UI_AddServerToList,
	UI_GetCursorPos,
	UI_SetCursorPos,
	UI_ShowCursor,
	UI_CharEvent,
	UI_MouseInRect,
	UI_IsVisible,
	UI_CreditsActive,
	UI_FinalCredits
};

//=======================================================================
//			GetApi
//=======================================================================
extern "C" EXPORT int GetMenuAPI(UI_FUNCTIONS *pFunctionTable, ui_enginefuncs_t* pEngfuncsFromEngine, ui_globalvars_t *pGlobals)
{
	if( !pFunctionTable || !pEngfuncsFromEngine )
	{
		return FALSE;
	}

	// copy HUD_FUNCTIONS table to engine, copy engfuncs table from engine
	memcpy( pFunctionTable, &gFunctionTable, sizeof( UI_FUNCTIONS ));
	memcpy( &EngFuncs::engfuncs, pEngfuncsFromEngine, sizeof( ui_enginefuncs_t ));
	memset( &EngFuncs::textfuncs, 0, sizeof( ui_textfuncs_t ));
	gpGlobals = pGlobals;

	return TRUE;
}

extern "C" EXPORT int GiveTextAPI( ui_textfuncs_t* pTextfuncsFromEngine )
{
	if( !pTextfuncsFromEngine )
	{
		return FALSE;
	}

	// copy HUD_FUNCTIONS table to engine, copy engfuncs table from engine
	memcpy( &EngFuncs::textfuncs, pTextfuncsFromEngine, sizeof( ui_textfuncs_t ));

	return TRUE;
}
#else // MAINUI_STUB

#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

typedef unsigned char byte;

#define EXPORT __attribute__((visibility("default")))

#include "menu_int.h"

void *cl_dll;

static UI_FUNCTIONS gRealFunctionTable = {};
static UI_FUNCTIONS gFakeFunctionTable = {};

extern "C"
{
	EXPORT int GetMenuAPI( UI_FUNCTIONS *pFunctionTable, ui_enginefuncs_t* pEngfuncsFromEngine, ui_globalvars_t *pGlobals );
	EXPORT int GiveTextAPI( ui_textfuncs_t* pTextfuncsFromEngine );
	EXPORT void AddTouchButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags );
}

void UI_Shutdown( void )
{
	dlclose( cl_dll );

	gRealFunctionTable.pfnShutdown();
}

MENUAPI realGetMenuAPI = 0;
UITEXTAPI realGiveTextAPI = 0;
ADDTOUCHBUTTONTOLIST realAddTouchButtonToList = 0;

EXPORT int GetMenuAPI( UI_FUNCTIONS *pFunctionTable, ui_enginefuncs_t* pEngfuncsFromEngine, ui_globalvars_t *pGlobals )
{
	char filename[1024];

#ifdef __ANDROID__
	#ifdef LOAD_HARDFP
		snprintf( filename, 1024, "%s/libclient_hardfp.so", getenv( "XASH3D_GAMELIBDIR" ) );
	#else
		snprintf( filename, 1024, "%s/libclient.so", getenv( "XASH3D_GAMELIBDIR" ) );
	#endif
#else
	char gamedir[64];
	pEngfuncsFromEngine->pfnGetGameDir( gamedir );
	snprintf( filename, 1024, "%s/cl_dlls/client.so", gamedir );
#endif

	cl_dll = dlopen( filename, RTLD_NOW );

	if( !cl_dll )
	{
		pEngfuncsFromEngine->pfnHostError( "Failed to load %s from mainui_stub: %s\n", filename, dlerror() );
	}

	realGetMenuAPI = (MENUAPI)dlsym( cl_dll, "GetMenuAPI" );
	realGiveTextAPI = (UITEXTAPI)dlsym( cl_dll, "GiveTextAPI" );
	realAddTouchButtonToList = (ADDTOUCHBUTTONTOLIST)dlsym( cl_dll, "AddTouchButtonToList" );

	int ret = realGetMenuAPI( &gRealFunctionTable, pEngfuncsFromEngine, pGlobals );

	memcpy( &gFakeFunctionTable, &gRealFunctionTable, sizeof( UI_FUNCTIONS ));
	gFakeFunctionTable.pfnShutdown = UI_Shutdown;

	memcpy( pFunctionTable, &gFakeFunctionTable, sizeof( UI_FUNCTIONS ));

	return ret;
}
EXPORT int GiveTextAPI( ui_textfuncs_t* pTextfuncsFromEngine )
{
	return realGiveTextAPI( pTextfuncsFromEngine );
}
EXPORT void AddTouchButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags )
{
	realAddTouchButtonToList( name, texture, command, color, flags );
}

#endif // MAINUI_STUB

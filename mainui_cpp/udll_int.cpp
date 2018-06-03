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


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"

ui_enginefuncs_t EngFuncs::engfuncs;
#ifndef XASH_DISABLE_FWGS_EXTENSIONS
ui_textfuncs_t	EngFuncs::textfuncs;
#endif
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
#ifndef XASH_DISABLE_FWGS_EXTENSIONS
	memset( &EngFuncs::textfuncs, 0, sizeof( ui_textfuncs_t ));
#endif
	gpGlobals = pGlobals;

	return TRUE;
}

#ifndef XASH_DISABLE_FWGS_EXTENSIONS
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
#endif

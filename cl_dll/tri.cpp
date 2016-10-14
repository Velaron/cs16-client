//========= Copyright ? 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any
#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "rain.h"

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	gHUD.m_Spectator.DrawOverview();
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	ProcessFXObjects();
	ProcessRain();
	DrawRain();
	DrawFXObjects();
}

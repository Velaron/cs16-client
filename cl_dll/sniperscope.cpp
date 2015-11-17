/*
hud_overlays.cpp - HUD Overlays
Copyright (C) 2015 a1batross
*/

#include "hud.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "cl_util.h"


int CHudSniperScope::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	for( int i = 0; i < 3; i++ ) m_iScopeArc[i] = NULL;
}

int CHudSniperScope::Draw(float flTime)
{
	return 1;
}

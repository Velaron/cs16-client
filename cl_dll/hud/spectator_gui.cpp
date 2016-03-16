/*
spectator_gui.cpp - HUD Overlays
Copyright (C) 2015 a1batross

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.
*/

#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#define DIVIDER_XPOS (ScreenWidth * 0.85)
#define DIVIDER_GAP (15)

#define BAR_HEIGHT (ScreenHeight / 7)

DECLARE_MESSAGE( m_SpectatorGui, SpecHealth );
DECLARE_MESSAGE( m_SpectatorGui, SpecHealth2 );

int CHudSpectatorGui::Init()
{
	HOOK_MESSAGE( SpecHealth );
	HOOK_MESSAGE( SpecHealth2 );

	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;

	return 1;
}

int CHudSpectatorGui::VidInit()
{
	if( g_iXash )
	{
		m_hTimerTexture = gRenderAPI.GL_LoadTexture("gfx/vgui/timer.tga", NULL, 0, TF_NEAREST |TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP );
	}
	else
	{
		m_hTimerTexture = 0;
	}

	return 1;
}

int CHudSpectatorGui::Draw( float flTime )
{
	if( !g_iUser1 )
		return 1;

	// check for scoreboard. We will don't draw it, because screen space econodmy
	if( gHUD.m_Scoreboard.m_bForceDraw || !(!gHUD.m_Scoreboard.m_bShowscoresHeld && gHUD.m_Health.m_iHealth > 0 && !gHUD.m_iIntermission ))
		return 1;

	int r, g, b;
	r = 255; g = 140; b = 0;

	// at first, draw these silly black bars
	DrawUtils::DrawRectangle(0, 0, ScreenWidth, BAR_HEIGHT, 0, 0, 0, 153, false);
	DrawUtils::DrawRectangle(0, BAR_HEIGHT * 6, ScreenWidth, BAR_HEIGHT, 0, 0, 0, 153, false);

	// divider
	FillRGBABlend( ScreenWidth * 0.85, BAR_HEIGHT * 0.25, 1, BAR_HEIGHT * 0.5, r, g, b, 255 );

	// function name says it
	CalcAllNeededData( );


	// draw counter-terrorist and terrorist remaining and time3
	DrawUtils::DrawHudString( DIVIDER_XPOS + DIVIDER_GAP, BAR_HEIGHT * 0.25, ScreenWidth, label.m_szMap, r, g, b );

	if( m_hTimerTexture )
	{
		gRenderAPI.GL_Bind( 0, m_hTimerTexture );
		DrawUtils::Draw2DQuad( TrueWidth * 0.85 + DIVIDER_GAP, TrueHeight / 14,
							   TrueWidth * 0.85 + DIVIDER_GAP + gHUD.GetCharHeight(), TrueHeight / 14 + gHUD.GetCharHeight() );
	}

	DrawUtils::DrawHudString( DIVIDER_XPOS + DIVIDER_GAP * 2 + gHUD.GetCharWidth('M'), BAR_HEIGHT * 0.5, ScreenWidth, label.m_szTimer, r, g, b);

	// CTs, Ts labels
	DrawUtils::DrawHudString( ScreenWidth * 0.7, BAR_HEIGHT * 0.25, ScreenWidth * 0.8, "Counter-Terrorists:", r, g, b );
	DrawUtils::DrawHudString( ScreenWidth * 0.7, BAR_HEIGHT * 0.5, ScreenWidth * 0.8, "Terrorists:", r, g, b );

	// CTs, Ts count
	DrawUtils::DrawHudNumberString( DIVIDER_XPOS - DIVIDER_GAP, BAR_HEIGHT * 0.25, ScreenWidth * 0.8, label.m_iCounterTerrorists, r, g, b );
	DrawUtils::DrawHudNumberString( DIVIDER_XPOS - DIVIDER_GAP, BAR_HEIGHT * 0.5, ScreenWidth * 0.8, label.m_iTerrorists, r, g, b );

	if( g_iUser2 > 0 && g_iUser2 < MAX_PLAYERS )
	{
		cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( g_iUser2 );
		if( pEnt && pEnt->player )
		{
			hud_player_info_t *pInfo = g_PlayerInfoList + pEnt->index;
			GetPlayerInfo( pEnt->index, pInfo );
			if( pInfo->name )
			{
				char szNameAndHealth[64];
				snprintf( szNameAndHealth, 64, "%s (%i)", pInfo->name, g_PlayerExtraInfo[pEnt->index].health );
				int iLen = DrawUtils::HudStringLen( szNameAndHealth );

				GetTeamColor( r, g, b, g_PlayerExtraInfo[ pEnt->index ].teamnumber );
				DrawUtils::DrawHudString( ScreenWidth * 0.5 - iLen * 0.5, BAR_HEIGHT * 13 / 2, ScreenWidth, szNameAndHealth, r, g, b );
			}
		}
	}
	return 1;
}

void CHudSpectatorGui::CalcAllNeededData( )
{
	// mapname
	static char szMapNameStripped[55];
	const char *szMapName = gEngfuncs.pfnGetLevelName(); //  "maps/%s.bsp"
	strncpy( szMapNameStripped, szMapName + 5, sizeof( szMapNameStripped ) );
	szMapNameStripped[strlen(szMapNameStripped) - 4] = '\0';
	snprintf( label.m_szMap, sizeof( label.m_szMap ), "Map: %s", szMapNameStripped );

	// team
	label.m_iTerrorists        = 0;
	label.m_iCounterTerrorists = 0;
	for( int i = 0; i < MAX_TEAMS; i++ )
	{
		if( !stricmp( g_TeamInfo[i].name, "CT") )
			label.m_iCounterTerrorists = g_TeamInfo[i].players;
		else if( !stricmp( g_TeamInfo[i].name, "TERRORIST") )
			label.m_iTerrorists = g_TeamInfo[i].players;

		if( label.m_iTerrorists && label.m_iCounterTerrorists )
			break;
	}

	// timer
	// time must be positive
	if( m_bBombPlanted )
	{
		label.m_szTimer[0] = '\0';
	}
	else
	{
		int iMinutes = max( 0, (int)( gHUD.m_Timer.m_iTime + gHUD.m_Timer.m_fStartTime - gHUD.m_flTime ) / 60);
		int iSeconds = max( 0, (int)( gHUD.m_Timer.m_iTime + gHUD.m_Timer.m_fStartTime - gHUD.m_flTime ) - (iMinutes * 60));

		sprintf( label.m_szTimer, "%i:%i", iMinutes, iSeconds );
	}
}

void CHudSpectatorGui::InitHUDData()
{
	m_bBombPlanted = false;
}

void CHudSpectatorGui::Think()
{

}

void CHudSpectatorGui::Reset()
{
	m_bBombPlanted = false;
}

int CHudSpectatorGui::MsgFunc_SpecHealth(const char *pszName, int iSize, void *buf)
{
	BEGIN_READ( buf, iSize );

	int health = READ_BYTE();

	g_PlayerExtraInfo[g_iUser2].health = health;
	m_iPlayerLastPointedAt = g_iUser2;

	return 1;
}

int CHudSpectatorGui::MsgFunc_SpecHealth2(const char *pszName, int iSize, void *buf)
{
	BEGIN_READ( buf, iSize );

	int health = READ_BYTE();
	int client = READ_BYTE();

	g_PlayerExtraInfo[client].health = health;
	m_iPlayerLastPointedAt = g_iUser2;

	return 1;
}

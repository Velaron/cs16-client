/*
radar.cpp - Radar
Copyright (C) 2016 a1batross

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

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

DECLARE_COMMAND( m_Radar, ShowRadar )
DECLARE_COMMAND( m_Radar, HideRadar )

DECLARE_MESSAGE( m_Radar, Radar )

int CHudRadar::Init()
{
	HOOK_MESSAGE( Radar );
	HOOK_COMMAND( "drawradar", ShowRadar );
	HOOK_COMMAND( "hideradar", HideRadar );

	m_iFlags = HUD_ACTIVE;

	cl_radartype = CVAR_CREATE( "cl_radartype", "0", FCVAR_ARCHIVE );

	gHUD.AddHudElem( this );
	return 1;
}

void CHudRadar::Reset()
{
	// make radar don't draw old players after new map
	for( int i = 0; i < 34; i++ )
	{
		g_PlayerExtraInfo[i].radarflashon = false;

		if( i <= MAX_HOSTAGES ) g_HostageInfo[i].radarflashon = false;
	}
}

int CHudRadar::VidInit(void)
{
	m_hRadar.SetSpriteByName( "radar" );
	m_hRadarOpaque.SetSpriteByName( "radaropaque" );
	return 1;
}

void CHudRadar::UserCmd_HideRadar()
{
	m_iFlags = 0;
}

void CHudRadar::UserCmd_ShowRadar()
{
	m_iFlags = HUD_ACTIVE;
}

int CHudRadar::MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int index = READ_BYTE();
	g_PlayerExtraInfo[index].origin.x = READ_COORD();
	g_PlayerExtraInfo[index].origin.y = READ_COORD();
	g_PlayerExtraInfo[index].origin.z = READ_COORD();
	return 1;
}

int CHudRadar::Draw(float flTime)
{
	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) ||
		 gEngfuncs.IsSpectateOnly() ||
		 !(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT))) ||
		 gHUD.m_fPlayerDead )
		return 1;

	int iTeamNumber = g_PlayerExtraInfo[ gHUD.m_Scoreboard.m_iPlayerNum ].teamnumber;
	int r, g, b;

	if( cl_radartype->value )
	{
		SPR_Set(m_hRadarOpaque.spr, 200, 200, 200);
		SPR_DrawHoles(0, 0, 0, &m_hRadarOpaque.rect);
	}
	else
	{
		SPR_Set( m_hRadar.spr, 25, 75, 25 );
		SPR_DrawAdditive( 0, 0, 0, &m_hRadarOpaque.rect );
	}

	for(int i = 0; i < 33; i++)
	{
		// skip local player and dead players
		if( i == gHUD.m_Scoreboard.m_iPlayerNum || g_PlayerExtraInfo[i].dead)
			continue;

		// skip non-teammates
		if( g_PlayerExtraInfo[i].teamnumber != iTeamNumber )
			continue;

		// player with C4 must be red
		if( g_PlayerExtraInfo[i].has_c4 )
		{
			DrawUtils::UnpackRGB( r, g, b, RGB_REDISH );
		}
		else
		{
			// white
			DrawUtils::UnpackRGB( r, g, b, RGB_WHITE );
		}

		// calc radar position
		Vector pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[i].origin, gHUD.m_vecAngles);


		if( !g_PlayerExtraInfo[i].radarflashon )
		{
			if( pos.z < 20 && pos.z > -20 )
			{
				DrawRadarDot( pos.x, pos.y, 4, r, g, b, 255 );
			}
			else if( gHUD.m_vecOrigin.z > g_PlayerExtraInfo[i].origin.z )
			{
				DrawFlippedT( pos.x, pos.y, 2, r, g, b, 255);
			}
			else
			{
				DrawT( pos.x, pos.y, 2, r, g, b, 255 );
			}
		}
		else
		{
			// radar flashing
			if( g_PlayerExtraInfo[i].radarflashes )
			{
				float timer = (flTime - g_PlayerExtraInfo[i].radarflash);
				if( timer > 0.5f )
				{
					g_PlayerExtraInfo[i].nextflash = !g_PlayerExtraInfo[i].nextflash;
					g_PlayerExtraInfo[i].radarflash = flTime;
					g_PlayerExtraInfo[i].radarflashes--;
				}
			}
			else g_PlayerExtraInfo[i].radarflashon = 0;

			if( g_PlayerExtraInfo[i].nextflash )
			{
				if( pos.z < 20 && pos.z > -20 )
				{
					DrawRadarDot( pos.x, pos.y, 4, r, g, b, 255 );
				}
				else if( gHUD.m_vecOrigin.z > g_PlayerExtraInfo[i].origin.z )
				{
					DrawFlippedT( pos.x, pos.y, 2, r, g, b, 255);
				}
				else
				{
					DrawT( pos.x, pos.y, 2, r, g, b, 255 );
				}
			}
		}
	}

	// Terrorist specific code( C4 Bomb )
	if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST && g_PlayerExtraInfo[33].radarflashon)
	{
		Vector pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[33].origin, gHUD.m_vecAngles);
		if( g_PlayerExtraInfo[33].radarflashes )
		{
			float timer = (flTime - g_PlayerExtraInfo[33].radarflash);
			if( timer > 0.5f )
			{
				g_PlayerExtraInfo[33].nextflash = !g_PlayerExtraInfo[33].nextflash;
				g_PlayerExtraInfo[33].radarflash = flTime;
				g_PlayerExtraInfo[33].radarflashes--;
			}
		}
		else g_PlayerExtraInfo[33].radarflashon = 0;
		if( g_PlayerExtraInfo[33].nextflash )
		{
			if( g_PlayerExtraInfo[33].playerclass )
			{
				DrawCross( pos.x, pos.y, 2, 255, 0, 0, 255);
			}
			else
			{
				DrawRadarDot( pos.x, pos.y, 4, 255, 0, 0, 255 );
			}
		}
	}

	// Counter-Terrorist specific code( hostages )
	if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_CT )
	{
		// draw hostages for CT
		for( int i = 0; i < MAX_HOSTAGES; i++ )
		{
			if( !g_HostageInfo[i].radarflashon || g_HostageInfo[i].dead )
				continue;

			Vector pos = WorldToRadar(gHUD.m_vecOrigin, g_HostageInfo[i].origin, gHUD.m_vecAngles);
			if( g_HostageInfo[i].radarflashes )
			{
				float timer = (flTime - g_HostageInfo[i].radarflash);
				if( timer > 0.5f )
				{
					g_HostageInfo[i].nextflash = !g_HostageInfo[i].nextflash;
					g_HostageInfo[i].radarflash = flTime;
					g_HostageInfo[i].radarflashes--;
				}
			}
			else g_HostageInfo[i].radarflashon = 0;
			if( g_HostageInfo[i].nextflash )
			{
				DrawRadarDot( pos.x, pos.y, 4, 255, 0, 0, 255 );
			}
		}
	}

	return 0;
}

void CHudRadar::DrawPlayerLocation()
{
	DrawUtils::DrawConsoleString( 30, 30, g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].location );
}

void CHudRadar::DrawRadarDot(int x, int y, int size, int r, int g, int b, int a)
{
	FillRGBA(62.5f + x - size/2.0f, 62.5f + y - size/2.0f, size, size, r, g, b, a);
}

void CHudRadar::DrawCross(int x, int y, int size, int r, int g, int b, int a)
{
	FillRGBA(62.5f + x, 62.5f + y, size, size, r, g, b, a);
	FillRGBA(62.5f + x - size, 62.5f + y - size, size, size, r, g, b, a);
	FillRGBA(62.5f + x - size, 62.5f + y + size, size, size, r, g, b, a);
	FillRGBA(62.5f + x + size, 62.5f + y - size, size, size, r, g, b, a);
	FillRGBA(62.5f + x + size, 62.5f + y + size, size, size, r, g, b, a);

}

void CHudRadar::DrawT(int x, int y, int size, int r, int g, int b, int a)
{
	FillRGBA( 62.5f + x - size, 62.5 + y - size, 3*size, size, r, g, b, a);
	FillRGBA( 62.5f + x, 62.5 + y, size, 2*size, r, g, b, a);
}

void CHudRadar::DrawFlippedT(int x, int y, int size, int r, int g, int b, int a)
{
	FillRGBA( 62.5f + x, 62.5 + y - size, size, 2*size, r, g, b, a);
	FillRGBA( 62.5f + x - size, 62.5 + y + size, 3*size, size, r, g, b, a);
}

Vector CHudRadar::WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles  )
{
	float xdiff = vObjectOrigin.x - vPlayerOrigin.x;
	float ydiff = vObjectOrigin.y - vPlayerOrigin.y;

	// Supply epsilon values to avoid divide-by-zero
	if(xdiff == 0)
		xdiff = 0.00001f;
	if(ydiff == 0)
		ydiff = 0.00001f;

	int iMaxRadius = (m_hRadar.rect.right - m_hRadar.rect.left) / 2.0f;

	float flOffset = atan(ydiff / xdiff) * 180.0f / M_PI;

	if ((xdiff < 0) && (ydiff >= 0))
		flOffset += 180;
	else if ((xdiff < 0) && (ydiff < 0))
		flOffset += 180;
	else if ((xdiff >= 0) && (ydiff < 0))
		flOffset += 360;

	// this magic 32.0f just scales position on radar
	float iRadius = -( sqrt( xdiff*xdiff + ydiff*ydiff ) ) / 32.0f;
	if( -iRadius > iMaxRadius)
		iRadius = -iMaxRadius;

	flOffset = (vAngles.y - flOffset) * M_PI / 180.0f;

	// transform origin difference to radar source
	return { -iRadius * sin(flOffset),
				   iRadius * cos(flOffset),
				   vPlayerOrigin.z - vObjectOrigin.z };
}

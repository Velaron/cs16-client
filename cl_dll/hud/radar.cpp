/*
radar.cpp - Radar
Copyright (C) 2016 a1batross
*/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_COMMAND( m_Radar, ShowRadar )
DECLARE_COMMAND( m_Radar, HideRadar )

DECLARE_MESSAGE( m_Radar, Radar )

int CHudRadar::Init()
{
	HOOK_MESSAGE( Radar );
	HOOK_COMMAND( "drawradar", ShowRadar );
	HOOK_COMMAND( "hideradar", HideRadar );

	m_iFlags = HUD_ACTIVE;

	m_hRadar = 0;
	m_hRadaropaque = 0;

	m_hrad.bottom = m_hrad.left = m_hrad.right = m_hrad.top = 0;
	m_hradopaque.bottom = m_hradopaque.left = m_hradopaque.right = m_hradopaque.top = 0;

	cl_radartype = CVAR_CREATE( "cl_radartype", "0", FCVAR_ARCHIVE );

	gHUD.AddHudElem( this );
	return 1;
}

void CHudRadar::Reset()
{
	// make radar don't draw old players after new map
	for( int i = 0; i < 34; i++ )
	{
		g_PlayerExtraInfo[i].dead = 1;

		if( i <= MAX_HOSTAGES ) g_HostageInfo[i].dead = 1;
	}
}

int CHudRadar::VidInit(void)
{
	m_hRadar = gHUD.GetSprite( gHUD.GetSpriteIndex( "radar" ));
	m_hRadaropaque = gHUD.GetSprite( gHUD.GetSpriteIndex( "radaropaque" ));
	m_hrad = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "radar" ) );
	m_hradopaque = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "radaropaque" ) );
	//m_hHostage = gHUD.GetSprite( gHUD.GetSpriteIndex("hostage") );

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
		SPR_Set(m_hRadaropaque, 200, 200, 200);
		SPR_DrawHoles(0, 0, 0, &m_hradopaque);
	}
	else
	{
		SPR_Set( m_hRadar, 25, 75, 25 );
		SPR_DrawAdditive( 0, 0, 0, &m_hrad );
	}

	for(int i = 0; i < 33; i++)
	{
		if( i == gHUD.m_Scoreboard.m_iPlayerNum || g_PlayerExtraInfo[i].dead)
			continue;

		if( g_PlayerExtraInfo[i].teamnumber != iTeamNumber )
			continue;

		if( g_PlayerExtraInfo[i].has_c4 )
		{
			UnpackRGB( r, g, b, RGB_REDISH );
		}
		else
		{
			// white
			UnpackRGB( r, g, b, 0x00FFFFFF );
		}
		Vector2D pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[i].origin, gHUD.m_vecAngles);
		float zdiff = gHUD.m_vecOrigin.z - g_PlayerExtraInfo[i].origin.z;
		if( !g_PlayerExtraInfo[i].radarflashon )
		{
			if( zdiff < 20 && zdiff > -20 )
				DrawRadarDot( pos.x, pos.y, 4, r, g, b, 255 );
			else if( gHUD.m_vecOrigin.z > g_PlayerExtraInfo[i].origin.z )
				DrawFlippedT( pos.x, pos.y, 2, r, g, b, 255);
			else DrawT( pos.x, pos.y, 2, r, g, b, 255 );
		}
		else
		{
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
				if( zdiff < 20 && zdiff > -20 )
					DrawRadarDot( pos.x, pos.y, 4, r, g, b, 255 );
				else if( gHUD.m_vecOrigin.z > g_PlayerExtraInfo[i].origin.z )
					DrawFlippedT( pos.x, pos.y, 2, r, g, b, 255);
				else DrawT( pos.x, pos.y, 2, r, g, b, 255 );
			}
		}
	}

	if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST && g_PlayerExtraInfo[33].radarflashon)
	{
		Vector2D pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[33].origin, gHUD.m_vecAngles);
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
				DrawCross( pos.x, pos.y, 2, 255, 0, 0, 255);
			else DrawRadarDot( pos.x, pos.y, 4, 255, 0, 0, 255 );
		}
	}

	if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_CT )
	{
		// draw hostages for CT
		for( int i = 0; i < MAX_HOSTAGES; i++ )
		{
			if( !g_HostageInfo[i].radarflashon || g_HostageInfo[i].dead )
				continue;

			Vector2D pos = WorldToRadar(gHUD.m_vecOrigin, g_HostageInfo[i].origin, gHUD.m_vecAngles);
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
}

void CHudRadar::DrawPlayerLocation()
{
	DrawConsoleString( 30, 30, g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].location );
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

Vector2D CHudRadar::WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles  )
{
	Vector2D diff = vObjectOrigin.Make2D() - vPlayerOrigin.Make2D();

	// Supply epsilon values to avoid divide-by-zero
	if(diff.x == 0)
		diff.x = 0.00001f;
	if(diff.y == 0)
		diff.y = 0.00001f;

	int iMaxRadius = (m_hrad.right - m_hrad.left) / 2.0f;

	float flOffset = atan(diff.y / diff.x) * 180.0f / M_PI;

	if ((diff.x < 0) && (diff.y >= 0))
		flOffset += 180;
	else if ((diff.x < 0) && (diff.y < 0))
		flOffset += 180;
	else if ((diff.x >= 0) && (diff.y < 0))
		flOffset += 360;

	// this magic 32.0f just scales position on radar
	float iRadius = -diff.Length() / 32.0f;
	if( -iRadius > iMaxRadius)
		iRadius = -iMaxRadius;

	flOffset = (vAngles.y - flOffset) * M_PI / 180.0f;

	// transform origin difference to radar source
	Vector2D new_diff;
	new_diff.x = -iRadius * sin(flOffset);
	new_diff.y =  iRadius * cos(flOffset);

	return new_diff;
}

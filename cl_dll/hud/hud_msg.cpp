/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "rain.h"
#include "com_model.h"
#include "studio.h"
#include "studio_util.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "com_weapons.h"

#include <cstring>

#include "events.h"

#define MAX_CLIENTS 32

extern float g_flRoundTime;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

int CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
	return 1;
}

int CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	g_iFreezeTimeOver = 0;

	memset( g_PlayerExtraInfo, 0, sizeof(g_PlayerExtraInfo) );

	ResetRain();

	// reset round time
	g_flRoundTime   = 0.0f;

	// reinitialize models. We assume that server already precached all models.
	g_iRShell       = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/rshell.mdl" );
	g_iPShell       = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/pshell.mdl" );
	g_iShotgunShell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" );
	g_iBlackSmoke   = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/black_smoke4.spr" );

	return 1;
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pbuf, iSize );
	m_Teamplay = reader.ReadByte();

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pbuf, iSize );
	m_iConcussionEffect = reader.ReadByte();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud::MsgFunc_BombDrop(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader(pbuf, iSize);

	g_PlayerExtraInfo[33].origin.x = reader.ReadCoord();
	g_PlayerExtraInfo[33].origin.y = reader.ReadCoord();
	g_PlayerExtraInfo[33].origin.z = reader.ReadCoord();

	g_PlayerExtraInfo[33].radarflashon = 1;
	g_PlayerExtraInfo[33].radarflashes = 99999;
	g_PlayerExtraInfo[33].radarflash = gHUD.m_flTime;
	strncpy(g_PlayerExtraInfo[33].teamname, "TERRORIST", MAX_TEAM_NAME);
	g_PlayerExtraInfo[33].dead = 0;
	g_PlayerExtraInfo[33].nextflash = true;

	int Flag = reader.ReadByte();
	g_PlayerExtraInfo[33].playerclass = Flag;

	if( Flag ) // bomb planted
	{
		m_SpectatorGui.m_bBombPlanted = 0;
		m_Timer.m_iFlags = 0;
	}
	return 1;
}

int CHud::MsgFunc_BombPickup(const char *pszName, int iSize, void *pbuf)
{
	g_PlayerExtraInfo[33].radarflashon = 0;
	g_PlayerExtraInfo[33].radarflash = 0.0f;
	g_PlayerExtraInfo[33].radarflashes = 0;
	g_PlayerExtraInfo[33].dead = 1;

	return 1;
}

int CHud::MsgFunc_HostagePos(const char *pszName, int iSize, void *pbuf)
{

	BufferReader reader(pbuf, iSize);
	int Flag = reader.ReadByte();
	int idx = reader.ReadByte();
	if ( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].origin.x = reader.ReadCoord();
		g_HostageInfo[idx].origin.y = reader.ReadCoord();
		g_HostageInfo[idx].origin.z = reader.ReadCoord();
		if ( Flag == 1 )
		{
			g_HostageInfo[idx].radarflash = gHUD.m_flTime;
			g_HostageInfo[idx].radarflashon = 1;
			g_HostageInfo[idx].radarflashes = 99999;
		}
		strncpy(g_HostageInfo[idx].teamname, "CT", MAX_TEAM_NAME);
		g_HostageInfo[idx].dead = 0;
	}

	return 1;
}

int CHud::MsgFunc_HostageK(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader(pbuf, iSize);
	int idx = reader.ReadByte();
	if ( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].dead = 1;
		g_HostageInfo[idx].radarflash = gHUD.m_flTime;
		g_HostageInfo[idx].radarflashes = 15;
	}

	return 1;
}

int CHud::MsgFunc_ShadowIdx(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader(pbuf, iSize);

	int idx = reader.ReadByte();
	g_StudioRenderer.StudioSetShadowSprite(idx);
	return 1;
}

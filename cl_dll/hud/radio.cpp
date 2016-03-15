/*
radio.cpp -- Radio HUD implementation
Copyright (C) 2015-2016 a1batross
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
#include "parsemsg.h"
#include "cl_util.h"
#include <string.h>

DECLARE_MESSAGE( m_Radio, SendAudio )
DECLARE_MESSAGE( m_Radio, ReloadSound )

int CHudRadio::Init( )
{
	HOOK_MESSAGE( SendAudio );
	HOOK_MESSAGE( ReloadSound );
	gHUD.AddHudElem(this);
	m_iFlags = 0;
	return 1;
}

int CHudRadio::Draw(float flTime)
{
	if( m_sentence[0] == '%' )
		PlaySound( &m_sentence[1], m_iPitch );
	else
		PlaySound( m_sentence, m_iPitch );

	m_iFlags = 0;

	return 1;
}

int CHudRadio::VidInit()
{
	return 1;
}

int CHudRadio::MsgFunc_SendAudio(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iSenderID = READ_BYTE( );
	strncpy( m_sentence, READ_STRING( ), sizeof(m_sentence));
	m_iPitch = READ_SHORT( );
	m_iFlags = HUD_ACTIVE;

	g_PlayerExtraInfo[m_iSenderID].radarflashes = 22;
	g_PlayerExtraInfo[m_iSenderID].radarflash = gHUD.m_flTime;
	g_PlayerExtraInfo[m_iSenderID].radarflashon = 1;
	return 1;
}

int CHudRadio::MsgFunc_ReloadSound(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iPitch = READ_BYTE();
	int isNotShotgun = READ_BYTE();

	strncpy( m_sentence,  (char*)(isNotShotgun? "weapons/generic_reload.wav" : "weapons/generic_shot_reload.wav"), sizeof(m_sentence));
	m_iFlags = HUD_ACTIVE;

   return 0;
}

void Broadcast( const char *sentence )
{
	if( sentence[0] == '%' )
		PlaySound( (char*)&sentence[1], 100.0 );
	else
		PlaySound( (char*)sentence, 100.0 );
}

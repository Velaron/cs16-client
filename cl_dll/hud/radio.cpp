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
#include "r_efx.h"
#include "event_api.h"
#include "com_model.h"
#include <string.h>

DECLARE_MESSAGE( m_Radio, SendAudio )
DECLARE_MESSAGE( m_Radio, ReloadSound )
DECLARE_MESSAGE( m_Radio, BotVoice )

int CHudRadio::Init( )
{
	HOOK_MESSAGE( SendAudio );
	HOOK_MESSAGE( ReloadSound );
	HOOK_MESSAGE( BotVoice );
	gHUD.AddHudElem( this );
	m_iFlags = 0;
	return 1;
}

int CHudRadio::MsgFunc_SendAudio( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	char sentence[64];
	int SenderID = READ_BYTE( );
	strncpy( sentence, READ_STRING( ), sizeof( sentence ) );
	int pitch = READ_SHORT( );

	if ( sentence[0] == '%' && sentence[1] == '!' )
		gEngfuncs.pfnPlaySoundByNameAtPitch( &sentence[1], 1.0f, pitch );
	else
		gEngfuncs.pfnPlaySoundVoiceByName( sentence, 1.0f, pitch );

	g_PlayerExtraInfo[SenderID].radarflashes = 22;
	g_PlayerExtraInfo[SenderID].radarflash   = gHUD.m_flTime;
	g_PlayerExtraInfo[SenderID].radarflashon = 1;
	return 1;
}

int CHudRadio::MsgFunc_ReloadSound( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int vol = READ_BYTE( );
	if ( READ_BYTE( ) )
	{
		gEngfuncs.pfnPlaySoundByName( "weapon/generic_reload.wav", vol / 255.0f );
	}
	else
	{
		gEngfuncs.pfnPlaySoundByName( "weapon/generic_shot_reload.wav", vol / 255.0f );
	}
	return 1;
}

void Broadcast( const char *msg )
{
	if ( msg[0] == '%' && msg[1] == '!' )
		PlaySound( &msg[1], 1.0f );
	else
		PlaySound( msg, 1.0f );
}

void VoiceIconCallback(struct tempent_s *ent, float frametime, float currenttime)
{
	int entIndex = ent->clientIndex;
	if( !g_PlayerExtraInfo[entIndex].talking )
		ent->die = 0.0f;
}

int CHudRadio::MsgFunc_BotVoice( const char *pszName, int iSize, void *buf )
{
	BEGIN_READ( buf, iSize );

	int enable   = READ_BYTE();
	int entIndex = READ_BYTE();


	if( entIndex < 0 || entIndex > 32 )
		return 0;

	if( !enable )
	{
		g_PlayerExtraInfo[entIndex].talking = false;
		ConsolePrint("Removing bot voice icon\n");
		return 1;
	}

	int spr = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/voiceicon.spr" );
	if( !spr )
		return 0;

	TEMPENTITY *temp = gEngfuncs.pEfxAPI->R_DefaultSprite( vec3_origin, spr, 0 );

	if( !temp )
		return 0;

	temp->flags = FTENT_SPRANIMATELOOP | FTENT_CLIENTCUSTOM | FTENT_PLYRATTACHMENT;
	temp->tentOffset.z = 40;
	temp->clientIndex = entIndex;
	temp->callback = VoiceIconCallback;
	temp->entity.curstate.scale = 0.60f;
	temp->entity.curstate.rendermode = kRenderTransAdd;
	temp->die = gHUD.m_flTime + 60.0f; // 60 seconds must be enough?
	g_PlayerExtraInfo[entIndex].talking = true;

	return 1;
}

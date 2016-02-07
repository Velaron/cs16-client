/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
#include "events.h"

enum usp_e
{
	USP_IDLE,
	USP_SHOOT1,
	USP_SHOOT2,
	USP_SHOOT3,
	USP_SHOOT_EMPTY,
	USP_RELOAD,
	USP_DRAW,
	USP_ATTACH_SILENCER,
	USP_UNSIL_IDLE,
	USP_UNSIL_SHOOT1,
	USP_UNSIL_SHOOT2,
	USP_UNSIL_SHOOT3,
	USP_UNSIL_SHOOT_EMPTY,
	USP_UNSIL_RELOAD,
	USP_UNSIL_DRAW,
	USP_DETACH_SILENCER
};

enum usp_shield_e
{
	USP_SHIELD_IDLE,
	USP_SHIELD_SHOOT1,
	USP_SHIELD_SHOOT2,
	USP_SHIELD_SHOOT_EMPTY,
	USP_SHIELD_RELOAD,
	USP_SHIELD_DRAW,
	USP_SHIELD_UP_IDLE,
	USP_SHIELD_UP,
	USP_SHIELD_DOWN
};

void EV_FireUSP( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;
	const char *szSoundName;

	idx = args->entindex;
	bool silencer_on = !args->bparam2;
	bool empty		 = !!args->bparam1;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		int seq;
		if( g_bHoldingShield )
		{
			if( !empty )
				seq = gEngfuncs.pfnRandomLong(USP_SHIELD_SHOOT1, USP_SHIELD_SHOOT2);
			else seq = USP_SHIELD_SHOOT_EMPTY;
		}
		else if ( silencer_on )
		{
			if( !empty )
				seq = gEngfuncs.pfnRandomLong(USP_UNSIL_SHOOT1, USP_UNSIL_SHOOT3);
			else seq = USP_UNSIL_SHOOT_EMPTY;
		}
		else
		{
			EV_MuzzleFlash();
			if( !empty )
				seq = gEngfuncs.pfnRandomLong(USP_SHOOT1, USP_SHOOT3);
			else seq = USP_SHOOT_EMPTY;
		}

		gEngfuncs.pEventAPI->EV_WeaponAnimation(seq, 2);
	}
#if defined(_CS16CLIENT_FIX_EVENT_ORIGIN)
	else
	{
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex(idx);
		origin = ent->origin;
	}
#endif


	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);

	if( !silencer_on )
	{
		szSoundName = gEngfuncs.pfnRandomLong( 0, 1 ) ? "weapons/usp1.wav" : "weapons/usp2.wav";
	}
	else
	{
		szSoundName = "weapons/usp_unsil-1.wav";
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON,
									   szSoundName,
									   1, ATTN_NORM, 0,
									   94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	Vector vSpread;
	int tracerCount;
	vSpread.x = args->fparam1;
	vSpread.y = args->fparam2;
	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, vecAiming,
		vSpread, 8192.0, BULLET_PLAYER_45ACP, 0, &tracerCount,
		2 );
}

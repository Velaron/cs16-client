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

enum famas_e
{
	FAMAS_IDLE = 0,
	FAMAS_RELOAD,
	FAMAS_DRAW,
	FAMAS_SHOOT1,
	FAMAS_SHOOT2,
	FAMAS_SHOOT3
};

void EV_FireFAMAS( event_args_t *args )
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

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(FAMAS_SHOOT1,FAMAS_SHOOT3), 2);
	}

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON,
		gEngfuncs.pfnRandomLong( 0, 1) ? "weapons/famas-1.wav" : "weapons/famas-2.wav",
									   1, ATTN_NORM, 0,
									   94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );


	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	EV_HLDM_FireBullets( idx,
						 forward,
						 right,
						 up,
						 1,
						 vecSrc,
						 vecAiming,
						 8192,
						 0,
						 0,
						 0,
						 args->fparam1,
						 args->fparam2 );
}

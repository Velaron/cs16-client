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

enum m249_e
{
	M249_IDLE1,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_RELOAD,
	M249_DRAW
};

void EV_FireM249(event_args_s *args)
{
	vec3_t origin, angles, velocity;

	vec3_t ShellVelocity, ShellOrigin;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;
	int shell, idx;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( args->entindex ) )
	{
		++g_iShotsFired;
		gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(M249_SHOOT1, M249_SHOOT2), 2);
		EV_MuzzleFlash();
	}

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON,
		gEngfuncs.pfnRandomLong(0, 1) ? "weapons/m249-1.wav" : "weapons/m249-2.wav",
		1, ATTN_NORM, 0,
		94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );


	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, vecAiming,
		8192, 0, 0, 0,
		args->fparam1, args->fparam2 );
}

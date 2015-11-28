#include "events.h"

enum m4a1_e
{
	M4A1_IDLE1 = 0,
	M4A1_SHOOT1,
	M4A1_SHOOT2,
	M4A1_SHOOT3,
	M4A1_RELOAD,
	M4A1_DRAW,
	M4A1_ADD_SILENCER,
	M4A1_IDLE_UNSIL,
	M4A1_SHOOT1_UNSIL,
	M4A1_SHOOT2_UNSIL,
	M4A1_SHOOT3_UNSIL,
	M4A1_RELOAD_UNSIL,
	M4A1_DRAW_UNSIL,
	M4A1_DETACH_SILENCER
};

void EV_FireM4A1( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell, sequence;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;
	const char *szSoundName;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		if( args->bparam1 )
		{
			sequence = gEngfuncs.pfnRandomLong( M4A1_SHOOT1, M4A1_SHOOT3 );
		}
		else
		{
			sequence = gEngfuncs.pfnRandomLong( M4A1_SHOOT1_UNSIL, M4A1_SHOOT3_UNSIL );
		}
		gEngfuncs.pEventAPI->EV_WeaponAnimation(sequence, 2);
	}

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);
	if( args->bparam1 )
	{
		szSoundName = "weapons/m4a1-1.wav";
	}
	else
	{
		szSoundName = gEngfuncs.pfnRandomLong( 0, 1) ? "weapons/m4a1_unsil-1.wav" : "weapons/m4a1_unsil-2.wav";
	}
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON,
									   szSoundName,
									   1, ATTN_NORM, 0,
									   94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );


	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	EV_HLDM_FireBullets( idx,
		forward, right, up,
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

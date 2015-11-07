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
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		if( args->bparam2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(USP_SHOOT1,USP_SHOOT3), 2);
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(USP_UNSIL_SHOOT1, USP_UNSIL_SHOOT3), 2);
	}

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);

	if( args->bparam2 )
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

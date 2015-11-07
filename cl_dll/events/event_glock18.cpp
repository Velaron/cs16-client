#include "events.h"

enum glock18_e
{
	GLOCK18_IDLE1 = 0,
	GLOCK18_IDLE2,
	GLOCK18_IDLE3,
	GLOCK18_SHOOT1,
	GLOCK18_SHOOT2,
	GLOCK18_SHOOT3,
	GLOCK18_SHOOT_EMPTY,
	GLOCK18_RELOAD,
	GLOCK18_DRAW,
	GLOCK18_HOLSTER,
	GLOCK18_ADD_SILENCER,
	GLOCK18_DRAW2,
	GLOCK18_RELOAD2
};

void EV_Fireglock18( event_args_t *args )
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
		EV_MuzzleFlash();
		if( args->bparam1 )
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(gEngfuncs.pfnRandomLong(GLOCK18_SHOOT1, GLOCK18_SHOOT3), 2);
		}
		else
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation(GLOCK18_SHOOT_EMPTY, 2);
		}
	}

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON,
										   "weapons/glock18-2.wav",
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

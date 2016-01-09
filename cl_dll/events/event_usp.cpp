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
	bool empty		 = args->bparam1;
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

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);

	if( silencer_on )
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

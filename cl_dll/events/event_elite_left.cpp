#include "events.h"

enum elite_e
{
	ELITE_IDLE,
	ELITE_IDLE_LEFTEMPTY,
	ELITE_SHOOTLEFT1,
	ELITE_SHOOTLEFT2,
	ELITE_SHOOTLEFT3,
	ELITE_SHOOTLEFT4,
	ELITE_SHOOTLEFT5,
	ELITE_SHOOTLEFTLAST,
	ELITE_SHOOTRIGHT1,
	ELITE_SHOOTRIGHT2,
	ELITE_SHOOTRIGHT3,
	ELITE_SHOOTRIGHT4,
	ELITE_SHOOTRIGHT5,
	ELITE_SHOOTRIGHTLAST,
	ELITE_RELOAD,
	ELITE_DRAW
};

void EV_FireElite( event_args_s *args, int sequence )
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
		gEngfuncs.pEventAPI->EV_WeaponAnimation(sequence, 2);
	}

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 12, -10, -7 );
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL);
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON,
		"weapons/elite_fire.wav",
		1, ATTN_NORM, 0,
		94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );



	// don't draw Bullets and decals of elite
	// TODO: fix it

	/*EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( Vector(forward, right, up), vecAiming );
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
						 args->fparam2 );*/
}

void EV_FireEliteLeft(event_args_s *args)
{
	EV_FireElite( args, gEngfuncs.pfnRandomLong( ELITE_SHOOTLEFT1, ELITE_SHOOTLEFT4 ));
}

void EV_FireEliteRight( event_args_s *args )
{
	EV_FireElite( args, gEngfuncs.pfnRandomLong( ELITE_SHOOTRIGHT1, ELITE_SHOOTRIGHT4 ));
}

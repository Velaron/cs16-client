/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
// shared event functions
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "r_efx.h"

#include "eventscripts.h"
#include "event_api.h"
#include "pm_shared.h"

/*
=================
EV_GetGunPosition

Figure out the height of the gun
=================
*/
void EV_GetGunPosition( event_args_t *args, float *pos, float *origin )
{
	int idx;
	Vector view_ofs(0, 0, 0);

	idx = args->entindex;

	view_ofs[2] = DEFAULT_VIEWHEIGHT;

	if ( EV_IsPlayer( idx ) )
	{
		// in spec mode use entity viewheigh, not own
		if ( EV_IsLocal( idx ) && !IS_FIRSTPERSON_SPEC )
		{
			// Grab predicted result for local player
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if ( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	VectorAdd( origin, view_ofs, pos );
}

/*
=================
EV_GetDefaultShellInfo

Determine where to eject shells from
=================
*/
void EV_GetDefaultShellInfo( event_args_t *args, float *origin, float *velocity, float *ShellVelocity, float *ShellOrigin, float *forward, float *right, float *up, float forwardScale, float upScale, float rightScale, bool bReverseDirection )
{
	int idx = args->entindex;

	vec3_t view_ofs = { 0, 0, DEFAULT_VIEWHEIGHT };
	if ( EV_IsPlayer( idx ) )
	{
		if ( EV_IsLocal( idx ) )
		{
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if ( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	float fR = gEngfuncs.pfnRandomFloat( 50, 70 );
	float fU = gEngfuncs.pfnRandomFloat( 75, 175 );
	float fF = gEngfuncs.pfnRandomFloat( 25, 250 );
	float fDirection = rightScale > 0.0f ? -1.0f : 1.0f;

	for ( int i = 0; i < 3; i++ )
	{
		if( bReverseDirection )
		{
			ShellVelocity[i] = velocity[i] * 0.5f - right[i] * fR * fDirection + up[i] * fU + forward[i] * fF;
		}
		else
		{
			ShellVelocity[i] = velocity[i] * 0.5f + right[i] * fR * fDirection + up[i] * fU + forward[i] * fF;
		}
		ShellOrigin[i]   = velocity[i] * 0.1f + origin[i] + view_ofs[i] +
				upScale * up[i] + forwardScale * forward[i] + rightScale * right[i];
	}
}

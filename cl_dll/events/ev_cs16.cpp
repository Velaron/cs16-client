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
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "triangleapi.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

#include <assert.h>

extern float g_flRoundTime;

extern "C" char PM_FindTextureType( char *name );

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
void EV_HLDM_NewExplode( float x, float y, float z, float ScaleExplode1 )
{

	int  iNewExplode = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/dexplo.spr");
	TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z + 5 ),
														 Vector( 0, 0, 0 ),
														 ScaleExplode1, iNewExplode, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT | FTENT_COLLIDEKILL );

	if(pTemp)
	{
		pTemp->fadeSpeed = 90.0;
		pTemp->entity.curstate.framerate = 37.0;
		pTemp->entity.curstate.renderamt = 155;
		pTemp->entity.curstate.rendercolor.r = 255;
		pTemp->entity.curstate.rendercolor.g = 255;
		pTemp->entity.curstate.rendercolor.b = 255;
	}

	iNewExplode = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/fexplo.spr");
	pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z + 10),
											 Vector( 0, 0, 0 ),
											 ScaleExplode1, iNewExplode, kRenderTransAdd, kRenderFxNone, 1.0, 0.5, FTENT_SPRANIMATE | FTENT_FADEOUT | FTENT_COLLIDEKILL );

	if(pTemp)
	{
		pTemp->fadeSpeed = 90.0;
		pTemp->entity.curstate.framerate = 35.0;
		pTemp->entity.curstate.renderamt = 150;
		pTemp->entity.curstate.rendercolor.r = 255;
		pTemp->entity.curstate.rendercolor.g = 255;
		pTemp->entity.curstate.rendercolor.b = 255;
		pTemp->entity.angles = Vector( 90, 0, 0 );
	}

	for( int i = 1; i <= 10; i++ )
	{
		int  iSmokeSprite = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/smoke.spr");
		TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( Vector( x, y, z ),
															 Vector( (int)gEngfuncs.pfnRandomLong( -100, 100 ), (int)gEngfuncs.pfnRandomLong( -100, 100 ), (int)gEngfuncs.pfnRandomLong( -100, 100 ) ),
															 5, iSmokeSprite, kRenderTransAlpha, kRenderFxNone, 1.0, 0.5, FTENT_FADEOUT | FTENT_PERSIST );

		if(pTemp)
		{
			pTemp->fadeSpeed = 0.6;
			pTemp->entity.curstate.framerate = 1;
			pTemp->entity.curstate.renderamt = 255;
			int Color = gEngfuncs.pfnRandomLong( 0, 140 );
			pTemp->entity.curstate.rendercolor.r = Color;
			pTemp->entity.curstate.rendercolor.g = Color;
			pTemp->entity.curstate.rendercolor.b = Color;
		}
	}

}

char EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	const char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player
	if ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if ( entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );

		if ( pTextureName )
		{
			strncpy( texname, pTextureName, sizeof( texname ) );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}

			// '}}'
			strncpy( szbuffer, pTextureName, sizeof(szbuffer) );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;

			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );
		}
	}

	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_METAL:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_DIRT:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	}
	case CHAR_TEX_VENT:
	{
		fvol = 0.5;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_GRATE:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	}
	case CHAR_TEX_TILE:
	{
		fvol = 0.8;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	}
	case CHAR_TEX_SLOSH:
	{
		fvol = 0.9;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	}
	case CHAR_TEX_SNOW:
	{
		fvol = 0.7;
		rgsz[0] = "debris/pl_snow1.wav";
		rgsz[1] = "debris/pl_snow2.wav";
		rgsz[2] = "debris/pl_snow3.wav";
		rgsz[3] = "debris/pl_snow4.wav";
		cnt = 4;
		break;
	}
	case CHAR_TEX_WOOD:
	{
		fvol = 0.9;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	}
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
	{
		fvol = 0.8;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	}
	case CHAR_TEX_FLESH:
	{
		fvol = 1.0;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0,cnt-1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0,0xf) );

	return chTextureType;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[ 32 ];
	int idx;

	if ( pe->classnumber == 1 )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		sprintf( decalname, "{bproof1" );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName, char chTextureType )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );


	iRand = gEngfuncs.pfnRandomLong(0,0x7FFF);
	if ( iRand < (0x7fff/2) )// not every bullet makes a sound.
	{
		if( chTextureType == CHAR_TEX_VENT || chTextureType == CHAR_TEX_METAL )
		{
			switch( iRand % 2 )
			{
			case 0: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_metal-1.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			case 1: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_metal-2.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			}
		}
		else
		{
			switch( iRand % 7)
			{
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 3:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 4:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 5: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_conc-1.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			case 6: gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_conc-2.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM); break;
			}
		}

	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot(
						gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ),
						gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );

		}
	}
}

void EV_HLDM_DecalGunshot(pmtrace_t *pTrace, int iBulletType, float scale, int r, int g, int b, bool bCreateSparks, char cTextureType)
{
	physent_t *pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ), cTextureType );

		if( bCreateSparks )
		{
			Vector dir = pTrace->plane.normal;
			dir.x = dir.x * dir.x * gEngfuncs.pfnRandomFloat(4.0f, 12.0f );
			dir.y = dir.y * dir.y * gEngfuncs.pfnRandomFloat(4.0f, 12.0f );
			dir.z = dir.z * dir.z * gEngfuncs.pfnRandomFloat(4.0f, 12.0f );
			gEngfuncs.pEfxAPI->R_StreakSplash( pTrace->endpos, dir, 4, gEngfuncs.pfnRandomLong( 15, 30 ), dir.z, -75.0f, 75.0f );
		}


		TEMPENTITY *te = gEngfuncs.pEfxAPI->R_DefaultSprite( pTrace->endpos,
							gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/wall_puff1.spr"), 30.0f );
		if( te )
		{
			te->flags = FTENT_SPRANIMATE | FTENT_FADEOUT;
			te->entity.curstate.rendermode = kRenderTransAdd;
			te->entity.curstate.rendercolor.r = r;
			te->entity.curstate.rendercolor.g = g;
			te->entity.curstate.rendercolor.b = b;
			te->entity.curstate.renderamt = 255;
			te->entity.curstate.scale = 0.33;
			te->entity.baseline.origin = 20 * pTrace->plane.normal;
		}

	}
}

int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount )
{
	int tracer = 0;
	int i;
	qboolean player = idx >= 1 && idx <= gEngfuncs.GetMaxClients() ? true : false;

	if ( iTracerFreq != 0 && ( (*tracerCount)++ % iTracerFreq) == 0 )
	{
		vec3_t vecTracerSrc;

		if ( player )
		{
			vec3_t offset( 0, 0, -4 );

			// adjust tracer position for player
			for ( i = 0; i < 3; i++ )
			{
				vecTracerSrc[ i ] = vecSrc[ i ] + offset[ i ] + right[ i ] * 2 + forward[ i ] * 16;
			}
		}
		else
		{
			VectorCopy( vecSrc, vecTracerSrc );
		}

		if ( iTracerFreq != 1 )		// guns that always trace also always decal
			tracer = 1;


		EV_CreateTracer( vecTracerSrc, end );
	}

	return tracer;
}

/*
============
EV_DescribeBulletTypeParameters

Sets iPenetrationPower and flPenetrationDistance.
If iBulletType is unknown, calls assert() and sets these two vars to 0
============
*/
void EV_DescribeBulletTypeParameters(int iBulletType, int &iPenetrationPower, float &flPenetrationDistance)
{
	switch (iBulletType)
	{
	case BULLET_PLAYER_9MM:
	{
		iPenetrationPower = 21;
		flPenetrationDistance = 800;
		break;
	}

	case BULLET_PLAYER_45ACP:
	{
		iPenetrationPower = 15;
		flPenetrationDistance = 500;
		break;
	}

	case BULLET_PLAYER_50AE:
	{
		iPenetrationPower = 30;
		flPenetrationDistance = 1000;
		break;
	}

	case BULLET_PLAYER_762MM:
	{
		iPenetrationPower = 39;
		flPenetrationDistance = 5000;
		break;
	}

	case BULLET_PLAYER_556MM:
	{
		iPenetrationPower = 35;
		flPenetrationDistance = 4000;
		break;
	}

	case BULLET_PLAYER_338MAG:
	{
		iPenetrationPower = 45;
		flPenetrationDistance = 8000;
		break;
	}

	case BULLET_PLAYER_57MM:
	{
		iPenetrationPower = 30;
		flPenetrationDistance = 2000;
		break;
	}

	case BULLET_PLAYER_357SIG:
	{
		iPenetrationPower = 25;
		flPenetrationDistance = 800;
		break;
	}

	default:
	{
		iPenetrationPower = 0;
		flPenetrationDistance = 0;
		break;
	}
	}
}



/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets(int idx,
						 float *forward, float *right, float *up,
						 int cShots,
						 float *vecSrc, float *vecDirShooting, float *vecSpread,
						 float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, int iPenetration)
{
	int i;
	pmtrace_t tr;
	int iShot;
	int iPenetrationPower;
	float flPenetrationDistance;

	EV_DescribeBulletTypeParameters( iBulletType, iPenetrationPower, flPenetrationDistance );

	for ( iShot = 1; iShot <= cShots; iShot++ )
	{
		Vector vecShotSrc = vecSrc;
		int iShotPenetration = iPenetration;
		Vector vecDir, vecEnd;

		if ( iBulletType == BULLET_PLAYER_BUCKSHOT )
		{
			//We randomize for the Shotgun.
			float x, y, z;
			do {
				x = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				z = x*x+y*y;
			} while (z > 1);

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * vecSpread[0] * right[ i ] + y * vecSpread[1] * up [ i ];
				vecEnd[i] = vecShotSrc[ i ] + flDistance * vecDir[ i ];
			}
		}
		else //But other guns already have their spread randomized in the synched spread.
		{
			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + vecSpread[0] * right[ i ] + vecSpread[1] * up [ i ];
				vecEnd[i] = vecShotSrc[ i ] + flDistance * vecDir[ i ];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );


		while (iShotPenetration != 0)
		{
			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace(vecShotSrc, vecEnd, 0, -1, &tr);
			EV_HLDM_CheckTracer( idx, vecShotSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

			float flCurrentDistance = tr.fraction * flDistance;
#ifdef _DEBUG
			// absolute end
			gEngfuncs.pEfxAPI->R_ParticleLine( vecEnd, vecEnd, 255, 255, 255, 20.0f);
#endif

			if( flCurrentDistance == 0.0f )
			{
#ifdef _DEBUG
				// absolute start ( don't traceline after )
				gEngfuncs.pEfxAPI->R_ParticleLine( vecShotSrc, vecShotSrc, 255, 255, 0, 20.0f );
#endif
				break;
			}

#ifdef _DEBUG
			// absoulute start
			gEngfuncs.pEfxAPI->R_ParticleLine( vecShotSrc, vecShotSrc, 0, 0, 255, 20.0f);
			// trace
			gEngfuncs.pEfxAPI->R_ParticleLine( vecShotSrc, tr.endpos, 0, 255, 0, 20.0f );
			// hit
			gEngfuncs.pEfxAPI->R_ParticleLine( tr.endpos, tr.endpos, 255, 0, 0, 20.0f);
#endif
			if ( flCurrentDistance > flPenetrationDistance )
				iShotPenetration = 0;
			else iShotPenetration--;

			char cTextureType = EV_HLDM_PlayTextureSound(idx, &tr, vecShotSrc, vecEnd, iBulletType );
			bool bSparks = true;
			int r_smoke, g_smoke, b_smoke;
			r_smoke = g_smoke = b_smoke = 40;

			switch (cTextureType)
			{
			case CHAR_TEX_METAL:
				iPenetrationPower *= 0.15;
				break;
			case CHAR_TEX_CONCRETE:
				r_smoke = g_smoke = b_smoke = 65;
				iPenetrationPower *= 0.25;
				break;
			case CHAR_TEX_VENT:
			case CHAR_TEX_GRATE:
				iPenetrationPower *= 0.5;
				break;
			case CHAR_TEX_TILE:
				iPenetrationPower *= 0.65;
				break;
			case CHAR_TEX_COMPUTER:
				iPenetrationPower *= 0.4;
				break;
			case CHAR_TEX_WOOD:
				bSparks = false;
				r_smoke = 75;
				g_smoke = 42;
				b_smoke = 15;
				break;
			}

			// do damage, paint decals
			EV_HLDM_DecalGunshot( &tr, iBulletType, 0, r_smoke, g_smoke, b_smoke, bSparks, cTextureType );

			if( iBulletType == BULLET_PLAYER_BUCKSHOT || iShotPenetration == 0 )
			{
				break;
			}

			flDistance = (flDistance - flCurrentDistance) * 0.5;
			for( int i = 0; i < 3; i++ )
			{
				vecShotSrc[i] = tr.endpos[i]  + iPenetrationPower * vecDir[i];
				vecEnd[i]     = vecShotSrc[i] + flDistance        * vecDir[i];
			}

			Vector vStartPos, vEndPos;
			pmtrace_t trOriginal;
			int i;
			for( i = 1; i <= iPenetrationPower; i++ )
			{
				for( int j = 0; j < 3; j++ )
				{
					vStartPos[j] = tr.endpos[j] + i * vecDir[j];
					vEndPos[j] = vecDir[j] + vStartPos[j];
				}
				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace(vStartPos, vEndPos, 0, -1, &trOriginal);
				if ( trOriginal.startsolid && trOriginal.inopen )
					break;
			}
			if( i != iPenetrationPower )
			{
				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace(vEndPos, vStartPos, 0, -1, &trOriginal);
				EV_HLDM_DecalGunshot( &trOriginal, iBulletType, 0, r_smoke, g_smoke, b_smoke, bSparks, cTextureType );
			}
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}

}

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;

	char sz[ 256 ];

	idx = args->entindex;

	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strncpy( sz, "plats/ttrain1.wav", sizeof(sz)); break;
	case 2: strncpy( sz, "plats/ttrain2.wav", sizeof(sz)); break;
	case 3: strncpy( sz, "plats/ttrain3.wav", sizeof(sz)); break;
	case 4: strncpy( sz, "plats/ttrain4.wav", sizeof(sz)); break;
	case 5: strncpy( sz, "plats/ttrain6.wav", sizeof(sz)); break;
	case 6: strncpy( sz, "plats/ttrain7.wav", sizeof(sz)); break;
	default:
		// no sound
		strncpy( sz, "",  sizeof(sz) );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, 0, pitch );
	}
}

void EV_CS16Client_KillEveryRound( TEMPENTITY *te, float frametime, float current_time )
{
	if( g_flRoundTime > te->entity.curstate.fuser4 )
	{
		// Mark it die on next TempEntUpdate
		te->die = 0.0f;
		// Set null renderamt, so it will be invisible now
		// Also it will die immediately, if FTEMP_FADEOUT was set
		te->entity.curstate.renderamt = 0;
	}
}

void RemoveBody(TEMPENTITY *te, float frametime, float current_time)
{
	// go underground...
	/*if ( current_time >= 2 * te->entity.curstate.fuser2 + 5.0 )
		te->entity.origin.z -= 5.0 * frametime;*/
}

void HitBody(TEMPENTITY *ent, pmtrace_s *ptr)
{
	/*if ( ptr->plane.normal.z > 0.0 )
		ent->flags |= FTENT_NONE;*/
}


void CreateCorpse(Vector *p_vOrigin, Vector *p_vAngles, const char *pModel, float flAnimTime, int iSequence, int iBody)
{
	int modelIdx = gEngfuncs.pEventAPI->EV_FindModelIndex(pModel);
	vec3_t null(0, 0, 0);
	TEMPENTITY *model = gEngfuncs.pEfxAPI->R_TempModel( (float*)p_vOrigin,
														null,
														(float*)p_vAngles,
														gEngfuncs.pfnGetCvarFloat("cl_corpsestay"),
														modelIdx,
														0 );

	if(model)
	{
		//model->frameMax = -1;
		model->entity.curstate.animtime = flAnimTime;
		model->entity.curstate.framerate = 1.0;
		model->entity.curstate.frame = 0;
		model->entity.curstate.sequence = iSequence;
		model->entity.curstate.body = iBody;
		model->entity.curstate.fuser1 = gHUD.m_flTime + 1.0;
		model->entity.curstate.fuser2 = gEngfuncs.pfnGetCvarFloat("cl_corpsestay") + gHUD.m_flTime;
		model->hitcallback = HitBody;
		model->callback = RemoveBody;
	}
}

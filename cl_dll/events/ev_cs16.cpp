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

#include "pm_shared.h"

extern float g_flRoundTime;

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
															 Vector( (int)Com_RandomLong( -100, 100 ), (int)Com_RandomLong( -100, 100 ), (int)Com_RandomLong( -100, 100 ) ),
															 5, iSmokeSprite, kRenderTransAlpha, kRenderFxNone, 1.0, 0.5, FTENT_FADEOUT | FTENT_PERSIST );

		if(pTemp)
		{
			pTemp->fadeSpeed = 0.6;
			pTemp->entity.curstate.framerate = 1;
			pTemp->entity.curstate.renderamt = 255;
			int Color = Com_RandomLong( 0, 140 );
			pTemp->entity.curstate.rendercolor.r = Color;
			pTemp->entity.curstate.rendercolor.g = Color;
			pTemp->entity.curstate.rendercolor.b = Color;
		}
	}

}

char EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType, bool& isSky )
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
	isSky = false;

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

			if( !strcmp( pTextureName, "sky" ) )
			{
				isSky = true;
			}
			// strip leading '-0' or '+0~' or '{' or '!'
			else if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}
			else if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
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
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[Com_RandomLong(0,cnt-1)], fvol, fattn, 0, 96 + Com_RandomLong(0,0xf) );

	return chTextureType;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[ 32 ];
	int idx;

	if ( pe->classnumber == 1 )
	{
		idx = Com_RandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		sprintf( decalname, "{bproof1" );
	}
	else
	{
		idx = Com_RandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName, char chTextureType )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );


	iRand = Com_RandomLong(0,0x7FFF);
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


void EV_WallPuff_Wind( struct tempent_s *te, float frametime, float currenttime )
{
	static bool xWindDirection = true;
	static bool yWindDirection = true;
	static float xWindMagnitude;
	static float yWindMagnitude;

	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin.x = 0.97 * te->entity.baseline.origin.x;
		te->entity.baseline.origin.y = 0.97 * te->entity.baseline.origin.y;
		te->entity.baseline.origin.z = 0.97 * te->entity.baseline.origin.z + 0.7;
		if ( te->entity.baseline.origin.z > 70.0 )
			te->entity.baseline.origin.z = 70.0;
	}

	if ( te->entity.curstate.frame > 6.0 )
	{
		xWindMagnitude += 0.075;
		if ( xWindMagnitude > 5.0 )
			xWindMagnitude = 5.0;

		yWindMagnitude += 0.075;
		if ( yWindMagnitude > 5.0 )
			yWindMagnitude = 5.0;

		if( xWindDirection )
			te->entity.baseline.origin.x += xWindMagnitude;
		else
			te->entity.baseline.origin.x -= xWindMagnitude;

		if( yWindDirection )
			te->entity.baseline.origin.y += yWindMagnitude;
		else
			te->entity.baseline.origin.y -= yWindMagnitude;

		if ( !Com_RandomLong(0, 10) && yWindMagnitude > 3.0 )
		{
			yWindMagnitude = 0;
			yWindDirection = !yWindDirection;
		}
		if ( !Com_RandomLong(0, 10) && xWindMagnitude > 3.0 )
		{
			xWindMagnitude = 0;
			xWindDirection = !xWindDirection;
		}
	}
}

void EV_SmokeRise( struct tempent_s *te, float frametime, float currenttime )
{
	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin = 0.97f * te->entity.baseline.origin;
		te->entity.baseline.origin.z += 0.7f;

		if( te->entity.baseline.origin.z > 70.0f )
			te->entity.baseline.origin.z = 70.0f;
	}
}

void EV_HugWalls(TEMPENTITY *te, pmtrace_s *ptr)
{
	Vector norm = te->entity.baseline.origin.Normalize();
	float len = te->entity.baseline.origin.Length();

	Vector v =
	{
		ptr->plane.normal.y * norm.x - norm.y * ptr->plane.normal.x,
		ptr->plane.normal.x * norm.z - norm.x * ptr->plane.normal.z,
		ptr->plane.normal.z * norm.y - norm.z * ptr->plane.normal.y
	};
	Vector v2 =
	{
		ptr->plane.normal.y * v.z - v.y * ptr->plane.normal.x,
		ptr->plane.normal.x * v.x - v.z * ptr->plane.normal.z,
		ptr->plane.normal.z * v.y - v.x * ptr->plane.normal.y
	};

	if( len <= 2000.0f )
		len *= 1.5;
	else len = 3000.0f;

	te->entity.baseline.origin.x = v2.z * len * 1.5;
	te->entity.baseline.origin.y = v2.y * len * 1.5;
	te->entity.baseline.origin.z = v2.x * len * 1.5;
}

void EV_CS16Client_CreateSmoke(int type, Vector origin, Vector dir, int speed, float scale, int r, int g, int b , bool wind, Vector velocity, int framerate )
{
	TEMPENTITY *te = NULL;
	void ( *callback )( struct tempent_s *ent, float frametime, float currenttime ) = NULL;
	char path[64];

	switch( type )
	{
	case SMOKE_WALLPUFF:
		if( !gHUD.fastsprites->value )
		{
			strcpy( path, "sprites/wall_puff1.spr" );

			path[17] += Com_RandomLong(0, 3); // randomize a bit
		}
		else
		{
			strcpy( path, "sprites/fast_wallpuff1.spr" );
			te = gEngfuncs.pEfxAPI->R_DefaultSprite( origin,
								gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fast_wallpuff1.spr"), 30.0f );
		}
		break;
	case SMOKE_RIFLE:
		strcpy( path, "sprites/rifle_smoke1.spr" );
		path[19] += Com_RandomLong(0, 2); // randomize a bit


		break;
	case SMOKE_PISTOL:
		strcpy( path, "sprites/pistol_smoke1.spr" );
		path[20] += Com_RandomLong(0, 1);  // randomize a bit


		break;
	case SMOKE_BLACK:
		strcpy( path, "sprites/black_smoke1.spr" );
		path[19] += Com_RandomLong(0, 3); // randomize a bit


		break;
	default:
		assert(("Unknown smoketype!"));
	}

	if( wind )
		callback = EV_WallPuff_Wind;
	else
		callback = EV_SmokeRise;


	te = gEngfuncs.pEfxAPI->R_DefaultSprite( origin, gEngfuncs.pEventAPI->EV_FindModelIndex( path ), framerate );

	if( te )
	{
		te->callback = callback;
		te->hitcallback = EV_HugWalls;
		te->flags |= FTENT_COLLIDEALL | FTENT_CLIENTCUSTOM;
		te->entity.curstate.rendermode = kRenderTransAdd;
		te->entity.curstate.rendercolor.r = r;
		te->entity.curstate.rendercolor.g = g;
		te->entity.curstate.rendercolor.b = b;
		te->entity.curstate.renderamt = Com_RandomLong( 100, 180 );
		te->entity.curstate.scale = scale;
		te->entity.baseline.origin = speed * dir;

		if( !velocity.IsNull() )
		{
			velocity.x *= 0.5;
			velocity.y *= 0.5;
			velocity.z *= 0.9;
			te->entity.baseline.origin = te->entity.baseline.origin + velocity;
		}
	}
}


void EV_HLDM_DecalGunshot(pmtrace_t *pTrace, int iBulletType, float scale, int r, int g, int b, bool bCreateWallPuff, bool bCreateSparks, char cTextureType, bool isSky)
{
	physent_t *pe;

	if( isSky )
		return; // don't try to draw decals, spawn wall puff on skybox?

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ), cTextureType );

		// create sparks
		if( gHUD.cl_weapon_sparks && gHUD.cl_weapon_sparks->value && bCreateSparks )
		{
			Vector dir = pTrace->plane.normal;
			dir.x = dir.x * dir.x * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			dir.y = dir.y * dir.y * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			dir.z = dir.z * dir.z * gEngfuncs.pfnRandomFloat( 4.0f, 12.0f );
			gEngfuncs.pEfxAPI->R_StreakSplash( pTrace->endpos, dir, 4, Com_RandomLong( 5, 10 ), dir.z, -75.0f, 75.0f );
		}

		// create wallpuff
		if( gHUD.cl_weapon_wallpuff && gHUD.cl_weapon_wallpuff->value && bCreateWallPuff )
		{
			/*TEMPENTITY *te = NULL;
			if( gHUD.fastsprites && !gHUD.fastsprites->value )
			{
				char path[] = "sprites/wall_puff1.spr";

				path[17] += Com_RandomLong(0, 3);
				te = gEngfuncs.pEfxAPI->R_DefaultSprite( pTrace->endpos,
									gEngfuncs.pEventAPI->EV_FindModelIndex(path), 30.0f );
			}
			else
			{
				te = gEngfuncs.pEfxAPI->R_DefaultSprite( pTrace->endpos,
									gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fast_wallpuff1.spr"), 30.0f );
			}

			if( te )
			{
				te->callback = EV_WallPuff_Wind;
				te->hitcallback = EV_HugWalls;
				te->flags |= FTENT_COLLIDEALL | FTENT_CLIENTCUSTOM;
				te->entity.curstate.rendermode = kRenderTransAdd;
				te->entity.curstate.rendercolor.r = r;
				te->entity.curstate.rendercolor.g = g;
				te->entity.curstate.rendercolor.b = b;
				te->entity.curstate.renderamt = Com_RandomLong( 100, 180 );
				te->entity.curstate.scale = 0.5;
				te->entity.baseline.origin = (25 + Com_RandomLong( 0, 4 ) ) * pTrace->plane.normal;
			}*/

			EV_CS16Client_CreateSmoke( SMOKE_WALLPUFF, pTrace->endpos, pTrace->plane.normal, 25, 0.5, r, g, b, true );
		}
	}
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
EV_HLDM_FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets(int idx,
						 float *forward, float *right, float *up,
						 int cShots,
						 float *vecSrc, float *vecDirShooting, float *vecSpread,
						 float flDistance, int iBulletType, int iPenetration)
{
	int i;
	pmtrace_t tr;
	int iShot;
	int iPenetrationPower;
	float flPenetrationDistance;
	bool isSky;

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
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecShotSrc, vecEnd, 0, -1, &tr );

			float flCurrentDistance = tr.fraction * flDistance;

			if( flCurrentDistance == 0.0f )
			{
				break;
			}

			if ( flCurrentDistance > flPenetrationDistance )
				iShotPenetration = 0;
			else iShotPenetration--;

			char cTextureType = EV_HLDM_PlayTextureSound(idx, &tr, vecShotSrc, vecEnd, iBulletType, isSky );
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
			EV_HLDM_DecalGunshot( &tr, iBulletType, 0, r_smoke, g_smoke, b_smoke, true, bSparks, cTextureType, isSky );

			if(/* iBulletType == BULLET_PLAYER_BUCKSHOT ||*/ iShotPenetration <= 0 )
			{
				break;
			}

			flDistance = (flDistance - flCurrentDistance) * 0.5;
			for( int i = 0; i < 3; i++ )
			{
				vecShotSrc[i] = tr.endpos[i]  + iPenetrationPower * vecDir[i];
				vecEnd[i]     = vecShotSrc[i] + flDistance        * vecDir[i];
			}


			// trace back, so we will have a decal on the other side of solid area
			pmtrace_t trOriginal;
			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace(vecShotSrc, vecSrc, 0, -1, &trOriginal);
			if( !trOriginal.startsolid )
				EV_HLDM_DecalGunshot( &trOriginal, iBulletType, 0, r_smoke, g_smoke, b_smoke, true, bSparks, cTextureType, isSky );
		}
		gEngfuncs.pEventAPI->EV_PopPMStates();
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

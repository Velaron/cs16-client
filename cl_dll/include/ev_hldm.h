//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once
#if !defined ( EV_HLDMH )
#define EV_HLDMH

// bullet types
typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM,
	BULLET_PLAYER_MP5,
	BULLET_PLAYER_357,
	BULLET_PLAYER_BUCKSHOT,
	BULLET_PLAYER_CROWBAR,

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,

	BULLET_PLAYER_45ACP,
	BULLET_PLAYER_338MAG,
	BULLET_PLAYER_762MM,
	BULLET_PLAYER_556MM,
	BULLET_PLAYER_50AE,
	BULLET_PLAYER_57MM,
	BULLET_PLAYER_357SIG
}
Bullet;

enum
{
	SMOKE_WALLPUFF = 0,
	SMOKE_RIFLE,
	SMOKE_PISTOL,
	SMOKE_BLACK
};

void EV_HLDM_GunshotDecalTrace(pmtrace_t *pTrace, char *decalName , char chTextureType);
void EV_HLDM_DecalGunshot(pmtrace_t *pTrace, int iBulletType, float scale, int r, int g, int b, bool bCreateSparks, char cTextureType, bool isSky);
void EV_HLDM_FireBullets(int idx,
						 float *forward, float *right, float *up,
						 int cShots,
						 float *vecSrc, float *vecDirShooting, float *vecSpread,
						 float flDistance, int iBulletType, int iPenetration);
void EV_CS16Client_KillEveryRound( struct tempent_s *te, float frametime, float currenttime );
void EV_CS16Client_CreateSmoke(int type, Vector origin, Vector dir, int speed, float scale, int r, int g, int b , bool wind, Vector velocity = Vector(0, 0, 0), int framerate = 35 );
#endif // EV_HLDMH

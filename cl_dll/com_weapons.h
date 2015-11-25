//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// com_weapons.h
// Shared weapons common function prototypes
#if !defined( COM_WEAPONSH )
#define COM_WEAPONSH
#ifdef _WIN32
#pragma once
#endif

#include "hud_iface.h"

#define PLAYER_CAN_SHOOT (1 << 0)
#define PLAYER_FREEZE_TIME_OVER ( 1 << 1 )
#define PLAYER_IN_BOMB_ZONE (1 << 2)
#define PLAYER_HOLDING_SHIELD (1 << 3)

extern "C"
{
	void _DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );
}

void			COM_Log( char *pszFile, char *fmt, ...);
int				CL_IsDead( void );

float			UTIL_SharedRandomFloat( unsigned int seed, float low, float high );
int				UTIL_SharedRandomLong( unsigned int seed, int low, int high );

int				HUD_GetWeaponAnim( void );
void			HUD_SendWeaponAnim( int iAnim, int body, int force );
void			HUD_PlaySound( char *sound, float volume );
void			HUD_PlaybackEvent( int flags, const struct edict_s *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
void			HUD_SetMaxSpeed( const struct edict_s *ed, float speed );
int				stub_PrecacheModel( char* s );
int				stub_PrecacheSound( char* s );
unsigned short	stub_PrecacheEvent( int type, const char *s );
const char		*stub_NameForFunction	( u_int32_t function );
void			stub_SetModel			( struct edict_s *e, const char *m );


extern cvar_t *cl_lw;

extern int g_runfuncs;
extern vec3_t v_angles;
extern float g_lastFOV;
extern int g_iWeaponFlags;
extern bool g_bInBombZone;
extern int g_iFreezeTimeOver;
extern int g_bHoldingShield;
extern vec3_t g_vPlayerVelocity;
extern float g_flPlayerSpeed;
extern struct local_state_s *g_finalstate;

#endif

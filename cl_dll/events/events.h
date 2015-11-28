#pragma  once
#ifndef EVENTS_H
#define EVENTS_H

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "r_efx.h"

#include "eventscripts.h"
#include "event_api.h"
#include "pm_defs.h"
#include "ev_hldm.h"
#include "com_weapons.h"

#ifndef PITCH
// MOVEMENT INFO
// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2
#endif

extern "C" char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );


#define DECLARE_EVENT( x ) void EV_##x( struct event_args_s *args )
#define HOOK_EVENT( x, y ) gEngfuncs.pfnHookEvent( "events/" #x ".sc", EV_##y )

extern "C"
{
DECLARE_EVENT(FireAK47);
DECLARE_EVENT(FireAUG);
DECLARE_EVENT(FireAWP);
DECLARE_EVENT(CreateExplo);
DECLARE_EVENT(CreateSmoke);
DECLARE_EVENT(FireDEAGLE);
DECLARE_EVENT(DecalReset);
DECLARE_EVENT(FireEliteLeft);
DECLARE_EVENT(FireEliteRight);
DECLARE_EVENT(FireFAMAS);
DECLARE_EVENT(Fire57);
DECLARE_EVENT(FireG3SG1);
DECLARE_EVENT(FireGALIL);
DECLARE_EVENT(Fireglock18);
DECLARE_EVENT(Knife);
DECLARE_EVENT(FireM249);
DECLARE_EVENT(FireM3);
DECLARE_EVENT(FireM4A1);
DECLARE_EVENT(FireMAC10);
DECLARE_EVENT(FireMP5);
DECLARE_EVENT(FireP228);
DECLARE_EVENT(FireP90);
DECLARE_EVENT(FireScout);
DECLARE_EVENT(FireSG550);
DECLARE_EVENT(FireSG552);
DECLARE_EVENT(FireTMP);
DECLARE_EVENT(FireUMP45);
DECLARE_EVENT(FireUSP);
DECLARE_EVENT(Vehicle);
DECLARE_EVENT(FireXM1014);
DECLARE_EVENT(TrainPitchAdjust);
}

void Game_HookEvents( void );
void EV_HLDM_SmokeGrenade( float x, float y, float z );

#endif

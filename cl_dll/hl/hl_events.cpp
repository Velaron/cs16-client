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
#include "../hud.h"
#include "../cl_util.h"
#include "event_api.h"

extern "C"
{
// HLDM
	void EV_Knife( event_args_t *args );
	void EV_FireUSP( struct event_args_s *args );
	void EV_Fireglock18( struct event_args_s *args );
	void EV_FireM4A1( struct event_args_s *args );
	void EV_FireAK47( struct event_args_s *args );
	void EV_FireAWP( struct event_args_s *args );
	void EV_FireGALIL( struct event_args_s *args );
	void EV_FireFAMAS( struct event_args_s *args );
	void EV_FireDEAGLE( struct event_args_s *args );
	void EV_FireAUG( struct event_args_s *args );
	void EV_FireSG552( struct event_args_s *args );
	void EV_FireMP5( struct event_args_s *args );
	void EV_FireM3( struct event_args_s *args );
	void EV_Dummy( struct event_args_s *args );

void EV_TrainPitchAdjust( struct event_args_s *args );
}

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void Game_HookEvents( void )
{
	gEngfuncs.pfnHookEvent("events/ak47.sc", EV_FireAK47);
	gEngfuncs.pfnHookEvent("events/aug.sc", EV_FireAUG);
	gEngfuncs.pfnHookEvent("events/awp.sc", EV_FireAWP);
	gEngfuncs.pfnHookEvent("events/createexplo.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/createsmoke.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/deagle.sc", EV_FireDEAGLE);
	gEngfuncs.pfnHookEvent("events/decal_reset.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/elite_left.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/elite_right.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/famas.sc", EV_FireFAMAS);
	gEngfuncs.pfnHookEvent("events/fiveseven.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/g3sg1.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/galil.sc", EV_FireGALIL);
	gEngfuncs.pfnHookEvent("events/glock18.sc", EV_Fireglock18);
	gEngfuncs.pfnHookEvent("events/knife.sc", EV_Knife);
	gEngfuncs.pfnHookEvent("events/m249.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/m3.sc", EV_FireM3);
	gEngfuncs.pfnHookEvent("events/m4a1.sc", EV_FireM4A1);
	gEngfuncs.pfnHookEvent("events/mac10.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/mp5n.sc", EV_FireMP5);
	gEngfuncs.pfnHookEvent("events/p228.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/p90.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/scout.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/sg550.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/sg552.sc", EV_FireSG552);
	gEngfuncs.pfnHookEvent("events/tmp.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/ump45.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/usp.sc", EV_FireUSP);
	gEngfuncs.pfnHookEvent("events/vehicle.sc", EV_Dummy);
	gEngfuncs.pfnHookEvent("events/xm1014.sc", EV_Dummy);
}

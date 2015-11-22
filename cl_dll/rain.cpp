/***
*
*	Copyright (c) 2005, BUzer.
*	
*	Used with permission for Spirit of Half-Life 1.5
*
****/
/*
====== rain.cpp ========================================================
*/

#include <memory.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "cdll_int.h"
#include "pm_defs.h"
#include "event_api.h"

#include "rain.h"

void WaterLandingEffect(cl_drip *drip);

rain_properties     Rain;

cl_drip     FirstChainDrip;
cl_rainfx   FirstChainFX;

double rain_curtime;    // current time
double rain_oldtime;    // last time we have updated drips
double rain_timedelta;  // difference between old time and current time
double rain_nextspawntime;  // when the next drip should be spawned

int dripcounter = 0;
int fxcounter = 0;
int RainInfo = 0;

/*
=================================
ProcessRain

Must think every frame.
=================================
*/
void ProcessRain( void )
{
	rain_oldtime = rain_curtime; // save old time
	rain_curtime = gEngfuncs.GetClientTime();
	rain_timedelta = rain_curtime - rain_oldtime;

	// first frame
	if (rain_oldtime == 0)
	{
		// fix first frame bug with nextspawntime
		rain_nextspawntime = gEngfuncs.GetClientTime();
		return;
	}

	if (Rain.dripsPerSecond == 0 && FirstChainDrip.p_Next == NULL)
	{
		// keep nextspawntime correct
		rain_nextspawntime = rain_curtime;
		return;
	}

	if (rain_timedelta == 0)
		return; // not in pause

	double timeBetweenDrips = 1 / (double)Rain.dripsPerSecond;

	cl_drip* curDrip = FirstChainDrip.p_Next;
	cl_drip* nextDrip = NULL;


	// save debug info
	float debug_lifetime = 0;
	int debug_howmany = 0;
	int debug_attempted = 0;
	int debug_dropped = 0;

	while (curDrip != NULL) // go through list
	{
		nextDrip = curDrip->p_Next; // save pointer to next drip

		if (Rain.weatherMode == 0)
			curDrip->origin.z -= rain_timedelta * DRIPSPEED; // rain
		else
			curDrip->origin.z -= rain_timedelta * SNOWSPEED; // snow

		curDrip->origin.x += rain_timedelta * curDrip->xDelta;
		curDrip->origin.y += rain_timedelta * curDrip->yDelta;
		
		// remove drip if its origin lower than minHeight
		if (curDrip->origin.z < curDrip->minHeight) 
		{
			if (curDrip->landInWater/* && Rain.weatherMode == 0*/)
				WaterLandingEffect(curDrip); // create water rings

			if (RainInfo)
			{
				debug_lifetime += (rain_curtime - curDrip->birthTime);
				debug_howmany++;
			}
			
			curDrip->p_Prev->p_Next = curDrip->p_Next; // link chain
			if (nextDrip != NULL)
				nextDrip->p_Prev = curDrip->p_Prev; 
			delete curDrip;
					
			dripcounter--;
		}

		curDrip = nextDrip; // restore pointer, so we can continue moving through chain
	}

	int maxDelta; // maximum height randomize distance
	float falltime;
	if (Rain.weatherMode == 0)
	{
		maxDelta = DRIPSPEED * rain_timedelta; // for rain
		falltime = (Rain.globalHeight + 4096) / DRIPSPEED;
	}
	else
	{
		maxDelta = SNOWSPEED * rain_timedelta; // for snow
		falltime = (Rain.globalHeight + 4096) / SNOWSPEED;
	}

	while (rain_nextspawntime < rain_curtime)
	{
		rain_nextspawntime += timeBetweenDrips;		
		if (RainInfo)
			debug_attempted++;
				
		if (dripcounter < MAXDRIPS) // check for overflow
		{
			float deathHeight;
			vec3_t vecStart, vecEnd;

			vecStart[0] = gEngfuncs.pfnRandomFloat(gHUD.m_vecOrigin.x - Rain.distFromPlayer, gHUD.m_vecOrigin.x + Rain.distFromPlayer);
			vecStart[1] = gEngfuncs.pfnRandomFloat(gHUD.m_vecOrigin.y - Rain.distFromPlayer, gHUD.m_vecOrigin.y + Rain.distFromPlayer);
			vecStart[2] = Rain.globalHeight;

			float xDelta = Rain.windX + gEngfuncs.pfnRandomFloat(Rain.randX * -1, Rain.randX);
			float yDelta = Rain.windY + gEngfuncs.pfnRandomFloat(Rain.randY * -1, Rain.randY);

			// find a point at bottom of map
			vecEnd[0] = falltime * xDelta;
			vecEnd[1] = falltime * yDelta;
			vecEnd[2] = -4096;

			pmtrace_t pmtrace;
			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecStart, vecEnd, PM_STUDIO_IGNORE, -1, &pmtrace );

			if (pmtrace.startsolid)
			{
				if (RainInfo)
					debug_dropped++;

				continue; // drip cannot be placed
			}
			
			// falling to water?
			int contents = gEngfuncs.PM_PointContents( pmtrace.endpos, NULL );
			if (contents == CONTENTS_WATER)
			{
				int waterEntity = gEngfuncs.PM_WaterEntity( pmtrace.endpos );
				if ( waterEntity > 0 )
				{
					cl_entity_t *pwater = gEngfuncs.GetEntityByIndex( waterEntity );
					if ( pwater && ( pwater->model != NULL ) )
					{
						deathHeight = pwater->curstate.maxs[2];
					}
					else
					{
						gEngfuncs.Con_Printf("Rain error: can't get water entity\n");
						continue;
					}
				}
				else
				{
					gEngfuncs.Con_Printf("Rain error: water is not func_water entity\n");
					continue;
				}
			}
			else
			{
				deathHeight = pmtrace.endpos[2];
			}

			// just in case..
			if (deathHeight > vecStart[2])
			{
				gEngfuncs.Con_Printf("Rain error: can't create drip in water\n");
				continue;
			}


			cl_drip *newClDrip = new cl_drip;
			if (!newClDrip)
			{
				Rain.dripsPerSecond = 0; // disable rain
				gEngfuncs.Con_Printf( "Rain error: failed to allocate object!\n");
				return;
			}
			
			vecStart[2] -= gEngfuncs.pfnRandomFloat(0, maxDelta); // randomize a bit
			
			newClDrip->alpha = gEngfuncs.pfnRandomFloat(0.12, 0.2);
			VectorCopy(vecStart, newClDrip->origin);
			
			newClDrip->xDelta = xDelta;
			newClDrip->yDelta = yDelta;
	
			newClDrip->birthTime = rain_curtime; // store time when it was spawned
			newClDrip->minHeight = deathHeight;

			if (contents == CONTENTS_WATER)
				newClDrip->landInWater = 1;
			else
				newClDrip->landInWater = 0;

			// add to first place in chain
			newClDrip->p_Next = FirstChainDrip.p_Next;
			newClDrip->p_Prev = &FirstChainDrip;
			if (newClDrip->p_Next != NULL)
				newClDrip->p_Next->p_Prev = newClDrip;
			FirstChainDrip.p_Next = newClDrip;

			dripcounter++;
		}
		else
		{
			gEngfuncs.Con_Printf( "Rain error: Drip limit overflow!\n" );
			return;
		}
	}

	if (RainInfo) // print debug info
	{
		gEngfuncs.Con_Printf( "Rain info: Drips exist: %i\n", dripcounter );
		gEngfuncs.Con_Printf( "Rain info: FX's exist: %i\n", fxcounter );
		gEngfuncs.Con_Printf( "Rain info: Attempted/Dropped: %i, %i\n", debug_attempted, debug_dropped);
		if (debug_howmany)
		{
			float ave = debug_lifetime / (float)debug_howmany;
			gEngfuncs.Con_Printf( "Rain info: Average drip life time: %f\n", ave);
		}
		else
			gEngfuncs.Con_Printf( "Rain info: Average drip life time: --\n");
	}
	return;
}

/*
=================================
WaterLandingEffect
=================================
*/
void WaterLandingEffect(cl_drip *drip)
{
	if (fxcounter >= MAXFX)
	{
		gEngfuncs.Con_Printf( "Rain error: FX limit overflow!\n" );
		return;
	}	
	
	cl_rainfx *newFX = new cl_rainfx;
	if (!newFX)
	{
		gEngfuncs.Con_Printf( "Rain error: failed to allocate FX object!\n");
		return;
	}
			
	newFX->alpha = gEngfuncs.pfnRandomFloat(0.6, 0.9);
	VectorCopy(drip->origin, newFX->origin);
	newFX->origin[2] = drip->minHeight; // correct position
			
	newFX->birthTime = gEngfuncs.GetClientTime();
	newFX->life = gEngfuncs.pfnRandomFloat(0.7, 1);

	// add to first place in chain
	newFX->p_Next = FirstChainFX.p_Next;
	newFX->p_Prev = &FirstChainFX;
	if (newFX->p_Next != NULL)
		newFX->p_Next->p_Prev = newFX;
	FirstChainFX.p_Next = newFX;
			
	fxcounter++;
}

/*
=================================
ProcessFXObjects

Remove all fx objects with out time to live
Call every frame before ProcessRain
=================================
*/
void ProcessFXObjects( void )
{
	float curtime = gEngfuncs.GetClientTime();
	
	cl_rainfx* curFX = FirstChainFX.p_Next;
	cl_rainfx* nextFX = NULL;	

	while (curFX != NULL) // go through FX objects list
	{
		nextFX = curFX->p_Next; // save pointer to next
		
		// delete current?
		if ((curFX->birthTime + curFX->life) < curtime)
		{
			curFX->p_Prev->p_Next = curFX->p_Next; // link chain
			if (nextFX != NULL)
				nextFX->p_Prev = curFX->p_Prev; 
			delete curFX;					
			fxcounter--;
		}
		curFX = nextFX; // restore pointer
	}
}

/*
=================================
ResetRain
clear memory, delete all objects
=================================
*/
void ResetRain( void )
{
// delete all drips
	cl_drip* delDrip = FirstChainDrip.p_Next;
	FirstChainDrip.p_Next = NULL;
	
	while (delDrip != NULL)
	{
		cl_drip* nextDrip = delDrip->p_Next; // save pointer to next drip in chain
		delete delDrip;
		delDrip = nextDrip; // restore pointer
		dripcounter--;
	}
// delete all FX objects
	cl_rainfx* delFX = FirstChainFX.p_Next;
	FirstChainFX.p_Next = NULL;
	
	while (delFX != NULL)
	{
		cl_rainfx* nextFX = delFX->p_Next;
		delete delFX;
		delFX = nextFX;
		fxcounter--;
	}

	InitRain();
	return;
}

/*
=================================
InitRain
initialze system
=================================
*/
void InitRain( void )
{
	Rain.dripsPerSecond = 0;
	Rain.distFromPlayer = 0;
	Rain.windX = 0;
	Rain.windY = 0;
	Rain.randX = 0;
	Rain.randY = 0;
	Rain.weatherMode = 0;
	Rain.globalHeight = 0;

	FirstChainDrip.birthTime = 0;
	FirstChainDrip.minHeight = 0;
	FirstChainDrip.origin[0]=0;
	FirstChainDrip.origin[1]=0;
	FirstChainDrip.origin[2]=0;
	FirstChainDrip.alpha = 0;
	FirstChainDrip.xDelta = 0;
	FirstChainDrip.yDelta = 0;
	FirstChainDrip.landInWater = 0;
	FirstChainDrip.p_Next = NULL;
	FirstChainDrip.p_Prev = NULL;

	FirstChainFX.alpha = 0;
	FirstChainFX.birthTime = 0;
	FirstChainFX.life = 0;
	FirstChainFX.origin[0] = 0;
	FirstChainFX.origin[1] = 0;
	FirstChainFX.origin[2] = 0;
	FirstChainFX.p_Next = NULL;
	FirstChainFX.p_Prev = NULL;
	
	rain_oldtime = 0;
	rain_curtime = 0;
	rain_nextspawntime = 0;

	return;
}

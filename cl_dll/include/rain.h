/***
*
*	Copyright (c) 1996-2004, Shambler Team. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Shambler Team.  All other use, distribution, or modification is prohibited
*   without written permission from Shambler Team.
*
****/
/*
====== rain.h ========================================================
*/
#pragma once
#ifndef __RAIN_H__
#define __RAIN_H__

#define DRIPSPEED	900		// speed of raindrips (pixel per secs)
#define SNOWSPEED	200		// speed of snowflakes
#define SNOWFADEDIST	80

#define MAXDRIPS	2000	// max raindrops
#define MAXFX		3000	// max effects

#define DRIP_SPRITE_HALFHEIGHT	64
#define DRIP_SPRITE_HALFWIDTH	1
#define SNOW_SPRITE_HALFSIZE	3

// radius water rings
#define MAXRINGHALFSIZE		25	

typedef struct
{
	int			dripsPerSecond;
	float		distFromPlayer;
	float		windX, windY;
	float		randX, randY;
	int			weatherMode;	// 0 - snow, 1 - rain
	float		globalHeight;
} rain_properties;

typedef struct cl_drip
{
	float		birthTime;
	float		minHeight;	// minimal height to kill raindrop
	vec3_t		origin;
	float		alpha;

	float		xDelta;		// side speed
	float		yDelta;
	int		landInWater;
	
	cl_drip*		p_Next;		// next drip in chain
	cl_drip*		p_Prev;		// previous drip in chain
} cl_drip_t;

typedef struct cl_rainfx
{
	float		birthTime;
	float		life;
	vec3_t		origin;
	float		alpha;
	
	cl_rainfx*		p_Next;		// next fx in chain
	cl_rainfx*		p_Prev;		// previous fx in chain
} cl_rainfx_t;

extern rain_properties Rain;
extern cl_drip_t FirstChainDrip;
extern cl_rainfx_t FirstChainFX;


void ProcessRain( void );
void ProcessFXObjects( void );
void ResetRain( void );
void InitRain( void );

#endif

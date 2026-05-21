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
#include "demo.h"
#include "demo_api.h"
#include <memory.h>

float g_demozoom;

// FIXME:  There should be buffer helper functions to avoid all of the *(int *)& crap.

/*
=====================
Demo_WriteBuffer

Write some data to the demo stream
=====================
*/
void Demo_WriteBuffer( int type, int size, unsigned char *buffer )
{
	int pos = 0;
	unsigned char buf[8];

	if( type != TYPE_ZOOM && size != 4 )
	{
		gEngfuncs.Con_DPrintf( "%s: unexpected demo buffer type %i, skipping.\n", __func__, type );
		return;
	}

	memcpy( buf, &type, sizeof( type ));
	pos += sizeof( type );

	memcpy( &buf[pos], buffer, size );

	// Write full buffer out
	gEngfuncs.pDemoAPI->WriteBuffer( size + sizeof( int ), buf );
}

/*
=====================
Demo_ReadBuffer

Engine wants us to parse some data from the demo stream
=====================
*/
void DLLEXPORT Demo_ReadBuffer( int size, unsigned char *buffer )
{
	int type;
	int i = 0;

	if( size != 8 )
	{
		gEngfuncs.Con_DPrintf( "%s: unexpected size %d\n", __func__, size );
		return;
	}

	memcpy( &type, buffer, sizeof( type ));
	i += sizeof( type );
	switch( type )
	{
	case TYPE_ZOOM:
		memcpy( &g_demozoom, &buffer[i], sizeof( g_demozoom ));
		i += sizeof( g_demozoom );
		break;
	default:
		gEngfuncs.Con_DPrintf( "%s: unexpected demo buffer type %i, skipping.\n", __func__, type );
		break;
	}
}

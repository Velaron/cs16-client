//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once
#if !defined( DEMOH )
#define DEMOH

// Types of demo messages we can write/parse
enum
{
	TYPE_ZOOM = 1,
};

void Demo_WriteBuffer( int type, int size, unsigned char *buffer );

extern float g_demozoom;

#endif

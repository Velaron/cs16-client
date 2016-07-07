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
//
//  parsemsg.cpp
//
typedef unsigned char byte;
#define true 1

#include <stddef.h>
#include "parsemsg.h"
/*
static byte *gpBuf;
static size_t giSize;
static size_t giRead;
static bool giBadRead;

void BufferReader reader( void *buf, int size )
{
	giRead = 0;
	giBadRead = false;
	giSize = size;
	gpBuf = (byte*)buf;
}

int reader.ReadChar( void )
{
	int     c;
	
	if (giRead + 1 > giSize)
	{
		giBadRead = true;
		return -1;
	}
		
	c = (signed char)gpBuf[giRead];
	giRead++;
	
	return c;
}

int reader.ReadByte( void )
{
	int     c;
	
	if (giRead+1 > giSize)
	{
		giBadRead = true;
		return -1;
	}
		
	c = (unsigned char)gpBuf[giRead];
	giRead++;
	
	return c;
}

int reader.ReadShort( void )
{
	int     c;
	
	if (giRead+2 > giSize)
	{
		giBadRead = true;
		return -1;
	}
		
	c = (short)( gpBuf[giRead] + ( gpBuf[giRead+1] << 8 ) );
	
	giRead += 2;
	
	return c;
}

int reader.ReadWord( void )
{
	return reader.ReadShort();
}


int reader.ReadLong( void )
{
	int     c;
	
	if (giRead+4 > giSize)
	{
		giBadRead = true;
		return -1;
	}
		
 	c = gpBuf[giRead] + (gpBuf[giRead + 1] << 8) + (gpBuf[giRead + 2] << 16) + (gpBuf[giRead + 3] << 24);
	
	giRead += 4;
	
	return c;
}

float reader.ReadFloat( void )
{
	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;
	
	dat.b[0] = gpBuf[giRead];
	dat.b[1] = gpBuf[giRead+1];
	dat.b[2] = gpBuf[giRead+2];
	dat.b[3] = gpBuf[giRead+3];
	giRead += 4;
	
//	dat.l = LittleLong (dat.l);

	return dat.f;   
}

char* reader.ReadString( void )
{
	static char     string[2048];
	int             c;
	size_t			l = 0;

	string[0] = 0;
	do
	{
		if ( giRead+1 > giSize )
			break; // no more characters

		c = reader.ReadChar();
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string) - 1);
	
	string[l] = 0;
	
	return string;
}

float reader.ReadCoord( void )
{
	return (float)(reader.ReadShort() * (1.0/8));
}

float reader.ReadAngle( void )
{
	return (float)(reader.ReadChar() * (360.0/256));
}

float reader.ReadHiResAngle( void )
{
	return (float)(reader.ReadShort() * (360.0/65536));
}
*/

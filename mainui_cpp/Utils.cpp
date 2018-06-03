/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


// ui_qmenu.c -- Quake menu framework


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "keydefs.h"
#include "BtnsBMPTable.h"

#ifdef _DEBUG
void DBG_AssertFunction( bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage )
{
	if( fExpr ) return;

	char szOut[512];
	if( szMessage != NULL )
		sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)\n%s", szExpr, szFile, szLine, szMessage );
	else sprintf( szOut, "ASSERT FAILED:\n %s \n(%s@%d)", szExpr, szFile, szLine );
	Host_Error( szOut );
}
#endif	// DEBUG

void AddSpaces(char *s, int size, int buffersize)
{
	int len = strlen(s);

	size += len - ColorStrlen(s);
	if( size > buffersize )
		size = buffersize;

	while( len < size - 1 )
	{	
		s[len] = ' ';
		len++;
	}
	s[len] = '\0';
}

int ColorStrlen( const char *str )
{
	const char *p;

	if( !str )
		return 0;

	int len = 0;
	p = str;
	EngFuncs::UtfProcessChar( 0 );
	while( *p )
	{
		if( IsColorString( p ))
		{
			p += 2;
			continue;
		}

		p++;
		if( EngFuncs::UtfProcessChar( (unsigned char) *p ) )
			len++;
	}

	EngFuncs::UtfProcessChar( 0 );

	return len;
}

int ColorPrexfixCount( const char *str )
{
	const char *p;

	if( !str )
		return 0;

	int len = 0;
	p = str;

	//EngFuncs::UtfProcessChar(0);

	while( *p )
	{
		if( IsColorString( p ))
		{
			len += 2;
			p += 2;
			continue;
		}
		//if(!EngFuncs::UtfProcessChar((unsigned char)*p))
			//len++;
		p++;
	}

	//EngFuncs::UtfProcessChar(0);

	return len;
}

void StringConcat( char *dst, const char *src, size_t size )
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = size;
	size_t dlen;

	if( !dst || !src || !size )
		return;

#if 0
	// find the end of dst and adjust bytes left but don't go past end
	while(n-- != 0 && *d != '\0') d++;
	dlen = d - dst;
#else
	// VERY UNSAFE. SURE THAT DST IS BIG
	dlen = ColorStrlen( dst );
	d += strlen( dst );
#endif

	n = size - dlen;

	if ( n == 0 ) return;
	while ( *s != '\0' )
	{
		if ( n != 1 )
		{
			*d++ = *s;
			n--;
		}
		s++;
	}

	*d = '\0';
	return;
}

char *StringCopy( const char *input )
{
	if( !input ) return NULL;

	char *out = (char *)MALLOC( strlen( input ) + 1 );
	strcpy( out, input );

	return out;
}

/*
============
COM_CompareSaves
============
*/
int COM_CompareSaves( const void **a, const void **b )
{
	char *file1, *file2;

	file1 = (char *)*a;
	file2 = (char *)*b;

	int bResult;

	EngFuncs::CompareFileTime( file2, file1, &bResult );

	return bResult;
}

/*
============
COM_FileBase
============
*/
// Extracts the base name of a file (no path, no extension, assumes '/' as path separator)
void COM_FileBase ( const char *in, char *out )
{
	int len, start, end;

	len = strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end && in[end] != '.' && in[end] != '/' && in[end] != '\\' )
		end--;
	
	if ( in[end] != '.' )		// no '.', copy to end
		end = len-1;
	else 
		end--;			// Found ',', copy to left of '.'


	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && in[start] != '/' && in[start] != '\\' )
		start--;

	if ( in[start] != '/' && in[start] != '\\' )
		start = 0;
	else 
		start++;

	// Length of new sting
	len = end - start + 1;

	// Copy partial string
	strncpy( out, &in[start], len );
	// Terminate it
	out[len] = 0;
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
const char *Info_ValueForKey( const char *s, const char *key )
{
	char	pkey[MAX_INFO_STRING];
	static	char value[2][MAX_INFO_STRING]; // use two buffers so compares work without stomping on each other
	static	int valueindex;
	char	*o;
	
	valueindex ^= 1;
	if( *s == '\\' ) s++;
	printf("I_VFK '%s' '%s'\n", s, key );

	while( 1 )
	{
		o = pkey;
		while( *s != '\\' && *s != '\n' )
		{
			if( !*s ) return "";
			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value[valueindex];

		while( *s != '\\' && *s != '\n' && *s )
		{
			if( !*s ) return "";
			*o++ = *s++;
		}
		*o = 0;

		if( !strcmp( key, pkey ))
			return value[valueindex];
		if( !*s ) return "";
		s++;
	}
}


/* 
===================
Key_GetKey
===================
*/
int KEY_GetKey( const char *binding )
{
	const char *b;

	if ( !binding )
		return -1;

	for ( int i = 0; i < 256; i++ )
	{
		b = EngFuncs::KEY_GetBinding( i );
		if( !b ) continue;

		if( !stricmp( binding, b ))
			return i;
	}
	return -1;
}

/*
================
UI_FadeAlpha
================
*/
int UI_FadeAlpha( int starttime, int endtime )
{
	int	time, fade_time;

	if( starttime == 0 )
	{
		return 0xFFFFFFFF;
	}

	time = ( gpGlobals->time * 1000 ) - starttime;

	if( time >= endtime )
	{
		return 0x00FFFFFF;
	}

	// fade time is 1/4 of endtime
	fade_time = endtime / 4;
	fade_time = bound( 300, fade_time, 10000 );

	int alpha;

	// fade out
	if(( endtime - time ) < fade_time )
		alpha = bound( 0, (( endtime - time ) * 1.0f / fade_time ) * 255, 255 );
	else alpha = 255;

	return PackRGBA( 255, 255, 255, alpha );
}

void UI_EnableTextInput( bool enable )
{
	uiStatic.textInput = enable;
	EngFuncs::EnableTextInput( enable );
}

// Doesn't need anymore

/*
void *operator new( size_t a )
{
	return MALLOC( a );
}

void *operator new[]( size_t a )
{
	return MALLOC( a );
}

void operator delete( void *ptr )
{
	if( ptr ) FREE( ptr );
}

void operator delete[]( void *ptr )
{
	if( ptr ) FREE( ptr );
}
*/

#define BI_SIZE	40 // size of bitmap info header.

typedef unsigned short       word;

CBMP::CBMP( uint w, uint h )
{
	bmp_t bhdr;

	const size_t cbPalBytes = 0; // UNUSED
	const uint pixel_size = 4; // Always RGBA

	bhdr.id[0] = 'B';
	bhdr.id[1] = 'M';
	bhdr.width = ( w + 3 ) & ~3;
	bhdr.height = h;
	bhdr.bitmapHeaderSize = BI_SIZE; // must be 40!
	bhdr.bitmapDataOffset = sizeof( bmp_t ) + cbPalBytes;
	bhdr.bitmapDataSize = bhdr.width * bhdr.height * pixel_size;
	bhdr.fileSize = bhdr.bitmapDataOffset + bhdr.bitmapDataSize;

	// constant
	bhdr.reserved0 = 0;
	bhdr.planes = 1;
	bhdr.bitsPerPixel = pixel_size * 8;
	bhdr.compression = 0;
	bhdr.hRes = bhdr.vRes = 0;
	bhdr.colors = ( pixel_size == 1 ) ? 256 : 0;
	bhdr.importantColors = 0;

	data = new byte[bhdr.fileSize];
	memcpy( data, &bhdr, sizeof( bhdr ));
	memset( data + bhdr.bitmapDataOffset, 0, bhdr.bitmapDataSize );
}

CBMP* CBMP::LoadFile( const char *filename )
{
	int length = 0;
	bmp_t *bmp = (bmp_t*)EngFuncs::COM_LoadFile( filename, &length );

	// cannot load
	if( !bmp )
		return NULL;

	// too small for BMP
	if( length < sizeof( bmp_t ))
		return NULL;

	// not a BMP
	if( bmp->id[0] != 'B' || bmp->id[1] != 'M' )
		return NULL;

	// bogus data
	if( !bmp->width || !bmp->height )
		return NULL;

	CBMP *ret = new CBMP( bmp->width, bmp->height );
	memcpy( ret->GetBitmap(), bmp, length );

	EngFuncs::COM_FreeFile( bmp );

	return ret;
}

#pragma pack( push, 1 )
struct rgbquad_t
{
	byte b;
	byte g;
	byte r;
	byte reserved;
};
#pragma pack( pop )

void CBMP::RemapLogo(int r, int g, int b)
{
	// palette is always right after header
	rgbquad_t *palette = (rgbquad_t*)(data + sizeof( bmp_t ));

	// no palette
	if( GetBitmapHdr()->bitsPerPixel > 8 )
		return;

	for( int i = 0; i < 256; i++ )
	{
		float t = (float)i/256.0f;

		palette[i].r = (byte)(r * t);
		palette[i].g = (byte)(g * t);
		palette[i].b = (byte)(b * t);
	}
}

void CBMP::Increase( uint w, uint h )
{
	bmp_t *hdr = GetBitmapHdr();
	bmp_t bhdr;

	const int pixel_size = 4; // create 32 bit image everytime

	memcpy( &bhdr, hdr, sizeof( bhdr ));

	bhdr.width  = (w + 3) & ~3;
	bhdr.height = h;
	bhdr.bitmapDataSize = bhdr.width * bhdr.height * pixel_size;
	bhdr.fileSize = bhdr.bitmapDataOffset + bhdr.bitmapDataSize;

	// I think that we need to only increase BMP size
	assert( bhdr.width >= hdr->width );
	assert( bhdr.height >= hdr->height );

	byte *newData = new byte[bhdr.fileSize];
	memcpy( newData, &bhdr, sizeof( bhdr ));
	memset( newData + bhdr.bitmapDataOffset, 0, sizeof( bhdr.bitmapDataSize ));

	// now copy texture
	byte *src = GetTextureData();
	byte *dst = newData + bhdr.bitmapDataOffset;

	// to keep texcoords still valid, we copy old texture through the end
	for( int y = 0; y < hdr->height; y++ )
	{
		byte *ydst = &dst[(y + (bhdr.height - hdr->height))* bhdr.width * 4];
		byte *ysrc = &src[y * hdr->width * 4];

		memcpy( ydst, ysrc, 4 * hdr->width );
	}

	delete []data;
	data = newData;
}

/*
============================
Con_UtfProcessChar

Ripped from engine.

Xash3D FWGS uses multibyte, converting it to current single-byte encoding
Converting to single-byte not necessary anymore, as UI uses custom font render which works with 32-bit chars
============================
*/
int Con_UtfProcessChar( int in )
{
#ifndef XASH_DISABLE_FWGS_EXTENSIONS
	static int m = -1, k = 0; //multibyte state
	static int uc = 0; //unicode char

	if( !in )
	{
		m = -1;
		k = 0;
		uc = 0;
		return 0;
	}

	// Get character length
	if(m == -1)
	{
		uc = 0;
		if( in >= 0xF8 )
		{
			return 0;
		}
		else if( in >= 0xF0 )
		{
			uc = in & 0x07;
			m = 3;
		}
		else if( in >= 0xE0 )
		{
			uc = in & 0x0F;
			m = 2;
		}
		else if( in >= 0xC0 )
		{
			uc = in & 0x1F;
			m = 1;
		}
		else if( in <= 0x7F )
		{
			return in; // ascii
		}
		// return 0 if we need more chars to decode one
		k = 0;
		return 0;
	}
	// get more chars
	else if( k <= m )
	{
		uc <<= 6;
		uc += in & 0x3F;
		k++;
	}
	if( in > 0xBF || m < 0 )
	{
		m = -1;
		return 0;
	}
	if( k == m )
	{
		k = m = -1;

		return uc;

		// not implemented yet
		// return '?';
	}
	return 0;
#else
	// remap in unicode
	if( in >= 0xC0 && in <= 0xFF )
	{
		return in - 0xC0 + 0x410;
	}
	return in;
#endif
}

/*
=================
Con_UtfMoveLeft

get position of previous printful char
=================
*/
int Con_UtfMoveLeft( char *str, int pos )
{
	int i, k = 0;
	// int j;
	Con_UtfProcessChar( 0 );
	if(pos == 1) return 0;
	for( i = 0; i < pos-1; i++ )
		if( Con_UtfProcessChar( (unsigned char)str[i] ) )
			k = i+1;
	Con_UtfProcessChar( 0 );
	return k;
}

/*
=================
Con_UtfMoveRight

get next of previous printful char
=================
*/
int Con_UtfMoveRight( char *str, int pos, int length )
{
	int i;
	Con_UtfProcessChar( 0 );
	for( i = pos; i <= length; i++ )
	{
		if( Con_UtfProcessChar( (unsigned char)str[i] ) )
			return i+1;
	}
	Con_UtfProcessChar( 0 );

	return pos + 1;
}


bool UI::Names::CheckIsNameValid(const char *name)
{
	if( !name || !*name )
		return false;

	// exclude some default names, that may be set from engine or just come with config files
	static struct
	{
		const char *name;
		bool substring;
	} prohibitedNames[] =
	{
	{ "default", false, },
	{ "unnamed", false, },
	{ "Player", false, },
	{ "<Warrior> Player", false, },
	{ "Shinji", false, },
	{ "CSDuragiCOM", true },
	{ "Nero Claudius", true }, // *purrrt* you found a secret area!
	};

	for( size_t i = 0; i < ARRAYSIZE( prohibitedNames ); i++ )
	{
		if( prohibitedNames[i].substring )
		{
			if( strstr( name, prohibitedNames[i].name ) )
			{
				return false;
			}
		}
		else
		{
			if( !stricmp( name, prohibitedNames[i].name ) )
			{
				return false;
			}
		}
	}

	return true;
}

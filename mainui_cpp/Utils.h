/*
utils.h - draw helper
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef UTILS_H
#define UTILS_H

//extern ui_enginefuncs_t g_engfuncs;
//extern ui_textfuncs_t g_textfuncs;

#include "enginecallback_menu.h"
#include "gameinfo.h"
#include "FontManager.h"

#define FILE_GLOBAL	static
#define DLL_GLOBAL

#define MAX_INFO_STRING	256	// engine limit

#define RAD2DEG( x )	((float)(x) * (float)(180.f / M_PI))
#define DEG2RAD( x )	((float)(x) * (float)(M_PI / 180.f))

//
// How did I ever live without ASSERT?
//
#ifdef _DEBUG
void DBG_AssertFunction( bool fExpr, const char* szExpr, const char* szFile, int szLine, const char* szMessage );
#define ASSERT( f )		DBG_AssertFunction( f, #f, __FILE__, __LINE__, NULL )
#define ASSERTSZ( f, sz )	DBG_AssertFunction( f, #f, __FILE__, __LINE__, sz )
#else
#define ASSERT( f )
#define ASSERTSZ( f, sz )
#endif

extern ui_globalvars_t		*gpGlobals;

// exports
extern int UI_VidInit( void );
extern void UI_Init( void );
extern void UI_Shutdown( void );
extern void UI_UpdateMenu( float flTime );
extern void UI_KeyEvent( int key, int down );
extern void UI_MouseMove( int x, int y );
extern void UI_SetActiveMenu( int fActive );
extern void UI_AddServerToList( netadr_t adr, const char *info );
extern void UI_GetCursorPos( int *pos_x, int *pos_y );
extern void UI_SetCursorPos( int pos_x, int pos_y );
extern void UI_ShowCursor( int show );
extern void UI_CharEvent( int key );
extern int UI_MouseInRect( void );
extern int UI_IsVisible( void );
extern int UI_CreditsActive( void );
extern void UI_FinalCredits( void );

#include "cvardef.h"

// ScreenHeight returns the height of the screen, in ppos.xels
#define ScreenHeight	((float)(gpGlobals->scrHeight))
// ScreenWidth returns the width of the screen, in ppos.xels
#define ScreenWidth		((float)(gpGlobals->scrWidth))

#define Alpha( x )	( ((x) & 0xFF000000 ) >> 24 )
#define Red( x )	( ((x) & 0xFF0000) >> 16 )
#define Green( x )	( ((x) & 0xFF00 ) >> 8 )
#define Blue( x )	( ((x) & 0xFF ) >> 0 )

inline unsigned int PackRGB( int r, int g, int b )
{
	return ((0xFF)<<24|(r)<<16|(g)<<8|(b));
}

inline unsigned int PackRGBA( int r, int g, int b, int a )
{
	return ((a)<<24|(r)<<16|(g)<<8|(b));
}

inline void UnpackRGB( int &r, int &g, int &b, unsigned int ulRGB )
{
	r = (ulRGB & 0xFF0000) >> 16;
	g = (ulRGB & 0xFF00) >> 8;
	b = (ulRGB & 0xFF) >> 0;
}

inline void UnpackRGBA( int &r, int &g, int &b, int &a, unsigned int ulRGBA )
{
	a = (ulRGBA & 0xFF000000) >> 24;
	r = (ulRGBA & 0xFF0000) >> 16;
	g = (ulRGBA & 0xFF00) >> 8;
	b = (ulRGBA & 0xFF) >> 0;
}

inline int PackAlpha( unsigned int ulRGB, unsigned int ulAlpha )
{
	return (ulRGB)|(ulAlpha<<24);
}

inline int UnpackAlpha( unsigned int ulRGBA )
{
	return ((ulRGBA & 0xFF000000) >> 24);	
}

inline float InterpVal( float from, float to, float frac )
{
	return from + (to - from) * frac;
}

inline int InterpColor( int from, int to, float frac )
{
	return PackRGBA(
		InterpVal( Red( from ), Red( to ), frac ),
		InterpVal( Green( from ), Green( to ), frac ),
		InterpVal( Blue( from ), Blue( to ), frac ),
		InterpVal( Alpha( from ), Alpha( to ), frac ) );
}



inline float RemapVal( float val, float A, float B, float C, float D)
{
	return C + (D - C) * (val - A) / (B - A);
}

extern void AddSpaces( char *s, int size, int buffersize = 99999 );
extern int ColorStrlen( const char *str );	// returns string length without color symbols
extern int ColorPrexfixCount( const char *str );
extern const unsigned int g_iColorTable[8];
extern void COM_FileBase( const char *in, char *out );		// ripped out from hlsdk 2.3
extern int UI_FadeAlpha( int starttime, int endtime );
extern void StringConcat( char *dst, const char *src, size_t size );	// strncat safe prototype
extern const char *Info_ValueForKey( const char *s, const char *key );
extern int KEY_GetKey( const char *binding );			// ripped out from engine
extern char *StringCopy( const char *input );			// copy string into new memory
extern int COM_CompareSaves( const void **a, const void **b );

extern void UI_LoadCustomStrings( void );
extern void UI_EnableTextInput( bool enable );

#ifdef __APPLE__
#define register
#endif // __APPLE__

inline size_t Q_strncpy( char *dst, const char *src, size_t size )
{
	register char	*d = dst;
	register const char	*s = src;
	register size_t	n = size;

	if( !dst || !src || !size )
		return 0;

	// copy as many bytes as will fit
	if( n != 0 && --n != 0 )
	{
		do
		{
			if(( *d++ = *s++ ) == 0 )
				break;
		} while( --n != 0 );
	}

	// not enough room in dst, add NULL and traverse rest of src
	if( n == 0 )
	{
		if( size != 0 )
			*d = '\0'; // NULL-terminate dst
		while( *s++ );
	}
	return ( s - src - 1 ); // count does not include NULL
}

#define ARRAYSIZE( x ) ( sizeof( x ) / sizeof( x[0] ) )

#ifdef register
#undef register
#endif // register

#define CS_SIZE			64	// size of one config string
#define CS_TIME			16	// size of time string

#define MAX_SCOREBOARDNAME	32 // engine and dlls allows only 32 chars

// color strings
#define ColorIndex( c )		((( c ) - '0' ) & 7 )
#define IsColorString( p )		( p && *( p ) == '^' && *(( p ) + 1) && *(( p ) + 1) >= '0' && *(( p ) + 1 ) <= '9' )

// stringize utilites
#define STR( x ) #x
#define STR2( x ) STR( x )

typedef unsigned short       word;

#pragma pack( push, 1 )
struct bmp_t
{
	char id[2];		// bmfh.bfType
	uint fileSize;		// bmfh.bfSize
	uint reserved0;	// bmfh.bfReserved1 + bmfh.bfReserved2
	uint bitmapDataOffset;	// bmfh.bfOffBits
	uint bitmapHeaderSize;	// bmih.biSize
	uint width;		// bmih.biWidth
	int	 height;		// bmih.biHeight
	word planes;		// bmih.biPlanes
	word bitsPerPixel;	// bmih.biBitCount
	uint compression;	// bmih.biCompression
	uint bitmapDataSize;	// bmih.biSizeImage
	uint hRes;		// bmih.biXPelsPerMeter
	uint vRes;		// bmih.biYPelsPerMeter
	uint colors;		// bmih.biClrUsed
	uint importantColors;	// bmih.biClrImportant
};
#pragma pack( pop )

class CBMP
{
public:
	CBMP( uint w, uint h );
	~CBMP() { if( data ) delete []data; }

	void Increase(uint newW, uint newH);

	static CBMP* LoadFile( const char *filename );

	void RemapLogo( int r, int g, int b );

	inline byte *GetBitmap()
	{
		return data;
	}

	inline bmp_t *GetBitmapHdr()
	{
		return (bmp_t*)data;
	}

	inline byte *GetTextureData()
	{
		return data + GetBitmapHdr()->bitmapDataOffset;
	}

private:
	byte    *data;
};

namespace UI
{

namespace Key
{
inline bool IsEscape( int key )
{
	return ( key == K_ESCAPE
#ifndef XASH_DISABLE_FWGS_EXTENSIONS
	|| key == K_B_BUTTON
#endif // XASH_DISABLE_FWGS_EXTENSIONS
	);
}

inline bool IsEnter( int key )
{
	return ( key == K_ENTER
#ifndef XASH_DISABLE_FWGS_EXTENSIONS
	|| key == K_A_BUTTON
#endif // XASH_DISABLE_FWGS_EXTENSIONS
	);
}
}

namespace Names
{
bool CheckIsNameValid( const char *name );
}
}
int Con_UtfProcessChar(int in );
int Con_UtfMoveLeft( char *str, int pos );
int Con_UtfMoveRight( char *str, int pos, int length );

#endif//UTILS_H

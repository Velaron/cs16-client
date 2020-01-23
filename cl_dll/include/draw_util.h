/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
#pragma once
#ifndef DRAW_UTIL_H
#define DRAW_UTIL_H
// Drawing primitives

#include "cl_dll.h"
#include "cl_util.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4

float GetScale();

class DrawUtils
{
public:
	static int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b );
	static int DrawHudNumber2( int x, int y, bool DrawZero, int iDigits, int iNumber, int r, int g, int b);
	static int DrawHudNumber2( int x, int y, int iNumber, int r, int g, int b);
	static int DrawHudString(int x, int y, int iMaxX, const char *szString,
							 int r, int g, int b, bool drawing = false );
	static int DrawHudStringReverse( int x, int y, int iMinX, const char *szString,
								 int r, int g, int b, bool drawing = false );

	static inline int DrawHudNumberString( int x, int y, int iMinX, int iNumber,
								int r, int g, int b )
	{
		char szString[16];
		snprintf( szString, sizeof(szString), "%d", iNumber );
		return DrawHudStringReverse( x, y, iMinX, szString, r, g, b );
	}

	static int HudStringLen(const char *szIt);

	// legacy shit came with Valve
	static inline int GetNumWidth(int iNumber, int iFlags)
	{
		if ( iFlags & ( DHN_3DIGITS ) )
			return 3;

		if ( iFlags & ( DHN_2DIGITS ) )
			return 2;

		if ( iNumber <= 0 )
			return iFlags & DHN_DRAWZERO ? 1 : 0;

		if ( iNumber < 10 )
			return 1;

		if ( iNumber < 100 )
			return 2;

		return 3;
	}

	static inline int DrawConsoleString(int x, int y, const char *string)
	{
		int ret  = DrawHudString( x, y, 9999, string, color[0], color[1], color[2] );
		color[0] = color[1] = color[2] = 255;
		return ret;
	}

	static inline void SetConsoleTextColor( float r, float g, float b )
	{
		color[0] = bound( 0, r * 255.0f, 255 );
		color[1] = bound( 0, g * 255.0f, 255 );
		color[2] = bound( 0, b * 255.0f, 255 );
	}

	static inline void SetConsoleTextColor( unsigned char r, unsigned char g, unsigned char b )
	{
		color[0] = r;
		color[1] = g;
		color[2] = b;
	}

	static inline int ConsoleStringLen(  const char *szIt )
	{
		return HudStringLen( szIt );
	}

	static inline void ConsoleStringSize( const char *szIt, int *width, int *height )
	{
		g_pMenu->GetTextSize( QM_SMALLFONT, szIt, width, height );
	}

	static inline unsigned int PackRGB( int r, int g, int b )
	{
		return ((0xFF)<<24|(r)<<16|(g)<<8|(b));
	}

	static inline unsigned int PackRGBA( int r, int g, int b, int a )
	{
		return ((a)<<24|(r)<<16|(g)<<8|(b));
	}

	static inline int TextMessageDrawChar( int x, int y, int number, int r, int g, int b )
	{
		float hudscale = GetScale();
		return g_pMenu->DrawCharacter( QM_SMALLFONT, number, x * hudscale, y * hudscale, UI_SMALL_CHAR_HEIGHT * hudscale, PackRGB( r, g, b ), true ) / hudscale;
	}

	static inline int HudFontHeight( )
	{
		//float hudscale = GetScale();
		return g_pMenu->GetFontTall( QM_SMALLFONT );
	}

	static inline void UnpackRGB( int &r, int &g, int &b, const unsigned long ulRGB )
	{
		r = (ulRGB & 0xFF0000) >>16;
		g = (ulRGB & 0xFF00) >> 8;
		b = ulRGB & 0xFF;
	}

	static inline void ScaleColors( int &r, int &g, int &b, const int a )
	{
		r *= a / 255.0f;
		g *= a / 255.0f;
		b *= a / 255.0f;
	}

	static inline void DrawRectangle( int x, int y, int wide, int tall,
						   int r = 0, int g = 0, int b = 0, int a = 153,
						   bool drawStroke = true )
	{
		FillRGBABlend( x, y, wide, tall, r, g, b, a );
		if ( drawStroke )
		{
			// TODO: remove this hardcoded hardcore
			FillRGBA( x + 1,        y,            wide - 1, 1,        255, 140, 0, 255 );
			FillRGBA( x,            y,            1,        tall - 1, 255, 140, 0, 255 );
			FillRGBA( x + wide - 1, y + 1,        1,        tall - 1, 255, 140, 0, 255 );
			FillRGBA( x,            y + tall - 1, wide - 1, 1,        255, 140, 0, 255 );
		}
	}

	static void Draw2DQuad( float x1, float y1, float x2, float y2 );
	static void DrawStretchPic( float x, float y, float w, float h,
								float s1 = 0, float t1 = 0, float s2 = 1, float t2 = 1);
private:
	// console string color
	static int color[3];
};

#endif // DRAW_UTIL_H

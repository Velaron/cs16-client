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

class DrawUtils
{
public:
	static void DrawRectangle( int x, int y, int wide, int tall,
						   int r = 0, int g = 0, int b = 0, int a = 153,
						   bool drawStroke = true );

	static int DrawHudNumber(int x, int y, int iFlags, int iNumber,
						 int r, int g, int b );

	static int DrawHudNumber2( int x, int y, bool DrawZero, int iDigits, int iNumber,
						   int r, int g, int b);

	static int DrawHudNumber2( int x, int y, int iNumber,
						   int r, int g, int b);

	static int DrawHudString(int x, int y, int iMaxX, const char *szString,
						 int r, int g, int b, bool drawing = false );

	static int DrawHudStringReverse( int xpos, int ypos, int iMinX, const char *szString,
								 int r, int g, int b, bool drawing = false );

	static int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber,
								int r, int g, int b );

	static int HudStringLen( char *szIt );

	static int GetNumWidth(int iNumber, int iFlags);

	static int DrawConsoleString(int x, int y, const char *string);

	static void SetConsoleTextColor( float r, float g, float b );

	static void SetConsoleTextColor( unsigned char r, unsigned char g, unsigned char b );

	static int ConsoleStringLen(  const char *szIt );

	static void ConsoleStringSize( const char *szIt, int *width, int *height );

	static int TextMessageDrawChar( int x, int y, int number, int r, int g, int b );

	static inline void UnpackRGB(int &r, int &g, int &b, unsigned long ulRGB)
	{
		r = (ulRGB & 0xFF0000) >>16;
		g = (ulRGB & 0xFF00) >> 8;
		b = ulRGB & 0xFF;
	}

	static inline void ScaleColors( int &r, int &g, int &b, int a )
	{
		float x = (float)a / 255;
		r = (int)(r * x);
		g = (int)(g * x);
		b = (int)(b * x);
	}

	static void Draw2DQuad( float x1, float y1, float x2, float y2 );

private:
	// console string color
	static float color[3];
};

#endif // DRAW_UTIL_H

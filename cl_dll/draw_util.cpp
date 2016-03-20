/*
draw_util.cpp - Draw Utils
Copyright (C) 2016 a1batross

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.
*/

#include "draw_util.h"
#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include <string.h>

extern cvar_t *hud_textmode;
float DrawUtils::color[3];

int DrawUtils::DrawHudString( int xpos, int ypos, int iMaxX, const char *str, int r, int g, int b, bool drawing )
{
	char *szIt = (char *)str;
	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		int next = xpos + gHUD.GetCharWidth((unsigned char)*szIt); // variable-width fonts look cool
		if ( next > iMaxX )
			return xpos;

		if ( *szIt == '\\' && *( szIt + 1 ) != '\n' && *( szIt + 1 ) != 0 )
		{
			// an escape character

			switch ( *( ++szIt ) )
			{
			case 'y':
				UnpackRGB( r, g, b, RGB_YELLOWISH );
				continue;
			case 'w':
				r = g = b = 255;
				continue;
			case 'd':
				continue;
			case 'R':
				//if( drawing ) return xpos;
				//return DrawHudStringReverse( iMaxX, ypos, first_xpos, szIt, r, g, b, true ); // set 'drawing' to true, to stop when '\R' is catched
				xpos = iMaxX - gHUD.GetCharWidth('M') * 10;
				++szIt;
			}
		}

		xpos += TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
	}

	return xpos;
}

int DrawUtils::HudStringLen( const char *szIt )
{
	int l = 0;
	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		l += gHUD.m_scrinfo.charWidths[(unsigned char)*szIt]; // variable-width fonts look cool
	}
	return l;
}

int DrawUtils::DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b )
{
	char szString[32];
	snprintf( szString, 32, "%d", iNumber );
	return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b );
}

int DrawUtils::DrawHudStringReverse( int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b, bool drawing )
{
	// iterate throug the string in reverse
	for ( signed int i = strlen( szString ); i >= 0; i-- )
	{
		int next = xpos - gHUD.GetCharWidth((unsigned char)szString[i]); // variable-width fonts look cool
		if ( next < iMinX )
			return xpos;
		xpos = next;

		if ( i > 1 && szString[i - 1] == '\\' )
		{
			// an escape character

			switch ( szString[i] )
			{
			case 'y':
				UnpackRGB( r, g, b, RGB_YELLOWISH );
				break;
			case 'w':
				r = g = b = 255;
				break;
			case 'R':
			//if( drawing ) return xpos;
			//else return DrawHudString( iMinX, ypos, first_xpos, &szString[i - 1], r, g, b, true ); // set 'drawing' to true, to stop when '\R' is catched
			//xpos = iMinX + gHUD.m_scrinfo.charWidths['M'] * i ;
			case 'd':
				break;
			}
			continue;
		}

		TextMessageDrawChar( xpos, ypos, szString[i], r, g, b );
	}

	return xpos;
}

int DrawUtils::DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b )
{
	int iWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
	int k;

	if ( iNumber > 0 )
	{
		// SPR_Draw 100's
		if ( iNumber >= 100 )
		{
			k = iNumber / 100;
			SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if ( iFlags & ( DHN_3DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw 10's
		if ( iNumber >= 10 )
		{
			k = ( iNumber % 100 ) / 10;
			SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
			x += iWidth;
		}
		else if ( iFlags & ( DHN_3DIGITS | DHN_2DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
		x += iWidth;
	}
	else if ( iFlags & DHN_DRAWZERO )
	{
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 ), r, g, b );

		// SPR_Draw 100's
		if ( iFlags & ( DHN_3DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		if ( iFlags & ( DHN_3DIGITS | DHN_2DIGITS ) )
		{
			//SPR_DrawAdditive( 0, x, y, &rc );
			x += iWidth;
		}

		// SPR_Draw ones

		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ) );
		x += iWidth;
	}

	return x;
}

int DrawUtils::GetNumWidth( int iNumber, int iFlags )
{
	if ( iFlags & ( DHN_3DIGITS ) )
		return 3;

	if ( iFlags & ( DHN_2DIGITS ) )
		return 2;

	if ( iNumber <= 0 )
	{
		if ( iFlags & ( DHN_DRAWZERO ) )
			return 1;
		else
			return 0;
	}

	if ( iNumber < 10 )
		return 1;

	if ( iNumber < 100 )
		return 2;

	return 3;
}

void DrawUtils::DrawRectangle( int x, int y, int wide, int tall, int r, int g, int b, int a, bool drawStroke )
{
	FillRGBABlend( x, y, wide, tall, r, g, b, a );
	if ( drawStroke )
	{
		// TODO: remove this hardcoded hardcore
		FillRGBA( x + 1, y, wide - 1, 1, 255, 140, 0, 255 );
		FillRGBA( x, y, 1, tall - 1, 255, 140, 0, 255 );
		FillRGBA( x + wide - 1, y + 1, 1, tall - 1, 255, 140, 0, 255 );
		FillRGBA( x, y + tall - 1, wide - 1, 1, 255, 140, 0, 255 );
	}
}

int DrawUtils::DrawHudNumber2( int x, int y, bool DrawZero, int iDigits, int iNumber, int r, int g, int b )
{
	int iWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;
	x += ( iDigits - 1 ) * iWidth;

	int ResX = x + iWidth;
	do
	{
		int k = iNumber % 10;
		iNumber /= 10;
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
		x -= iWidth;
		iDigits--;
	} while ( iNumber > 0 || ( iDigits > 0 && DrawZero ) );

	return ResX;
}

int DrawUtils::DrawHudNumber2( int x, int y, int iNumber, int r, int g, int b )
{
	int iWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left;

	int iDigits = 0;
	int temp    = iNumber;
	do
	{
		iDigits++;
		temp /= 10;
	} while ( temp > 0 );

	x += ( iDigits - 1 ) * iWidth;

	int ResX = x + iWidth;
	do
	{
		int k = iNumber % 10;
		iNumber /= 10;
		SPR_Set( gHUD.GetSprite( gHUD.m_HUD_number_0 + k ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( gHUD.m_HUD_number_0 + k ) );
		x -= iWidth;
	} while ( iNumber > 0 );

	return ResX;
}

int DrawUtils::DrawConsoleString( int x, int y, const char *string )
{
	if ( hud_textmode->value )
	{
		int ret  = DrawHudString( x, y, 9999, (char *)string, color[0] * 255, color[1] * 255, color[2] * 255 );
		color[0] = color[1] = color[2] = 1.0f;
		return ret;
	}
	else
		return gEngfuncs.pfnDrawConsoleString( x, y, (char *)string );
}

void DrawUtils::SetConsoleTextColor( float r, float g, float b )
{
	if ( hud_textmode->value )
		color[0] = r, color[1] = g, color[2] = b;
	else
		gEngfuncs.pfnDrawSetTextColor( r, g, b );
}

void DrawUtils::SetConsoleTextColor( unsigned char r, unsigned char g, unsigned char b )
{
	if ( hud_textmode->value )
		color[0] = r / 255.0f, color[1] = g / 255.0f, color[2] = b / 255.0f;
	else
		gEngfuncs.pfnDrawSetTextColor( r / 255.0f, g / 255.0f, b / 255.0f );
}

void DrawUtils::ConsoleStringSize( const char *string, int *width, int *height )
{
	if ( hud_textmode->value )
		*height = 13, *width = HudStringLen( (char *)string );
	else
		gEngfuncs.pfnDrawConsoleStringLen( string, width, height );
}

int DrawUtils::ConsoleStringLen( const char *string )
{
	int _width, _height;
	if ( hud_textmode->value )
	{
		return HudStringLen( (char *)string );
	}
	else
	{
		ConsoleStringSize( string, &_width, &_height );
		return _width;
	}
}

int DrawUtils::TextMessageDrawChar( int x, int y, int number, int r, int g, int b )
{
	return gEngfuncs.pfnDrawCharacter( x, y, number, r, g, b );
}

void DrawUtils::Draw2DQuad( float x1, float y1, float x2, float y2 )
{
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( x1, y1, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( x1, y2, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( x2, y2, 0 );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( x2, y1, 0 );

	gEngfuncs.pTriAPI->End( );
}

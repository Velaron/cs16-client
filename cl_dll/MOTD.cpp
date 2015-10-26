/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// MOTD.cpp
//
// for displaying a server-sent message of the day
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "kbutton.h"
#include "triangleapi.h"
#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE( m_MOTD, MOTD );

float fStartTime;
int iMaxLength;
extern float scale;
int CHudMOTD :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( MOTD );

	m_bShow = false;

	m_iFlags &= ~HUD_ACTIVE;  // start out inactive
	m_szMOTD[0] = 0;

	return 1;
}

int CHudMOTD :: VidInit( void )
{
	// Load sprites here
	scale = gEngfuncs.pfnGetCvarFloat("hud_scale");
	if(scale <= 0.01) scale = 1;
	return 1;
}

void CHudMOTD :: Reset( void )
{
	m_iFlags &= ~HUD_ACTIVE;  // start out inactive
	m_szMOTD[0] = 0;
	m_iLines = 0;
	m_bShow = 0;
}
void DarkSquare( int x, int y, int wide, int tall )
{
	gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	gEngfuncs.pTriAPI->Color4f(0.0, 0.0, 0.0, 0.6);
	gEngfuncs.pTriAPI->Vertex3f(x * scale, (y+tall)*scale, 0);
	gEngfuncs.pTriAPI->Vertex3f(x * scale, y*scale, 0);
	gEngfuncs.pTriAPI->Vertex3f((x + wide)*scale, y*scale, 0);
	gEngfuncs.pTriAPI->Vertex3f((x + wide)*scale, (y+tall)*scale, 0);
	gEngfuncs.pTriAPI->End();
	FillRGBA( x+1, y, wide-1, 1, 255, 140, 0, 255 );
	FillRGBA( x, y, 1, tall-1, 255, 140, 0, 255 );
	FillRGBA( x+wide-1, y+1, 1, tall-1, 255, 140, 0, 255 );
	FillRGBA( x, y+tall-1, wide-1, 1, 255, 140, 0, 255 );
}

#define LINE_HEIGHT  13
#define ROW_GAP  13
#define ROW_RANGE_MIN 15
#define ROW_RANGE_MAX ( ScreenHeight - 50 )
int CHudMOTD :: Draw( float fTime )
{
	// find the top of where the MOTD should be drawn,  so the whole thing is centered in the screen
	int ypos = ROW_RANGE_MIN + 3 + scroll; // shift it up slightly
	char *ch = m_szMOTD;
	int xpos = (ScreenWidth - gHUD.m_scrinfo.charWidths[ 'M' ] * iMaxLength) / 2;
	if( xpos < 30 ) xpos = 30;
	int xmax = xpos + gHUD.m_scrinfo.charWidths[ 'M' ] * iMaxLength;
	if( xmax > ScreenWidth - 30 ) xmax = ScreenWidth - 30;
	DarkSquare(xpos-5, ROW_RANGE_MIN-5, xmax - xpos+10, ROW_RANGE_MAX + 23);
	while ( *ch )
	{
		int line_length = 0;  // count the length of the current line
		for ( char *next_line = ch; *next_line != '\n' && *next_line != 0; next_line++ )
			line_length += gHUD.m_scrinfo.charWidths[ *next_line ];
		char *top = next_line;
		if ( *top == '\n' )
			*top = 0;
		else
			top = NULL;

		// find where to start drawing the line
		if( ypos > ROW_RANGE_MIN && ypos < ROW_RANGE_MAX )
			gHUD.DrawHudString( xpos, ypos, xmax, ch, 255, 180, 0 );

		ypos += LINE_HEIGHT;

		if ( top )  // restore 
			*top = '\n';
		ch = next_line;
		if ( *ch == '\n' )
			ch++;

		if ( ypos > (ScreenHeight - 20) )
			break;  // don't let it draw too low
	}
	
	return 1;
}

int CHudMOTD :: MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf )
{
	if ( m_iFlags & HUD_ACTIVE )
	{
		Reset(); // clear the current MOTD in prep for this one
	}

	BEGIN_READ( pbuf, iSize );

	int is_finished = READ_BYTE();
	strcat( m_szMOTD, READ_STRING() );

	if ( is_finished )
	{
		int length = 0;
		
		iMaxLength = 0;
		m_iFlags |= HUD_ACTIVE;


		for ( char *sz = m_szMOTD; *sz != 0; sz++ )  // count the number of lines in the MOTD
		{
			if ( *sz == '\n' )
			{
				m_iLines++;
				if( length > iMaxLength )
				{
					iMaxLength = length;
					length = 0;
				}
			}
			length++;
		}
		m_bShow = true;
	}

	return 1;
}

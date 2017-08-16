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


// ui_menu.c -- main menu interface
#define OEMRESOURCE		// for OCR_* cursor junk



#include "extdll_menu.h"
#include "BaseMenu.h"
#include "PicButton.h"
#include "keydefs.h"
#include "menufont.h"	// built-in menu font
#include "Utils.h"
#include "BtnsBMPTable.h"
#include "YesNoMessageBox.h"
#include "ConnectionProgress.h"
#include "BackgroundBitmap.h"
#include "con_nprint.h"

cvar_t		*ui_precache;
cvar_t		*ui_showmodels;
cvar_t		*ui_show_window_stack;

uiStatic_t	uiStatic;

const char	*uiSoundIn          = "media/launch_upmenu1.wav";
const char	*uiSoundOut         = "media/launch_dnmenu1.wav";
const char	*uiSoundLaunch      = "media/launch_select2.wav";
const char	*uiSoundGlow        = "media/launch_glow1.wav";
const char	*uiSoundBuzz        = "media/launch_deny2.wav";
const char	*uiSoundKey         = "media/launch_select1.wav";
const char	*uiSoundRemoveKey   = "media/launch_deny1.wav";
const char	*uiSoundMove        = "";		// Xash3D not use movesound
const char	*uiSoundNull        = "";

int		uiColorHelp         = 0xFFFFFFFF;	// 255, 255, 255, 255	// hint letters color
int		uiPromptBgColor     = 0x80404040;	// 64,  64,  64,  255	// dialog background color
int		uiPromptTextColor   = 0xFFF0B418;	// 255, 160,  0,  255	// dialog or button letters color
int		uiPromptFocusColor  = 0xFFFFFF00;	// 255, 255,  0,  255	// dialog or button focus letters color
int		uiInputTextColor    = 0xFFC0C0C0;	// 192, 192, 192, 255
int		uiInputBgColor      = 0x80404040;	// 64,  64,  64,  255	// field, scrollist, checkbox background color
int		uiInputFgColor      = 0xFF555555;	// 85,  85,  85,  255	// field, scrollist, checkbox foreground color
int		uiColorWhite        = 0xFFFFFFFF;	// 255, 255, 255, 255	// useful for bitmaps
int		uiColorDkGrey       = 0x80404040;	// 64,  64,  64,  255	// shadow and grayed items
int		uiColorBlack        = 0x80000000;	//  0,   0,   0,  255	// some controls background
int		uiColorConsole      = 0xFFF0B418;	// just for reference

// color presets (this is nasty hack to allow color presets to part of text)
const unsigned int g_iColorTable[8] =
{
0xFF000000, // black
0xFFFF0000, // red
0xFF00FF00, // green
0xFFFFFF00, // yellow
0xFF0000FF, // blue
0xFF00FFFF, // cyan
0xFFF0B418, // dialog or button letters color
0xFFFFFFFF, // white
};

/*
=================
UI_ScaleCoords

Any parameter can be NULL if you don't want it
=================
*/
void UI_ScaleCoords( int *x, int *y, int *w, int *h )
{
	if( x ) *x *= uiStatic.scaleX;
	if( y ) *y *= uiStatic.scaleY;
	if( w ) *w *= uiStatic.scaleX;
	if( h ) *h *= uiStatic.scaleY;
}

void UI_ScaleCoords( int &x, int &y )
{
	x *= uiStatic.scaleX;
	y *= uiStatic.scaleY;
}

void UI_ScaleCoords( int &x, int &y, int &w, int &h )
{
	UI_ScaleCoords( x, y );
	UI_ScaleCoords( w, h );
}

Point Point::Scale()
{
	return Point( x * uiStatic.scaleX, y * uiStatic.scaleY );
}

Size Size::Scale()
{
	return Size( w * uiStatic.scaleX, h * uiStatic.scaleY );
}

/*
=================
UI_CursorInRect
=================
*/
bool UI_CursorInRect( int x, int y, int w, int h )
{
	if( uiStatic.cursorX < x || uiStatic.cursorX > x + w )
		return false;
	if( uiStatic.cursorY < y || uiStatic.cursorY > y + h )
		return false;
	return true;
}

/*
=================
UI_DrawPic
=================
*/
void UI_DrawPic( int x, int y, int width, int height, const int color, const char *pic )
{
	HIMAGE hPic = EngFuncs::PIC_Load( pic );
	if (!hPic)
		return;

	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	EngFuncs::PIC_Set( hPic, r, g, b, a );
	EngFuncs::PIC_Draw( x, y, width, height );
}

/*
=================
UI_DrawPicAdditive
=================
*/
void UI_DrawPicAdditive( int x, int y, int width, int height, const int color, const char *pic )
{
	HIMAGE hPic = EngFuncs::PIC_Load( pic );
	if (!hPic)
		return;

	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	EngFuncs::PIC_Set( hPic, r, g, b, a );
	EngFuncs::PIC_DrawAdditive( x, y, width, height );
}

/*
=================
UI_DrawPicAdditive
=================
*/
void UI_DrawPicTrans( int x, int y, int width, int height, const int color, const char *pic )
{
	HIMAGE hPic = EngFuncs::PIC_Load( pic );
	if (!hPic)
		return;

	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	EngFuncs::PIC_Set( hPic, r, g, b, a );
	EngFuncs::PIC_DrawTrans( x, y, width, height );
}


/*
=================
UI_DrawPicAdditive
=================
*/
void UI_DrawPicHoles( int x, int y, int width, int height, const int color, const char *pic )
{
	HIMAGE hPic = EngFuncs::PIC_Load( pic );
	if (!hPic)
		return;

	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	EngFuncs::PIC_Set( hPic, r, g, b, a );
	EngFuncs::PIC_DrawHoles( x, y, width, height );
}


/*
=================
UI_FillRect
=================
*/
void UI_FillRect( int x, int y, int width, int height, const int color )
{
	int r, g, b, a;
	UnpackRGBA( r, g, b, a, color );

	EngFuncs::FillRGBA( x, y, width, height, r, g, b, a );
}

/*
=================
UI_DrawRectangleExt
=================
*/
void UI_DrawRectangleExt( int in_x, int in_y, int in_w, int in_h, const int color, int outlineWidth )
{
	int	x, y, w, h;

	x = in_x - outlineWidth;
	y = in_y - outlineWidth;
	w = outlineWidth;
	h = in_h + outlineWidth + outlineWidth;

	// draw left
	UI_FillRect( x, y, w, h, color );

	x = in_x + in_w;
	y = in_y - outlineWidth;
	w = outlineWidth;
	h = in_h + outlineWidth + outlineWidth;

	// draw right
	UI_FillRect( x, y, w, h, color );

	x = in_x;
	y = in_y - outlineWidth;
	w = in_w;
	h = outlineWidth;

	// draw top
	UI_FillRect( x, y, w, h, color );

	// draw bottom
	x = in_x;
	y = in_y + in_h;
	w = in_w;
	h = outlineWidth;

	UI_FillRect( x, y, w, h, color );
}

/*
=================
UI_DrawCharacter
=================
*/
void UI_DrawCharacter( int x, int y, int width, int height, int ch, int ulRGBA, HIMAGE hFont )
{
#if 1
	EngFuncs::DrawCharacter( x, y, width, height, ch, ulRGBA, hFont );
#else
	// TODO: Custom font rendering!
#endif
}



/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, int w, int h, const char *string, const int color, int forceColor, int charW, int charH, ETextAlignment justify, bool shadow, EVertAlignment vertAlignment )
{
	int	modulate, shadowModulate;
	char	line[1024], *l;
	int	xx = 0, yy, ofsX = 0, ofsY = 0, len, ch;

	if( !string || !string[0] )
		return;

#if 0	// g-cont. disabled 29/06/2011
	// this code do a bad things with prompt dialogues
	// vertically centered
	if( !strchr( string, '\n' ))
		y = y + (( h - charH ) / 2 );
#endif

	if( shadow )
	{
		shadowModulate = PackAlpha( uiColorBlack, UnpackAlpha( color ));

		ofsX = charW / 8;
		ofsY = charH / 8;
	}

	modulate = color;

	switch( vertAlignment )
	{
	case QM_TOP: yy = y; break;
	case QM_VCENTER: yy = y + (h - charH)/2; break;
	case QM_BOTTOM: yy = y + h - charH; break;
	}

	yy = y;
	while( *string )
	{
		// get a line of text
		len = 0;
		while( *string )
		{
			if( *string == '\n' )
			{
				string++;
				break;
			}

			line[len++] = *string++;
			if( len == sizeof( line ) - 1 )
				break;
		}
		line[len] = 0;

		// align the text as appropriate
		switch( justify )
		{
		case QM_LEFT: xx = x; break;
		case QM_CENTER: xx = x + ((w - (ColorStrlen( line ) * charW )) / 2); break;
		case QM_RIGHT: xx = x + (w - (ColorStrlen( line ) * charW )); break;
		}

		// draw it
		l = line;
		while( *l )
		{
			if( IsColorString( l ))
			{
				int colorNum = ColorIndex( *(l+1) );

				if( colorNum == 7 && color != 0 )
				{
					modulate = color;
				}
				else if( !forceColor )
				{
					modulate = PackAlpha( g_iColorTable[colorNum], UnpackAlpha( color ));
				}

				l += 2;
				continue;
			}

			ch = *l++;
			ch &= 255;
#if 0
#ifdef _WIN32
			// fpos.x for letter �
			if( ch == 0xB8 ) ch = (byte)'�';
			if( ch == 0xA8 ) ch = (byte)'�';
#endif
#endif
			ch = EngFuncs::UtfProcessChar( (unsigned char) ch );
			if( !ch )
				continue;
			if( ch != ' ' )
			{
				if( shadow ) UI_DrawCharacter( xx + ofsX, yy + ofsY, charW, charH, ch, shadowModulate, uiStatic.hFont );
				UI_DrawCharacter( xx, yy, charW, charH, ch, modulate, uiStatic.hFont );
			}
			xx += charW;
		}
          	yy += charH;
	}
}

#ifdef XASH_DISABLE_FWGS_EXTENSIONS
#include <windows.h> // DrawMouseCursor
#endif

/*
=================
UI_DrawMouseCursor
=================
*/
void UI_DrawMouseCursor( void )
{
#ifdef XASH_DISABLE_FWGS_EXTENSIONS
	CMenuBaseItem	*item;
	HICON		hCursor = NULL;
	int		i;

	if( uiStatic.hideCursor ) return;

	for( i = 0; i < uiStatic.menuActive->m_numItems; i++ )
	{
		item = (CMenuBaseItem *)uiStatic.menuActive->m_pItems[i];

		if ( !item->IsVisible() )
			continue;

		if( !(item->iFlags & QMF_HASMOUSEFOCUS) )
			continue;

		if ( item->iFlags & QMF_GRAYED )
		{
			hCursor = (HICON)LoadCursor( NULL, (LPCTSTR)OCR_NO );
		}
		else
		{
			//if( item->type == QMTYPE_FIELD )
			//	hCursor = (HICON)LoadCursor( NULL, (LPCTSTR)OCR_IBEAM );
		}
		break;
	}

	if( !hCursor ) hCursor = (HICON)LoadCursor( NULL, (LPCTSTR)OCR_NORMAL );

	EngFuncs::SetCursor( hCursor );
#endif
	//TODO: Unified LoadCursor interface extension
}

const char *COM_ExtractExtension( const char *s )
{
	int len = strlen( s );

	for( int i = len; i >= 0; i-- )
		if( s[i] == '.' )
			return s + i + 1;
	return s;
}

// =====================================================================








/*
=================
UI_RefreshServerList
=================
*/
void UI_RefreshServerList( void )
{
	uiStatic.numServers = 0;
	uiStatic.serversRefreshTime = gpGlobals->time;

	memset( uiStatic.serverAddresses, 0, sizeof( uiStatic.serverAddresses ));
	memset( uiStatic.serverNames, 0, sizeof( uiStatic.serverNames ));
	memset( uiStatic.serverPings, 0, sizeof( uiStatic.serverPings ));

	EngFuncs::ClientCmd( FALSE, "localservers\n" );
}

/*
=================
UI_RefreshInternetServerList
=================
*/
void UI_RefreshInternetServerList( void )
{
	uiStatic.numServers = 0;
	uiStatic.serversRefreshTime = gpGlobals->time;

	memset( uiStatic.serverAddresses, 0, sizeof( uiStatic.serverAddresses ));
	memset( uiStatic.serverNames, 0, sizeof( uiStatic.serverNames ));
	memset( uiStatic.serverPings, 0, sizeof( uiStatic.serverPings ));

	EngFuncs::ClientCmd( FALSE, "internetservers\n" );
}

/*
=================
UI_StartBackGroundMap
=================
*/
bool UI_StartBackGroundMap( void )
{
	static bool	first = TRUE;

	if( !first ) return FALSE;

	first = FALSE;

	// some map is already running
	if( !uiStatic.bgmapcount || CL_IsActive() || gpGlobals->demoplayback )
		return FALSE;

	int bgmapid = EngFuncs::RandomLong( 0, uiStatic.bgmapcount - 1 );

	char cmd[128];
	sprintf( cmd, "maps/%s.bsp", uiStatic.bgmaps[bgmapid] );
	if( !EngFuncs::FileExists( cmd, TRUE )) return FALSE;

	sprintf( cmd, "map_background %s\n", uiStatic.bgmaps[bgmapid] );
	EngFuncs::ClientCmd( FALSE, cmd );

	return TRUE;
}

// =====================================================================

/*
=================
UI_CloseMenu
=================
*/
void UI_CloseMenu( void )
{
	uiStatic.menuActive = NULL;
	uiStatic.menuDepth = 0;
	uiStatic.rootPosition = 0;
	uiStatic.visible = false;

	// clearing serverlist
	uiStatic.numServers = 0;
	memset( uiStatic.serverAddresses, 0, sizeof( uiStatic.serverAddresses ));
	memset( uiStatic.serverNames, 0, sizeof( uiStatic.serverNames ));

	CMenuPicButton::ClearButtonStack();

//	EngFuncs::KEY_ClearStates ();
	EngFuncs::KEY_SetDest( KEY_GAME );
}



// =====================================================================

/*
=================
UI_UpdateMenu
=================
*/
void UI_UpdateMenu( float flTime )
{
	if( !uiStatic.initialized )
		return;

	UI_DrawFinalCredits ();

	if( !uiStatic.visible )
		return;

	if( !uiStatic.menuActive )
		return;

	uiStatic.realTime = flTime * 1000;
	uiStatic.framecount++;

	if( !EngFuncs::ClientInGame() && EngFuncs::GetCvarFloat( "cl_background" ))
		return;	// don't draw menu while level is loading

	if( uiStatic.firstDraw )
	{
		// we loading background so skip SCR_Update
		if( UI_StartBackGroundMap( )) return;

		uiStatic.menuActive->Activate();
	}

	// find last root element
	int i;
#if 0
	for( i = uiStatic.menuDepth-1; i >= 0; i-- )
	{
		if( uiStatic.menuStack[i]->IsRoot() )
			break;
	}
#endif


	for( i = uiStatic.rootPosition ; i < uiStatic.menuDepth; i++ )
	{
		CMenuBaseWindow *window = uiStatic.menuStack[i];

		if( window->bInTransition )
		{
			if( window->DrawAnimation( CMenuBaseWindow::ANIM_IN ) )
				window->bInTransition = false;
		}

		// transition is ended, so just draw
		if( !window->bInTransition )
		{
			window->Draw();
		}
	}

	if( uiStatic.prevMenu && uiStatic.prevMenu->bInTransition )
		if( uiStatic.prevMenu->DrawAnimation( CMenuBaseWindow::ANIM_OUT ) )
		{
			uiStatic.prevMenu->bInTransition = false;
		}


	if( uiStatic.firstDraw )
	{
		uiStatic.firstDraw = false;
		static int first = TRUE;
                    
		if( first )
		{
			// if game was launched with commandline e.g. +map or +load ignore the music
			if( !CL_IsActive( ))
				EngFuncs::PlayBackgroundTrack( "gamestartup", "gamestartup" );
			first = FALSE;
		}
	}

	// a1batross: moved to CMenuBaseWindow::DrawAnimation()
	//CR
	// CMenuPicButton::DrawTitleAnim();
	//

	// draw cursor
	UI_DrawMouseCursor();

	// delay playing the enter sound until after the menu has been
	// drawn, to avoid delay while caching images
	if( uiStatic.enterSound > 0.0f && uiStatic.enterSound <= gpGlobals->time )
	{
		EngFuncs::PlayLocalSound( uiSoundIn );
		uiStatic.enterSound = -1;
	}

	con_nprint_t con;
	con.time_to_live = 0.1;

	if( ui_show_window_stack && ui_show_window_stack->value )
	{
		for( int i = 0; i < uiStatic.menuDepth; i++ )
		{
			con.index++;
			if( uiStatic.menuActive == uiStatic.menuStack[i] )
			{
				con.color[0] = 0.0f;
				con.color[1] = 1.0f;
				con.color[2] = 0.0f;
			}
			else
			{
				con.color[0] = con.color[1] = con.color[2] = 1.0f;
			}


			if( uiStatic.menuStack[i]->IsRoot() )
			{
				if( uiStatic.rootActive == uiStatic.menuStack[i] &&
					uiStatic.rootActive != uiStatic.menuActive )
				{
					con.color[0] = 1.0f;
					con.color[1] = 1.0f;
					con.color[2] = 0.0f;
				}
				Con_NXPrintf( &con, "%p - %s\n", uiStatic.menuStack[i], uiStatic.menuStack[i]->szName );
			}
			else
			{
				Con_NXPrintf( &con, "     %p - %s\n", uiStatic.menuStack[i], uiStatic.menuStack[i]->szName );
			}
		}
	}
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, int down )
{
	const char	*sound = NULL;

	if( !uiStatic.initialized )
		return;

	if( !uiStatic.visible )
		return;

	if( !uiStatic.menuActive )
		return;
	if( key == K_MOUSE1 )
	{
		g_bCursorDown = !!down;
	}

	// go down on stack to nearest root or dialog
	int rootPos = uiStatic.rootPosition;
	for( int i = uiStatic.menuDepth-1; i >= rootPos; i-- )
	{
		sound = uiStatic.menuStack[i]->Key( key, down );

		if( !down && sound && sound != uiSoundNull )
			EngFuncs::PlayLocalSound( sound );

		if( uiStatic.menuStack[i]->iFlags & QMF_DIALOG )
			break;
	}
}

/*
=================
UI_CharEvent
=================
*/
void UI_CharEvent( int key )
{
	if( !uiStatic.initialized )
		return;

	if( !uiStatic.visible )
		return;

	if( !uiStatic.menuActive )
		return;

	int rootPos = uiStatic.rootPosition;
	for( int i = uiStatic.menuDepth-1; i >= rootPos; i-- )
	{
		uiStatic.menuStack[i]->Char( key );

		if( uiStatic.menuStack[i]->iFlags & QMF_DIALOG )
			break;
	}
}



bool g_bCursorDown;
float cursorDY;

/*
=================
UI_MouseMove
=================
*/
void UI_MouseMove( int x, int y )
{
	int		i;
	CMenuBaseItem	*item;

	if( !uiStatic.initialized )
		return;

	if( !uiStatic.visible )
		return;

	if( uiStatic.cursorX == x && uiStatic.cursorY == y )
		return;

	if( g_bCursorDown )
	{
		static bool prevDown = false;
		if(!prevDown)
			prevDown = true, cursorDY = 0;
		else
			if( y - uiStatic.cursorY )
				cursorDY += y - uiStatic.cursorY;
	}
	else
		cursorDY = 0;
	//Con_Printf("%d %d %f\n",x, y, cursorDY);
	if( !uiStatic.menuActive )
		return;

	// now menu uses absolute coordinates
	uiStatic.cursorX = x;
	uiStatic.cursorY = y;

	// hack: prevent changing focus when field active
	// a1ba: should this ever exist? We can't find FIELD in menu anymore, until RTTI is not enabled
	// maybe put this somewhere in draw method of field?
#if (defined(__ANDROID__) || defined(MENU_FIELD_RESIZE_HACK)) && 0
	CMenuField *f = uiStatic.menuActive->ItemAtCursor();
	if( f && f->type == QMTYPE_FIELD )
	{
		float y = f->y;

		if( y > ScreenHeight - f->height - 40 )
			y = ScreenHeight - f->height - 15;

		if( UI_CursorInRect( f->x - 30, y - 30, f->width + 60, f->height + 60 ) )
			return;
	}
#endif

	if( UI_CursorInRect( 1, 1, ScreenWidth - 1, ScreenHeight - 1 ))
		uiStatic.mouseInRect = true;
	else uiStatic.mouseInRect = false;

	uiStatic.cursorX = bound( 0, uiStatic.cursorX, ScreenWidth );
	uiStatic.cursorY = bound( 0, uiStatic.cursorY, ScreenHeight );

	// go down on stack to nearest root or dialog
	int rootPos = uiStatic.rootPosition;
	for( i = uiStatic.menuDepth-1; i >= rootPos; i-- )
	{
		uiStatic.menuStack[i]->MouseMove( x, y );

		if( uiStatic.menuStack[i]->iFlags & QMF_DIALOG )
			break;
	}
}


/*
=================
UI_SetActiveMenu
=================
*/
void UI_SetActiveMenu( int fActive )
{
	if( !uiStatic.initialized )
		return;

	uiStatic.framecount = 0;

	if( fActive )
	{
		// don't continue firing if we leave game
		EngFuncs::KEY_ClearStates();

		EngFuncs::KEY_SetDest( KEY_MENU );
		UI_Main_Menu();
	}
	else
	{
		UI_CloseMenu();
	}
}


#if defined _WIN32
#include <windows.h>
#include <winbase.h>
/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime( void )
{
	static LARGE_INTEGER g_PerformanceFrequency;
	static LARGE_INTEGER g_ClockStart;
	LARGE_INTEGER CurrentTime;

	if( !g_PerformanceFrequency.QuadPart )
	{
		QueryPerformanceFrequency( &g_PerformanceFrequency );
		QueryPerformanceCounter( &g_ClockStart );
	}

	QueryPerformanceCounter( &CurrentTime );
	return (double)( CurrentTime.QuadPart - g_ClockStart.QuadPart ) / (double)( g_PerformanceFrequency.QuadPart );
}
#elif defined __APPLE__
typedef unsigned long long longtime_t;
#include <sys/time.h>
/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime( void )
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double) tv.tv_sec + (double) tv.tv_usec/1000000.0;
}
#else
typedef unsigned long long longtime_t;
#include <time.h>
/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime( void )
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec/1000000000.0;
}
#endif

/*
=================
UI_AddServerToList
=================
*/
void UI_AddServerToList( netadr_t adr, const char *info )
{
	int	i;

	if( !uiStatic.initialized )
		return;

	if( uiStatic.numServers == UI_MAX_SERVERS )
		return;	// full

	if( stricmp( gMenu.m_gameinfo.gamefolder, Info_ValueForKey( info, "gamedir" )) != 0 )
		return;

	// ignore if duplicated
	for( i = 0; i < uiStatic.numServers; i++ )
	{
		if( !stricmp( uiStatic.serverNames[i], info ))
			return;
	}

	// add it to the list
	uiStatic.updateServers = true; // info has been updated
	uiStatic.serverAddresses[uiStatic.numServers] = adr;
	strncpy( uiStatic.serverNames[uiStatic.numServers], info, 256 );
	uiStatic.serverPings[uiStatic.numServers] = Sys_DoubleTime() - uiStatic.serversRefreshTime;
	if( uiStatic.serverPings[uiStatic.numServers] < 0 || uiStatic.serverPings[uiStatic.numServers] > 9.999f )
		uiStatic.serverPings[uiStatic.numServers] = 9.999f;
	uiStatic.numServers++;
}

/*
=================
UI_MenuResetPing_f
=================
*/
void UI_MenuResetPing_f( void )
{
	Con_Printf("UI_MenuResetPing_f\n");
	uiStatic.serversRefreshTime = Sys_DoubleTime();
}

/*
=================
UI_IsVisible

Some systems may need to know if it is visible or not
=================
*/
int UI_IsVisible( void )
{
	if( !uiStatic.initialized )
		return false;
	return uiStatic.visible;
}

void UI_GetCursorPos( int *pos_x, int *pos_y )
{
	if( pos_x ) *pos_x = uiStatic.cursorX;
	if( pos_y ) *pos_y = uiStatic.cursorY;
}

void UI_SetCursorPos( int pos_x, int pos_y )
{
//	uiStatic.cursorX = bound( 0, pos_x, ScreenWidth );
//	uiStatic.cursorY = bound( 0, pos_y, ScreenHeight );
	uiStatic.mouseInRect = true;
}

void UI_ShowCursor( int show )
{
	uiStatic.hideCursor = (show) ? false : true;
}

int UI_MouseInRect( void )
{
	return uiStatic.mouseInRect;
}

/*
=================
UI_Precache
=================
*/
void UI_Precache( void )
{
	if( !uiStatic.initialized )
		return;

	if( !ui_precache->value )
		return;

	EngFuncs::PIC_Load( UI_LEFTARROW );
	EngFuncs::PIC_Load( UI_LEFTARROWFOCUS );
	EngFuncs::PIC_Load( UI_RIGHTARROW );
	EngFuncs::PIC_Load( UI_RIGHTARROWFOCUS );
	EngFuncs::PIC_Load( UI_UPARROW );
	EngFuncs::PIC_Load( UI_UPARROWFOCUS );
	EngFuncs::PIC_Load( UI_DOWNARROW );
	EngFuncs::PIC_Load( UI_DOWNARROWFOCUS );
	EngFuncs::PIC_Load( "gfx/shell/splash" );

	if( ui_precache->value == 1 )
		return;

	UI_Main_Precache();
	UI_NewGame_Precache();
	UI_LoadGame_Precache();
	UI_SaveLoad_Precache();
	UI_MultiPlayer_Precache();
	UI_Options_Precache();
	UI_InternetGames_Precache();
	UI_PlayerSetup_Precache();
	UI_Controls_Precache();
	UI_AdvControls_Precache();
	UI_GameOptions_Precache();
	UI_CreateGame_Precache();
	UI_Audio_Precache();
	UI_Video_Precache();
	UI_VidOptions_Precache();
	UI_VidModes_Precache();
	UI_CustomGame_Precache();
	UI_Credits_Precache();
	UI_Touch_Precache();
	UI_TouchOptions_Precache();
	UI_TouchButtons_Precache();
	UI_TouchEdit_Precache();
	UI_FileDialog_Precache();
	UI_GamePad_Precache();
}

void UI_ParseColor( char *&pfile, int *outColor )
{
	int	i, color[3];
	char	token[1024];

	memset( color, 0xFF, sizeof( color ));

	for( i = 0; i < 3; i++ )
	{
		pfile = EngFuncs::COM_ParseFile( pfile, token );
		if( !pfile ) break;
		color[i] = atoi( token );
	}

	*outColor = PackRGB( color[0], color[1], color[2] );
}

void UI_ApplyCustomColors( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/colors.lst" );
	char *pfile = afile;
	char token[1024];

	if( !afile )
	{
		// not error, not warning, just notify
		Con_Printf( "UI_iColor = s: colors.lst not found\n" );
		return;
	}

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		if( !stricmp( token, "HELP_COLOR" ))
		{
			UI_ParseColor( pfile, &uiColorHelp );
		}
		else if( !stricmp( token, "PROMPT_BG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptBgColor );
		}
		else if( !stricmp( token, "PROMPT_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptTextColor );
		}
		else if( !stricmp( token, "PROMPT_FOCUS_COLOR" ))
		{
			UI_ParseColor( pfile, &uiPromptFocusColor );
		}
		else if( !stricmp( token, "INPUT_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputTextColor );
		}
		else if( !stricmp( token, "INPUT_BG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputBgColor );
		}
		else if( !stricmp( token, "INPUT_FG_COLOR" ))
		{
			UI_ParseColor( pfile, &uiInputFgColor );
		}
		else if( !stricmp( token, "CON_TEXT_COLOR" ))
		{
			UI_ParseColor( pfile, &uiColorConsole );
		}
	}

	int	r, g, b;

	UnpackRGB( r, g, b, uiColorConsole );
	EngFuncs::SetConsoleDefaultColor( r, g, b );

	EngFuncs::COM_FreeFile( afile );
}

static void UI_LoadBackgroundMapList( void )
{
	if( !EngFuncs::FileExists( "scripts/chapterbackgrounds.txt", TRUE ))
		return;

	char *afile = (char *)EngFuncs::COM_LoadFile( "scripts/chapterbackgrounds.txt", NULL );
	char *pfile = afile;
	char token[1024];

	uiStatic.bgmapcount = 0;

	if( !afile )
	{
		Con_Printf( "UI_LoadBackgroundMapList: chapterbackgrounds.txt not found\n" );
		return;
	}

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		// skip the numbers (old format list)
		if( isdigit( token[0] )) continue;

		strncpy( uiStatic.bgmaps[uiStatic.bgmapcount], token, sizeof( uiStatic.bgmaps[0] ));
		if( ++uiStatic.bgmapcount > UI_MAX_BGMAPS )
			break; // list is full
	}

	EngFuncs::COM_FreeFile( afile );
}

static void UI_ShowMessageBox( void )
{
	static char msg[1024];
	static CMenuYesNoMessageBox msgBox( true );

	strncpy( msg, EngFuncs::CmdArgv(1), 1023 );
	msg[1023] = 0;

	if( !UI_IsVisible() )
	{
		UI_Main_Menu();
		UI_SetActiveMenu( TRUE );
	}
	msgBox.SetMessage( msg );
	msgBox.Show();
}

/*
=================
UI_VidInit
=================
*/
int UI_VidInit( void )
{
	static bool calledOnce = true;
	if( uiStatic.textInput )
	{
	/*
		int rootPos = uiStatic.rootPosition;
		for( int i = uiStatic.menuDepth-1; i >= rootPos; i-- )
		{
			uiStatic.menuStack[i]->_Event( QM_IMRESIZED );

			if( uiStatic.menuStack[i]->iFlags & QMF_DIALOG )
				break;
		}*/
		if( uiStatic.menuActive && uiStatic.menuActive->ItemAtCursor() )
			uiStatic.menuActive->ItemAtCursor()->_Event( QM_IMRESIZED );
		
		return 0;
	}
	UI_Precache ();
	// Sizes are based on screen height
	uiStatic.scaleX = uiStatic.scaleY = ScreenHeight / 768.0f;
	uiStatic.width = ScreenWidth / uiStatic.scaleX;
	// move cursor to screen center
	uiStatic.cursorX = ScreenWidth / 2;
	uiStatic.cursorY = ScreenHeight / 2;
	uiStatic.outlineWidth = 4;

	// all menu buttons have the same view sizes
	uiStatic.buttons_draw_width = UI_BUTTONS_WIDTH;
	uiStatic.buttons_draw_height = UI_BUTTONS_HEIGHT;

	UI_ScaleCoords( NULL, NULL, &uiStatic.outlineWidth, NULL );
	UI_ScaleCoords( NULL, NULL, &uiStatic.buttons_draw_width, &uiStatic.buttons_draw_height );

	// trying to load colors.lst
	UI_ApplyCustomColors ();

	// trying to load chapterbackgrounds.txt
	UI_LoadBackgroundMapList ();

	// register menu font
	uiStatic.hFont = EngFuncs::PIC_Load( "#XASH_SYSTEMFONT_001.bmp", menufont_bmp, sizeof( menufont_bmp ));

	CMenuBackgroundBitmap::LoadBackground( );
#if 0
	FILE *f;

	// dump menufont onto disk
	f = fopen( "menufont.bmp", "wb" );
	fwrite( menufont_bmp, sizeof( menufont_bmp ), 1, f );
	fclose( f );
#endif

	// reload all menu buttons
	UI_LoadBmpButtons ();

	// now recalc all the menus in stack
	for( int i = 0; i < uiStatic.menuDepth; i++ )
	{
		CMenuBaseWindow *item = uiStatic.menuStack[i];

		if( item )
		{
			int cursor, cursorPrev;
			bool valid = false;

			// HACKHACK: Save cursor values when VidInit is called once
			// this don't let menu "forget" actual cursor values after, for example, window resizing
			if( calledOnce
				&& item->GetCursor() > 0 // ignore 0, because useless
				&& item->GetCursor() < item->ItemCount()
				&& item->GetCursorPrev() > 0
				&& item->GetCursorPrev() < item->ItemCount() )
			{
				valid = true;
				cursor = item->GetCursor();
				cursorPrev = item->GetCursorPrev();
			}

			// do vid restart for all pushed elements
			item->VidInit();

			if( valid )
			{
				// don't notify menu widget about cursor changes
				item->SetCursor( cursorPrev, false );
				item->SetCursor( cursor, false );
			}
		}
	}

	if( !calledOnce ) calledOnce = true;

	return 1;
}

/*
=================
UI_Init
=================
*/
void UI_Init( void )
{
	// register our cvars and commands
	ui_precache = EngFuncs::CvarRegister( "ui_precache", "0", FCVAR_ARCHIVE );
	ui_showmodels = EngFuncs::CvarRegister( "ui_showmodels", "0", FCVAR_ARCHIVE );
	ui_show_window_stack = EngFuncs::CvarRegister( "ui_show_window_stack", 0, FCVAR_ARCHIVE );

	// show cl_predict dialog
	EngFuncs::CvarRegister( "menu_mp_firsttime", "1", FCVAR_ARCHIVE );

	EngFuncs::Cmd_AddCommand( "menu_main", UI_Main_Menu );
	EngFuncs::Cmd_AddCommand( "menu_newgame", UI_NewGame_Menu );
	EngFuncs::Cmd_AddCommand( "menu_options", UI_Options_Menu );
	EngFuncs::Cmd_AddCommand( "menu_multiplayer", UI_MultiPlayer_Menu );
	EngFuncs::Cmd_AddCommand( "menu_langame", UI_LanGame_Menu );
	EngFuncs::Cmd_AddCommand( "menu_internetgames", UI_InternetGames_Menu );
	EngFuncs::Cmd_AddCommand( "menu_audio", UI_Audio_Menu );
	EngFuncs::Cmd_AddCommand( "menu_loadgame", UI_LoadGame_Menu );
	EngFuncs::Cmd_AddCommand( "menu_savegame", UI_SaveGame_Menu );
	EngFuncs::Cmd_AddCommand( "menu_saveload", UI_SaveLoad_Menu );
	EngFuncs::Cmd_AddCommand( "menu_playersetup", UI_PlayerSetup_Menu );
	EngFuncs::Cmd_AddCommand( "menu_controls", UI_Controls_Menu );
	EngFuncs::Cmd_AddCommand( "menu_advcontrols", UI_AdvControls_Menu );
	EngFuncs::Cmd_AddCommand( "menu_gameoptions", UI_GameOptions_Menu );
	EngFuncs::Cmd_AddCommand( "menu_creategame", UI_CreateGame_Menu );
	EngFuncs::Cmd_AddCommand( "menu_video", UI_Video_Menu );
	EngFuncs::Cmd_AddCommand( "menu_vidoptions", UI_VidOptions_Menu );
	EngFuncs::Cmd_AddCommand( "menu_vidmodes", UI_VidModes_Menu );
	EngFuncs::Cmd_AddCommand( "menu_customgame", UI_CustomGame_Menu );
	EngFuncs::Cmd_AddCommand( "menu_touch", UI_Touch_Menu );
	EngFuncs::Cmd_AddCommand( "menu_touchoptions", UI_TouchOptions_Menu );
	EngFuncs::Cmd_AddCommand( "menu_touchbuttons", UI_TouchButtons_Menu );
	EngFuncs::Cmd_AddCommand( "menu_touchedit", UI_TouchEdit_Menu );
	EngFuncs::Cmd_AddCommand( "menu_filedialog", UI_FileDialog_Menu );
	EngFuncs::Cmd_AddCommand( "menu_gamepad", UI_GamePad_Menu );
	EngFuncs::Cmd_AddCommand( "menu_resetping", UI_MenuResetPing_f );
	EngFuncs::Cmd_AddCommand( "menu_showmessagebox", UI_ShowMessageBox );
	EngFuncs::Cmd_AddCommand( "menu_connectionprogress", UI_ConnectionProgress_f );

	EngFuncs::CreateMapsList( TRUE );

	uiStatic.initialized = true;

	// setup game info
	EngFuncs::GetGameInfo( &gMenu.m_gameinfo );

	// load custom strings
	UI_LoadCustomStrings();

	// load scr
	UI_LoadScriptConfig();

	//CR
	CMenuPicButton::ClearButtonStack();
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void )
{
	if( !uiStatic.initialized )
		return;

	EngFuncs::Cmd_RemoveCommand( "menu_main" );
	EngFuncs::Cmd_RemoveCommand( "menu_newgame" );
	EngFuncs::Cmd_RemoveCommand( "menu_loadgame" );
	EngFuncs::Cmd_RemoveCommand( "menu_savegame" );
	EngFuncs::Cmd_RemoveCommand( "menu_saveload" );
	EngFuncs::Cmd_RemoveCommand( "menu_multiplayer" );
	EngFuncs::Cmd_RemoveCommand( "menu_options" );
	EngFuncs::Cmd_RemoveCommand( "menu_langame" );
	EngFuncs::Cmd_RemoveCommand( "menu_internetgames" );
	EngFuncs::Cmd_RemoveCommand( "menu_playersetup" );
	EngFuncs::Cmd_RemoveCommand( "menu_controls" );
	EngFuncs::Cmd_RemoveCommand( "menu_advcontrols" );
	EngFuncs::Cmd_RemoveCommand( "menu_gameoptions" );
	EngFuncs::Cmd_RemoveCommand( "menu_creategame" );
	EngFuncs::Cmd_RemoveCommand( "menu_audio" );
	EngFuncs::Cmd_RemoveCommand( "menu_video" );
	EngFuncs::Cmd_RemoveCommand( "menu_vidoptions" );
	EngFuncs::Cmd_RemoveCommand( "menu_vidmodes" );
	EngFuncs::Cmd_RemoveCommand( "menu_customgame" );
	EngFuncs::Cmd_RemoveCommand( "menu_touch" );
	EngFuncs::Cmd_RemoveCommand( "menu_touchoptions" );
	EngFuncs::Cmd_RemoveCommand( "menu_touchbuttons" );
	EngFuncs::Cmd_RemoveCommand( "menu_touchedit" );
	EngFuncs::Cmd_RemoveCommand( "menu_filedialog" );
	EngFuncs::Cmd_RemoveCommand( "menu_gamepad" );
	EngFuncs::Cmd_RemoveCommand( "menu_zoo" );

	memset( &uiStatic, 0, sizeof( uiStatic_t ));
}

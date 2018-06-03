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

#include "Framework.h"
#include "Bitmap.h"

#define UI_CREDITS_PATH		"credits.txt"
#define UI_CREDITS_MAXLINES		2048

static const char *uiCreditsDefault[] = 
{
	"",
	"Copyright XashXT Group 2017 (C)",
	"Copyright Flying With Gauss 2017 (C)",
	0
};

static class CMenuCredits : public CMenuBaseWindow
{
public:
	CMenuCredits() : CMenuBaseWindow( "Credits" ) { }

	void Draw();
	const char *Key(int key, int down);
	bool DrawAnimation(EAnimation anim) { return false; }
	void Show();

	friend void UI_DrawFinalCredits( void );
	friend void UI_FinalCredits( void );
	friend int UI_CreditsActive( void );

private:
	virtual void _Init();

	const char	**credits;
	int		startTime;
	int		showTime;
	int		fadeTime;
	int		numLines;
	int		active;
	int		finalCredits;
	char		*index[UI_CREDITS_MAXLINES];
	char		*buffer;
} uiCredits;


void CMenuCredits::Show()
{
	CMenuBaseWindow::Show();
	EngFuncs::KEY_SetDest( KEY_GAME );
}

/*
=================
CMenuCredits::Draw
=================
*/
void CMenuCredits::Draw( void )
{
	int	i, y;
	float	speed;
	int	w = UI_MED_CHAR_WIDTH;
	int	h = UI_MED_CHAR_HEIGHT;
	int	color = 0;

	// draw the background first
	if( CL_IsActive() && !finalCredits )
	{
		background.Draw();
	}

	speed = 32.0f * ( 768.0f / ScreenHeight );	// syncronize with final background track :-)


	// now draw the credits
	UI_ScaleCoords( NULL, NULL, &w, &h );

	y = ScreenHeight - (((gpGlobals->time * 1000) - uiCredits.startTime ) / speed );

	// draw the credits
	for ( i = 0; i < uiCredits.numLines && uiCredits.credits[i]; i++, y += h )
	{
		// skip not visible lines, but always draw end line
		if( y <= -h && i != uiCredits.numLines - 1 ) continue;

		if(( y < ( ScreenHeight - h ) / 2 ) && i == uiCredits.numLines - 1 )
		{
			if( !uiCredits.fadeTime ) uiCredits.fadeTime = (gpGlobals->time * 1000);
			color = UI_FadeAlpha( uiCredits.fadeTime, uiCredits.showTime );
			if( UnpackAlpha( color ))
				UI_DrawString( uiStatic.hDefaultFont, 0, ( ScreenHeight - h ) / 2, ScreenWidth, h, uiCredits.credits[i], color, true, w, h, QM_CENTER, true );
		}
		else UI_DrawString( uiStatic.hDefaultFont, 0, y, ScreenWidth, h, uiCredits.credits[i], uiColorWhite, false, w, h, QM_CENTER, true );
	}

	if( y < 0 && UnpackAlpha( color ) == 0 )
	{
		uiCredits.active = false; // end of credits
		if( uiCredits.finalCredits )
			EngFuncs::HostEndGame( gMenu.m_gameinfo.title );
	}

	if( !uiCredits.active )
		Hide();
}

/*
=================
CMenuCredits::Key
=================
*/
const char *CMenuCredits::Key( int key, int down )
{
	if( !down ) return uiSoundNull;

	// final credits can't be intterupted
	if( uiCredits.finalCredits )
		return uiSoundNull;

	uiCredits.active = false;
	return uiSoundNull;
}

/*
=================
CMenuCredits::_Init
=================
*/
void CMenuCredits::_Init( void )
{
	if( !uiCredits.buffer )
	{
		int	count;
		char	*p;

		// load credits if needed
		uiCredits.buffer = (char *)EngFuncs::COM_LoadFile( UI_CREDITS_PATH, &count );
		if( count )
		{
			if( uiCredits.buffer[count - 1] != '\n' && uiCredits.buffer[count - 1] != '\r' )
			{
				char *tmp = (char *)MALLOC( count + 2 );
				memcpy( tmp, uiCredits.buffer, count ); 
				EngFuncs::COM_FreeFile( uiCredits.buffer );
				uiCredits.buffer = tmp; 
				strncpy( uiCredits.buffer + count, "\r", 1 ); // add terminator
				count += 2; // added "\r\0"
			}
			p = uiCredits.buffer;

			// convert customs credits to 'ideal' strings array
			for ( uiCredits.numLines = 0; uiCredits.numLines < UI_CREDITS_MAXLINES; uiCredits.numLines++ )
			{
				uiCredits.index[uiCredits.numLines] = p;
				while ( *p != '\r' && *p != '\n' )
				{
					p++;
					if ( --count == 0 )
						break;
				}

				if ( *p == '\r' )
				{
					*p++ = 0;
					if( --count == 0 ) break;
				}

				*p++ = 0;
				if( --count == 0 ) break;
			}
			uiCredits.index[++uiCredits.numLines] = 0;
			uiCredits.credits = (const char **)uiCredits.index;
		}
		else
		{
			// use built-in credits
			uiCredits.credits =  uiCreditsDefault;
			uiCredits.numLines = ( sizeof( uiCreditsDefault ) / sizeof( uiCreditsDefault[0] )) - 1; // skip term
		}
	}

	// run credits
	uiCredits.startTime = (gpGlobals->time * 1000) + 500; // make half-seconds delay
	uiCredits.showTime = bound( 1000, strlen( uiCredits.credits[uiCredits.numLines - 1]) * 1000, 10000 );
	uiCredits.fadeTime = 0; // will be determined later
	uiCredits.active = true;
}

void UI_DrawFinalCredits( void )
{
	if( uiCredits.finalCredits && uiCredits.active )
		uiCredits.Draw ();
}

int UI_CreditsActive( void )
{
	return uiCredits.active && uiCredits.finalCredits;
}

/*
=================
UI_Credits_Precache
=================
*/
void UI_Credits_Precache( void )
{
}

/*
=================
UI_Credits_Menu
=================
*/
void UI_Credits_Menu( void )
{
	UI_Credits_Precache();
	uiCredits.Show();
}

void UI_FinalCredits( void )
{
	uiCredits.finalCredits = true;
	uiCredits.Show();
}

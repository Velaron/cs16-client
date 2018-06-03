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
#include "MenuStrings.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Table.h"
#include "CheckBox.h"
#include "Action.h"
#include "YesNoMessageBox.h"

#define ART_BANNER		"gfx/shell/head_vidmodes"

enum
{
	VID_NOMODE = -2, // engine values
	VID_AUTOMODE = -1,
	VID_NOMODE_POS = 0, // values in array
	VID_AUTOMODE_POS = 1,
	VID_MODES_POS = 2, // there starts engine modes
};

class CMenuVidModesModel : public CMenuBaseModel
{
public:
	void Update();
	int GetColumns() const { return 1; }
	int GetRows() const { return m_iNumModes; }
	const char *GetCellText(int line, int column) { return m_szModes[line]; }
private:
	int m_iNumModes;
	const char *m_szModes[32];
};

class CMenuVidModes : public CMenuFramework
{
private:
	void _Init();
	void _VidInit();
	void Draw(); // put test mode timer here
public:
	CMenuVidModes() : CMenuFramework( "CMenuVidModes" ) { testModeTimer = 0; }
	void SetConfig( );
	void RevertChanges();
	void ApplyChanges();

	CMenuCheckBox	windowed;
	CMenuCheckBox	vsync;

	CMenuTable	vidList;
	CMenuVidModesModel vidListModel;

	CMenuYesNoMessageBox testModeMsgBox;

	int prevMode;
	bool prevFullscreen;
	float testModeTimer;
	char testModeMsg[256];
} uiVidModes;


/*
=================
UI_VidModes_GetModesList
=================
*/
void CMenuVidModesModel::Update( void )
{
	unsigned int i;

	m_szModes[0] = "<Current window size>";
	m_szModes[1] = "<Desktop size>";

	for( i = VID_MODES_POS; i < 64; i++ )
	{
		const char *mode = EngFuncs::GetModeString( i - VID_MODES_POS );
		if( !mode ) break;
		m_szModes[i] = mode;
	}
	m_iNumModes = i;
}

/*
=================
UI_VidModes_SetConfig
=================
*/
void CMenuVidModes::SetConfig( )
{
	bool testMode = false;
	if( prevMode != vidList.GetCurrentIndex() - VID_MODES_POS )
	{
		char cmd[64];

		snprintf( cmd, sizeof( cmd ), "vid_mode %i", vidList.GetCurrentIndex() - VID_MODES_POS );
		EngFuncs::ClientCmd( TRUE, cmd );

		// have changed resolution, but enable test mode only in fullscreen
		testMode |= !windowed.bChecked;
	}

	if( prevFullscreen == windowed.bChecked )
	{
		EngFuncs::CvarSetValue( "fullscreen", !windowed.bChecked );

		// moved to fullscreen, enable test mode
		testMode |= !windowed.bChecked;
	}

	vsync.WriteCvar();

	if( testMode )
	{
		testModeMsgBox.Show();
		testModeTimer = gpGlobals->time + 10.0f; // ten seconds should be enough
	}
	else
	{
		// We're done now, just close
		Hide();
	}
}

void CMenuVidModes::ApplyChanges()
{
	prevMode = EngFuncs::GetCvarFloat( "vid_mode" );
	prevFullscreen = EngFuncs::GetCvarFloat( "fullscreen" );
}

void CMenuVidModes::RevertChanges()
{
	EngFuncs::CvarSetValue( "vid_mode", prevMode );
	EngFuncs::CvarSetValue( "fullscreen", prevFullscreen );
}

void CMenuVidModes::Draw()
{
	if( testModeMsgBox.IsVisible() )
	{
		if( testModeTimer - gpGlobals->time > 0 )
		{
			snprintf( testModeMsg, sizeof( testModeMsg ) - 1, "Keep this resolution? %i seconds remaining", (int)(testModeTimer - gpGlobals->time) );
			testModeMsg[sizeof(testModeMsg)-1] = 0;
		}
		else
		{
			RevertChanges();
			testModeMsgBox.Hide();
		}
	}
	CMenuFramework::Draw();
}

/*
=================
UI_VidModes_Init
=================
*/
void CMenuVidModes::_Init( void )
{
	banner.SetPicture(ART_BANNER);

	vidList.SetRect( 360, 255, -20, 340 );
	vidList.SetupColumn( 0, MenuStrings[IDS_VIDEO_MODECOL], 1.0f );
	vidList.SetModel( &vidListModel );

	windowed.SetNameAndStatus( "Run in a window", "Run game in window mode" );
	windowed.SetCoord( 360, 620 );
	SET_EVENT_MULTI( windowed.onChanged,
	{
		if( !uiVidModes.windowed.bChecked && uiVidModes.vidList.GetCurrentIndex() < VID_AUTOMODE_POS )
			uiVidModes.vidList.SetCurrentIndex( VID_AUTOMODE_POS );
	});

	SET_EVENT_MULTI( vidList.onChanged,
	{
		if( !uiVidModes.windowed.bChecked && uiVidModes.vidList.GetCurrentIndex() < VID_AUTOMODE_POS )
			uiVidModes.vidList.SetCurrentIndex( VID_AUTOMODE_POS );
	});

	vsync.SetNameAndStatus( "Vertical sync", "Enable vertical synchronization" );
	vsync.SetCoord( 360, 670 );
	vsync.LinkCvar( "gl_swapInterval" );

	testModeMsgBox.SetMessage( testModeMsg );
	testModeMsgBox.onPositive = VoidCb( &CMenuVidModes::ApplyChanges );
	testModeMsgBox.onNegative = VoidCb( &CMenuVidModes::RevertChanges );
	testModeMsgBox.Link( this );

	AddItem( background );
	AddItem( banner );
	AddButton( "Apply", "Apply changes", PC_OK, VoidCb( &CMenuVidModes::SetConfig ) );
	AddButton( "Cancel", "Return back to previous menu", PC_CANCEL, VoidCb( &CMenuVidModes::Hide ) );
	AddItem( windowed );
	AddItem( vsync );
	AddItem( vidList );
}

void CMenuVidModes::_VidInit()
{
	// don't overwrite prev values
	if( !testModeMsgBox.IsVisible() )
	{
		prevMode = EngFuncs::GetCvarFloat( "vid_mode" );
		vidList.SetCurrentIndex( prevMode + VID_MODES_POS );

		prevFullscreen = EngFuncs::GetCvarFloat( "fullscreen" );
		windowed.bChecked = !prevFullscreen;
	}
}

/*
=================
UI_VidModes_Precache
=================
*/
void UI_VidModes_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_VidModes_Menu
=================
*/
void UI_VidModes_Menu( void )
{
	UI_VidModes_Precache();
	uiVidModes.Show();
}

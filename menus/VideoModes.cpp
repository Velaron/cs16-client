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
#include "ScrollList.h"
#include "CheckBox.h"
#include "Action.h"

#define ART_BANNER		"gfx/shell/head_vidmodes"

#define MAX_VIDMODES	(sizeof( uiVideoModes ) / sizeof( uiVideoModes[0] )) + 1

static const char *uiVideoModes[] =
{
	"640 x 480",
	"800 x 600",
	"960 x 720",
	"1024 x 768",
	"1152 x 864",
	"1280 x 800",
	"1280 x 960",
	"1280 x 1024",
	"1600 x 1200",
	"2048 x 1536",
	"800 x 480 (wide)",
	"856 x 480 (wide)",
	"960 x 540 (wide)",
	"1024 x 576 (wide)",
	"1024 x 600 (wide)",
	"1280 x 720 (wide)",
	"1360 x 768 (wide)",
	"1366 x 768 (wide)",
	"1440 x 900 (wide)",
	"1680 x 1050 (wide)",
	"1920 x 1080 (wide)",
	"1920 x 1200 (wide)",
	"2560 x 1600 (wide)",
	"1600 x 900 (wide)",
};

class CMenuVidModes : public CMenuFramework
{
private:
	void _Init();
public:
	CMenuVidModes() : CMenuFramework( "CMenuVidModes" ) { }
	void GetConfig();
	void SetConfig();

	const char	*videoModesPtr[MAX_VIDMODES];

	CMenuPicButton	ok;
	CMenuPicButton	cancel;
	CMenuCheckBox	windowed;
	CMenuCheckBox	vsync;

	CMenuScrollList	vidList;
	CMenuAction	listCaption;
} uiVidModes;


/*
=================
UI_VidModes_GetModesList
=================
*/
void CMenuVidModes::GetConfig( void )
{
	unsigned int i;

	for( i = 0; i < MAX_VIDMODES-1; i++ )
		videoModesPtr[i] = uiVideoModes[i];
	videoModesPtr[i] = NULL;	// terminator

	vidList.pszItemNames = videoModesPtr;
	vidList.iCurItem = EngFuncs::GetCvarFloat( "vid_mode" );

	windowed.bChecked = !EngFuncs::GetCvarFloat( "fullscreen" );
}

/*
=================
UI_VidModes_SetConfig
=================
*/
void CMenuVidModes::SetConfig( void )
{
	EngFuncs::CvarSetValue( "vid_mode", vidList.iCurItem );
	// TODO: windowed.WriteCvar();
	EngFuncs::CvarSetValue( "fullscreen", !windowed.bChecked );
	vsync.WriteCvar();
}


/*
=================
UI_VidModes_Init
=================
*/
void CMenuVidModes::_Init( void )
{
	banner.SetPicture(ART_BANNER);

	ok.SetCoord( 72, 230 );
	ok.SetNameAndStatus( "Apply", "Apply changes" );
	ok.SetPicture( PC_OK );
	SET_EVENT( ok, onActivated )
	{
		((CMenuVidModes*)pSelf->Parent())->SetConfig();
		pSelf->Parent()->Hide();
	}
	END_EVENT( ok, onActivated )

	cancel.SetCoord( 72, 280 );
	cancel.SetNameAndStatus( "Cancel", "Return back to previous menu" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onActivated = HideCb;

	listCaption.iFlags = QMF_INACTIVE;
	listCaption.SetCharSize( QM_SMALLFONT );
	listCaption.iColor = uiColorHelp;
	listCaption.szName = MenuStrings[IDS_VIDEO_MODECOL];
	listCaption.SetCoord( 400, 270 );

	vidList.SetRect( 400, 300, 560, 300 );

	windowed.SetNameAndStatus( "Run in a window", "Run game in window mode" );
	windowed.SetCoord( 400, 620 );

	vsync.SetNameAndStatus( "Vertical sync", "Enable vertical synchronization" );
	vsync.SetCoord( 400, 670 );
	vsync.LinkCvar( "gl_swapInterval" );

	GetConfig();

	AddItem( background );
	AddItem( banner );
	AddItem( ok );
	AddItem( cancel );
	AddItem( windowed );
	AddItem( vsync );
	AddItem( listCaption );
	AddItem( vidList );
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

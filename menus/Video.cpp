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
#include "PicButton.h"

#define ART_BANNER		"gfx/shell/head_video"

class CMenuVideo : public CMenuFramework
{
public:
	CMenuVideo() : CMenuFramework( "CMenuVideo" ) { }

private:
	void _Init();

	CMenuPicButton	vidOptions;
	CMenuPicButton	vidModes;
	CMenuPicButton	done;
} uiVideo;

/*
=================
UI_Video_Init
=================
*/
void CMenuVideo::_Init( void )
{
	banner.SetPicture(ART_BANNER);

	vidOptions.SetNameAndStatus( "Video options", "Set video options such as screen size, gamma and image quality." );
	vidOptions.SetCoord( 72, 230 );
	vidOptions.SetPicture( PC_VID_OPT );
	vidOptions.iFlags |= QMF_NOTIFY;
	vidOptions.onActivated = UI_VidOptions_Menu;

	vidModes.SetNameAndStatus( "Video modes", "Set video modes and configure 3D accelerators." );
	vidModes.SetCoord( 72, 280 );
	vidModes.SetPicture( PC_VID_MODES);
	vidModes.iFlags |= QMF_NOTIFY;
	vidModes.onActivated = UI_VidModes_Menu;

	done.SetNameAndStatus( "Done", "Go back to the previous menu" );
	done.SetCoord( 72, 330 );
	done.SetPicture( PC_DONE );
	done.onActivated = HideCb;

	AddItem( background );
	AddItem( banner );
	AddItem( vidOptions );
	AddItem( vidModes );
	AddItem( done );
}

/*
=================
UI_Video_Precache
=================
*/
void UI_Video_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_Video_Menu
=================
*/
void UI_Video_Menu( void )
{
	UI_Video_Precache();
	uiVideo.Show();
}

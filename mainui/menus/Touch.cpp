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

#define ART_BANNER		"gfx/shell/head_touch"

class CMenuTouch : public CMenuFramework
{
public:
	CMenuTouch() : CMenuFramework( "CMenuTouch" ) { }

private:
	void _Init();
	void _VidInit();

	CMenuPicButton	touchOptions;
	CMenuPicButton	touchButtons;
	CMenuPicButton	done;
};

static CMenuTouch	uiTouch;

/*
=================
UI_Touch_Init
=================
*/
void CMenuTouch::_Init( void )
{
	banner.SetPicture(ART_BANNER );

	touchOptions.SetNameAndStatus( "Touch options", "Touch sensitivity and profile options" );
	touchOptions.SetPicture( "gfx/shell/btn_touch_options" );
	touchOptions.iFlags |= QMF_NOTIFY;
	touchOptions.onActivated = UI_TouchOptions_Menu;

	touchButtons.SetNameAndStatus( "Touch Buttons", "Add, remove, edit touch buttons" );
	touchButtons.SetPicture( "gfx/shell/btn_touch_buttons" );
	touchButtons.iFlags |= QMF_NOTIFY;
	touchButtons.onActivated = UI_TouchButtons_Menu;

	done.SetNameAndStatus( "Done",  "Go back to the previous menu" );
	done.SetPicture( PC_DONE );
	done.onActivated = HideCb;

	AddItem( background );
	AddItem( banner );
	AddItem( touchOptions );
	AddItem( touchButtons );
	AddItem( done );
}

void CMenuTouch::_VidInit()
{
	touchOptions.SetCoord( 72, 230 );
	touchButtons.SetCoord( 72, 280 );
	done.SetCoord( 72, 330 );
}

/*
=================
UI_Touch_Precache
=================
*/
void UI_Touch_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_Touch_Menu
=================
*/
void UI_Touch_Menu( void )
{
	UI_Touch_Precache();
	uiTouch.Show();
}


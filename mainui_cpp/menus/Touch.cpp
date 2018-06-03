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
	static CMenuFramework touch("CMenuTouch");

	UI_Touch_Precache();

	if( !touch.WasInit() )
	{
		touch.banner.SetPicture( ART_BANNER );
		touch.AddItem( touch.background );
		touch.AddItem( touch.banner );

		touch.AddButton( "Touch options", "Touch sensitivity and profile options", "gfx/shell/btn_touch_options",
			UI_TouchOptions_Menu, QMF_NOTIFY );

		touch.AddButton( "Touch buttons", "Add, remove, edit touch buttons", "gfx/shell/btn_touch_buttons",
			UI_TouchButtons_Menu, QMF_NOTIFY );

		touch.AddButton( "Done",  "Go back to the previous menu", PC_DONE, VoidCb( &CMenuFramework::Hide ), QMF_NOTIFY );
	}

	touch.Show();
}


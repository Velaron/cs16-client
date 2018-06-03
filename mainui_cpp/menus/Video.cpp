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
	static CMenuFramework video("CMenuVideo");

	UI_Video_Precache();

	if( !video.WasInit() )
	{
		video.banner.SetPicture(ART_BANNER);

		video.AddItem( video.background );
		video.AddItem( video.banner );
		video.AddButton( "Video options", "Set video options such as screen size, gamma and image quality.", PC_VID_OPT, UI_VidOptions_Menu, QMF_NOTIFY );
		video.AddButton( "Video modes", "Set video modes and configure 3D accelerators.", PC_VID_MODES, UI_VidModes_Menu, QMF_NOTIFY );
		video.AddButton( "Done", "Go back to the previous menu", PC_DONE, VoidCb( &CMenuFramework::Hide ), QMF_NOTIFY );
	}


	video.Show();
}

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
#include "YesNoMessageBox.h"
#include "keydefs.h"

#define ART_BANNER	     	"gfx/shell/head_config"

class CMenuOptions: public CMenuFramework
{
private:
	virtual void _Init( void );
	virtual void _VidInit( void );

public:
	CMenuOptions() : CMenuFramework("CMenuOptions") { }

	CMenuPicButton	controls;
	CMenuPicButton	audio;
	CMenuPicButton	video;
	CMenuPicButton	touch;
	CMenuPicButton	update;
	CMenuPicButton  gamepad;
	CMenuPicButton	done;

	// update dialog
	CMenuYesNoMessageBox msgBox;

	const char *m_szUpdateUrl;
};

static CMenuOptions	uiOptions;

/*
=================
CMenuOptions::Init
=================
*/
void CMenuOptions::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	controls.SetNameAndStatus( "Controls", "Change keyboard and mouse settings" );
	controls.SetPicture( PC_CONTROLS );
	controls.iFlags |= QMF_NOTIFY;
	controls.onActivated = UI_Controls_Menu;

	audio.SetNameAndStatus( "Audio", "Change sound volume and quality" );
	audio.SetPicture( PC_AUDIO );
	audio.iFlags |= QMF_NOTIFY;
	audio.onActivated = UI_Audio_Menu;

	video.SetNameAndStatus( "Video", "Change screen size, video mode and gamma" );
	video.SetPicture( PC_VIDEO );
	video.iFlags |= QMF_NOTIFY;
	video.onActivated = UI_Video_Menu;

	touch.SetNameAndStatus( "Touch", "Change touch settings and buttons" );
	touch.SetPicture( PC_TOUCH );
	touch.iFlags |= QMF_NOTIFY;
	touch.onActivated = UI_Touch_Menu;

	gamepad.SetNameAndStatus( "Gamepad", "Change gamepad axis and button settings" );
	gamepad.SetPicture( PC_GAMEPAD );
	gamepad.iFlags |= QMF_NOTIFY;
	gamepad.onActivated = UI_GamePad_Menu;

	update.SetNameAndStatus( "Update", "Download the latest version of Xash3D engine" );
	update.SetPicture( PC_UPDATE );
	update.iFlags |= QMF_NOTIFY;
	update.onActivated = msgBox.MakeOpenEvent();

	done.SetNameAndStatus( "Done", "Go back to the Main menu" );
	done.SetPicture( PC_DONE );
	done.iFlags |= QMF_NOTIFY;
	done.onActivated = HideCb;

	msgBox.SetMessage( "Check the Internet for updates?" );
	SET_EVENT( msgBox, onPositive )
	{
		EngFuncs::ShellExecute( ((CMenuOptions*)pSelf->Parent())->m_szUpdateUrl, NULL, TRUE );
	}
	END_EVENT( msgBox, onPositive )
	msgBox.Link( this );

	if( gMenu.m_gameinfo.update_url[0] != 0 )
		m_szUpdateUrl = gMenu.m_gameinfo.update_url;
	else
		m_szUpdateUrl = "https://github.com/FWGS/xash3d/releases/latest";

	AddItem( background );
	AddItem( banner );
	AddItem( controls );
	AddItem( audio );
	AddItem( video );
	AddItem( touch );
	AddItem( gamepad );
	AddItem( update );
	AddItem( done );
}

void CMenuOptions::_VidInit( void )
{
	controls.SetCoord( 72, 230 );
	audio.SetCoord( 72, 280 );
	video.SetCoord( 72, 330 );
	touch.SetCoord( 72, 380 );
	gamepad.SetCoord( 72, 430 );
	update.SetCoord( 72, 480 );
	done.SetCoord( 72, 530 );
}

/*
=================
CMenuOptions::Precache
=================
*/
void UI_Options_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
CMenuOptions::Menu
=================
*/
void UI_Options_Menu( void )
{
	UI_Options_Precache();
	uiOptions.Show();
}

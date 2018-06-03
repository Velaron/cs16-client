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
#include "kbutton.h"
#include "MenuStrings.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "Slider.h"
#include "YesNoMessageBox.h"

#define ART_BANNER			"gfx/shell/head_advanced"

class CMenuInputDevices : public CMenuFramework
{
private:
	virtual void _Init( void );
	virtual void _VidInit( void );

	void GetConfig( void );
	void SaveAndPopMenu( void );

public:
	CMenuInputDevices() : CMenuFramework("CMenuInputDevices") { }

	CMenuPicButton done, evdev;
	CMenuCheckBox mouse, touch, joystick;
};

static CMenuInputDevices	uiInputDevices;

/*
=================
UI_AdvControls_GetConfig
=================
*/
void CMenuInputDevices::GetConfig( void )
{
	mouse.LinkCvar( "m_ignore" );
	touch.LinkCvar( "touch_enable" );
	joystick.LinkCvar( "joy_enable" );

}

void CMenuInputDevices::SaveAndPopMenu()
{
	mouse.WriteCvar();
	touch.WriteCvar();
	joystick.WriteCvar();
	CMenuFramework::SaveAndPopMenu();
}


/*
=================
UI_AdvControls_Init
=================
*/
void CMenuInputDevices::_Init( void )
{
	//banner.SetPicture( ART_BANNER );
	eTransitionType = ANIM_OUT;

	done.SetNameAndStatus( "Done", "save changed and go back to the Customize Menu" );
	done.SetPicture( PC_DONE );
	done.onActivated = VoidCb( &CMenuInputDevices::SaveAndPopMenu );
	done.SetCoord( 72, 680 );

	mouse.szName = "Ignore mouse";
	mouse.szStatusText = "Need for some servers. Will disable mouse in menu too";
	mouse.iFlags |= QMF_NOTIFY;
#ifndef __ANDROID__
	SET_EVENT_MULTI( mouse.onChanged,
	{
		if( ((CMenuCheckBox*)pSelf)->bChecked )
		{
			static CMenuYesNoMessageBox msgbox(false);
			msgbox.SetMessage("If you do not have touchscreen, or joystick, you will not be able to play withot mouse. Are you sure to disable mouse?");
			SET_EVENT_MULTI( msgbox.onNegative,
			{
				uiInputDevices.mouse.bChecked = false;
			});

			msgbox.Show();
		}
	});
#endif
	mouse.SetCoord( 72, 230 );

	touch.szName = "Enable touch";
	touch.szStatusText = "On-screen controls for touchscreen";
	touch.iFlags |= QMF_NOTIFY;
	touch.SetCoord( 72, 280 );

	joystick.szName = "Enable joystick";
	joystick.SetCoord( 72, 330 );

	evdev.szName = "Evdev input (root)";
	evdev.szStatusText = "Press this to enable full mouse and keyboard control on Android";
	evdev.iFlags |= QMF_NOTIFY;
	evdev.SetCoord( 72, 380 );
	evdev.onActivated.SetCommand( FALSE, "evdev_autodetect\n" );

	AddItem( background );
	//AddItem( banner );
	AddItem( done );
	AddItem( mouse );
	AddItem( touch );
	AddItem( joystick );
#ifdef __ANDROID__
	AddItem( evdev );
#endif
}


void CMenuInputDevices::_VidInit()
{
	GetConfig();
}

/*
=================
UI_AdvControls_Precache
=================
*/
void UI_InputDevices_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_AdvControls_Menu
=================
*/
void UI_InputDevices_Menu( void )
{
	UI_InputDevices_Precache();
	uiInputDevices.Show();
}

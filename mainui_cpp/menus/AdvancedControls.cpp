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

#define ART_BANNER			"gfx/shell/head_advanced"

class CAdvancedControls : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;
	CAdvancedControls() : CMenuFramework("CAdvancedControls") { }

	void ToggleLookCheckboxes( bool write );

private:
	virtual void _Init( void );
	virtual void _VidInit( void );

	void GetConfig( void );
	void PitchInvert( void );
	void SaveAndPopMenu();

	CMenuPicButton done, inputDev;

	CMenuCheckBox	crosshair;
	CMenuCheckBox	invertMouse;
	CMenuCheckBox	mouseLook;
	CMenuCheckBox	lookSpring;
	CMenuCheckBox	lookStrafe;
	CMenuCheckBox	lookFilter;
	CMenuCheckBox	autoaim;
	CMenuSlider	sensitivity;
};

static CAdvancedControls	uiAdvControls;

/*
=================
UI_AdvControls_GetConfig
=================
*/
void CAdvancedControls::GetConfig( )
{
	kbutton_t	*mlook;

	if( EngFuncs::GetCvarFloat( "m_pitch" ) < 0 )
		invertMouse.bChecked = true;

	mlook = (kbutton_s *)EngFuncs::KEY_GetState( "in_mlook" );
	if( mlook )
	{
		if( mlook && mlook->state & 1 )
			mouseLook.bChecked = true;
		else
			mouseLook.bChecked = false;
	}
	else
	{
		mouseLook.SetGrayed( true );
		mouseLook.bChecked = true;
	}

	crosshair.LinkCvar( "crosshair" );
	lookSpring.LinkCvar( "lookspring" );
	lookStrafe.LinkCvar( "lookstrafe" );
	lookFilter.LinkCvar( "look_filter" );

	autoaim.LinkCvar( "sv_aim" );
	sensitivity.LinkCvar( "sensitivity" );

	ToggleLookCheckboxes( false );
}

void CAdvancedControls::PitchInvert()
{
	bool invert = invertMouse.bChecked;
	float m_pitch = EngFuncs::GetCvarFloat( "m_pitch" );
	if( ( invert && (m_pitch > 0) ) ||
		( !invert && (m_pitch < 0) ) )
	{
		EngFuncs::CvarSetValue( "m_pitch", -m_pitch );
	}
}

void CAdvancedControls::ToggleLookCheckboxes( bool write )
{
	lookSpring.SetGrayed( mouseLook.bChecked );
	lookStrafe.SetGrayed( mouseLook.bChecked );

	if( write )
	{
		if( mouseLook.bChecked )
			EngFuncs::ClientCmd( FALSE, "+mlook\nbind _force_write\n" );
		else
			EngFuncs::ClientCmd( FALSE, "-mlook\nbind _force_write\n" );
	}
}

void CAdvancedControls::SaveAndPopMenu()
{
	crosshair.WriteCvar();
	lookSpring.WriteCvar();
	lookStrafe.WriteCvar();
	lookFilter.WriteCvar();
	if( EngFuncs::GetCvarString("m_filter")[0] )
		EngFuncs::CvarSetValue( "m_filter", lookFilter.bChecked );
	autoaim.WriteCvar();
	sensitivity.WriteCvar();

	ToggleLookCheckboxes( true );

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
UI_AdvControls_Init
=================
*/
void CAdvancedControls::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	done.SetNameAndStatus( "Done", "save changed and go back to the Customize Menu" );
	done.SetPicture( PC_DONE );
	done.onActivated = VoidCb( &CAdvancedControls::SaveAndPopMenu );
	done.SetCoord( 72, 680 );

	crosshair.SetNameAndStatus( "Crosshair", "Enable the weapon aiming crosshair" );
	crosshair.iFlags |= QMF_NOTIFY;
	crosshair.SetCoord( 72, 280 );

	invertMouse.SetNameAndStatus( "Invert mouse", "Reverse mouse up/down axis" );
	invertMouse.iFlags |= QMF_NOTIFY;
	invertMouse.onChanged = VoidCb( &CAdvancedControls::PitchInvert );
	invertMouse.SetCoord( 72, 330 );

	mouseLook.SetNameAndStatus( "Mouse look", "Use the mouse to look around instead of using the mouse to move" );
	mouseLook.iFlags |= QMF_NOTIFY;
	SET_EVENT( mouseLook.onChanged,
		((CAdvancedControls*)pSelf->Parent())->ToggleLookCheckboxes( true ) );
	mouseLook.SetCoord( 72, 380 );

	lookSpring.SetNameAndStatus("Look spring", "Causes the screen to 'spring' back to looking straight ahead when you move forward" );
	lookSpring.iFlags |= QMF_NOTIFY;
	lookSpring.SetCoord( 72, 430 );

	lookStrafe.SetNameAndStatus( "Look strafe", "In combination with your mouse look modifier, causes left-right movements to strafe instead of turn");
	lookStrafe.iFlags |= QMF_NOTIFY;
	lookStrafe.SetCoord( 72, 480 );

	lookFilter.SetNameAndStatus( "Look filter", "Average look inputs over the last two frames to smooth out movements(generic)" );
	lookFilter.iFlags |= QMF_NOTIFY;
	lookFilter.SetCoord( 72, 530 );

	autoaim.SetNameAndStatus( "Autoaim", "Let game to help you aim at enemies" );
	autoaim.iFlags |= QMF_NOTIFY;
	autoaim.SetCoord( 72, 580 );

	sensitivity.SetNameAndStatus( "Senitivity", "Set in-game mouse sensitivity" );
	sensitivity.Setup( 0.0, 20.0f, 0.1 );
	sensitivity.SetCoord( 72, 660 );

	inputDev.SetNameAndStatus( "Input devices", "Toggle mouse, touch controls" );
	inputDev.onActivated = UI_InputDevices_Menu;
	inputDev.iFlags |= QMF_NOTIFY;
	if( CL_IsActive() && !EngFuncs::GetCvarFloat( "host_serverstate" ))
		inputDev.SetGrayed( true );
	//inputDev.SetRect( 72, 230, UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT );
	inputDev.SetCoord( 72, 230 );

	AddItem( background );
	AddItem( banner );
	AddItem( done );
	AddItem( inputDev );
	AddItem( crosshair );
	AddItem( invertMouse );
	AddItem( mouseLook );
	AddItem( lookSpring );
	AddItem( lookStrafe );
	AddItem( lookFilter );
	AddItem( autoaim );
	AddItem( sensitivity );
}


void CAdvancedControls::_VidInit()
{
	GetConfig();
}

/*
=================
UI_AdvControls_Precache
=================
*/
void UI_AdvControls_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_AdvControls_Menu
=================
*/
void UI_AdvControls_Menu( void )
{
	UI_AdvControls_Precache();
	uiAdvControls.Show();
}

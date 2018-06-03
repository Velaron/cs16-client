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
#include "Slider.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "StringArrayModel.h"
#include "Action.h"

#define ART_BANNER			"gfx/shell/head_gamepad"

enum engineAxis_t
{
	JOY_AXIS_SIDE = 0,
	JOY_AXIS_FWD,
	JOY_AXIS_PITCH,
	JOY_AXIS_YAW,
	JOY_AXIS_RT,
	JOY_AXIS_LT,
	JOY_AXIS_NULL
};

static const char *axisNames[7] =
{
	"Side Movement",
	"Forward Movement",
	"Camera Vertical Turn",
	"Camera Horizontal Turn",
	"Right Trigger",
	"Left Trigger",
	"NOT BOUND"
};

static class CMenuGamePad : public CMenuFramework
{
public:
	CMenuGamePad() : CMenuFramework("CMenuGamePad") { }

private:

	virtual void _Init();
	virtual void _VidInit();
	void GetConfig();
	void SaveAndPopMenu();

	CMenuSlider side, forward, pitch, yaw;
	CMenuCheckBox invSide, invFwd, invPitch, invYaw;

	CMenuSpinControl axisBind[6];

	CMenuAction axisBind_label;
} uiGamePad;


/*
=================
CMenuGamePad::GetConfig
=================
*/
void CMenuGamePad::GetConfig( void )
{
	float _side, _forward, _pitch, _yaw;
	char binding[7] = { 0 };

	Q_strncpy( binding, EngFuncs::GetCvarString( "joy_axis_binding"), sizeof( binding ));

	_side = EngFuncs::GetCvarFloat( "joy_side" );
	_forward = EngFuncs::GetCvarFloat( "joy_forward" );
	_pitch = EngFuncs::GetCvarFloat( "joy_pitch" );
	_yaw = EngFuncs::GetCvarFloat( "joy_yaw" );

	side.SetCurrentValue( fabs( _side ) );
	forward.SetCurrentValue( fabs( _forward ));
	pitch.SetCurrentValue( fabs( _pitch ));
	yaw.SetCurrentValue( fabs( _yaw ));

	invSide.bChecked = _side < 0.0f ? true: false;
	invFwd.bChecked = _forward < 0.0f ? true: false;
	invPitch.bChecked = _pitch < 0.0f ? true: false;
	invYaw.bChecked = _yaw < 0.0f ? true: false;

	// I made a monster...
	for( unsigned int i = 0; i < sizeof( binding ) - 1; i++ )
	{
		switch( binding[i] )
		{
		case 's':
			axisBind[i].ForceDisplayString( axisNames[JOY_AXIS_SIDE] );
			axisBind[i].SetCurrentValue( JOY_AXIS_SIDE );
			break;
		case 'f':
			axisBind[i].ForceDisplayString( axisNames[JOY_AXIS_FWD] );
			axisBind[i].SetCurrentValue( JOY_AXIS_FWD );
			break;
		case 'p':
			axisBind[i].ForceDisplayString( axisNames[JOY_AXIS_PITCH] );
			axisBind[i].SetCurrentValue( JOY_AXIS_PITCH );
			break;
		case 'y':
			axisBind[i].ForceDisplayString( axisNames[JOY_AXIS_YAW] );
			axisBind[i].SetCurrentValue( JOY_AXIS_YAW );
			break;
		case 'r':
			axisBind[i].ForceDisplayString( axisNames[JOY_AXIS_RT] );
			axisBind[i].SetCurrentValue( JOY_AXIS_RT );
			break;
		case 'l':
			axisBind[i].ForceDisplayString( axisNames[JOY_AXIS_LT] );
			axisBind[i].SetCurrentValue( JOY_AXIS_LT );
			break;
		default:
			axisBind[i].ForceDisplayString( axisNames[JOY_AXIS_NULL] );
			axisBind[i].SetCurrentValue( JOY_AXIS_NULL );
		}
	}
}

/*
=================
CMenuGamePad::SetConfig
=================
*/
void CMenuGamePad::SaveAndPopMenu()
{
	float _side, _forward, _pitch, _yaw;
	char binding[7] = { 0 };

	_side = side.GetCurrentValue();
	if( invSide.bChecked )
		_side *= -1;

	_forward = forward.GetCurrentValue();
	if( invFwd.bChecked )
		_forward *= -1;

	_pitch = pitch.GetCurrentValue();
	if( invPitch.bChecked )
		_pitch *= -1;

	_yaw = yaw.GetCurrentValue();
	if( invYaw.bChecked )
		_yaw *= -1;

	for( int i = 0; i < 6; i++ )
	{
		switch( (int)axisBind[i].GetCurrentValue() )
		{
		case JOY_AXIS_SIDE: binding[i]  = 's'; break;
		case JOY_AXIS_FWD: binding[i]   = 'f'; break;
		case JOY_AXIS_PITCH: binding[i] = 'p'; break;
		case JOY_AXIS_YAW: binding[i]   = 'y'; break;
		case JOY_AXIS_RT: binding[i]    = 'r'; break;
		case JOY_AXIS_LT: binding[i]    = 'l'; break;
		default: binding[i] = '0'; break;
		}
	}

	EngFuncs::CvarSetValue( "joy_side", _side );
	EngFuncs::CvarSetValue( "joy_forward", _forward );
	EngFuncs::CvarSetValue( "joy_pitch", _pitch );
	EngFuncs::CvarSetValue( "joy_yaw", _yaw );
	EngFuncs::CvarSetString( "joy_axis_binding", binding );

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuGamePad::Init
=================
*/
void CMenuGamePad::_Init( void )
{
	int i, y;

	static CStringArrayModel model( axisNames, ARRAYSIZE( axisNames ) );

	banner.SetPicture( ART_BANNER );

	axisBind_label.eTextAlignment = QM_CENTER;
	axisBind_label.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	axisBind_label.iColor = uiColorHelp;
	axisBind_label.szName = "Axis binding map";

	for( i = 0, y = 230; i < 6; i++, y += 50 )
	{
		axisBind[i].szStatusText = "Set axis binding";
		axisBind[i].Setup( &model );
	}

	side.Setup( 0.0f, 1.0f, 0.1f );
	side.SetNameAndStatus( "Side", "Side movement sensitity" );
	invSide.SetNameAndStatus( "Invert", "Invert side movement axis" );

	forward.Setup( 0.0f, 1.0f, 0.1f );
	forward.SetNameAndStatus( "Forward", "Forward movement sensitivity" );
	invFwd.SetNameAndStatus( "Invert", "Invert forward movement axis" );

	pitch.Setup( 0.0f, 200.0f, 0.1f );
	pitch.SetNameAndStatus( "Pitch", "Pitch rotating sensitivity" );
	invPitch.SetNameAndStatus( "Invert", "Invert pitch axis" );

	yaw.Setup( 0.0f, 200.0f, 0.1f );
	yaw.SetNameAndStatus( "Yaw", "Yaw rotating sensitivity" );
	invYaw.SetNameAndStatus( "Invert", "Invert yaw axis" );

	AddItem( background );
	AddItem( banner );
	AddButton( "Controls", "Change keyboard and mouse settings", PC_CONTROLS, UI_Controls_Menu );
	AddButton( "Done", "Go back to the Configuration Menu", PC_DONE, VoidCb( &CMenuGamePad::SaveAndPopMenu ) );
	for( i = 0; i < 6; i++ )
	{
		AddItem( axisBind[i] );
	}
	AddItem( side );
	AddItem( invSide );
	AddItem( forward );
	AddItem( invFwd );
	AddItem( pitch );
	AddItem( invPitch );
	AddItem( yaw );
	AddItem( invYaw );
	AddItem( axisBind_label );
}

void CMenuGamePad::_VidInit()
{
	axisBind_label.SetCoord( 360, 230 );
	axisBind_label.SetCharSize( QM_SMALLFONT );

	for( int i = 0, y = 280; i < 6; i++, y += 50 )
	{
		axisBind[i].SetRect( 360, y, 256, invSide.size.h );
		axisBind[i].SetCharSize( QM_SMALLFONT );
	}

	int sliderAlign = invSide.size.h - side.size.h;

	side.SetCoord( 630, 280 + sliderAlign );
	side.SetCharSize( QM_SMALLFONT );
	invSide.SetCoord( 850, 280 );

	forward.SetCoord( 630, 330 + sliderAlign );
	forward.SetCharSize( QM_SMALLFONT );
	invFwd.SetCoord( 850, 330 );

	pitch.SetCoord( 630, 380 + sliderAlign );
	pitch.SetCharSize( QM_SMALLFONT );
	invPitch.SetCoord( 850, 380 );

	yaw.SetCoord( 630, 430 + sliderAlign );
	yaw.SetCharSize( QM_SMALLFONT );
	invYaw.SetCoord( 850, 430 );

	GetConfig();
}

/*
=================
CMenuGamePad::Precache
=================
*/
void UI_GamePad_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
CMenuGamePad::Menu
=================
*/
void UI_GamePad_Menu( void )
{
	UI_GamePad_Precache();
	uiGamePad.Show();
}

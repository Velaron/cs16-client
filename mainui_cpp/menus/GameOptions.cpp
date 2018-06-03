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
#include "keydefs.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "SpinControl.h"
#include "Action.h"
#include "CheckBox.h"

#define ART_BANNER			"gfx/shell/head_advoptions"

class CMenuGameOptions : public CMenuFramework
{
public:
	CMenuGameOptions() : CMenuFramework("CMenuGameOptions") { }

	virtual const char *Key(int key, int down);
	void SetNetworkMode( int maxpacket, int maxpayload, int cmdrate, int updaterate, int rate );
private:
	virtual void _Init();
	void SaveCb( );
	void RestoreCb( );
	void Restore();
	void GetConfig();

	CMenuSpinControl	maxFPS;
	//CMenuCheckBox	hand;
	CMenuCheckBox	allowDownload;
	CMenuCheckBox	cl_predict;
	CMenuCheckBox	cl_lw;

	CMenuSpinControl	maxpacket, maxpayload, cmdrate, updaterate, rate;
	CMenuAction networkMode;
	CMenuCheckBox normal, dsl, slowest;
	CMenuCheckBox split, compress;
};

static CMenuGameOptions	uiGameOptions;

/*
=================
UI_GameOptions_KeyFunc
=================
*/
const char *CMenuGameOptions::Key( int key, int down )
{
	if( down && UI::Key::IsEscape( key ) )
		Restore();
	return CMenuFramework::Key( key, down );
}

void CMenuGameOptions::SetNetworkMode( int maxpacket1, int maxpayload1, int cmdrate1, int updaterate1, int rate1 )
{
	split.bChecked = true;
	compress.bChecked = false;
	normal.bChecked = dsl.bChecked = slowest.bChecked = false;
	maxpacket.SetCurrentValue( maxpacket1 );
	maxpayload.SetCurrentValue( maxpayload1 );
	if( !maxpayload1 )
	maxpayload.ForceDisplayString( "auto" );
	cmdrate.SetCurrentValue( cmdrate1 );
	updaterate.SetCurrentValue( updaterate1 );
	rate.SetCurrentValue( rate1 );
}

void CMenuGameOptions::SaveCb()
{
	maxFPS.WriteCvar();
	//hand.WriteCvar();
	allowDownload.WriteCvar();
	maxpacket.WriteCvar();
	maxpayload.WriteCvar();
	cmdrate.WriteCvar();
	updaterate.WriteCvar();
	rate.WriteCvar();
	split.WriteCvar();
	compress.WriteCvar();
	cl_predict.WriteCvar();
	cl_lw.WriteCvar();

	SaveAndPopMenu();
}

void CMenuGameOptions::Restore()
{
	maxFPS.DiscardChanges();
	//hand.DiscardChanges();
	allowDownload.DiscardChanges();
	maxpacket.DiscardChanges();
	maxpayload.DiscardChanges();
	cmdrate.DiscardChanges();
	updaterate.DiscardChanges();
	rate.DiscardChanges();
	split.DiscardChanges();
	compress.DiscardChanges();
	cl_lw.DiscardChanges();
	cl_predict.DiscardChanges();
}

void CMenuGameOptions::RestoreCb()
{
	Restore();
	Hide();
}

/*
=================
UI_GameOptions_Init
=================
*/
void CMenuGameOptions::_Init( void )
{
	banner.SetPicture( ART_BANNER );
	maxFPS.szName = "Limit game FPS";
	maxFPS.szStatusText = "Cap your game frame rate";
	maxFPS.Setup( 20, 500, 20 );
	maxFPS.LinkCvar( "fps_max", CMenuEditable::CVAR_VALUE );
	maxFPS.SetRect( 240, 270, 220, 32 );

	//hand.SetNameAndStatus( "Use left hand", "Draw gun at left side" );
	//hand.LinkCvar( "cl_righthand" );
	// hand.SetCoord( 240, 330 );

	allowDownload.SetNameAndStatus( "Allow download", "Allow download of files from servers" );
	allowDownload.LinkCvar( "sv_allow_download" );
	allowDownload.SetCoord( 240, 315 );

#ifdef NEW_ENGINE_INTERFACE
	cl_predict.SetNameAndStatus( "Disable predicting", "Disable player movement prediction" );
	cl_predict.LinkCvar( "cl_nopred" );
#else
	cl_predict.SetNameAndStatus( "Predict movement", "Enable player movement prediction" );
	cl_predict.LinkCvar( "cl_predict" );
#endif
	cl_predict.SetCoord( 240, 365 );

	cl_lw.SetNameAndStatus( "Local weapons", "Enable local weapons" );
	cl_lw.LinkCvar( "cl_lw" );
	cl_lw.SetCoord( 240, 415 );

	maxpacket.SetRect( 650, 270, 200, 32 );
	maxpacket.Setup( 150, 1550, 50 );
	maxpacket.LinkCvar( "cl_maxpacket", CMenuEditable::CVAR_VALUE );
	maxpacket.SetNameAndStatus( "Network packet size limit (cl_maxpacket)", "Split packet size and minimum size to compress");
	SET_EVENT_MULTI( maxpacket.onChanged,
	{
		CMenuSpinControl *self = (CMenuSpinControl *)pSelf;
		if( self->GetCurrentValue() == 1550 || self->GetCurrentValue() == 150 )
		{
			self->SetCurrentValue( 40000 );
			self->ForceDisplayString( "auto" );
		}
		else if( self->GetCurrentValue() > 1550 )
		{
			self->SetCurrentValue( 1500 );
		}
	});
	if( maxpacket.GetCurrentValue() == 40000 )
		maxpacket.ForceDisplayString( "auto" );

	maxpayload.SetRect( 650, 370, 200, 32 );
	maxpayload.Setup( 150, 1550, 50 );
	maxpayload.LinkCvar( "cl_maxpayload", CMenuEditable::CVAR_VALUE );
	maxpayload.SetNameAndStatus( "Singon size (cl_maxpayload)", "Singon cnain split decrease if cl_maxpacket does not help");
	SET_EVENT_MULTI( maxpayload.onChanged,
	{
		CMenuSpinControl *self = (CMenuSpinControl *)pSelf;
		if( self->GetCurrentValue() == 250 || self->GetCurrentValue() == 40050 )
		{
			self->SetCurrentValue( 0.0f );
			self->ForceDisplayString( "auto" );
		}
		else if( self->GetCurrentValue() > 0 && self->GetCurrentValue() < 250 )
		{
			self->SetCurrentValue( 300 );
		}
	});
	if( maxpayload.GetCurrentValue() == 0 )
		maxpayload.ForceDisplayString( "auto" );

	cmdrate.SetRect( 650, 470, 200, 32 );
	cmdrate.Setup( 20, 60, 5 );
	cmdrate.LinkCvar( "cl_cmdrate", CMenuEditable::CVAR_VALUE );
	cmdrate.SetNameAndStatus( "Command rate (cl_cmdrate)", "How many commands sent to server in second");

	updaterate.SetRect( 650, 570, 200, 32 );
	updaterate.Setup( 20, 100, 5 );
	updaterate.LinkCvar( "cl_updaterate", CMenuEditable::CVAR_VALUE );
	updaterate.SetNameAndStatus( "Update rate (cl_updaterate)", "How many uodates sent from server per second");

	rate.SetRect( 650, 670, 200, 32 );
	rate.Setup( 2500, 90000, 500 );
	rate.LinkCvar( "rate", CMenuEditable::CVAR_VALUE );
	rate.SetNameAndStatus( "Network speed (rate)", "Limit traffic (bytes per second)");

	networkMode.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	networkMode.szName = "Select network mode:";
	networkMode.iColor = uiColorHelp;
	networkMode.SetCharSize( QM_BIGFONT );
	networkMode.SetRect( 240, 450, 400, 32 );

	normal.SetRect( 240, 510, 24, 24 );
	normal.szName = "Normal internet connection";
	SET_EVENT_MULTI( normal.onChanged,
	{
		uiGameOptions.SetNetworkMode( 1400, 0, 30, 60, 25000 );
		((CMenuCheckBox*)pSelf)->bChecked = true;
	});

	dsl.SetRect( 240, 560, 24, 24 );
	dsl.szName = "DSL or PPTP with limited packet size";
	SET_EVENT_MULTI( dsl.onChanged,
	{
		uiGameOptions.SetNetworkMode( 1200, 1000, 30, 60, 25000 );
		((CMenuCheckBox*)pSelf)->bChecked = true;
	});


	slowest.SetRect( 240, 610, 24, 24 );
	slowest.szName = "Slow connection mode (64kbps)";
	SET_EVENT_MULTI( slowest.onChanged,
	{
		uiGameOptions.SetNetworkMode( 900, 700, 25, 30, 7500 );
		((CMenuCheckBox*)pSelf)->bChecked = true;
	});
	compress.SetNameAndStatus( "Compress", "Compress splitted packets (need split to work)" );
	compress.LinkCvar("cl_enable_splitcompress" );
	compress.SetCoord( 390, 680 );

	split.SetCoord( 240, 680 );
	split.SetNameAndStatus( "Split", "Split network packets" );
	split.LinkCvar("cl_enable_split" );

	AddItem( background );
	AddItem( banner );
	AddButton( "Done", "Save changes and go back to the Customize Menu", PC_DONE, VoidCb( &CMenuGameOptions::SaveCb ) );
	AddButton( "Cancel", "Go back to the Customize Menu", PC_CANCEL, VoidCb( &CMenuGameOptions::RestoreCb ) );

	AddItem( maxFPS );
	//AddItem( hand );

	AddItem( allowDownload );
	AddItem( cl_predict );
	AddItem( cl_lw );
	AddItem( maxpacket );
	AddItem( maxpayload );
	AddItem( cmdrate );
	AddItem( updaterate );
	AddItem( rate );
	AddItem( networkMode );
	AddItem( normal );
	AddItem( dsl );
	AddItem( slowest );
	AddItem( split );
	AddItem( compress );

	// only for game/engine developers
#ifdef NEW_ENGINE_INTERFACE
	if( EngFuncs::GetCvarFloat( "developer" ) < 1 )
#else
	if( EngFuncs::GetCvarFloat( "developer" ) < 3 )
#endif
	{
		maxpacket.Hide();
		rate.Hide();
	}

#ifdef NEW_ENGINE_INTERFACE
	if( EngFuncs::GetCvarFloat( "developer" ) < 2 )
#else
	if( EngFuncs::GetCvarFloat( "developer" ) < 4 )
#endif
	{
		maxpayload.Hide();
		cmdrate.Hide();
		updaterate.Hide();
		rate.SetCoord( 650, 370 );
	}
}

/*
=================
UI_GameOptions_Precache
=================
*/
void UI_GameOptions_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_GameOptions_Menu
=================
*/
void UI_GameOptions_Menu( void )
{
	UI_GameOptions_Precache();

	uiGameOptions.Show();
}

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
#include "YesNoMessageBox.h"
#include "keydefs.h"

#define ART_BANNER			"gfx/shell/head_multi"

class CMenuMultiplayer : public CMenuFramework
{
private:
	virtual void _Init();
	virtual void _VidInit();
public:
	CMenuMultiplayer() : CMenuFramework( "CMenuMultiplayer" ) { }

	CMenuPicButton	internetGames;
	CMenuPicButton	spectateGames;
	CMenuPicButton	LANGame;
	CMenuPicButton	Customize;	// playersetup
	CMenuPicButton	Controls;
	CMenuPicButton	done;

	// prompt dialog
	CMenuYesNoMessageBox msgBox;
};

static CMenuMultiplayer	uiMultiPlayer;


/*
=================
CMenuMultiplayer::Init
=================
*/
void CMenuMultiplayer::_Init( void )
{
	// memset( &uiMultiPlayer, 0, sizeof( CMenuMultiplayer ));

	banner.SetPicture( ART_BANNER );

	internetGames.SetNameAndStatus( "Internet game", "View list of a game internet servers and join the one of your choice" );
	internetGames.SetPicture( PC_INET_GAME );
	internetGames.iFlags |= QMF_NOTIFY;
	internetGames.onActivated = UI_InternetGames_Menu;

	spectateGames.iFlags |= QMF_GRAYED | QMF_NOTIFY;
	spectateGames.SetNameAndStatus( "Spectate game", "Spectate internet games" );
	spectateGames.SetPicture( PC_SPECTATE_GAMES );
	// TODO: implement
	// SET_EVENT_VOID( spectateGames, onActivated, UI_SpectateGames_Menu );

	LANGame.SetNameAndStatus( "LAN game", "Set up the game on the local area network" );
	LANGame.SetPicture( PC_LAN_GAME );
	LANGame.iFlags |= QMF_NOTIFY;
	LANGame.onActivated = UI_LanGame_Menu;

	Customize.SetNameAndStatus( "Customize", "Choose your player name, and select visual options for your character" );
	Customize.SetPicture( PC_CUSTOMIZE );
	Customize.iFlags |= QMF_NOTIFY;
	Customize.onActivated = UI_PlayerSetup_Menu;

	Controls.SetNameAndStatus( "Controls", "Change keyboard and mouse settings" );
	Controls.SetPicture( PC_CONTROLS );
	Controls.iFlags |= QMF_NOTIFY;
	Controls.onActivated = UI_Controls_Menu;

	done.SetNameAndStatus( "Done", "Go back to the Main menu" );
	done.SetPicture( PC_DONE );
	done.iFlags |= QMF_NOTIFY;
	done.onActivated = HideCb;

	msgBox.SetMessage( "It is recomended to enable\nclient movement prediction\nPress OK to enable it now\nOr enable it later in\n^5(Multiplayer/Customize)");
	msgBox.SetPositiveButton( "Ok", PC_OK );
	msgBox.SetNegativeButton( "Cancel", PC_CANCEL );
	msgBox.HighlightChoice( 1 );
	SET_EVENT( msgBox, onPositive )
	{
		EngFuncs::CvarSetValue( "cl_predict", 1.0f );
		EngFuncs::CvarSetValue( "menu_mp_firsttime", 0.0f );
	}
	END_EVENT( msgBox, onPositive )

	SET_EVENT( msgBox, onNegative )
	{
		EngFuncs::CvarSetValue( "menu_mp_firsttime", 0.0f );
	}
	END_EVENT( msgBox, onNegative )
	msgBox.Link( this );

	AddItem( background );
	AddItem( banner );
	AddItem( internetGames );
	AddItem( spectateGames );
	AddItem( LANGame );
	AddItem( Customize );
	AddItem( Controls );
	AddItem( done );
}

void CMenuMultiplayer::_VidInit()
{
	internetGames.SetCoord( 72, 230 );
	spectateGames.SetCoord( 72, 280 );
	LANGame.SetCoord( 72, 330 );
	Customize.SetCoord( 72, 380 );
	Controls.SetCoord( 72, 430 );
	done.SetCoord( 72, 480 );
}

/*
=================
CMenuMultiplayer::Precache
=================
*/
void UI_MultiPlayer_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_MultiPlayer_Menu
=================
*/
void UI_MultiPlayer_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	UI_MultiPlayer_Precache();
	uiMultiPlayer.Show();

	if( EngFuncs::GetCvarFloat( "menu_mp_firsttime" ) && !EngFuncs::GetCvarFloat( "cl_predict" ) )
		uiMultiPlayer.msgBox.Show();
}

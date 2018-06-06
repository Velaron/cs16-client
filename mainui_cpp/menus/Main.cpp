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
#include "Action.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"
#include "PlayerIntroduceDialog.h"

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	virtual const char *Key( int key, int down );
	virtual const char *Activate( );

private:
	virtual void _Init();
	virtual void _VidInit( );

	void QuitDialog( void *pExtra = NULL );
	void DisconnectDialogCb();

	CMenuPicButton	console;
	CMenuPicButton	resumeGame;
	CMenuPicButton	disconnect;
	CMenuPicButton	newGame;
	CMenuPicButton	configuration;
	CMenuPicButton	multiPlayer;
	CMenuPicButton	previews;
	CMenuPicButton	quit;

	// quit dialog
	CMenuYesNoMessageBox dialog;
};

static CMenuMain uiMain;

void CMenuMain::QuitDialog(void *pExtra)
{
	if( CL_IsActive() && EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
		dialog.SetMessage( MenuStrings[IDS_MAIN_QUITPROMPTINGAME] );
	else
		dialog.SetMessage( MenuStrings[IDS_MAIN_QUITPROMPT] );

	dialog.onPositive.SetCommand( FALSE, "quit\n" );
	dialog.Show();
}

void CMenuMain::DisconnectDialogCb()
{
	dialog.onPositive.SetCommand( FALSE, "cmd disconnect;endgame disconnect;wait;wait;wait;menu_options;menu_main\n" );
	dialog.SetMessage( "Really disconnect?" );
	dialog.Show();
}

/*
=================
CMenuMain::Key
=================
*/
const char *CMenuMain::Key( int key, int down )
{
	if( down && UI::Key::IsEscape( key ) )
	{
		if ( CL_IsActive( ))
		{
			if( !dialog.IsVisible() )
				UI_CloseMenu();
		}
		else
		{
			QuitDialog( );
		}
		return uiSoundNull;
	}
	return CMenuFramework::Key( key, down );
}

/*
=================
UI_Main_ActivateFunc
=================
*/
const char *CMenuMain::Activate( void )
{
	if ( CL_IsActive( ))
	{
		resumeGame.Show();
		disconnect.Show();
		console.SetCoord( 72, 280 );
	}
	else
	{
		resumeGame.Hide();
		disconnect.Hide();
		console.SetCoord( 72, 380 );
	}

	CMenuPicButton::ClearButtonStack();

	return 0;
}

void CMenuMain::_Init( void )
{
	// console
	console.SetNameAndStatus( "Console", "Show console" );
	console.iFlags |= QMF_NOTIFY;
	console.SetPicture( PC_CONSOLE );
	SET_EVENT_MULTI( console.onActivated,
	{
		UI_SetActiveMenu( FALSE );
		EngFuncs::KEY_SetDest( KEY_CONSOLE );
	});

	resumeGame.SetNameAndStatus( "Resume Game", MenuStrings[IDS_MAIN_RETURNHELP] );
	resumeGame.SetPicture( PC_RESUME_GAME );
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.onActivated = UI_CloseMenu;

	disconnect.SetNameAndStatus( "Disconnect", "Disconnect from server" );
	disconnect.SetPicture( PC_DISCONNECT );
	disconnect.iFlags |= QMF_NOTIFY;
	disconnect.onActivated = VoidCb( &CMenuMain::DisconnectDialogCb );

	newGame.SetNameAndStatus( "New Game", MenuStrings[IDS_MAIN_NEWGAMEHELP] );
	newGame.SetPicture( PC_NEW_GAME );
	newGame.iFlags |= QMF_NOTIFY;
	SET_EVENT_MULTI( newGame.onActivated,
	{
		EngFuncs::CvarSetValue( "public", 1.0f );
		UI_CreateGame_Menu();
	});

	multiPlayer.SetNameAndStatus( "Find Servers", MenuStrings[IDS_MAIN_MULTIPLAYERHELP] );
	multiPlayer.SetPicture( PC_MULTIPLAYER );
	multiPlayer.iFlags |= QMF_NOTIFY;
	multiPlayer.onActivated = UI_InternetGames_Menu;

	configuration.SetNameAndStatus( "Options", MenuStrings[IDS_MAIN_CONFIGUREHELP] );
	configuration.SetPicture( PC_CONFIG );
	configuration.iFlags |= QMF_NOTIFY;
	configuration.onActivated = UI_Options_Menu;

	previews.SetNameAndStatus( "Previews", MenuStrings[ IDS_MAIN_PREVIEWSHELP ] );
	previews.SetPicture( PC_PREVIEWS );
	previews.iFlags |= QMF_NOTIFY;
	SET_EVENT( previews.onActivated, EngFuncs::ShellExecute( MenuStrings[IDS_MEDIA_PREVIEWURL], NULL, false ) );

	quit.SetNameAndStatus( "Quit", MenuStrings[IDS_MAIN_QUITPROMPT] );
	quit.SetPicture( PC_QUIT );
	quit.iFlags |= QMF_NOTIFY;
	quit.onActivated = MenuCb( &CMenuMain::QuitDialog );

	dialog.Link( this );

	AddItem( background );

	if ( gpGlobals->developer )
		AddItem( console );

	AddItem( disconnect );
	AddItem( resumeGame );
	AddItem( newGame );
	AddItem( multiPlayer );
	AddItem( configuration );
	AddItem( previews );
	AddItem( quit );
}

/*
=================
UI_Main_Init
=================
*/
void CMenuMain::_VidInit( void )
{
	Activate();

	disconnect.SetCoord( 72, 330 );
	resumeGame.SetCoord( 72, 380 );
	newGame.SetCoord( 72, 430 );
	multiPlayer.SetCoord( 72, 480 );
	configuration.SetCoord( 72, 530 );
	previews.SetCoord( 72, 580 );
	quit.SetCoord( 72, 630 );
}

/*
=================
UI_Main_Precache
=================
*/
void UI_Main_Precache( void )
{
}

/*
=================
UI_Main_Menu
=================
*/
void UI_Main_Menu( void )
{
	UI_Main_Precache();
	uiMain.Show();

	if( !UI::Names::CheckIsNameValid( EngFuncs::GetCvarString( "name" ) ) )
	{
		UI_PlayerIntroduceDialog_Show();
	}
}

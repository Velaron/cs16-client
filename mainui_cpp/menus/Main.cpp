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

#define ART_MINIMIZE_N	"gfx/shell/min_n"
#define ART_MINIMIZE_F	"gfx/shell/min_f"
#define ART_MINIMIZE_D	"gfx/shell/min_d"
#define ART_CLOSEBTN_N	"gfx/shell/cls_n"
#define ART_CLOSEBTN_F	"gfx/shell/cls_f"
#define ART_CLOSEBTN_D	"gfx/shell/cls_d"

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
	void HazardCourseDialogCb();
	void HazardCourseCb();

	CMenuPicButton	console;
	class CMenuMainBanner : public CMenuBannerBitmap
	{
	public:
		virtual void Draw();
	} banner;

	CMenuPicButton	resumeGame;
	CMenuPicButton	disconnect;
	CMenuPicButton	newGame;
	CMenuPicButton	hazardCourse;
	CMenuPicButton	configuration;
	CMenuPicButton	saveRestore;
	CMenuPicButton	multiPlayer;
	CMenuPicButton	customGame;
	CMenuPicButton	previews;
	CMenuPicButton	quit;

	// buttons on top right. Maybe should be drawn if fullscreen == 1?
	CMenuBitmap	minimizeBtn;
	CMenuBitmap	quitButton;

	// quit dialog
	CMenuYesNoMessageBox dialog;

	bool bTrainMap;
	bool bCustomGame;
};

static CMenuMain uiMain;

void CMenuMain::CMenuMainBanner::Draw()
{
	if( !uiMain.background.ShouldDrawLogoMovie() )
		return; // no logos for steam background

	if( EngFuncs::GetLogoLength() <= 0.05f || EngFuncs::GetLogoWidth() <= 32 )
		return;	// don't draw stub logo (GoldSrc rules)

	float	logoWidth, logoHeight, logoPosY;
	float	scaleX, scaleY;

	scaleX = ScreenWidth / 640.0f;
	scaleY = ScreenHeight / 480.0f;

	// a1ba: multiply by height scale to look better on widescreens
	logoWidth = EngFuncs::GetLogoWidth() * scaleX;
	logoHeight = EngFuncs::GetLogoHeight() * scaleY * uiStatic.scaleY;
	logoPosY = 70 * scaleY * uiStatic.scaleY;	// 70 it's empirically determined value (magic number)

	EngFuncs::DrawLogo( "logo.avi", 0, logoPosY, logoWidth, logoHeight );
}

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

void CMenuMain::HazardCourseDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::HazardCourseCb );;
	dialog.SetMessage( MenuStrings[IDS_TRAINING_EXITCURRENT] );
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
	}
	else
	{
		resumeGame.Hide();
		disconnect.Hide();
	}

	if( gpGlobals->developer )
	{
		console.pos.y = CL_IsActive() ? 130 : 230;
	}

	CMenuPicButton::ClearButtonStack();

	return 0;
}

/*
=================
UI_Main_HazardCourse
=================
*/
void CMenuMain::HazardCourseCb()
{
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", 1.0f );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f ); // singleplayer

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	EngFuncs::ClientCmd( FALSE, "hazardcourse\n" );
}

void CMenuMain::_Init( void )
{
	if( gMenu.m_gameinfo.trainmap[0] && stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ) != 0 )
		bTrainMap = true;
	else bTrainMap = false;

	if( EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		bCustomGame = true;
	else bCustomGame = false;

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
	newGame.onActivated = UI_NewGame_Menu;

	hazardCourse.SetNameAndStatus( "Hazard Course", MenuStrings[IDS_MAIN_TRAININGHELP] );
	hazardCourse.SetPicture( PC_HAZARD_COURSE );
	hazardCourse.iFlags |= QMF_NOTIFY;
	hazardCourse.onActivatedClActive = VoidCb( &CMenuMain::HazardCourseDialogCb );
	hazardCourse.onActivated = VoidCb( &CMenuMain::HazardCourseCb );

	multiPlayer.SetNameAndStatus( "Multiplayer", MenuStrings[IDS_MAIN_MULTIPLAYERHELP] );
	multiPlayer.SetPicture( PC_MULTIPLAYER );
	multiPlayer.iFlags |= QMF_NOTIFY;
	multiPlayer.onActivated = UI_MultiPlayer_Menu;

	configuration.SetNameAndStatus( "Configuration", MenuStrings[IDS_MAIN_CONFIGUREHELP] );
	configuration.SetPicture( PC_CONFIG );
	configuration.iFlags |= QMF_NOTIFY;
	configuration.onActivated = UI_Options_Menu;

	saveRestore.iFlags |= QMF_NOTIFY;
	saveRestore.onActivatedClActive = UI_SaveLoad_Menu;
	saveRestore.onActivated = UI_LoadGame_Menu;

	customGame.SetNameAndStatus( "Custom Game", MenuStrings[IDS_MAIN_CUSTOMHELP] );
	customGame.SetPicture( PC_CUSTOM_GAME );
	customGame.iFlags |= QMF_NOTIFY;
	customGame.onActivated = UI_CustomGame_Menu;

	previews.SetNameAndStatus( "Previews", MenuStrings[ IDS_MAIN_PREVIEWSHELP ] );
	previews.SetPicture( PC_PREVIEWS );
	previews.iFlags |= QMF_NOTIFY;
	SET_EVENT( previews.onActivated, EngFuncs::ShellExecute( MenuStrings[IDS_MEDIA_PREVIEWURL], NULL, false ) );

	quit.SetNameAndStatus( "Quit", MenuStrings[IDS_MAIN_QUITPROMPT] );
	quit.SetPicture( PC_QUIT );
	quit.iFlags |= QMF_NOTIFY;
	quit.onActivated = MenuCb( &CMenuMain::QuitDialog );

	quitButton.SetPicture( ART_CLOSEBTN_N, ART_CLOSEBTN_F, ART_CLOSEBTN_D );
	quitButton.iFlags = QMF_MOUSEONLY|QMF_ACT_ONRELEASE;
	quitButton.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	quitButton.onActivated = MenuCb( &CMenuMain::QuitDialog );

	minimizeBtn.SetPicture( ART_MINIMIZE_N, ART_MINIMIZE_F, ART_MINIMIZE_D );
	minimizeBtn.iFlags = QMF_MOUSEONLY|QMF_ACT_ONRELEASE;
	minimizeBtn.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	minimizeBtn.onActivated.SetCommand( FALSE, "minimize\n" );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || gMenu.m_gameinfo.startmap[0] == 0 )
		newGame.SetGrayed( true );

	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		multiPlayer.SetGrayed( true );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
	}

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
	{
		saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
		newGame.SetGrayed( true );
	}

	dialog.Link( this );

	AddItem( background );
	AddItem( banner );

	if ( gpGlobals->developer )
		AddItem( console );

	AddItem( disconnect );
	AddItem( resumeGame );
	AddItem( newGame );

	if ( bTrainMap )
		AddItem( hazardCourse );

	AddItem( saveRestore );
	AddItem( configuration );
	AddItem( multiPlayer );

	if ( bCustomGame )
		AddItem( customGame );

	AddItem( previews );
	AddItem( quit );
	AddItem( minimizeBtn );
	AddItem( quitButton );
}

/*
=================
UI_Main_Init
=================
*/
void CMenuMain::_VidInit( void )
{
	Activate();

	console.pos.x = 72;
	resumeGame.SetCoord( 72, 230 );
	disconnect.SetCoord( 72, 180 );
	newGame.SetCoord( 72, 280 );
	hazardCourse.SetCoord( 72, 330 );

	if( CL_IsActive( ))
	{
		saveRestore.SetNameAndStatus( "Save\\Load Game", MenuStrings[IDS_MAIN_LOADSAVEHELP] );
		saveRestore.SetPicture( PC_SAVE_LOAD_GAME );
	}
	else
	{
		saveRestore.SetNameAndStatus( "Load Game", MenuStrings[IDS_MAIN_LOADHELP] );
		saveRestore.SetPicture( PC_LOAD_GAME );
	}

	saveRestore.SetCoord( 72, bTrainMap ? 380 : 330 );
	configuration.SetCoord( 72, bTrainMap ? 430 : 380 );
	multiPlayer.SetCoord( 72, bTrainMap ? 480 : 430 );

	customGame.SetCoord( 72, bTrainMap ? 530 : 480 );

	previews.SetCoord( 72, (bCustomGame) ? (bTrainMap ? 580 : 530) : (bTrainMap ? 530 : 480) );

	// too short execute string - not a real command
	if( strlen( MenuStrings[IDS_MEDIA_PREVIEWURL] ) <= 3 )
		previews.SetGrayed( true );

	quit.SetCoord( 72, (bCustomGame) ? (bTrainMap ? 630 : 580) : (bTrainMap ? 580 : 530));

	minimizeBtn.SetRect( uiStatic.width - 72, 13, 32, 32 );

	quitButton.SetRect( uiStatic.width - 36, 13, 32, 32 );
}

/*
=================
UI_Main_Precache
=================
*/
void UI_Main_Precache( void )
{
	EngFuncs::PIC_Load( ART_MINIMIZE_N );
	EngFuncs::PIC_Load( ART_MINIMIZE_F );
	EngFuncs::PIC_Load( ART_MINIMIZE_D );
	EngFuncs::PIC_Load( ART_CLOSEBTN_N );
	EngFuncs::PIC_Load( ART_CLOSEBTN_F );
	EngFuncs::PIC_Load( ART_CLOSEBTN_D );

	// precache .avi file and get logo width and height
	EngFuncs::PrecacheLogo( "logo.avi" );
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
}

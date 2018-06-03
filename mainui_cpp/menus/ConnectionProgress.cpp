/*
ConnectionProgress.cpp -- connection progress window
Copyright (C) 2017 mittorn

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
#include "extdll_menu.h"
#include "BaseMenu.h"
#include "ConnectionProgress.h"
#include "Utils.h"

#include "ItemsHolder.h"
#include "ProgressBar.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"

enum EState
{
	STATE_NONE,
	STATE_MENU, // do not hide when disconnected or in game
	STATE_DOWNLOAD, // enlarge your connectionprogress window
	STATE_CONNECTING, // showing single progress
	STATE_CONSOLE // do not show until state reset
};

enum ESource
{
	SOURCE_CONSOLE,
	SOURCE_SERVERBROWSER,
	SOURCE_CREATEGAME
};


class CMenuConnectionProgress : public CMenuBaseWindow
{
public:
	CMenuConnectionProgress();
	virtual void _Init();
	virtual void _VidInit();
	virtual void Draw();
	virtual bool DrawAnimation(EAnimation anim);
	virtual const char *Key( int key, int down );
	void Disconnect();
	void HandleDisconnect( void );
	void HandlePrecache( void )
	{
		SetCommonText( "Precaching resources" );
		commonProgress.LinkCvar( "scr_loading", 0, 100 );
		m_iState = STATE_CONNECTING;
	}
	void HandleStufftext( float flProgress, const char *pszText )
	{
		SetCommonText( pszText );
		commonProgress.SetValue( flProgress );
		m_iState = STATE_CONNECTING;
	}
	void HandleDownload( const char *pszFileName, const char *pszServerName, int iCurrent, int iTotal, const char *comment )
	{
		snprintf( sDownloadString, sizeof( sDownloadString ) - 1, "Downloading %s \nfrom %s", pszFileName, pszServerName );
		snprintf( sCommonString, sizeof( sCommonString ) - 1, "%d of %d %s", iCurrent + 1, iTotal, comment );
		m_iState = STATE_DOWNLOAD;
		commonProgress.SetValue( (float)iCurrent/iTotal +  0.01f / iTotal * EngFuncs::GetCvarFloat("scr_download") );
	}
	void SetCommonText( const char *pszText )
	{
		Q_strncpy( sCommonString, pszText, sizeof( sCommonString ) );
		// snprintf( sCommonString, sizeof( sCommonString ) - 1, "%s", pszText );
	}
	void SetServer( const char *pszName )
	{
		if( m_iSource == SOURCE_CREATEGAME )
		{
			strcpy( sTitleString, "Starting game..." );
		}
		else
		{
			snprintf( sTitleString, sizeof( sTitleString ) - 1, "Connecting to %s...", pszName );
		}

		commonProgress.SetValue( 0 );
	}

	EState m_iState;
	ESource m_iSource;
private:
	CMenuProgressBar commonProgress;
	CMenuProgressBar downloadProgress;
	CMenuPicButton consoleButton;
	CMenuPicButton disconnectButton;
	CMenuPicButton skipButton;
	CMenuYesNoMessageBox dialog;
	CMenuAction title;
	CMenuAction downloadText;
	CMenuAction commonText;
	char sTitleString[256];
	char sDownloadString[512];
	char sCommonString[512];
};

static CMenuConnectionProgress uiConnectionProgress;

CMenuConnectionProgress::CMenuConnectionProgress() : CMenuBaseWindow( "ConnectionProgress" )
{
	sDownloadString[0] = sCommonString[0] = sTitleString[0] = '\0';
	m_iSource = SOURCE_CONSOLE;
	m_iState = STATE_NONE;
	szName = "ConnectionProgress";
}

const char *CMenuConnectionProgress::Key( int key, int down )
{
	if( down )
	{
		switch( key )
		{
		case K_ESCAPE:
			dialog.Show();
			return uiSoundOut;
		case '~':
			consoleButton.Activate();
			return uiSoundLaunch;
		case 'A':
			HandleDisconnect();
			break;
		default: break;
		}
	}

	return CMenuItemsHolder::Key( key, down );
}

void CMenuConnectionProgress::HandleDisconnect( void )
{
	if( m_iState == STATE_NONE )
		return;

	if( m_iState == STATE_CONSOLE )
	{
		m_iState = STATE_NONE;
		return;
	}

	if( UI_IsVisible() && uiStatic.menuActive == this )
	{
		Hide();
		if( m_iSource != SOURCE_CONSOLE && m_iState != STATE_MENU )
		{
			UI_CloseMenu();
			UI_SetActiveMenu( true );
			UI_Main_Menu();
			UI_MultiPlayer_Menu();
			UI_ServerBrowser_Menu();
			if( m_iSource == SOURCE_CREATEGAME )
				UI_CreateGame_Menu();
			if( m_iState == STATE_DOWNLOAD )
			{
				Show();
				return;
			}
			m_iSource = SOURCE_CONSOLE;
		}
	}
	
	SetCommonText( "Disconnected." );

	m_iState = STATE_NONE;
	VidInit();
}

void CMenuConnectionProgress::Disconnect()
{
	if( m_iState == STATE_DOWNLOAD )
	{
		EngFuncs::ClientCmd( true, "http_clear\n" );
		m_iState = STATE_CONNECTING;
		HandleDisconnect();
	}

	EngFuncs::ClientCmd( false, "cmd disconnect;endgame disconnect\n");
}

void CMenuConnectionProgress::_Init( void )
{
	iFlags |= QMF_DIALOG;

	background.bForceColor = true;
	background.iColor = uiPromptBgColor;

	consoleButton.SetPicture( PC_CONSOLE );
	consoleButton.szName = "Console";
	SET_EVENT_MULTI( consoleButton.onActivated,
	{
		CMenuConnectionProgress *parent = (CMenuConnectionProgress *)pSelf->Parent();
		EngFuncs::KEY_SetDest( KEY_CONSOLE );
		parent->m_iState = STATE_CONSOLE;
		parent->m_iSource = SOURCE_CONSOLE;
		UI_CloseMenu();
		UI_SetActiveMenu( false );
	});
	consoleButton.bEnableTransitions = false;

	disconnectButton.SetPicture( PC_DISCONNECT );
	disconnectButton.szName = "Disconnect";
	disconnectButton.onActivated = VoidCb( &CMenuConnectionProgress::Disconnect );
	disconnectButton.bEnableTransitions = false;

	dialog.SetMessage( "Really disconnect?" );
	dialog.Link( this );
	dialog.onPositive = VoidCb( &CMenuConnectionProgress::Disconnect );

	title.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	title.eTextAlignment = QM_CENTER;
	title.szName = sTitleString;

	skipButton.szName = "Skip";
	skipButton.onActivated.SetCommand( TRUE, "http_skip\n" );
	skipButton.bEnableTransitions = false;

	downloadText.iFlags = commonText.iFlags = QMF_INACTIVE;
	downloadText.szName = sDownloadString;
	commonText.szName = sCommonString;

	downloadProgress.LinkCvar( "scr_download", 0.0f, 100.0f );

	AddItem( background );
	AddItem( consoleButton );
	AddItem( disconnectButton );
	AddItem( downloadProgress );
	AddItem( commonProgress );
	AddItem( title );
	AddItem( skipButton );
	AddItem( downloadText );
	AddItem( commonText );
}

void CMenuConnectionProgress::_VidInit( void )
{
	int dlg_h = ( m_iState == STATE_DOWNLOAD )?256:192;
	int dlg_y = 768 / 2 - dlg_h / 2;
	int cursor = dlg_h;

	SetRect( DLG_X + 192, dlg_y, 640, dlg_h );
	pos.x += uiStatic.xOffset;
	pos.y += uiStatic.yOffset;

	title.SetCharSize( QM_DEFAULTFONT );
	title.SetRect( 0, 16, 640, 20 );

	cursor -= 44;
	consoleButton.SetRect( 188, cursor, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );
	disconnectButton.SetRect( 338, cursor, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );

	if( !gpGlobals->developer )
		consoleButton.Hide();

	cursor -= 30;
	commonProgress.SetRect( 20, cursor, 600, 20 );

	cursor -= 50;
	commonText.SetCharSize( QM_SMALLFONT );
	commonText.SetRect( 20, cursor, 500, 40 );

	if( m_iState == STATE_DOWNLOAD )
	{
		cursor -= 30;
		downloadProgress.Show();
		downloadProgress.SetRect( 20, cursor, 500, 20 );
		skipButton.SetRect( 540, cursor, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );
		skipButton.Show();

		cursor -= 50;
		downloadText.Show();
		downloadText.SetCharSize( QM_SMALLFONT );
		downloadText.SetRect( 20, cursor, 500, 40 );
	}
	else
	{
		downloadProgress.Hide();
		skipButton.Hide();
		downloadText.Hide();
	}

	CalcPosition();
	CalcSizes();
}

bool CMenuConnectionProgress::DrawAnimation(EAnimation anim)
{
	// We don't have animation
	return true;
}

void CMenuConnectionProgress::Draw( void )
{
	if( ( m_iState != STATE_MENU && CL_IsActive() ) || ( m_iState == STATE_NONE && uiStatic.menuActive == this ) )
	{
		m_iState = STATE_NONE;
		Hide();
		return;
	}
	UI_FillRect( 0,0, gpGlobals->scrWidth, gpGlobals->scrHeight, m_iState == STATE_NONE ? 0xFF000000 : 0x40000000 );
	CMenuBaseWindow::Draw();
}


void UI_ConnectionProgress_f( void )
{
	if( !strcmp( EngFuncs::CmdArgv(1), "disconnect" ) )
	{
		uiConnectionProgress.HandleDisconnect();
		return;
	}

	if( uiConnectionProgress.m_iState == STATE_CONSOLE )
	{
		return;
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "dl" ) )
	{
		uiConnectionProgress.HandleDownload(  EngFuncs::CmdArgv( 2 ), EngFuncs::CmdArgv( 3 ), atoi(EngFuncs::CmdArgv( 4 )), atoi(EngFuncs::CmdArgv( 5 )), EngFuncs::CmdArgv( 6 ) );
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "dlend" ) )
	{
		uiConnectionProgress.m_iState = STATE_CONNECTING;
		uiConnectionProgress.HandleDisconnect();
		return;
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "stufftext" ) )
	{
		uiConnectionProgress.HandleStufftext( atof( EngFuncs::CmdArgv( 2 ) ), EngFuncs::CmdArgv( 3 ) );
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "precache" ) )
	{
		uiConnectionProgress.HandlePrecache();
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "menu" ) )
	{
		uiConnectionProgress.m_iState = STATE_MENU;
		uiConnectionProgress.m_iSource = SOURCE_SERVERBROWSER;
		if( EngFuncs::CmdArgc() > 2 )
			uiConnectionProgress.SetServer( EngFuncs::CmdArgv(2) );
		uiConnectionProgress.SetCommonText( "Establishing network connection to server...");
		uiConnectionProgress.Show();
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "localserver" ) )
	{
		uiConnectionProgress.m_iState = STATE_MENU;
		uiConnectionProgress.m_iSource = SOURCE_CREATEGAME;
		uiConnectionProgress.SetServer( "" );
		uiConnectionProgress.SetCommonText( "Starting local server...");
		uiConnectionProgress.Show();
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "changelevel" ) )
	{
		uiConnectionProgress.m_iState = STATE_MENU;
		uiConnectionProgress.SetCommonText( "Changing level on server");
		uiConnectionProgress.Show();
	}

	else if( !strcmp( EngFuncs::CmdArgv(1), "serverinfo" ) )
	{
		if( EngFuncs::CmdArgc() > 2 )
			uiConnectionProgress.SetServer( EngFuncs::CmdArgv(2) );
		uiConnectionProgress.m_iState = STATE_CONNECTING;
		uiConnectionProgress.SetCommonText( "Parsing server info..." );
		uiConnectionProgress.Show();
	}

	uiConnectionProgress.VidInit();
}
 

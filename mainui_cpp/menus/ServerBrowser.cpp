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
#include "Table.h"
#include "keydefs.h"
#include "Switch.h"
#include "Field.h"

#define ART_BANNER_INET		"gfx/shell/head_inetgames"
#define ART_BANNER_LAN		"gfx/shell/head_lan"
#define ART_BANNER_LOCK		"gfx/shell/lock"

// Server browser
#define QMSB_GAME_LENGTH	25
#define QMSB_MAPNAME_LENGTH	20+QMSB_GAME_LENGTH
#define QMSB_MAXCL_LENGTH	10+QMSB_MAPNAME_LENGTH
#define QMSB_PING_LENGTH    10+QMSB_MAXCL_LENGTH

class CMenuGameListModel : public CMenuBaseModel
{
public:
	void Update();
	int GetColumns() const
	{
		return 5; // havePassword, game, mapname, maxcl, ping
	}
	int GetRows() const
	{
		return m_iNumItems;
	}
	ECellType GetCellType( int line, int column )
	{
		if( column == 0 )
			return CELL_IMAGE_ADDITIVE;
		return CELL_TEXT;
	}
	const char *GetCellText( int line, int column )
	{
		if( column == 0 )
		{
			if( m_bHavePassword[line] )
				return ART_BANNER_LOCK;
			else
				return NULL;
		}

		return m_szCells[line][column - 1];
	}
	void OnActivateEntry(int line);

	void Flush()
	{
		m_iNumItems = 0;
	}

	bool IsHavePassword( int line )
	{
		return m_bHavePassword[line];
	}

private:
	char m_szCells[UI_MAX_SERVERS][4][64];
	bool m_bHavePassword[UI_MAX_SERVERS];
	int m_iNumItems;
};

struct serverSelect_t
{
	netadr_t adr;
	bool havePassword;
};

class CMenuServerBrowser: public CMenuFramework
{
public:
	CMenuServerBrowser() : CMenuFramework( "CMenuServerBrowser" ) { }
	virtual void Draw();
	virtual void Show();

	void SetLANOnly( bool lanOnly )
	{
		m_bLanOnly = lanOnly;
	}
	void GetGamesList( void );
	void ClearList( void );
	void RefreshList( void );
	void JoinGame( void );

	static void Connect( netadr_t server, bool havePassword );
	static void Connect( serverSelect_t server );

	CMenuPicButton *joinGame;
	CMenuPicButton *createGame;
	CMenuPicButton *refresh;
	CMenuSwitch natOrDirect;

	CMenuYesNoMessageBox msgBox;
	CMenuTable	gameList;
	CMenuGameListModel gameListModel;

	CMenuYesNoMessageBox askPassword;
	CMenuField password;

	int	  refreshTime;
	int   refreshTime2;

	bool m_bLanOnly;
private:
	virtual void _Init();
	virtual void _VidInit();
};

static serverSelect_t staticServerSelect;
static bool staticWaitingPassword = false;

static CMenuServerBrowser	uiServerBrowser;

/*
=================
CMenuServerBrowser::GetGamesList
=================
*/
void CMenuGameListModel::Update( void )
{
	int		i;
	const char	*info;

	for( i = 0; i < uiStatic.numServers; i++ )
	{
		if( i >= UI_MAX_SERVERS )
			break;
		info = uiStatic.serverNames[i];

		Q_strncpy( m_szCells[i][0], Info_ValueForKey( info, "host" ), 64 );
		Q_strncpy( m_szCells[i][1], Info_ValueForKey( info, "map" ), 64 );
		snprintf( m_szCells[i][2], 64, "%s\\%s", Info_ValueForKey( info, "numcl" ), Info_ValueForKey( info, "maxcl" ) );
		snprintf( m_szCells[i][3], 64, "%.f ms", uiStatic.serverPings[i] * 1000 );


		const char *passwd = Info_ValueForKey( info, "password" );
		if( passwd[0] && !stricmp( passwd, "1") )
		{
			m_bHavePassword[i] = true;
		}
		else
		{
			m_bHavePassword[i] = false;
		}
	}

	m_iNumItems = i;

	if( uiStatic.numServers )
		uiServerBrowser.joinGame->SetGrayed( false );
}

void CMenuGameListModel::OnActivateEntry( int line )
{
	CMenuServerBrowser::Connect( uiStatic.serverAddresses[line], m_bHavePassword[line] );
}

void CMenuServerBrowser::Connect( netadr_t server, bool havePassword )
{
	serverSelect_t select;
	select.adr = server;
	select.havePassword = havePassword;

	Connect( select );
}

void CMenuServerBrowser::Connect( serverSelect_t server )
{
	// prevent refresh during connect
	uiServerBrowser.refreshTime = uiStatic.realTime + 999999;

	// ask user for password
	if( server.havePassword )
	{
		// if dialog window is still open, then user have entered the password
		if( !staticWaitingPassword )
		{
			// save current select
			staticServerSelect = server;
			staticWaitingPassword = true;

			// show password request window
			uiServerBrowser.askPassword.Show();

			return;
		}
	}
	else
	{
		// remove password, as server don't require it
		EngFuncs::CvarSetString( "password", "" );
	}

	staticWaitingPassword = false;

	//BUGBUG: ClientJoin not guaranted to return, need use ClientCmd instead!!!
	//BUGBUG: But server addres is known only as netadr_t here!!!
	EngFuncs::ClientJoin( server.adr );
	EngFuncs::ClientCmd( false, "menu_connectionprogress menu server\n" );
}

/*
=================
CMenuServerBrowser::JoinGame
=================
*/
void CMenuServerBrowser::JoinGame()
{
	int select = gameList.GetCurrentIndex();

	Connect( uiStatic.serverAddresses[select], gameListModel.IsHavePassword( select ) );
}

void CMenuServerBrowser::ClearList()
{
	uiStatic.numServers = 0;
	gameListModel.Flush();
}

void CMenuServerBrowser::RefreshList()
{
	if( m_bLanOnly )
	{
		UI_RefreshServerList();
	}
	else
	{
		if( uiStatic.realTime > refreshTime2 )
		{
			UI_RefreshInternetServerList();
			refreshTime2 = uiStatic.realTime + (EngFuncs::GetCvarFloat("cl_nat") ? 4000:1000);
			refresh->SetGrayed( true );
			if( uiStatic.realTime + 20000 < refreshTime )
				refreshTime = uiStatic.realTime + 20000;
		}
	}
	gameListModel.Flush();
}

/*
=================
UI_Background_Ownerdraw
=================
*/
void CMenuServerBrowser::Draw( void )
{
	CMenuFramework::Draw();

	if( uiStatic.realTime > refreshTime )
	{
		RefreshList();
		refreshTime = uiStatic.realTime + 20000; // refresh every 10 secs
	}

	if( uiStatic.realTime > refreshTime2 )
	{
		refresh->SetGrayed( false );
	}

	// serverinfo has been changed update display
	if( uiStatic.updateServers )
	{
		gameListModel.Update();
		uiStatic.updateServers = false;
	}
}

/*
=================
CMenuServerBrowser::Init
=================
*/
void CMenuServerBrowser::_Init( void )
{
	AddItem( background );
	AddItem( banner );

	joinGame = AddButton( "Join game", "Join to selected game", PC_JOIN_GAME,
		VoidCb( &CMenuServerBrowser::JoinGame ), QMF_GRAYED );
	joinGame->onActivatedClActive = msgBox.MakeOpenEvent();

	createGame = AddButton( "Create game", NULL, PC_CREATE_GAME );
	SET_EVENT_MULTI( createGame->onActivated,
	{
		if( ((CMenuServerBrowser*)pSelf->Parent())->m_bLanOnly )
			EngFuncs::CvarSetValue( "public", 0.0f );
		else EngFuncs::CvarSetValue( "public", 1.0f );

		UI_CreateGame_Menu();
	});

	// TODO: implement!
	AddButton( "View game info", "Get detail game info", PC_VIEW_GAME_INFO, CEventCallback::NoopCb, QMF_GRAYED );

	refresh = AddButton( "Refresh", "Refresh servers list", PC_REFRESH, VoidCb( &CMenuServerBrowser::RefreshList ) );

	AddButton( "Done", "Return to main menu", PC_DONE, VoidCb( &CMenuServerBrowser::Hide ) );

	msgBox.SetMessage( "Join a network game will exit any current game, OK to exit?" );
	msgBox.SetPositiveButton( "Ok", PC_OK );
	msgBox.HighlightChoice( CMenuYesNoMessageBox::HIGHLIGHT_YES );
	msgBox.onPositive = VoidCb( &CMenuServerBrowser::JoinGame );
	msgBox.Link( this );

	gameList.SetCharSize( QM_SMALLFONT );
	gameList.SetupColumn( 0, NULL, 32.0f, true );
	gameList.SetupColumn( 1, "Name", 0.40f );
	gameList.SetupColumn( 2, "Map", 0.25f );
	gameList.SetupColumn( 3, "Players", 100.0f, true );
	gameList.SetupColumn( 4, "Ping", 120.0f, true );
	gameList.SetModel( &gameListModel );
	gameList.bFramedHintText = true;

	natOrDirect.szLeftName = "Direct";
	natOrDirect.szRightName = "NAT";
	natOrDirect.eTextAlignment = QM_CENTER;
	natOrDirect.bMouseToggle = false;
	natOrDirect.LinkCvar( "cl_nat" );
	natOrDirect.iSelectColor = uiInputFgColor;
	// bit darker
	natOrDirect.iFgTextColor = uiInputFgColor - 0x00151515;
	SET_EVENT_MULTI( natOrDirect.onChanged,
	{
		CMenuSwitch *self = (CMenuSwitch*)pSelf;
		CMenuServerBrowser *parent = (CMenuServerBrowser*)self->Parent();

		self->WriteCvar();
		parent->ClearList();
		parent->RefreshList();
	});

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
		createGame->SetGrayed( true );	// server.dll is missed - remote servers only

	password.bHideInput = true;
	password.bAllowColorstrings = false;
	password.bNumbersOnly = false;
	password.szName = "Password:";
	password.iMaxLength = 16;
	password.SetRect( 188, 140, 270, 32 );

	SET_EVENT_MULTI( askPassword.onPositive,
	{
		CMenuServerBrowser *parent = (CMenuServerBrowser*)pSelf->Parent();

		EngFuncs::CvarSetString( "password", parent->password.GetBuffer() );
		parent->password.Clear(); // we don't need entered password anymore
		CMenuServerBrowser::Connect( staticServerSelect );
	});

	SET_EVENT_MULTI( askPassword.onNegative,
	{
		CMenuServerBrowser *parent = (CMenuServerBrowser*)pSelf->Parent();

		EngFuncs::CvarSetString( "password", "" );
		parent->password.Clear(); // we don't need entered password anymore
		staticWaitingPassword = false;
	});

	askPassword.SetMessage( "Enter server password to continue:" );
	askPassword.Link( this );
	askPassword.Init();
	askPassword.AddItem( password );

	AddItem( gameList );
	AddItem( natOrDirect );
}

/*
=================
CMenuServerBrowser::VidInit
=================
*/
void CMenuServerBrowser::_VidInit()
{
	if( m_bLanOnly )
	{
		banner.SetPicture( ART_BANNER_LAN );
		createGame->szStatusText = ( "Create new LAN game" );
		natOrDirect.Hide();
	}
	else
	{
		banner.SetPicture( ART_BANNER_INET );
		createGame->szStatusText = ( "Create new Internet game" );
		natOrDirect.Show();
	}

	gameList.SetRect( 360, 255, -20, 440 );
	natOrDirect.SetCoord( -20 - natOrDirect.size.w, 255 - gameList.charSize.h * 1.5 - UI_OUTLINE_WIDTH * 2 - natOrDirect.size.h );

	refreshTime = uiStatic.realTime + 500; // delay before update 0.5 sec
	refreshTime2 = uiStatic.realTime + 500;
}

void CMenuServerBrowser::Show()
{
	CMenuFramework::Show();

	// clear out server table
	gameListModel.Flush();
	joinGame->SetGrayed( true );
}

/*
=================
CMenuServerBrowser::Precache
=================
*/
void UI_InternetGames_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER_INET );
	EngFuncs::PIC_Load( ART_BANNER_LAN );
}

/*
=================
CMenuServerBrowser::Menu
=================
*/
void UI_ServerBrowser_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	// stop demos to allow network sockets to open
	if ( gpGlobals->demoplayback && EngFuncs::GetCvarFloat( "cl_background" ))
	{
		uiStatic.m_iOldMenuDepth = uiStatic.menuDepth;
		EngFuncs::ClientCmd( FALSE, "stop\n" );
		uiStatic.m_fDemosPlayed = true;
	}

	UI_InternetGames_Precache();
	uiServerBrowser.Show();
}

void UI_InternetGames_Menu( void )
{
	uiServerBrowser.SetLANOnly( false );

	UI_ServerBrowser_Menu();
}

void UI_LanGame_Menu( void )
{
	uiServerBrowser.SetLANOnly( true );

	UI_ServerBrowser_Menu();
}

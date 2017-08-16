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
#include "ScrollList.h"
#include "keydefs.h"
#include "Switch.h"

#define ART_BANNER_INET		"gfx/shell/head_inetgames"
#define ART_BANNER_LAN		"gfx/shell/head_lan"

// Server browser
#define QMSB_GAME_LENGTH	25
#define QMSB_MAPNAME_LENGTH	20+QMSB_GAME_LENGTH
#define QMSB_MAXCL_LENGTH	10+QMSB_MAPNAME_LENGTH
#define QMSB_PING_LENGTH    10+QMSB_MAXCL_LENGTH

class CMenuServerBrowser: public CMenuFramework
{
public:
	CMenuServerBrowser() : CMenuFramework( "CMenuServerBrowser" ) { }
	virtual void Draw();

	void SetLANOnly( bool lanOnly ) { m_bLanOnly = lanOnly; }

private:
	virtual void _Init();
	virtual void _VidInit();

public:
	void GetGamesList( void );
	void ClearList( void );
	void RefreshList( void );

	DECLARE_EVENT_TO_MENU_METHOD( CMenuServerBrowser, RefreshList )

	static void JoinGame( CMenuBaseItem *pSelf, void *pExtra );

	char		gameDescription[UI_MAX_SERVERS][256];
	char		*gameDescriptionPtr[UI_MAX_SERVERS];

	CMenuPicButton joinGame;
	CMenuPicButton createGame;
	CMenuPicButton gameInfo;
	CMenuPicButton refresh;
	CMenuPicButton done;
	CMenuSwitch natOrDirect;

	CMenuYesNoMessageBox msgBox;
	CMenuScrollList	gameList;

	char  hintText[MAX_HINT_TEXT];
	int	  refreshTime;
	int   refreshTime2;

	bool m_bLanOnly;
};

static CMenuServerBrowser	uiServerBrowser;

/*
=================
CMenuServerBrowser::GetGamesList
=================
*/
void CMenuServerBrowser::GetGamesList( void )
{
	int		i;
	const char	*info;

	for( i = 0; i < uiStatic.numServers; i++ )
	{
		if( i >= UI_MAX_SERVERS )
			break;
		info = uiStatic.serverNames[i];

		gameDescription[i][0] = 0; // mark this string as empty

		StringConcat( gameDescription[i], Info_ValueForKey( info, "host" ), QMSB_GAME_LENGTH );
		AddSpaces( gameDescription[i], QMSB_GAME_LENGTH );

		StringConcat( gameDescription[i], Info_ValueForKey( info, "map" ), QMSB_MAPNAME_LENGTH );
		AddSpaces( gameDescription[i], QMSB_MAPNAME_LENGTH );

		StringConcat( gameDescription[i], Info_ValueForKey( info, "numcl" ), QMSB_MAXCL_LENGTH );
		StringConcat( gameDescription[i], "\\", QMSB_MAXCL_LENGTH );
		StringConcat( gameDescription[i], Info_ValueForKey( info, "maxcl" ), QMSB_MAXCL_LENGTH );
		AddSpaces( gameDescription[i], QMSB_MAXCL_LENGTH );

		char ping[10];
		snprintf( ping, 10, "%.f ms", uiStatic.serverPings[i] * 1000 );
		StringConcat( gameDescription[i], ping, QMSB_PING_LENGTH );
		AddSpaces( gameDescription[i], QMSB_PING_LENGTH );

		gameDescriptionPtr[i] = gameDescription[i];
	}

	for( ; i < UI_MAX_SERVERS; i++ )
		gameDescriptionPtr[i] = NULL;

	gameList.pszItemNames = (const char **)gameDescriptionPtr;
	gameList.iNumItems = 0; // reset it
	gameList.iCurItem = 0; // reset it

	if( !gameList.charSize.h )
		return; // to avoid divide integer by zero

	// count number of items
	while( gameList.pszItemNames[gameList.iNumItems] )
		gameList.iNumItems++;

	// calculate number of visible rows
	int h = gameList.size.h;
	UI_ScaleCoords( NULL, NULL, NULL, &h );

	gameList.iNumRows = (h / gameList.charSize.h) - 2;
	if( gameList.iNumRows > gameList.iNumItems )
		gameList.iNumRows = gameList.iNumItems;

	if( uiStatic.numServers )
		joinGame.iFlags &= ~QMF_GRAYED;
}

/*
=================
CMenuServerBrowser::JoinGame
=================
*/
void CMenuServerBrowser::JoinGame( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuServerBrowser *parent = (CMenuServerBrowser *)pSelf->Parent();

	if( parent->gameDescription[parent->gameList.iCurItem][0] == 0 )
		return;

	// prevent refresh during connect
	parent->refreshTime = uiStatic.realTime + 999999;
	//BUGBUG: ClientJoin not guaranted to return, need use ClientCmd instead!!!
	//BUGBUG: But server addres is known only as netadr_t here!!!
	EngFuncs::ClientJoin( uiStatic.serverAddresses[parent->gameList.iCurItem] );
	EngFuncs::ClientCmd( false, "menu_connectionprogress menu server\n" );
}

void CMenuServerBrowser::ClearList()
{
	uiStatic.numServers = 0;
	gameList.iNumItems = 0;
	gameList.iCurItem = 0;
	memset( gameDescriptionPtr, 0, sizeof( gameDescriptionPtr ) );
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
			refresh.iFlags |= QMF_GRAYED;
			if( uiStatic.realTime + 20000 < refreshTime )
				refreshTime = uiStatic.realTime + 20000;
		}
	}
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
		refresh.iFlags &= ~QMF_GRAYED;
	}

	// serverinfo has been changed update display
	if( uiStatic.updateServers )
	{
		GetGamesList ();
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
	// memset( &uiServerBrowser, 0, sizeof( CMenuServerBrowser ));

	StringConcat( hintText, "Name", QMSB_GAME_LENGTH );
	AddSpaces( hintText, QMSB_GAME_LENGTH );
	StringConcat( hintText, "Map", QMSB_MAPNAME_LENGTH );
	AddSpaces( hintText, QMSB_MAPNAME_LENGTH );
	StringConcat( hintText, "Players", QMSB_MAXCL_LENGTH );
	AddSpaces( hintText, QMSB_MAXCL_LENGTH );
	StringConcat( hintText, "Ping", QMSB_PING_LENGTH );
	AddSpaces( hintText, QMSB_PING_LENGTH );


	joinGame.iFlags |= QMF_GRAYED;
	joinGame.SetNameAndStatus( "Join game", "Join to selected game" );
	joinGame.SetPicture( PC_JOIN_GAME );
	joinGame.onActivatedClActive = msgBox.MakeOpenEvent();
	joinGame.onActivated = JoinGame;

	createGame.SetNameAndStatus( "Create game", "Create new Internet game" );
	createGame.SetPicture( PC_CREATE_GAME );
	SET_EVENT( createGame, onActivated )
	{
		if( ((CMenuServerBrowser*)pSelf->Parent())->m_bLanOnly )
			EngFuncs::CvarSetValue( "public", 0.0f );
		else EngFuncs::CvarSetValue( "public", 1.0f );

		UI_CreateGame_Menu();
	}
	END_EVENT( createGame, onActivated )

	gameInfo.iFlags |= QMF_GRAYED;
	gameInfo.SetNameAndStatus( "View game info", "Get detail game info" );
	gameInfo.SetPicture( PC_VIEW_GAME_INFO );
	// TODO: implement!

	refresh.SetNameAndStatus( "Refresh", "Refresh servers list" );
	refresh.SetPicture( PC_REFRESH );
	refresh.onActivated = RefreshListCb;

	done.SetNameAndStatus( "Done", "Return to main menu" );
	done.onActivated = HideCb;
	done.SetPicture( PC_DONE );

	msgBox.SetMessage( "Join a network game will exit\nany current game, OK to exit?" );
	msgBox.SetPositiveButton( "Ok", PC_OK );
	msgBox.HighlightChoice( 1 );
	msgBox.onPositive = JoinGame;
	msgBox.Link( this );

	gameList.SetCharSize( QM_SMALLFONT );
	gameList.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	gameList.pszItemNames = (const char **)gameDescriptionPtr;
	gameList.bFramedHintText = true;
	gameList.szName = hintText;

	natOrDirect.szLeftName = "Direct";
	natOrDirect.szRightName = "NAT";
	natOrDirect.eTextAlignment = QM_CENTER;
	natOrDirect.bMouseToggle = false;
	natOrDirect.LinkCvar( "cl_nat" );
	natOrDirect.iSelectColor = uiInputFgColor;
	// bit darker
	natOrDirect.iFgTextColor = uiInputFgColor - 0x00151515;
	SET_EVENT( natOrDirect, onChanged )
	{
		CMenuSwitch *self = (CMenuSwitch*)pSelf;
		CMenuServerBrowser *parent = (CMenuServerBrowser*)self->Parent();

		self->WriteCvar();
		parent->ClearList();
		parent->RefreshList();
	}
	END_EVENT( natOrDirect, onChanged )

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
		createGame.iFlags |= QMF_GRAYED;	// server.dll is missed - remote servers only


	AddItem( background );
	AddItem( banner );
	AddItem( joinGame );
	AddItem( createGame );
	AddItem( gameInfo );
	AddItem( refresh );
	AddItem( done );
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
	memset( gameDescription, 0, sizeof( gameDescription ));
	memset( gameDescriptionPtr, 0, sizeof( gameDescriptionPtr ));

	if( m_bLanOnly )
	{
		banner.SetPicture( ART_BANNER_LAN );
		createGame.szStatusText = ( "Create new LAN game" );
		natOrDirect.Hide();
	}
	else
	{
		banner.SetPicture( ART_BANNER_INET );
		createGame.szStatusText = ( "Create new Internet game" );
		natOrDirect.Show();
	}

	joinGame.SetCoord( 72, 230 );
	createGame.SetCoord( 72, 280 );
	gameInfo.SetCoord( 72, 330 );
	refresh.SetCoord( 72, 380 );
	done.SetCoord( 72, 430 );

	gameList.SetRect( 340, 255, 660, 440 );
	natOrDirect.SetCoord( 780, 180 );

	refreshTime = uiStatic.realTime + 500; // delay before update 0.5 sec
	refreshTime2 = uiStatic.realTime + 500;
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

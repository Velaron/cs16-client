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
#include "Field.h"
#include "CheckBox.h"
#include "PicButton.h"
#include "Table.h"
#include "Action.h"
#include "YesNoMessageBox.h"
#include "Table.h"

#define ART_BANNER		"gfx/shell/head_creategame"

#define MAPNAME_LENGTH	20
#define TITLE_LENGTH	20+MAPNAME_LENGTH

class CMenuMapListModel : public CMenuBaseModel
{
public:
	void Update();
	int GetColumns() const { return 2; }
	int GetRows() const { return m_iNumItems; }
	const char *GetCellText( int line, int column )
	{
		switch( column )
		{
		case 0: return mapName[line];
		case 1: return mapsDescription[line];
		}

		return NULL;
	}

	char		mapName[UI_MAXGAMES][64];
	char		mapsDescription[UI_MAXGAMES][64];
	int	m_iNumItems;
};

class CMenuCreateGame : public CMenuFramework
{
public:
	CMenuCreateGame() : CMenuFramework("CMenuCreateGame") { }
	static void Begin( CMenuBaseItem *pSelf, void *pExtra );

	char		*mapsDescriptionPtr[UI_MAXGAMES];

	CMenuField	maxClients;
	CMenuField	hostName;
	CMenuField	password;
	CMenuCheckBox   nat;
	CMenuCheckBox	hltv;
	CMenuCheckBox	dedicatedServer;

	// newgame prompt dialog
	CMenuYesNoMessageBox msgBox;

	CMenuTable        mapsList;
	CMenuMapListModel mapsListModel;

	CMenuPicButton *done;
private:
	virtual void _Init();
	virtual void _VidInit();
};

static CMenuCreateGame	uiCreateGame;

/*
=================
CMenuCreateGame::Begin
=================
*/
void CMenuCreateGame::Begin( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuCreateGame *menu = (CMenuCreateGame*)pSelf->Parent();
	int item = menu->mapsList.GetCurrentIndex();
	if( item < 0 || item > UI_MAXGAMES )
		return;

	const char *mapName;
	if( menu->mapsList.GetCurrentIndex() == 0 )
	{
		int idx = EngFuncs::RandomLong( 1, menu->mapsListModel.GetRows() );
		mapName = menu->mapsListModel.mapName[idx];
	}
	else
	{
		mapName = menu->mapsListModel.mapName[menu->mapsList.GetCurrentIndex()];
	}

	if( !EngFuncs::IsMapValid( mapName ))
		return;	// bad map

	if( EngFuncs::GetCvarFloat( "host_serverstate" ) )
	{
		if(	EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
			EngFuncs::HostEndGame( "end of the game" );
		else
			EngFuncs::HostEndGame( "starting new server" );
	}

	EngFuncs::CvarSetValue( "deathmatch", 1.0f );	// start deathmatch as default
	EngFuncs::CvarSetString( "defaultmap", mapName );
	EngFuncs::CvarSetValue( "sv_nat", EngFuncs::GetCvarFloat( "public" ) ? menu->nat.bChecked : 0 );
	menu->password.WriteCvar();
	menu->hostName.WriteCvar();
	menu->hltv.WriteCvar();
	menu->maxClients.WriteCvar();

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	// all done, start server
	if( menu->dedicatedServer.bChecked )
	{
		EngFuncs::WriteServerConfig( EngFuncs::GetCvarString( "servercfgfile" ));

		char cmd[128];
		sprintf( cmd, "#%s", gMenu.m_gameinfo.gamefolder );

		// NOTE: dedicated server will be executed "defaultmap"
		// from engine after restarting
		EngFuncs::ChangeInstance( cmd, "Starting dedicated server...\n" );
	}
	else
	{
		EngFuncs::WriteServerConfig( EngFuncs::GetCvarString( "lservercfgfile" ));

		char cmd[128];
		sprintf( cmd, "exec %s\n", EngFuncs::GetCvarString( "lservercfgfile" ) );
	
		EngFuncs::ClientCmd( TRUE, cmd );

		// dirty listenserver config form old xash may rewrite maxplayers
		EngFuncs::CvarSetValue( "maxplayers", atoi( menu->maxClients.GetBuffer() ));

		// hack: wait three frames allowing server to completely shutdown, reapply maxplayers and start new map
		sprintf( cmd, "endgame;menu_connectionprogress localserver;wait;wait;wait;maxplayers %i;latch;map %s\n", atoi( menu->maxClients.GetBuffer() ), mapName );
		EngFuncs::ClientCmd( FALSE, cmd );

	}
}

/*
=================
CMenuMapListModel::Update
=================
*/
void CMenuMapListModel::Update( void )
{
	char *afile;

	if( !EngFuncs::CreateMapsList( FALSE ) || (afile = (char *)EngFuncs::COM_LoadFile( "maps.lst", NULL )) == NULL )
	{
		uiCreateGame.done->SetGrayed( true );
		m_iNumItems = 0;
		Con_Printf( "Cmd_GetMapsList: can't open maps.lst\n" );
		return;
	}

	char *pfile = afile;
	char token[1024];
	int numMaps = 1;

	strcpy( mapName[0], "<Random Map>" );
	mapsDescription[0][0] = 0;
	
	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		if( numMaps >= UI_MAXGAMES ) break;

		Q_strncpy( mapName[numMaps], token, 64 );
		if(( pfile = EngFuncs::COM_ParseFile( pfile, token )) == NULL )
		{
			Q_strncpy( mapsDescription[numMaps], mapName[numMaps], 64 );
			break; // unexpected end of file
		}
		Q_strncpy( mapsDescription[numMaps], token, 64 );
		numMaps++;
	}

	if( !( numMaps - 1) ) uiCreateGame.done->SetGrayed( true );
	m_iNumItems = numMaps;
	EngFuncs::COM_FreeFile( afile );
}

/*
=================
CMenuCreateGame::Init
=================
*/
void CMenuCreateGame::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	nat.SetNameAndStatus( "NAT", "Use NAT Bypass instead of direct mode" );
	nat.bChecked = true;

	dedicatedServer.SetNameAndStatus( "Dedicated server", "faster, but you can't join the server from this machine" );

	hltv.SetNameAndStatus( "HLTV", "Enable HLTV mode in Multiplayer" );
	hltv.LinkCvar( "hltv" );

	// add them here, so "done" button can be used by mapsListModel::Update
	AddItem( background );
	AddItem( banner );
	CMenuPicButton *advOpt = AddButton( "Adv. Options", "Open the game advanced options menu", PC_ADV_OPT, UI_AdvServerOptions_Menu );
	advOpt->SetGrayed( !UI_AdvServerOptions_IsAvailable() );

	done = AddButton( "Ok", "Start the multiplayer game", PC_DONE, Begin );
	done->onActivatedClActive = msgBox.MakeOpenEvent();

	mapsList.SetCharSize( QM_SMALLFONT );
	mapsList.SetupColumn( 0, "Map", 0.5f );
	mapsList.SetupColumn( 1, "Title", 0.5f );
	mapsList.SetModel( &mapsListModel );

	hostName.szName = "Server Name:";
	hostName.iMaxLength = 28;
	hostName.LinkCvar( "hostname" );

	maxClients.iMaxLength = 2;
	maxClients.bNumbersOnly = true;
	maxClients.szName = "Max Players:";
	maxClients.LinkCvar( "maxplayers" );
	SET_EVENT_MULTI( maxClients.onCvarChange,
	{
		CMenuField *self = (CMenuField*)pSelf;
		if( !self->GetBuffer()[0] )
			return;
		int players = atoi( self->GetBuffer() );
		if( players <= 1 )
			self->SetBuffer( "2" );
		else if( players > 32 )
			self->SetBuffer( "32" );
	});

	SET_EVENT_MULTI( maxClients.onLostFocus,
	{
		CMenuField *self = (CMenuField*)pSelf;
		int players = atoi( self->GetBuffer() );
		if( players <= 1 )
			self->SetBuffer( "2" );
		else if( players > 32 )
			self->SetBuffer( "32" );
	});

	password.szName = "Password:";
	password.iMaxLength = 16;
	password.eTextAlignment = QM_CENTER;
	password.bHideInput = true;
	password.LinkCvar( "sv_password" );

	msgBox.onPositive = Begin;
	msgBox.SetMessage( "Starting a new game will exit any current game, OK to exit?" );
	msgBox.Link( this );

	AddButton( "Cancel", "Return to the previous menu", PC_CANCEL, VoidCb( &CMenuCreateGame::Hide ) );
	AddItem( maxClients );
	AddItem( hostName );
	AddItem( password );
	AddItem( dedicatedServer );
	AddItem( hltv );
	AddItem( nat );
	AddItem( mapsList );
}

void CMenuCreateGame::_VidInit()
{
	nat.SetCoord( 72, 585 );
	if( !EngFuncs::GetCvarFloat("public") )
		nat.Hide();
	else nat.Show();

	hltv.SetCoord( 72, 635 );
	dedicatedServer.SetCoord( 72, 685 );

	mapsList.SetRect( 590, 245, -20, 440 );

	hostName.SetRect( 350, 260, 205, 32 );
	maxClients.SetRect( 350, 360, 205, 32 );
	password.SetRect( 350, 460, 205, 32 );
}

/*
=================
CMenuCreateGame::Precache
=================
*/
void UI_CreateGame_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
CMenuCreateGame::Menu
=================
*/
void UI_CreateGame_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	UI_CreateGame_Precache();

	uiCreateGame.Show();
}

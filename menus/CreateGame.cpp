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
#include "ScrollList.h"
#include "Action.h"
#include "YesNoMessageBox.h"

#define ART_BANNER		"gfx/shell/head_creategame"

#define MAPNAME_LENGTH	20
#define TITLE_LENGTH	20+MAPNAME_LENGTH

class CMenuCreateGame : public CMenuFramework
{
public:
	CMenuCreateGame() : CMenuFramework("CMenuCreateGame") { }
private:
	virtual void _Init();
	virtual void _VidInit();
	static void Begin( CMenuBaseItem *pSelf, void *pExtra );
	void GetMapsList();

	char		mapName[UI_MAXGAMES][64];
	char		mapsDescription[UI_MAXGAMES][256];
	char		*mapsDescriptionPtr[UI_MAXGAMES];

	CMenuPicButton	advOptions;
	CMenuPicButton	done;
	CMenuPicButton	cancel;

	CMenuField	maxClients;
	CMenuField	hostName;
	CMenuField	password;
	CMenuCheckBox   nat;
	CMenuCheckBox	hltv;
	CMenuCheckBox	dedicatedServer;

	// newgame prompt dialog
	CMenuYesNoMessageBox msgBox;

	CMenuScrollList	mapsList;
	char		hintText[MAX_HINT_TEXT];
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
	int item = menu->mapsList.iCurItem;
	if( item < 0 || item > UI_MAXGAMES )
		return;

	const char *mapName = menu->mapName[menu->mapsList.iCurItem];

	if( !EngFuncs::IsMapValid( mapName ))
		return;	// bad map

	if( EngFuncs::GetCvarFloat( "host_serverstate" ) )
	{
		if(	EngFuncs::GetCvarFloat( "maxplayers" ) == 1 )
			EngFuncs::HostEndGame( "end of the game" );
		else
			EngFuncs::HostEndGame( "starting new server" );
	}

	EngFuncs::CvarSetValue( "deathmatch", 1.0f );	// start deathmatch as default
	EngFuncs::CvarSetString( "defaultmap", mapName );
	EngFuncs::CvarSetValue( "sv_nat", EngFuncs::GetCvarFloat( "public" ) ? menu->nat.bChecked : 0 );
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
CMenuCreateGame::GetMapsList
=================
*/
void CMenuCreateGame::GetMapsList( void )
{
	char *afile;

	if( !EngFuncs::CreateMapsList( FALSE ) || (afile = (char *)EngFuncs::COM_LoadFile( "maps.lst", NULL )) == NULL )
	{
		done.iFlags |= QMF_GRAYED;
		mapsList.pszItemNames = (const char **)mapsDescriptionPtr;
		Con_Printf( "Cmd_GetMapsList: can't open maps.lst\n" );
		return;
	}

	char *pfile = afile;
	char token[1024];
	int numMaps = 0;
	
	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		if( numMaps >= UI_MAXGAMES ) break;
		StringConcat( mapName[numMaps], token, sizeof( mapName[0] ));
		StringConcat( mapsDescription[numMaps], token, MAPNAME_LENGTH );
		AddSpaces( mapsDescription[numMaps], MAPNAME_LENGTH );
		if(( pfile = EngFuncs::COM_ParseFile( pfile, token )) == NULL ) break; // unexpected end of file
		StringConcat( mapsDescription[numMaps], token, TITLE_LENGTH );
		AddSpaces( mapsDescription[numMaps], TITLE_LENGTH );
		mapsDescriptionPtr[numMaps] = mapsDescription[numMaps];
		numMaps++;
	}

	if( !numMaps ) done.iFlags |= QMF_GRAYED;

	for( ; numMaps < UI_MAXGAMES; numMaps++ ) mapsDescriptionPtr[numMaps] = NULL;
	mapsList.pszItemNames = (const char **)mapsDescriptionPtr;
	EngFuncs::COM_FreeFile( afile );
}

/*
=================
CMenuCreateGame::Init
=================
*/
void CMenuCreateGame::_Init( void )
{
	// memset( &uiCreateGame, 0, sizeof( uiCreateGame_t ));

	StringConcat( hintText, "Map", MAPNAME_LENGTH );
	AddSpaces( hintText, MAPNAME_LENGTH );
	StringConcat( hintText, "Title", TITLE_LENGTH );
	AddSpaces( hintText, TITLE_LENGTH );

	banner.SetPicture( ART_BANNER );

	advOptions.SetNameAndStatus( "Adv. Options", "Open the game advanced options menu" );
	advOptions.SetPicture( PC_ADV_OPT );
	advOptions.onActivated = UI_AdvServerOptions_Menu;

	done.SetNameAndStatus( "Ok", "Start the multiplayer game" );
	done.SetPicture( PC_OK );
	done.onActivated = Begin;
	done.onActivatedClActive = msgBox.MakeOpenEvent();

	cancel.SetNameAndStatus( "Cancel", "Return to the previous menu" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onActivated = HideCb;

	nat.SetNameAndStatus( "NAT", "Use NAT Bypass instead of direct mode" );
	nat.bChecked = true;

	dedicatedServer.SetNameAndStatus( "Dedicated server", "faster, but you can't join the server from this machine" );

	hltv.SetNameAndStatus( "HLTV", "Enable HLTV mode in Multiplayer" );
	hltv.LinkCvar( "hltv" );

	mapsList.SetCharSize( QM_SMALLFONT );
	mapsList.szName = hintText;
	GetMapsList();

	hostName.szName = "Server Name:";
	hostName.iMaxLength = 28;
	hostName.LinkCvar( "hostname" );

	maxClients.iMaxLength = 2;
	maxClients.bNumbersOnly = true;
	maxClients.szName = "Max Players:";
	maxClients.LinkCvar( "maxplayers" );
	SET_EVENT( maxClients, onCvarChange )
	{
		CMenuField *self = (CMenuField*)pSelf;
		if( atoi(self->GetBuffer()) <= 1 )
			self->SetBuffer( "8" );
	}
	END_EVENT( maxClients, onCvarChange )

	password.szName = "Password:";
	password.iMaxLength = 16;
	password.eTextAlignment = QM_CENTER;
	password.bHideInput = true;
	// TODO: This field is completely ignored. Add password option to the engine!

	msgBox.onPositive = Begin;
	msgBox.SetMessage( "Starting a new game will exit\nany current game, OK to exit?" );
	msgBox.Link( this );

	AddItem( background );
	AddItem( banner );
	AddItem( advOptions );
	AddItem( done );
	AddItem( cancel );
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
	advOptions.SetCoord( 72, 230 );
	done.SetCoord( 72, 280 );
	cancel.SetCoord( 72, 330 );

	nat.SetCoord( 72, 585 );
	if( !EngFuncs::GetCvarFloat("public") )
		nat.Hide();
	else nat.Show();

	hltv.SetCoord( 72, 635 );
	dedicatedServer.SetCoord( 72, 685 );

	mapsList.SetRect( 590, 245, 410, 440 );

	hostName.SetRect( 350, 260, 205, 32 );
	maxClients.SetRect( 350, 360, 205, 32 );
	password.SetRect( 350, 460, 205, 32 );

	advOptions.SetGrayed( !UI_AdvServerOptions_IsAvailable() );
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

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
#include "Action.h"
#include "ScrollList.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"

#define ART_BANNER		"gfx/shell/head_custom"

#define MAX_MODS		512	// engine limit

#define TYPE_LENGTH		16
#define NAME_SPACE		4
#define NAME_LENGTH		32+TYPE_LENGTH
#define VER_LENGTH		6+NAME_LENGTH
#define SIZE_LENGTH		10+VER_LENGTH

class CMenuCustomGame: public CMenuFramework
{
public:
	CMenuCustomGame() : CMenuFramework("CMenuCustomGame") { }

private:
	static void ChangeGame( CMenuBaseItem *pSelf, void *pExtra );
	static void Go2Site( CMenuBaseItem *pSelf, void *pExtra );
	static void UpdateExtras(CMenuBaseItem *pSelf, void *pExtra);
	virtual void _Init( );
	virtual void _VidInit( );

	void GetModList( );

	char		modsDir[MAX_MODS][64];
	char		modsWebSites[MAX_MODS][256];
	char		modsDescription[MAX_MODS][256];
	char		*modsDescriptionPtr[MAX_MODS];

	CMenuPicButton	load;
	CMenuPicButton	go2url;
	CMenuPicButton	done;

	// prompt dialog
	CMenuYesNoMessageBox msgBox;

	CMenuScrollList	modList;
	char		hintText[MAX_HINT_TEXT];
};

static CMenuCustomGame	uiCustomGame;

void CMenuCustomGame::ChangeGame(CMenuBaseItem *pSelf, void *pExtra)
{
	CMenuCustomGame *parent = (CMenuCustomGame*)pSelf->Parent();

	char cmd[128];
	sprintf( cmd, "game %s\n", (const char*)pExtra );
	EngFuncs::ClientCmd( FALSE, cmd );
}

void CMenuCustomGame::Go2Site(CMenuBaseItem *pSelf, void *pExtra)
{
	const char *url = (const char *)pExtra;
	if( url[0] )
		EngFuncs::ShellExecute( url, NULL, false );
}

void CMenuCustomGame::UpdateExtras( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuCustomGame *parent = (CMenuCustomGame*)pSelf->Parent();
	CMenuScrollList *self = (CMenuScrollList*)pSelf;

	int i = self->iCurItem;

	if( !stricmp( parent->modsDir[i], gMenu.m_gameinfo.gamefolder ) )
		parent->load.iFlags |= QMF_GRAYED;
	else
		parent->load.iFlags &= ~QMF_GRAYED;

	parent->go2url.onActivated.pExtra = parent->modsWebSites[i];
	if( parent->modsWebSites[i][0] )
		parent->go2url.iFlags &= ~QMF_GRAYED;
	else
		parent->go2url.iFlags |= QMF_GRAYED;

	parent->msgBox.onPositive.pExtra = parent->modsDir[i];
	parent->load.onActivated.pExtra = parent->modsDir[i];
}

/*
=================
UI_CustomGame_GetModList
=================
*/
void CMenuCustomGame::GetModList( void )
{
	int	numGames, i;
	GAMEINFO	**games;

	games = EngFuncs::GetGamesList( &numGames );

	for( i = 0; i < numGames; i++ )
	{
		strncpy( modsDir[i], games[i]->gamefolder, sizeof( modsDir[i] ));
		strncpy( modsWebSites[i], games[i]->game_url, sizeof( modsWebSites[i] ));

		if( games[i]->type[0] )
		{
			StringConcat( modsDescription[i], games[i]->type, TYPE_LENGTH );
			AddSpaces( modsDescription[i], TYPE_LENGTH );
		}
		else
		{
			AddSpaces( modsDescription[i], TYPE_LENGTH+1 );
		}

		if( ColorStrlen( games[i]->title ) > 31 ) // NAME_LENGTH
		{
			StringConcat( modsDescription[i], games[i]->title, ( NAME_LENGTH - NAME_SPACE ));
			StringConcat( modsDescription[i], "...", NAME_LENGTH );
		}
		else StringConcat( modsDescription[i], games[i]->title, NAME_LENGTH );

		AddSpaces( modsDescription[i], NAME_LENGTH );
		StringConcat( modsDescription[i], games[i]->version, VER_LENGTH );
		AddSpaces( modsDescription[i], VER_LENGTH );
		if( games[i]->size[0] )
			StringConcat( modsDescription[i], games[i]->size, SIZE_LENGTH );
		else StringConcat( modsDescription[i], "0.0 Mb", SIZE_LENGTH );
		AddSpaces( modsDescription[i], SIZE_LENGTH );
		modsDescriptionPtr[i] = modsDescription[i];

		if( !strcmp( gMenu.m_gameinfo.gamefolder, games[i]->gamefolder ))
			modList.iCurItem = i;
	}

	for( ; i < MAX_MODS; i++ )
		modsDescriptionPtr[i] = NULL;

	modList.pszItemNames = (const char **)modsDescriptionPtr;

	// see if the load button should be grayed
	if( !stricmp( gMenu.m_gameinfo.gamefolder, modsDir[modList.iCurItem] ))
		load.iFlags |= QMF_GRAYED;
	else
		load.iFlags &= ~QMF_GRAYED;

	if( modsWebSites[modList.iCurItem][0] == '\0' )
		go2url.iFlags |= QMF_GRAYED;
	else
	{
		go2url.iFlags &= ~QMF_GRAYED;
		go2url.onActivated.pExtra = modsWebSites[modList.iCurItem];
	}
}

/*
=================
UI_CustomGame_Init
=================
*/
void CMenuCustomGame::_Init( void )
{
	StringConcat( hintText, "Type", TYPE_LENGTH );
	AddSpaces( hintText, TYPE_LENGTH );
	StringConcat( hintText, "Name", NAME_LENGTH );
	AddSpaces( hintText, NAME_LENGTH );
	StringConcat( hintText, "Ver", VER_LENGTH );
	AddSpaces( hintText, VER_LENGTH );
	StringConcat( hintText, "Size", SIZE_LENGTH );
	AddSpaces( hintText, SIZE_LENGTH );

	banner.SetPicture( ART_BANNER );

	load.SetNameAndStatus("Activate", "Activate selected custom game" );
	load.SetPicture( PC_ACTIVATE );
	load.onActivatedClActive = msgBox.MakeOpenEvent();
	load.onActivated = ChangeGame;

	go2url.SetNameAndStatus( "Visit web site", "Visit the web site of game developers" );
	go2url.SetPicture( PC_VISIT_WEB_SITE );
	go2url.onActivated = Go2Site;

	done.SetNameAndStatus( "Done", "Return to main menu" );
	done.SetPicture( PC_DONE );
	done.onActivated = HideCb;

	modList.onChanged = UpdateExtras;
	modList.szName = hintText;
	GetModList();

	msgBox.SetMessage( "Leave current game?" );
	msgBox.onPositive = ChangeGame;
	msgBox.Link( this );

	AddItem( background );
	AddItem( banner );
	AddItem( load );
	AddItem( go2url );
	AddItem( done );
	AddItem( modList );
}

void CMenuCustomGame::_VidInit()
{
	load.SetCoord( 72, 230 );
	go2url.SetCoord( 72, 280 );
	done.SetCoord( 72, 330 );
	modList.SetRect( 300, 255, 640, 440 );
}

/*
=================
UI_CustomGame_Precache
=================
*/
void UI_CustomGame_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_CustomGame_Menu
=================
*/
void UI_CustomGame_Menu( void )
{
	// current instance is not support game change
	if( !EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		return;

	UI_CustomGame_Precache();
	uiCustomGame.Show();
}

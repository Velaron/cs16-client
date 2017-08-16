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
#include "ScrollList.h"
#include "Action.h"
#include "YesNoMessageBox.h"

#define ART_BANNER_LOAD "gfx/shell/head_load"
#define ART_BANNER_SAVE "gfx/shell/head_save"

#define LEVELSHOT_X		72
#define LEVELSHOT_Y		400
#define LEVELSHOT_W		192
#define LEVELSHOT_H		160

#define TIME_LENGTH		20
#define NAME_LENGTH		32+TIME_LENGTH
#define GAMETIME_LENGTH	15+NAME_LENGTH

class CMenuSavePreview : public CMenuBaseItem
{
public:
	CMenuSavePreview()
	{
		szName = NULL;
		iFlags = QMF_INACTIVE;
	}

	virtual void VidInit()
	{
		CalcPosition();
		m_scSize = size.Scale();
	}

	virtual void Draw()
	{
		const char *fallback = "{GRAF001";

		if( szName && *szName )
		{
			char saveshot[128];

			snprintf( saveshot, sizeof( saveshot ),
				"save/%s.bmp", szName );

			if( EngFuncs::FileExists( saveshot ))
				UI_DrawPic( m_scPos, m_scSize, uiColorWhite, saveshot );
			else
				UI_DrawPicAdditive( m_scPos, m_scSize, uiColorWhite, fallback );
		}
		else
			UI_DrawPic( m_scPos, m_scSize, uiColorWhite, fallback );

		// draw the rectangle
		UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );

	}
};

class CMenuLoadGame : public CMenuFramework
{
public:
	// true to turn this menu into save mode, false to turn into load mode
	void SetSaveMode( bool saveMode );

private:
	virtual void _Init( void );
	virtual void _VidInit( void );

	void DeleteDialog();
	DECLARE_EVENT_TO_MENU_METHOD( CMenuLoadGame, DeleteDialog )
	bool m_fSaveMode;

public:
	void GetGameList();

	char		saveName[UI_MAXGAMES][CS_SIZE];
	char		delName[UI_MAXGAMES][CS_SIZE];
	char		saveDescription[UI_MAXGAMES][95];
	char		*saveDescriptionPtr[UI_MAXGAMES];

	CMenuPicButton	load;
	CMenuPicButton  save;
	CMenuPicButton	remove;
	CMenuPicButton	cancel;

	CMenuScrollList	savesList;

	CMenuSavePreview	levelShot;
	char		hintText[MAX_HINT_TEXT];

	// prompt dialog
	CMenuYesNoMessageBox msgBox;
};

static CMenuLoadGame		uiLoadGame;

/*
=================
UI_LoadGame_GetGameList
=================
*/
void CMenuLoadGame::GetGameList( void )
{
	char	comment[256];
	char	**filenames;
	int	i = 0, j, numFiles;

	filenames = EngFuncs::GetFilesList( "save/*.sav", &numFiles, TRUE );

	// sort the saves in reverse order (oldest past at the end)
	qsort( filenames, numFiles, sizeof( char* ), (cmpfunc)COM_CompareSaves );
	memset( saveDescription, 0, sizeof( saveDescription ) );

	if ( m_fSaveMode && CL_IsActive() )
	{
		// create new entry for current save game
		strncpy( saveName[i], "new", CS_SIZE );
		StringConcat( saveDescription[i], "Current", TIME_LENGTH );
		AddSpaces( saveDescription[i], TIME_LENGTH ); // fill remaining entries
		StringConcat( saveDescription[i], "New Saved Game", NAME_LENGTH );
		AddSpaces( saveDescription[i], NAME_LENGTH );
		StringConcat( saveDescription[i], "New", GAMETIME_LENGTH );
		AddSpaces( saveDescription[i], GAMETIME_LENGTH );
		saveDescriptionPtr[i] = saveDescription[i];
		i++;
	}

	for ( j = 0; j < numFiles; i++, j++ )
	{
		if( i >= UI_MAXGAMES ) break;
		
		if( !EngFuncs::GetSaveComment( filenames[j], comment ))
		{
			if( comment[0] )
			{
				// get name string even if not found - SV_GetComment can be mark saves
				// as <CORRUPTED> <OLD VERSION> etc
				AddSpaces( saveDescription[i], TIME_LENGTH );
				StringConcat( saveDescription[i], comment, NAME_LENGTH );
				AddSpaces( saveDescription[i], NAME_LENGTH );
				saveDescriptionPtr[i] = saveDescription[i];
				COM_FileBase( filenames[j], saveName[i] );
				COM_FileBase( filenames[j], delName[i] );
			}
			else saveDescriptionPtr[j] = NULL;
			continue;
		}

		// strip path, leave only filename (empty slots doesn't have savename)
		COM_FileBase( filenames[j], saveName[i] );
		COM_FileBase( filenames[j], delName[i] );

		// fill save desc
		StringConcat( saveDescription[i], comment + CS_SIZE, TIME_LENGTH );
		StringConcat( saveDescription[i], " ", TIME_LENGTH );
		StringConcat( saveDescription[i], comment + CS_SIZE + CS_TIME, TIME_LENGTH );
		AddSpaces( saveDescription[i], TIME_LENGTH );// fill remaining entries
		StringConcat( saveDescription[i], comment, NAME_LENGTH );
		AddSpaces( saveDescription[i], NAME_LENGTH );
		StringConcat( saveDescription[i], comment + CS_SIZE + (CS_TIME * 2), GAMETIME_LENGTH );
		AddSpaces( saveDescription[i], GAMETIME_LENGTH );
		saveDescriptionPtr[i] = saveDescription[i];
	}

	for ( ; i < UI_MAXGAMES; i++ )
		saveDescriptionPtr[i] = NULL;

	savesList.pszItemNames = (const char **)saveDescriptionPtr;

	if ( saveName[0][0] == 0 )
		load.iFlags |= QMF_GRAYED;
	else
	{
		levelShot.szName = saveName[0];
		load.iFlags &= ~QMF_GRAYED;
	}

	if ( saveName[0][0] == 0 || !CL_IsActive() )
		save.iFlags |= QMF_GRAYED;
	else save.iFlags &= ~QMF_GRAYED;

	if ( delName[0][0] == 0 )
		remove.iFlags |= QMF_GRAYED;
	else remove.iFlags &= ~QMF_GRAYED;
}

/*
=================
UI_LoadGame_Init
=================
*/
void CMenuLoadGame::_Init( void )
{

	StringConcat( hintText, "Time", TIME_LENGTH );
	AddSpaces( hintText, TIME_LENGTH );
	StringConcat( hintText, "Game", NAME_LENGTH );
	AddSpaces( hintText, NAME_LENGTH );
	StringConcat( hintText, "Elapsed time", GAMETIME_LENGTH );
	AddSpaces( hintText, GAMETIME_LENGTH );

	save.SetNameAndStatus( "Save", "Save curret game" );
	save.SetPicture( PC_SAVE_GAME );
	SET_EVENT( save, onActivated )
	{
		CMenuLoadGame *parent = (CMenuLoadGame*)pSelf->Parent();
		const char *saveName = parent->saveName[parent->savesList.iCurItem];
		if( saveName[0] )
		{
			char	cmd[128];

			sprintf( cmd, "save/%s.bmp", saveName );
			EngFuncs::PIC_Free( cmd );

			sprintf( cmd, "save \"%s\"\n", saveName );

			EngFuncs::ClientCmd( FALSE, cmd );

			UI_CloseMenu();
		}
	}
	END_EVENT( save, onActivated )

	load.SetNameAndStatus( "Load", "Load saved game" );
	load.SetPicture( PC_LOAD_GAME );
	SET_EVENT( load, onActivated )
	{
		CMenuLoadGame *parent = (CMenuLoadGame*)pSelf->Parent();
		const char *saveName = parent->saveName[parent->savesList.iCurItem];
		if( saveName[0] )
		{
			char	cmd[128];
			sprintf( cmd, "load \"%s\"\n", saveName );

			EngFuncs::StopBackgroundTrack( );

			EngFuncs::ClientCmd( FALSE, cmd );

			UI_CloseMenu();
		}
	}
	END_EVENT( load, onActivated )

	remove.SetNameAndStatus( "Delete", "Delete saved game" );
	remove.SetPicture( PC_DELETE );
	remove.onActivated = msgBox.MakeOpenEvent();

	cancel.SetNameAndStatus( "Cancel", "Return back to main menu" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onActivated = HideCb;

	savesList.szName = hintText;
	SET_EVENT( savesList, onChanged )
	{
		CMenuScrollList *self = (CMenuScrollList*)pSelf;
		CMenuLoadGame *parent = (CMenuLoadGame*)self->Parent();

		parent->levelShot.szName = parent->saveName[self->iCurItem];
	}
	END_EVENT( savesList, onChanged )
	savesList.onDeleteEntry = msgBox.MakeOpenEvent();

	msgBox.SetMessage( "Delete this save?" );
	SET_EVENT( msgBox, onPositive )
	{
		CMenuLoadGame *parent = (CMenuLoadGame*)pSelf->Parent();
		const char *delName = parent->delName[parent->savesList.iCurItem];

		if( delName[0] )
		{
			char	cmd[128];
			sprintf( cmd, "killsave \"%s\"\n", delName );

			EngFuncs::ClientCmd( TRUE, cmd );

			sprintf( cmd, "save/%s.bmp", delName );
			EngFuncs::PIC_Free( cmd );

			parent->GetGameList();
		}
	}
	END_EVENT( msgBox, onPositive )
	msgBox.Link( this );

	AddItem( background );
	AddItem( banner );
	AddItem( load );
	AddItem( save );
	AddItem( remove );
	AddItem( cancel );
	AddItem( levelShot );
	AddItem( savesList );
}

void CMenuLoadGame::_VidInit()
{
	save.SetCoord( 72, 230 );
	load.SetCoord( 72, 230 );
	remove.SetCoord( 72, 280 );
	cancel.SetCoord( 72, 330 );
	levelShot.SetRect( LEVELSHOT_X, LEVELSHOT_Y, LEVELSHOT_W, LEVELSHOT_H );
	savesList.SetRect( 360, 255, 640, 440 );
	GetGameList();
}

void CMenuLoadGame::SetSaveMode(bool saveMode)
{
	m_fSaveMode = saveMode;
	if( saveMode )
	{
		banner.SetPicture( ART_BANNER_SAVE );
		save.SetVisibility( true );
		load.SetVisibility( false );
		szName = "CMenuSaveGame";
	}
	else
	{
		banner.SetPicture( ART_BANNER_LOAD );
		save.SetVisibility( false );
		load.SetVisibility( true );
		szName = "CMenuLoadGame";
	}
	GetGameList();
}

/*
=================
UI_LoadGame_Precache
=================
*/
void UI_LoadGame_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER_SAVE );
	EngFuncs::PIC_Load( ART_BANNER_LOAD );
}

/*
=================
UI_LoadGame_Menu
=================
*/
void UI_LoadGame_Menu( void )
{
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		// completely ignore save\load menus for multiplayer_only
		return;
	}

	if( !EngFuncs::CheckGameDll( )) return;

	UI_LoadGame_Precache();
	uiLoadGame.SetSaveMode(false);
	uiLoadGame.Show();
}

void UI_SaveGame_Menu( void )
{
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		// completely ignore save\load menus for multiplayer_only
		return;
	}

	if( !EngFuncs::CheckGameDll( )) return;

	UI_LoadGame_Precache();
	uiLoadGame.SetSaveMode(true);
	uiLoadGame.Show();
}

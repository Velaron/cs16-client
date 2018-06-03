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
#include "Table.h"
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

#define MAX_CELLSTRING 32

class CMenuSavePreview : public CMenuBaseItem
{
public:
	CMenuSavePreview() : CMenuBaseItem()
	{
		iFlags = QMF_INACTIVE;
	}

	virtual void Draw();
};

class CMenuSavesListModel : public CMenuBaseModel
{
public:
	void Update();
	int GetColumns() const
	{
		// time, name, gametime
		return 3;
	}
	int GetRows() const
	{
		return m_iNumItems;
	}
	const char *GetCellText( int line, int column )
	{
		return m_szCells[line][column];
	}
	ETextAlignment GetAlignmentForColumn(int column) const
	{
		if( column == 2 )
			return QM_RIGHT;
		return QM_LEFT;
	}
	void OnDeleteEntry( int line );

	char		saveName[UI_MAXGAMES][CS_SIZE];
	char		delName[UI_MAXGAMES][CS_SIZE];

private:
	char		m_szCells[UI_MAXGAMES][3][MAX_CELLSTRING];
	int			m_iNumItems;
};

static class CMenuLoadGame : public CMenuFramework
{
public:
	CMenuLoadGame() : CMenuFramework( "CMenuLoadGame" ) { }

	// true to turn this menu into save mode, false to turn into load mode
	void SetSaveMode( bool saveMode );
	bool IsSaveMode() { return m_fSaveMode; }
	void UpdateList() { savesListModel.Update(); }

private:
	virtual void _Init( void );

	void LoadGame();
	void SaveGame();
	void UpdateGame();
	void DeleteGame();

	CMenuPicButton	load;
	CMenuPicButton  save;
	CMenuPicButton	remove;
	CMenuPicButton	cancel;

	CMenuTable	savesList;

	CMenuSavePreview	levelShot;
	bool m_fSaveMode;
	char		hintText[MAX_HINT_TEXT];

	// prompt dialog
	CMenuYesNoMessageBox msgBox;
	CMenuSavesListModel savesListModel;

	friend class CMenuSavesListModel;
} uiLoadGame;

void CMenuSavePreview::Draw()
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
		UI_DrawPicAdditive( m_scPos, m_scSize, uiColorWhite, fallback );

	// draw the rectangle
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}

/*
=================
CMenuSavesListModel::Update
=================
*/
void CMenuSavesListModel::Update( void )
{
	char	comment[256];
	char	**filenames;
	int	i = 0, j, numFiles;

	filenames = EngFuncs::GetFilesList( "save/*.sav", &numFiles, TRUE );

	// sort the saves in reverse order (oldest past at the end)
	qsort( filenames, numFiles, sizeof( char* ), (cmpfunc)COM_CompareSaves );

	if ( uiLoadGame.IsSaveMode() && CL_IsActive() )
	{
		// create new entry for current save game
		Q_strncpy( saveName[i], "new", CS_SIZE );
		Q_strncpy( delName[i], "", CS_SIZE );
		strcpy( m_szCells[i][0], "Current" );
		strcpy( m_szCells[i][1], "New Saved Game" );
		strcpy( m_szCells[i][2], "New" );
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
				Q_strncpy( m_szCells[i][0], comment, MAX_CELLSTRING );
				m_szCells[i][1][0] = 0;
				m_szCells[i][2][0] = 0;
				COM_FileBase( filenames[j], saveName[i] );
				COM_FileBase( filenames[j], delName[i] );
			}
			continue;
		}

		// strip path, leave only filename (empty slots doesn't have savename)
		COM_FileBase( filenames[j], saveName[i] );
		COM_FileBase( filenames[j], delName[i] );

		// fill save desc
		snprintf( m_szCells[i][0], MAX_CELLSTRING, "%s %s", comment + CS_SIZE, comment + CS_SIZE + CS_TIME );
		Q_strncpy( m_szCells[i][1], comment, MAX_CELLSTRING );
		Q_strncpy( m_szCells[i][2], comment + CS_SIZE + (CS_TIME * 2), MAX_CELLSTRING );
	}

	m_iNumItems = i;

	if ( saveName[0][0] == 0 )
	{
		uiLoadGame.load.SetGrayed( true );
	}
	else
	{
		uiLoadGame.levelShot.szName = saveName[0];
		uiLoadGame.load.SetGrayed( false );
	}

	if ( saveName[0][0] == 0 || !CL_IsActive() )
		uiLoadGame.save.SetGrayed( true );
	else uiLoadGame.save.SetGrayed( false );

	if ( delName[0][0] == 0 )
		uiLoadGame.remove.SetGrayed( true );
	else uiLoadGame.remove.SetGrayed( false );
}

void CMenuSavesListModel::OnDeleteEntry(int line)
{
	uiLoadGame.msgBox.Show();
}

/*
=================
UI_LoadGame_Init
=================
*/
void CMenuLoadGame::_Init( void )
{
	save.SetNameAndStatus( "Save", "Save curret game" );
	save.SetPicture( PC_SAVE_GAME );
	save.onActivated = VoidCb( &CMenuLoadGame::SaveGame );
	save.SetCoord( 72, 230 );

	load.SetNameAndStatus( "Load", "Load saved game" );
	load.SetPicture( PC_LOAD_GAME );
	load.onActivated = VoidCb( &CMenuLoadGame::LoadGame );
	load.SetCoord( 72, 230 );

	remove.SetNameAndStatus( "Delete", "Delete saved game" );
	remove.SetPicture( PC_DELETE );
	remove.onActivated = msgBox.MakeOpenEvent();
	remove.SetCoord( 72, 280 );

	cancel.SetNameAndStatus( "Cancel", "Return back to main menu" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onActivated = VoidCb( &CMenuLoadGame::Hide );
	cancel.SetCoord( 72, 330 );

	savesList.szName = hintText;
	savesList.onChanged = VoidCb( &CMenuLoadGame::UpdateGame );
	// savesList.onDeleteEntry = msgBox.MakeOpenEvent();
	savesList.SetupColumn( 0, "Time", 0.30f );
	savesList.SetupColumn( 1, "Game", 0.55f );
	savesList.SetupColumn( 2, "Elapsed Time", 0.15f );

	savesList.SetModel( &savesListModel );
	savesList.SetCharSize( QM_SMALLFONT );
	savesList.SetRect( 360, 255, -20, 440 );

	msgBox.SetMessage( "Delete this save?" );
	msgBox.onPositive = VoidCb( &CMenuLoadGame::DeleteGame );
	msgBox.Link( this );

	levelShot.SetRect( LEVELSHOT_X, LEVELSHOT_Y, LEVELSHOT_W, LEVELSHOT_H );

	AddItem( background );
	AddItem( banner );
	AddItem( load );
	AddItem( save );
	AddItem( remove );
	AddItem( cancel );
	AddItem( levelShot );
	AddItem( savesList );
}

void CMenuLoadGame::LoadGame()
{
	const char *saveName = savesListModel.saveName[savesList.GetCurrentIndex()];
	if( saveName[0] )
	{
		char	cmd[128];
		sprintf( cmd, "load \"%s\"\n", saveName );

		EngFuncs::StopBackgroundTrack( );

		EngFuncs::ClientCmd( FALSE, cmd );

		UI_CloseMenu();
	}
}

void CMenuLoadGame::SaveGame()
{
	const char *saveName = savesListModel.saveName[savesList.GetCurrentIndex()];
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

void CMenuLoadGame::UpdateGame()
{
	// first item is for creating new saves
	if( IsSaveMode() && savesList.GetCurrentIndex() == 0 )
	{
		remove.SetGrayed( true );
		levelShot.szName = NULL;
	}
	else
	{
		remove.SetGrayed( false );
		levelShot.szName = savesListModel.saveName[savesList.GetCurrentIndex()];
	}
}

void CMenuLoadGame::DeleteGame()
{
	const char *delName = savesListModel.delName[savesList.GetCurrentIndex()];

	if( delName[0] )
	{
		char	cmd[128];
		sprintf( cmd, "killsave \"%s\"\n", delName );

		EngFuncs::ClientCmd( TRUE, cmd );

		sprintf( cmd, "save/%s.bmp", delName );
		EngFuncs::PIC_Free( cmd );

		savesListModel.Update();
	}
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

void UI_LoadSaveGame_Menu( bool saveMode )
{
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		// completely ignore save\load menus for multiplayer_only
		return;
	}

	if( !EngFuncs::CheckGameDll( )) return;

	UI_LoadGame_Precache();
	uiLoadGame.Show();
	uiLoadGame.SetSaveMode( saveMode );
	uiLoadGame.UpdateList();
}

/*
=================
UI_LoadGame_Menu
=================
*/
void UI_LoadGame_Menu( void )
{
	UI_LoadSaveGame_Menu( false );
}

void UI_SaveGame_Menu( void )
{
	UI_LoadSaveGame_Menu( true );
}

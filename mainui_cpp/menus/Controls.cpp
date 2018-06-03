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
#include "Action.h"
#include "YesNoMessageBox.h"
#include "MessageBox.h"
#include "Table.h"

#define ART_BANNER		"gfx/shell/head_controls"

#define MAX_KEYS		256
#define CMD_LENGTH		38
#define KEY1_LENGTH		20+CMD_LENGTH
#define KEY2_LENGTH		20+KEY1_LENGTH

class CMenuKeysModel : public CMenuBaseModel
{
public:
	void Update();
	void OnActivateEntry( int line );
	void OnDeleteEntry( int line );
	int GetRows() const
	{
		return m_iNumItems;
	}
	int GetColumns() const
	{
		return 3; // cmd, key1, key2
	}
	const char *GetCellText( int line, int column )
	{
		switch( column )
		{
		case 0: return name[line];
		case 1: return firstKey[line];
		case 2: return secondKey[line];
		}

		return NULL;
	}

	bool IsCellTextWrapped( int line, int column )
	{
		return IsLineUsable( line );
	}

	bool IsLineUsable( int line )
	{
		return keysBind[line][0] != 0;
	}

	char name[MAX_KEYS][CMD_LENGTH];
	char keysBind[MAX_KEYS][CMD_LENGTH];
	char firstKey[MAX_KEYS][20];
	char secondKey[MAX_KEYS][20];
	int m_iNumItems;
};

static class CMenuControls : public CMenuFramework
{
public:
	CMenuControls() : CMenuFramework("CMenuControls") { }

	void _Init();
	void _VidInit();
	const char *Key( int key, int down );
	void EnterGrabMode( void );
	void UnbindEntry( void );
	static void GetKeyBindings( const char *command, int *twoKeys );

private:
	void UnbindCommand( const char *command );
	void PromptDialog( void );
	void ResetKeysList( void );
	void Cancel( void )
	{
		EngFuncs::ClientCmd( TRUE, "exec keyboard\n" );
		Hide();
	}


	CMenuBannerBitmap banner;

	// state toggle by
	CMenuTable keysList;
	CMenuKeysModel keysListModel;

	// redefine key wait dialog
	CMenuMessageBox msgBox1; // small msgbox

	CMenuYesNoMessageBox msgBox2; // large msgbox

	int bind_grab;
} uiControls;

void CMenuControls::PromptDialog( void )
{
	// show\hide quit dialog
	msgBox1.ToggleVisibility();
}

/*
=================
UI_Controls_GetKeyBindings
=================
*/
void CMenuControls::GetKeyBindings( const char *command, int *twoKeys )
{
	int		i, count = 0;
	const char	*b;

	twoKeys[0] = twoKeys[1] = -1;

	for( i = 0; i < 256; i++ )
	{
		b = EngFuncs::KEY_GetBinding( i );
		if( !b ) continue;

		if( !stricmp( command, b ))
		{
			twoKeys[count] = i;
			count++;

			if( count == 2 ) break;
		}
	}

	// swap keys if needed
	if( twoKeys[0] != -1 && twoKeys[1] != -1 )
	{
		int tempKey = twoKeys[1];
		twoKeys[1] = twoKeys[0];
		twoKeys[0] = tempKey;
	}
}

void CMenuControls::UnbindCommand( const char *command )
{
	int i, l;
	const char *b;

	l = strlen( command );

	for( i = 0; i < 256; i++ )
	{
		b = EngFuncs::KEY_GetBinding( i );
		if( !b ) continue;

		if( !strncmp( b, command, l ))
			EngFuncs::KEY_SetBinding( i, "" );
	}
}

void CMenuKeysModel::Update( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/kb_act.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int i = 0;

	if( !afile )
	{
		m_iNumItems = 0;

		Con_Printf( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	memset( keysBind, 0, sizeof( keysBind ));
	memset( firstKey, 0, sizeof( firstKey ));
	memset( secondKey, 0, sizeof( secondKey ));

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		if( !stricmp( token, "blank" ))
		{
			// separator
			pfile = EngFuncs::COM_ParseFile( pfile, token );
			if( !pfile ) break;	// technically an error

			snprintf( name[i], CMD_LENGTH, "^6%s^7", token );
			keysBind[i][0] = firstKey[i][0] = secondKey[i][0] = 0;
			i++;
		}
		else
		{
			// key definition
			int	keys[2];

			CMenuControls::GetKeyBindings( token, keys );
			Q_strncpy( keysBind[i], token, sizeof( keysBind[i] ));

			pfile = EngFuncs::COM_ParseFile( pfile, token );
			if( !pfile ) break; // technically an error

			snprintf( name[i], CMD_LENGTH, "^6%s^7", token );

			const char *firstKeyStr = NULL, *secondKeyStr = NULL;

			if( keys[0] != -1 )	firstKeyStr = EngFuncs::KeynumToString( keys[0] );
			if( keys[1] != -1 ) secondKeyStr = EngFuncs::KeynumToString( keys[1] );

			if( firstKeyStr )
				if( !strnicmp( firstKeyStr, "MOUSE", 5 ) )
					snprintf( firstKey[i], 20, "^5%s^7", firstKeyStr );
				else snprintf( firstKey[i], 20, "^3%s^7", firstKeyStr );
			else firstKey[i][0] = 0;

			if( secondKeyStr )
				if( !strnicmp( secondKeyStr, "MOUSE", 5 ) )
					snprintf( secondKey[i], 20, "^5%s^7", secondKeyStr );
				else snprintf( secondKey[i], 20, "^3%s^7", secondKeyStr );
			else secondKey[i][0] = 0;

			i++;
		}
	}

	m_iNumItems = i;

	EngFuncs::COM_FreeFile( afile );
}

void CMenuKeysModel::OnActivateEntry(int line)
{
	uiControls.EnterGrabMode();
}

void CMenuKeysModel::OnDeleteEntry(int line)
{
	uiControls.UnbindEntry();
}

void CMenuControls::ResetKeysList( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/kb_def.lst", NULL );
	char *pfile = afile;
	char token[1024];

	if( !afile )
	{
		Con_Printf( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		char	key[32];

		Q_strncpy( key, token, sizeof( key ));

		pfile = EngFuncs::COM_ParseFile( pfile, token );
		if( !pfile ) break;	// technically an error

		char	cmd[128];

		if( key[0] == '\\' && key[1] == '\\' )
		{
			key[0] = '\\';
			key[1] = '\0';
		}

		UnbindCommand( token );

		snprintf( cmd, sizeof( cmd ), "bind \"%s\" \"%s\"\n", key, token );
		EngFuncs::ClientCmd( TRUE, cmd );
	}

	EngFuncs::COM_FreeFile( afile );
	keysListModel.Update();
}

/*
=================
UI_Controls_KeyFunc
=================
*/
const char *CMenuControls::Key( int key, int down )
{
	char	cmd[128];

	if( msgBox1.IsVisible() && down )
	{
		if( bind_grab )	// assume we are in grab-mode
		{
			// defining a key
			if( key == '`' || key == '~' )
			{
				return uiSoundBuzz;
			}
			else if( key != K_ESCAPE )
			{
				const char *bindName = keysListModel.keysBind[keysList.GetCurrentIndex()];
				sprintf( cmd, "bind \"%s\" \"%s\"\n", EngFuncs::KeynumToString( key ), bindName );
				EngFuncs::ClientCmd( TRUE, cmd );
			}

			bind_grab = false;
			keysListModel.Update();

			PromptDialog();

			return uiSoundLaunch;
		}
	}
	return CMenuFramework::Key( key, down );
}

void CMenuControls::UnbindEntry()
{
	if( !keysListModel.IsLineUsable( keysList.GetCurrentIndex() ) )
	{
		EngFuncs::PlayLocalSound( uiSoundBuzz );
		return; // not a key
	}

	const char *bindName = keysListModel.keysBind[keysList.GetCurrentIndex()];

	UnbindCommand( bindName );
	EngFuncs::PlayLocalSound( uiSoundRemoveKey );
	keysListModel.Update();

	PromptDialog();
}

void CMenuControls::EnterGrabMode()
{
	if( !keysListModel.IsLineUsable( keysList.GetCurrentIndex() ) )
	{
		EngFuncs::PlayLocalSound( uiSoundBuzz );
		return;
	}

	// entering to grab-mode
	const char *bindName = keysListModel.keysBind[keysList.GetCurrentIndex()];

	int keys[2];

	GetKeyBindings( bindName, keys );
	if( keys[1] != -1 )
		UnbindCommand( bindName );

	bind_grab = true;

	PromptDialog();

	EngFuncs::PlayLocalSound( uiSoundKey );
}

/*
=================
UI_Controls_Init
=================
*/
void CMenuControls::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	keysList.SetRect( 360, 255, -20, 440 );
	keysList.SetModel( &keysListModel );
	keysList.SetupColumn( 0, "Action", 0.50f );
	keysList.SetupColumn( 1, "Key/Button", 0.25f );
	keysList.SetupColumn( 2, "Alternate", 0.25f );

	msgBox1.SetMessage( "Press a key or button" );

	msgBox2.SetMessage( "Reset buttons to default?" );
	msgBox2.onPositive = VoidCb( &CMenuControls::ResetKeysList );
	msgBox2.Link( this );

	AddItem( background );
	AddItem( banner );
	AddButton( "Use defaults", "Reset all buttons binding to their default values", PC_USE_DEFAULTS, msgBox2.MakeOpenEvent() );
	AddButton( "Adv controls", "Change mouse sensitivity, enable autoaim, mouselook and crosshair", PC_ADV_CONTROLS, UI_AdvControls_Menu );
	AddButton( "Ok", "Save changed and return to configuration menu", PC_DONE,
		VoidCb( &CMenuControls::SaveAndPopMenu ) );
	AddButton( "Cancel", "Discard changes and return to configuration menu", PC_CANCEL,
		VoidCb( &CMenuControls::Cancel ) );
	AddItem( keysList );
}

void CMenuControls::_VidInit()
{
	msgBox1.SetRect( DLG_X + 192, 256, 640, 128 );
	msgBox1.pos.x += uiStatic.xOffset;
	msgBox1.pos.y += uiStatic.yOffset;

	keysListModel.Update();
}

/*
=================
UI_Controls_Precache
=================
*/
void UI_Controls_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_Controls_Menu
=================
*/
void UI_Controls_Menu( void )
{
	UI_Controls_Precache();
	uiControls.Show();
}

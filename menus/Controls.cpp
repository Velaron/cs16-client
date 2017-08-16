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
#include "ScrollList.h"

#define ART_BANNER		"gfx/shell/head_controls"

#define MAX_KEYS		256
#define CMD_LENGTH		38
#define KEY1_LENGTH		20+CMD_LENGTH
#define KEY2_LENGTH		20+KEY1_LENGTH

class CMenuControls : public CMenuFramework
{
public:
	CMenuControls() : CMenuFramework("CMenuControls") { }

	void _Init();
	const char *Key( int key, int down );
private:

	char		keysBind[MAX_KEYS][CMD_LENGTH];
	char		firstKey[MAX_KEYS][20];
	char		secondKey[MAX_KEYS][20];
	char		keysDescription[MAX_KEYS][256];
	char		*keysDescriptionPtr[MAX_KEYS];

	void GetKeyBindings( const char *command, int *twoKeys );
	void UnbindCommand( const char *command );
	void ParseKeysList( void );
	void PromptDialog( void );
	// void RestartMenu( void );
	void ResetKeysList( void );
	void EnterGrabMode( void );
	void UnbindEntry( void );

	DECLARE_EVENT_TO_MENU_METHOD( CMenuControls, ResetKeysList )
	DECLARE_EVENT_TO_MENU_METHOD( CMenuControls, EnterGrabMode )
	DECLARE_EVENT_TO_MENU_METHOD( CMenuControls, UnbindEntry )

	
	CMenuBannerBitmap banner;

	// state toggle by
	CMenuPicButton defaults;
	CMenuPicButton advanced;
	CMenuPicButton done;
	CMenuPicButton cancel;
	CMenuScrollList keysList;

	// redefine key wait dialog
	CMenuMessageBox msgBox1; // small msgbox

	CMenuYesNoMessageBox msgBox2; // large msgbox

	int bind_grab;
	char hintText[KEY2_LENGTH+1];
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

void CMenuControls::ParseKeysList( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/kb_act.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int i = 0;

	if( !afile )
	{
		for( ; i < MAX_KEYS; i++ ) keysDescriptionPtr[i] = NULL;
		keysList.pszItemNames = (const char **)keysDescriptionPtr;

		Con_Printf( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	memset( keysBind, 0, sizeof( keysBind ));
	memset( firstKey, 0, sizeof( firstKey ));
	memset( secondKey, 0, sizeof( secondKey ));
	memset( keysDescription, 0, sizeof( keysDescription ));
	memset( keysDescriptionPtr, 0, sizeof( keysDescriptionPtr ));

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		char	str[128];

		if( !stricmp( token, "blank" ))
		{
			// separator
			pfile = EngFuncs::COM_ParseFile( pfile, token );
			if( !pfile ) break;	// technically an error

			snprintf( str, sizeof(str), "^6%s^7", token );	// enable uiPromptTextColor
			StringConcat( keysDescription[i], str, strlen( str ) + 1 );
			keysDescriptionPtr[i] = keysDescription[i];
			strcpy( keysBind[i], "" );
			strcpy( firstKey[i], "" );
			strcpy( secondKey[i], "" );
			i++;
		}
		else
		{
			// key definition
			int	keys[2];

			GetKeyBindings( token, keys );
			strncpy( keysBind[i], token, sizeof( keysBind[i] ));

			pfile = EngFuncs::COM_ParseFile( pfile, token );
			if( !pfile ) break; // technically an error

			snprintf( str, sizeof( str ), "^6%s^7", token );	// enable uiPromptTextColor

			if( keys[0] == -1 ) strcpy( firstKey[i], "" );
			else strncpy( firstKey[i], EngFuncs::KeynumToString( keys[0] ), sizeof( firstKey[i] ));

			if( keys[1] == -1 ) strcpy( secondKey[i], "" );
			else strncpy( secondKey[i], EngFuncs::KeynumToString( keys[1] ), sizeof( secondKey[i] ));

			StringConcat( keysDescription[i], str, CMD_LENGTH );
			AddSpaces( keysDescription[i], CMD_LENGTH );

			// HACKHACK this color should be get from kb_keys.lst
			if( !strnicmp( firstKey[i], "MOUSE", 5 ))
				snprintf( str, sizeof( str ), "^5%s^7", firstKey[i] );	// cyan
			else snprintf( str, sizeof( str ), "^3%s^7", firstKey[i] );	// yellow
			StringConcat( keysDescription[i], str, KEY1_LENGTH );
			AddSpaces( keysDescription[i], KEY1_LENGTH );

			// HACKHACK this color should be get from kb_keys.lst
			if( !strnicmp( secondKey[i], "MOUSE", 5 ))
				snprintf( str, sizeof( str ), "^5%s^7", secondKey[i] );// cyan
			else snprintf( str, sizeof( str ), "^3%s^7", secondKey[i] );	// yellow

			StringConcat( keysDescription[i], str, KEY2_LENGTH );
			AddSpaces( keysDescription[i],KEY2_LENGTH );
			keysDescriptionPtr[i] = keysDescription[i];
			i++;
		}
	}

	EngFuncs::COM_FreeFile( afile );

	for( ; i < MAX_KEYS; i++ ) keysDescriptionPtr[i] = NULL;
	keysList.pszItemNames = (const char **)keysDescriptionPtr;
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

		strncpy( key, token, sizeof( key ));

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
	ParseKeysList();
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
				const char *bindName = keysBind[keysList.iCurItem];
				sprintf( cmd, "bind \"%s\" \"%s\"\n", EngFuncs::KeynumToString( key ), bindName );
				EngFuncs::ClientCmd( TRUE, cmd );
			}

			bind_grab = false;
			ParseKeysList();

			PromptDialog();

			return uiSoundLaunch;
		}
	}
	return CMenuFramework::Key( key, down );
}

void CMenuControls::UnbindEntry()
{
	const char *bindName = keysBind[keysList.iCurItem];

	if( !bindName[0] )
		EngFuncs::PlayLocalSound( uiSoundBuzz );

	UnbindCommand( bindName );
	EngFuncs::PlayLocalSound( uiSoundRemoveKey );
	ParseKeysList();

	PromptDialog();
}

void CMenuControls::EnterGrabMode()
{
	// entering to grab-mode
	const char *bindName = keysBind[keysList.iCurItem];

	if( !bindName[0] )
	{
		EngFuncs::PlayLocalSound( uiSoundBuzz );
		return; // not a key
	}

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
	StringConcat( hintText, "Action", CMD_LENGTH );
	AddSpaces( hintText, CMD_LENGTH-4 );
	StringConcat( hintText, "Key/Button", KEY1_LENGTH );
	AddSpaces( hintText, KEY1_LENGTH-8 );
	StringConcat( hintText, "Alternate", KEY2_LENGTH );
	AddSpaces( hintText, KEY2_LENGTH );

	banner.SetPicture( ART_BANNER );

	defaults.SetNameAndStatus( "Use defaults", "Reset all buttons binding to their default values" );
	defaults.SetCoord( 72, 230 );
	defaults.SetPicture( PC_USE_DEFAULTS );
	defaults.onActivated = msgBox2.MakeOpenEvent();

	advanced.SetNameAndStatus( "Adv controls", "Change mouse sensitivity, enable autoaim, mouselook and crosshair" );
	advanced.SetCoord( 72, 280 );
	advanced.SetPicture( PC_ADV_CONTROLS );
	advanced.onActivated = UI_AdvControls_Menu;

	done.SetNameAndStatus( "Ok", "Save changed and return to configuration menu" );
	done.SetCoord( 72, 330 );
	done.SetPicture( PC_DONE );
	done.onActivated = SaveAndPopMenuCb;

	cancel.SetNameAndStatus( "Cancel", "Discard changes and return to configuration menu" );
	cancel.SetCoord( 72, 380 );
	cancel.SetPicture( PC_CANCEL );
	SET_EVENT( cancel, onActivated )
	{
		EngFuncs::ClientCmd( TRUE, "exec keyboard\n");
		pSelf->Parent()->Hide();
	}
	END_EVENT( cancel, onActivated )

	keysList.SetRect( 360, 255, 640, 440 );
	keysList.onDeleteEntry = UnbindEntryCb;
	keysList.onActivateEntry = EnterGrabModeCb;
	keysList.szName = hintText;
	ParseKeysList();

	msgBox1.SetRect( DLG_X + 192, 256, 640, 128 );
	msgBox1.SetMessage( "Press a key or button" );

	msgBox2.SetMessage( "Reset buttons to default?" );
	msgBox2.onPositive = ResetKeysListCb;
	msgBox2.Link( this );

	AddItem( background );
	AddItem( banner );
	AddItem( defaults );
	AddItem( advanced );
	AddItem( done );
	AddItem( cancel );
	AddItem( keysList );
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

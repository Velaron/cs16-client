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
#include "Slider.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "ScrollList.h"
#include "SpinControl.h"
#include "Field.h"
#include "YesNoMessageBox.h"

#define ART_BANNER	  	"gfx/shell/head_touch_options"

class CMenuTouchOptions : public CMenuFramework
{
private:
	void _Init();
	void _VidInit();

public:
	CMenuTouchOptions() : CMenuFramework( "CMenuTouchOptions" ) { }

	static void DeleteProfileCb( CMenuBaseItem *pSelf, void *pExtra );
	static void ResetButtonsCb( CMenuBaseItem *pSelf, void *pExtra );

	void GetProfileList();
	void SaveAndPopMenu();
	void GetConfig();

	char		profileDesc[UI_MAXGAMES][95];
	char		*profileDescPtr[UI_MAXGAMES];
	int			firstProfile;
	
	CMenuPicButton	done;

	CMenuSlider	lookX;
	CMenuSlider	lookY;
	CMenuSlider	moveX;
	CMenuSlider	moveY;
	CMenuCheckBox	enable;
	CMenuCheckBox	grid;
	CMenuCheckBox	nomouse;
	CMenuPicButton	reset;
	CMenuPicButton	save;
	CMenuPicButton	remove;
	CMenuPicButton	apply;
	CMenuField	profilename;
	CMenuScrollList profiles;
	CMenuSpinControl gridsize;

	// prompt dialog
	CMenuYesNoMessageBox msgBox;
};

static CMenuTouchOptions	uiTouchOptions;

void CMenuTouchOptions::GetProfileList( void )
{
	char	**filenames;
	int	i = 0, numFiles, j = 0;
	char *curprofile;

	strncpy( profileDesc[i], "Presets:", CS_SIZE );
	profileDescPtr[i] = profileDesc[i];
	i++;

	filenames = EngFuncs::GetFilesList( "touch_presets/*.cfg", &numFiles, TRUE );
	for ( ; j < numFiles; i++, j++ )
	{
		if( i >= UI_MAXGAMES ) break;

		// strip path, leave only filename (empty slots doesn't have savename)
		COM_FileBase( filenames[j], profileDesc[i] );
		profileDescPtr[i] = profileDesc[i];
	}

	// Overwrite "Presets:" line if there is no presets
	if( i == 1 )
		i = 0;

	filenames = EngFuncs::GetFilesList( "touch_profiles/*.cfg", &numFiles, TRUE );
	j = 0;
	curprofile = EngFuncs::GetCvarString("touch_config_file");

	strncpy( profileDesc[i], "Profiles:", CS_SIZE );
	profileDescPtr[i] = profileDesc[i];
	i++;

	strncpy( profileDesc[i], "default", CS_SIZE );
	profileDescPtr[i] = profileDesc[i];

	profiles.iHighlight = i;

	firstProfile = i;
	i++;

	for ( ; j < numFiles; i++, j++ )
	{
		if( i >= UI_MAXGAMES ) break;

		COM_FileBase( filenames[j], profileDesc[i] );
		profileDescPtr[i] = profileDesc[i];
		if( !strcmp( filenames[j], curprofile ) )
			profiles.iHighlight = i;
	}
	profiles.iNumItems = i;

	remove.iFlags |= QMF_GRAYED;
	apply.iFlags |= QMF_GRAYED;

	if( profiles.charSize.h )
	{
		profiles.iNumRows = (profiles.size.h / profiles.charSize.h) - 2;
		if( profiles.iNumRows > profiles.iNumItems )
			profiles.iNumRows = i;
	}

	for ( ; i < UI_MAXGAMES; i++ )
		profileDescPtr[i] = NULL;
	profiles.iCurItem = profiles.iHighlight;


	profiles.pszItemNames = (const char **)profileDescPtr;
}

/*
=================
UI_TouchOptions_SetConfig
=================
*/
void CMenuTouchOptions::SaveAndPopMenu( void )
{
	grid.WriteCvar();
	gridsize.WriteCvar();
	lookX.WriteCvar();
	lookY.WriteCvar();
	moveX.WriteCvar();
	moveY.WriteCvar();
	enable.WriteCvar();
	nomouse.WriteCvar();

	CMenuFramework::SaveAndPopMenu();
}

void CMenuTouchOptions::GetConfig( void )
{
	grid.UpdateEditable();
	gridsize.UpdateEditable();
	lookX.UpdateEditable();
	lookY.UpdateEditable();
	moveX.UpdateEditable();
	moveY.UpdateEditable();
	enable.UpdateEditable();
	nomouse.UpdateEditable();
}

void CMenuTouchOptions::DeleteProfileCb(CMenuBaseItem *pSelf, void *pExtra)
{
	CMenuTouchOptions *parent = (CMenuTouchOptions*)pSelf->Parent();
	char command[256];

	if( parent->profiles.iCurItem <= parent->firstProfile )
		return;

	snprintf(command, 256, "touch_deleteprofile \"%s\"\n", parent->profiles.GetSelectedItem() );
	EngFuncs::ClientCmd(1, command);

	parent->GetProfileList();
}

void CMenuTouchOptions::ResetButtonsCb(CMenuBaseItem *pSelf, void *pExtra)
{
	CMenuTouchOptions *parent = (CMenuTouchOptions*)pSelf->Parent();

	EngFuncs::ClientCmd( 0, "touch_pitch 90\n" );
	EngFuncs::ClientCmd( 0, "touch_yaw 120\n" );
	EngFuncs::ClientCmd( 0, "touch_forwardzone 0.06\n" );
	EngFuncs::ClientCmd( 0, "touch_sidezone 0.06\n" );
	EngFuncs::ClientCmd( 0, "touch_grid 1\n" );
	EngFuncs::ClientCmd( 1, "touch_grid_count 50\n" );

	parent->GetConfig();
}

/*
=================
UI_TouchOptions_Init
=================
*/
void CMenuTouchOptions::_Init( void )
{
	banner.SetPicture(ART_BANNER);
/*
	testImage.generic.id = ID_BANNER;
	testImage.generic.type = QMTYPE_BITMAP;
	testImage.iFlags = QMF_INACTIVE;
	testImage.generic.x = 390;
	testImage.generic.y = 225;
	testImage.generic.width = 480;
	testImage.generic.height = 450;
	testImage.pic = ART_GAMMA;
	testImage.generic.ownerdraw = UI_TouchOptions_Ownerdraw;
*/
	done.SetNameAndStatus( "Done", "Go back to the Touch Menu" );
	done.SetPicture( PC_DONE );
	done.onActivated = SaveAndPopMenuCb;

	lookX.SetNameAndStatus( "Look X", "Horizontal look sensitivity" );
	lookX.Setup( 50, 500, 5 );
	lookX.LinkCvar( "touch_yaw" );

	lookY.SetNameAndStatus( "Look Y", "Vertical look sensitivity" );
	lookY.Setup( 50, 500, 5 );
	lookY.LinkCvar( "touch_pitch" );

	moveX.SetNameAndStatus( "Side", "Side move sensitivity" );
	moveX.Setup( 0.02, 1.0, 0.05 );
	moveX.LinkCvar( "touch_sidezone" );

	moveY.SetNameAndStatus( "Forward", "Forward move sensitivity" );
	moveY.Setup( 0.02, 1.0, 0.05 );
	moveY.LinkCvar( "touch_forwardzone" );

	gridsize.szStatusText = "Set grid size";
	gridsize.Setup( 25, 100, 5 );
	gridsize.LinkCvar( "touch_grid_count", CMenuEditable::CVAR_VALUE );

	grid.SetNameAndStatus( "Grid", "Enable/disable grid" );
	grid.LinkCvar( "touch_grid_enable" );

	enable.SetNameAndStatus( "Enable", "enable/disable touch controls" );
	enable.LinkCvar( "touch_enable" );

	nomouse.SetNameAndStatus( "Ignore Mouse", "Ignore mouse input" );
	nomouse.LinkCvar( "m_ignore" );

	GetProfileList();

	SET_EVENT( profiles, onChanged )
	{
		CMenuTouchOptions *parent = (CMenuTouchOptions *)pSelf->Parent();
		CMenuScrollList *self = (CMenuScrollList*)pSelf;
		char curprofile[256];
		int isCurrent;
		COM_FileBase( EngFuncs::GetCvarString( "touch_config_file" ), curprofile );
		isCurrent = !strcmp( curprofile, parent->profileDesc[ self->iCurItem ]);

			// Scrolllist changed, update availiable options
		parent->remove.iFlags |= QMF_GRAYED;
		if( ( self->iCurItem > parent->firstProfile ) && !isCurrent )
			parent->remove.iFlags &= ~QMF_GRAYED;

		parent->apply.iFlags &= ~QMF_GRAYED;
		if( self->iCurItem == 0 || self->iCurItem == parent->firstProfile - 1 )
			self->iCurItem++;
		if( isCurrent )
			parent->apply.iFlags |= QMF_GRAYED;
	}
	END_EVENT( profiles, onChanged )

	profilename.szName = "New Profile:";
	profilename.iMaxLength = 16;

	reset.SetNameAndStatus( "Reset", "Reset touch to default state" );
	reset.SetPicture("gfx/shell/btn_touch_reset");
	SET_EVENT( reset, onActivated )
	{
		CMenuTouchOptions *parent = (CMenuTouchOptions *)pSelf->Parent();

		parent->msgBox.SetMessage( "Reset all buttons?");
		parent->msgBox.onPositive = CMenuTouchOptions::ResetButtonsCb;
		parent->msgBox.Show();
	}
	END_EVENT( reset, onActivated )

	remove.SetNameAndStatus( "Delete", "Delete saved game" );
	remove.SetPicture( PC_DELETE );
	SET_EVENT( remove, onActivated )
	{
		CMenuTouchOptions *parent = (CMenuTouchOptions *)pSelf->Parent();

		parent->msgBox.SetMessage( "Delete selected profile?");
		parent->msgBox.onPositive = CMenuTouchOptions::DeleteProfileCb;
		parent->msgBox.Show();
	}
	END_EVENT( remove, onActivated )

	apply.SetNameAndStatus( "Activate", "Apply selected profile" );
	apply.SetPicture( PC_ACTIVATE );
	SET_EVENT( apply, onActivated )
	{
		CMenuTouchOptions *parent = (CMenuTouchOptions *)pSelf->Parent();
		int i = parent->profiles.iCurItem;

		// preset selected
		if( i > 0 && i < parent->firstProfile - 1 )
		{
			char command[256];
			char *curconfig = EngFuncs::GetCvarString( "touch_config_file" );
			snprintf( command, 256, "exec \"touch_presets/%s\"\n", parent->profileDesc[ i ] );
			EngFuncs::ClientCmd( 1,  command );

			while( EngFuncs::FileExists( curconfig, TRUE ) )
			{
				char copystring[256];
				char filebase[256];

				COM_FileBase( curconfig, filebase );

				if( snprintf( copystring, 256, "touch_profiles/%s (new).cfg", filebase ) > 255 )
					break;

				EngFuncs::CvarSetString( "touch_config_file", copystring );
				curconfig = EngFuncs::GetCvarString( "touch_config_file" );
			}
		}
		else if( i == parent->firstProfile )
			EngFuncs::ClientCmd( 1,"exec touch.cfg\n" );
		else if( i > parent->firstProfile )
		{
			char command[256];
			snprintf( command, 256, "exec \"touch_profiles/%s\"\n", parent->profileDesc[ i ] );
			EngFuncs::ClientCmd( 1,  command );
		}

		// try save config
		EngFuncs::ClientCmd( 1,  "touch_writeconfig\n" );

		// check if it failed ant reset profile to default if it is
		if( !EngFuncs::FileExists( EngFuncs::GetCvarString( "touch_config_file" ), TRUE ) )
		{
			EngFuncs::CvarSetString( "touch_config_file", "touch.cfg" );
			parent->profiles.iCurItem = parent->firstProfile;
		}
		parent->GetProfileList();
		parent->GetConfig();
	}
	END_EVENT( apply, onActivated )

	save.SetNameAndStatus( "Save", "Save new profile" );
	save.SetPicture("gfx/shell/btn_touch_save");
	SET_EVENT( save, onActivated )
	{
		CMenuTouchOptions *parent = (CMenuTouchOptions*)pSelf->Parent();
		char name[256];

		if( parent->profilename.GetBuffer()[0] )
		{
			snprintf(name, 256, "touch_profiles/%s.cfg", parent->profilename.GetBuffer() );
			EngFuncs::CvarSetString("touch_config_file", name );
		}
		EngFuncs::ClientCmd( 1, "touch_writeconfig\n" );
		parent->GetProfileList();
		parent->profilename.Clear();
	}
	END_EVENT( save, onActivated )

	msgBox.SetPositiveButton( "Ok", PC_OK );
	msgBox.Link( this );
	
	AddItem( background );
	AddItem( banner );
	AddItem( done );
	AddItem( lookX );
	AddItem( lookY );
	AddItem( moveX );
	AddItem( moveY );
	AddItem( enable );
	AddItem( nomouse );
	AddItem( reset );
	AddItem( profiles );
	AddItem( save );
	AddItem( profilename );
	AddItem( remove );
	AddItem( apply );
	AddItem( grid );
	AddItem( gridsize );
}

void CMenuTouchOptions::_VidInit()
{
	done.SetCoord( 72, 700 );
	reset.SetCoord( 72, 640 );
	lookX.SetCoord( 72, 280 );
	lookY.SetCoord( 72, 340 );
	moveX.SetCoord( 72, 400 );
	moveY.SetCoord( 72, 460 );
	grid.SetCoord( 72, 520 );
	gridsize.SetRect( 72+30, 580, 150, 30 );
	enable.SetCoord( 680, 650 );
	nomouse.SetCoord( 680, 590 );
	profiles.SetRect( 360, 255, 300, 340 );
	remove.SetCoord( 560, 650 );
	remove.size.w = 100;

	apply.SetCoord( 360, 650 );
	apply.size.w = 120;

	profilename.SetRect( 680, 260, 205, 32 );
	save.SetCoord( 680, 330 );

}

/*
=================
UI_TouchOptions_Precache
=================
*/
void UI_TouchOptions_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_TouchOptions_Menu
=================
*/
void UI_TouchOptions_Menu( void )
{
	UI_TouchOptions_Precache();
	uiTouchOptions.Show();
}

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
#include "mathlib.h"
#include "const.h"
#include "keydefs.h"
#include "ref_params.h"
#include "cl_entity.h"
#include "com_model.h"
#include "entity_types.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "Slider.h"
#include "Field.h"
#include "SpinControl.h"

#define ART_BANNER		"gfx/shell/head_customize"

#define MAX_PLAYERMODELS	100

class CMenuPlayerModelView : public CMenuBaseItem
{
public:
	CMenuPlayerModelView();
	virtual void Init();
	virtual void VidInit();
	virtual void Draw();
	virtual const char *Key(int key, int down);

	HIMAGE hPlayerImage;

	void CalcFov();

	ref_params_t refdef;
	cl_entity_t *ent;

	bool mouseYawControl;
	int prevCursorX, prevCursorY;
};

CMenuPlayerModelView::CMenuPlayerModelView() : CMenuBaseItem()
{
	memset( &refdef, 0, sizeof( refdef ) );

	ent = NULL;
	mouseYawControl = false;
	prevCursorX = 0;
	prevCursorY = 0;
	hPlayerImage = 0;
	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
}

void CMenuPlayerModelView::Init()
{

}

void CMenuPlayerModelView::VidInit()
{
	CalcPosition();
	m_scSize = size.Scale();

	ent = EngFuncs::GetPlayerModel();

	if( !ent )
		return;

	EngFuncs::SetModel( ent, "models/player.mdl" );

	// setup render and actor
	refdef.fov_x = 40;

	// NOTE: must be called after UI_AddItem whan we sure what UI_ScaleCoords is done
	refdef.viewport[0] = m_scPos.x;
	refdef.viewport[1] = m_scPos.y;
	refdef.viewport[2] = m_scSize.w;
	refdef.viewport[3] = m_scSize.h;

	CalcFov();

	// adjust entity params
	ent->curstate.number = 1;	// IMPORTANT: always set playerindex to 1
	ent->curstate.animtime = gpGlobals->time;	// start animation
	ent->curstate.sequence = 1;
	ent->curstate.scale = 1.0f;
	ent->curstate.frame = 0.0f;
	ent->curstate.framerate = 1.0f;
	ent->curstate.effects |= EF_FULLBRIGHT;
	ent->curstate.controller[0] = 127;
	ent->curstate.controller[1] = 127;
	ent->curstate.controller[2] = 127;
	ent->curstate.controller[3] = 127;
	ent->latched.prevcontroller[0] = 127;
	ent->latched.prevcontroller[1] = 127;
	ent->latched.prevcontroller[2] = 127;
	ent->latched.prevcontroller[3] = 127;
	ent->origin[0] = ent->curstate.origin[0] = 45.0f / tan( DEG2RAD( refdef.fov_y / 2.0f ));
	ent->origin[2] = ent->curstate.origin[2] = 2.0f;
	ent->angles[1] = ent->curstate.angles[1] = 180.0f;
	
	ent->player = true; // yes, draw me as playermodel
}

const char *CMenuPlayerModelView::Key(int key, int down)
{
	if( !ent )
		return uiSoundNull;

	if( key == K_MOUSE1 && UI_CursorInRect( m_scPos, m_scSize ) &&
		down && !mouseYawControl )
	{
		mouseYawControl = true;
		prevCursorX =  uiStatic.cursorX;
		prevCursorY =  uiStatic.cursorY;
		
	}
	else if( key == K_MOUSE1 && !down && mouseYawControl )
	{
		mouseYawControl = false;
	}

	float yaw = ent->angles[1];

	switch( key )
	{
	case K_LEFTARROW:
	case K_KP_RIGHTARROW:
		if( down )
		{
			yaw -= 10.0f;

			if( yaw > 180.0f ) yaw -= 360.0f;
			else if( yaw < -180.0f ) yaw += 360.0f;

			ent->angles[1] = ent->curstate.angles[1] = yaw;
		}
		break;
	case K_RIGHTARROW:
	case K_KP_LEFTARROW:
		if( down )
		{
			yaw += 10.0f;

			if( yaw > 180.0f ) yaw -= 360.0f;
			else if( yaw < -180.0f ) yaw += 360.0f;

			ent->angles[1] = ent->curstate.angles[1] = yaw;
		}
		break;
	case K_ENTER:
	case K_AUX1:
	case K_MOUSE2:
		if( down ) ent->curstate.sequence++;
		break;
	default:
		return CMenuBaseItem::Key( key, down );
	}

	return uiSoundLaunch;
}

void CMenuPlayerModelView::Draw()
{
	// draw the background
	UI_FillRect( m_scPos, m_scSize, uiPromptBgColor );

	// draw the rectangle
	if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS && IsCurrentSelected() )
		UI_DrawRectangle( m_scPos, m_scSize, uiInputTextColor );
	else
		UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );

	if( !ui_showmodels->value )
	{
		EngFuncs::PIC_Set( hPlayerImage, 255, 255, 255, 255 );
		EngFuncs::PIC_Draw( m_scPos, m_scSize );
	}
	else
	{
		EngFuncs::ClearScene();

		// update renderer timings
		refdef.time = gpGlobals->time;
		refdef.frametime = gpGlobals->frametime;
		ent->curstate.body = 0;

		if( mouseYawControl )
		{
			float diffX = uiStatic.cursorX - prevCursorX;
			if( diffX )
			{
				float yaw = ent->angles[1];

				yaw += diffX / uiStatic.scaleX;

				if( yaw > 180.0f )
					yaw -= 360.0f;
				else if( yaw < -180.0f )
					yaw += 360.0f;
				ent->angles[1] = ent->curstate.angles[1] = yaw;
			}

			prevCursorX = uiStatic.cursorX;
			float diffY = uiStatic.cursorY - prevCursorY;
			if( diffY )
			{
				float pitch = refdef.viewangles[2];

				pitch += diffY / uiStatic.scaleY;

				if( pitch > 180.0f )
					pitch -= 360.0f;
				else if( pitch < -180.0f )
					pitch += 360.0f;
				refdef.viewangles[2] = pitch;
				ent->angles[2] = ent->curstate.angles[2] = -pitch;
			}

			prevCursorY = uiStatic.cursorY;
		}

		// draw the player model
		EngFuncs::CL_CreateVisibleEntity( ET_NORMAL, ent );
		EngFuncs::RenderScene( &refdef );
	}
}

/*
=================
UI_PlayerSetup_CalcFov

assume refdef is valid
=================
*/
void CMenuPlayerModelView::CalcFov( )
{
	float x = refdef.viewport[2] / tan( DEG2RAD( refdef.fov_x ) * 0.5f );
	float half_fov_y = atan( refdef.viewport[3] / x );
	refdef.fov_y = RAD2DEG( half_fov_y ) * 2;
}

class CMenuPlayerSetup : public CMenuFramework
{
private:
	void _Init();
	void _VidInit();
public:
	CMenuPlayerSetup() : CMenuFramework( "CMenuPlayerSetup" ) { }

	void FindModels();
	void SetConfig();
	void SaveAndPopMenu();

	char	models[MAX_PLAYERMODELS][CS_SIZE];
	char	*modelsPtr[MAX_PLAYERMODELS];
	int		num_models;
	char	currentModel[CS_SIZE];

	CMenuPicButton	done;
	CMenuPicButton	gameOptions;
	CMenuPicButton	advOptions;
	CMenuPlayerModelView	view;

	CMenuCheckBox	showModels;
	CMenuCheckBox	hiModels;
	CMenuCheckBox	clPredict;
	CMenuCheckBox	clLW;
	CMenuSlider	topColor;
	CMenuSlider	bottomColor;

	CMenuField	name;
	CMenuSpinControl	model;

} uiPlayerSetup;

/*
=================
UI_PlayerSetup_FindModels
=================
*/
void CMenuPlayerSetup::FindModels( void )
{
	char	name[256], path[256];
	char	**filenames;
	int numFiles, i;
	
	num_models = 0;

	// Get file list
	filenames = EngFuncs::GetFilesList(  "models/player/*", &numFiles, TRUE );
	if( !numFiles )
		filenames = EngFuncs::GetFilesList(  "models/player/*", &numFiles, FALSE );
	// a1batross: do we need this at all?
#if 1
	// add default singleplayer model
	strcpy( models[num_models], "player" );
	modelsPtr[num_models] = models[num_models];
	num_models++;
#endif
	// build the model list
	for( i = 0; i < numFiles; i++ )
	{
		COM_FileBase( filenames[i], name );
		snprintf( path, sizeof(path), "models/player/%s/%s.mdl", name, name );
		if( !EngFuncs::FileExists( path, TRUE ))
			continue;

		strcpy( models[num_models], name );
		modelsPtr[num_models] = models[num_models];
		num_models++;
	}

	for( i = num_models; i < MAX_PLAYERMODELS; i++ )
		modelsPtr[i] = NULL;
}


/*
=================
UI_PlayerSetup_SetConfig
=================
*/
void CMenuPlayerSetup::SetConfig( void )
{
	name.WriteCvar();
	model.WriteCvar();
	topColor.WriteCvar();
	bottomColor.WriteCvar();
	hiModels.WriteCvar();
	showModels.WriteCvar();
	clPredict.WriteCvar();
	clLW.WriteCvar();
}

void CMenuPlayerSetup::SaveAndPopMenu( void )
{
	SetConfig();
	CMenuFramework::SaveAndPopMenu();
}

/*
=================
UI_PlayerSetup_Init
=================
*/
void CMenuPlayerSetup::_Init( void )
{
	bool game_hlRally = false;
	int addFlags = 0;

	// disable playermodel preview for HLRally to prevent crash
	if( !stricmp( gMenu.m_gameinfo.gamefolder, "hlrally" ))
		game_hlRally = true;

	if( gMenu.m_gameinfo.flags & GFL_NOMODELS )
		addFlags |= QMF_INACTIVE;

	banner.SetPicture(ART_BANNER);

	done.SetNameAndStatus( "Done", "Go back to the Multiplayer Menu" );
	done.SetPicture( PC_DONE );\
	done.onActivated = SaveAndPopMenuCb;

	gameOptions.SetNameAndStatus( "Game options", "Configure handness, fov and other advanced options" );
	gameOptions.SetPicture( PC_GAME_OPTIONS );
	SET_EVENT( gameOptions, onActivated )
	{
		((CMenuPlayerSetup*)pSelf->Parent())->SetConfig();
		UI_GameOptions_Menu();
	}
	END_EVENT( gameOptions, onActivated )

	advOptions.SetNameAndStatus( "Adv options", "" );
	advOptions.SetPicture( PC_ADV_OPT );
	advOptions.onActivated = UI_AdvUserOptions_Menu;

	name.szStatusText = "Enter your multiplayer display name";
	name.iMaxLength = 32;
	name.LinkCvar( "name" );

	FindModels();
	model.Setup( (const char **)modelsPtr, (size_t)num_models );
	model.LinkCvar( "model", CMenuEditable::CVAR_STRING );
	SET_EVENT( model, onChanged )
	{
		CMenuPlayerSetup *parent = (CMenuPlayerSetup*)pSelf->Parent();
		CMenuSpinControl *self = (CMenuSpinControl*)pSelf;
		char image[256];
		const char *model = self->GetCurrentString();

		snprintf( image, 256, "models/player/%s/%s.bmp", model, model );
		parent->view.hPlayerImage = EngFuncs::PIC_Load( image );
		EngFuncs::CvarSetString("model", model );
		if( !strcmp( model, "player" ) )
			strcpy( image, "models/player.mdl" );
		else
			snprintf( image, sizeof(image), "models/player/%s/%s.mdl", model, model );
		if( parent->view.ent )
			EngFuncs::SetModel( parent->view.ent, image );
	}
	END_EVENT( model, onChanged )

	topColor.iFlags |= addFlags;
	topColor.SetNameAndStatus( "Top color", "Set a player model top color" );
	topColor.Setup( 0.0, 255, 1 );
	topColor.LinkCvar( "topcolor" );
	topColor.onCvarChange = CMenuEditable::WriteCvarCb;

	bottomColor.iFlags |= addFlags;
	bottomColor.SetNameAndStatus( "Bottom color", "Set a player model bottom color" );
	bottomColor.Setup( 0.0, 255.0, 1 );
	bottomColor.LinkCvar( "bottomcolor" );
	bottomColor.onCvarChange = CMenuEditable::WriteCvarCb;

	clPredict.SetNameAndStatus( "Predict movement", "Enable player movement prediction" );
	clPredict.LinkCvar( "cl_predict" );

	clLW.SetNameAndStatus( "Local weapons", "Enable local weapons" );
	clLW.LinkCvar( "cl_lw" );

	showModels.iFlags |= addFlags;
	showModels.SetNameAndStatus( "Show 3D preview", "Show 3D player models instead of preview thumbnails" );
	showModels.LinkCvar( "ui_showmodels" );
	showModels.onCvarChange = CMenuEditable::WriteCvarCb;

	hiModels.iFlags |= addFlags;
	hiModels.SetNameAndStatus( "High quality models", "Show HD models in multiplayer" );
	hiModels.LinkCvar( "cl_himodels" );
	hiModels.onCvarChange = CMenuEditable::WriteCvarCb;

	view.iFlags |= addFlags;
	//view.

	AddItem( background );
	AddItem( banner );
	AddItem( done );
	AddItem( gameOptions );
	AddItem( advOptions );
	AddItem( name );
	AddItem( clPredict);
	AddItem( clLW);
	if( !(gMenu.m_gameinfo.flags & GFL_NOMODELS) )
	{
		AddItem( topColor );
		AddItem( bottomColor );
		AddItem( showModels );
		AddItem( hiModels );
		AddItem( model );
		// disable playermodel preview for HLRally to prevent crash
		if( game_hlRally == FALSE )
			AddItem( view );
	}
}

void CMenuPlayerSetup::_VidInit()
{
	done.SetCoord( 72, 230 );
	gameOptions.SetCoord( 72, 280 );
	advOptions.SetCoord( 72, 330 );

	view.SetRect( 660, 260, 260, 320 );
	name.SetRect( 320, 260, 256, 36 );
	model.SetRect( 702, 590, 176, 32 );
	topColor.SetCoord( 250, 550 );
	topColor.size.w = 300;

	bottomColor.SetCoord( 250, 620 );
	bottomColor.size.w = 300;

	clPredict.SetCoord( 72, 380 );
	clLW.SetCoord( 72, 430 );
	showModels.SetCoord( 340, 380 );
	hiModels.SetCoord( 340, 430 );

	advOptions.SetGrayed( !UI_AdvUserOptions_IsAvailable() );
}

/*
=================
UI_PlayerSetup_Precache
=================
*/
void UI_PlayerSetup_Precache( void )
{
	EngFuncs::PIC_Load( ART_BANNER );
}

/*
=================
UI_PlayerSetup_Menu
=================
*/
void UI_PlayerSetup_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	UI_PlayerSetup_Precache();
	uiPlayerSetup.Show();
}

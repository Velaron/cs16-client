/*
PlayerModelView.cpp -- player model view
Copyright (C) 2018 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "PlayerModelView.h"

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

void CMenuPlayerModelView::VidInit()
{
	CMenuBaseItem::VidInit();

	ent = EngFuncs::GetPlayerModel();

	if( !ent )
		return;

	EngFuncs::SetModel( ent, "models/player.mdl" );

	// setup render and actor
	refdef.fov_x = 40;

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
		if( hPlayerImage )
		{
			EngFuncs::PIC_Set( hPlayerImage, 255, 255, 255, 255 );
			EngFuncs::PIC_DrawTrans( m_scPos, m_scSize );
		}
		else
		{
			UI_DrawString( font, m_scPos, m_scSize, "No preview", iColor, 0, m_scChSize, QM_CENTER, true );
		}
	}
	else
	{
		EngFuncs::ClearScene();

		// update renderer timings
#ifndef NEW_ENGINE_INTERFACE
		refdef.time = gpGlobals->time;
		refdef.frametime = gpGlobals->frametime;
#endif
		ent->curstate.body = 0;

		if( uiStatic.enableAlphaFactor )
		{
			ent->curstate.rendermode = kRenderTransTexture;
			ent->curstate.renderamt = uiStatic.alphaFactor * 255;
		}
		else
		{
			ent->curstate.rendermode = kRenderNormal;
			ent->curstate.renderamt = 255;
		}

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
#if 0 // Disabled. Pitch changing is ugly
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
#endif

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

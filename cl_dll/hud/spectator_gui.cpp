/*
spectator_gui.cpp - HUD Overlays
Copyright (C) 2015 a1batross

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.
*/

#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

/*
 * We will draw all elements inside a box. It's size 16x9.
 */

#define XPOS( x ) ( (x) / 16.0f )
#define YPOS( x ) ( (x) / 9.0f  )

/*
 * Black bar:
 * Height = 2/9 of ScreenHeight.
 * Width = ScreenWidth.
 */
#define BAR_HEIGHT ( ( 2.0f / 9.0f ) * ScreenHeight )
#define BAR_WIDTH  ( ScreenWidth )


/*
 * Button:
 * Height = bar's height * 0.5
 * Width = ScreenWidth * 0.25
 */
#define BUTTON_HEIGHT ( BAR_HEIGHT * 0.5 )
#define BUTTON_WIDTH  ( ScreenWidth * 0.25 )

/*
 * Little button:
 * Height = button's height
 * Width = button's height
 */
#define LITTLE_BUTTON_HEIGHT BUTTON_HEIGHT
#define LITTLE_BUTTON_WIDTH  BUTTON_HEIGHT

/*
 * DIVIDER POSITION AND HEIGHT
 */
#define DIVIDER_XPOS (ScreenWidth * 0.85f)
#define DIVIDER_GAP (20)

DECLARE_MESSAGE( m_SpectatorGui, SpecHealth );
DECLARE_MESSAGE( m_SpectatorGui, SpecHealth2 );

DECLARE_COMMAND( m_SpectatorGui, ToggleSpectatorMenu );

DECLARE_COMMAND( m_SpectatorGui, ToggleSpectatorMenuOptions );
// close
// help
// settings
// pip
// autodirector
// showscores

DECLARE_COMMAND( m_SpectatorGui, ToggleSpectatorMenuOptionsSettings );
// settings
// // chat msgs
// // show status
// // view cone
// // player names

DECLARE_COMMAND( m_SpectatorGui, ToggleSpectatorMenuSpectateOptions );
// chase map overview
// free map overview
// first person
// free look
// free chase camera
// locked chase camera

void __CmdFunc_FindNextPlayerReverse( void )
{
	gHUD.m_Spectator.FindNextPlayer(true);
}

void __CmdFunc_FindNextPlayer( void )
{
	gHUD.m_Spectator.FindNextPlayer(false);
}

int CHudSpectatorGui::Init()
{
	HOOK_MESSAGE( SpecHealth );
	HOOK_MESSAGE( SpecHealth2 );

	HOOK_COMMAND( "_spec_toggle_menu", ToggleSpectatorMenu );
	HOOK_COMMAND( "_spec_toggle_menu_options", ToggleSpectatorMenuOptions );
	HOOK_COMMAND( "_spec_toggle_menu_options_settings", ToggleSpectatorMenuOptionsSettings );
	HOOK_COMMAND( "_spec_toggle_menu_spectate_options", ToggleSpectatorMenuSpectateOptions );
	HOOK_COMMAND( "_spec_find_next_player_reverse", FindNextPlayerReverse );
	HOOK_COMMAND( "_spec_find_next_player", FindNextPlayer );

	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	m_bIsMenuShown = false;

	return 1;
}

int CHudSpectatorGui::VidInit()
{
	if( g_iXash )
	{
		m_hTimerTexture = gRenderAPI.GL_LoadTexture("gfx/vgui/timer.tga", NULL, 0, TF_NEAREST |TF_NOPICMIP|TF_NOMIPMAP|TF_CLAMP );
	}
	else
	{
		m_hTimerTexture = 0;
	}

	return 1;
}

int CHudSpectatorGui::Draw( float flTime )
{
	if( !g_iUser1 )
		return 1;

	// check for scoreboard. We will don't draw it, because screen space econodmy
	/*if( gHUD.m_Scoreboard.m_bForceDraw || !(!gHUD.m_Scoreboard.m_bShowscoresHeld && gHUD.m_Health.m_iHealth > 0 && !gHUD.m_iIntermission ))
		return 1;*/

	int r, g, b;
	r = 255; g = 140; b = 0;

	// at first, draw these silly black bars
	if( gHUD.m_Spectator.m_pip->value == INSET_OFF )
	{
		DrawUtils::DrawRectangle(0, 0, BAR_WIDTH, BAR_HEIGHT, 0, 0, 0, 153, false);
	}
	else
	{
		int startpos;

		startpos = XRES(gHUD.m_Spectator.m_OverviewData.insetWindowWidth) + XRES(gHUD.m_Spectator.m_OverviewData.insetWindowX);

		startpos *= ScreenWidth / TrueWidth; // hud_scale adjust

		DrawUtils::DrawRectangle(startpos, 0, BAR_WIDTH - startpos, BAR_HEIGHT, 0, 0, 0, 153, false);
	}

	DrawUtils::DrawRectangle(0, ScreenHeight - BAR_HEIGHT, ScreenWidth, BAR_HEIGHT, 0, 0, 0, 153, false);

	// divider
	FillRGBABlend( DIVIDER_XPOS, BAR_HEIGHT * 0.25, 1, BAR_HEIGHT * 0.5, r, g, b, 255 );

	// function name says it
	CalcAllNeededData( );


	// draw counter-terrorist and terrorist remaining and time3
	DrawUtils::DrawHudString( DIVIDER_XPOS + DIVIDER_GAP, BAR_HEIGHT * 0.25, ScreenWidth, label.m_szMap, r, g, b );

	if( m_hTimerTexture )
	{
		gRenderAPI.GL_Bind( 0, m_hTimerTexture );
		DrawUtils::Draw2DQuad( TrueWidth * 0.85 + DIVIDER_GAP, TrueHeight / 14,
							   TrueWidth * 0.85 + DIVIDER_GAP + gHUD.GetCharHeight(), TrueHeight / 14 + gHUD.GetCharHeight() );
	}

	DrawUtils::DrawHudString( DIVIDER_XPOS + DIVIDER_GAP * 2 + gHUD.GetCharWidth('M'), BAR_HEIGHT * 0.5, ScreenWidth, label.m_szTimer, r, g, b);

	// CTs, Ts labels
	DrawUtils::DrawHudString( ScreenWidth * 0.7, BAR_HEIGHT * 0.25, ScreenWidth * 0.8, "Counter-Terrorists:", r, g, b );
	DrawUtils::DrawHudString( ScreenWidth * 0.7, BAR_HEIGHT * 0.5, ScreenWidth * 0.8, "Terrorists:", r, g, b );

	// CTs, Ts count
	DrawUtils::DrawHudNumberString( DIVIDER_XPOS - DIVIDER_GAP, BAR_HEIGHT * 0.25, ScreenWidth * 0.8, label.m_iCounterTerrorists, r, g, b );
	DrawUtils::DrawHudNumberString( DIVIDER_XPOS - DIVIDER_GAP, BAR_HEIGHT * 0.5, ScreenWidth * 0.8, label.m_iTerrorists, r, g, b );

	if( g_iUser2 > 0 && g_iUser2 < MAX_PLAYERS )
	{
		cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( g_iUser2 );
		if( pEnt && pEnt->player )
		{
			hud_player_info_t *pInfo = g_PlayerInfoList + pEnt->index;
			GetPlayerInfo( pEnt->index, pInfo );
			if( pInfo->name )
			{
				char szNameAndHealth[64];
				snprintf( szNameAndHealth, 64, "%s (%i)", pInfo->name, g_PlayerExtraInfo[pEnt->index].health );
				int iLen = DrawUtils::HudStringLen( szNameAndHealth );

				GetTeamColor( r, g, b, g_PlayerExtraInfo[ pEnt->index ].teamnumber );
				DrawUtils::DrawHudString( ScreenWidth * 0.5 - iLen * 0.5, ScreenHeight - BAR_HEIGHT * 0.5 , ScreenWidth, szNameAndHealth, r, g, b );
			}
		}
	}
	return 1;
}

void CHudSpectatorGui::CalcAllNeededData( )
{
	// mapname
	static char szMapNameStripped[55];
	const char *szMapName = gEngfuncs.pfnGetLevelName(); //  "maps/%s.bsp"
	strncpy( szMapNameStripped, szMapName + 5, sizeof( szMapNameStripped ) );
	szMapNameStripped[strlen(szMapNameStripped) - 4] = '\0';
	snprintf( label.m_szMap, sizeof( label.m_szMap ), "Map: %s", szMapNameStripped );

	// team
	label.m_iTerrorists        = 0;
	label.m_iCounterTerrorists = 0;
	for( int i = 0; i < MAX_PLAYERS; i++ )
	{
		if( g_PlayerExtraInfo[i].dead )
			continue; // show remaining

		switch( g_PlayerExtraInfo[i].teamnumber )
		{
		case TEAM_CT:
			label.m_iCounterTerrorists++;
		case TEAM_TERRORIST:
			label.m_iTerrorists++;
		}
	}

	// timer
	// time must be positive
	if( m_bBombPlanted )
	{
		label.m_szTimer[0] = '\0';
	}
	else
	{
		int iMinutes = max( 0, (int)( gHUD.m_Timer.m_iTime + gHUD.m_Timer.m_fStartTime - gHUD.m_flTime ) / 60);
		int iSeconds = max( 0, (int)( gHUD.m_Timer.m_iTime + gHUD.m_Timer.m_fStartTime - gHUD.m_flTime ) - (iMinutes * 60));

		sprintf( label.m_szTimer, "%i:%i", iMinutes, iSeconds );
	}
}

void CHudSpectatorGui::InitHUDData()
{
	m_bBombPlanted = false;
}

void CHudSpectatorGui::Think()
{

}

void CHudSpectatorGui::Reset()
{
	m_bBombPlanted = false;
}

int CHudSpectatorGui::MsgFunc_SpecHealth(const char *pszName, int iSize, void *buf)
{
	BEGIN_READ( buf, iSize );

	int health = READ_BYTE();

	g_PlayerExtraInfo[g_iUser2].health = health;
	m_iPlayerLastPointedAt = g_iUser2;

	return 1;
}

int CHudSpectatorGui::MsgFunc_SpecHealth2(const char *pszName, int iSize, void *buf)
{
	BEGIN_READ( buf, iSize );

	int health = READ_BYTE();
	int client = READ_BYTE();

	g_PlayerExtraInfo[client].health = health;
	m_iPlayerLastPointedAt = g_iUser2;

	return 1;
}

#define PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y(x, y) XPOS(x), YPOS(y), XPOS(x + 4.0f), YPOS(y + 1.0f)

void CHudSpectatorGui::UserCmd_ToggleSpectatorMenu()
{
	static byte color[4] = {0, 0, 0, 153};

	m_bIsMenuShown = !m_bIsMenuShown;

	gMobileAPI.pfnTouchSetClientOnly( m_bIsMenuShown );

	if( m_bIsMenuShown )
	{
		ClientCmd("touch_set_stroke 1 255 140 0 255");

		gMobileAPI.pfnTouchAddClientButton( "_spec_menu_options", "*white", "_spec_menu_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 0.5f, 7.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );

		gMobileAPI.pfnTouchAddClientButton( "_spec_menu_find_next_player_reverse", "*white", "_spec_find_next_player_reverse",
			XPOS(5.0f), YPOS(7.5f), XPOS(6.0f), YPOS(8.5f), color, 0, 1.0f, TOUCH_FL_STROKE );

		gMobileAPI.pfnTouchAddClientButton( "_spec_menu_nick", "*white", "*white",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 6.0f, 7.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );

		gMobileAPI.pfnTouchAddClientButton( "_spec_menu_find_next_player", "*white", "_spec_find_next_player",
			XPOS(10.0f),YPOS(7.5f), XPOS(11.0f),YPOS(8.5f), color, 0, 1.0f, TOUCH_FL_STROKE );

		gMobileAPI.pfnTouchAddClientButton( "_spec_menu_spectate_options", "*white", "_spec_menu_spectate_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 11.5f, 7.5f ),color, 0, 1.0f, TOUCH_FL_STROKE );
	}
	else
	{
		gMobileAPI.pfnTouchRemoveButton( "_spec_*" );
	}
}

void CHudSpectatorGui::UserCmd_ToggleSpectatorMenuOptions()
{
	static byte color[4] = {0, 0, 0, 153};
	static bool isShown = false;

	if( !m_bIsMenuShown )
		return;

	if( isShown = !isShown )
	{
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_close", "*white", "_spec_menu_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 0.5f, 1.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_help", "*white", "TODO; _spec_menu_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 0.5f, 2.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_settings", "*white", "_spec_toggle_menu_options_settings",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 0.5f, 3.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_pip", "*white", "spec_pip t; _spec_menu_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 0.5f, 4.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_ad", "*white", "spec_autodirector t; _spec_menu_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 0.5f, 5.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_showscores", "*white", "scoreboard; _spec_menu_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 0.5f, 6.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
	}
	else
	{
		gMobileAPI.pfnTouchRemoveButton( "_spec_opt_*" );
	}
}

void CHudSpectatorGui::UserCmd_ToggleSpectatorMenuOptionsSettings()
{
	static byte color[4] = {0, 0, 0, 153};
	static bool isShown = false;

	if( !m_bIsMenuShown )
		return;

	if( isShown = !isShown )
	{
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_chat_msgs", "*white", "TODO; _spec_toggle_menu_options_settings",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 4.5f, 3.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_set_status", "*white", "spec_drawstatus t; _spec_toggle_menu_options_settings",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 4.5f, 4.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_draw_cones", "*white", "spec_drawcone t; _spec_toggle_menu_options_settings",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 4.5f, 5.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_opt_draw_names", "*white", "spec_drawnames t; _spec_toggle_menu_options_settings",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 4.5f, 6.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
	}
	else
	{
		gMobileAPI.pfnTouchRemoveButton( "_spec_opt_set_*" );
	}
}

void CHudSpectatorGui::UserCmd_ToggleSpectatorMenuSpectateOptions()
{
	static byte color[4] = {0, 0, 0, 153};
	static bool isShown = false;

	if( !m_bIsMenuShown )
		return;

	if( isShown = !isShown )
	{
		gMobileAPI.pfnTouchAddClientButton( "_spec_spec_6", "*white", "spec_mode 6; _spec_menu_spectate_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 11.5f, 1.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_spec_5", "*white", "spec_mode 5; _spec_menu_spectate_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 11.5f, 2.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_spec_4", "*white", "spec_mode 4; _spec_menu_spectate_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 11.5f, 3.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_spec_3", "*white", "spec_mode 3; _spec_menu_spectate_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 11.5f, 4.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_spec_2", "*white", "spec_mode 2; _spec_menu_spectate_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 11.5f, 5.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
		gMobileAPI.pfnTouchAddClientButton( "_spec_spec_1", "*white", "spec_mode 1; _spec_menu_spectate_options",
			PLACE_DEFAULT_SIZE_BUTTON_AT_X_Y( 11.5f, 6.5f ), color, 0, 1.0f, TOUCH_FL_STROKE );
	}
	else
	{
		gMobileAPI.pfnTouchRemoveButton( "_spec_spec_*" );
	}
}

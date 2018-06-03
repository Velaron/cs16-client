/*
PicButton.h - animated button with picture
Copyright (C) 2010 Uncle Mike
Copyright (C) 2017 a1batross

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
#include "Bitmap.h"
#include "PicButton.h"
#include "Utils.h"
#include "Scissor.h"
#include "BtnsBMPTable.h"
#include <stdlib.h>

// Button stack
CMenuPicButton *ButtonStack[UI_MAX_MENUDEPTH] = { 0 };
int		ButtonStackDepth = 0;


CMenuPicButton::CMenuPicButton() : BaseClass()
{
	bEnableTransitions = true;
	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	iFlags = QMF_DROPSHADOW|QMF_ACT_ONRELEASE;

	iFocusStartTime = 0;

	eTextAlignment = QM_TOPLEFT;

	hPic = 0;
	button_id = 0;
	iOldState = BUTTON_NOFOCUS;
	m_iLastFocusTime = -512;
	bPulse = false;

	SetSize( UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT );

#ifdef MAINUI_RENDER_PICBUTTON_TEXT
	SetCharSize( QM_LIGHTBLUR );
#endif

	TransPic = 0;
	memset( TitleLerpQuads, 0, sizeof( TitleLerpQuads ));
}

/*
=================
CMenuPicButton::Key
=================
*/
const char *CMenuPicButton::Key( int key, int down )
{
	const char	*sound = 0;

	switch( key )
	{
	case K_MOUSE1:
		if(!( iFlags & QMF_HASMOUSEFOCUS ))
			break;
		sound = uiSoundLaunch;
		break;
	case K_ENTER:
	case K_KP_ENTER:
	case K_AUX1:
		if( iFlags & QMF_MOUSEONLY )
			break;
		sound = uiSoundLaunch;
	}
	if( sound && ( iFlags & QMF_SILENT ))
		sound = uiSoundNull;

	if( sound )
	{
		if( iFlags & QMF_ACT_ONRELEASE )
		{
			int event;
			if( down )
			{
				event = QM_PRESSED;
				m_bPressed = true;
			}
			else
			{
				temp = this;
				event = QM_ACTIVATED;
			}

			_Event( event );
#if !defined(TA_ALT_MODE2)
			if( event == QM_ACTIVATED )
				SetTransPicForLast( hPic );
#endif
		}
		else if( down )
		{
			temp = this;
			_Event( QM_ACTIVATED );
#if !defined(TA_ALT_MODE2)
			SetTransPicForLast( hPic );
#endif
		}
	}

	return sound;
}

// #define ALT_PICBUTTON_FOCUS_ANIM

/*
=================
CMenuPicButton::DrawButton
=================
*/
void CMenuPicButton::DrawButton(int r, int g, int b, int a, wrect_t *rects, int state)
{
	EngFuncs::PIC_Set( hPic, r, g, b, a );
#ifdef ALT_PICBUTTON_FOCUS_ANIM
	UI::PushScissor( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width * flFill, uiStatic.buttons_draw_height );
#endif
	EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[state] );

#ifdef ALT_PICBUTTON_FOCUS_ANIM
	UI::PopScissor();
#endif
}

/*
=================
CMenuPicButton::Draw
=================
*/
void CMenuPicButton::Draw( )
{
	int state = BUTTON_NOFOCUS;

	if( iFlags & (QMF_HASMOUSEFOCUS|QMF_HASKEYBOARDFOCUS))
	{
		state = BUTTON_FOCUS;
	}

	// make sure what cursor in rect
	if( m_bPressed && g_bCursorDown )
		state = BUTTON_PRESSED;
	else m_bPressed = false;

	if( iOldState == BUTTON_NOFOCUS && state != BUTTON_NOFOCUS )
		iFocusStartTime = uiStatic.realTime;

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		coord.x = m_scPos.x + 290 * uiStatic.scaleX;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}

	int a = (512 - (uiStatic.realTime - m_iLastFocusTime)) >> 1;

	if( hPic )
	{
		if( ButtonStackDepth && ButtonStack[ButtonStackDepth-1] == this )
			return;

		int r, g, b;

		UnpackRGB( r, g, b, iFlags & QMF_GRAYED ? uiColorDkGrey : uiColorWhite );

		wrect_t rects[] =
		{
		{ 0, uiStatic.buttons_width, 0,  26 },
		{ 0, uiStatic.buttons_width, 26, 52 },
		{ 0, uiStatic.buttons_width, 52, 78 },
		};
		if( state == BUTTON_NOFOCUS && a > 0 )
		{
			DrawButton( r, g, b, a, rects, BUTTON_FOCUS );
		}

		// pulse code.
		if( ( state == BUTTON_NOFOCUS && bPulse ) ||
			( state == BUTTON_FOCUS   && eFocusAnimation == QM_PULSEIFFOCUS ) )
		{
			EngFuncs::PIC_Set( hPic, r, g, b, 255 *(0.5 + 0.5 * sin( (float)uiStatic.realTime / ( UI_PULSE_DIVISOR * 2 ))));
			EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[BUTTON_FOCUS] );

			EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[BUTTON_NOFOCUS] );
		}
		else
		{
			// special handling for focused
			if( state == BUTTON_FOCUS )
			{
				EngFuncs::PIC_Set( hPic, r, g, b, 255 );
				EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[BUTTON_FOCUS] );

				EngFuncs::PIC_Set( hPic, r, g, b, 255 ); // set colors again
				EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[BUTTON_NOFOCUS] );
			}
			else
			{
				// just draw
				EngFuncs::PIC_Set( hPic, r, g, b, 255 );
				EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width, uiStatic.buttons_draw_height, &rects[state] );
			}
		}
	}
	else
	{
		int	shadow;

		shadow = (iFlags & QMF_DROPSHADOW);

		if( iFlags & QMF_GRAYED )
		{
#ifdef MAINUI_RENDER_PICBUTTON_TEXT
			if( UI_CursorInRect( m_scPos, m_scSize ) )
				a = 255;

			if( a > 0 )
			{
				UI_DrawString( uiStatic.hHeavyBlur, m_scPos, m_scSize, szName,
					InterpColor( uiColorBlack, uiColorDkGrey, a / 255.0f ), false, m_scChSize, eTextAlignment, shadow );
			}
#endif
			UI_DrawString( font, m_scPos, m_scSize, szName, uiColorDkGrey, true, m_scChSize, eTextAlignment, shadow );
			return; // grayed
		}

		if(this != m_pParent->ItemAtCursor())
		{
#ifdef MAINUI_RENDER_PICBUTTON_TEXT
			if( a > 0 )
			{
				UI_DrawString( uiStatic.hHeavyBlur, m_scPos, m_scSize, szName,
					InterpColor( uiColorBlack, iColor, a / 255.0f ), false, m_scChSize, eTextAlignment, shadow );
			}
#endif
			UI_DrawString( font, m_scPos, m_scSize, szName, iColor, false, m_scChSize, eTextAlignment, shadow );
			return; // no focus
		}

		if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
		{
#ifdef MAINUI_RENDER_PICBUTTON_TEXT
			UI_DrawString( uiStatic.hHeavyBlur, m_scPos, m_scSize, szName, iColor, false, m_scChSize, eTextAlignment, shadow );
			UI_DrawString( font, m_scPos, m_scSize, szName, iColor, false, m_scChSize, eTextAlignment, shadow );
#else
			UI_DrawString( font, m_scPos, m_scSize, szName, iFocusColor, false, m_scChSize, eTextAlignment, shadow );
#endif
		}
		else if( eFocusAnimation == QM_PULSEIFFOCUS )
		{
			float pulsar = 0.5 + 0.5 * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR );
#ifdef MAINUI_RENDER_PICBUTTON_TEXT

			UI_DrawString( uiStatic.hHeavyBlur, m_scPos, m_scSize, szName,
				InterpColor( uiColorBlack, iColor, pulsar ), false, m_scChSize, eTextAlignment, shadow );
			UI_DrawString( font, m_scPos, m_scSize, szName, iColor, false, m_scChSize, eTextAlignment, shadow );
#else
			UI_DrawString( font, m_scPos, m_scSize, szName,
				InterpColor( iColor, iFocusColor, pulsar ), false, m_scChSize, eTextAlignment, shadow );
#endif
		}
	}

	iOldState = state;
}

void CMenuPicButton::SetPicture( EDefaultBtns ID )
{
#ifndef MAINUI_RENDER_PICBUTTON_TEXT
	if( ID < 0 || ID > PC_BUTTONCOUNT )
		return; // bad id

	hPic = uiStatic.buttonsPics[ID];

	button_id = ID;
#endif
}

void CMenuPicButton::SetPicture( const char *filename )
{
#ifndef MAINUI_RENDER_PICBUTTON_TEXT
	hPic = EngFuncs::PIC_Load( filename );
#endif
}

// =========================== Animations ===========================


// Pressed cancel, done or ESC in menu
void CMenuPicButton::PopPButtonStack()
{
	if( ButtonStackDepth )
	{
		if( ButtonStack[ButtonStackDepth-1] )
		{
			ButtonStack[ButtonStackDepth-1]->SetTitleAnim( AS_TO_BUTTON );
		}

		ButtonStackDepth--;
	}
}

// Opened new menu, awaiting Quad from Banner
void CMenuPicButton::PushPButtonStack()
{
	if( ButtonStackDepth && ButtonStack[ButtonStackDepth-1] == this )
		return;

	ButtonStack[ButtonStackDepth++] = this;
}

int	CMenuPicButton::transition_state = CMenuPicButton::AS_TO_TITLE;
int	CMenuPicButton::transition_initial_time;
CMenuPicButton* CMenuPicButton::temp = NULL;
HIMAGE CMenuPicButton::s_hCurrentTransPic = 0;
wrect_t CMenuPicButton::s_pCurrentTransRect = { 0, 0, 0, 0 };
CMenuPicButton::Quad CMenuPicButton::s_CurrentLerpQuads[2];

float CMenuPicButton::GetTitleTransFraction( void )
{
	float fraction = (float)(uiStatic.realTime - transition_initial_time ) / TTT_PERIOD;

	if( fraction > 1.0f )
		fraction = 1.0f;

	return fraction;
}

void CMenuPicButton::SetTitleAnim( int anim_state )
{
	static	wrect_t r = { 0, uiStatic.buttons_width, 26, 51 };
	CMenuPicButton *button = NULL;

	// choose target button
	if( anim_state == AS_TO_TITLE )
	{
		if( temp )
			temp->PushPButtonStack();

		button = temp;
	}
	else
	{
		if( !ButtonStackDepth )
			return;

		button = ButtonStack[ButtonStackDepth-1];
	}

	if(	!button )
		return;

	if( !button->hPic )
		return;

	if( !button->bEnableTransitions )
		return;

	transition_state = anim_state;

	button->TitleLerpQuads[0].x = button->m_scPos.x;
	button->TitleLerpQuads[0].y = button->m_scPos.y;
	button->TitleLerpQuads[0].lx = button->m_scSize.w;
	button->TitleLerpQuads[0].ly = button->m_scSize.h;

	transition_initial_time = uiStatic.realTime;
	s_hCurrentTransPic = button->TransPic;
	s_pCurrentTransRect = r;
	memcpy( s_CurrentLerpQuads, button->TitleLerpQuads, sizeof( s_CurrentLerpQuads ));
}

void CMenuPicButton::RootChanged( bool isForward )
{
	// A guarantee, that we have changed root active menu
	if( isForward )
	{
		SetTitleAnim( AS_TO_TITLE );
	}
	else
	{
		SetTitleAnim( AS_TO_BUTTON );
		PopPButtonStack();
	}
}

CMenuPicButton::Quad CMenuPicButton::LerpQuad( Quad a, Quad b, float frac )
{
	Quad c;

	c.x = a.x + (b.x - a.x) * frac;
	c.y = a.y + (b.y - a.y) * frac;
	c.lx = a.lx + (b.lx - a.lx) * frac;
	c.ly = a.ly + (b.ly - a.ly) * frac;

	return c;
}

void CMenuPicButton::SetupTitleQuadForLast(int x, int y, int w, int h)
{
	if( !ButtonStackDepth )
		return;

	if( !ButtonStack[ButtonStackDepth-1] )
		return;

	ButtonStack[ButtonStackDepth-1]->SetupTitleQuad( x, y, w, h );

}

void CMenuPicButton::SetTransPicForLast( HIMAGE pic )
{
	if( !ButtonStackDepth )
		return;

	if( !ButtonStack[ButtonStackDepth-1] )
		return;

	ButtonStack[ButtonStackDepth-1]->SetTransPic( pic );
}

// TODO: Find CMenuBannerBitmap in next menu page and correct
void CMenuPicButton::SetupTitleQuad( int x, int y, int w, int h )
{
	TitleLerpQuads[1].x  = x * uiStatic.scaleX;
	TitleLerpQuads[1].y  = y * uiStatic.scaleY;
	TitleLerpQuads[1].lx = w * uiStatic.scaleX;
	TitleLerpQuads[1].ly = h * uiStatic.scaleY;

	s_CurrentLerpQuads[1] = TitleLerpQuads[1];
}

void CMenuPicButton::SetTransPic(HIMAGE pic)
{
	TransPic = pic;

	s_hCurrentTransPic = TransPic;
}

bool CMenuPicButton::DrawTitleAnim( CMenuBaseWindow::EAnimation state )
{
	// don't draw it twice
	if( state == CMenuBaseWindow::ANIM_OUT )
		return false; // but request more frame for animation

#if 1
	float frac = GetTitleTransFraction();
#else
	float frac = (sin(gpGlobals->time*4)+1)/2;
#endif

#ifdef TA_ALT_MODE
	if( frac >= 1.0f && transition_state == AS_TO_BUTTON )
#else
	if( frac >= 1.0f )
#endif
	{
		s_hCurrentTransPic = 0;
		return true;
	}

	if( !s_hCurrentTransPic )
		return false; // wait for transition pic

	Quad c;

	if( transition_state == AS_TO_TITLE )
		c = LerpQuad( s_CurrentLerpQuads[0], s_CurrentLerpQuads[1], frac );
	else if( transition_state == AS_TO_BUTTON )
		c = LerpQuad( s_CurrentLerpQuads[1], s_CurrentLerpQuads[0], frac );

	//UI_FillRect( c.x, c.y, c.lx, c.ly, 0xFF0F00FF );

	EngFuncs::PIC_Set( s_hCurrentTransPic, 255, 255, 255 );
#if !defined(TA_ALT_MODE2)
	wrect_t rect = { 0, uiStatic.buttons_width, 26, 52 };
	EngFuncs::PIC_DrawAdditive( c.x, c.y, c.lx, c.ly, &rect );
#else
	EngFuncs::PIC_DrawAdditive( c.x, c.y, c.lx, c.ly );
#endif

	return false;

}

void CMenuPicButton::ClearButtonStack()
{
	ButtonStackDepth = 0;
	memset( ButtonStack, 0, sizeof( ButtonStack ));
}

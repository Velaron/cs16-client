/*
Switch.cpp - simple switches, like Android 4.0+
Copyright (C) 2017 a1batross

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

#include "BaseMenu.h"
#include "Switch.h"

CMenuSwitch::CMenuSwitch( ) : BaseClass( )
{
	szLeftName = szRightName = NULL;
	bMouseToggle = true;
	bState = false;
	SetSize( 220, 35 );
	SetCharSize( QM_BOLDFONT );

	// text offsets are not needed anymore,
	// they are useless now
	fTextOffsetX = 0.0f;
	fTextOffsetY = 0.0f;

	eTextAlignment = QM_CENTER;
	iFlags |= QMF_DROPSHADOW;
}

void CMenuSwitch::VidInit()
{
	iSelectColor.SetDefault( uiPromptTextColor );
	iBackgroundColor.SetDefault( uiColorBlack );
	iFgTextColor.SetDefault( uiInputFgColor );
	iBgTextColor.SetDefault( uiPromptTextColor );

	BaseClass::VidInit();

	int leftSize, rightSize;

	if( szLeftName )
		leftSize = g_FontMgr.GetTextWideScaled( font, szLeftName, m_scChSize.h );
	else
		leftSize = m_scSize.w / 2;

	if( szRightName )
		rightSize = g_FontMgr.GetTextWideScaled( font, szRightName, m_scChSize.h );
	else
		rightSize = m_scSize.w / 2;

	// calculate fraction from two string sizes
	float frac = (float)leftSize / (float)( leftSize + rightSize );

	// then adjust widths
	m_leftSize.w = m_scSize.w * frac;
	m_rightSize.w = m_scSize.w - m_leftSize.w;
	m_rightSize.h = m_leftSize.h = m_scSize.h; // height... height never changes...

	m_rightPoint = m_leftPoint = m_scPos; // correct positions based on width sizes
	m_rightPoint.x += m_leftSize.w;

	m_scTextPos.x = m_scPos.x + (m_scSize.w * 1.5f );
	m_scTextPos.y = m_scPos.y;

	m_scTextSize.w = g_FontMgr.GetTextWideScaled( font, szName, m_scChSize.h );
	m_scTextSize.h = m_scChSize.h;
}

const char * CMenuSwitch::Key(int key, int down)
{
	const char *sound = NULL;

	switch( key )
	{
	case K_MOUSE1:
		if(!( iFlags & QMF_HASMOUSEFOCUS ))
			break;
		if( bMouseToggle )
		{
			sound = uiSoundGlow;
		}
		else
		{
			if( (UI_CursorInRect( m_leftPoint, m_leftSize ) && bState) ||
				(UI_CursorInRect( m_rightPoint, m_rightSize ) && !bState) )
			{
				sound = uiSoundGlow;
			}
		}
		break;
	case K_ENTER:
	case K_KP_ENTER:
	case K_SPACE:
	case K_AUX1:
		if( iFlags & QMF_MOUSEONLY )
			break;
		sound = uiSoundGlow;
		break;
	}

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
				event = QM_CHANGED;
				bState = !bState;
				SetCvarValue( bState );
			}
			_Event( event );
		}
		else if( down )
		{
			bState = !bState;
			SetCvarValue( bState );
			_Event( QM_CHANGED );
		}
	}

	if( iFlags & QMF_SILENT )
		return 0;

	return sound;
}

void CMenuSwitch::Draw( void )
{
	bool shadow = (iFlags & QMF_DROPSHADOW);

	int selectColor = iSelectColor;
	int bgColor = iBackgroundColor;
	UI_DrawString( font, m_scTextPos, m_scTextSize, szName, uiColorHelp, true, m_scChSize, eTextAlignment, shadow );

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		coord.x = m_scPos.x + 250 * uiStatic.scaleX;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}

	if( iFlags & QMF_GRAYED )
	{
		selectColor = uiColorDkGrey;
	}
	else if( bMouseToggle )
	{
		if( UI_CursorInRect( m_scPos, m_scSize ) )
		{
			selectColor = iFocusColor;
		}
	}
	else
	{
		if( bState && UI_CursorInRect( m_leftPoint, m_leftSize ) )
		{
			bgColor = iFocusColor;
		}
		else if(!bState && UI_CursorInRect( m_rightPoint, m_rightSize ))
		{
			bgColor = iFocusColor;
		}
	}

	// draw toggle rectangles
	UI_FillRect( m_leftPoint, m_leftSize, bState? bgColor: selectColor );
	UI_FillRect( m_rightPoint, m_rightSize, bState? selectColor: bgColor );

	// strings with same coordinates looks a bit ugly
	Point stringLeftPoint = m_leftPoint;
	stringLeftPoint.x += fTextOffsetX * uiStatic.scaleX;
	stringLeftPoint.y += fTextOffsetY * uiStatic.scaleY;

	Point stringRightPoint = m_rightPoint;
	stringRightPoint.x += fTextOffsetX * uiStatic.scaleX;
	stringRightPoint.y += fTextOffsetY * uiStatic.scaleY;

	UI_DrawString( font, stringLeftPoint, m_leftSize, szLeftName, bState?iBgTextColor: iFgTextColor, iColor, m_scChSize, eTextAlignment, shadow );
	UI_DrawString( font, stringRightPoint, m_rightSize, szRightName, bState?iFgTextColor:iBgTextColor, iColor, m_scChSize, eTextAlignment, shadow );

	// draw rectangle
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}

void CMenuSwitch::UpdateEditable()
{
	bState = !!EngFuncs::GetCvarFloat( m_szCvarName );
}

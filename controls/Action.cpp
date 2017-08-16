/*
Action.cpp - simple label with background item
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
#include "Action.h"
#include "Utils.h"


CMenuAction::CMenuAction() : CMenuBaseItem()
{
	m_szBackground = NULL;
	m_bfillBackground = false;
	m_iBackcolor = 0;
}

/*
=================
CMenuAction::Init
=================
*/
void CMenuAction::VidInit( )
{
	if( size.w < 1 || size.h < 1 )
	{
		if( m_szBackground )
		{
			HIMAGE handle = EngFuncs::PIC_Load( m_szBackground );
			size.w = EngFuncs::PIC_Width( handle );
			size.h = EngFuncs::PIC_Height( handle );
		}
		else
		{
			if( size.w < 1 )
				size.w = charSize.w * strlen( szName );

			if( size.h < 1 )
				size.h = charSize.h * 1.5;
		}
	}

	CalcPosition();
	CalcSizes();
}

/*
=================
CMenuAction::Key
=================
*/
const char *CMenuAction::Key( int key, int down )
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
		//if( !down ) return sound;
		if( iFlags & QMF_MOUSEONLY )
			break;
		sound = uiSoundLaunch;
		break;
	}

	if( sound && ( iFlags & QMF_SILENT ))
		sound = uiSoundNull;

	if( iFlags & QMF_ACT_ONRELEASE )
	{
		if( sound )
		{
			int	event;

			if( down )
			{
				event = QM_PRESSED;
				m_bPressed = true;
			}
			else event = QM_ACTIVATED;
			_Event( event );
		}
	}
	else if( down )
	{
		if( sound )
			_Event( QM_ACTIVATED );
	}

	return sound;
}

/*
=================
CMenuAction::Draw
=================
*/
void CMenuAction::Draw( )
{
	bool	shadow = (iFlags & QMF_DROPSHADOW);

	if( m_szBackground )
		UI_DrawPic( m_scPos, m_scSize, m_iBackcolor, m_szBackground );
	else if( m_bfillBackground )
		UI_FillRect( m_scPos, m_scSize, m_iBackcolor );

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		coord.x = m_scPos.x + 16 * uiStatic.scaleX;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}

	if( iFlags & QMF_GRAYED )
	{
		UI_DrawString( m_scPos, m_scSize, szName, uiColorDkGrey, true, m_scChSize, eTextAlignment, shadow );
		return; // grayed
	}

	if( this != m_pParent->ItemAtCursor() )
	{
		UI_DrawString( m_scPos, m_scSize, szName, iColor, false, m_scChSize, eTextAlignment, shadow );
		return; // no focus
	}

	if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
	{
		UI_DrawString( m_scPos, m_scSize, szName, iFocusColor, false, m_scChSize, eTextAlignment, shadow );
	}
	else if( eFocusAnimation == QM_PULSEIFFOCUS )
	{
		int	color;

		color = PackAlpha( iColor, 255 * (0.5 + 0.5 * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));

		UI_DrawString( m_scPos, m_scSize, szName, color, false, m_scChSize, eTextAlignment, shadow );
	}
}

void CMenuAction::SetBackground(const char *path, unsigned int color)
{
	m_szBackground = path;
	m_iBackcolor = color;
	m_bfillBackground = false;
}

void CMenuAction::SetBackground(unsigned int color)
{
	m_bfillBackground = true;
	m_szBackground = NULL;
	m_iBackcolor = color;
}

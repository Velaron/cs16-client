/*
CheckBox.h - checkbox
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
#include "CheckBox.h"
#include "Utils.h"

CMenuCheckBox::CMenuCheckBox() : CMenuEditable()
{
	SetCharSize( 12, 24 );
	SetSize( 32, 32 );
	SetPicture( UI_CHECKBOX_EMPTY,
		UI_CHECKBOX_FOCUS,
		UI_CHECKBOX_PRESSED,
		UI_CHECKBOX_ENABLED,
		UI_CHECKBOX_GRAYED );
	bChecked = false;
	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	iFlags |= QMF_ACT_ONRELEASE|QMF_DROPSHADOW;
	iColor = uiColorWhite;
	iFocusColor = uiColorWhite;
	iMask = 0;
}

/*
=================
CMenuCheckBox::Init
=================
*/
void CMenuCheckBox::VidInit( void )
{
	CalcPosition();
	CalcSizes();
	m_scTextPos.x = m_scPos.x + (m_scSize.w * 1.7f );
	m_scTextPos.y = m_scPos.y + (m_scSize.h >> 2);

	m_scTextSize.w = strlen( szName ) * m_scChSize.w;
	m_scTextSize.h = m_scChSize.h;
}

/*
=================
CMenuCheckBox::Key
=================
*/
const char *CMenuCheckBox::Key( int key, int down )
{
	const char	*sound = 0;

	switch( key )
	{
	case K_MOUSE1:
		if(!( iFlags & QMF_HASMOUSEFOCUS ))
			break;
		sound = uiSoundGlow;
		break;
	case K_ENTER:
	case K_KP_ENTER:
	case K_AUX1:
		//if( !down ) return sound;
		if( iFlags & QMF_MOUSEONLY )
			break;
		sound = uiSoundGlow;
		break;
	}

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
			else
			{
				m_bPressed = false;
				event = QM_CHANGED;
				bChecked = !bChecked;	// apply on release
				SetCvarValue( bChecked );

			}
			_Event( event );
		}
	}
	else if( down )
	{
		if( sound )
		{
			bChecked = !bChecked;
			SetCvarValue( bChecked );
			_Event( QM_CHANGED );
		}
	}

	if( iFlags & QMF_SILENT )
		return 0;
	return sound;
}

/*
=================
CMenuCheckBox::Draw
=================
*/
void CMenuCheckBox::Draw( void )
{
	bool shadow = (iFlags & QMF_DROPSHADOW);

	UI_DrawString( m_scTextPos, m_scTextSize, szName, uiColorHelp, true, m_scChSize, eTextAlignment, shadow );

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		if( szName[0] )
			coord.x = 250 * uiStatic.scaleX;
		else
			coord.x = m_scSize.w + 16 * uiStatic.scaleX;
		coord.x += m_scPos.x;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}

	if( iFlags & QMF_GRAYED )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, szGrayedPic );
		return; // grayed
	}

	if((( iFlags & QMF_MOUSEONLY ) && !( iFlags & QMF_HASMOUSEFOCUS ))
	   || ( this != m_pParent->ItemAtCursor() ) )
	{
		if( !bChecked )
			UI_DrawPic( m_scPos, m_scSize, iColor, szEmptyPic );
		else UI_DrawPic( m_scPos, m_scSize, iColor, szCheckPic );
		return; // no focus
	}

	if( m_bPressed )
	{
		UI_DrawPic( m_scPos, m_scSize, iColor, szPressPic );
	}
	else if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
	{
		if( bChecked )
		{
			// use two textures for it. Second is just focus texture, slightly orange. Looks pretty.
			UI_DrawPic( m_scPos, m_scSize, iColor, szPressPic );
			UI_DrawPicAdditive( m_scPos, m_scSize, uiInputTextColor, szFocusPic );
		}
		else
		{
			UI_DrawPic( m_scPos, m_scSize, iFocusColor, szFocusPic );
		}
	}
	else if( bChecked )
	{
		UI_DrawPic( m_scPos, m_scSize, iColor, szCheckPic );
	}
	else
	{
		UI_DrawPic( m_scPos, m_scSize, iColor, szEmptyPic );
	}
}

void CMenuCheckBox::UpdateEditable()
{
	bChecked = !!EngFuncs::GetCvarFloat( m_szCvarName );
}

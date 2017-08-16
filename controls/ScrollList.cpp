/*
ScrollList.h - animated button with picture
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
#include "ScrollList.h"
#include "Utils.h"
#include "Scissor.h"

CMenuScrollList::CMenuScrollList() : CMenuBaseItem()
{
	// iCurItem = 0;
	iTopItem = 0;
	iHighlight = -1;

	pszItemNames = NULL;
	iNumItems = 0;

	SetUpArrowPicture( UI_UPARROW, UI_UPARROWFOCUS, UI_UPARROWPRESSED );
	SetDownArrowPicture( UI_DOWNARROW, UI_DOWNARROWFOCUS, UI_DOWNARROWPRESSED );

	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	SetCharSize( QM_SMALLFONT );
	bFramedHintText = false;
}


/*
=================
menuScrollList_t::Init
=================
*/
void CMenuScrollList::VidInit( )
{
	CalcPosition();
	CalcSizes();

	for( iNumItems = 0; pszItemNames && pszItemNames[iNumItems]; iNumItems++ );

	if( m_scChSize.h )
	{
		iNumRows = (m_scSize.h / m_scChSize.h) - 2;
		if( iNumRows > iNumItems )
			iNumRows = iNumItems;
	}
}
/*
=================
menuScrollList_t::Key
=================
*/
const char *CMenuScrollList::Key( int key, int down )
{
	const char	*sound = 0;
	Point upArrow, downArrow;
	Size arrow;
	int		i, y;
	bool noscroll = false;

	if( !down )
	{
		iScrollBarSliding = false;
		return uiSoundNull;
	}

	switch( key )
	{
	case K_MOUSE1:
		noscroll = true; // don't scroll to current when mouse used
		if(!( iFlags & QMF_HASMOUSEFOCUS ))
			break;

		// use fixed size for arrows
		arrow.w = 24;
		arrow.h = 24;

		UI_ScaleCoords( NULL, NULL, &arrow.w, &arrow.h );

		// glue with right top and right bottom corners
		upArrow.x = m_scPos.x + m_scSize.w - arrow.w;
		upArrow.y = m_scPos.y + UI_OUTLINE_WIDTH;
		downArrow.x = m_scPos.x + m_scSize.w - arrow.w;
		downArrow.y = m_scPos.y + (m_scSize.h - arrow.h) - UI_OUTLINE_WIDTH;

		// ADAMIX
		if( UI_CursorInRect( upArrow.x, upArrow.y + arrow.h, arrow.w, iScrollBarY - upArrow.y - arrow.h ) ||
			  UI_CursorInRect( upArrow.x, iScrollBarY + iScrollBarHeight , arrow.w, downArrow.y - ( iScrollBarY + iScrollBarHeight ) ) )
		{
			iScrollBarSliding = true;
		}
		// ADAMIX END

		// Now see if either upArrow or downArrow has focus
		if( UI_CursorInRect( upArrow, arrow ))
		{
			if( iTopItem > 5 )
			{
				iTopItem-=5;
				sound = uiSoundMove;
			}
			else
			{
				iTopItem = 0;
				sound = uiSoundBuzz;
			}
			break;
		}
		else if( UI_CursorInRect( downArrow, arrow ))
		{
			if( iTopItem < iNumItems - iNumRows - 5 )
			{
				iTopItem+=5;
				sound = uiSoundMove;
			}
			else
			{
				iTopItem = iNumItems - iNumRows;
				sound = uiSoundBuzz;
			}
			break;
		}

		// see if an item has been selected
		y = m_scPos.y + m_scChSize.h;
		for( i = iTopItem; i < iTopItem + iNumRows; i++, y += m_scChSize.h )
		{
			if( !pszItemNames[i] )
				break; // done

			if( UI_CursorInRect( m_scPos.x, y, m_scSize.w - arrow.w, m_scChSize.h ))
			{
				iCurItem = i;
				sound = uiSoundNull;
				break;
			}
		}
		break;
	case K_HOME:
	case K_KP_HOME:
		if( iCurItem != 0 )
		{
			iCurItem = 0;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_END:
	case K_KP_END:
		if( iCurItem != iNumItems - 1 )
		{
			iCurItem = iNumItems - 1;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_PGUP:
	case K_KP_PGUP:
		if( iCurItem != 0 )
		{
			iCurItem -= 2;
			if( iCurItem < 0 )
				iCurItem = 0;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_PGDN:
	case K_KP_PGDN:
		if( iCurItem != iNumItems - 1 )
		{
			iCurItem += 2;
			if( iCurItem > iNumItems - 1 )
				iCurItem = iNumItems - 1;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_MWHEELUP:
		if( iCurItem != 0 )
		{
			iCurItem--;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		if( iNumItems > 0 && iCurItem != iNumItems - 1 )
		{
			iCurItem++;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_BACKSPACE:
	case K_DEL:
	case K_AUX30:
		if( onDeleteEntry ) onDeleteEntry( this );
		break;
	case K_ENTER:
	case K_AUX1:
	case K_AUX31:
	case K_AUX32:
		if( onActivateEntry ) onActivateEntry( this );
		break;
	}
	if( !noscroll )
	{
		if( iCurItem < iTopItem )
			iTopItem = iCurItem;
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem < 0 ) iTopItem = 0;
		if( iTopItem > iNumItems - iNumRows )
			iTopItem = iNumItems - iNumRows;
	}

	if( sound && ( iFlags & QMF_SILENT ))
		sound = uiSoundNull;

	if( sound )
	{
		if( sound != uiSoundBuzz )
		{
			// SetCvarString ?
			_Event( QM_CHANGED );
		}
	}
	return sound;
}

/*
=================
menuScrollList_t::Draw
=================
*/
void CMenuScrollList::Draw( )
{
	bool	shadow;
	int	i, x, y, w, h;
	int	selColor = 0xFF503818; // Red 80, Green 56, Blue 24, Alpha 255
	int	upFocus, downFocus, scrollbarFocus;
	Point up, down;

	shadow = (iFlags & QMF_DROPSHADOW);

	// use fixed size for arrows
	Size arrow( 24, 24 );

	arrow = arrow.Scale();

	x = m_scPos.x;
	y = m_scPos.y;
	w = m_scSize.w;
	h = m_scSize.h;

	if( !szBackground )
	{
		// draw the opaque outlinebox first
		UI_FillRect( x, y, w, h, uiColorBlack );
	}

	if( szName )
	{
		if( bFramedHintText )
		{
			UI_FillRect( x, y - m_scChSize.h * 1.75, w, m_scChSize.h * 1.75, uiColorBlack );
			UI_DrawRectangle( x, y - m_scChSize.h * 1.75, w, m_scChSize.h * 1.75, uiInputFgColor );
		}

		UI_DrawString( x, y - m_scChSize.h * 1.5, w, h, szName, uiColorHelp, true, m_scChSize.w, m_scChSize.h, eTextAlignment, shadow );

	}


	// hightlight the selected item
	if( !( iFlags & QMF_GRAYED ))
	{
		y = m_scPos.y + m_scChSize.h;
		for( i = iTopItem; i < iTopItem + iNumRows && pszItemNames[i]; i++, y += m_scChSize.h )
		{
			// hightlighted item
			if( i == iHighlight )
			{
				UI_FillRect( m_scPos.x, y, m_scSize.w - arrow.w, m_scChSize.h, 0xFF383838 );
			}

			if( i == iCurItem )
			{
				UI_FillRect( m_scPos.x, y, m_scSize.w - arrow.w, m_scChSize.h, selColor );
			}
		}
	}

	if( szBackground )
	{
		// get size and position for the center box
		UI_DrawPic( x, y, w, h, uiColorWhite, szBackground );
	}
	else
	{

		int color;

		if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS && iFlags & QMF_HASKEYBOARDFOCUS )
			color = uiInputTextColor;
		else
			color = uiInputFgColor;

		x = m_scPos.x - UI_OUTLINE_WIDTH;
		y = m_scPos.y;
		w = UI_OUTLINE_WIDTH;
		h = m_scSize.h;

		// draw left
		UI_FillRect( x, y, w, h, color );

		x = m_scPos.x + m_scSize.w;
		y = m_scPos.y;
		w = UI_OUTLINE_WIDTH;
		h = m_scSize.h;

		// draw right
		UI_FillRect( x, y, w, h, color );

		x = m_scPos.x;
		y = m_scPos.y;
		w = m_scSize.w + UI_OUTLINE_WIDTH;
		h = UI_OUTLINE_WIDTH;

		// draw top
		UI_FillRect( x, y, w, h, color );

		// draw bottom
		x = m_scPos.x;
		y = m_scPos.y + m_scSize.h - UI_OUTLINE_WIDTH;
		w = m_scSize.w + UI_OUTLINE_WIDTH;
		h = UI_OUTLINE_WIDTH;

		UI_FillRect( x, y, w, h, color );
	}

	// glue with right top and right bottom corners
	up.x = m_scPos.x + m_scSize.w - arrow.w;
	up.y = m_scPos.y + UI_OUTLINE_WIDTH;
	down.x = m_scPos.x + m_scSize.w - arrow.w;
	down.y = m_scPos.y + (m_scSize.h - arrow.h) - UI_OUTLINE_WIDTH;

	float step = (iNumItems <= 1 ) ? 1 : (down.y - up.y - arrow.h) / (float)(iNumItems - 1);

	if( g_bCursorDown && !iScrollBarSliding && ( iFlags & QMF_HASMOUSEFOCUS ) )
	{
		if( UI_CursorInRect( m_scPos.x, m_scPos.y, m_scSize.w - arrow.w, m_scSize.h ))
		{
			static float ac_y = 0;
			ac_y += cursorDY;
			cursorDY = 0;
			if( ac_y > m_scChSize.h / 2.0f )
			{
				iTopItem -= ac_y/ m_scChSize.h - 0.5;
				if( iTopItem < 0 )
					iTopItem = 0;
				ac_y = 0;
			}
			if( ac_y < -m_scChSize.h / 2.0f )
			{
				iTopItem -= ac_y/ m_scChSize.h - 0.5 ;
				if( iTopItem > iNumItems - iNumRows )
					iTopItem = iNumItems - iNumRows;
				ac_y = 0;
			}
		}
		else if( UI_CursorInRect( iScrollBarX, iScrollBarY, iScrollBarWidth, iScrollBarHeight ))
		{
			static float ac_y = 0;
			ac_y += cursorDY;
			cursorDY = 0;
			if( ac_y < -step )
			{
				iTopItem += ac_y / step + 0.5;
				if( iTopItem < 0 )
					iTopItem = 0;
				ac_y = 0;
			}
			if( ac_y > step )
			{
				iTopItem += ac_y / step + 0.5;
				if( iTopItem > iNumItems - iNumRows )
					iTopItem = iNumItems - iNumRows;
				ac_y = 0;
			}
		}

	}

	// draw the arrows base
	UI_FillRect( up.x, up.y + arrow.h, arrow.w, down.y - up.y - arrow.h, uiInputFgColor );


	// ADAMIX
	iScrollBarX = up.x + m_scChSize.h/4;
	iScrollBarWidth = arrow.w - m_scChSize.h/4;

	if(((down.y - up.y - arrow.h) - (((iNumItems-1)*m_scChSize.h)/2)) < 2)
	{
		iScrollBarHeight = (down.y - up.y - arrow.h) - (step * (iNumItems - iNumRows));
		iScrollBarY = up.y + arrow.h + (step*iTopItem);
	}
	else
	{
		iScrollBarHeight = down.y - up.y - arrow.h - (((iNumItems- iNumRows) * m_scChSize.h) / 2);
		iScrollBarY = up.y + arrow.h + (((iTopItem) * m_scChSize.h)/2);
	}

	if( iScrollBarSliding )
	{
		int dist = uiStatic.cursorY - iScrollBarY - (iScrollBarHeight>>1);

		if((((dist / 2) > (m_scChSize.h / 2)) || ((dist / 2) < (m_scChSize.h / 2))) && iTopItem <= (iNumItems - iNumRows ) && iTopItem >= 0)
		{
			//_Event( QM_CHANGED );

			if((dist / 2) > ( m_scChSize.h / 2 ) && iTopItem < ( iNumItems - iNumRows - 1 ))
			{
				iTopItem++;
			}

			if((dist / 2) < -(m_scChSize.h / 2) && iTopItem > 0 )
			{
				iTopItem--;
			}
		}

		//iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem < 0 ) iTopItem = 0;
		if( iTopItem > ( iNumItems - iNumRows - 1 ))
			iTopItem = iNumItems - iNumRows - 1;
	}

	if( iScrollBarSliding )
	{
		// Draw scrollbar background
		UI_FillRect ( iScrollBarX, up.y + arrow.h, iScrollBarWidth, down.y - up.y - arrow.h, uiColorBlack);
	}

	// ADAMIX END
	// draw the arrows
	if( iFlags & QMF_GRAYED )
	{
		UI_DrawPic( up.x, up.y, arrow.w, arrow.h, uiColorDkGrey, szUpArrow );
		UI_DrawPic( down.x, down.y, arrow.w, arrow.h, uiColorDkGrey, szDownArrow );
	}
	else
	{
		scrollbarFocus = UI_CursorInRect( iScrollBarX, iScrollBarY, iScrollBarWidth, iScrollBarHeight );

		// special case if we sliding but lost focus
		if( iScrollBarSliding ) scrollbarFocus = true;

		// Draw scrollbar itself
		UI_FillRect( iScrollBarX, iScrollBarY, iScrollBarWidth, iScrollBarHeight, scrollbarFocus ? uiInputTextColor : uiColorBlack );

		if(this != m_pParent->ItemAtCursor())
		{
			UI_DrawPic( up.x, up.y, arrow.w, arrow.h, uiColorWhite, szUpArrow );
			UI_DrawPic( down.x, down.y, arrow.w, arrow.h, uiColorWhite, szDownArrow );
		}
		else
		{
			// see which arrow has the mouse focus
			upFocus = UI_CursorInRect( up.x, up.y, arrow.w, arrow.h );
			downFocus = UI_CursorInRect( down.x, down.y, arrow.w, arrow.h );

			if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
			{
				UI_DrawPic( up.x, up.y, arrow.w, arrow.h, uiColorWhite, (upFocus) ? szUpArrowFocus : szUpArrow );
				UI_DrawPic( down.x, down.y, arrow.w, arrow.h, uiColorWhite, (downFocus) ? szDownArrowFocus : szDownArrow );
			}
			else if( eFocusAnimation == QM_PULSEIFFOCUS )
			{
				int	color;

				color = PackAlpha( iColor, 255 * (0.5 + 0.5 * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));

				UI_DrawPic( up.x, up.y, arrow.w, arrow.h, (upFocus) ? color : iColor, (upFocus) ? szUpArrowFocus : szUpArrow );
				UI_DrawPic( down.x, down.y, arrow.w, arrow.h, (downFocus) ? color : iColor, (downFocus) ? szDownArrowFocus : szDownArrow );
			}
		}
	}

	// Draw the list
	x = m_scPos.x;
	w = m_scSize.w;
	h = m_scChSize.h;
	y = m_scPos.y + m_scChSize.h;

	// prevent the columns out of rectangle bounds
	UI::PushScissor( x, y, m_scSize.w - arrow.w - uiStatic.outlineWidth, m_scSize.h );

	for( i = iTopItem; i < iTopItem + iNumRows; i++, y += m_scChSize.h )
	{
		if( !pszItemNames[i] )
			break;	// done

		if( iFlags & QMF_GRAYED )
		{
			UI_DrawString( x, y, w, h, pszItemNames[i], uiColorDkGrey, true, m_scChSize.w, m_scChSize.h, eTextAlignment, shadow );
			continue;	// grayed
		}

		if( i != iCurItem )
		{
			UI_DrawString( x, y, w, h, pszItemNames[i], iColor, false, m_scChSize.w, m_scChSize.h, eTextAlignment, shadow );
			continue;	// no focus
		}

		if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
			UI_DrawString( x, y, w, h, pszItemNames[i], iFocusColor, false, m_scChSize.w, m_scChSize.h, eTextAlignment, shadow );
		else if( eFocusAnimation == QM_PULSEIFFOCUS )
		{
			int	color;

			color = PackAlpha( iColor, 255 * (0.5 + 0.5 * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));

			UI_DrawString( x, y, w, h, pszItemNames[i], color, false, m_scChSize.w, m_scChSize.h, eTextAlignment, shadow );
		}
	}

	UI::PopScissor();
}

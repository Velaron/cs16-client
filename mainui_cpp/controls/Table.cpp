/*
Table.cpp
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
#include "Table.h"
#include "Utils.h"
#include "Scissor.h"

CMenuTable::CMenuTable() : BaseClass(),
	bFramedHintText( false ),
	szHeaderTexts(), szBackground(),
	szUpArrow( UI_UPARROW ), szUpArrowFocus( UI_UPARROWFOCUS ), szUpArrowPressed( UI_UPARROWPRESSED ),
	szDownArrow( UI_DOWNARROW ), szDownArrowFocus( UI_DOWNARROWFOCUS ), szDownArrowPressed( UI_DOWNARROWPRESSED ),
	iTopItem( 0 ),
	iScrollBarSliding( false ),
	iHighlight( -1 ), iCurItem( 0 ),
	m_iLastItemMouseChange( 0 ), m_pModel( NULL )
{
	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	SetCharSize( QM_SMALLFONT );
}

void CMenuTable::VidInit()
{
	BaseClass::VidInit();

	iNumRows = ( m_scSize.h - UI_OUTLINE_WIDTH * 2 ) / m_scChSize.h - 1;

	if( !iCurItem )
	{
		if( iCurItem < iTopItem )
			iTopItem = iCurItem;
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem > m_pModel->GetRows() - iNumRows )
			iTopItem = m_pModel->GetRows() - iNumRows;
		if( iTopItem < 0 )
			iTopItem = 0;
	}


	flFixedSumm = 0.0f;
	flDynamicSumm = 0.0f;

	for( int i = 0; i < m_pModel->GetColumns(); i++ )
	{
		// this isn't valid behaviour, but still enough for tables without
		// set columns width
		if( !columns[i].flWidth )
		{
			SetColumnWidth( i, 1 / m_pModel->GetColumns(), false );
		}

		if( columns[i].fStaticWidth )
			flFixedSumm += columns[i].flWidth;
		else
			flDynamicSumm += columns[i].flWidth;
	}

	flFixedSumm *= uiStatic.scaleX;
}

bool CMenuTable::MoveView(int delta )
{
	iTopItem += delta;

	if( iTopItem < abs(delta) )
	{
		iTopItem = 0;
		return false;
	}
	else if( iTopItem > m_pModel->GetRows() - iNumRows )
	{
		if( m_pModel->GetRows() - iNumRows < 0 )
			iTopItem = 0;
		else
			iTopItem = m_pModel->GetRows() - iNumRows;
		return false;
	}

	return true;
}

bool CMenuTable::MoveCursor(int delta)
{
	iCurItem += delta;

	if( iCurItem < 0 )
	{
		iCurItem = 0;
		return false;
	}
	else if( iCurItem > m_pModel->GetRows() - 1 )
	{
		iCurItem = m_pModel->GetRows() - 1;
		return false;
	}
	return true;
}

void CMenuTable::SetCurrentIndex( int idx )
{
	iCurItem = bound( 0, idx, m_pModel->GetRows() );

	if( iCurItem < iTopItem )
		iTopItem = iCurItem;
	if( iNumRows ) // check if already vidinit
	{
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem > m_pModel->GetRows() - iNumRows )
			iTopItem = m_pModel->GetRows() - iNumRows;
		if( iTopItem < 0 )
			iTopItem = 0;
	}
	else
	{
		iTopItem = 0; // will be recalculated on vidinit
	}
}

const char *CMenuTable::Key( int key, int down )
{
	const char *sound = 0;
	int i, y;
	bool noscroll = false;
	Point upArrow, downArrow;
	Size arrow;

	if( !down )
	{
		iScrollBarSliding = false;
		return uiSoundNull;
	}

	switch( key )
	{
	case K_MOUSE1:
	{
		noscroll = true; // don't scroll to current when mouse used

		if( !( iFlags & QMF_HASMOUSEFOCUS ) )
			break;

		arrow.w = arrow.h = 24;
		arrow = arrow.Scale();

		// glue with right top and right bottom corners
		upArrow.x = m_scPos.x + m_scSize.w - arrow.w;
		upArrow.y = m_scPos.y;
		downArrow.x = m_scPos.x + m_scSize.w - arrow.w;
		downArrow.y = m_scPos.y + (m_scSize.h - arrow.h);

		if( uiStatic.cursorX > m_scPos.x + m_scSize.w - arrow.w )
		{
			// ADAMIX
			if( UI_CursorInRect( upArrow.x, upArrow.y + arrow.h, arrow.w, iScrollBarY - upArrow.y - arrow.h ) ||
				  UI_CursorInRect( upArrow.x, iScrollBarY + iScrollBarHeight , arrow.w, downArrow.y - ( iScrollBarY + iScrollBarHeight ) ) )
			{
				iScrollBarSliding = true;
			}
			// ADAMIX END

			// test for arrows
			if( UI_CursorInRect( upArrow, arrow ) )
			{
				if( MoveView( -5 ) )
					sound = uiSoundMove;
				else sound = uiSoundBuzz;
			}
			else if( UI_CursorInRect( downArrow, arrow ))
			{
				if( MoveView( 5 ) )
					sound = uiSoundMove;
				else sound = uiSoundBuzz;
			}
		}
		else
		{
			// test for item select
			int starty = m_scPos.y + m_scChSize.h + UI_OUTLINE_WIDTH;
			int endy = starty + iNumRows * m_scChSize.h;
			if( uiStatic.cursorY > starty && uiStatic.cursorY < endy )
			{
				int offsety = uiStatic.cursorY - starty;
				int newCur = iTopItem + offsety / m_scChSize.h;

				if( newCur < m_pModel->GetRows() )
				{
					if( newCur == iCurItem )
					{
						if( uiStatic.realTime - m_iLastItemMouseChange < 200 ) // 200 msec to double click
						{
							m_pModel->OnActivateEntry( iCurItem );
						}
					}
					else
					{
						iCurItem = newCur;
						sound = uiSoundNull;
					}

					m_iLastItemMouseChange = uiStatic.realTime;
				}
			}
		}

		break;
	}
	case K_HOME:
	case K_KP_HOME:
		if( iCurItem )
		{
			iCurItem = 0;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_END:
	case K_KP_END:
		if( iCurItem != m_pModel->GetRows() - 1 )
		{
			iCurItem = m_pModel->GetRows() - 1;
			sound = uiSoundMove;
		}
		else sound = uiSoundBuzz;
		break;
	case K_PGDN:
	case K_KP_PGDN:
		sound = MoveCursor( 2 ) ? uiSoundMove : uiSoundBuzz;
		break;
	case K_PGUP:
	case K_KP_PGUP:
		sound = MoveCursor( -2 ) ? uiSoundMove : uiSoundBuzz;
		break;
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_MWHEELUP:
		sound = MoveCursor( -1 ) ? uiSoundMove : uiSoundBuzz;
		break;
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	case K_MWHEELDOWN:
		sound = MoveCursor( 1 ) ? uiSoundMove : uiSoundBuzz;
		break;
	case K_BACKSPACE:
	case K_DEL:
	case K_AUX30:
		m_pModel->OnDeleteEntry( iCurItem );
		break;
	case K_ENTER:
	case K_AUX1:
	case K_AUX31:
	case K_AUX32:
		m_pModel->OnActivateEntry( iCurItem );
		break;
	}

	if( !noscroll )
	{
		if( iCurItem < iTopItem )
			iTopItem = iCurItem;
		if( iCurItem > iTopItem + iNumRows - 1 )
			iTopItem = iCurItem - iNumRows + 1;
		if( iTopItem > m_pModel->GetRows() - iNumRows )
			iTopItem = m_pModel->GetRows() - iNumRows;
		if( iTopItem < 0 )
			iTopItem = 0;
	}

	if( sound )
	{
		if( iFlags & QMF_SILENT )
			sound = uiSoundNull;

		if( sound != uiSoundBuzz )
			_Event( QM_CHANGED );
	}

	return sound;
}

void CMenuTable::DrawLine( Point p, const char **psz, size_t size, int textColor, bool forceCol, int fillColor )
{
	int ix;
	size_t i;
	Size sz;
	bool shadow = iFlags & QMF_DROPSHADOW;
	int width = m_scSize.w - 24 * uiStatic.scaleX;

	sz.h = m_scChSize.h * 1.75;

	if( fillColor )
	{
		sz.w = width;
		UI_FillRect( p, sz, fillColor );
	}

	for( ix = p.x, i = 0;
		i < size;
		i++, ix += sz.w )
	{
		if( columns[i].fStaticWidth )
			sz.w = columns[i].flWidth * uiStatic.scaleX;
		else
			sz.w = ((float)width - flFixedSumm) * columns[i].flWidth / flDynamicSumm;

		if( !psz[i] ) // headers may be null, cells too
			continue;

		UI_DrawString( font,
			ix, p.y,
			sz.w, sz.h,
			psz[i],
			textColor, forceCol,
			m_scChSize.w, m_scChSize.h,
			m_pModel->GetAlignmentForColumn( i ),
			shadow, false );
	}
}

void CMenuTable::DrawLine( Point p, int line, int textColor, bool forceCol, int fillColor )
{
	int ix, i;
	Size sz;
	bool shadow = iFlags & QMF_DROPSHADOW;
	int width = m_scSize.w - 24 * uiStatic.scaleX;

	sz.h = m_scChSize.h;

	if( fillColor )
	{
		sz.w = width;
		UI_FillRect( p, sz, fillColor );
	}

	for( ix = p.x, i = 0;
		i < m_pModel->GetColumns();
		i++, ix += sz.w )
	{
		if( columns[i].fStaticWidth )
			sz.w = columns[i].flWidth * uiStatic.scaleX;
		else
			sz.w = ((float)width - flFixedSumm) * columns[i].flWidth / flDynamicSumm;

		const char *str = m_pModel->GetCellText( line, i );
		const ECellType type = m_pModel->GetCellType( line, i );

		if( !str /* && type != CELL_ITEM  */) // headers may be null, cells too
			continue;

		switch( type )
		{
		case CELL_TEXT:
			UI_DrawString( font,
				ix, p.y,
				sz.w, sz.h,
				str,
				textColor, forceCol,
				m_scChSize.w, m_scChSize.h,
				m_pModel->GetAlignmentForColumn( i ),
				shadow,
				m_pModel->IsCellTextWrapped( line, i ) );
			break;
		case CELL_IMAGE_ADDITIVE:
		case CELL_IMAGE_DEFAULT:
		case CELL_IMAGE_HOLES:
		case CELL_IMAGE_TRANS:
		{
			HIMAGE pic = EngFuncs::PIC_Load( str );

			if( !pic )
				continue;

			int height = EngFuncs::PIC_Height( pic );
			int width = EngFuncs::PIC_Width( pic );
			float scale = (float)m_scChSize.h/(float)height;

			width = width * scale;
			height = height * scale;

			int x;

			switch( m_pModel->GetAlignmentForColumn( i ) )
			{
			case QM_LEFT: x = ix; break;
			case QM_RIGHT: x = ix + ( sz.w - width ); break;
			case QM_CENTER: x = ix + ( sz.w - width ) / 2; break;
			default: break;
			}

			EngFuncs::PIC_Set( pic, 255, 255, 255 );

			switch( type )
			{
			case CELL_IMAGE_ADDITIVE:
				EngFuncs::PIC_DrawAdditive( x, p.y, width, height );
				break;
			case CELL_IMAGE_DEFAULT:
				EngFuncs::PIC_Draw( x, p.y, width, height );
				break;
			case CELL_IMAGE_HOLES:
				EngFuncs::PIC_DrawHoles( x, p.y, width, height );
				break;
			case CELL_IMAGE_TRANS:
				EngFuncs::PIC_DrawTrans( x, p.y, width, height );
				break;
			default: break; // shouldn't happen
			}

			break;
		}
		}
	}
}


void CMenuTable::Draw()
{
	int i, x, y, w, h;
	int selColor = PackRGB( 80, 56, 24 );
	int upFocus, downFocus, scrollbarFocus;

	Point up, down;

	// use fixed size for arrows
	Size arrow( 24, 24 );

	arrow = arrow.Scale();

	x = m_scPos.x;
	y = m_scPos.y;
	w = m_scSize.w;
	h = m_scSize.h;

	// HACKHACK: recalc iNumRows, to be not greater than iNumItems
	iNumRows = ( m_scSize.h - UI_OUTLINE_WIDTH * 2 ) / m_scChSize.h - 1;
	if( iNumRows > m_pModel->GetRows() )
		iNumRows = m_pModel->GetRows();

	Point listPt( m_scPos.x, m_scPos.y + m_scChSize.h + UI_OUTLINE_WIDTH );
	Size listSz( m_scSize.w, iNumRows * m_scChSize.h );
	if( UI_CursorInRect( listPt, listSz ) )
	{
		int newCur = iTopItem + ( uiStatic.cursorY - listPt.y ) / m_scChSize.h;

		if( newCur < m_pModel->GetRows() )
			iHighlight = newCur;
		else iHighlight = -1;
	}
	else iHighlight = -1;

	if( szBackground )
	{
		UI_DrawPic( x, y, w, h, uiColorWhite, szBackground );
	}
	else
	{
		// draw the opaque outlinebox first
		UI_FillRect( x, y, w, h, uiColorBlack );
	}

	int columns = Q_min( m_pModel->GetColumns(), MAX_TABLE_COLUMNS );

	Point headerPos = m_scPos;
	headerPos.y -= m_scChSize.h * 1.75 - UI_OUTLINE_WIDTH / 2;

	if( bFramedHintText )
	{
		UI_FillRect( headerPos.x, headerPos.y, w, m_scChSize.h * 1.75, uiColorBlack );
		UI_DrawRectangle( headerPos.x, headerPos.y, w, m_scChSize.h * 1.75, uiInputFgColor, QM_LEFT | QM_TOP | QM_RIGHT );
	}

	DrawLine( headerPos, szHeaderTexts, columns, uiColorHelp, true );

	if( !szBackground )
	{
		int color;

		if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS && iFlags & QMF_HASKEYBOARDFOCUS )
			color = uiInputTextColor;
		else
			color = uiInputFgColor;

		UI_DrawRectangle( m_scPos, m_scSize, color );
	}

	// glue with right top and right bottom corners
	up.x = m_scPos.x + m_scSize.w - arrow.w;
	up.y = m_scPos.y;
	down.x = m_scPos.x + m_scSize.w - arrow.w;
	down.y = m_scPos.y + (m_scSize.h - arrow.h);

	float step = (m_pModel->GetRows() <= 1 ) ? 1 : (down.y - up.y - arrow.h) / (float)(m_pModel->GetRows() - 1);

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
				if( iTopItem > m_pModel->GetRows() - iNumRows )
					iTopItem = m_pModel->GetRows() - iNumRows;
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
				if( iTopItem > m_pModel->GetRows() - iNumRows )
					iTopItem = m_pModel->GetRows() - iNumRows;
				ac_y = 0;
			}
		}
	}

	// draw the arrows base
	UI_FillRect( up.x, up.y + arrow.h,
		arrow.w, down.y - up.y - arrow.h, uiInputFgColor );

	// ADAMIX
	iScrollBarX = up.x + m_scChSize.h/4;
	iScrollBarWidth = arrow.w - m_scChSize.h/4;

	if(((down.y - up.y - arrow.h) - (((m_pModel->GetRows()-1)*m_scChSize.h)/2)) < 2)
	{
		iScrollBarHeight = (down.y - up.y - arrow.h) - (step * (m_pModel->GetRows() - iNumRows));
		iScrollBarY = up.y + arrow.h + (step*iTopItem);
	}
	else
	{
		iScrollBarHeight = down.y - up.y - arrow.h - (((m_pModel->GetRows()- iNumRows) * m_scChSize.h) / 2);
		iScrollBarY = up.y + arrow.h + (((iTopItem) * m_scChSize.h)/2);
	}

	if( iScrollBarSliding )
	{
		int dist = uiStatic.cursorY - iScrollBarY - (iScrollBarHeight>>1);

		if((((dist / 2) > (m_scChSize.h / 2)) || ((dist / 2) < (m_scChSize.h / 2))) && iTopItem <= (m_pModel->GetRows() - iNumRows ) && iTopItem >= 0)
		{
			//_Event( QM_CHANGED );

			if((dist / 2) > ( m_scChSize.h / 2 ) && iTopItem < ( m_pModel->GetRows() - iNumRows - 1 ))
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
		if( iTopItem > ( m_pModel->GetRows() - iNumRows - 1 ))
			iTopItem = m_pModel->GetRows() - iNumRows - 1;
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

				UI_DrawPic( up.x, up.y, arrow.w, arrow.h, (upFocus) ? color : (int)iColor, (upFocus) ? szUpArrowFocus : szUpArrow );
				UI_DrawPic( down.x, down.y, arrow.w, arrow.h, (downFocus) ? color : (int)iColor, (downFocus) ? szDownArrowFocus : szDownArrow );
			}
		}
	}

	x = m_scPos.x;
	w = m_scSize.w;
	h = m_scChSize.h;
	y = m_scPos.y + m_scChSize.h;

	// prevent the columns out of rectangle bounds
	UI::Scissor::PushScissor( x, y, m_scSize.w - arrow.w - uiStatic.outlineWidth, m_scSize.h );

	for( i = iTopItem; i < m_pModel->GetRows() && i < iNumRows + iTopItem; i++, y += m_scChSize.h )
	{
		int color = iColor; // predict state
		bool forceCol = false;
		int fillColor = 0;

		if( iFlags & QMF_GRAYED )
		{
			color = uiColorDkGrey;
			forceCol = true;
		}
		else if( i == iCurItem )
		{
			if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
				color = iFocusColor;
			else if( eFocusAnimation == QM_PULSEIFFOCUS )
				color = PackAlpha( iColor, 255 * (0.5 + 0.5 * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));

			fillColor = selColor;
		}
		else if( i == iHighlight )
		{
			fillColor = 0x80383838;
		}

		DrawLine( Point( x, y ), i, color, forceCol, fillColor );
	}

	UI::Scissor::PopScissor();
}

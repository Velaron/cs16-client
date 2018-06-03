/*
Field.h - edit field
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
#include "Field.h"
#include "Utils.h"


CMenuField::CMenuField() : BaseClass(), szBuffer()
{
	bAllowColorstrings = true;
	bHideInput = false;
	bNumbersOnly = false;

	iFlags |= QMF_DROPSHADOW;
	eTextAlignment = QM_CENTER;

	SetSize( 200, 32 );

	iMaxLength = 0;
	iCursor = 0;
	iScroll = 0;
	iWidthInChars = 0;
	szBackground = NULL;
}

void CMenuField::Init()
{
	//Clear();
	iMaxLength++;
	if( iMaxLength <= 1 || iMaxLength >= UI_MAX_FIELD_LINE )
		iMaxLength = UI_MAX_FIELD_LINE - 1;
}

/*
=================
CMenuField::Init
=================
*/
void CMenuField::VidInit( void )
{
	BaseClass::VidInit();

	// calculate number of visible characters
	iWidthInChars = (size.w / charSize.w);

	iCursor = strlen( szBuffer );
}

/*
================
CMenuField::_Event
================
*/
void CMenuField::_Event( int ev )
{
	switch( ev )
	{
	case QM_LOSTFOCUS:
		UI_EnableTextInput( false );
		VidInit();
		break;
	case QM_GOTFOCUS:
		UI_EnableTextInput( true );
		break;
	case QM_IMRESIZED:
	{
		int originalY = 0;

		if( iFlags & QMF_DISABLESCAILING )
			originalY = pos.y;
		else
			originalY = pos.Scale().y;

		if( m_pParent && !IsAbsolutePositioned() )
			originalY += m_pParent->GetRenderPosition().y;

		if( originalY > gpGlobals->scrHeight - 100 * uiStatic.scaleY )
			m_scPos.y = gpGlobals->scrHeight - 100 * uiStatic.scaleY;
		else
			VidInit();
	}
		break;
	}

	CMenuBaseItem::_Event( ev );
}

/*
================
CMenuField::Paste
================
*/
void CMenuField::Paste( void )
{
	char	*str;
	int	pasteLen, i;

	str = EngFuncs::GetClipboardData ();
	if( !str ) return;

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( str );
	for( i = 0; i < pasteLen; i++ )
		Char( str[i] );
	FREE( str );
}

/*
================
CMenuField::Clear
================
*/
void CMenuField::Clear( void )
{
	memset( szBuffer, 0, UI_MAX_FIELD_LINE );
	iCursor = 0;
	iScroll = 0;
}


/*
=================
CMenuField::Key
=================
*/
const char *CMenuField::Key( int key, int down )
{
	int	len;

	if( !down ) return 0;

	// clipboard paste
	if((( key == K_INS ) || ( key == K_KP_INS )) && EngFuncs::KEY_IsDown( K_SHIFT ))
	{
		Paste();
		return 0;
	}

	len = strlen( szBuffer );

	if( key == K_INS )
	{
		// toggle overstrike mode
		EngFuncs::KEY_SetOverstrike( !EngFuncs::KEY_GetOverstrike( ));
		return uiSoundNull; // handled
	}

	// previous character
	if( key == K_LEFTARROW )
	{
		if( iCursor > 0 ) iCursor = EngFuncs::UtfMoveLeft( szBuffer, iCursor );
		if( iCursor < iScroll ) iScroll = EngFuncs::UtfMoveLeft( szBuffer, iScroll );
		return uiSoundNull;
	}

	// next character
	if( key == K_RIGHTARROW )
	{
		if( iCursor < len ) iCursor = EngFuncs::UtfMoveRight( szBuffer, iCursor, len );
		if( iCursor >= iScroll + iWidthInChars && iCursor <= len )
			iScroll = EngFuncs::UtfMoveRight( szBuffer, iScroll, len );
		return uiSoundNull;
	}

	// first character
	if( key == K_HOME )
	{
		iCursor = 0;
		return uiSoundNull;
	}

	// last character
	if( key == K_END )
	{
		iCursor = len;
		return uiSoundNull;
	}

	if( key == K_BACKSPACE )
	{
		if( iCursor > 0 )
		{
			int pos = EngFuncs::UtfMoveLeft( szBuffer, iCursor );
			memmove( szBuffer + pos, szBuffer + iCursor, len - iCursor + 1 );
			iCursor = pos;
			if( iScroll )
				iScroll = EngFuncs::UtfMoveLeft( szBuffer, iScroll );
		}
	}
	if( key == K_DEL )
	{
		if( iCursor < len )
			memmove( szBuffer + iCursor, szBuffer + iCursor + 1, len - iCursor );
	}

	if( key == K_MOUSE1 )
	{
		float y = m_scPos.y;

		if( y > ScreenHeight - size.h - 40 )
			y = ScreenHeight - size.h - 15;

		if( UI_CursorInRect( m_scPos.x, y, m_scSize.w, m_scSize.h ) )
		{
			int x, charpos;
			char	text[UI_MAX_FIELD_LINE];

			memcpy( text, szBuffer + iScroll, iWidthInChars - iScroll );
			text[iWidthInChars] = 0;

			int w = 0;

			if( !(eTextAlignment & QM_LEFT))
				w = g_FontMgr.GetTextWide( font, text, m_scChSize.h );

			if( eTextAlignment & QM_LEFT )
			{
				x = m_scPos.x;
			}
			else if( eTextAlignment & QM_RIGHT )
			{
				x = m_scPos.x + (m_scSize.w - w);
			}
			else
			{
				x = m_scPos.x + (m_scSize.w - w) / 2;
			}
			charpos = g_FontMgr.CutText(font, szBuffer + iScroll, m_scChSize.h, uiStatic.cursorX - x, w);

			//text[charpos] = 0;
			iCursor = charpos + iScroll;
			if( iCursor > 0 )
			{
				iCursor = EngFuncs::UtfMoveLeft( szBuffer, iCursor );
				iCursor = EngFuncs::UtfMoveRight( szBuffer, iCursor, len );
			}
			if( charpos == 0 && iScroll )
				iScroll = EngFuncs::UtfMoveLeft( szBuffer, iScroll );
			if( charpos == iWidthInChars && iScroll < len - 1 )
				iScroll = EngFuncs::UtfMoveRight( szBuffer, iScroll, len );
			if( iScroll > len )
				iScroll = len;
			if( iCursor > len )
				iCursor = len;
		}
	}

	SetCvarString( szBuffer );
	_Event( QM_CHANGED );
	return 0;
}

/*
=================
CMenuField::Char
=================
*/
void CMenuField::Char( int key )
{
	int	len;

	if( key == 'v' - 'a' + 1 )
	{
		// ctrl-v is paste
		Paste();
		return;
	}

	if( key == 'c' - 'a' + 1 )
	{
		// ctrl-c clears the field
		Clear( );
		return;
	}

	len = strlen( szBuffer );

	if( key == 'a' - 'a' + 1 )
	{
		// ctrl-a is home
		iCursor = 0;
		iScroll = 0;
		return;
	}

	if( key == 'e' - 'a' + 1 )
	{
		// ctrl-e is end
		iCursor = len;
		iScroll = iCursor - iWidthInChars;
		return;
	}

	// ignore any other non printable chars
	//if( key < 32 ) return;

	if( key == '^' && !( bAllowColorstrings ))
	{
		// ignore color key-symbol
		return;
	}

	if( bNumbersOnly )
	{
		if( key < '0' || key > '9' )
			return;
	}

	// non-printable
	if( key < 32 )
		return;

	if( eLetterCase == QM_LOWERCASE )
		key = tolower( key );
	else if( eLetterCase == QM_UPPERCASE )
		key = toupper( key );

	if( EngFuncs::KEY_GetOverstrike( ))
	{
		if( iCursor == iMaxLength - 1 ) return;
		szBuffer[iCursor] = key;
		iCursor++;
	}
	else
	{
		// insert mode
		if( len == iMaxLength - 1 ) return; // all full
		memmove( szBuffer + iCursor + 1, szBuffer + iCursor, len + 1 - iCursor );
		szBuffer[iCursor] = key;
		iCursor++;
	}

	if( iCursor >= iWidthInChars ) iScroll = EngFuncs::UtfMoveRight(szBuffer, iScroll, len);
	if( iCursor == len + 1 ) szBuffer[iCursor] = 0;

	SetCvarString( szBuffer );
	_Event( QM_CHANGED );
}

/*
=================
CMenuField::Draw
=================
*/
void CMenuField::Draw( void )
{
	bool	shadow;
	char	text[UI_MAX_FIELD_LINE];
	int	len, drawLen, prestep;
	int	cursor, x, textHeight;
	char	cursor_char[3];
	float y = m_scPos.y;
	const bool limitBySize = false; // Field have own control about size limit

	Point newPos = m_scPos;

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		int	x;

		x = m_scPos.x + m_scSize.w + 16 * uiStatic.scaleX;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( x, m_scPos.y, szStatusText );
	}

	if( newPos.y > ScreenHeight - m_scSize.h - 40 )
	{
		if(iFlags & QMF_HASKEYBOARDFOCUS)
			newPos.y = ScreenHeight - m_scSize.h - 15;
		else
			return;
	}

	shadow = (iFlags & QMF_DROPSHADOW);

	cursor_char[1] = '\0';
	if( EngFuncs::KEY_GetOverstrike( ))
		cursor_char[0] = 11;
	else cursor_char[0] = '_';

	drawLen = iWidthInChars;
	len = strlen( szBuffer ) + 1;

	// guarantee that cursor will be visible
	if( len <= drawLen )
	{
		prestep = 0;
	}
	else
	{
		if( iScroll + drawLen > len )
		{
			iScroll = len - drawLen;
			if( iScroll < 0 ) iScroll = 0;
		}
		prestep = iScroll;
	}

	if( prestep + drawLen > len )
		drawLen = len - prestep;

	// extract <drawLen> characters from the field at <prestep>
	if( drawLen >= UI_MAX_FIELD_LINE )
		Host_Error( "CMenuField::Draw: drawLen >= UI_MAX_FIELD_LINE\n" );

	if( bHideInput )
	{
		EngFuncs::UtfProcessChar( 0 );

		const char *sz = szBuffer + prestep;
		int i, j;
		for( i = 0, j = 0; i < drawLen; i++ )
		{
			int uch = EngFuncs::UtfProcessChar( (unsigned char)sz[i] );
			if( uch )
				text[j++] = '*';
		}
		text[j] = 0;

		EngFuncs::UtfProcessChar( 0 );
	}
	else
	{
		memcpy( text, szBuffer + prestep, drawLen );
		text[drawLen] = 0;
	}

	// find cursor position
	x = drawLen - (ColorStrlen( text ) + 1 );
	if( x < 0 ) x = 0;
	cursor = ( iCursor - prestep );
	if( cursor < 0 ) cursor = 0;

	if( szBackground )
	{
		UI_DrawPic( newPos, m_scSize, uiColorWhite, szBackground );
	}
	else
	{
		// draw the background
		UI_FillRect( newPos, m_scSize, uiInputBgColor );

		// draw the rectangle
		UI_DrawRectangle( newPos, m_scSize, uiInputFgColor );
	}

	textHeight = y - (m_scChSize.h * 1.5f);
	UI_DrawString( font, m_scPos.x, textHeight, m_scSize.w, m_scChSize.h, szName, uiColorHelp, true, m_scChSize.w, m_scChSize.h, QM_LEFT, shadow, limitBySize );

	if( iFlags & QMF_GRAYED )
	{
		UI_DrawString( font, newPos, m_scSize, text, uiColorDkGrey, true, m_scChSize, eTextAlignment, shadow, limitBySize );
		return; // grayed
	}

	if(this != m_pParent->ItemAtCursor())
	{
		UI_DrawString( font, newPos, m_scSize, text, iColor, false, m_scChSize, eTextAlignment, shadow, limitBySize );
		return; // no focus
	}

	if( eTextAlignment & QM_LEFT )
	{
		x = newPos.x;
	}
	else if( eTextAlignment & QM_RIGHT )
	{
		x = newPos.x + (m_scSize.w - g_FontMgr.GetTextWideScaled( font, text, m_scChSize.h ) );
	}
	else
	{
		x = newPos.x + (m_scSize.w - g_FontMgr.GetTextWideScaled( font, text, m_scChSize.h )) / 2;
	}

	UI_DrawString( font, newPos, m_scSize, text, iColor, false, m_scChSize, eTextAlignment, shadow, limitBySize );

	int cursorOffset = cursor? g_FontMgr.GetTextWideScaled( font, text, m_scChSize.h, cursor ):0;

	// int cursorOffset = 0;

	if(( uiStatic.realTime & 499 ) < 250 )
		UI_DrawString( font, x + cursorOffset, y, m_scChSize.w, m_scSize.h, cursor_char, iColor, true, m_scChSize.w, m_scChSize.h, QM_LEFT, shadow, limitBySize );


	switch( eFocusAnimation )
	{
	case QM_HIGHLIGHTIFFOCUS:
		UI_DrawString( font, newPos, m_scSize, text, iFocusColor, false, m_scChSize, eTextAlignment, shadow, limitBySize );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( font, x + cursorOffset, y, m_scChSize.w, m_scSize.h, cursor_char, iFocusColor, true, m_scChSize.w, m_scChSize.h, QM_LEFT, shadow, limitBySize );
		break;
	case QM_PULSEIFFOCUS:
	{
		int	color;

		color = PackAlpha( iColor, 255 * (0.5 + 0.5 * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));
		UI_DrawString( font, newPos, m_scSize, text, color, false, m_scChSize, eTextAlignment, shadow, limitBySize );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( font, x + cursorOffset, y, m_scChSize.w, m_scSize.h, cursor_char, color, true, m_scChSize.w, m_scChSize.h, QM_LEFT, shadow, limitBySize );

		break;
	}
	default: break;
	}
}

void CMenuField::UpdateEditable()
{
	const char *szValue = EngFuncs::GetCvarString( m_szCvarName );

	if( szValue )
	{
		Q_strncpy( szBuffer, szValue, iMaxLength );
	}
}

bool CMenuField::KeyValueData(const char *key, const char *data)
{
	if( !strcmp( key, "maxchars" ) )
	{
		iMaxLength = atoi( data );
	}
	else if( !strcmp( key, "NumericInputOnly" ) )
	{
		bNumbersOnly = (bool) atoi( data );
	}
	else if( !strcmp( key, "textHidden" ) )
	{
		bHideInput = (bool) atoi( data );
	}
	else
	{
		return CMenuBaseItem::KeyValueData(key, data);
	}

	return true;
}

/*
BaseItem.cpp -- base menu item
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
#include "BaseItem.h"

/*
==================
CMenuBaseItem::CMenuBaseItem
==================
*/

CMenuBaseItem::CMenuBaseItem()
{
	SetNameAndStatus( "", NULL );
	SetCharSize( QM_DEFAULTFONT );
	SetCoord( 0, 0 );
	SetSize( 0, 0 );

	iFlags = 0;

	eTextAlignment = QM_TOPLEFT;
	eFocusAnimation = QM_NOFOCUSANIMATION;
	eLetterCase = QM_NOLETTERCASE;

	m_iLastFocusTime = 0;
	m_bPressed = false;

	m_pParent = NULL;

	m_bAllocName = false;
}

CMenuBaseItem::~CMenuBaseItem()
{
	if( m_bAllocName )
	{
		delete[] (char*)szName;
	}
}

void CMenuBaseItem::Init()
{
	;
}

void CMenuBaseItem::VidInit()
{
	CalcPosition();
	CalcSizes();

	iColor.SetDefault( uiPromptTextColor );
	iFocusColor.SetDefault( uiPromptFocusColor );
}

void CMenuBaseItem::Reload()
{
	;
}

void CMenuBaseItem::Draw()
{
	;
}

void CMenuBaseItem::Char(int key)
{
	;
}

const char *CMenuBaseItem::Key(int key, int down)
{
	return 0;
}

void CMenuBaseItem::SetCharSize(EFontSizes fs)
{
	font = fs + 1; // It's guaranteed that handles will match font sizes

	switch( fs )
	{
	case QM_DEFAULTFONT:
	case QM_BOLDFONT:
#ifdef MAINUI_RENDER_PICBUTTON_TEXT
	case QM_LIGHTBLUR:
	case QM_HEAVYBLUR:
#endif
		charSize.w = UI_MED_CHAR_WIDTH;
		charSize.h = UI_MED_CHAR_HEIGHT;
		break;
	case QM_SMALLFONT:
		charSize.w = UI_SMALL_CHAR_WIDTH;
		charSize.h = UI_SMALL_CHAR_HEIGHT;
		break;
	case QM_BIGFONT:
		charSize.w = UI_BIG_CHAR_WIDTH;
		charSize.h = UI_BIG_CHAR_HEIGHT;
		break;
	}
}

/*
=================
CMenuBaseItem::Activate
=================
*/
const char *CMenuBaseItem::Activate( )
{
	_Event( QM_ACTIVATED );

	if( !( iFlags & QMF_SILENT ))
		return uiSoundMove;
	return 0;
}


void CMenuBaseItem::_Event( int ev )
{
	CEventCallback callback;

	switch( ev )
	{
	case QM_CHANGED:   callback = onChanged; break;
	case QM_PRESSED:   callback = onPressed; break;
	case QM_GOTFOCUS:  callback = onGotFocus; break;
	case QM_LOSTFOCUS: callback = onLostFocus; break;
	case QM_ACTIVATED:
		if( (bool)onActivatedClActive && CL_IsActive( ))
			callback = onActivatedClActive;
		else callback = onActivated;
		break;
	}

	if( callback ) callback( this );
}

bool CMenuBaseItem::IsCurrentSelected() const
{
	if( m_pParent )
		return this == m_pParent->ItemAtCursor();
	return false;
}

void CMenuBaseItem::CalcPosition()
{
	if( iFlags & QMF_DISABLESCAILING )
		m_scPos = pos;
	else
		m_scPos = pos.Scale();

	if( m_scPos.x < 0 )
	{
		int pos;
		if( m_pParent && !IsAbsolutePositioned() )
			pos = m_pParent->GetRenderSize().w;
		else pos = ScreenWidth;

		m_scPos.x = pos + m_scPos.x;
	}

	if( m_scPos.y < 0 )
	{
		int pos;
		if( m_pParent && !IsAbsolutePositioned() )
			pos = m_pParent->GetRenderSize().h;
		else pos = ScreenHeight;

		m_scPos.y = pos + m_scPos.y;
	}

	if( !IsAbsolutePositioned() && m_pParent )
	{
		Point offset = m_pParent->GetPositionOffset();
		m_scPos += offset;
	}
}

void CMenuBaseItem::CalcSizes()
{
	if( iFlags & QMF_DISABLESCAILING )
	{
		m_scSize = size;
		m_scChSize = charSize;
	}
	else
	{
		m_scSize = size.Scale();
		m_scChSize = charSize.Scale();
	}

	if( m_scSize.w < 0 )
	{
		int size;
		if( m_pParent && !IsAbsolutePositioned() )
			size = m_pParent->GetRenderSize().w;
		else size = ScreenWidth;

		m_scSize.w = size + m_scSize.w - m_scPos.x;
	}

	if( m_scSize.h < 0 )
	{
		int size;
		if( m_pParent && !IsAbsolutePositioned() )
			size = m_pParent->GetRenderSize().h;
		else size = ScreenHeight;

		m_scSize.h = size + m_scSize.h - m_scPos.y;
	}
}

// we need to remap position, because resource files are keep screen at 640x480, but we in 1024x768
#define REMAP_RATIO ( 1.6f )

bool CMenuBaseItem::KeyValueData(const char *key, const char *data)
{
	if( !strcmp( key, "xpos" ))
	{
		int coord;
		if( data[0] == 'c' ) // center
		{
			data++;

			coord = 320 + atoi( data );
		}
		else
		{
			coord = atoi( data );
			if( coord  < 0 )
				coord += 640;
		}
		pos.x = coord * REMAP_RATIO;
	}
	else if( !strcmp( key, "ypos" ) )
	{
		int coord;
		if( data[0] == 'c' ) // center
		{
			data++;

			coord = 240 + atoi( data );
		}
		else
		{
			coord = atoi( data );
			if( coord  < 0 )
				coord += 480;
		}
		pos.y = coord * REMAP_RATIO;
	}
	else if( !strcmp( key, "wide" ) )
	{
		size.w = atoi( data ) * REMAP_RATIO;
	}
	else if( !strcmp( key, "tall" ) )
	{
		size.h = atoi( data ) * REMAP_RATIO;
	}
	else if( !strcmp( key, "visible" ) )
	{
		SetVisibility( (bool) atoi( data ) );
	}
	else if( !strcmp( key, "enabled" ) )
	{
		bool enabled = (bool) atoi( data );

		SetInactive( !enabled );
		SetGrayed( !enabled );
	}
	else if( !strcmp( key, "labelText" ) )
	{
		/*if( *data == '#')
		{
			szName = Localize( data + 1 );
			if( szName == data + 1 ) // not localized
			{
				m_bAllocName = true;
			}
		}
		else*/ m_bAllocName = true;

		if( m_bAllocName )
		{
			char *name = new char[strlen( data ) + 1];
			strcpy( name, data );

			szName = name;
		}
	}
	else if( !strcmp( key, "textAlignment" ) )
	{
		if( !strcmp( data, "west" ) )
		{
			eTextAlignment = QM_LEFT;
		}
		else if( !strcmp( data, "east" ) )
		{
			eTextAlignment = QM_RIGHT;
		}
		else
		{
			Con_DPrintf( "KeyValueData: unknown textAlignment %s\n", data );
		}
	}
	/*else if( !strcmp( key, "command" ) )
	{
		CEventCallback ev;

		if( m_pParent )
		{
			onActivated = m_pParent->FindEventByName( data );
		}
		else if( !strcmp( data, "engine " ) )
		{
			onActivated.SetCommand( FALSE, data + sizeof( "engine " ) );
		}
		else
		{
			// should not happen, as parent parses the resource file and sends KeyValueData to every item inside
			// if this happens, there is a bug
			Con_DPrintf( "KeyValueData: cannot set command '%s' on '%s'\n", data, szName );
		}
	}*/
	// TODO: nomulti, nosingle, nosteam

	return true;
}

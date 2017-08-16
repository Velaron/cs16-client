#include "BaseItem.h"

/*
==================
CMenuBaseItem::CMenuBaseItem
==================
*/

CMenuBaseItem::CMenuBaseItem()
{
	SetNameAndStatus( "", NULL );
	SetCharSize( UI_MED_CHAR_WIDTH, UI_MED_CHAR_HEIGHT );
	SetCoord( 0, 0 );
	SetSize( 0, 0 );

	iFlags = 0;

	iColor = uiPromptTextColor;
	iFocusColor = uiPromptFocusColor;

	eTextAlignment = QM_LEFT;
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
		delete[] szName;
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
	return uiSoundNull;
}

void CMenuBaseItem::SetCharSize(EFontSizes fs)
{
	switch( fs )
	{
	case QM_DEFAULTFONT:
		SetCharSize( UI_MED_CHAR_WIDTH, UI_MED_CHAR_HEIGHT );
		break;
	case QM_SMALLFONT:
		SetCharSize( UI_SMALL_CHAR_WIDTH, UI_SMALL_CHAR_HEIGHT );
		break;
	case QM_BIGFONT:
		SetCharSize( UI_BIG_CHAR_WIDTH, UI_BIG_CHAR_HEIGHT );
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

bool CMenuBaseItem::IsCurrentSelected()
{
	if( m_pParent )
		return this == m_pParent->ItemAtCursor();
	return false;
}

void CMenuBaseItem::CalcPosition()
{
	m_scPos = pos.Scale();

	if( !IsAbsolutePositioned() && m_pParent )
		m_scPos += m_pParent->m_scPos;
}

void CMenuBaseItem::CalcSizes()
{
	m_scSize = size.Scale();
	m_scChSize = charSize.Scale();
}

#define BIND_KEY_TO_INTEGER_VALUE( _key, _val ) if( !strcmp( key, (_key) ) ) { (_val) = atoi( data ); }

bool CMenuBaseItem::KeyValueData(const char *key, const char *data)
{

	if( !strcmp( key, "xpos" ) )
	{
		// TODO: special keys here
		pos.x = atoi( data );
	}
	else if( !strcmp( key, "ypos" ) )
	{
		pos.y = atoi( data );
	}
	else BIND_KEY_TO_INTEGER_VALUE( "wide", size.w )
	else BIND_KEY_TO_INTEGER_VALUE( "tall", size.h )
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
	else if( !strcmp( key, "command" ) )
	{
		CEventCallback ev;

		if( m_pParent )
		{
			ev = m_pParent->FindEventByName( data );

			if( ev )
				onActivated = ev;
			else
				Con_DPrintf( "KeyValueData: cannot find event named %s", data );
		}
		else
		{
			// should not happen, as parent parses the resource file and sends KeyValueData to every item inside
			// if this happens, there is a bug
			Con_DPrintf( "KeyValueData: no parent on '%s'\n", szName );
		}
	}
	// TODO: nomulti, nosingle, nosteam

	return true;
}

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "PicButton.h"
#include "ItemsHolder.h"
#include "Scissor.h"
#include <string.h>

CMenuItemsHolder::CMenuItemsHolder() :
	CMenuBaseItem(), m_iCursor( 0 ), m_iCursorPrev( 0 ), m_pItems( ), m_numItems( 0 ),
	m_events(), m_numEvents(), m_bInit( false ), m_bAllowEnterActivate( false ),
	m_szResFile( 0 )
{
	;
}

const char *CMenuItemsHolder::Key( int key, int down )
{
	const char *sound = uiSoundNull;

	if( m_numItems )
	{
		CMenuBaseItem *item = ItemAtCursor();
		int cursorPrev;

		if( item && item->IsVisible() && !(item->iFlags & (QMF_GRAYED|QMF_INACTIVE) ) )
		{
			if( key == K_ENTER && !m_bAllowEnterActivate )
			{
				if( !down )
					m_bAllowEnterActivate = true;
				return uiSoundNull;
			}
			else
			{
				sound = item->Key( key, down );

				if( sound ) return sound;
			}
		}

		// system keys are always wait for keys down and never keys up
		if( !down )
			return 0;

		// default handling
		switch( key )
		{
		case K_UPARROW:
		case K_KP_UPARROW:
		case K_LEFTARROW:
		case K_KP_LEFTARROW:
			cursorPrev = m_iCursorPrev = m_iCursor;

			m_iCursor--;
			AdjustCursor( -1 );
			if( cursorPrev != m_iCursor )
			{
				CursorMoved();
				if( !( m_pItems[m_iCursor]->iFlags & QMF_SILENT ) )
					sound = uiSoundMove;

				m_pItems[m_iCursorPrev]->iFlags &= ~QMF_HASKEYBOARDFOCUS;
				m_pItems[m_iCursor]->iFlags |= QMF_HASKEYBOARDFOCUS;
			}
			m_bAllowEnterActivate = true;
			break;
		case K_DOWNARROW:
		case K_KP_DOWNARROW:
		case K_RIGHTARROW:
		case K_KP_RIGHTARROW:
		case K_TAB:
			cursorPrev = m_iCursorPrev = m_iCursor;
			m_iCursor++;
			AdjustCursor( 1 );

			if( cursorPrev != m_iCursor )
			{
				CursorMoved();
				if( !(m_pItems[m_iCursor]->iFlags & QMF_SILENT ) )
					sound = uiSoundMove;

				m_pItems[m_iCursorPrev]->iFlags &= ~QMF_HASKEYBOARDFOCUS;
				m_pItems[m_iCursor]->iFlags |= QMF_HASKEYBOARDFOCUS;
			}
			m_bAllowEnterActivate = true;
			break;
		case K_MOUSE1:
			if( item && (item->iFlags & QMF_HASMOUSEFOCUS) && item->IsVisible() && !(item->iFlags & (QMF_GRAYED|QMF_INACTIVE)))
				return item->Activate();
			break;
		case K_ENTER:
		case K_KP_ENTER:
		case K_AUX1:
		case K_AUX13:
			if( m_bAllowEnterActivate && item && item->IsVisible() && !(item->iFlags & (QMF_GRAYED|QMF_INACTIVE|QMF_MOUSEONLY)) )
				return item->Activate();
			break;
		}
	}

	return sound;
}

void CMenuItemsHolder::Char( int ch )
{
	if( !m_numItems )
		return;

	CMenuBaseItem *item = ItemAtCursor();

	if( item && item->IsVisible() && !(item->iFlags & (QMF_GRAYED|QMF_INACTIVE)) )
		item->Char( ch );
}

const char *CMenuItemsHolder::Activate()
{
	return 0;
}

bool CMenuItemsHolder::MouseMove( int x, int y )
{
	int i;
	// region test the active menu items
	// go in reverse direction, so last items will be first
	for( i = m_numItems - 1; i >= 0; i-- )
	{
		CMenuBaseItem *item = m_pItems[i];

		// Invisible or inactive items will be skipped
		if( !item->IsVisible() || item->iFlags & (QMF_INACTIVE) )
		{
			if( item->iFlags & QMF_HASMOUSEFOCUS )
			{
				if( !UI_CursorInRect( item->m_scPos, item->m_scSize ) )
					item->iFlags &= ~QMF_HASMOUSEFOCUS;
				else item->m_iLastFocusTime = uiStatic.realTime;
			}
			continue;
		}

		// simple region test
		if( !UI_CursorInRect( item->m_scPos, item->m_scSize ) || !item->MouseMove( x, y ) )
		{
			item->m_bPressed = false;
			item->iFlags &= ~QMF_HASMOUSEFOCUS;
			continue;
		}

		if( m_iCursor != i )
		{
			SetCursor( i );
			// reset two focus states, because we are changed cursor
			if( m_iCursorPrev != -1 )
				m_pItems[m_iCursorPrev]->iFlags &= ~(QMF_HASMOUSEFOCUS|QMF_HASKEYBOARDFOCUS);

			if( !( m_pItems[m_iCursor]->iFlags & QMF_SILENT ) )
				EngFuncs::PlayLocalSound( uiSoundMove );
		}

		m_pItems[m_iCursor]->iFlags |= QMF_HASMOUSEFOCUS;
		m_pItems[m_iCursor]->m_iLastFocusTime = uiStatic.realTime;
		// Should we stop at first matched item?
		return true;
	}

	// out of any region
	if( !i )
	{
		m_pItems[m_iCursor]->iFlags &= ~QMF_HASMOUSEFOCUS;
		m_pItems[m_iCursor]->m_bPressed = false;

		// a mouse only item restores focus to the previous item
		if( m_pItems[m_iCursor]->iFlags & QMF_MOUSEONLY )
			if( m_iCursorPrev != -1 )
				m_iCursor = m_iCursorPrev;
	}

	return false;
}

void CMenuItemsHolder::Init()
{
	if( !WasInit() )
	{
		_Init();
		// m_pLayout->Init();

		if( m_szResFile )
			LoadRES( m_szResFile );

		m_bInit = true;
	}
}

void CMenuItemsHolder::VidInit()
{
	CalcPosition();
	CalcSizes();
	_VidInit();

	for( CMenuBaseItem **i = m_pItems; i < m_pItems + m_numItems; i++ )
	{
		(*i)->VidInit();
	}

	// m_pLayout->VidInit();
}

void CMenuItemsHolder::ToggleInactive()
{
	for( int i = 0; i < m_numItems; i++ )
	{
		m_pItems[i]->ToggleInactive();
	}
}

void CMenuItemsHolder::SetInactive( bool inactive )
{
	for( int i = 0; i < m_numItems; i++ )
	{
		m_pItems[i]->SetInactive( inactive );
	}
}

void CMenuItemsHolder::Draw( )
{
	static int statusFadeTime;
	static CMenuBaseItem *lastItem;
	CMenuBaseItem *item;

	const char *statusText;

	// draw contents
	for( int i = 0; i < m_numItems; i++ )
	{
		item = m_pItems[i];

		if( !item->IsVisible() )
			continue;

		item->Draw();
	}

	item = ItemAtCursor();
	// a1ba: maybe this should be somewhere else?
	if( item != lastItem )
	{
		if( item ) item->m_iLastFocusTime = uiStatic.realTime;
		statusFadeTime = uiStatic.realTime;

		lastItem = item;
	}

	// draw status text
	if( item && item == lastItem && ( ( statusText = item->szStatusText ) != NULL ) )
	{
		float alpha = bound( 0, ((( uiStatic.realTime - statusFadeTime ) - 100 ) * 0.01f ), 1 );
		int r, g, b, x, len;

		EngFuncs::ConsoleStringLen( statusText, &len, NULL );

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b, alpha * 255 );
		x = ( ScreenWidth - len ) * 0.5; // centering

		EngFuncs::DrawConsoleString( x, 720 * uiStatic.scaleY, statusText );
	}
	else statusFadeTime = uiStatic.realTime;
}

/*
=================
UI_AdjustCursor

This functiont takes the given menu, the direction, and attempts to
adjust the menu's cursor so that it's at the next available slot
=================
*/
void CMenuItemsHolder::AdjustCursor( int dir )
{
	CMenuBaseItem *item;
	bool wrapped = false;

wrap:
	while( m_iCursor >= 0 && m_iCursor < m_numItems )
	{
		item = m_pItems[m_iCursor];
		if( !item->IsVisible() || item->iFlags & (QMF_INACTIVE|QMF_MOUSEONLY) )
		{
			m_iCursor += dir;
		}
		else break;
	}

	if( dir > 0 )
	{
		if( m_iCursor >= m_numItems )
		{
			if( wrapped )
			{
				m_iCursor = m_iCursorPrev;
				return;
			}
			m_iCursor = 0;
			wrapped = true;
			goto wrap;
		}
	}
	else if( dir < 0 )
	{
		if( m_iCursor < 0 )
		{
			if( wrapped )
			{
				m_iCursor = m_iCursorPrev;
				return;
			}
			m_iCursor = m_numItems - 1;
			wrapped = true;
			goto wrap;
		}
	}
}

CMenuBaseItem *CMenuItemsHolder::ItemAtCursor()
{
	if( m_iCursor < 0 || m_iCursor >= m_numItems )
		return 0;

	// inactive items can't be has focus
	if( m_pItems[m_iCursor]->iFlags & QMF_INACTIVE )
		return 0;

	return m_pItems[m_iCursor];
}

CMenuBaseItem *CMenuItemsHolder::ItemAtCursorPrev()
{
	if( m_iCursorPrev < 0 || m_iCursorPrev >= m_numItems )
		return 0;

	// inactive items can't be has focus
	if( m_pItems[m_iCursorPrev]->iFlags & QMF_INACTIVE )
		return 0;

	return m_pItems[m_iCursorPrev];
}

CMenuBaseItem *CMenuItemsHolder::FindItemByTag(const char *tag)
{
	if( !tag )
		return NULL;

	for( int i = 0; i < m_numItems; i++ )
	{
		if( !strcmp( m_pItems[i]->szTag, tag ) )
			return m_pItems[i];
	}

	return NULL;
}

void CMenuItemsHolder::SetCursorToItem(CMenuBaseItem &item, bool notify )
{
	for( int i = 0; i < m_numItems; i++ )
	{
		if( m_pItems[i] == &item )
		{
			SetCursor( i, notify );
			return;
		}
	}
}

void CMenuItemsHolder::CalcItemsPositions()
{
	for( int i = 0; i < m_numItems; i++ )
	{
		m_pItems[i]->CalcPosition();
	}
}

void CMenuItemsHolder::CalcItemsSizes()
{
	for( int i = 0; i < m_numItems; i++ )
	{
		m_pItems[i]->CalcSizes();
	}
}

void CMenuItemsHolder::SetCursor( int newCursor, bool notify )
{
	if( newCursor < 0 || newCursor >= m_numItems )
		return;

	if( !m_pItems[newCursor]->IsVisible() || (m_pItems[newCursor]->iFlags & (QMF_INACTIVE)) )
		return;

	m_iCursorPrev = m_iCursor;
	m_iCursor = newCursor;

	if( notify )
		CursorMoved();
}

void CMenuItemsHolder::CursorMoved()
{
	CMenuBaseItem *item;

	if( m_iCursor == m_iCursorPrev )
		return;

	if( m_iCursorPrev >= 0 && m_iCursorPrev < m_numItems )
	{
		item = m_pItems[m_iCursorPrev];

		item->_Event( QM_LOSTFOCUS );
	}

	if( m_iCursor >= 0 && m_iCursor < m_numItems )
	{
		item = m_pItems[m_iCursor];

		item->_Event( QM_GOTFOCUS );
	}
}

void CMenuItemsHolder::AddItem(CMenuBaseItem &item)
{
	if( m_numItems >= UI_MAX_MENUITEMS )
		Host_Error( "UI_AddItem: UI_MAX_MENUITEMS limit exceeded\n" );

	m_pItems[m_numItems] = &item;
	item.m_pParent = this; // U OWNED
	item.iFlags &= ~QMF_HASMOUSEFOCUS;

	m_numItems++;

	item.Init();
}

void CMenuItemsHolder::RemoveItem(CMenuBaseItem &item)
{
	for( int i = m_numItems; i >= 0; i-- )
	{
		if( m_pItems[i] == &item )
		{
			item.m_pParent = NULL;

			memmove( m_pItems + i, m_pItems + i + 1, ( m_numItems - i + 1 ) * sizeof( *m_pItems ) );

			m_numItems--;
			break;
		}
	}
}

bool RES_ExpectString( char **data, const char *expect, bool skip = true )
{
	char token[1024];
	char *tmp;

	if( !data || !*data )
		return true;

	tmp = EngFuncs::COM_ParseFile( *data, token );

	if( skip )
		*data = tmp;

	if( !*data )
		return true;

	if( !strncmp( token, expect, 1024 ) )
	{
		*data = tmp; // skip anyway, if expected string was here
		return true;
	}

	return false;
}

inline bool FreeFile( byte *pfile, const bool retval )
{
	EngFuncs::COM_FreeFile( pfile );
	return retval;
}

bool CMenuItemsHolder::LoadRES(const char *filename)
{
	byte *pfile = EngFuncs::COM_LoadFile( filename );
	char *afile = (char*)pfile;
	char token[1024];

	if( !pfile )
		return false;

	afile = EngFuncs::COM_ParseFile( afile, token );

	Con_DPrintf( "Loading res file from %s, name %s\n", filename, token );

	if( !afile )
		return FreeFile( pfile, false );

	if( !RES_ExpectString( &afile, "{" ) )
	{
		Con_DPrintf( "LoadRES: failed to parse, want '{'\n" );
		return FreeFile( pfile, false );
	}

	do
	{
		CMenuBaseItem *item;

		afile = EngFuncs::COM_ParseFile( afile, token );

		if( !afile )
			return FreeFile( pfile, false );

		item = FindItemByTag( token );

		if( !RES_ExpectString( &afile, "{" ))
		{
			Con_DPrintf( "LoadRES: failed to parse, want '{', near %s\n", token );
			return FreeFile( pfile, false );
		}

		if( item )
		{
			do
			{
				char key[1024];
				char value[1024];

				afile = EngFuncs::COM_ParseFile( afile, key );
				if( !afile )
					return FreeFile( pfile, false );

				afile = EngFuncs::COM_ParseFile( afile, value );
				if( !afile )
					return FreeFile( pfile, false );

				item->KeyValueData( key, value );
			}
			while( !RES_ExpectString( &afile, "}", false ) );
		}
		else
		{
			Con_DPrintf( "LoadRES: cannot find item %s, skipping!\n", token );

			while( !RES_ExpectString( &afile, "}" ) );
		}
	}
	while( !RES_ExpectString( &afile, "}", false ) );

	if( !RES_ExpectString( &afile, "}" ))
	{
		Con_DPrintf( "LoadRES: failed to parse, want '{'\n" );
		return FreeFile( pfile, false );
	}

	return FreeFile( pfile, true );
}

void CMenuItemsHolder::RegisterNamedEvent(CEventCallback ev, const char *name)
{
	if( m_numEvents >= UI_MAX_MENUITEMS )
		Host_Error( "RegisterNamedEvent: UI_MAX_MENUITEMS limit exceeded\n" );

	ASSERT( name );
	ASSERT( ev );

	ev.szName = name;
	m_events[m_numItems] = ev;

	m_numItems++;
}

CEventCallback CMenuItemsHolder::FindEventByName(const char *name)
{
	for( int i = 0; i < m_numItems; i++ )
	{
		if( !strcmp( m_events[i].szName, name ))
			return m_events[i];
	}

	return CEventCallback();
}

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "PicButton.h"
#include "ItemsHolder.h"
#include "BaseWindow.h"

CMenuBaseWindow::CMenuBaseWindow(const char *name) : CMenuItemsHolder()
{
	bAllowDrag = false; // UNDONE
	m_bHolding = false;
	bInTransition = false;
	szName = name;
}

void CMenuBaseWindow::Show()
{
	Init();
	VidInit();
	PushMenu();
	m_bAllowEnterActivate = false;
}

void CMenuBaseWindow::Hide()
{
	PopMenu();
}

bool CMenuBaseWindow::IsVisible()
{
	return this == uiStatic.menuStack[uiStatic.menuDepth-1];
}

void CMenuBaseWindow::PushMenu()
{
	int		i;
	CMenuBaseItem	*item;

	// if this menu is already present, drop back to that level to avoid stacking menus by hotkeys
	for( i = 0; i < uiStatic.menuDepth; i++ )
	{
		if( uiStatic.menuStack[i] == this )
		{
			if( IsRoot() )
				uiStatic.menuDepth = i;
			else
			{
				if( i != uiStatic.menuDepth - 1 )
				{
					// swap windows
					uiStatic.menuStack[i] = uiStatic.menuActive;
					uiStatic.menuStack[uiStatic.menuDepth] = this;
				}
			}
			break;
		}
	}

	if( i == uiStatic.menuDepth )
	{
		if( uiStatic.menuDepth >= UI_MAX_MENUDEPTH )
			Host_Error( "UI_PushMenu: menu stack overflow\n" );
		uiStatic.menuStack[uiStatic.menuDepth++] = this;
	}

	if(( uiStatic.prevMenu = uiStatic.menuActive ))
	{
		uiStatic.prevMenu->bInTransition = true;
	}

	uiStatic.menuActive = this;
	bInTransition = true; // enable transitions

	uiStatic.firstDraw = true;
	uiStatic.enterSound = gpGlobals->time + 0.15;	// make some delay
	uiStatic.visible = true;

	EngFuncs::KEY_SetDest ( KEY_MENU );

	m_iCursor = 0;
	m_iCursorPrev = 0;

	// force first available item to have focus
	for( i = 0; i < m_numItems; i++ )
	{
		item = m_pItems[i];

		if( !item->IsVisible() || item->iFlags & (QMF_GRAYED|QMF_INACTIVE|QMF_MOUSEONLY))
			continue;

		m_iCursorPrev = -1;
		SetCursor( i );
		break;
	}
}

void CMenuBaseWindow::PopMenu()
{
	EngFuncs::PlayLocalSound( uiSoundOut );

	uiStatic.menuDepth--;

	if( uiStatic.menuDepth < 0 )
		Host_Error( "UI_PopMenu: menu stack underflow\n" );

	if( uiStatic.menuDepth )
	{
		if(( uiStatic.prevMenu = uiStatic.menuActive ))
			uiStatic.prevMenu->bInTransition = true;

		uiStatic.menuActive = uiStatic.menuStack[uiStatic.menuDepth-1];

		uiStatic.firstDraw = true;
	}
	else if ( CL_IsActive( ))
	{
		UI_CloseMenu();
	}
	else
	{
		// never trying the close menu when client isn't connected
		EngFuncs::KEY_SetDest( KEY_MENU );
		UI_Main_Menu();
	}

	if( uiStatic.m_fDemosPlayed && uiStatic.m_iOldMenuDepth == uiStatic.menuDepth )
	{
		EngFuncs::ClientCmd( FALSE, "demos\n" );
		uiStatic.m_fDemosPlayed = false;
		uiStatic.m_iOldMenuDepth = 0;
	}
}

void CMenuBaseWindow::SaveAndPopMenu( )
{
	EngFuncs::ClientCmd( FALSE, "trysaveconfig\n" );
	Hide();
}

const char *CMenuBaseWindow::Key(int key, int down)
{
	if( key == K_MOUSE1 && bAllowDrag )
	{
		m_bHolding = down;
		m_bHoldOffset.x = uiStatic.cursorX / uiStatic.scaleX;
		m_bHoldOffset.y = uiStatic.cursorY / uiStatic.scaleX;
	}

	if( down && key == K_ESCAPE )
	{
		Hide( );
		return uiSoundOut;
	}

	return CMenuItemsHolder::Key( key, down );
}

void CMenuBaseWindow::Draw()
{
	if( m_bHolding && bAllowDrag )
	{
		pos.x += uiStatic.cursorX / uiStatic.scaleX - m_bHoldOffset.x;
		pos.y += uiStatic.cursorY / uiStatic.scaleX- m_bHoldOffset.y;

		m_bHoldOffset.x = uiStatic.cursorX / uiStatic.scaleX;
		m_bHoldOffset.y = uiStatic.cursorY / uiStatic.scaleX;
		CalcPosition();
		CalcItemsPositions();
	}
	CMenuItemsHolder::Draw();
}


bool CMenuBaseWindow::DrawAnimation(EAnimation anim)
{
	return true;
}

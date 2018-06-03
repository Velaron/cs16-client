/*
Framework.cpp -- base menu fullscreen root window
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
#include "Framework.h"
#include "PicButton.h"

CMenuFramework::CMenuFramework( const char *name ) : BaseClass( name )
{
	memset( m_apBtns, 0, sizeof( m_apBtns ) );
	m_iBtnsNum = 0;
}

CMenuFramework::~CMenuFramework()
{
	for( int i = 0; i < m_iBtnsNum; i++ )
	{
		RemoveItem( *( m_apBtns[i] ) );
		delete m_apBtns[i];
		m_apBtns[i] = NULL;
	}
}

void CMenuFramework::Show()
{
	CMenuPicButton::RootChanged( true );
	BaseClass::Show();

	uiStatic.rootActive = this;
	uiStatic.rootPosition = uiStatic.menuDepth-1;
}

void CMenuFramework::Hide()
{
	int i;
	BaseClass::Hide();

	for( i = uiStatic.menuDepth-1; i >= 0; i-- )
	{
		if( uiStatic.menuStack[i]->IsRoot() )
		{
			uiStatic.rootActive = uiStatic.menuStack[i];
			uiStatic.rootPosition = i;
			CMenuPicButton::RootChanged( false );
			return;
		}
	}


	// looks like we are have a modal or some window over game
	uiStatic.rootActive = NULL;
	uiStatic.rootPosition = 0;
}

void CMenuFramework::Init()
{
	BaseClass::Init();
	pos.x = uiStatic.xOffset;
	pos.y = uiStatic.yOffset;
	size.w = uiStatic.width;
	size.h = 768;
}

void CMenuFramework::VidInit()
{
	pos.x = uiStatic.xOffset;
	pos.y = uiStatic.yOffset;
	size.w = uiStatic.width;
	size.h = 768;
	BaseClass::VidInit();
}

CMenuPicButton * CMenuFramework::AddButton(const char *szName, const char *szStatus, EDefaultBtns buttonPicId, CEventCallback onActivated, int iFlags)
{
	if( m_iBtnsNum >= MAX_FRAMEWORK_PICBUTTONS )
	{
		Host_Error( "Too many pic buttons in framework!" );
		return NULL;
	}

	CMenuPicButton *btn = new CMenuPicButton();

	btn->SetNameAndStatus( szName, szStatus );
	btn->SetPicture( buttonPicId );
	btn->iFlags |= iFlags;
	btn->onActivated = onActivated;
	btn->SetCoord( 72, 230 + m_iBtnsNum * 50 );
	AddItem( btn );

	m_apBtns[m_iBtnsNum++] = btn;

	return btn;
}

CMenuPicButton * CMenuFramework::AddButton(const char *szName, const char *szStatus, const char *szButtonPath, CEventCallback onActivated, int iFlags)
{
	if( m_iBtnsNum >= MAX_FRAMEWORK_PICBUTTONS )
	{
		Host_Error( "Too many pic buttons in framework!" );
		return NULL;
	}

	CMenuPicButton *btn = new CMenuPicButton();

	btn->SetNameAndStatus( szName, szStatus );
	btn->SetPicture( szButtonPath );
	btn->iFlags |= iFlags;
	btn->onActivated = onActivated;
	btn->SetCoord( 72, 230 + m_iBtnsNum * 50 );
	AddItem( btn );

	m_apBtns[m_iBtnsNum++] = btn;

	return btn;
}


bool CMenuFramework::DrawAnimation(EAnimation anim)
{
	return CMenuBaseWindow::DrawAnimation( anim );
}


#include "Framework.h"
#include "PicButton.h"

CMenuFramework::CMenuFramework( const char *name ) : CMenuBaseWindow( name )
{
	SetCoord( 0, 0 );
}

void CMenuFramework::Show()
{
	CMenuPicButton::RootChanged( true );
	CMenuBaseWindow::Show();

	uiStatic.rootActive = this;
	uiStatic.rootPosition = uiStatic.menuDepth-1;
}

void CMenuFramework::Hide()
{
	int i;
	CMenuBaseWindow::Hide();

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

bool CMenuFramework::IsVisible()
{
	return this == uiStatic.rootActive;
}

void CMenuFramework::Init()
{
	CMenuBaseWindow::Init();
	pos.x = pos.y = 0;
	size.w = 1024;
	size.h = 768;
}

void CMenuFramework::VidInit()
{
	CMenuBaseWindow::VidInit();
	m_scPos.x = m_scPos.y = 0;
	m_scSize.w = ScreenWidth;
	m_scSize.h = ScreenHeight;
}

bool CMenuFramework::DrawAnimation(EAnimation anim)
{
	if( anim == ANIM_IN )
		Draw();

	return CMenuPicButton::DrawTitleAnim( anim );
}


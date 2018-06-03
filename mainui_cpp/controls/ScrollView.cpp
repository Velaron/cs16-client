#include "ScrollView.h"
#include "Scissor.h"

CMenuScrollView::CMenuScrollView() : CMenuItemsHolder ()
{
}

void CMenuScrollView::VidInit()
{
	CalcPosition();
	CalcSizes();

	_VidInit();
	VidInitItems();

	m_iMax = 0;
	m_iPos = 0;

	for( int i = 0; i < m_numItems; i++ )
	{
		Point pt = m_pItems[i]->GetRenderPosition();
		Size sz = m_pItems[i]->GetRenderSize();

		if( m_iMax < pt.y + sz.h )
			m_iMax = pt.y + sz.h;
	}

	m_bDisableScrolling = (m_iMax < m_scPos.y + m_scSize.h);
}

const char *CMenuScrollView::Key( int key, int down )
{
	// act when key is pressed or repeated
	if( down )
	{
		if( !m_bDisableScrolling )
		{
			int newPos = m_iPos;
			switch( key )
			{
			case K_MWHEELUP:
			case K_UPARROW:
				newPos -= 20;
				break;
			case K_MWHEELDOWN:
			case K_DOWNARROW:
				newPos += 20;
				break;

			case K_PGUP:
				newPos -= 100;
				break;
			case K_PGDN:
				newPos += 100;
				break;
			case K_MOUSE1:
				// drag & drop
				// scrollbar
				break;
			}
			// TODO: overscrolling
			newPos = bound( 0, newPos, m_iMax - m_scSize.h );


			// recalc
			if( newPos != m_iPos )
			{
				m_iPos = newPos;
				for( int i = 0; i < m_numItems; i++ )
				{
					CMenuBaseItem *pItem = m_pItems[i];

					pItem->VidInit();
				}
				CMenuItemsHolder::MouseMove( uiStatic.cursorX, uiStatic.cursorY );
			}
		}
	}

	return CMenuItemsHolder::Key( key, down );
}

Point CMenuScrollView::GetPositionOffset() const
{
	return Point( 0, -m_iPos );
}

bool CMenuScrollView::MouseMove( int x, int y )
{
	return CMenuItemsHolder::MouseMove( x, y );
}

bool CMenuScrollView::IsRectVisible(Point pt, Size sz)
{
	if( pt.x >= m_scPos.x &&
		pt.y >= m_scPos.y &&
		pt.x <= m_scPos.x + m_scSize.w &&
		pt.y <= m_scPos.y + m_scSize.h )
		return true;
	if( pt.x + sz.w >= m_scPos.x &&
		pt.y + sz.h >= m_scPos.y &&
		pt.x + sz.w <= m_scPos.x + m_scSize.w &&
		pt.y + sz.h <= m_scPos.y + m_scSize.h )
		return true;
	return false;
}

void CMenuScrollView::Draw()
{
	int drawn = 0, skipped = 0;
	for( int i = 0; i < m_numItems; i++ )
	{
		if( !IsRectVisible( m_pItems[i]->GetRenderPosition(), m_pItems[i]->GetRenderSize() ) )
		{
			m_pItems[i]->iFlags |= QMF_HIDDENBYPARENT;
			skipped++;
		}
		else
		{
			m_pItems[i]->iFlags &= ~QMF_HIDDENBYPARENT;
			drawn++;
		}
	}

	Con_NPrintf( 0, "Drawn: %i Skipped: %i", drawn, skipped );

	UI::Scissor::PushScissor( m_scPos, m_scSize );
		CMenuItemsHolder::Draw();
	UI::Scissor::PopScissor();
}

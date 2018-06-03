#ifndef SCROLLVIEW_H
#define SCROLLVIEW_H

#include "ItemsHolder.h"

class CMenuScrollView : public CMenuItemsHolder
{
public:
	CMenuScrollView();

	void VidInit();
	void Draw();
	const char *Key( int key, int down );
	bool MouseMove( int x, int y  );

	Point GetPositionOffset() const;

private:
	bool IsRectVisible( Point pt, Size sz );

	Point m_scScrollBarPos;
	Size  m_scScrollBarSize;
	bool  m_bScrollBarSliding;
	bool  m_bDisableScrolling; // can't actually scroll due to item placement

	int m_iPos;
	int m_iMax;
	// float m_flOverScrolling;
};

#endif // SCROLLVIEW_H

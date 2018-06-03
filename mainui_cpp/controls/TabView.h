/*
TabView.cpp -- tabbed view
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
#ifndef TABWIDGET_H
#define TABWIDGET_H

#include "ItemsHolder.h"

class CMenuTabView : public CMenuItemsHolder
{
public:
	typedef CMenuItemsHolder BaseClass;
	CMenuTabView();

	virtual void VidInit();
	virtual const char *Key( int key, int down );
	virtual void Draw();
	virtual Point GetPositionOffset() const;

	inline void SetTabName( int idx, const char *name )
	{
		if( idx >= 0 && idx < UI_MAX_MENUITEMS )
			m_szTabNames[idx] = name;
	}

	inline void AddTabItem( CMenuBaseItem &item, const char *name )
	{
		AddItem( item );
		SetTabName( m_numItems - 1, name );
	}

private:
	void DrawTab(Point pt, const char *name, bool isEnd , bool isSelected, bool isHighlighted);

	// number of tab names matches m_iNumItems
	const char *m_szTabNames[UI_MAX_MENUITEMS];

	Size m_szTab;
};

#endif // TABWIDGET_H

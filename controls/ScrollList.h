/*
ScrollList.h - animated button with picture
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

#pragma once
#ifndef MENU_SCROLLLIST_H
#define MENU_SCROLLLIST_H

#include "BaseItem.h"

// TODO: This should be rewritten
// because preformatted strings ScrollList for table simulating
// is a TOTALLY BAD IDEA

class CMenuScrollList : public CMenuBaseItem
{
public:
	CMenuScrollList();

	virtual void VidInit( void );
	virtual const char *Key( int key, int down );
	virtual void Draw( void );

	void SetUpArrowPicture( const char *upArrow, const char *upArrowFocus, const char *upArrowPressed )
	{
		szUpArrow = upArrow;
		szUpArrowFocus = upArrowFocus;
		szUpArrowPressed = upArrowPressed;
	}

	void SetDownArrowPicture( const char *downArrow, const char *downArrowFocus, const char *downArrowPressed )
	{
		szDownArrow = downArrow;
		szDownArrowFocus = downArrowFocus;
		szDownArrowPressed = downArrowPressed;
	}

	const char *GetSelectedItem()
	{
		return pszItemNames[iCurItem];
	}


// TODO:
#if 0
	CMenuBaseItem *GetSelectedItem()
	{
		return pszItems[iCurItem];
	}
#endif

	CEventCallback onDeleteEntry;
	CEventCallback onActivateEntry;

	const char	*szBackground;
	const char	*szUpArrow;
	const char	*szUpArrowFocus;
	const char  *szUpArrowPressed;
	const char	*szDownArrow;
	const char	*szDownArrowFocus;
	const char  *szDownArrowPressed;
	const char	**pszItemNames;
#if 0
	CMenuBaseItem *pszItems;
#endif
	int		iNumItems;
	int		iCurItem;
	int		iTopItem;
	int		iNumRows;
// scrollbar stuff // ADAMpos.x
	int		iScrollBarX;
	int		iScrollBarY;
	int		iScrollBarWidth;
	int		iScrollBarHeight;
	int		iScrollBarSliding;
// highlight // mittorn
	int		iHighlight;
	bool    bFramedHintText;
// a1batross // smooth scrolling
	//float flFracScroll;
};

#endif // MENU_SCROLLLIST_H

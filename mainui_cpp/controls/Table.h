/*
Table.h - Table
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
#ifndef MENU_TABLE_H
#define MENU_TABLE_H

#include "BaseItem.h"
#include "BaseModel.h"

#define MAX_TABLE_COLUMNS 16

/*
 * CMenuTable
 *
 * In order to keep simplicity, there is some limitations.
 * If you will keep them in mind, you will find tables simple, easy and fast to add on your window
 *
 * 1. CMenuTable uses a very simple MVC pattern, but there controller is merged with model(OnDelete, OnActivate)
 * It means, that you have to inherit from CMenuBaseModel, implement all pure methods and only then you can put your data on table.
 *
 * 2. CMenuTable will call Update() only when you will pass a model pointer to table.
 *
 * 3. Adding table on items holder before you have passed a model pointer is PROHIBITED and will lead to crash.
 *
 * 4. You can't use columns more than MAX_TABLE_COLUMNS
 *
 * 5. Header text is set per Table instance.
 *
 * 6. Column widths are constant(at this moment). You should not exceed 1.0 in total columns width
 *
 */

class CMenuTable : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuTable();


	virtual const char *Key( int key, int down );
	virtual void Draw();
	virtual void VidInit();

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

	bool MoveView( int delta );
	bool MoveCursor( int delta );

	void SetModel( CMenuBaseModel *model )
	{
		m_pModel = model;
		m_pModel->Update();
	}

	void SetHeaderText( int num, const char *text )
	{
		if( num < MAX_TABLE_COLUMNS && num >= 0 )
			szHeaderTexts[num] = text;
	}

	// widths are in [0.0; 1.0]
	// Total of all widths should be 1.0f, but not necessary.
	// to keep everything simple, if first few columns exceeds 1.0, the other will not be shown
	// if you have set fixed == true, then column size is set in logical units
	void SetColumnWidth( int num, float width, bool fixed = false )
	{
		if( num < MAX_TABLE_COLUMNS && num >= 0 )
		{
			columns[num].flWidth = width;
			columns[num].fStaticWidth = fixed;
		}
	}

	// shortcut for SetHeaderText && SetColumnWidth
	inline void SetupColumn( int num, const char *text, float width, bool fixed = false )
	{
		SetHeaderText( num, text );
		SetColumnWidth( num, width, fixed );
	}

	int GetCurrentIndex() { return iCurItem; }
	void SetCurrentIndex( int idx );

	bool    bFramedHintText;

private:
	void DrawLine(Point p, const char **psz, size_t size, int textColor, bool forceCol, int fillColor = 0);
	void DrawLine(Point p, int line, int textColor, bool forceCol, int fillColor = 0);

	const char	*szHeaderTexts[MAX_TABLE_COLUMNS];
	struct column
	{
		float flWidth;
		bool fStaticWidth;
	} columns[MAX_TABLE_COLUMNS];

	float flFixedSumm, flDynamicSumm;

	const char	*szBackground;
	const char	*szUpArrow;
	const char	*szUpArrowFocus;
	const char  *szUpArrowPressed;
	const char	*szDownArrow;
	const char	*szDownArrowFocus;
	const char  *szDownArrowPressed;

	int		iTopItem;
	int     iNumRows;
// scrollbar stuff // ADAMpos.x
	int		iScrollBarX;
	int		iScrollBarY;
	int		iScrollBarWidth;
	int		iScrollBarHeight;
	bool	iScrollBarSliding;
// highlight // mittorn
	int		iHighlight;
	int		iCurItem;

	int		m_iLastItemMouseChange;

	CMenuBaseModel *m_pModel;
};


#endif // MENU_TABLE_H

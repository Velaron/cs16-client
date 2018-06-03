/*
CheckBox.h - checkbox
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
#ifndef MENU_CHECKBOX_H
#define MENU_CHECKBOX_H

#include "Editable.h"

#define UI_CHECKBOX_EMPTY		"gfx/shell/cb_empty"
#define UI_CHECKBOX_GRAYED		"gfx/shell/cb_disabled"
#define UI_CHECKBOX_FOCUS		"gfx/shell/cb_over"
#define UI_CHECKBOX_PRESSED		"gfx/shell/cb_down"
#define UI_CHECKBOX_ENABLED		"gfx/shell/cb_checked"

class CMenuCheckBox : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	CMenuCheckBox();
	virtual void VidInit();
	virtual const char * Key( int key, int down );
	virtual void Draw( void );
	virtual void UpdateEditable();
	void LinkCvar( const char *name )
	{
		CMenuEditable::LinkCvar( name, CMenuEditable::CVAR_VALUE );
	}

	void SetPicture( const char *empty, const char *focus, const char *press, const char *check, const char *grayed )
	{
		szEmptyPic = empty;
		szFocusPic = focus;
		szPressPic = press;
		szCheckPic = check;
		szGrayedPic = grayed;
	}

	bool		bChecked;
	const char	*szEmptyPic;
	const char	*szFocusPic;
	const char	*szPressPic;
	const char	*szCheckPic;
	const char	*szGrayedPic;	// when QMF_GRAYED is set

	unsigned int iMask; // used only for BitMaskCb
	bool bInvertMask;
	static void BitMaskCb( CMenuBaseItem *pSelf, void *pExtra )
	{
		CMenuCheckBox *self = (CMenuCheckBox*)pSelf;

		if( !self->bInvertMask == self->bChecked )
		{
			*(unsigned int*)pExtra |= self->iMask;
		}
		else
		{
			*(unsigned int*)pExtra &= ~self->iMask;
		}
	}

private:
	Point m_scTextPos;
	Size m_scTextSize;
};

#endif // MENU_CHECKBOX_H

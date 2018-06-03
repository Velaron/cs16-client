/*
Field.h - edit field
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
#ifndef MENU_FIELD_H
#define MENU_FIELD_H

#include "Editable.h"

#define UI_MAX_FIELD_LINE		256

class CMenuField : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	CMenuField();
	virtual void Init( void );
	virtual void VidInit( void );
	virtual const char * Key( int key, int down );
	virtual void Draw( void );
	virtual void Char( int key );
	virtual void UpdateEditable();

	virtual bool KeyValueData(const char *key, const char *data);
	void LinkCvar(const char *name)
	{
		CMenuEditable::LinkCvar( name, CVAR_STRING );
	}

	void Paste();
	void Clear();

	void SetBuffer( const char *buffer )
	{
		Q_strncpy( szBuffer, buffer, UI_MAX_FIELD_LINE );
		iCursor = strlen( szBuffer );
		if( iCursor > iWidthInChars )
			iScroll = iCursor;
		else
			iScroll = 0;
	}

	const char *GetBuffer()
	{
		return szBuffer;
	}

	bool bAllowColorstrings;
	bool bHideInput;
	bool bNumbersOnly;
	const char	*szBackground;
	int		iMaxLength;		// can't be more than UI_MAX_FIELD_LINE

protected:
	virtual void _Event( int ev );
private:
	char	szBuffer[UI_MAX_FIELD_LINE];
	int		iCursor;
	int		iScroll;
	int		iWidthInChars;

};

#endif // MENU_FIELD_H

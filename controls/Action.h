/*
Action.h - simple label with background item
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
#ifndef MENU_ACTION_H
#define MENU_ACTION_H

#include "BaseItem.h"

class CMenuAction : public CMenuBaseItem
{
public:
	CMenuAction();

	virtual void VidInit( void );
	virtual const char * Key( int key, int down );
	virtual void Draw( void );

	void SetBackground( const char *path, unsigned int color = uiColorWhite );
	void SetBackground( unsigned int color );

private:
	bool m_bfillBackground;
	unsigned int m_iBackcolor;
	const char *m_szBackground;
};

#endif // MENU_ACTION_H

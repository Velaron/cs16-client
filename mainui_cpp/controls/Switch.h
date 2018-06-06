/*
Switch.h - simple switches, like Android 4.0+
Copyright (C) 2017 a1batross

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#pragma once
#ifndef SWITCH_H
#define SWITCH_H

#include "Editable.h"

class CMenuSwitch : public CMenuEditable
{
public:
	typedef CMenuEditable BaseClass;

	CMenuSwitch();

	const char *Key(int key, int down);
	void VidInit();
	void Draw();
	void UpdateEditable();
	void LinkCvar( const char *name )
	{
		CMenuEditable::LinkCvar( name, CMenuEditable::CVAR_VALUE );
	}
	void AddSwitch( const char *text );

	int GetState() { return m_iState; }

	bool bMouseToggle;
	bool bKeepToggleWidth;

	CColor iSelectColor;
	CColor iBackgroundColor;
	CColor iFgTextColor;
	CColor iBgTextColor;

	float fTextOffsetX;
	float fTextOffsetY;
private:
	const char *m_szNames[UI_MAX_MENUITEMS];
	int m_iSwitches;

	int m_iState;

	Point m_Points[UI_MAX_MENUITEMS];
	Size m_Sizes[UI_MAX_MENUITEMS];
	Point m_scTextPos;
	Size m_scTextSize;
};


#endif // SWITCH_H

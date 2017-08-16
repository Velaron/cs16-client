/*
SpinControl.h - spin selector
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
#ifndef MENU_SPINCONTROL_H
#define MENU_SPINCONTROL_H

#include "Editable.h"

class CMenuSpinControl : public CMenuEditable
{
public:
	CMenuSpinControl();

	virtual void VidInit( void );
	virtual const char * Key( int key, int down );
	virtual void Draw( void );
	virtual void UpdateEditable();

	void Setup( float minValue, float maxValue, float range );
	void Setup( const char **stringValues, size_t size );

	void SetDisplayPrecision( short precision );

	void SetCurrentValue( const char *stringValue );
	void SetCurrentValue( float curValue );

	float GetCurrentValue( ) { return m_flCurValue; }
	const char *GetCurrentString( ) { return m_stringValues[(int)m_flCurValue]; }

	void ForceDisplayString( const char *display );

private:
	const char *MoveLeft();
	const char *MoveRight();
	void Display();

	const char	*m_szBackground;
	const char	*m_szLeftArrow;
	const char	*m_szRightArrow;
	const char	*m_szLeftArrowFocus;
	const char	*m_szRightArrowFocus;
	float		m_flMinValue;
	float		m_flMaxValue;
	float		m_flCurValue;
	float		m_flRange;

	const char **m_stringValues;
	short m_iFloatPrecision;

	char m_szDisplay[CS_SIZE];
};

#endif // MENU_SPINCONTROL_H

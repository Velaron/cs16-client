#pragma once
#ifndef SWITCH_H
#define SWITCH_H

#include "Editable.h"

class CMenuSwitch : public CMenuEditable
{
public:
	CMenuSwitch();

	const char *Key(int key, int down);
	void VidInit();
	void Draw();
	void UpdateEditable();
	void LinkCvar( const char *name )
	{
		CMenuEditable::LinkCvar( name, CMenuEditable::CVAR_VALUE );
	}

	const char *szLeftName;
	const char *szRightName;
	bool bState;
	bool bMouseToggle;

	int iSelectColor;
	int iBackgroundColor;
	int iFgTextColor;
	int iBgTextColor;

	float fTextOffsetX;
	float fTextOffsetY;
private:
	Point m_rightPoint, m_leftPoint;
	Size m_rightSize, m_leftSize;

	Point m_scTextPos;
	Size m_scTextSize;
};


#endif // SWITCH_H

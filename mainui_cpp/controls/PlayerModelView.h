/*
PlayerModelView.cpp -- player model view
Copyright (C) 2018 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef CMENUPLAYERMODELVIEW_H
#define CMENUPLAYERMODELVIEW_H

// HLSDK includes
#include "mathlib.h"
#include "const.h"
#include "keydefs.h"
#include "ref_params.h"
#include "cl_entity.h"
#include "com_model.h"
#include "entity_types.h"
// HLSDK includes end
#include "BaseItem.h"

class CMenuPlayerModelView : public CMenuBaseItem
{
public:
	CMenuPlayerModelView();
	virtual void VidInit();
	virtual void Draw();
	virtual const char *Key(int key, int down);
	void CalcFov();

	HIMAGE hPlayerImage;

	ref_menu_params_t refdef;
	cl_entity_t *ent;
private:
	bool mouseYawControl;

	int prevCursorX, prevCursorY;
};

#endif // CMENUPLAYERMODELVIEW_H

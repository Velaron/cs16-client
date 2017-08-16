/*
Bitmap.h - bitmap menu item
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
#ifndef MENU_BITMAP_H
#define MENU_BITMAP_H

#include "BaseItem.h"

class CMenuBitmap : public CMenuBaseItem
{
public:
	CMenuBitmap();

	virtual void VidInit( void );
	virtual const char * Key( int key, int down );
	virtual void Draw( void );
	void SetPicture( const char *pic, const char *focusPic = NULL, const char *pressPic = NULL)
	{
		szPic = pic;
		szFocusPic = focusPic;
		szPressPic = pressPic;
	}

	bool bDrawAdditive; // enable additive for this bitmap
	const char *szPic;
	const char *szFocusPic;
	const char *szPressPic;
};

// menu banners used fiexed rectangle (virtual screenspace at 640x480)
#define UI_BANNER_POSX		72
#define UI_BANNER_POSY		72
#define UI_BANNER_WIDTH		736
#define UI_BANNER_HEIGHT		128

class CMenuBannerBitmap : public CMenuBitmap
{
public:
	CMenuBannerBitmap() : CMenuBitmap()
	{
		iFlags = QMF_INACTIVE;
		SetRect( UI_BANNER_POSX, UI_BANNER_POSY, UI_BANNER_WIDTH, UI_BANNER_HEIGHT );
		bDrawAdditive = true;
	}
	void Draw( void );
	void VidInit( void );
	void SetInactive(bool) { }
	void ToggleInactive() { }
};


#endif // MENU_BITMAP_H

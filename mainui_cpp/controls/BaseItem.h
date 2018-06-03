/*
BaseItem.h -- base menu item
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
#ifndef BASEITEM_H
#define BASEITEM_H

#include "BaseMenu.h"
#include "Color.h"

class CMenuItemsHolder;
class CMenuBaseItem
{
public:
	friend class CMenuItemsHolder;

	// The main constructor
	CMenuBaseItem();
	virtual ~CMenuBaseItem();

	// Init is called when Item is added to Framework
	// Called once by Framework
	virtual void Init( void );

	// VidInit is called after VidInit method of Framework
	// VidInit can be called multiple times
	virtual void VidInit( void );

	// Reload is called after VidInit item method
	// should be used for reloading internal data, like cvar values
	virtual void Reload( void );

	// Key is called every key press
	// Must return sound name, or NULL
	virtual const char *Key( int key, int down );

	// Draw is called every frame
	virtual void Draw( void );

	// Char is a special key press event for text input
	virtual void Char( int key );

	// Called every mouse movement got from engine.
	// Should return true, if
	virtual bool MouseMove( int x, int y ) { return true; }

	// Called when UI is shown, for example, in pause during play
	virtual const char *Activate( void );

	// Toggle inactivity of item
	virtual void ToggleInactive( void )
	{
		iFlags ^= QMF_INACTIVE;
	}

	// Direct inacivity set
	virtual void SetInactive( bool visible )
	{
		if( visible ) iFlags |= QMF_INACTIVE;
		else iFlags &= ~QMF_INACTIVE;
	}

	// Cause item to be shown.
	// Simple items will be drawn
	// Window will be added to current window stack
	virtual void Show() { iFlags &= ~QMF_HIDDEN; }

	// Cause item to be hidden
	// Simple item will be hidden
	// Window will be removed from current window stack
	virtual void Hide() { iFlags |= QMF_HIDDEN;  }

	// Determine, is this item is visible
	virtual bool IsVisible() const { return !(iFlags & QMF_HIDDEN); }

	// Key value data reading, both parameters are zero-terminated string
	virtual bool KeyValueData( const char *key, const char *data );

	// Toggle visibiltiy.
	inline void ToggleVisibility()
	{
		if( IsVisible() ) Hide();
		else Show();
	}

	// Direct visibility set
	inline void SetVisibility( const bool show )
	{
		if( show ) Show();
		else Hide();
	}

	inline void SetGrayed( const bool grayed )
	{
		if( grayed ) iFlags |= QMF_GRAYED;
		else iFlags &= ~(QMF_GRAYED);
	}

	inline void ToggleGrayed( )
	{
		iFlags ^= QMF_GRAYED;
	}

	// Checks item is current selected in parent Framework
	bool IsCurrentSelected( void ) const;

	// Calculate render positions based on relative positions.
	void CalcPosition( void );

	// Calculate scale size(item size, char size)
	void CalcSizes( void );


	CEventCallback onGotFocus;
	CEventCallback onLostFocus;
	CEventCallback onActivated;
	CEventCallback onChanged;
	CEventCallback onPressed;

	// called when CL_IsActive returns true, otherwise onActivate
	CEventCallback onActivatedClActive;

	inline void SetCoord( int x, int y )                { pos.x = x; pos.y = y; }
	inline void SetSize( int w, int h )                 { size.w = w; size.h = h; }
	inline void SetRect( int x, int y, int w, int h )   { SetCoord( x, y ); SetSize( w, h ); }
	// inline void SetCharSize( int w, int h )             { charSize.w = w; charSize.h = h; }
	inline Point GetRenderPosition() const { return m_scPos; }
	inline Size  GetRenderSize()     const { return m_scSize; }

	void SetCharSize( EFontSizes fs );

	inline void SetNameAndStatus( const char *name, const char *status, const char *tag = NULL )
	{
		szName = name;
		szStatusText = status;
		if( tag ) szTag = tag;
	}

	CMenuItemsHolder* Parent() const			{ return m_pParent; }
	template <class T> T* Parent() const	{ return (T*) m_pParent; } // a shortcut to parent
	bool IsPressed() const { return m_bPressed; }
	int LastFocusTime() const { return m_iLastFocusTime; }

	unsigned int iFlags;

	Point pos;
	Size size;
	Size charSize;

	const char *szName;
	const char *szStatusText;
	const char *szTag; // tag for searching in res file

	CColor iColor;
	CColor iFocusColor;

	ETextAlignment eTextAlignment;
	EFocusAnimation eFocusAnimation;
	ELetterCase eLetterCase;

	HFont font;

protected:
	// calls specific EventCallback
	virtual void _Event( int ev );

	// Determine, is this item is absolute positioned
	// If false, it will be positiond relative to it's parent
	virtual bool IsAbsolutePositioned( void ) const { return false; }

	friend int UI_VidInit( void );

	CMenuItemsHolder	*m_pParent;
	bool	m_bPressed;
	int		m_iLastFocusTime;

	bool	m_bAllocName;

	Point m_scPos;
	Size m_scSize;
	Size m_scChSize;
};

#include "ItemsHolder.h"

#endif // BASEITEM_H

/*
ItemsHolder.h -- an item that can contain and operate with other items
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
#ifndef EMBEDITEM_H
#define EMBEDITEM_H

#include "BaseItem.h"

class CMenuItemsHolder : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuItemsHolder();

	// Overload _Init, _VidInit instead of these methods
	void Init();
	void VidInit();

	virtual void Reload();
	virtual const char *Key( int key, int down );
	virtual void Char( int key );
	virtual const char *Activate( void );
	virtual void ToggleInactive( void );
	virtual void SetInactive( bool visible );
	virtual void Draw( void );

	virtual bool MouseMove( int x, int y );
	virtual bool IsWindow() { return false; }

	virtual bool KeyValueData(const char *key, const char *data);

	// returns a position where actual items holder is located
	virtual Point GetPositionOffset() const;

	void CursorMoved( void );
	void SetCursor( int newCursor, bool notify = true );
	void SetCursorToItem( CMenuBaseItem &item, bool notify = true );
	bool AdjustCursor( int dir );

	void AddItem( CMenuBaseItem &item );
	void RemoveItem( CMenuBaseItem &item );
	CMenuBaseItem *ItemAtCursor( void );
	CMenuBaseItem *ItemAtCursorPrev( void );
	CMenuBaseItem *FindItemByTag( const char *tag );
	inline CMenuBaseItem *GetItemByIndex( int idx )
	{
		if( idx >= 0 && idx < UI_MAX_MENUITEMS )
			return m_pItems[idx];
		return NULL;
	}
	inline int GetItemCount() { return m_numItems; }

	void CalcItemsPositions();
	void CalcItemsSizes();


	inline void AddItem( CMenuBaseItem *item ) { AddItem( *item ); }
	inline int GetCursor() const { return m_iCursor; }
	inline int GetCursorPrev() const { return m_iCursorPrev; }
	inline int ItemCount() const { return m_numItems; }
	inline bool WasInit() const { return m_bInit; }

	void SetResourceFilename( const char *filename ) { m_szResFile = filename; }

	void RegisterNamedEvent( CEventCallback ev, const char *name );
	CEventCallback FindEventByName( const char *name );

protected:
	virtual void _Init() {}
	virtual void _VidInit() {}
	void VidInitItems();

	bool LoadRES( const char *filename );

	int m_iCursor;
	int m_iCursorPrev;

	CMenuBaseItem *m_pItems[UI_MAX_MENUITEMS];
	int m_numItems;

	// it's unnecessary to register here, it's only for searching events by res file
	CEventCallback m_events[UI_MAX_MENUITEMS];
	int m_numEvents;

	bool m_bInit;
	bool m_bWrapCursor;

	const char *m_szResFile;
};

#endif // EMBEDITEM_H

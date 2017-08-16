/*
Editable.h - generic item for editables
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
#ifndef MENU_EDITABLE_H
#define MENU_EDITABLE_H

#include "BaseItem.h"

class CMenuEditable : public CMenuBaseItem
{
public:
	CMenuEditable() : CMenuBaseItem(),
		m_szCvarName(), m_eType(), m_szString(), m_szOriginalString(), m_flValue(), m_flOriginalValue()
	{
	}

	enum cvarType_e { CVAR_STRING = 0, CVAR_VALUE };
	virtual void UpdateEditable() = 0;
	virtual void LinkCvar( const char *name )
	{
		assert(("Derivative class does not implement LinkCvar(const char*) method. You need to specify types."));
	}

	const char *CvarName()   { return m_szCvarName; }
	float       CvarValue()  { return m_flValue; }
	const char *CvarString() { return m_szString; }
	cvarType_e  CvarType()   { return m_eType; }

	void SetCvarValue( float value )
	{
		m_flValue = value;

		if( onCvarChange ) onCvarChange( this );
	}
	void SetCvarString( const char *string )
	{
		if( string != m_szString )
		{
			strncpy( m_szString, string, CS_SIZE );
			m_szString[CS_SIZE-1] = 0;
		}

		if( onCvarChange ) onCvarChange( this );
	}

	void ResetCvar()
	{
		switch( m_eType )
		{
		case CVAR_STRING: SetCvarString( m_szOriginalString ); break;
		case CVAR_VALUE: SetCvarValue( m_flOriginalValue ); break;
		}
	}
	void UpdateCvar()
	{
		if( onCvarGet ) onCvarGet( this );
		else
		{
			switch( m_eType )
			{
			case CVAR_STRING:
				strncpy( m_szString, EngFuncs::GetCvarString( m_szCvarName ), CS_SIZE );
				strncpy( m_szOriginalString, m_szString, CS_SIZE );
				m_szOriginalString[CS_SIZE-1] = 0;

				SetCvarString( m_szOriginalString );
				break;
			case CVAR_VALUE:
				m_flOriginalValue = EngFuncs::GetCvarFloat( m_szCvarName );

				SetCvarValue( m_flOriginalValue );
				break;
			}
		}

		UpdateEditable();
	}

	void LinkCvar( const char *name, cvarType_e type )
	{
		m_szCvarName = name;
		m_eType = type;

		UpdateCvar();
	}

	void WriteCvar()
	{
		if( onCvarWrite ) onCvarWrite( this );
		else
		{
			switch( m_eType )
			{
			case CVAR_STRING: EngFuncs::CvarSetString( m_szCvarName, m_szString ); break;
			case CVAR_VALUE: EngFuncs::CvarSetValue( m_szCvarName, m_flValue ); break;
			}
		}
	}

	void DiscardChanges()
	{
		switch( m_eType )
		{
		case CVAR_STRING: EngFuncs::CvarSetString( m_szCvarName, m_szOriginalString ); break;
		case CVAR_VALUE: EngFuncs::CvarSetValue( m_szCvarName, m_flOriginalValue ); break;
		}
	}

	CEventCallback onCvarWrite;  // called on final writing of cvar value(except DiscardChanges)
	CEventCallback onCvarChange; // called on internal values changes
	CEventCallback onCvarGet;    // called on any cvar update

	DECLARE_EVENT_TO_ITEM_METHOD( CMenuEditable, WriteCvar )
	DECLARE_EVENT_TO_ITEM_METHOD( CMenuEditable, DiscardChanges )
	DECLARE_EVENT_TO_ITEM_METHOD( CMenuEditable, ResetCvar )
	DECLARE_EVENT_TO_ITEM_METHOD( CMenuEditable, UpdateCvar )
protected:
	const char *m_szCvarName;
	cvarType_e  m_eType;

	char		m_szString[CS_SIZE], m_szOriginalString[CS_SIZE];
	float		m_flValue, m_flOriginalValue;
};

#endif // MENU_EDITABLE_H

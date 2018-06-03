/*
Editable.cpp - generic item for editables
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

#include "Editable.h"

CMenuEditable::CMenuEditable() : BaseClass(),
	m_szCvarName(), m_eType(), m_bForceUpdate( false ),
	m_szString(), m_szOriginalString(),
	m_flValue(), m_flOriginalValue()
{

}

void CMenuEditable::LinkCvar(const char *name)
{
	assert(("Derivative class does not implement LinkCvar(const char*) method. You need to specify types."));
}

void CMenuEditable::LinkCvar(const char *name, cvarType_e type)
{
	m_szCvarName = name;
	m_eType = type;

	m_bForceUpdate = true;

		UpdateCvar();

	m_bForceUpdate = false;
}

void CMenuEditable::Reload()
{
	// editable already initialized, so update
	if( m_szCvarName )
		UpdateCvar();
}

void CMenuEditable::SetCvarValue( float value )
{
	m_flValue = value;

	if( onCvarChange ) onCvarChange( this );
}

void CMenuEditable::SetCvarString(const char *string)
{
	if( string != m_szString )
	{
		Q_strncpy( m_szString, string, CS_SIZE );
		m_szString[CS_SIZE-1] = 0;
	}

	if( onCvarChange ) onCvarChange( this );
}

void CMenuEditable::SetOriginalString( const char *psz )
{
	Q_strncpy( m_szString, psz, CS_SIZE );
	Q_strncpy( m_szOriginalString, m_szString, CS_SIZE );
	m_szOriginalString[CS_SIZE-1] = 0;

	SetCvarString( m_szOriginalString );
}

void CMenuEditable::SetOriginalValue( float val )
{
	m_flValue =	m_flOriginalValue = val;

	SetCvarValue( m_flOriginalValue );
}

void CMenuEditable::UpdateCvar()
{
	bool haveUpdate = m_bForceUpdate;

	if( onCvarGet )
	{
		onCvarGet( this );
		haveUpdate = true; // FIXME
	}
	else
	{
		switch( m_eType )
		{
		case CVAR_STRING:
		{
			const char *str = EngFuncs::GetCvarString( m_szCvarName );
			if( m_bForceUpdate || strcmp( m_szOriginalString, str ) )
			{
				SetOriginalString( str );
				haveUpdate = true;
			}
			break;
		}
		case CVAR_VALUE:
		{
			float val = EngFuncs::GetCvarFloat( m_szCvarName );
			if( m_bForceUpdate || m_flOriginalValue != val )
			{
				SetOriginalValue( val );
				haveUpdate = true;
			}
			break;
		}
		}
	}

	if( haveUpdate )
		UpdateEditable();
}

void CMenuEditable::ResetCvar()
{
	switch( m_eType )
	{
	case CVAR_STRING: SetCvarString( m_szOriginalString ); break;
	case CVAR_VALUE: SetCvarValue( m_flOriginalValue ); break;
	}
}

void CMenuEditable::DiscardChanges()
{
	ResetCvar();
	WriteCvar();
}

void CMenuEditable::WriteCvar()
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


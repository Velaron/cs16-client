// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include <list>
#include <string.h>

#include "wrect.h"
#include "cl_dll.h"
#include "cdll_int.h"
#include "cvardef.h"
#include "cl_util.h"

#include "cvar_checker.h"

#ifdef __ANDROID__
#define FORCE_DEFAULTS_CVAR "cl_android_force_defaults"
#else
#define FORCE_DEFAULTS_CVAR "cl_force_defaults"
#endif

struct cvar_check_t
{
	cvar_t *pCVar;
	enum
	{
		CVARCHECK_SET_FLOAT,
		CVARCHECK_SET_STRING
	} eCheckType;
	union
	{
		float flValue;
		const char *szString;
	};
};

class CCVarCheckerPrivate
{
public:
	cvar_t *force_defaults;
	std::list<cvar_check_t> checkList;
};

CCVarChecker::CCVarChecker( )
{
	p = new CCVarCheckerPrivate;
}

CCVarChecker::~CCVarChecker( )
{
	delete p;
}

void CCVarChecker::Init( void )
{
	p->force_defaults = CVAR_CREATE( FORCE_DEFAULTS_CVAR, "1", FCVAR_ARCHIVE );
}

void CCVarChecker::Run( float flTime )
{
	static float nextCheck = flTime + 1.0f;

	if( !p->force_defaults->value || nextCheck < flTime )
		return;

	nextCheck = flTime + 1.0f;

	for( auto it = p->checkList.begin(); it != p->checkList.end(); it++ )
	{
		cvar_check_t &i = *it;

		switch( i.eCheckType )
		{
		case cvar_check_t::CVARCHECK_SET_FLOAT:
			if( i.pCVar->value == i.flValue )
				continue;

			gEngfuncs.Cvar_SetValue( i.pCVar->name, i.flValue );
			break;
		case cvar_check_t::CVARCHECK_SET_STRING:
			if( !strcmp( i.pCVar->string, i.szString ) )
				continue;

			gEngfuncs.Cvar_Set( i.pCVar->name, i.szString );

			break;
		}

		gEngfuncs.Con_Printf( "CVarChecker: %s is forced to %s. "
			"Set " FORCE_DEFAULTS_CVAR " to 0, if you want to disable this behaviour.\n",
			i.pCVar->name, i.pCVar->string );
	}
}

void CCVarChecker::AddToCheckList(const char *szName, const char *szValue)
{
	AddToCheckList( gEngfuncs.pfnGetCvarPointer( szName ), szValue );
}

void CCVarChecker::AddToCheckList(const char *szName, float flValue )
{
	AddToCheckList( gEngfuncs.pfnGetCvarPointer( szName ), flValue );
}

void CCVarChecker::AddToCheckList(cvar_t *pCVar, const char *szValue)
{
	cvar_check_t check;

	check.pCVar = pCVar;
	check.eCheckType = cvar_check_t::CVARCHECK_SET_STRING;
	check.szString = szValue;

	p->checkList.push_back( check );
}

void CCVarChecker::AddToCheckList(cvar_t *pCVar, float flValue)
{
	cvar_check_t check;

	check.pCVar = pCVar;
	check.eCheckType = cvar_check_t::CVARCHECK_SET_FLOAT;
	check.flValue = flValue;

	p->checkList.push_back( check );
}


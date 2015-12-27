/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "stdafx.h"
#include "cbase.h"
#include "mapinfo.h"

LINK_ENTITY_TO_CLASS(info_map_parameters, CMapInfo);

void CMapInfo::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "buying"))
	{
		m_iBuyingStatus = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bombradius"))
	{
		m_flBombRadius = atoi(pkvd->szValue);

		if (m_flBombRadius > 2048)
			m_flBombRadius = 2048;

		pkvd->fHandled = TRUE;
	}
}

void CMapInfo::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
}
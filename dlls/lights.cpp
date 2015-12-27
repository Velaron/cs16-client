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

class CLight : public CPointEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Restart(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	int m_iStyle;
	int m_iszPattern;
	BOOL m_bStartOff;
};

LINK_ENTITY_TO_CLASS(light, CLight);

TYPEDESCRIPTION CLight::m_SaveData[] =
{
	DEFINE_FIELD(CLight, m_iStyle, FIELD_INTEGER),
	DEFINE_FIELD(CLight, m_iszPattern, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CLight, CPointEntity);

void CLight::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pev->angles.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pattern"))
	{
		m_iszPattern = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CLight::Spawn(void)
{
	if (FStringNull(pev->targetname))
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
		m_bStartOff = TRUE;
	else
		m_bStartOff = FALSE;

	if (m_iStyle >= 32)
	{
		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
			LIGHT_STYLE(m_iStyle, "a");
		else if (m_iszPattern)
			LIGHT_STYLE(m_iStyle, (char *)STRING(m_iszPattern));
		else
			LIGHT_STYLE(m_iStyle, "m");
	}
}

void CLight::Restart(void)
{
	if (m_iStyle >= 32)
	{
		if (m_bStartOff)
		{
			SetBits(pev->spawnflags, SF_LIGHT_START_OFF);
			LIGHT_STYLE(m_iStyle, "a");
		}
		else
		{
			ClearBits(pev->spawnflags, SF_LIGHT_START_OFF);

			if (m_iszPattern)
				LIGHT_STYLE(m_iStyle, (char *)STRING(m_iszPattern));
			else
				LIGHT_STYLE(m_iStyle, "m");
		}
	}
}

void CLight::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_iStyle >= 32)
	{
		if (!ShouldToggle(useType, !FBitSet(pev->spawnflags, SF_LIGHT_START_OFF)))
			return;

		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
		{
			if (m_iszPattern)
				LIGHT_STYLE(m_iStyle, (char *)STRING(m_iszPattern));
			else
				LIGHT_STYLE(m_iStyle, "m");

			ClearBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}
		else
		{
			LIGHT_STYLE(m_iStyle, "a");
			SetBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}
	}
}

LINK_ENTITY_TO_CLASS(light_spot, CLight);

class CEnvLight : public CLight
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(light_environment, CEnvLight);

void CEnvLight::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "_light"))
	{
		int r, g, b, v, j;
		j = sscanf(pkvd->szValue, "%d %d %d %d\n", &r, &g, &b, &v);

		if (j == 1)
		{
			g = b = r;
		}
		else if (j == 4)
		{
			r = r * (v / 255.0);
			g = g * (v / 255.0);
			b = b * (v / 255.0);
		}

		r = (int)(powf(r / 114.0, 0.6) * 264);
		g = (int)(powf(g / 114.0, 0.6) * 264);
		b = (int)(powf(b / 114.0, 0.6) * 264);
		pkvd->fHandled = TRUE;

		char szColor[64];
		sprintf(szColor, "%d", r);
		CVAR_SET_STRING("sv_skycolor_r", szColor);
		sprintf(szColor, "%d", g);
		CVAR_SET_STRING("sv_skycolor_g", szColor);
		sprintf(szColor, "%d", b);
		CVAR_SET_STRING("sv_skycolor_b", szColor);
	}
	else
		CLight::KeyValue(pkvd);
}

void CEnvLight::Spawn(void)
{
	char szVector[64];
	UTIL_MakeAimVectors(pev->angles);

	sprintf(szVector, "%f", gpGlobals->v_forward.x);
	CVAR_SET_STRING("sv_skyvec_x", szVector);
	sprintf(szVector, "%f", gpGlobals->v_forward.y);
	CVAR_SET_STRING("sv_skyvec_y", szVector);
	sprintf(szVector, "%f", gpGlobals->v_forward.z);
	CVAR_SET_STRING("sv_skyvec_z", szVector);
	CLight::Spawn();
}
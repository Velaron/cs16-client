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
#include "saverestore.h"
#include "weapons.h"
#include "decals.h"
#include "soundent.h"

class CFuncMortarField : public CBaseToggle
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	int ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT FieldUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_iszXController;
	int m_iszYController;
	float m_flSpread;
	//float m_flDelay; //Already exists in CBaseDelay
	int m_iCount;
	int m_fControl;
};

LINK_ENTITY_TO_CLASS(func_mortar_field, CFuncMortarField);

TYPEDESCRIPTION CFuncMortarField::m_SaveData[] =
{
	DEFINE_FIELD(CFuncMortarField, m_iszXController, FIELD_STRING),
	DEFINE_FIELD(CFuncMortarField, m_iszYController, FIELD_STRING),
	DEFINE_FIELD(CFuncMortarField, m_flSpread, FIELD_FLOAT),
	DEFINE_FIELD(CFuncMortarField, m_flDelay, FIELD_FLOAT),
	DEFINE_FIELD(CFuncMortarField, m_iCount, FIELD_INTEGER),
	DEFINE_FIELD(CFuncMortarField, m_fControl, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CFuncMortarField, CBaseToggle);

void CFuncMortarField::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszXController"))
	{
		m_iszXController = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszYController"))
	{
		m_iszYController = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_flSpread"))
	{
		m_flSpread = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fControl"))
	{
		m_fControl = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iCount"))
	{
		m_iCount = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
}

void CFuncMortarField::Spawn(void)
{
	pev->solid = SOLID_NOT;
	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->movetype = MOVETYPE_NONE;
	SetBits(pev->effects, EF_NODRAW);
	SetUse(&CFuncMortarField::FieldUse);
	Precache();
}

void CFuncMortarField::Precache(void)
{
	PRECACHE_SOUND("weapons/mortar.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");
	PRECACHE_MODEL("sprites/lgtning.spr");
}

void CFuncMortarField::FieldUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Vector vecStart;
	vecStart.x = RANDOM_FLOAT(pev->mins.x, pev->maxs.x);
	vecStart.y = RANDOM_FLOAT(pev->mins.y, pev->maxs.y);
	vecStart.z = pev->maxs.z;

	switch (m_fControl)
	{
		case 0: break;
		case 1:
		{
			if (pActivator != NULL)
			{
				vecStart.x = pActivator->pev->origin.x;
				vecStart.y = pActivator->pev->origin.y;
			}

			break;
		}

		case 2:
		{
			CBaseEntity *pController;

			if (!FStringNull(m_iszXController))
			{
				pController = UTIL_FindEntityByTargetname(NULL, STRING(m_iszXController));

				if (pController)
					vecStart.x = pev->mins.x + pController->pev->ideal_yaw * (pev->size.x);
			}

			if (!FStringNull(m_iszYController))
			{
				pController = UTIL_FindEntityByTargetname(NULL, STRING(m_iszYController));

				if (pController)
					vecStart.y = pev->mins.y + pController->pev->ideal_yaw * (pev->size.y);
			}

			break;
		}
	}

	int pitch = RANDOM_LONG(95, 124);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/mortar.wav", VOL_NORM, ATTN_NONE, 0, pitch);
	float t = 2.5;

	for (int i = 0; i < m_iCount; i++)
	{
		Vector vecSpot = vecStart;
		vecSpot.x += RANDOM_FLOAT(-m_flSpread, m_flSpread);
		vecSpot.y += RANDOM_FLOAT(-m_flSpread, m_flSpread);

		TraceResult tr;
		UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -1) * 4096, ignore_monsters, ENT(pev), &tr);
		edict_t *pentOwner = NULL;

		if (pActivator)
			pentOwner = pActivator->edict();

		CBaseEntity *pMortar = Create("monster_mortar", tr.vecEndPos, Vector(0, 0, 0), pentOwner);
		pMortar->pev->nextthink = gpGlobals->time + t;
		t += RANDOM_FLOAT(0.2, 0.5);

		if (!i)
			CSoundEnt::InsertSound(bits_SOUND_DANGER, tr.vecEndPos, 400, 0.3);
	}
}

class CMortar : public CGrenade
{
public:
	void Spawn(void);
	void Precache(void);
	void EXPORT MortarExplode(void);

public:
	int m_spriteTexture;
};

LINK_ENTITY_TO_CLASS(monster_mortar, CMortar);

void CMortar::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->dmg = 200;

	SetThink(&CMortar::MortarExplode);
	pev->nextthink = 0;

	Precache();
}

void CMortar::Precache(void)
{
	m_spriteTexture = PRECACHE_MODEL("sprites/lgtning.spr");
}

void CMortar::MortarExplode(void)
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 1024);
	WRITE_SHORT(m_spriteTexture);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(1);
	WRITE_BYTE(40);
	WRITE_BYTE(0);
	WRITE_BYTE(255);
	WRITE_BYTE(160);
	WRITE_BYTE(100);
	WRITE_BYTE(128);
	WRITE_BYTE(0);
	MESSAGE_END();

	TraceResult tr;
	UTIL_TraceLine(pev->origin + Vector(0, 0, 1024), pev->origin - Vector(0, 0, 1024), dont_ignore_monsters, ENT(pev), &tr);
	Explode(&tr, DMG_BLAST | DMG_MORTAR);
	UTIL_ScreenShake(tr.vecEndPos, 25, 150, 1, 750);
}
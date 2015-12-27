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
#include "skill.h"
#include "gamerules.h"

class CRecharge : public CBaseToggle
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return (CBaseToggle::ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT Off(void);
	void EXPORT Recharge(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_flNextCharge;
	int m_iReactivate;
	int m_iJuice;
	int m_iOn;
	float m_flSoundTime;
};

TYPEDESCRIPTION CRecharge::m_SaveData[] =
{
	DEFINE_FIELD(CRecharge, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD(CRecharge, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD(CRecharge, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD(CRecharge, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD(CRecharge, m_flSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CRecharge, CBaseEntity);
LINK_ENTITY_TO_CLASS(func_recharge, CRecharge);

void CRecharge::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style") || FStrEq(pkvd->szKeyName, "height") || FStrEq(pkvd->szKeyName, "value1") || FStrEq(pkvd->szKeyName, "value2") || FStrEq(pkvd->szKeyName, "value3"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

void CRecharge::Spawn(void)
{
	Precache();

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model));

	m_iJuice = (int)gSkillData.suitchargerCapacity;
	pev->frame = 0;
}

void CRecharge::Precache(void)
{
	PRECACHE_SOUND("items/suitcharge1.wav");
	PRECACHE_SOUND("items/suitchargeno1.wav");
	PRECACHE_SOUND("items/suitchargeok1.wav");
}

void CRecharge::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!FClassnameIs(pActivator->pev, "player"))
		return;

	if (m_iJuice <= 0)
	{
		pev->frame = 1;
		Off();
	}

	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1 << WEAPON_SUIT))))
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/suitchargeno1.wav", 0.85, ATTN_NORM);
		}

		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink(&CRecharge::Off);

	if (m_flNextCharge >= gpGlobals->time)
		return;

	if (!pActivator)
		return;

	m_hActivator = pActivator;

	if (!m_hActivator->IsPlayer())
		return;

	if (!m_iOn)
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/suitchargeok1.wav", 0.85, ATTN_NORM);
		m_flSoundTime = 0.56 + gpGlobals->time;
	}

	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/suitcharge1.wav", 0.85, ATTN_NORM);
	}

	if (m_hActivator->pev->armorvalue < 100)
	{
		m_iJuice--;
		m_hActivator->pev->armorvalue += 1;

		if (m_hActivator->pev->armorvalue > 100)
			m_hActivator->pev->armorvalue = 100;
	}

	m_flNextCharge = gpGlobals->time + 0.1;
}

void CRecharge::Recharge(void)
{
	m_iJuice = (int)gSkillData.suitchargerCapacity;
	pev->frame = 0;
	SetThink(&CBaseEntity::SUB_DoNothing);
}

void CRecharge::Off(void)
{
	if (m_iOn > 1)
		STOP_SOUND(ENT(pev), CHAN_STATIC, "items/suitcharge1.wav");

	m_iOn = 0;

	if ((!m_iJuice) && ((m_iReactivate = (int)g_pGameRules->FlHEVChargerRechargeTime()) > 0))
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(&CRecharge::Recharge);
	}
	else
		SetThink(&CBaseEntity::SUB_DoNothing);
}
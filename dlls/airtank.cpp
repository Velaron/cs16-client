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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

class CAirtank : public CGrenade
{
public:
	void Spawn(void);
	void Precache(void);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void Killed(entvars_t *pevAttacker, int iGib);

public:
	void EXPORT TankThink(void);
	void EXPORT TankTouch(CBaseEntity *pOther);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_state;
};

LINK_ENTITY_TO_CLASS(item_airtank, CAirtank);

TYPEDESCRIPTION CAirtank::m_SaveData[] =
{
	DEFINE_FIELD(CAirtank, m_state, FIELD_INTEGER)
};

IMPLEMENT_SAVERESTORE(CAirtank, CGrenade);

void CAirtank::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_oxygen.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 36));
	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CAirtank::TankThink);
	SetTouch(&CAirtank::TankTouch);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_YES;
	pev->health = 20;
	pev->dmg = 50;
	m_state = 1;
}

void CAirtank::Precache(void)
{
	PRECACHE_MODEL("models/w_oxygen.mdl");
	PRECACHE_SOUND("doors/aliendoor3.wav");
}

void CAirtank::Killed(entvars_t *pevAttacker, int iGib)
{
	pev->owner = ENT(pevAttacker);
	Explode(pev->origin, Vector(0, 0, -1));
}

void CAirtank::TankThink(void)
{
	m_state = 1;
	SUB_UseTargets(this, USE_TOGGLE, 0);
}

void CAirtank::TankTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	if (!m_state)
	{
		EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", VOL_NORM, ATTN_NORM);
		return;
	}

	pOther->pev->air_finished = gpGlobals->time + 12;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "doors/aliendoor3.wav", VOL_NORM, ATTN_NORM);
	pev->nextthink = gpGlobals->time + 30;
	m_state = 0;
	SUB_UseTargets(this, USE_TOGGLE, 1);
}
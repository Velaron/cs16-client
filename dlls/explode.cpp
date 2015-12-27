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
#include "decals.h"
#include "explode.h"

class CShower : public CBaseEntity
{
public:
	void Spawn(void);
	void Think(void);
	void Touch(CBaseEntity *pOther);
	int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS(spark_shower, CShower);

void CShower::Spawn(void)
{
	pev->velocity = RANDOM_FLOAT(200, 300) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT(-100, 100);
	pev->velocity.y += RANDOM_FLOAT(-100, 100);

	if (pev->velocity.z >= 0)
		pev->velocity.z += 200;
	else
		pev->velocity.z -= 200;

	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;

	SET_MODEL(edict(), "models/grenade.mdl");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT(0.5, 1.5);
	pev->angles = g_vecZero;
}

void CShower::Think(void)
{
	UTIL_Sparks(pev->origin);
	pev->speed -= 0.1;

	if (pev->speed > 0)
		pev->nextthink = gpGlobals->time + 0.1;
	else
		UTIL_Remove(this);

	pev->flags &= ~FL_ONGROUND;
}

void CShower::Touch(CBaseEntity *pOther)
{
	if (pev->flags & FL_ONGROUND)
		pev->velocity = pev->velocity * 0.1;
	else
		pev->velocity = pev->velocity * 0.6;

	if ((pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y) < 10)
		pev->speed = 0;
}

class CEnvExplosion : public CBaseMonster
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT Smoke(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_iMagnitude;
	int m_spriteScale;
};

TYPEDESCRIPTION CEnvExplosion::m_SaveData[] =
{
	DEFINE_FIELD(CEnvExplosion, m_iMagnitude, FIELD_INTEGER),
	DEFINE_FIELD(CEnvExplosion, m_spriteScale, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CEnvExplosion, CBaseMonster);
LINK_ENTITY_TO_CLASS(env_explosion, CEnvExplosion);

void CEnvExplosion::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		m_iMagnitude = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CEnvExplosion::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->movetype = MOVETYPE_NONE;

	float flSpriteScale = (m_iMagnitude - 50) * 0.6;

	if (flSpriteScale < 10)
		flSpriteScale = 10;

	m_spriteScale = (int)flSpriteScale;
}

void CEnvExplosion::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	pev->model = iStringNull;
	pev->solid = SOLID_NOT;

	TraceResult tr;
	Vector vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	if (tr.flFraction != 1)
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (m_iMagnitude - 24) * 0.6);
	/*
	else
		pev->origin = pev->origin;
	*/

	if (!(pev->spawnflags & SF_ENVEXPLOSION_NODECAL))
	{
		if (RANDOM_FLOAT(0, 1) < 0.5)
			UTIL_DecalTrace(&tr, DECAL_SCORCH1);
		else
			UTIL_DecalTrace(&tr, DECAL_SCORCH2);
	}

	if (!(pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL))
	{
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE((BYTE)m_spriteScale);
		WRITE_BYTE(15);
		WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE(0);
		WRITE_BYTE(15);
		WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();
	}

	if (!(pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE))
		RadiusDamage(pev, pev, m_iMagnitude, CLASS_NONE, DMG_BLAST);

	SetThink(&CEnvExplosion::Smoke);
	pev->nextthink = gpGlobals->time + 0.3;

	if (!(pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS))
	{
		int sparkCount = RANDOM_LONG(0, 3);

		for (int i = 0; i < sparkCount; i++)
			Create("spark_shower", pev->origin, tr.vecPlaneNormal, NULL);
	}
}

void CEnvExplosion::Smoke(void)
{
	if (!(pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE))
	{
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE((BYTE)m_spriteScale);
		WRITE_BYTE(12);
		MESSAGE_END();
	}

	if (!(pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE))
		UTIL_Remove(this);
}

void ExplosionCreate(const Vector &center, const Vector &angles, edict_t *pOwner, int magnitude, BOOL doDamage)
{
	CBaseEntity *pExplosion = CBaseEntity::Create("env_explosion", center, angles, pOwner);

	char buf[128];
	sprintf(buf, "%3d", magnitude);

	KeyValueData kvd;
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue(&kvd);

	if (!doDamage)
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->Use(NULL, NULL, USE_TOGGLE, 0);
}
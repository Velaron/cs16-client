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
#include "animation.h"
#include "weapons.h"
#include "player.h"

#define TEMP_FOR_SCREEN_SHOTS
#ifdef TEMP_FOR_SCREEN_SHOTS

class CCycler : public CBaseMonster
{
public:
	void GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax);
	int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() | FCAP_IMPULSE_USE); }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void Spawn(void);
	void Think(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	BOOL IsAlive(void) { return FALSE; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_animate;
};

TYPEDESCRIPTION CCycler::m_SaveData[] =
{
	DEFINE_FIELD(CCycler, m_animate, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CCycler, CBaseMonster);

class CGenericCycler : public CCycler
{
public:
	void Spawn(void) { GenericCyclerSpawn((char *)STRING(pev->model), Vector(-16, -16, 0), Vector(16, 16, 72)); }
};

LINK_ENTITY_TO_CLASS(cycler, CGenericCycler);

class CCyclerProbe : public CCycler
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(cycler_prdroid, CCyclerProbe);

void CCyclerProbe::Spawn(void)
{
	pev->origin = pev->origin + Vector(0, 0, 16);
	GenericCyclerSpawn("models/prdroid.mdl", Vector(-16, -16, -16), Vector(16, 16, 16));
}

void CCycler::GenericCyclerSpawn(char *szModel, Vector vecMin, Vector vecMax)
{
	if (!szModel || !*szModel)
	{
		ALERT(at_error, "cycler at %.0f %.0f %0.f missing modelname", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (pev->classname)
		RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->classname = MAKE_STRING("cycler");
	AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	PRECACHE_MODEL(szModel);
	SET_MODEL(ENT(pev), szModel);
	CCycler::Spawn();
	UTIL_SetSize(pev, vecMin, vecMax);
}

void CCycler::Spawn(void)
{
	InitBoneControllers();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->health = 80000;
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;
	ChangeYaw(360);

	m_flFrameRate = 75;
	m_flGroundSpeed = 0;
	pev->nextthink += 1;

	ResetSequenceInfo();

	if (pev->sequence != 0 || pev->frame != 0)
	{
		m_animate = 0;
		pev->framerate = 0;
	}
	else
		m_animate = 1;
}

void CCycler::Think(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_animate)
		StudioFrameAdvance();

	if (m_fSequenceFinished && !m_fSequenceLoops)
	{
		pev->animtime = gpGlobals->time;
		pev->framerate = 1;
		m_fSequenceFinished = FALSE;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;

		if (!m_animate)
			pev->framerate = 0;
	}
}

void CCycler::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;

	if (m_animate)
		pev->framerate = 1;
	else
		pev->framerate = 0;
}

int CCycler::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if (m_animate)
	{
		pev->sequence++;
		ResetSequenceInfo();

		if (m_flFrameRate == 0)
		{
			pev->sequence = 0;
			ResetSequenceInfo();
		}

		pev->frame = 0;
	}
	else
	{
		pev->framerate = 1.0;
		StudioFrameAdvance(0.1);
		pev->framerate = 0;
		ALERT(at_console, "sequence: %d, frame %.0f\n", pev->sequence, pev->frame);
	}

	return 0;
}

#endif

class CCyclerSprite : public CBaseEntity
{
public:
	void Spawn(void);
	void Restart(void);
	void Think(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() | FCAP_DONT_SAVE | FCAP_IMPULSE_USE); }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void Animate(float frames);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	inline int ShouldAnimate(void) { return m_animate && m_maxFrame > 1.0; }

public:
	static TYPEDESCRIPTION m_SaveData[];

	int m_animate;
	float m_lastTime;
	float m_maxFrame;
	int m_renderfx;
	int m_rendermode;
	float m_renderamt;
	vec3_t m_rendercolor;
};

LINK_ENTITY_TO_CLASS(cycler_sprite, CCyclerSprite);

TYPEDESCRIPTION CCyclerSprite::m_SaveData[] =
{
	DEFINE_FIELD(CCyclerSprite, m_animate, FIELD_INTEGER),
	DEFINE_FIELD(CCyclerSprite, m_lastTime, FIELD_TIME),
	DEFINE_FIELD(CCyclerSprite, m_maxFrame, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CCyclerSprite, CBaseEntity);

void CCyclerSprite::Spawn(void)
{
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;

	m_animate = 1;
	m_lastTime = gpGlobals->time;

	PRECACHE_MODEL((char *)STRING(pev->model));
	SET_MODEL(ENT(pev), STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
	m_renderfx = pev->renderfx;
	m_rendermode = pev->rendermode;
	m_renderamt = pev->renderamt;
	m_rendercolor[0] = pev->rendercolor[0];
	m_rendercolor[1] = pev->rendercolor[1];
	m_rendercolor[2] = pev->rendercolor[2];
}

void CCyclerSprite::Restart(void)
{
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;

	m_animate = 1;
	m_lastTime = gpGlobals->time;

	pev->renderfx = m_renderfx;
	pev->rendermode = m_rendermode;
	pev->renderamt = m_renderamt;
	pev->rendercolor[0] = m_rendercolor[0];
	pev->rendercolor[1] = m_rendercolor[1];
	pev->rendercolor[2] = m_rendercolor[2];
}

void CCyclerSprite::Think(void)
{
	if (ShouldAnimate())
		Animate(pev->framerate * (gpGlobals->time - m_lastTime));

	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CCyclerSprite::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_animate = !m_animate;
	ALERT(at_console, "Sprite: %s\n", STRING(pev->model));
}

int CCyclerSprite::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if (m_maxFrame > 1)
		Animate(1);

	return 1;
}

void CCyclerSprite::Animate(float frames)
{
	pev->frame += frames;

	if (m_maxFrame > 0)
		pev->frame = fmod(pev->frame, m_maxFrame);
}

class CWeaponCycler : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	int iItemSlot(void) { return 1; }
	int GetItemInfo(ItemInfo *p) { return 0; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	BOOL Deploy(void);
	void Holster(int skiplocal = 0);

public:
	int m_iszModel;
	int m_iModel;
};

LINK_ENTITY_TO_CLASS(cycler_weapon, CWeaponCycler);

void CWeaponCycler::Spawn(void)
{
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;

	PRECACHE_MODEL((char *)STRING(pev->model));
	SET_MODEL(ENT(pev), STRING(pev->model));

	m_iszModel = pev->model;
	m_iModel = pev->modelindex;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CBasePlayerItem::DefaultTouch);
}

BOOL CWeaponCycler::Deploy(void)
{
	m_pPlayer->pev->viewmodel = m_iszModel;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;

	SendWeaponAnim(0);
	m_iClip = 0;
	return TRUE;
}

void CWeaponCycler::Holster(int skiplocal)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CWeaponCycler::PrimaryAttack(void)
{
	SendWeaponAnim(pev->sequence);
	m_flNextPrimaryAttack = gpGlobals->time + 0.3;
}

void CWeaponCycler::SecondaryAttack(void)
{
	pev->sequence = (pev->sequence + 1) % 8;
	pev->modelindex = m_iModel;

	void *pmodel = GET_MODEL_PTR(ENT(pev));

	float flFrameRate, flGroundSpeed;
	GetSequenceInfo(pmodel, pev, &flFrameRate, &flGroundSpeed);
	pev->modelindex = 0;

	if (flFrameRate == 0)
		pev->sequence = 0;

	SendWeaponAnim(pev->sequence);
	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

class CWreckage : public CBaseMonster
{
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void Spawn(void);
	void Precache(void);
	void Think(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_flStartTime;
};

TYPEDESCRIPTION CWreckage::m_SaveData[] =
{
	DEFINE_FIELD(CWreckage, m_flStartTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CWreckage, CBaseMonster);
LINK_ENTITY_TO_CLASS(cycler_wreckage, CWreckage);

void CWreckage::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = 0;
	pev->effects = 0;
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->model)
	{
		PRECACHE_MODEL((char *)STRING(pev->model));
		SET_MODEL(ENT(pev), STRING(pev->model));
	}

	m_flStartTime = (int)(gpGlobals->time);
}

void CWreckage::Precache(void)
{
	if (pev->model)
		PRECACHE_MODEL((char *)STRING(pev->model));
}

void CWreckage::Think(void)
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->dmgtime)
	{
		if (pev->dmgtime < gpGlobals->time)
		{
			UTIL_Remove(this);
			return;
		}
		else
		{
			if (RANDOM_FLOAT(0, pev->dmgtime - m_flStartTime) > pev->dmgtime - gpGlobals->time)
				return;
		}
	}

	Vector VecSrc;
	VecSrc.x = RANDOM_FLOAT(pev->absmin.x, pev->absmax.x);
	VecSrc.y = RANDOM_FLOAT(pev->absmin.y, pev->absmax.y);
	VecSrc.z = RANDOM_FLOAT(pev->absmin.z, pev->absmax.z);

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, VecSrc);
	WRITE_BYTE(TE_SMOKE);
	WRITE_COORD(VecSrc.x);
	WRITE_COORD(VecSrc.y);
	WRITE_COORD(VecSrc.z);
	WRITE_SHORT(g_sModelIndexSmoke);
	WRITE_BYTE(RANDOM_LONG(0, 49) + 50);
	WRITE_BYTE(RANDOM_LONG(0, 3) + 8);
	MESSAGE_END();
}
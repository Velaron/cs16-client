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
#include "player.h"
#include "saverestore.h"
#include "trains.h"
#include "gamerules.h"
#include "weapons.h"
#include "hostage.h"

#define savesolid team

#define GRENADETYPE_SMOKE 1
#define GRENADETYPE_FLASH 2

class CBaseGrenCatch : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Touch(CBaseEntity *pOther);
	void Think(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_NeedGrenadeType;
	string_t sTriggerOnGrenade;
	string_t sDisableOnGrenade;
	bool m_fSmokeTouching;
	bool m_fFlashTouched;
};

TYPEDESCRIPTION CBaseGrenCatch::m_SaveData[] =
{
	DEFINE_FIELD(CBaseGrenCatch, m_NeedGrenadeType, FIELD_INTEGER),
	DEFINE_FIELD(CBaseGrenCatch, m_fSmokeTouching, FIELD_BOOLEAN),
	DEFINE_FIELD(CBaseGrenCatch, m_fFlashTouched, FIELD_BOOLEAN),
	DEFINE_FIELD(CBaseGrenCatch, sTriggerOnGrenade, FIELD_STRING),
	DEFINE_FIELD(CBaseGrenCatch, sDisableOnGrenade, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CBaseGrenCatch, CBaseEntity);
LINK_ENTITY_TO_CLASS(func_grencatch, CBaseGrenCatch);

void CBaseGrenCatch::Spawn(void)
{
	pev->solid = SOLID_TRIGGER;
	pev->flags |= FL_WORLDBRUSH;
	pev->effects |= EF_NODRAW;

	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBaseGrenCatch::Touch(CBaseEntity *pOther)
{
	if (!pOther)
		return;

	if (strstr(STRING(pev->model), "flash"))
		m_fFlashTouched = true;
}

void CBaseGrenCatch::Think(void)
{
	CGrenade *pGrenade;
	bool m_fSmokeTouchingLastFrame;
	CBaseEntity *pTrigger;
	Vector vMax, vMin;

	m_fSmokeTouchingLastFrame = m_fSmokeTouching;
	m_fSmokeTouching = false;
	pGrenade = NULL;

	while (pGrenade = (CGrenade *)UTIL_FindEntityByClassname(pGrenade, "grenade"))
	{
		vMin = pGrenade->pev->mins;
		vMax = pGrenade->pev->maxs;

		UTIL_SetSize(pGrenade->pev, Vector(-8, -8, 0), Vector(8, 8, 0));

		if (pGrenade->Intersects(this) && strstr(STRING(pGrenade->pev->model), "smoke"))
		{
			if (pGrenade->pev->velocity.Length() == 0)
				m_fSmokeTouching = true;
		}

		pGrenade->pev->mins = vMin;
		pGrenade->pev->maxs = vMax;
	}

	if ((m_NeedGrenadeType == GRENADETYPE_SMOKE && m_fSmokeTouching && !m_fSmokeTouchingLastFrame) || (m_NeedGrenadeType == GRENADETYPE_FLASH && m_fFlashTouched))
	{
		FireTargets(STRING(sTriggerOnGrenade), this, this, USE_TOGGLE, 0);

		if (m_NeedGrenadeType == GRENADETYPE_SMOKE)
		{
			CBaseEntity *pEntity = NULL;

			while (pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(sDisableOnGrenade)))
			{
				pEntity->pev->savesolid = pEntity->pev->solid;
				pEntity->pev->solid = SOLID_NOT;
			}
		}
		else if (m_NeedGrenadeType == GRENADETYPE_FLASH)
			pev->flags |= FL_KILLME;
	}

	if (m_fSmokeTouchingLastFrame && !m_fSmokeTouching)
	{
		pTrigger = NULL;

		while (pTrigger = UTIL_FindEntityByTargetname(pTrigger, STRING(sDisableOnGrenade)))
		{
			pTrigger->pev->solid = pTrigger->pev->savesolid;
			pTrigger->pev->savesolid = 0;
			UTIL_SetOrigin(pTrigger->pev, pTrigger->pev->origin);
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

void CBaseGrenCatch::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "triggerongrenade"))
	{
		sTriggerOnGrenade = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "disableongrenade"))
	{
		sDisableOnGrenade = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "grenadetype"))
	{
		if (!strcmp(pkvd->szValue, "smoke"))
		{
			m_NeedGrenadeType = GRENADETYPE_SMOKE;
			pkvd->fHandled = TRUE;
		}
		else if (!strcmp(pkvd->szValue, "flash"))
		{
			m_NeedGrenadeType = GRENADETYPE_FLASH;
			pkvd->fHandled = TRUE;
		}
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

#define MAX_ITEM_COUNTS 32

class CFuncWeaponCheck : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void Touch(CBaseEntity *pOther);

public:
	inline float GetNoItemDelay(void) { return pev->speed; }
	inline void SetNoItemDelay(float delay) { pev->speed = delay; }

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	string_t sTriggerWithItems;
	string_t sTriggerNoItems;
	string_t sMaster[MAX_ITEM_COUNTS];
	string_t sItemName;
	int iItemCount;
	int iAnyWeapon;
};

TYPEDESCRIPTION CFuncWeaponCheck::m_SaveData[] =
{
	DEFINE_FIELD(CFuncWeaponCheck, sTriggerWithItems, FIELD_STRING),
	DEFINE_FIELD(CFuncWeaponCheck, sTriggerNoItems, FIELD_STRING),
	DEFINE_FIELD(CFuncWeaponCheck, iItemCount, FIELD_INTEGER),
	DEFINE_ARRAY(CFuncWeaponCheck, sMaster, FIELD_STRING, MAX_ITEM_COUNTS),
	DEFINE_FIELD(CFuncWeaponCheck, sItemName, FIELD_STRING),
	DEFINE_FIELD(CFuncWeaponCheck, iAnyWeapon, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CFuncWeaponCheck, CBaseEntity);
LINK_ENTITY_TO_CLASS(func_weaponcheck, CFuncWeaponCheck);

void CFuncWeaponCheck::Spawn(void)
{
	pev->dmgtime = 0;
	pev->solid = SOLID_TRIGGER;
	pev->flags |= FL_WORLDBRUSH;
	pev->solid |= EF_NODRAW;

	SET_MODEL(ENT(pev), STRING(pev->model));
}

void CFuncWeaponCheck::Touch(CBaseEntity *pOther)
{
	if (!UTIL_IsMasterTriggered(sMaster[0], pOther))
		return;

	if (!pOther)
		return;

	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	for (int i = 1; i <= iItemCount; i++)
	{
		if (iAnyWeapon)
		{
			if (pPlayer->HasNamedPlayerItem(STRING(sMaster[i])))
				break;

			continue;
		}

		if (!pPlayer->HasNamedPlayerItem(STRING(sMaster[i])))
		{
			if (pev->dmgtime < gpGlobals->time)
			{
				if (pev->speed > -1)
				{
					FireTargets(STRING(sTriggerNoItems), pOther, pOther, USE_TOGGLE, 0);
					pev->dmgtime = pev->speed + gpGlobals->time;

					if (!pev->speed)
						pev->speed = -1;
				}
			}

			return;
		}
	}

	FireTargets(STRING(sTriggerWithItems), pOther, pOther, USE_TOGGLE, 0);
	SUB_Remove();
}

void CFuncWeaponCheck::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "trigger_items"))
	{
		sTriggerWithItems = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "trigger_noitems"))
	{
		sTriggerNoItems = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "trigger_noitems_delay"))
	{
		pev->speed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (strstr(pkvd->szKeyName, "item"))
	{
		if (iItemCount < MAX_ITEM_COUNTS)
			sMaster[++iItemCount] = ALLOC_STRING(pkvd->szValue);

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "master"))
	{
		sMaster[0] = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "any_weapon"))
	{
		iAnyWeapon = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

#define SF_TRIGGER_PUSH_START_OFF 2
#define SF_TRIGGER_HURT_TARGETONCE 1
#define SF_TRIGGER_HURT_START_OFF 2
#define SF_TRIGGER_HURT_NO_CLIENTS 8
#define SF_TRIGGER_HURT_CLIENTONLYFIRE 16
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32

extern DLL_GLOBAL BOOL g_fGameOver;
extern void SetMovedir(entvars_t *pev);

class CFrictionModifier : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

public:
	void EXPORT ChangeFriction(CBaseEntity *pOther);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_frictionFraction;
};

LINK_ENTITY_TO_CLASS(func_friction, CFrictionModifier);

TYPEDESCRIPTION CFrictionModifier::m_SaveData[] =
{
	DEFINE_FIELD(CFrictionModifier, m_frictionFraction, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CFrictionModifier, CBaseEntity);

void CFrictionModifier::Spawn(void)
{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->movetype = MOVETYPE_NONE;
	SetTouch(&CFrictionModifier::ChangeFriction);
}

void CFrictionModifier::ChangeFriction(CBaseEntity *pOther)
{
	if (pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE)
		pOther->pev->friction = m_frictionFraction;
}

void CFrictionModifier::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

#define SF_AUTO_FIREONCE 0x0001

class CAutoTrigger : public CBaseDelay
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Precache(void);
	void Think(void);
	int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	int m_globalstate;
	USE_TYPE triggerType;
};

LINK_ENTITY_TO_CLASS(trigger_auto, CAutoTrigger);

TYPEDESCRIPTION CAutoTrigger::m_SaveData[] =
{
	DEFINE_FIELD(CAutoTrigger, m_globalstate, FIELD_STRING),
	DEFINE_FIELD(CAutoTrigger, triggerType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CAutoTrigger, CBaseDelay);

void CAutoTrigger::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);

		switch (type)
		{
			case 0: triggerType = USE_OFF; break;
			case 2: triggerType = USE_TOGGLE; break;
			default: triggerType = USE_ON; break;
		}

		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CAutoTrigger::Spawn(void)
{
	Precache();
}

void CAutoTrigger::Precache(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
}

void CAutoTrigger::Think(void)
{
	if (!m_globalstate || gGlobalState.EntityGetState(m_globalstate) == GLOBAL_ON)
	{
		SUB_UseTargets(this, triggerType, 0);

		if (pev->spawnflags & SF_AUTO_FIREONCE)
			UTIL_Remove(this);
	}
}

#define SF_RELAY_FIREONCE 0x0001

class CTriggerRelay : public CBaseDelay
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	USE_TYPE triggerType;
};

LINK_ENTITY_TO_CLASS(trigger_relay, CTriggerRelay);

TYPEDESCRIPTION CTriggerRelay::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerRelay, triggerType, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerRelay, CBaseDelay);

void CTriggerRelay::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);

		switch (type)
		{
			case 0: triggerType = USE_OFF; break;
			case 2: triggerType = USE_TOGGLE; break;
			default: triggerType = USE_ON; break;
		}

		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerRelay::Spawn(void)
{

}

void CTriggerRelay::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SUB_UseTargets(this, triggerType, 0);

	if (pev->spawnflags & SF_RELAY_FIREONCE)
		UTIL_Remove(this);
}

#define SF_MULTIMAN_CLONE 0x80000000
#define SF_MULTIMAN_THREAD 0x00000001

class CMultiManager : public CBaseToggle
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Restart(void);
	BOOL HasTarget(string_t targetname);
	int ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT ManagerThink(void);
	void EXPORT ManagerUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
	inline BOOL IsClone(void) { return (pev->spawnflags & SF_MULTIMAN_CLONE) ? TRUE : FALSE; }
	inline BOOL ShouldClone(void)
	{
		if (IsClone())
			return FALSE;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) ? TRUE : FALSE;
	}

	CMultiManager *Clone(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_cTargets;
	int m_index;
	float m_startTime;
	int m_iTargetName[MAX_MULTI_TARGETS];
	float m_flTargetDelay[MAX_MULTI_TARGETS];
};

LINK_ENTITY_TO_CLASS(multi_manager, CMultiManager);

TYPEDESCRIPTION CMultiManager::m_SaveData[] =
{
	DEFINE_FIELD(CMultiManager, m_cTargets, FIELD_INTEGER),
	DEFINE_FIELD(CMultiManager, m_index, FIELD_INTEGER),
	DEFINE_FIELD(CMultiManager, m_startTime, FIELD_TIME),
	DEFINE_ARRAY(CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS),
	DEFINE_ARRAY(CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS),
};

IMPLEMENT_SAVERESTORE(CMultiManager, CBaseToggle);

void CMultiManager::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		if (m_cTargets < MAX_MULTI_TARGETS)
		{
			char tmp[128];
			UTIL_StripToken(pkvd->szKeyName, tmp);
			m_iTargetName[m_cTargets] = ALLOC_STRING(tmp);
			m_flTargetDelay[m_cTargets++] = atof(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
	}
}

void CMultiManager::Spawn(void)
{
	pev->solid = SOLID_NOT;
	SetUse(&CMultiManager::ManagerUse);
	SetThink(&CMultiManager::ManagerThink);

	int swapped = 1;

	while (swapped)
	{
		swapped = 0;

		for (int i = 1; i < m_cTargets; i++)
		{
			if (m_flTargetDelay[i] < m_flTargetDelay[i - 1])
			{
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i - 1];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_iTargetName[i - 1] = name;
				m_flTargetDelay[i - 1] = delay;
				swapped = 1;
			}
		}
	}
}

void CMultiManager::Restart(void)
{
	edict_t *pentTarget = NULL;

	for (int i = 0; i < m_cTargets; i++)
	{
		const char *name = STRING(m_iTargetName[i]);

		if (!name)
			continue;

		pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));

		if (FNullEnt(pentTarget))
			break;

		CBaseEntity *pTarget = (CBaseEntity *)CBaseEntity::Instance(pentTarget);

		if (pTarget && !(pTarget->pev->flags & FL_KILLME))
			pTarget->Restart();
	}

	SetThink(NULL);

	if (IsClone())
	{
		UTIL_Remove(this);
		return;
	}

	SetUse(&CMultiManager::ManagerUse);
	m_index = 0;
}

BOOL CMultiManager::HasTarget(string_t targetname)
{
	for (int i = 0; i < m_cTargets; i++)
	{
		if (FStrEq(STRING(targetname), STRING(m_iTargetName[i])))
			return TRUE;
	}

	return FALSE;
}

void CMultiManager::ManagerThink(void)
{
	float time = gpGlobals->time - m_startTime;

	while (m_index < m_cTargets && m_flTargetDelay[m_index] <= time)
	{
		FireTargets(STRING(m_iTargetName[m_index]), m_hActivator, this, USE_TOGGLE, 0);
		m_index++;
	}

	if (m_index >= m_cTargets)
	{
		SetThink(NULL);

		if (IsClone())
		{
			UTIL_Remove(this);
			return;
		}

		SetUse(&CMultiManager::ManagerUse);
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[m_index];
}

CMultiManager *CMultiManager::Clone(void)
{
	CMultiManager *pMulti = GetClassPtr((CMultiManager *)NULL);
	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy(pMulti->pev, pev, sizeof(*pev));
	pMulti->pev->pContainingEntity = pEdict;
	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy(pMulti->m_iTargetName, m_iTargetName, sizeof(m_iTargetName));
	memcpy(pMulti->m_flTargetDelay, m_flTargetDelay, sizeof(m_flTargetDelay));
	return pMulti;
}

void CMultiManager::ManagerUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (ShouldClone())
	{
		CMultiManager *pClone = Clone();
		pClone->ManagerUse(pActivator, pCaller, useType, value);
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse(NULL);
	SetThink(&CMultiManager::ManagerThink);
	pev->nextthink = gpGlobals->time;
}

#define SF_RENDER_MASKFX (1<<0)
#define SF_RENDER_MASKAMT (1<<1)
#define SF_RENDER_MASKMODE (1<<2)
#define SF_RENDER_MASKCOLOR (1<<3)

class CRenderFxManager : public CBaseEntity
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(env_render, CRenderFxManager);

void CRenderFxManager::Spawn(void)
{
	pev->solid = SOLID_NOT;
}

void CRenderFxManager::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FStringNull(pev->target))
		return;

	edict_t *pentTarget = NULL;

	while (1)
	{
		pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));

		if (FNullEnt(pentTarget))
			break;

		entvars_t *pevTarget = VARS(pentTarget);

		if (!FBitSet(pev->spawnflags, SF_RENDER_MASKFX))
			pevTarget->renderfx = pev->renderfx;

		if (!FBitSet(pev->spawnflags, SF_RENDER_MASKAMT))
			pevTarget->renderamt = pev->renderamt;

		if (!FBitSet(pev->spawnflags, SF_RENDER_MASKMODE))
			pevTarget->rendermode = pev->rendermode;

		if (!FBitSet(pev->spawnflags, SF_RENDER_MASKCOLOR))
			pevTarget->rendercolor = pev->rendercolor;
	}
}

class CBaseTrigger : public CBaseToggle
{
public:
	void KeyValue(KeyValueData *pkvd);
	int ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

public:
	void EXPORT TeleportTouch(CBaseEntity *pOther);
	void EXPORT MultiTouch(CBaseEntity *pOther);
	void EXPORT HurtTouch (CBaseEntity *pOther);
	void EXPORT CDAudioTouch(CBaseEntity *pOther);
	void ActivateMultiTrigger(CBaseEntity *pActivator);
	void EXPORT MultiWaitOver(void);
	void EXPORT CounterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void InitTrigger(void);
};

LINK_ENTITY_TO_CLASS(trigger, CBaseTrigger);

void CBaseTrigger::InitTrigger(void)
{
	if (pev->angles != g_vecZero)
		SetMovedir(pev);

	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));

	if (CVAR_GET_FLOAT("showtriggers") == 0)
		SetBits(pev->effects, EF_NODRAW);
}

void CBaseTrigger::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = (int) atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT RadiationThink(void);
};

LINK_ENTITY_TO_CLASS(trigger_hurt, CTriggerHurt);

class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn(void);
	void Touch(CBaseEntity *pOther);
	void Think(void);
};

LINK_ENTITY_TO_CLASS(trigger_monsterjump, CTriggerMonsterJump);

void CTriggerMonsterJump::Spawn(void)
{
	SetMovedir(pev);
	InitTrigger();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if (!FStringNull(pev->targetname))
	{
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin(pev, pev->origin);
		SetUse(&CBaseTrigger::ToggleUse);
	}
}

void CTriggerMonsterJump::Think(void)
{
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin(pev, pev->origin);
	SetThink(NULL);
}

void CTriggerMonsterJump::Touch(CBaseEntity *pOther)
{
	entvars_t *pevOther = pOther->pev;

	if (!FBitSet(pevOther->flags, FL_MONSTER))
		return;

	pevOther->origin.z += 1;

	if (FBitSet(pevOther->flags, FL_ONGROUND))
		pevOther->flags &= ~FL_ONGROUND;

	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}

class CTriggerCDAudio : public CBaseTrigger
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Touch(CBaseEntity *pOther);
	void PlayTrack(void);
};

LINK_ENTITY_TO_CLASS(trigger_cdaudio, CTriggerCDAudio);

void CTriggerCDAudio::Touch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	PlayTrack();
}

void CTriggerCDAudio::Spawn(void)
{
	InitTrigger();
}

void CTriggerCDAudio::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	PlayTrack();
}

void PlayCDTrack(int iTrack)
{
	edict_t *pClient = g_engfuncs.pfnPEntityOfEntIndex(1);

	if (!pClient)
		return;

	if (iTrack < -1 || iTrack > 30)
	{
		ALERT(at_console, "TriggerCDAudio - Track %d out of range\n");
		return;
	}

	if (iTrack == -1)
	{
		CLIENT_COMMAND(pClient, "cd pause\n");
	}
	else
	{
		char string[64];
		sprintf(string, "cd play %3d\n", iTrack);
		CLIENT_COMMAND(pClient, string);
	}
}

void CTriggerCDAudio::PlayTrack(void)
{
	PlayCDTrack((int)pev->health);
	SetTouch(NULL);
	UTIL_Remove(this);
}

class CTargetCDAudio : public CPointEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Think(void);
	void Play(void);
};

LINK_ENTITY_TO_CLASS(target_cdaudio, CTargetCDAudio);

void CTargetCDAudio::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->scale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CTargetCDAudio::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if (pev->scale > 0)
		pev->nextthink = gpGlobals->time + 1;
}

void CTargetCDAudio::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	Play();
}

void CTargetCDAudio::Think(void)
{
	edict_t *pClient = g_engfuncs.pfnPEntityOfEntIndex(1);

	if (!pClient)
		return;

	pev->nextthink = gpGlobals->time + 0.5;

	if ((pClient->v.origin - pev->origin).Length() <= pev->scale)
		Play();
}

void CTargetCDAudio::Play(void)
{
	PlayCDTrack((int)pev->health);
	UTIL_Remove(this);
}

void CTriggerHurt::Spawn(void)
{
	InitTrigger();
	SetTouch(&CBaseTrigger::HurtTouch);

	if (!FStringNull(pev->targetname))
		SetUse(&CBaseTrigger::ToggleUse);
	else
		SetUse(NULL);

	if (m_bitsDamageInflict & DMG_RADIATION)
	{
		SetThink(&CTriggerHurt::RadiationThink);
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0, 0.5);
	}

	if (FBitSet(pev->spawnflags, SF_TRIGGER_HURT_START_OFF))
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);
}

void CTriggerHurt::RadiationThink(void)
{
	Vector origin = pev->origin;
	Vector view_ofs = pev->view_ofs;

	pev->origin = (pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs = pev->view_ofs * 0;

	edict_t *pentPlayer = FIND_CLIENT_IN_PVS(edict());

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	if (!FNullEnt(pentPlayer))
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)VARS(pentPlayer));
		entvars_t *pevTarget = VARS(pentPlayer);
		Vector vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		Vector vecSpot2 = (pevTarget->absmin + pevTarget->absmax) * 0.5;
		Vector vecRange = vecSpot1 - vecSpot2;
		float flRange = vecRange.Length();

		if (pPlayer->m_flgeigerRange >= flRange)
			pPlayer->m_flgeigerRange = flRange;
	}

	pev->nextthink = gpGlobals->time + 0.25;
}

void CBaseTrigger::ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pev->solid == SOLID_NOT)
	{
		pev->solid = SOLID_TRIGGER;
		gpGlobals->force_retouch++;
	}
	else
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);
}

void CBaseTrigger::HurtTouch(CBaseEntity *pOther)
{
	if (!pOther->pev->takedamage)
		return;

	if ((pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH) && !pOther->IsPlayer())
		return;

	if ((pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS) && pOther->IsPlayer())
		return;

	if (g_pGameRules->IsMultiplayer())
	{
		if (pev->dmgtime > gpGlobals->time)
		{
			if (gpGlobals->time != pev->pain_finished)
			{
				if (pOther->IsPlayer())
				{
					int playerMask = 1 << (pOther->entindex() - 1);

					if (pev->impulse & playerMask)
						return;

					pev->impulse |= playerMask;
				}
				else
					return;
			}
		}
		else
		{
			pev->impulse = 0;

			if (pOther->IsPlayer())
			{
				int playerMask = 1 << (pOther->entindex() - 1);
				pev->impulse |= playerMask;
			}
		}
	}
	else if (pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished)
		return;

	float fldmg = pev->dmg * 0.5;

	if (fldmg < 0)
		pOther->TakeHealth(-fldmg, m_bitsDamageInflict);
	else
		pOther->TakeDamage(pev, pev, fldmg, m_bitsDamageInflict);

	pev->pain_finished = gpGlobals->time;
	pev->dmgtime = gpGlobals->time + 0.5;

	if (pev->target)
	{
		if (pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE)
		{
			if (!pOther->IsPlayer())
				return;
		}

		SUB_UseTargets(pOther, USE_TOGGLE, 0);

		if (pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE)
			pev->target = 0;
	}
}

class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(trigger_multiple, CTriggerMultiple);

void CTriggerMultiple::Spawn(void)
{
	if (!m_flWait)
		m_flWait = 0.2;

	InitTrigger();
	SetTouch(&CBaseTrigger::MultiTouch);
}

class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(trigger_once, CTriggerOnce);

void CTriggerOnce::Spawn(void)
{
	m_flWait = -1;
	CTriggerMultiple::Spawn();
}

void CBaseTrigger::MultiTouch(CBaseEntity *pOther)
{
	entvars_t *pevToucher = pOther->pev;

	if (((pevToucher->flags & FL_CLIENT) && !(pev->spawnflags & SF_TRIGGER_NOCLIENTS)) || ((pevToucher->flags & FL_MONSTER) && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS)) || (pev->spawnflags & SF_TRIGGER_PUSHABLES) && FClassnameIs(pevToucher, "func_pushable"))
		ActivateMultiTrigger(pOther);
}

void CBaseTrigger::ActivateMultiTrigger(CBaseEntity *pActivator)
{
	if (pev->nextthink > gpGlobals->time)
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator))
		return;

	if (FClassnameIs(pev, "trigger_secret"))
	{
		if (!pev->enemy || !FClassnameIs(pev->enemy, "player"))
			return;

		gpGlobals->found_secrets++;
	}

	if (!FStringNull(pev->noise))
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char *)STRING(pev->noise), 1, ATTN_NORM);

	m_hActivator = pActivator;
	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);

	if (pev->message && pActivator->IsPlayer())
		UTIL_ShowMessage(STRING(pev->message), pActivator);

	if (m_flWait > 0)
	{
		SetThink(&CBaseTrigger::MultiWaitOver);
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		SetTouch(NULL);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CBaseEntity::SUB_Remove);
	}
}

void CBaseTrigger::MultiWaitOver(void)
{
	SetThink(NULL);
}

void CBaseTrigger::CounterUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if (m_cTriggersLeft < 0)
		return;

	BOOL fTellActivator = (m_hActivator != 0) && FClassnameIs(m_hActivator->pev, "player") && !FBitSet(pev->spawnflags, SPAWNFLAG_NOMESSAGE);

	if (m_cTriggersLeft)
	{
		if (fTellActivator)
		{
			switch (m_cTriggersLeft)
			{
				case 1: ALERT(at_console, "Only 1 more to go..."); break;
				case 2: ALERT(at_console, "Only 2 more to go..."); break;
				case 3: ALERT(at_console, "Only 3 more to go..."); break;
				default: ALERT(at_console, "There are more to go..."); break;
			}
		}

		return;
	}

	if (fTellActivator)
		ALERT(at_console, "Sequence completed!");

	ActivateMultiTrigger(m_hActivator);
}

class CTriggerCounter : public CBaseTrigger
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(trigger_counter, CTriggerCounter);

void CTriggerCounter::Spawn(void)
{
	m_flWait = -1;

	if (!m_cTriggersLeft)
		m_cTriggersLeft = 2;

	SetUse(&CBaseTrigger::CounterUse);
}

class CTriggerVolume : public CPointEntity
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(trigger_transition, CTriggerVolume);

void CTriggerVolume::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	SET_MODEL(ENT(pev), STRING(pev->model));

	pev->model = 0;
	pev->modelindex = 0;
}

class CFireAndDie : public CBaseDelay
{
public:
	void Spawn(void);
	void Precache(void);
	void Think(void);
	int ObjectCaps(void) { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(fireanddie, CFireAndDie);

void CFireAndDie::Spawn(void)
{
	if (pev->classname)
		RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->classname = MAKE_STRING("fireanddie");
	AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);
}

void CFireAndDie::Precache(void)
{
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CFireAndDie::Think(void)
{
	SUB_UseTargets(this, USE_TOGGLE, 0);
	UTIL_Remove(this);
}

#define SF_CHANGELEVEL_USEONLY 0x0002

class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT UseChangeLevel(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TriggerChangeLevel(void);
	void EXPORT ExecuteChangeLevel(void);
	void EXPORT TouchChangeLevel(CBaseEntity *pOther);
	void ChangeLevelNow(CBaseEntity *pActivator);
	static edict_t *FindLandmark(const char *pLandmarkName);
	static int ChangeList(LEVELLIST *pLevelList, int maxList);
	static int AddTransitionToList(LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark);
	static int InTransitionVolume(CBaseEntity *pEntity, char *pVolumeName);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	char m_szMapName[cchMapNameMost];
	char m_szLandmarkName[cchMapNameMost];
	int m_changeTarget;
	float m_changeTargetDelay;
};

LINK_ENTITY_TO_CLASS(trigger_changelevel, CChangeLevel);

TYPEDESCRIPTION CChangeLevel::m_SaveData[] =
{
	DEFINE_ARRAY(CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost),
	DEFINE_ARRAY(CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost),
	DEFINE_FIELD(CChangeLevel, m_changeTarget, FIELD_STRING),
	DEFINE_FIELD(CChangeLevel, m_changeTargetDelay, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CChangeLevel, CBaseTrigger);

void CChangeLevel::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT(at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue);

		strcpy(m_szMapName, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT(at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue);

		strcpy(m_szLandmarkName, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}

void CChangeLevel::Spawn(void)
{
	if (FStrEq(m_szMapName, ""))
		ALERT(at_console, "a trigger_changelevel doesn't have a map");

	if (FStrEq(m_szLandmarkName, ""))
		ALERT(at_console, "trigger_changelevel to %s doesn't have a landmark", m_szMapName);

	if (!FStringNull(pev->targetname))
		SetUse(&CChangeLevel::UseChangeLevel);

	InitTrigger();

	if (!(pev->spawnflags & SF_CHANGELEVEL_USEONLY))
		SetTouch(&CChangeLevel::TouchChangeLevel);
}

void CChangeLevel::ExecuteChangeLevel(void)
{
	MESSAGE_BEGIN(MSG_ALL, SVC_CDTRACK);
	WRITE_BYTE(3);
	WRITE_BYTE(3);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();
}

FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t *CChangeLevel::FindLandmark(const char *pLandmarkName)
{
	edict_t *pentLandmark = FIND_ENTITY_BY_STRING(NULL, "targetname", pLandmarkName);

	while (!FNullEnt(pentLandmark))
	{
		if (FClassnameIs(pentLandmark, "info_landmark"))
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING(pentLandmark, "targetname", pLandmarkName);
	}

	ALERT(at_error, "Can't find landmark %s\n", pLandmarkName);
	return NULL;
}

void CChangeLevel::UseChangeLevel(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	ChangeLevelNow(pActivator);
}

void CChangeLevel::ChangeLevelNow(CBaseEntity *pActivator)
{
	ASSERT(!FStrEq(m_szMapName, ""));

	if (g_pGameRules->IsDeathmatch())
		return;

	if (gpGlobals->time == pev->dmgtime)
		return;

	pev->dmgtime = gpGlobals->time;

	CBaseEntity *pPlayer = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));

	if (!InTransitionVolume(pPlayer, m_szLandmarkName))
	{
		ALERT(at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName);
		return;
	}

	if (m_changeTarget)
	{
		CFireAndDie *pFireAndDie = GetClassPtr((CFireAndDie *)NULL);

		if (pFireAndDie)
		{
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			DispatchSpawn(pFireAndDie->edict());
		}
	}

	strcpy(st_szNextMap, m_szMapName);
	m_hActivator = pActivator;
	SUB_UseTargets(pActivator, USE_TOGGLE, 0);
	st_szNextSpot[0] = '\0';

	edict_t *pentLandmark = FindLandmark(m_szLandmarkName);

	if (!FNullEnt(pentLandmark))
	{
		strcpy(st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = VARS(pentLandmark)->origin;
	}

	ALERT(at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot);
	CHANGE_LEVEL(st_szNextMap, st_szNextSpot);
}

void CChangeLevel::TouchChangeLevel(CBaseEntity *pOther)
{
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	ChangeLevelNow(pOther);
}

int CChangeLevel::AddTransitionToList(LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark)
{
	if (!pLevelList || !pMapName || !pLandmarkName || !pentLandmark)
		return 0;

	for (int i = 0; i < listCount; i++)
	{
		if (pLevelList[i].pentLandmark == pentLandmark && !strcmp(pLevelList[i].mapName, pMapName))
			return 0;
	}

	strcpy(pLevelList[listCount].mapName, pMapName);
	strcpy(pLevelList[listCount].landmarkName, pLandmarkName);
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS(pentLandmark)->origin;
	return 1;
}

int BuildChangeList(LEVELLIST *pLevelList, int maxList)
{
	return CChangeLevel::ChangeList(pLevelList, maxList);
}

int CChangeLevel::InTransitionVolume(CBaseEntity *pEntity, char *pVolumeName)
{
	if (pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION)
		return 1;

	if (pEntity->pev->movetype == MOVETYPE_FOLLOW)
	{
		if (pEntity->pev->aiment)
			pEntity = CBaseEntity::Instance(pEntity->pev->aiment);
	}

	int inVolume = 1;
	edict_t *pentVolume = FIND_ENTITY_BY_TARGETNAME(NULL, pVolumeName);

	while (!FNullEnt(pentVolume))
	{
		CBaseEntity *pVolume = CBaseEntity::Instance(pentVolume);

		if (pVolume && FClassnameIs(pVolume->pev, "trigger_transition"))
		{
			if (pVolume->Intersects(pEntity))
				return 1;

			inVolume = 0;
		}

		pentVolume = FIND_ENTITY_BY_TARGETNAME(pentVolume, pVolumeName);
	}

	return inVolume;
}

#define MAX_ENTITY 512

int CChangeLevel::ChangeList(LEVELLIST *pLevelList, int maxList)
{
	int count = 0;
	edict_t *pentChangelevel = FIND_ENTITY_BY_STRING(NULL, "classname", "trigger_changelevel");

	if (FNullEnt(pentChangelevel))
		return 0;

	while (!FNullEnt(pentChangelevel))
	{
		CChangeLevel *pTrigger = GetClassPtr((CChangeLevel *)VARS(pentChangelevel));

		if (pTrigger)
		{
			edict_t *pentLandmark = FindLandmark(pTrigger->m_szLandmarkName);

			if (pentLandmark)
			{
				if (AddTransitionToList(pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark))
				{
					count++;

					if (count >= maxList)
						break;
				}
			}
		}

		pentChangelevel = FIND_ENTITY_BY_STRING(pentChangelevel, "classname", "trigger_changelevel");
	}

	if (gpGlobals->pSaveData && ((SAVERESTOREDATA *)gpGlobals->pSaveData)->pTable)
	{
		CSave saveHelper((SAVERESTOREDATA *)gpGlobals->pSaveData);

		for (int i = 0; i < count; i++)
		{
			int entityCount = 0;
			CBaseEntity *pEntList[MAX_ENTITY];
			int entityFlags[MAX_ENTITY];
			edict_t *pent = UTIL_EntitiesInPVS(pLevelList[i].pentLandmark);

			while (!FNullEnt(pent))
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pent);

				if (pEntity)
				{
					int caps = pEntity->ObjectCaps();

					if (!(caps & FCAP_DONT_SAVE))
					{
						int flags = 0;

						if (caps & FCAP_ACROSS_TRANSITION)
							flags |= FENTTABLE_MOVEABLE;

						if (pEntity->pev->globalname && !pEntity->IsDormant())
							flags |= FENTTABLE_GLOBAL;

						if (flags)
						{
							pEntList[entityCount] = pEntity;
							entityFlags[entityCount] = flags;
							entityCount++;

							if (entityCount > MAX_ENTITY)
								ALERT(at_error, "Too many entities across a transition!");
						}
					}
				}

				pent = pent->v.chain;
			}

			for (int j = 0; j < entityCount; j++)
			{
				if (entityFlags[j] && InTransitionVolume(pEntList[j], pLevelList[i].landmarkName))
				{
					int index = saveHelper.EntityIndex(pEntList[j]);
					saveHelper.EntityFlagsSet(index, entityFlags[j] | (1 << i));
				}
			}
		}
	}

	return count;
}

void NextLevel(void)
{
	CChangeLevel *pChange;
	edict_t *pent = FIND_ENTITY_BY_CLASSNAME(NULL, "trigger_changelevel");

	if (FNullEnt(pent))
	{
		gpGlobals->mapname = ALLOC_STRING("start");
		pChange = GetClassPtr((CChangeLevel *)NULL);
		strcpy(pChange->m_szMapName, "start");
	}
	else
		pChange = GetClassPtr((CChangeLevel *)VARS(pent));

	strcpy(st_szNextMap, pChange->m_szMapName);
	g_fGameOver = TRUE;

	if (pChange->pev->nextthink < gpGlobals->time)
	{
		pChange->SetThink(&CChangeLevel::ExecuteChangeLevel);
		pChange->pev->nextthink = gpGlobals->time + 0.1;
	}
}

class CLadder : public CBaseTrigger
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(func_ladder, CLadder);

void CLadder::KeyValue(KeyValueData *pkvd)
{
	CBaseTrigger::KeyValue(pkvd);
}

void CLadder::Precache(void)
{
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;

	if (CVAR_GET_FLOAT("showtriggers") == 0)
	{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}

	pev->effects &= ~EF_NODRAW;
}

void CLadder::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));
	pev->movetype = MOVETYPE_PUSH;
}

class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	void Touch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(trigger_push, CTriggerPush);

void CTriggerPush::KeyValue(KeyValueData *pkvd)
{
	CBaseTrigger::KeyValue(pkvd);
}

void CTriggerPush::Spawn(void)
{
	if (pev->angles == g_vecZero)
		pev->angles.y = 360;

	InitTrigger();

	if (!pev->speed)
		pev->speed = 100;

	if (FBitSet(pev->spawnflags, SF_TRIGGER_PUSH_START_OFF))
		pev->solid = SOLID_NOT;

	SetUse(&CBaseTrigger::ToggleUse);
	UTIL_SetOrigin(pev, pev->origin);
}

void CTriggerPush::Touch(CBaseEntity *pOther)
{
	entvars_t *pevToucher = pOther->pev;

	switch (pevToucher->movetype)
	{
		case MOVETYPE_NONE:
		case MOVETYPE_PUSH:
		case MOVETYPE_NOCLIP:
		case MOVETYPE_FOLLOW: return;
	}

	if (pevToucher->solid != SOLID_NOT && pevToucher->solid != SOLID_BSP)
	{
		if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
		{
			pevToucher->velocity = pevToucher->velocity + (pev->speed * pev->movedir);

			if (pevToucher->velocity.z > 0)
				pevToucher->flags &= ~FL_ONGROUND;

			UTIL_Remove(this);
		}
		else
		{
			Vector vecPush = (pev->speed * pev->movedir);

			if (pevToucher->flags & FL_BASEVELOCITY)
				vecPush = vecPush + pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;
			pevToucher->flags |= FL_BASEVELOCITY;
		}
	}
}

void CBaseTrigger::TeleportTouch(CBaseEntity *pOther)
{
	entvars_t *pevToucher = pOther->pev;

	if (!FBitSet(pevToucher->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if (!(pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS))
	{
		if (FBitSet(pevToucher->flags, FL_MONSTER))
			return;
	}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS))
	{
		if (pOther->IsPlayer())
			return;
	}

	edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (FNullEnt(pentTarget))
		return;

	Vector tmp = VARS(pentTarget)->origin;

	if (pOther->IsPlayer())
		tmp.z -= pOther->pev->mins.z;

	tmp.z++;
	pevToucher->flags &= ~FL_ONGROUND;
	UTIL_SetOrigin(pevToucher, tmp);
	pevToucher->angles = pentTarget->v.angles;

	if (pOther->IsPlayer())
		pevToucher->v_angle = pentTarget->v.angles;

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
}

class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(trigger_teleport, CTriggerTeleport);

void CTriggerTeleport::Spawn(void)
{
	InitTrigger();
	SetTouch(&CBaseTrigger::TeleportTouch);
}

LINK_ENTITY_TO_CLASS(info_teleport_destination, CPointEntity);

class CBuyZone : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT BuyTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(func_buyzone, CBuyZone);

void CBuyZone::Spawn(void)
{
	InitTrigger();
	SetTouch(&CBuyZone::BuyTouch);

	if (pev->team > TEAM_CT || pev->team < TEAM_UNASSIGNED)
	{
		ALERT(at_console, "Bad team number (%i) in func_buyzone\n", pev->team);
		pev->team = 0;
	}
}

void CBuyZone::BuyTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (pev->team == TEAM_UNASSIGNED || pev->team == pPlayer->m_iTeam)
		pPlayer->m_signals.Signal(SIGNAL_BUY);
}

class CBombTarget : public CBaseTrigger
{
public:
	void Spawn(void);

public:
	void EXPORT BombTargetTouch(CBaseEntity *pOther);
	void EXPORT BombTargetUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(func_bomb_target, CBombTarget);

void CBombTarget::Spawn(void)
{
	InitTrigger();

	SetTouch(&CBombTarget::BombTargetTouch);
	SetUse(&CBombTarget::BombTargetUse);
}

void CBombTarget::BombTargetTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (pPlayer->m_bHasC4 == true)
	{
		pPlayer->m_signals.Signal(SIGNAL_BOMB);
		pPlayer->m_pentCurBombTarget = ENT(pev);
	}
}

void CBombTarget::BombTargetUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SUB_UseTargets(NULL, USE_TOGGLE, 0);
}

class CHostageRescue : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT HostageRescueTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(func_hostage_rescue, CHostageRescue);

void CHostageRescue::Spawn(void)
{
	InitTrigger();
	SetTouch(&CHostageRescue::HostageRescueTouch);
}

void CHostageRescue::HostageRescueTouch(CBaseEntity *pOther)
{
	if (pOther->IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		pPlayer->m_signals.Signal(SIGNAL_RESCUE);
	}

	if (FClassnameIs(pOther->pev, "hostage_entity"))
		((CHostage *)pOther)->m_bRescueMe = TRUE;
}

class CEscapeZone : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT EscapeTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(func_escapezone, CEscapeZone);

void CEscapeZone::Spawn(void)
{
	InitTrigger();
	SetTouch(&CEscapeZone::EscapeTouch);
}

void CEscapeZone::EscapeTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (pPlayer->m_iTeam == TEAM_TERRORIST)
	{
		pPlayer->m_bEscaped = true;
		UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Terrorist_Escaped\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *client = (CBasePlayer *)UTIL_PlayerByIndex(i);

			if (!client)
				continue;

			if (FNullEnt(client->pev))
				continue;

			if (client->m_iTeam == pPlayer->m_iTeam)
				ClientPrint(client->pev, HUD_PRINTCENTER, "#Terrorist_Escaped");
		}
	}
	else if (pPlayer->m_iTeam == TEAM_CT)
		pPlayer->m_signals.Signal(SIGNAL_ESCAPE);
}

class CVIP_SafetyZone : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT VIP_SafetyTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(func_vip_safetyzone, CVIP_SafetyZone);

void CVIP_SafetyZone::Spawn(void)
{
	InitTrigger();
	SetTouch(&CVIP_SafetyZone::VIP_SafetyTouch);
}

void CVIP_SafetyZone::VIP_SafetyTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	pPlayer->m_signals.Signal(SIGNAL_VIPSAFETY);

	if (pPlayer->m_bIsVIP == true)
	{
		UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Escaped_As_VIP\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));

		pPlayer->m_bEscaped = true;
		pPlayer->Disappear();
		pPlayer->AddAccount(2500);
	}
}

class CTriggerSave : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT SaveTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(trigger_autosave, CTriggerSave);

void CTriggerSave::Spawn(void)
{
	if (g_pGameRules->IsDeathmatch())
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	InitTrigger();
	SetTouch(&CTriggerSave::SaveTouch);
}

void CTriggerSave::SaveTouch(CBaseEntity *pOther)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if (!pOther->IsPlayer())
		return;

	SetTouch(NULL);
	UTIL_Remove(this);
	SERVER_COMMAND("autosave\n");
}

#define SF_ENDSECTION_USEONLY 0x0001

class CTriggerEndSection : public CBaseTrigger
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);

public:
	void EXPORT EndSectionTouch(CBaseEntity *pOther);
	void EXPORT EndSectionUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(trigger_endsection, CTriggerEndSection);

void CTriggerEndSection::EndSectionUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pActivator && !pActivator->IsNetClient())
		return;

	SetUse(NULL);

	if (pev->message)
		g_engfuncs.pfnEndSection(STRING(pev->message));

	UTIL_Remove(this);
}

void CTriggerEndSection::Spawn(void)
{
	if (g_pGameRules->IsDeathmatch())
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	InitTrigger();
	SetUse(&CTriggerEndSection::EndSectionUse);

	if (!(pev->spawnflags & SF_ENDSECTION_USEONLY))
		SetTouch(&CTriggerEndSection::EndSectionTouch);
}

void CTriggerEndSection::EndSectionTouch(CBaseEntity *pOther)
{
	if (!pOther->IsNetClient())
		return;

	SetTouch(NULL);

	if (pev->message)
		g_engfuncs.pfnEndSection(STRING(pev->message));

	UTIL_Remove(this);
}

void CTriggerEndSection::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}

class CTriggerGravity : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT GravityTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(trigger_gravity, CTriggerGravity);

void CTriggerGravity::Spawn(void)
{
	InitTrigger();
	SetTouch(&CTriggerGravity::GravityTouch);
}

void CTriggerGravity::GravityTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	pOther->pev->gravity = pev->gravity;
}

class CTriggerChangeTarget : public CBaseDelay
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	int m_iszNewTarget;
};

LINK_ENTITY_TO_CLASS(trigger_changetarget, CTriggerChangeTarget);

TYPEDESCRIPTION CTriggerChangeTarget::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeTarget, CBaseDelay);

void CTriggerChangeTarget::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewTarget"))
	{
		m_iszNewTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerChangeTarget::Spawn(void)
{

}

void CTriggerChangeTarget::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pTarget = UTIL_FindEntityByString(NULL, "targetname", STRING(pev->target));

	if (pTarget)
	{
		pTarget->pev->target = m_iszNewTarget;
		CBaseMonster *pMonster = pTarget->MyMonsterPointer();

		if (pMonster)
			pMonster->m_pGoalEnt = NULL;
	}
}

#define SF_CAMERA_PLAYER_POSITION 1
#define SF_CAMERA_PLAYER_TARGET 2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseDelay
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

public:
	void EXPORT FollowTarget(void);
	void Move(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pentPath;
	int m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	int m_state;
};

LINK_ENTITY_TO_CLASS(trigger_camera, CTriggerCamera);

TYPEDESCRIPTION CTriggerCamera::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerCamera, m_hPlayer, FIELD_EHANDLE),
	DEFINE_FIELD(CTriggerCamera, m_hTarget, FIELD_EHANDLE),
	DEFINE_FIELD(CTriggerCamera, m_pentPath, FIELD_CLASSPTR),
	DEFINE_FIELD(CTriggerCamera, m_sPath, FIELD_STRING),
	DEFINE_FIELD(CTriggerCamera, m_flWait, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_flReturnTime, FIELD_TIME),
	DEFINE_FIELD(CTriggerCamera, m_flStopTime, FIELD_TIME),
	DEFINE_FIELD(CTriggerCamera, m_moveDistance, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_targetSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_initialSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_acceleration, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_deceleration, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerCamera, m_state, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerCamera, CBaseDelay);

void CTriggerCamera::Spawn(void)
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;
	m_initialSpeed = pev->speed;

	if (!m_acceleration)
		m_acceleration = 500;

	if (!m_deceleration)
		m_deceleration = 500;
}

void CTriggerCamera::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "moveto"))
	{
		m_sPath = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
	{
		m_acceleration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "deceleration"))
	{
		m_deceleration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CTriggerCamera::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_state))
		return;

	m_state = !m_state;

	if (!m_state)
	{
		m_flReturnTime = gpGlobals->time;

		if (pActivator&&pActivator->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
			pPlayer->ResetMaxSpeed();
		}

		return;
	}

	if (!pActivator || !pActivator->IsPlayer())
		pActivator = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));

	m_hPlayer = pActivator;
	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TARGET))
		m_hTarget = m_hPlayer;
	else
		m_hTarget = GetNextTarget();

	if (m_hTarget == NULL)
		return;

	if (pActivator->IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
		g_engfuncs.pfnSetClientMaxspeed(pActivator->edict(), 0.001);
	}

	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL))
		((CBasePlayer *)pActivator)->EnableControl(FALSE);

	if (m_sPath)
		m_pentPath = Instance(FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_sPath)));
	else
		m_pentPath = NULL;

	m_flStopTime = gpGlobals->time;

	if (m_pentPath)
	{
		if (m_pentPath->pev->speed != 0)
			m_targetSpeed = m_pentPath->pev->speed;

		m_flStopTime += m_pentPath->GetDelay();
	}

	if (FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_POSITION))
	{
		UTIL_SetOrigin(pev, pActivator->pev->origin + pActivator->pev->view_ofs);
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
		pev->velocity = Vector(0, 0, 0);

	SET_VIEW(pActivator->edict(), edict());
	SET_MODEL(ENT(pev), STRING(pActivator->pev->model));

	SetThink(&CTriggerCamera::FollowTarget);
	pev->nextthink = gpGlobals->time;
	m_moveDistance = 0;
	Move();
}

void CTriggerCamera::FollowTarget(void)
{
	if (m_hPlayer == NULL)
		return;

	if (m_hTarget == NULL || m_flReturnTime < gpGlobals->time)
	{
		if (m_hPlayer->IsAlive())
		{
			SET_VIEW(m_hPlayer->edict(), m_hPlayer->edict());
			((CBasePlayer *)((CBaseEntity *)m_hPlayer))->EnableControl(TRUE);
			((CBasePlayer *)((CBaseEntity *)m_hPlayer))->ResetMaxSpeed();
		}

		SUB_UseTargets(this, USE_TOGGLE, 0);
		pev->avelocity = Vector(0, 0, 0);
		m_state = 0;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles(m_hTarget->pev->origin - pev->origin);
	vecGoal.x = -vecGoal.x;

	if (pev->angles.y > 360)
		pev->angles.y -= 360;

	if (pev->angles.y < 0)
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if (dx < -180)
		dx += 360;

	if (dx > 180)
		dx = dx - 360;

	if (dy < -180)
		dy += 360;

	if (dy > 180)
		dy = dy - 360;

	pev->avelocity.x = dx * 40 * gpGlobals->frametime;
	pev->avelocity.y = dy * 40 * gpGlobals->frametime;

	if (!(FBitSet(pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL)))
	{
		pev->velocity = pev->velocity * 0.8;

		if (pev->velocity.Length() < 10)
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;
	Move();
}

void CTriggerCamera::Move(void)
{
	if (!m_pentPath)
		return;

	m_moveDistance -= pev->speed * gpGlobals->frametime;

	if (m_moveDistance <= 0)
	{
		if (m_pentPath->pev->message)
		{
			FireTargets(STRING(m_pentPath->pev->message), this, this, USE_TOGGLE, 0);

			if (FBitSet(m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE))
				m_pentPath->pev->message = 0;
		}

		m_pentPath = m_pentPath->GetNextTarget();

		if (!m_pentPath)
		{
			pev->velocity = g_vecZero;
		}
		else
		{
			if (m_pentPath->pev->speed)
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if (m_flStopTime > gpGlobals->time)
		pev->speed = UTIL_Approach(0, pev->speed, m_deceleration * gpGlobals->frametime);
	else
		pev->speed = UTIL_Approach(m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime);

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ((pev->movedir * pev->speed) * fraction) + (pev->velocity * (1 - fraction));
}

class CWeather : public CBaseTrigger
{
public:
	void Spawn(void) { InitTrigger(); }
};

LINK_ENTITY_TO_CLASS(env_snow, CWeather);
LINK_ENTITY_TO_CLASS(func_snow, CWeather);
LINK_ENTITY_TO_CLASS(env_rain, CWeather);
LINK_ENTITY_TO_CLASS(func_rain, CWeather);

void CClientFog::Spawn(void)
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderNormal;
}

void CClientFog::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_fDensity = atof(pkvd->szValue);

		if (m_fDensity < 0 || m_fDensity > 0.01)
			m_fDensity = 0;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(env_fog, CClientFog);
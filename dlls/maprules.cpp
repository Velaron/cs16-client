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
#include "eiface.h"
#include "gamerules.h"
#include "maprules.h"
#include "cbase.h"
#include "player.h"
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

class CRuleEntity : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void SetMaster(int iszMaster) { m_iszMaster = iszMaster; }

protected:
	BOOL CanFireForActivator(CBaseEntity *pActivator);

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_iszMaster;
};

TYPEDESCRIPTION CRuleEntity::m_SaveData[] =
{
	DEFINE_FIELD(CRuleEntity, m_iszMaster, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CRuleEntity, CBaseEntity);

void CRuleEntity::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
}

void CRuleEntity::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "master"))
	{
		SetMaster(ALLOC_STRING(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

BOOL CRuleEntity::CanFireForActivator(CBaseEntity *pActivator)
{
	if (m_iszMaster)
	{
		if (UTIL_IsMasterTriggered(m_iszMaster, pActivator))
			return TRUE;
		else
			return FALSE;
	}

	return TRUE;
}

class CRulePointEntity : public CRuleEntity
{
public:
	void Spawn(void);
};

void CRulePointEntity::Spawn(void)
{
	CRuleEntity::Spawn();
	pev->frame = 0;
	pev->model = 0;
}

class CRuleBrushEntity : public CRuleEntity
{
public:
	void Spawn(void);
};

void CRuleBrushEntity::Spawn(void)
{
	SET_MODEL(edict(), STRING(pev->model));
	CRuleEntity::Spawn();
}

#define SF_SCORE_NEGATIVE 0x0001
#define SF_SCORE_TEAM 0x0002

class CGameScore : public CRulePointEntity
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void KeyValue(KeyValueData *pkvd);

public:
	inline int Points(void) { return (int)(pev->frags); }
	inline BOOL AllowNegativeScore(void) { return pev->spawnflags & SF_SCORE_NEGATIVE; }
	inline BOOL AwardToTeam(void) { return pev->spawnflags & SF_SCORE_TEAM; }
	inline void SetPoints(int points) { pev->frags = points; }
};

LINK_ENTITY_TO_CLASS(game_score, CGameScore);

void CGameScore::Spawn(void)
{
	CRulePointEntity::Spawn();
}

void CGameScore::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "points"))
	{
		SetPoints(atoi(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue(pkvd);
}

void CGameScore::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	if (pActivator->IsPlayer())
	{
		if (AwardToTeam())
			pActivator->AddPointsToTeam(Points(), AllowNegativeScore());
		else
			pActivator->AddPoints(Points(), AllowNegativeScore());
	}
}

class CGameEnd : public CRulePointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(game_end, CGameEnd);

void CGameEnd::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	g_pGameRules->EndMultiplayerGame();
}

#define SF_ENVTEXT_ALLPLAYERS 0x0001

class CGameText : public CRulePointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	inline BOOL MessageToAll(void) { return (pev->spawnflags & SF_ENVTEXT_ALLPLAYERS); }
	inline void MessageSet(const char *pMessage) { pev->message = ALLOC_STRING(pMessage); }
	inline const char *MessageGet(void) { return STRING(pev->message); }

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	hudtextparms_t m_textParms;
};

LINK_ENTITY_TO_CLASS(game_text, CGameText);

TYPEDESCRIPTION CGameText::m_SaveData[] =
{
	DEFINE_ARRAY(CGameText, m_textParms, FIELD_CHARACTER, sizeof(hudtextparms_t)),
};

IMPLEMENT_SAVERESTORE(CGameText, CRulePointEntity);

void CGameText::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "channel"))
	{
		m_textParms.channel = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "x"))
	{
		m_textParms.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "y"))
	{
		m_textParms.y = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "effect"))
	{
		m_textParms.effect = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "color"))
	{
		int color[4];
		UTIL_StringToIntArray(color, 4, pkvd->szValue);
		m_textParms.r1 = color[0];
		m_textParms.g1 = color[1];
		m_textParms.b1 = color[2];
		m_textParms.a1 = color[3];
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "color2"))
	{
		int color[4];
		UTIL_StringToIntArray(color, 4, pkvd->szValue);
		m_textParms.r2 = color[0];
		m_textParms.g2 = color[1];
		m_textParms.b2 = color[2];
		m_textParms.a2 = color[3];
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_textParms.fadeinTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_textParms.fadeoutTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		m_textParms.holdTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fxtime"))
	{
		m_textParms.fxTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CRulePointEntity::KeyValue(pkvd);
}

void CGameText::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	if (MessageToAll())
	{
		UTIL_HudMessageAll(m_textParms, MessageGet());
	}
	else
	{
		if (pActivator->IsNetClient())
			UTIL_HudMessage(pActivator, m_textParms, MessageGet());
	}
}

#define SF_TEAMMASTER_FIREONCE 0x0001
#define SF_TEAMMASTER_ANYTEAM 0x0002

class CGameTeamMaster : public CRulePointEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return CRulePointEntity:: ObjectCaps() | FCAP_MASTER; }
	BOOL IsTriggered(CBaseEntity *pActivator);
	const char *TeamID(void);

public:
	inline BOOL RemoveOnFire(void) { return (pev->spawnflags & SF_TEAMMASTER_FIREONCE) ? TRUE : FALSE; }
	inline BOOL AnyTeam(void) { return (pev->spawnflags & SF_TEAMMASTER_ANYTEAM) ? TRUE : FALSE; }

private:
	BOOL TeamMatch(CBaseEntity *pActivator);

public:
	int m_teamIndex;
	USE_TYPE triggerType;
};

LINK_ENTITY_TO_CLASS(game_team_master, CGameTeamMaster);

void CGameTeamMaster::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "teamindex"))
	{
		m_teamIndex = atoi(pkvd->szValue);
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
		CRulePointEntity::KeyValue(pkvd);
}

void CGameTeamMaster::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	if (useType == USE_SET)
	{
		if (value < 0)
			m_teamIndex = -1;
		else
			m_teamIndex = g_pGameRules->GetTeamIndex(pActivator->TeamID());

		return;
	}

	if (TeamMatch(pActivator))
	{
		SUB_UseTargets(pActivator, triggerType, value);

		if (RemoveOnFire())
			UTIL_Remove(this);
	}
}

BOOL CGameTeamMaster::IsTriggered(CBaseEntity *pActivator)
{
	return TeamMatch(pActivator);
}

const char *CGameTeamMaster::TeamID(void)
{
	if (m_teamIndex < 0)
		return "";

	return g_pGameRules->GetIndexedTeamName(m_teamIndex);
}

BOOL CGameTeamMaster::TeamMatch(CBaseEntity *pActivator)
{
	if (m_teamIndex < 0 && AnyTeam())
		return TRUE;

	if (!pActivator)
		return FALSE;

	return UTIL_TeamsMatch(pActivator->TeamID(), TeamID());
}

#define SF_TEAMSET_FIREONCE 0x0001
#define SF_TEAMSET_CLEARTEAM 0x0002

class CGameTeamSet : public CRulePointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	inline BOOL RemoveOnFire(void) { return (pev->spawnflags & SF_TEAMSET_FIREONCE) ? TRUE : FALSE; }
	inline BOOL ShouldClearTeam(void) { return (pev->spawnflags & SF_TEAMSET_CLEARTEAM) ? TRUE : FALSE; }
};

LINK_ENTITY_TO_CLASS(game_team_set, CGameTeamSet);

void CGameTeamSet::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	if (ShouldClearTeam())
		SUB_UseTargets(pActivator, USE_SET, -1);
	else
		SUB_UseTargets(pActivator, USE_SET, 0);

	if (RemoveOnFire())
		UTIL_Remove(this);
}

class CGamePlayerZone : public CRuleBrushEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	string_t m_iszInTarget;
	string_t m_iszOutTarget;
	string_t m_iszInCount;
	string_t m_iszOutCount;
};

LINK_ENTITY_TO_CLASS(game_zone_player, CGamePlayerZone);

TYPEDESCRIPTION CGamePlayerZone::m_SaveData[] =
{
	DEFINE_FIELD(CGamePlayerZone, m_iszInTarget, FIELD_STRING),
	DEFINE_FIELD(CGamePlayerZone, m_iszOutTarget, FIELD_STRING),
	DEFINE_FIELD(CGamePlayerZone, m_iszInCount, FIELD_STRING),
	DEFINE_FIELD(CGamePlayerZone, m_iszOutCount, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CGamePlayerZone, CRuleBrushEntity);

void CGamePlayerZone::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "intarget"))
	{
		m_iszInTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "outtarget"))
	{
		m_iszOutTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "incount"))
	{
		m_iszInCount = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "outcount"))
	{
		m_iszOutCount = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CRuleBrushEntity::KeyValue(pkvd);
}

void CGamePlayerZone::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int playersInCount = 0;
	int playersOutCount = 0;

	if (!CanFireForActivator(pActivator))
		return;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex(i);

		if (pPlayer)
		{
			int hullNumber = human_hull;

			if (pPlayer->pev->flags & FL_DUCKING)
				hullNumber = head_hull;

			TraceResult trace;
			UTIL_TraceModel(pPlayer->pev->origin, pPlayer->pev->origin, hullNumber, edict(), &trace);

			if (trace.fStartSolid)
			{
				playersInCount++;

				if (m_iszInTarget)
					FireTargets(STRING(m_iszInTarget), pPlayer, pActivator, useType, value);
			}
			else
			{
				playersOutCount++;

				if (m_iszOutTarget)
					FireTargets(STRING(m_iszOutTarget), pPlayer, pActivator, useType, value);
			}
		}
	}

	if (m_iszInCount)
		FireTargets(STRING(m_iszInCount), pActivator, this, USE_SET, playersInCount);

	if (m_iszOutCount)
		FireTargets(STRING(m_iszOutCount), pActivator, this, USE_SET, playersOutCount);
}

#define SF_PKILL_FIREONCE 0x0001

class CGamePlayerHurt : public CRulePointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	inline BOOL RemoveOnFire(void) { return (pev->spawnflags & SF_PKILL_FIREONCE) ? TRUE : FALSE; }
};

LINK_ENTITY_TO_CLASS(game_player_hurt, CGamePlayerHurt);

void CGamePlayerHurt::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	if (pActivator->IsPlayer())
	{
		if (pev->dmg < 0)
			pActivator->TakeHealth(-pev->dmg, DMG_GENERIC);
		else
			pActivator->TakeDamage(pev, pev, pev->dmg, DMG_GENERIC);
	}

	SUB_UseTargets(pActivator, useType, value);

	if (RemoveOnFire())
		UTIL_Remove(this);
}

#define SF_GAMECOUNT_FIREONCE 0x0001
#define SF_GAMECOUNT_RESET 0x0002

class CGameCounter : public CRulePointEntity
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

public:
	inline BOOL RemoveOnFire(void) { return (pev->spawnflags & SF_GAMECOUNT_FIREONCE) ? TRUE : FALSE; }
	inline BOOL ResetOnFire(void) { return (pev->spawnflags & SF_GAMECOUNT_RESET) ? TRUE : FALSE; }
	inline void CountUp(void) { pev->frags++; }
	inline void CountDown(void) { pev->frags--; }
	inline void ResetCount(void) { pev->frags = pev->dmg; }
	inline int CountValue(void) { return (int)(pev->frags); }
	inline int LimitValue(void) { return (int)(pev->health); }
	inline BOOL HitLimit(void) { return CountValue() == LimitValue(); }

private:
	inline void SetCountValue(int value) { pev->frags = value; }
	inline void SetInitialValue(int value) { pev->dmg = value; }
};

LINK_ENTITY_TO_CLASS(game_counter, CGameCounter);

void CGameCounter::Spawn(void)
{
	SetInitialValue(CountValue());
	CRulePointEntity::Spawn();
}

void CGameCounter::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	switch (useType)
	{
		case USE_ON:
		case USE_TOGGLE: CountUp(); break;
		case USE_OFF: CountDown(); break;
		case USE_SET: SetCountValue((int)value); break;
	}

	if (HitLimit())
	{
		SUB_UseTargets(pActivator, USE_TOGGLE, 0);

		if (RemoveOnFire())
			UTIL_Remove(this);

		if (ResetOnFire())
			ResetCount();
	}
}

#define SF_GAMECOUNTSET_FIREONCE 0x0001

class CGameCounterSet : public CRulePointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	inline BOOL RemoveOnFire(void) { return (pev->spawnflags & SF_GAMECOUNTSET_FIREONCE) ? TRUE : FALSE; }
};

LINK_ENTITY_TO_CLASS(game_counter_set, CGameCounterSet);

void CGameCounterSet::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	SUB_UseTargets(pActivator, USE_SET, pev->frags);

	if (RemoveOnFire())
		UTIL_Remove(this);
}

#define SF_PLAYEREQUIP_USEONLY 0x0001
#define MAX_EQUIP 32

class CGamePlayerEquip : public CRulePointEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Touch(CBaseEntity *pOther);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

public:
	inline BOOL UseOnly(void) { return (pev->spawnflags & SF_PLAYEREQUIP_USEONLY) ? TRUE : FALSE; }

private:
	void EquipPlayer(CBaseEntity *pPlayer);

public:
	string_t m_weaponNames[MAX_EQUIP];
	int m_weaponCount[MAX_EQUIP];
};

LINK_ENTITY_TO_CLASS(game_player_equip, CGamePlayerEquip);

void CGamePlayerEquip::KeyValue(KeyValueData *pkvd)
{
	CRulePointEntity::KeyValue(pkvd);

	if (!pkvd->fHandled)
	{
		for (int i = 0; i < MAX_EQUIP; i++)
		{
			if (!m_weaponNames[i])
			{
				char tmp[128];
				UTIL_StripToken(pkvd->szKeyName, tmp);

				m_weaponNames[i] = ALLOC_STRING(tmp);
				m_weaponCount[i] = atoi(pkvd->szValue);
				m_weaponCount[i] = max(1, m_weaponCount[i]);
				pkvd->fHandled = TRUE;
				break;
			}
		}
	}
}

void CGamePlayerEquip::Touch(CBaseEntity *pOther)
{
	if (!CanFireForActivator(pOther))
		return;

	if (UseOnly())
		return;

	EquipPlayer(pOther);
}

void CGamePlayerEquip::EquipPlayer(CBaseEntity *pEntity)
{
	CBasePlayer *pPlayer = NULL;

	if (pEntity->IsPlayer())
		pPlayer = (CBasePlayer *)pEntity;

	if (!pPlayer)
		return;

	for (int i = 0; i < MAX_EQUIP; i++)
	{
		if (!m_weaponNames[i])
			break;

		for (int j = 0; j < m_weaponCount[i]; j++)
			pPlayer->GiveNamedItem(STRING(m_weaponNames[i]));
	}
}

void CGamePlayerEquip::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	EquipPlayer(pActivator);
}

#define SF_PTEAM_FIREONCE 0x0001
#define SF_PTEAM_KILL 0x0002
#define SF_PTEAM_GIB 0x0004

class CGamePlayerTeam : public CRulePointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
	inline BOOL RemoveOnFire(void) { return (pev->spawnflags & SF_PTEAM_FIREONCE) ? TRUE : FALSE; }
	inline BOOL ShouldKillPlayer(void) { return (pev->spawnflags & SF_PTEAM_KILL) ? TRUE : FALSE; }
	inline BOOL ShouldGibPlayer(void) { return (pev->spawnflags & SF_PTEAM_GIB) ? TRUE : FALSE; }
	const char *TargetTeamName(const char *pszTargetName);
};

LINK_ENTITY_TO_CLASS(game_player_team, CGamePlayerTeam);

const char *CGamePlayerTeam::TargetTeamName(const char *pszTargetName)
{
	CBaseEntity *pTeamEntity = NULL;

	while ((pTeamEntity = UTIL_FindEntityByTargetname(pTeamEntity, pszTargetName)) != NULL)
	{
		if (FClassnameIs(pTeamEntity->pev, "game_team_master"))
			return pTeamEntity->TeamID();
	}

	return NULL;
}

void CGamePlayerTeam::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CanFireForActivator(pActivator))
		return;

	if (pActivator->IsPlayer())
	{
		const char *pszTargetTeam = TargetTeamName(STRING(pev->target));

		if (pszTargetTeam)
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pActivator;
			g_pGameRules->ChangePlayerTeam(pPlayer, pszTargetTeam, ShouldKillPlayer(), ShouldGibPlayer());
		}
	}

	if (RemoveOnFire())
		UTIL_Remove(this);
}

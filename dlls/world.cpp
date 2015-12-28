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
#include "nodes.h"
#include "soundent.h"
#include "client.h"
#include "decals.h"
#include "skill.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

extern CGraph WorldGraph;
extern CSoundEnt *pSoundEnt;
extern CBaseEntity *g_pLastSpawn;
extern CBaseEntity *g_pLastCTSpawn, *g_pLastTerroristSpawn;
extern char g_szMapBriefingText[512];

DLL_GLOBAL edict_t *g_pBodyQueueHead;
CGlobalState gGlobalState;

extern DLL_GLOBAL int gDisplayTitle;
extern void W_Precache(void);

DLL_DECALLIST gDecals[] =
{
	{ "{shot1", 0 },
	{ "{shot2", 0 },
	{ "{shot3",0 },
	{ "{shot4", 0 },
	{ "{shot5", 0 },
	{ "{lambda01", 0 },
	{ "{lambda02", 0 },
	{ "{lambda03", 0 },
	{ "{lambda04", 0 },
	{ "{lambda05", 0 },
	{ "{lambda06", 0 },
	{ "{scorch1", 0 },
	{ "{scorch2", 0 },
	{ "{blood1", 0 },
	{ "{blood2", 0 },
	{ "{blood3", 0 },
	{ "{blood4", 0 },
	{ "{blood5", 0 },
	{ "{blood6", 0 },
	{ "{yblood1", 0 },
	{ "{yblood2", 0 },
	{ "{yblood3", 0 },
	{ "{yblood4", 0 },
	{ "{yblood5", 0 },
	{ "{yblood6", 0 },
	{ "{break1", 0 },
	{ "{break2", 0 },
	{ "{break3", 0 },
	{ "{bigshot1", 0 },
	{ "{bigshot2", 0 },
	{ "{bigshot3", 0 },
	{ "{bigshot4", 0 },
	{ "{bigshot5", 0 },
	{ "{spit1", 0 },
	{ "{spit2", 0 },
	{ "{bproof1", 0 },
	{ "{gargstomp", 0 },
	{ "{smscorch1", 0 },
	{ "{smscorch2", 0 },
	{ "{smscorch3", 0 },
	{ "{mommablob", 0 },
	{ "{mommablob", 0 }
};

#define SF_DECAL_NOTINDEATHMATCH 2048

class CDecal : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);

public:
	void EXPORT StaticDecal(void);
	void EXPORT TriggerDecal(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(infodecal, CDecal);

void CDecal::Spawn(void)
{
	if (pev->skin < 0 || (gpGlobals->deathmatch && FBitSet(pev->spawnflags, SF_DECAL_NOTINDEATHMATCH)))
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (FStringNull(pev->targetname))
	{
		SetThink(&CDecal::StaticDecal);
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		SetThink(&CBaseEntity::SUB_DoNothing);
		SetUse(&CDecal::TriggerDecal);
	}
}

void CDecal::TriggerDecal(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	TraceResult trace;
	UTIL_TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), ignore_monsters, ENT(pev), &trace);

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BSPDECAL);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT((int)pev->skin);

	int entityIndex = (short)ENTINDEX(trace.pHit);

	WRITE_SHORT(entityIndex);

	if (entityIndex)
		WRITE_SHORT((int)VARS(trace.pHit)->modelindex);

	MESSAGE_END();

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CDecal::StaticDecal(void)
{
	TraceResult trace;
	UTIL_TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), ignore_monsters, ENT(pev), &trace);

	int entityIndex = (short)ENTINDEX(trace.pHit);
	int modelIndex = entityIndex ? (int)VARS(trace.pHit)->modelindex : 0;

	g_engfuncs.pfnStaticDecal(pev->origin, (int)pev->skin, entityIndex, modelIndex);
	SUB_Remove();
}

void CDecal::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->skin = DECAL_INDEX(pkvd->szValue);

		if (pev->skin >= 0)
			return;

		ALERT(at_console, "Can't find decal %s\n", pkvd->szValue);
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

#define MAX_BODYQUES 32

class CCorpse : public CBaseEntity
{
public:
	int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS(bodyque, CCorpse);

static void InitBodyQue(void)
{
	string_t istrClassname = MAKE_STRING("bodyque");
	g_pBodyQueueHead = CREATE_NAMED_ENTITY(istrClassname);
	entvars_t *pev = VARS(g_pBodyQueueHead);

	for (int i = 0; i <= MAX_BODYQUES; i++)
	{
		pev->owner = CREATE_NAMED_ENTITY(istrClassname);
		pev = VARS(pev->owner);
	}

	pev->owner = g_pBodyQueueHead;
}

void CopyToBodyQue(entvars_t *pev)
{
	if (pev->effects & EF_NODRAW)
		return;

	entvars_t *pevHead = VARS(g_pBodyQueueHead);
	pevHead->angles = pev->angles;
	pevHead->model = pev->model;
	pevHead->modelindex = pev->modelindex;
	pevHead->frame = pev->frame;
	pevHead->colormap = pev->colormap;
	pevHead->movetype = MOVETYPE_TOSS;
	pevHead->velocity = pev->velocity;
	pevHead->flags = 0;
	pevHead->deadflag = pev->deadflag;
	pevHead->renderfx = kRenderFxDeadPlayer;
	pevHead->renderamt = ENTINDEX(ENT(pev));
	pevHead->effects = pev->effects | EF_NOINTERP;
	pevHead->sequence = pev->sequence;
	pevHead->animtime = pev->animtime;

	UTIL_SetOrigin(pevHead, pev->origin);
	UTIL_SetSize(pevHead, pev->mins, pev->maxs);
	g_pBodyQueueHead = pevHead->owner;
}

void ClearBodyQue(void)
{
	entvars_t *pevHead = VARS(g_pBodyQueueHead);
	entvars_t *pev = pevHead;

	while (pev)
	{
		pev->model = 0;
		pev->modelindex = 0;
		pev = VARS(pev->owner);

		if (pev == pevHead)
			break;
	}
}

CGlobalState::CGlobalState(void)
{
	Reset();
}

void CGlobalState::Reset(void)
{
	m_pList = NULL;
	m_listCount = 0;
}

globalentity_t *CGlobalState::Find(string_t globalname)
{
	if (!globalname)
		return NULL;

	globalentity_t *pTest = m_pList;
	const char *pEntityName = STRING(globalname);

	while (pTest)
	{
		if (!strcmp(pEntityName, pTest->name))
			break;

		pTest = pTest->pNext;
	}

	return pTest;
}

void CGlobalState::DumpGlobals(void)
{
	static char *estates[] = { "Off", "On", "Dead" };
	globalentity_t *pTest;

	ALERT(at_console, "-- Globals --\n");
	pTest = m_pList;

	while (pTest)
	{
		ALERT(at_console, "%s: %s (%s)\n", pTest->name, pTest->levelName, estates[pTest->state]);
		pTest = pTest->pNext;
	}
}

void CGlobalState::EntityAdd(string_t globalname, string_t mapName, GLOBALESTATE state)
{
	ASSERT(!Find(globalname));

	globalentity_t *pNewEntity = (globalentity_t *)calloc(sizeof(globalentity_t), 1);
	ASSERT(pNewEntity != NULL);
	pNewEntity->pNext = m_pList;
	m_pList = pNewEntity;
	strcpy(pNewEntity->name, STRING(globalname));
	strcpy(pNewEntity->levelName, STRING(mapName));
	pNewEntity->state = state;
	m_listCount++;
}

void CGlobalState::EntitySetState(string_t globalname, GLOBALESTATE state)
{
	globalentity_t *pEnt = Find(globalname);

	if (pEnt)
		pEnt->state = state;
}

const globalentity_t *CGlobalState::EntityFromTable(string_t globalname)
{
	return Find(globalname);
}

GLOBALESTATE CGlobalState::EntityGetState(string_t globalname)
{
	globalentity_t *pEnt = Find(globalname);

	if (pEnt)
		return pEnt->state;

	return GLOBAL_OFF;
}

TYPEDESCRIPTION CGlobalState::m_SaveData[] =
{
	DEFINE_FIELD(CGlobalState, m_listCount, FIELD_INTEGER)
};

TYPEDESCRIPTION gGlobalEntitySaveData[] =
{
	DEFINE_ARRAY(globalentity_t, name, FIELD_CHARACTER, 64),
	DEFINE_ARRAY(globalentity_t, levelName, FIELD_CHARACTER, 32),
	DEFINE_FIELD(globalentity_t, state, FIELD_INTEGER)
};

int CGlobalState::Save(CSave &save)
{
	if (!save.WriteFields("GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData)))
		return 0;

	globalentity_t *pEntity = m_pList;

	for (int i = 0; i < m_listCount && pEntity; i++)
	{
		if (!save.WriteFields("GENT", pEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData)))
			return 0;

		pEntity = pEntity->pNext;
	}

	return 1;
}

int CGlobalState::Restore(CRestore &restore)
{
	ClearStates();

	if (!restore.ReadFields("GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData)))
		return 0;

	int listCount = m_listCount;
	m_listCount = 0;

	for (int i = 0; i < listCount; i++)
	{
		globalentity_t tmpEntity;

		if (!restore.ReadFields("GENT", &tmpEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData)))
			return 0;

		EntityAdd(MAKE_STRING(tmpEntity.name), MAKE_STRING(tmpEntity.levelName), tmpEntity.state);
	}

	return 1;
}

void CGlobalState::EntityUpdate(string_t globalname, string_t mapname)
{
	globalentity_t *pEnt = Find(globalname);

	if (pEnt)
		strcpy(pEnt->levelName, STRING(mapname));
}

void CGlobalState::ClearStates(void)
{
	globalentity_t *pFree = m_pList;

	while (pFree)
	{
		globalentity_t *pNext = pFree->pNext;
		free(pFree);
		pFree = pNext;
	}

	Reset();
}

void SaveGlobalState(SAVERESTOREDATA *pSaveData)
{
	CSave saveHelper(pSaveData);
	gGlobalState.Save(saveHelper);
}

void RestoreGlobalState(SAVERESTOREDATA *pSaveData)
{
	CRestore restoreHelper(pSaveData);
	gGlobalState.Restore(restoreHelper);
}

void ResetGlobalState(void)
{
	gGlobalState.ClearStates();
	gInitHUD = TRUE;
}

LINK_ENTITY_TO_CLASS(worldspawn, CWorld);

#define SF_WORLD_DARK 0x0001
#define SF_WORLD_TITLE 0x0002
#define SF_WORLD_FORCETEAM 0x0004

extern DLL_GLOBAL BOOL g_fGameOver;
float g_flWeaponCheat;

void CWorld::Spawn(void)
{
	EmptyEntityHashTable();
	g_fGameOver = FALSE;
	Precache();
	g_flWeaponCheat = CVAR_GET_FLOAT("sv_cheats");
	g_szMapBriefingText[0] = '\0';

	int len = 0;
	char *buf = (char *)LOAD_FILE_FOR_ME(UTIL_VarArgs("maps/%s.txt", STRING(gpGlobals->mapname)), &len);

	if (buf && len)
	{
		strncpy(g_szMapBriefingText, buf, sizeof(g_szMapBriefingText) - 2);
		PRECACHE_GENERIC(UTIL_VarArgs("maps/%s.txt", STRING(gpGlobals->mapname)));
		FREE_FILE(buf);
	}
	else
	{
		buf = (char *)LOAD_FILE_FOR_ME(UTIL_VarArgs("maps/default.txt"), &len);

		if (buf && len)
		{
			strncpy(g_szMapBriefingText, buf, sizeof(g_szMapBriefingText) - 2);
			PRECACHE_GENERIC(UTIL_VarArgs("maps/default.txt"));
		}

		FREE_FILE(buf);
	}
}

void CWorld::Precache(void)
{
	g_pLastSpawn = NULL;
	g_pLastTerroristSpawn = g_pLastCTSpawn = NULL;

	CVAR_SET_STRING("sv_gravity", "800");
	CVAR_SET_STRING("sv_maxspeed", "900");
	CVAR_SET_STRING("sv_stepsize", "18");
	CVAR_SET_STRING("room_type", "0");

	if (g_pGameRules)
		delete g_pGameRules;

	g_pGameRules = (CHalfLifeMultiplay *)InstallGameRules();
	pSoundEnt = GetClassPtr((CSoundEnt *)NULL);
	if (!pSoundEnt)
		ALERT(at_console, "**COULD NOT CREATE SOUNDENT**\n");

	pSoundEnt->Spawn();
	InitBodyQue();
	SENTENCEG_Init();
	TEXTURETYPE_Init();
	W_Precache();
	ClientPrecache();

	PRECACHE_SOUND("common/null.wav");
	PRECACHE_SOUND("items/suitchargeok1.wav");
	PRECACHE_SOUND("items/gunpickup2.wav");
	PRECACHE_SOUND("common/bodydrop3.wav");
	PRECACHE_SOUND("common/bodydrop4.wav");

	g_Language = (int)CVAR_GET_FLOAT("sv_language");

	if (g_Language != LANGUAGE_GERMAN)
	{
		PRECACHE_MODEL("models/hgibs.mdl");
		PRECACHE_MODEL("models/agibs.mdl");
	}
	else
		PRECACHE_MODEL("models/germangibs.mdl");

	PRECACHE_SOUND("weapons/ric1.wav");
	PRECACHE_SOUND("weapons/ric2.wav");
	PRECACHE_SOUND("weapons/ric3.wav");
	PRECACHE_SOUND("weapons/ric4.wav");
	PRECACHE_SOUND("weapons/ric5.wav");
	PRECACHE_SOUND("weapons/ric_metal-1.wav");
	PRECACHE_SOUND("weapons/ric_metal-2.wav");
	PRECACHE_SOUND("weapons/ric_conc-1.wav");
	PRECACHE_SOUND("weapons/ric_conc-2.wav");

	LIGHT_STYLE(0, "m");
	LIGHT_STYLE(1, "mmnmmommommnonmmonqnmmo");
	LIGHT_STYLE(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	LIGHT_STYLE(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	LIGHT_STYLE(4, "mamamamamama");
	LIGHT_STYLE(5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	LIGHT_STYLE(6, "nmonqnmomnmomomno");
	LIGHT_STYLE(7, "mmmaaaabcdefgmmmmaaaammmaamm");
	LIGHT_STYLE(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	LIGHT_STYLE(9, "aaaaaaaazzzzzzzz");
	LIGHT_STYLE(10, "mmamammmmammamamaaamammma");
	LIGHT_STYLE(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	LIGHT_STYLE(12, "mmnnmmnnnmmnn");
	LIGHT_STYLE(63, "a");

	for (int i = 0; i < ARRAYSIZE(gDecals); i++)
		gDecals[i].index = DECAL_INDEX(gDecals[i].name);

	WorldGraph.InitGraph();

	if (WorldGraph.CheckNODFile((char *)STRING(gpGlobals->mapname)))
	{
		if (!WorldGraph.FLoadGraph((char *)STRING(gpGlobals->mapname)))
		{
			ALERT(at_console, "*Error opening .NOD file\n");
			WorldGraph.AllocNodes();
		}
		else
			ALERT(at_console, "\n*Graph Loaded!\n");
	}
	else
		WorldGraph.AllocNodes();

	if (pev->speed > 0)
		CVAR_SET_FLOAT("sv_zmax", pev->speed);
	else
		CVAR_SET_FLOAT("sv_zmax", 4096);

	if (pev->netname)
	{
		ALERT(at_aiconsole, "Chapter title: %s\n", STRING(pev->netname));
		CBaseEntity *pEntity = CBaseEntity::Create("env_message", g_vecZero, g_vecZero, NULL);

		if (pEntity)
		{
			pEntity->SetThink(&CBaseEntity::SUB_CallUseToggle);
			pEntity->pev->message = pev->netname;
			pev->netname = 0;
			pEntity->pev->nextthink = gpGlobals->time + 0.3;
			pEntity->pev->spawnflags = SF_MESSAGE_ONCE;
		}
	}

	if (pev->spawnflags & SF_WORLD_DARK)
		CVAR_SET_FLOAT("v_dark", 1);
	else
		CVAR_SET_FLOAT("v_dark", 0);

	if (pev->spawnflags & SF_WORLD_TITLE)
		gDisplayTitle = TRUE;
	else
		gDisplayTitle = FALSE;
}

void CWorld::KeyValue(KeyValueData *pkvd)
{
	if (!strcmp(pkvd->szKeyName, "skyname"))
	{
		CVAR_SET_STRING("sv_skyname", pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "sounds"))
	{
		gpGlobals->cdAudioTrack = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "WaveHeight"))
	{
		pev->scale = atof(pkvd->szValue) * (1.0 / 8);
		pkvd->fHandled = TRUE;
		CVAR_SET_FLOAT("sv_wateramp", pev->scale);
	}
	else if (!strcmp(pkvd->szKeyName, "MaxRange"))
	{
		pev->speed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "chaptertitle"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "startdark"))
	{
		int flag = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

		if (flag)
			pev->spawnflags |= SF_WORLD_DARK;
	}
	else if (!strcmp(pkvd->szKeyName, "newunit"))
	{
		if (atoi(pkvd->szValue))
			CVAR_SET_FLOAT("sv_newunit", 1);

		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "gametitle"))
	{
		if (atoi(pkvd->szValue))
			pev->spawnflags |= SF_WORLD_TITLE;

		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "mapteams"))
	{
		pev->team = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "defaultteam"))
	{
		if (atoi(pkvd->szValue))
			pev->spawnflags |= SF_WORLD_FORCETEAM;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}
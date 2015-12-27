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
#include "client.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "MemPool.h"

void EntvarsKeyvalue(entvars_t *pev, KeyValueData *pkvd);

extern "C" void PM_Move(struct playermove_s *ppmove, int server);
extern "C" void PM_Init(struct playermove_s *ppmove);
extern "C" char PM_FindTextureType(char *name);

Vector VecBModelOrigin(entvars_t *pevBModel);
extern DLL_GLOBAL Vector g_vecAttackDir;
extern DLL_GLOBAL int g_iSkillLevel;

CUtlVector<hash_item_t> stringsHashTable;
CMemoryPool hashItemMemPool(sizeof(hash_item_t), 64);

int CaseInsensitiveHash(const char *string, int iBounds)
{
	unsigned int hash = 0;

	if (!*string)
		return 0;

	while (*string)
	{
		if (*string < 'A' || *string > 'Z')
			hash = *string + 2 * hash;
		else
			hash = *string + 2 * hash + ' ';

		string++;
	}

	return (hash % iBounds);
}

void EmptyEntityHashTable(void)
{
	int i;
	hash_item_t *item;
	hash_item_t *temp;
	hash_item_t *free;

	for (i = 0; i < stringsHashTable.Count(); i++)
	{
		item = &stringsHashTable[i];
		temp = item->next;
		item->pev = NULL;
		item->pevIndex = 0;
		item->lastHash = NULL;
		item->next = NULL;

		while (temp)
		{
			free = temp;
			temp = temp->next;
			hashItemMemPool.Free(free);
		}
	}
}

void AddEntityHashValue(struct entvars_s *pev, const char *value, hash_types_e fieldType)
{
	int count;
	hash_item_t *item;
	hash_item_t *next;
	hash_item_t *temp;
	hash_item_t *newp;
	unsigned int hash = 0;
	int pevIndex;
	entvars_t *pevtemp;

	if (fieldType == CLASSNAME)
	{
		if (!FStringNull(pev->classname))
		{
			count = stringsHashTable.Count();
			hash = CaseInsensitiveHash(value, count);
			pevIndex = ENTINDEX(ENT(pev));
			item = &stringsHashTable[hash];

			while (item->pev)
			{
				if (!strcmp(STRING(item->pev->classname), STRING(pev->classname)))
					break;

				hash = (hash + 1) % count;
				item = &stringsHashTable[hash];
			}

			if (item->pev)
			{
				next = item->next;

				while (next)
				{
					if (item->pev == pev)
						break;

					if (item->pevIndex >= pevIndex)
						break;

					item = next;
					next = next->next;
				}

				if (pevIndex < item->pevIndex)
				{
					pevtemp = item->pev;
					item->pev = pev;
					item->lastHash = NULL;
					item->pevIndex = pevIndex;
					pevIndex = ENTINDEX(ENT(pevtemp));
				}
				else
					pevtemp = pev;

				if (item->pev != pevtemp)
				{
					temp = item->next;
					newp = (hash_item_t *)hashItemMemPool.Alloc(sizeof(hash_item_t));
					item->next = newp;
					newp->pev = pevtemp;
					newp->lastHash = NULL;
					newp->pevIndex = pevIndex;

					if (temp)
						newp->next = temp;
					else
						newp->next = NULL;
				}
			}
			else
			{
				item->pev = pev;
				item->lastHash = NULL;
				item->pevIndex = ENTINDEX(ENT(pev));
			}
		}
	}
}

void RemoveEntityHashValue(struct entvars_s *pev, const char *value, hash_types_e fieldType)
{
	int hash = 0;
	hash_item_t *item;
	hash_item_t *last;
	int pevIndex;
	int count;

	count = stringsHashTable.Count();
	hash = CaseInsensitiveHash(value, count);
	pevIndex = ENTINDEX(ENT(pev));

	if (fieldType == CLASSNAME)
	{
		hash = hash % count;
		item = &stringsHashTable[hash];

		while (item->pev)
		{
			if (!strcmp(STRING(item->pev->classname), STRING(pev->classname)))
				break;

			hash = (hash + 1) % count;
			item = &stringsHashTable[hash];
		}

		if (item->pev)
		{
			last = item;

			while (item->next)
			{
				if (item->pev == pev)
					break;

				last = item;
				item = item->next;
			}

			if (item->pev == pev)
			{
				if (last == item)
				{
					if (item->next)
					{
						item->pev = item->next->pev;
						item->pevIndex = item->next->pevIndex;
						item->lastHash = NULL;
						item->next = item->next->next;
					}
					else
					{
						item->pev = NULL;
						item->lastHash = NULL;
						item->pevIndex = 0;
					}
				}
				else
				{
					if (stringsHashTable[hash].lastHash == item)
						stringsHashTable[hash].lastHash = NULL;

					last->next = item->next;
					hashItemMemPool.Free(item);
				}
			}
		}
	}
}

void printEntities(void)
{
	int i;
	hash_item_t *item;

	for (i = 0; i < stringsHashTable.Count(); i++)
	{
		item = &stringsHashTable[i];

		if (item->pev)
			UTIL_LogPrintf("Print: %s %i %p\n", STRING(stringsHashTable[i].pev->classname), ENTINDEX(ENT(item->pev)), item->pev);

		for (item = stringsHashTable[i].next; item; item = item->next)
			UTIL_LogPrintf("Print: %s %i %p\n", STRING(item->pev->classname), ENTINDEX(ENT(item->pev)), item->pev);
	}
}

edict_t *CREATE_NAMED_ENTITY(int iClass)
{
	edict_t *named = g_engfuncs.pfnCreateNamedEntity(iClass);

	if (named)
		AddEntityHashValue(&named->v, STRING(iClass), CLASSNAME);

	return named;
}

void REMOVE_ENTITY(edict_t *e)
{
	if (e)
		g_engfuncs.pfnRemoveEntity(e);
}

#include "perf_counter.h"

void loopPerformance(void)
{
	CPerformanceCounter loopCounter;
	loopCounter.InitializePerformanceCounter();

	float start = loopCounter.GetCurTime();
	CBaseEntity *i;

	for (i = UTIL_FindEntityByString_Old(NULL, "classname", "info_player_start"); i; i = UTIL_FindEntityByString_Old(i, "classname", "info_player_start")) {}
	for (i = UTIL_FindEntityByString_Old(NULL, "classname", "info_player_deathmatch"); i; i = UTIL_FindEntityByString_Old(i, "classname", "info_player_deathmatch")) {}
	for (i = UTIL_FindEntityByString_Old(NULL, "classname", "player"); i; i = UTIL_FindEntityByString_Old(i, "classname", "player")) {}

	float end = loopCounter.GetCurTime();

	CONSOLE_ECHO(" Time in old search loop %.4f\n", (end - start) * 1000);

	start = loopCounter.GetCurTime();

	for (i = UTIL_FindEntityByString(NULL, "classname", "info_player_start"); i; i = UTIL_FindEntityByString(i, "classname", "info_player_start")) {}
	for (i = UTIL_FindEntityByString(NULL, "classname", "info_player_deathmatch"); i; i = UTIL_FindEntityByString(i, "classname", "info_player_deathmatch")) {}
	for (i = UTIL_FindEntityByString(NULL, "classname", "player"); i; i = UTIL_FindEntityByString(i, "classname", "player")) {}

	end = loopCounter.GetCurTime();

	CONSOLE_ECHO(" Time in new search loop %.4f\n", (end - start) * 1000);
}

void CONSOLE_ECHO(char *pszMsg, ...)
{
	va_list argptr;
	static char szStr[1024];

	va_start(argptr, pszMsg);
	vsprintf(szStr, pszMsg, argptr);
	va_end(argptr);

	g_engfuncs.pfnServerPrint(szStr);
}

void CONSOLE_ECHO_LOGGED(char *pszMsg, ...)
{
	va_list argptr;
	static char szStr[1024];

	va_start(argptr, pszMsg);
	vsprintf(szStr, pszMsg, argptr);
	va_end(argptr);

	g_engfuncs.pfnServerPrint(szStr);
	UTIL_LogPrintf(szStr);
}

static DLL_FUNCTIONS gFunctionTable =
{
	GameDLLInit,
	DispatchSpawn,
	DispatchThink,
	DispatchUse,
	DispatchTouch,
	DispatchBlocked,
	DispatchKeyValue,
	DispatchSave,
	DispatchRestore,
	DispatchObjectCollsionBox,
	SaveWriteFields,
	SaveReadFields,
	SaveGlobalState,
	RestoreGlobalState,
	ResetGlobalState,
	ClientConnect,
	ClientDisconnect,
	ClientKill,
	ClientPutInServer,
	ClientCommand,
	ClientUserInfoChanged,
	ServerActivate,
	ServerDeactivate,
	PlayerPreThink,
	PlayerPostThink,
	StartFrame,
	ParmsNewLevel,
	ParmsChangeLevel,
	GetGameDescription,
	PlayerCustomization,
	SpectatorConnect,
	SpectatorDisconnect,
	SpectatorThink,
	Sys_Error,
	PM_Move,
	PM_Init,
	PM_FindTextureType,
	SetupVisibility,
	UpdateClientData,
	AddToFullPack,
	CreateBaseline,
	RegisterEncoders,
	GetWeaponData,
	CmdStart,
	CmdEnd,
	ConnectionlessPacket,
	GetHullBounds,
	CreateInstancedBaselines,
	InconsistentFile,
	AllowLagCompensation
};

int GetEntityAPI(DLL_FUNCTIONS *pFunctionTable, int interfaceVersion)
{
	if (!pFunctionTable || interfaceVersion != INTERFACE_VERSION)
		return 0;

	memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));

	stringsHashTable.SetSize(2048);

	for (int i = 0; i < stringsHashTable.Count(); i++)
		stringsHashTable[i].next = NULL;

	EmptyEntityHashTable();
	return 1;
}

void OnFreeEntPrivateData(edict_t *pEnt)
{
	CBaseEntity *pEntity = CBaseEntity::Instance(pEnt);

	if (!pEntity)
		return;

	pEntity->UpdateOnRemove();
	RemoveEntityHashValue(pEntity->pev, STRING(pEntity->pev->classname), CLASSNAME);
}

void OnGameShutdown()
{
	//ALERT(at_logged, "Shutting down\n");

}
static NEW_DLL_FUNCTIONS gNewDLLFunctions =
{
	OnFreeEntPrivateData,
	OnGameShutdown,
	NULL
};

int GetNewDLLFunctions(NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
	if (!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return 0;
	}

	pFunctionTable->pfnOnFreeEntPrivateData = gNewDLLFunctions.pfnOnFreeEntPrivateData;
	pFunctionTable->pfnGameShutdown = gNewDLLFunctions.pfnGameShutdown;
	pFunctionTable->pfnShouldCollide = gNewDLLFunctions.pfnShouldCollide;
	return 1;
}

int DispatchSpawn(edict_t *pent)
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if (pEntity)
	{
		pEntity->pev->absmin = pEntity->pev->origin - Vector(1, 1, 1);
		pEntity->pev->absmax = pEntity->pev->origin + Vector(1, 1, 1);
		pEntity->Spawn();
		pEntity = (CBaseEntity *)GET_PRIVATE(pent);

		if (pEntity)
		{
			if (g_pGameRules && !g_pGameRules->IsAllowedToSpawn(pEntity))
			{
				ALERT(at_logged, "Trying to spawn invalid ent(IsAllowedToSpawn)\n");
				return -1;
			}

			if (pEntity->pev->flags & FL_KILLME)
			{
				ALERT(at_logged, "Trying to spawn invalid ent(FL_KILLME)\n");
				return -1;
			}
			if (pEntity->pev->globalname)
			{
				const globalentity_t *pGlobal = gGlobalState.EntityFromTable(pEntity->pev->globalname);

				if (pGlobal)
				{
					if (pGlobal->state == GLOBAL_DEAD)
					{
						ALERT(at_logged, "Trying to spawn invalid ent(FL_KILLME)\n");
						return -1;
					}
					if (strcmp(STRING(gpGlobals->mapname), pGlobal->levelName))
						pEntity->MakeDormant();
				}
				else
					gGlobalState.EntityAdd(pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON);
			}
		}
	
	}
	else
	{
		ALERT(at_logged, "Trying to spawn invalid ent\n");
	}

	return 0;
}

void DispatchKeyValue(edict_t *pentKeyvalue, KeyValueData *pkvd)
{
	if (!pkvd || !pentKeyvalue)
		return;

	EntvarsKeyvalue(VARS(pentKeyvalue), pkvd);

	if (pkvd->fHandled || !pkvd->szClassName)
		return;

	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentKeyvalue);

	if (pEntity)
		pEntity->KeyValue(pkvd);
}

BOOL gTouchDisabled = FALSE;

void DispatchTouch(edict_t *pentTouched, edict_t *pentOther)
{
	if (gTouchDisabled)
		return;

	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentTouched);
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE(pentOther);

	if (pEntity && pOther && !((pEntity->pev->flags | pOther->pev->flags) & FL_KILLME))
	{
		//ALERT(at_logged, "%s touched %s\n", STRING(pEntity->pev->classname), STRING(pOther->pev->classname));
		pEntity->Touch(pOther);
		
	}
}

void DispatchUse(edict_t *pentUsed, edict_t *pentOther)
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentUsed);
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE(pentOther);

	if (pEntity && !(pEntity->pev->flags & FL_KILLME))
		pEntity->Use(pOther, pOther, USE_TOGGLE, 0);
}

void DispatchThink(edict_t *pent)
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if (pEntity)
	{
		if (pEntity->pev->flags & FL_DORMANT)
			ALERT(at_console, "Dormant entity %s is trying to think (ignoring think)!!\n", STRING(pEntity->pev->classname));
		else
			pEntity->Think();
	}
}

void DispatchBlocked(edict_t *pentBlocked, edict_t *pentOther)
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentBlocked);
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE(pentOther);

	if (pEntity)
		pEntity->Blocked(pOther);
}

void DispatchSave(edict_t *pent, SAVERESTOREDATA *pSaveData)
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if (pEntity && pSaveData)
	{
		ENTITYTABLE *pTable = &pSaveData->pTable[pSaveData->currentIndex];

		if (pTable->pent != pent)
			ALERT(at_error, "ENTITY TABLE OR INDEX IS WRONG!!!!\n");

		if (pEntity->ObjectCaps() & FCAP_DONT_SAVE)
			return;

		if (pEntity->pev->movetype == MOVETYPE_PUSH)
		{
			float delta = pEntity->pev->nextthink - pEntity->pev->ltime;
			pEntity->pev->ltime = gpGlobals->time;
			pEntity->pev->nextthink = pEntity->pev->ltime + delta;
		}

		pTable->location = pSaveData->size;
		pTable->classname = pEntity->pev->classname;

		CSave saveHelper(pSaveData);
		pEntity->Save(saveHelper);
		pTable->size = pSaveData->size - pTable->location;
	}
}

CBaseEntity *FindGlobalEntity(string_t classname, string_t globalname)
{
	edict_t *pent = FIND_ENTITY_BY_STRING(NULL, "globalname", STRING(globalname));
	CBaseEntity *pReturn = CBaseEntity::Instance(pent);

	if (!pReturn)
		return NULL;

	if (strcmp(STRING(pReturn->pev->classname), STRING(classname)))
	{
		ALERT(at_console, "Global entity found %s, wrong class %s\n", STRING(globalname), STRING(pReturn->pev->classname));
		return NULL;
	}

	return pReturn;
}

int DispatchRestore(edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity)
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if (pEntity && pSaveData)
	{
		Vector oldOffset;
		CRestore restoreHelper(pSaveData);

		if (globalEntity)
		{
			entvars_t tmpVars;
			CRestore tmpRestore(pSaveData);
			tmpRestore.PrecacheMode(0);
			tmpRestore.ReadEntVars("ENTVARS", &tmpVars);

			pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
			pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable(tmpVars.globalname);

			if (strcmp(pSaveData->szCurrentMapName, pGlobal->levelName))
				return 0;

			oldOffset = pSaveData->vecLandmarkOffset;
			CBaseEntity *pNewEntity = FindGlobalEntity(tmpVars.classname, tmpVars.globalname);

			if (!pNewEntity)
				return 0;

			restoreHelper.SetGlobalMode(1);
			pSaveData->vecLandmarkOffset = (pSaveData->vecLandmarkOffset - pNewEntity->pev->mins) + tmpVars.mins;
			pEntity = pNewEntity;
			pent = ENT(pEntity->pev);
			gGlobalState.EntityUpdate(pEntity->pev->globalname, gpGlobals->mapname);
		}

		if (pEntity->ObjectCaps() & FCAP_MUST_SPAWN)
		{
			pEntity->Restore(restoreHelper);
			pEntity->Spawn();
		}
		else
		{
			pEntity->Restore(restoreHelper);
			pEntity->Precache();
		}

		pEntity = (CBaseEntity *)GET_PRIVATE(pent);

		if (globalEntity)
		{
			pSaveData->vecLandmarkOffset = oldOffset;

			if (pEntity)
			{
				UTIL_SetOrigin(pEntity->pev, pEntity->pev->origin);
				pEntity->OverrideReset();
			}
		}
		else if (pEntity && pEntity->pev->globalname)
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable(pEntity->pev->globalname);

			if (pGlobal)
			{
				if (pGlobal->state == GLOBAL_DEAD)
					return -1;

				if (strcmp(STRING(gpGlobals->mapname), pGlobal->levelName))
					pEntity->MakeDormant();
			}
			else
			{
				ALERT(at_error, "Global Entity %s (%s) not in table!!!\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->classname));
				gGlobalState.EntityAdd(pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON);
			}
		}
	}

	return 0;
}

void SetObjectCollisionBox(entvars_t *pev);

void DispatchObjectCollsionBox(edict_t *pent)
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if (pEntity)
		pEntity->SetObjectCollisionBox();
	else
		SetObjectCollisionBox(&pent->v);
}

void SaveWriteFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount)
{
	CSave saveHelper(pSaveData);
	saveHelper.WriteFields(pname, pBaseData, pFields, fieldCount);
}

void SaveReadFields(SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount)
{
	CRestore restoreHelper(pSaveData);
	restoreHelper.ReadFields(pname, pBaseData, pFields, fieldCount);
}

edict_t *EHANDLE::Get(void)
{
	if (!m_pent)
		return NULL;

	if (m_pent->serialnumber != m_serialnumber)
		return NULL;

	return m_pent;
}

edict_t *EHANDLE::Set(edict_t *pent)
{
	m_pent = pent;

	if (pent)
		m_serialnumber = m_pent->serialnumber;

	return pent;
}

EHANDLE::operator CBaseEntity *(void)
{
	return (CBaseEntity *)GET_PRIVATE(Get());
}

CBaseEntity *EHANDLE::operator = (CBaseEntity *pEntity)
{
	if (pEntity)
	{
		m_pent = ENT(pEntity->pev);

		if (m_pent)
			m_serialnumber = m_pent->serialnumber;
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}

	return pEntity;
}

EHANDLE::operator int(void)
{
	return Get() != NULL;
}

CBaseEntity *EHANDLE::operator ->(void)
{
	return (CBaseEntity *)GET_PRIVATE(Get());
}

int CBaseEntity::TakeHealth(float flHealth, int bitsDamageType)
{
	if (!pev->takedamage)
		return 0;

	if (pev->health >= pev->max_health)
		return 0;

	pev->health += flHealth;

	if (pev->health > pev->max_health)
		pev->health = pev->max_health;

	return 1;
}

int CBaseEntity::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if (!pev->takedamage)
		return 0;

	g_vecAttackDir = (pevInflictor->origin - VecBModelOrigin(pev)).Normalize();

	if (!FNullEnt(pevInflictor) && (pev->movetype == MOVETYPE_WALK || pev->movetype == MOVETYPE_STEP) && pevAttacker->solid != SOLID_TRIGGER)
	{
		Vector vecDir = (pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5).Normalize();
		float flForce = flDamage * ((32 * 32 * 72.0) / (pev->size.x * pev->size.y * pev->size.z)) * 5;

		if (flForce > 1000)
			flForce = 1000;

		pev->velocity = pev->velocity + vecDir * flForce;
	}

	pev->health -= flDamage;

	if (pev->health <= 0)
	{
		Killed(pevAttacker, GIB_NORMAL);
		return 0;
	}

	return 1;
}

void CBaseEntity::Killed(entvars_t *pevAttacker, int iGib)
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	UTIL_Remove(this);
}

CBaseEntity *CBaseEntity::GetNextTarget(void)
{
	if (FStringNull(pev->target))
		return NULL;

	edict_t *pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (FNullEnt(pTarget))
		return NULL;

	return Instance(pTarget);
}

TYPEDESCRIPTION CBaseEntity::m_SaveData[] =
{
	DEFINE_FIELD(CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR),
	DEFINE_FIELD(CBaseEntity, m_pfnThink, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnTouch, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnUse, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseEntity, m_pfnBlocked, FIELD_FUNCTION)
};

int CBaseEntity::Save(CSave &save)
{
	if (save.WriteEntVars("ENTVARS", pev))
		return save.WriteFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

	return 0;
}

int CBaseEntity::Restore(CRestore &restore)
{
	int status = restore.ReadEntVars("ENTVARS", pev);

	if (status)
		status = restore.ReadFields("BASE", this, m_SaveData, ARRAYSIZE(m_SaveData));

	if (pev->modelindex != 0 && !FStringNull(pev->model))
	{
		Vector mins = pev->mins;
		Vector maxs = pev->maxs;

		PRECACHE_MODEL((char *)STRING(pev->model));
		SET_MODEL(ENT(pev), STRING(pev->model));
		UTIL_SetSize(pev, mins, maxs);
	}

	return status;
}

void SetObjectCollisionBox(entvars_t *pev)
{
	if (pev->solid == SOLID_BSP && (pev->angles.x || pev->angles.y || pev->angles.z))
	{
		float v, max = 0;
		int i;

		for (i = 0; i < 3; i++)
		{
			v = fabs(((float *)pev->mins)[i]);

			if (v > max)
				max = v;

			v = fabs(((float *)pev->maxs)[i]);

			if (v > max)
				max = v;
		}

		for (i = 0; i < 3; i++)
		{
			((float *)pev->absmin)[i] = ((float *)pev->origin)[i] - max;
			((float *)pev->absmax)[i] = ((float *)pev->origin)[i] + max;
		}
	}
	else
	{
		pev->absmin = pev->origin + pev->mins;
		pev->absmax = pev->origin + pev->maxs;
	}

	pev->absmin.x -= 1;
	pev->absmin.y -= 1;
	pev->absmin.z -= 1;
	pev->absmax.x += 1;
	pev->absmax.y += 1;
	pev->absmax.z += 1;
}

void CBaseEntity::SetObjectCollisionBox(void)
{
	::SetObjectCollisionBox(pev);
}

int CBaseEntity::Intersects(CBaseEntity *pOther)
{
	if (pOther->pev->absmin.x > pev->absmax.x || pOther->pev->absmin.y > pev->absmax.y || pOther->pev->absmin.z > pev->absmax.z || pOther->pev->absmax.x < pev->absmin.x || pOther->pev->absmax.y < pev->absmin.y || pOther->pev->absmax.z < pev->absmin.z)
		return 0;

	return 1;
}

void CBaseEntity::MakeDormant(void)
{
	pev->flags |= FL_DORMANT;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	pev->nextthink = 0;
	UTIL_SetOrigin(pev, pev->origin);
}

int CBaseEntity::IsDormant(void)
{
	return pev->flags & FL_DORMANT;
}

BOOL CBaseEntity::IsInWorld(void)
{
	if (pev->origin.x >= 4096 || pev->origin.y >= 4096 || pev->origin.z >= 4096)
		return FALSE;

	if (pev->origin.x <= -4096 || pev->origin.y <= -4096 || pev->origin.z <= -4096)
		return FALSE;

	if (pev->velocity.x >= 2000 || pev->velocity.y >= 2000 || pev->velocity.z >= 2000)
		return FALSE;

	if (pev->velocity.x <= -2000 || pev->velocity.y <= -2000 || pev->velocity.z <= -2000)
		return FALSE;

	return TRUE;
}

int CBaseEntity::ShouldToggle(USE_TYPE useType, BOOL currentState)
{
	if (useType != USE_TOGGLE && useType != USE_SET)
	{
		if ((currentState && useType == USE_ON) || (!currentState && useType == USE_OFF))
			return 0;
	}

	return 1;
}

int CBaseEntity::DamageDecal(int bitsDamageType)
{
	if (pev->rendermode == kRenderTransAlpha)
		return -1;

	if (pev->rendermode != kRenderNormal)
		return DECAL_BPROOF1;

	return RANDOM_LONG(DECAL_GUNSHOT4, DECAL_GUNSHOT5);
}

CBaseEntity *CBaseEntity::Create(char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner)
{
	edict_t *pent = CREATE_NAMED_ENTITY(MAKE_STRING(szName));

	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in Create!\n");
		return NULL;
	}

	CBaseEntity *pEntity = Instance(pent);
	pEntity->pev->owner = pentOwner;
	pEntity->pev->origin = vecOrigin;
	pEntity->pev->angles = vecAngles;
	DispatchSpawn(pEntity->edict());
	return pEntity;
}
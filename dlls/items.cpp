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
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
extern int gmsgItemPickup;
extern int gmsgArmorType;
extern int gmsgStatusIcon;

class CWorldItem : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);

public:
	int m_iType;
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

void CWorldItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CWorldItem::Spawn(void)
{
	CBaseEntity *pEntity = NULL;

	switch (m_iType)
	{
		case 44: pEntity = CBaseEntity::Create("item_battery", pev->origin, pev->angles); break;
		case 42: pEntity = CBaseEntity::Create("item_antidote", pev->origin, pev->angles); break;
		case 43: pEntity = CBaseEntity::Create("item_security", pev->origin, pev->angles); break;
		case 45: pEntity = CBaseEntity::Create("item_suit", pev->origin, pev->angles); break;
	}

	if (pEntity)
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}

void CItem::Spawn(void)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		UTIL_Remove(this);
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (!g_pGameRules->CanHaveItem(pPlayer, this))
		return;

	if (MyTouch(pPlayer))
	{
		SUB_UseTargets(pOther, USE_TOGGLE, 0);
		SetTouch(NULL);
		g_pGameRules->PlayerGotItem(pPlayer, this);

		if (g_pGameRules->ItemShouldRespawn(this) == GR_ITEM_RESPAWN_YES)
			Respawn();
		else
			UTIL_Remove(this);
	}
	else if (gEvilImpulse101)
		UTIL_Remove(this);
}

CBaseEntity *CItem::Respawn(void)
{
	SetTouch(NULL);
	pev->effects |= EF_NODRAW;
	UTIL_SetOrigin(pev, g_pGameRules->VecItemRespawnSpot(this));
	SetThink(&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime(this);
	return this;
}

void CItem::Materialize(void)
{
	if (pev->effects & EF_NODRAW)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", VOL_NORM, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CItem::ItemTouch);
}

#define SF_SUIT_SHORTLOGON 0x0001

class CItemSuit : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_kevlar.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_kevlar.mdl");
		PRECACHE_SOUND("items/tr_kevlar.wav");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->pev->weapons & (1 << WEAPON_SUIT))
			return FALSE;

		EMIT_SOUND(pPlayer->edict(), CHAN_VOICE, "items/tr_kevlar.wav", VOL_NORM, ATTN_NORM);
		pPlayer->pev->weapons |= (1 << WEAPON_SUIT);
		pPlayer->m_iHideHUD &= ~HIDEHUD_HEALTH;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);

class CItemBattery : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_battery.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_battery.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY && (pPlayer->pev->weapons & (1 << WEAPON_SUIT)))
		{
			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = min(pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);
			EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			int pct = (int)((float)(pPlayer->pev->armorvalue * 100) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);

			if (pct > 0)
				pct--;

			char szcharge[64];
			sprintf(szcharge, "!HEV_%1dP", pct);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;
		}

		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);

class CItemAntidote : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_antidote.mdl");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		pPlayer->SetSuitUpdate("!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN);
		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);

class CItemSecurity : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_security.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_security.mdl");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

class CItemLongJump : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_longjump.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_longjump.mdl");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->m_fLongJump)
			return FALSE;

		if (pPlayer->pev->weapons & (1 << WEAPON_SUIT))
		{
			pPlayer->m_fLongJump = TRUE;
			g_engfuncs.pfnSetPhysicsKeyValue(pPlayer->edict(), "slj", "1");

			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();

			EMIT_SOUND_SUIT(pPlayer->edict(), "!HEV_A1");
			return TRUE;
		}

		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_longjump, CItemLongJump);

class CItemKevlar : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_kevlar.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_kevlar.mdl");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->m_iKevlar == 0)
			pPlayer->m_iKevlar = 1;

		pPlayer->pev->armorvalue = 100;
		EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
		WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_ONE, gmsgArmorType, NULL, pPlayer->pev);
		WRITE_BYTE(0);
		MESSAGE_END();
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_kevlar, CItemKevlar);

class CItemAssaultSuit : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_assault.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_assault.mdl");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		pPlayer->m_iKevlar = 2;
		pPlayer->pev->armorvalue = 100;
		EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
		WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_ONE, gmsgArmorType, NULL, pPlayer->pev);
		WRITE_BYTE(1);
		MESSAGE_END();
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_assaultsuit, CItemAssaultSuit);

class CItemThighPack : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_thighpack.mdl");
		CItem::Spawn();
	}

	void Precache(void)
	{
		PRECACHE_MODEL("models/w_thighpack.mdl");
	}

	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->m_iTeam != TEAM_CT)
			return FALSE;

		if (pPlayer->m_bHasDefuser)
			return FALSE;

		pPlayer->m_bHasDefuser = true;
		pPlayer->pev->body = 1;
		ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Got_defuser");

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev);
		WRITE_BYTE(STATUSICON_SHOW);
		WRITE_STRING("defuser");
		WRITE_BYTE(0);
		WRITE_BYTE(160);
		WRITE_BYTE(0);
		MESSAGE_END();

		EMIT_SOUND(pPlayer->edict(), CHAN_VOICE, "items/kevlar.wav", VOL_NORM, ATTN_NORM);
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_thighpack, CItemThighPack);

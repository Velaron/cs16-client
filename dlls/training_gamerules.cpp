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
#include "weapons.h"
#include "gamerules.h"
#include "training_gamerules.h"
#include "game.h"
#include "hostage.h"

extern DLL_GLOBAL BOOL g_fGameOver;

CHalfLifeTraining::CHalfLifeTraining(void)
{
	PRECACHE_MODEL("models/w_weaponbox.mdl");
}

void CHalfLifeTraining::HostageDied(void)
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(1);

	if (pPlayer)
		pPlayer->pev->radsuit_finished = gpGlobals->time + 3;
}

edict_t *CHalfLifeTraining::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
	CBaseEntity *pSpawnSpot = UTIL_FindEntityByClassname(NULL, "info_player_start");

	if (!pSpawnSpot)
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return INDEXENT(0);
	}

	edict_t *pentSpawnSpot = ENT(pSpawnSpot->pev);
	pPlayer->pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS(pentSpawnSpot)->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;
	return pentSpawnSpot;
}

extern int gmsgStatusIcon;
extern int gmsgBlinkAcct;

void CHalfLifeTraining::PlayerThink(CBasePlayer *pPlayer)
{
	if (pPlayer->pev->radsuit_finished && gpGlobals->time > pPlayer->pev->radsuit_finished)
		SERVER_COMMAND("reload\n");

	if (!pPlayer->m_iAccount)
	{
		if (pPlayer->pev->scale)
			pPlayer->m_iAccount = (int)pPlayer->pev->scale;
	}

	if (pPlayer->m_iTeam == TEAM_UNASSIGNED)
	{
		pPlayer->SetProgressBarTime(0);
		pPlayer->m_bHasDefuser = pPlayer->pev->ideal_yaw != 0;
	}

	m_iHostagesRescued = 0;
	m_iRoundTimeSecs = gpGlobals->time + 1;
	m_bFreezePeriod = FALSE;
	g_fGameOver = FALSE;

	pPlayer->m_iTeam = TEAM_CT;
	pPlayer->m_bCanShoot = true;
	pPlayer->m_fLastMovement = gpGlobals->time;

	if (pPlayer->m_pActiveItem)
		pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONS;
	else
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS;

	if (pPlayer->HasNamedPlayerItem("weapon_c4"))
	{
		if (pPlayer->m_rgAmmo[pPlayer->GetAmmoIndex("C4")] <= 0)
		{
			pPlayer->m_bHasC4 = false;

			if (FClassnameIs(pPlayer->m_pActiveItem->pev, "weapon_c4"))
			{
				pPlayer->pev->weapons &= ~(1 << WEAPON_C4);
				pPlayer->RemovePlayerItem(pPlayer->m_pActiveItem);
				pPlayer->m_pActiveItem->Drop();
			}
		}
		else
			pPlayer->m_bHasC4 = true;
	}

	if (!pPlayer->m_bVGUIMenus)
	{
		if (fVGUIMenus)
			pPlayer->m_bVGUIMenus = !!fVGUIMenus;
	}

	CGrenade *pGrenade = NULL;

	while ((pGrenade = (CGrenade *)UTIL_FindEntityByClassname(pGrenade, "grenade")) != NULL)
	{
		if (pGrenade->m_pentCurBombTarget != NULL)
			pGrenade->m_bStartDefuse = true;
	}

	if (pPlayer->m_signals.GetState() & SIGNAL_BUY)
	{
		if (fInBuyArea == FALSE)
		{
			FillAccountTime = 1;

			if (fVisitedBuyArea == FALSE)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev);
				WRITE_BYTE(STATUSICON_FLASH);
				WRITE_STRING("buyzone");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
				MESSAGE_END();
			}
		}

		fInBuyArea = TRUE;

		if (pPlayer->m_iAccount < 16000 && FillAccountTime == 0)
			FillAccountTime = gpGlobals->time + 5;

		if (FillAccountTime != 0 && gpGlobals->time > FillAccountTime)
		{
			if (fVisitedBuyArea == FALSE)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgBlinkAcct, NULL, pPlayer->pev);
				WRITE_BYTE(3);
				MESSAGE_END();

				MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev);
				WRITE_BYTE(STATUSICON_SHOW);
				WRITE_STRING("buyzone");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
				MESSAGE_END();

				fVisitedBuyArea = TRUE;
			}

			pPlayer->AddAccount(16000 - pPlayer->m_iAccount);
			FillAccountTime = 0;
		}
	}
	else
	{
		if (fInBuyArea != FALSE && fVisitedBuyArea != FALSE)
			fInBuyArea = FALSE;
	}

	pPlayer->pev->scale = pPlayer->m_iAccount;
	pPlayer->pev->ideal_yaw = pPlayer->m_bHasDefuser;
}

void CHalfLifeTraining::PlayerSpawn(CBasePlayer *pPlayer)
{
	if (pPlayer->m_bNotKilled)
		return;

	fVisitedBuyArea = FALSE;
	fInBuyArea = TRUE;
	FillAccountTime = 0;

	pPlayer->m_iTeam = TEAM_CT;
	pPlayer->m_iJoiningState = JOINED;
	pPlayer->m_bNotKilled = true;
	pPlayer->pev->body = 0;
	pPlayer->m_iModelName = MODEL_URBAN;
	fVGUIMenus = pPlayer->m_bVGUIMenus;
	SET_MODEL(ENT(pPlayer->pev), "models/player.mdl");

	CBaseEntity *pWeaponEntity = NULL;

	while (pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip"))
		pWeaponEntity->Touch(pPlayer);

	pPlayer->SetPlayerModel(false);
	pPlayer->Spawn();
	pPlayer->m_iHideHUD |= (HIDEHUD_WEAPONS | HIDEHUD_HEALTH | HIDEHUD_TIMER | HIDEHUD_MONEY);
}

BOOL CHalfLifeTraining::IsMultiplayer(void)
{
	return FALSE;
}

BOOL CHalfLifeTraining::IsDeathmatch(void)
{
	return FALSE;
}

int CHalfLifeTraining::ItemShouldRespawn(CItem *pItem)
{
	return GR_ITEM_RESPAWN_NO;
}

BOOL CHalfLifeTraining::FPlayerCanRespawn(CBasePlayer *pPlayer)
{
	return TRUE;
}

void CHalfLifeTraining::InitHUD(CBasePlayer *pl)
{

}

bool CHalfLifeTraining::PlayerCanBuy(CBasePlayer *pPlayer)
{
	return pPlayer->m_signals.GetState() & SIGNAL_BUY;
}

void CHalfLifeTraining::PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
	SET_VIEW(ENT(pVictim->pev), ENT(pVictim->pev));
	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);
}

void CHalfLifeTraining::CheckMapConditions(void)
{

}

void CHalfLifeTraining::CheckWinConditions(void)
{
	CGrenade *pBomb;
	CHostage *pHostage;

	if (m_bBombDefused)
	{
		pBomb = NULL;

		while ((pBomb = (CGrenade *)UTIL_FindEntityByClassname(pBomb, "grenade")) != NULL)
		{
			if (!pBomb->m_bIsC4 || !pBomb->m_bJustBlew)
				continue;

			pBomb->m_bJustBlew = false;
			m_bBombDefused = false;
			FireTargets(STRING(pBomb->pev->target), CBaseEntity::Instance(pBomb->pev->owner), CBaseEntity::Instance(pBomb->pev->owner), USE_TOGGLE, 0);
			break;
		}
	}
	else if (m_bTargetBombed)
	{
		pBomb = NULL;

		while ((pBomb = (CGrenade *)UTIL_FindEntityByClassname(pBomb, "grenade")) != NULL)
		{
			if (!pBomb->m_bIsC4 || !pBomb->m_bJustBlew)
				continue;

			if (FStringNull(pBomb->pev->noise1))
				continue;

			pBomb->m_bJustBlew = false;
			m_bTargetBombed = false;
			FireTargets(STRING(pBomb->pev->noise1), CBaseEntity::Instance(pBomb->pev->owner), CBaseEntity::Instance(pBomb->pev->owner), USE_TOGGLE, 0);
			break;
		}
	}

	pHostage = NULL;

	while ((pHostage = (CHostage *)UTIL_FindEntityByClassname(pHostage, "hostage_entity")) != NULL)
	{
		if (pHostage->pev->deadflag != DEAD_RESPAWNABLE || !FStringNull(pHostage->pev->noise1))
			continue;

		UTIL_SetSize(pHostage->pev, Vector(-16, -16, 0), Vector(16, 16, 72));

		CBaseEntity *pFirstRescueArea;
		CBaseEntity *pRescueArea;

		pFirstRescueArea = pRescueArea = UTIL_FindEntityByClassname(NULL, "func_hostage_rescue");

		if (pFirstRescueArea)
		{
			do
			{
				if (pFirstRescueArea == pRescueArea || pRescueArea->Intersects(pHostage))
				{
					if (pRescueArea)
					{
						pHostage->pev->noise1 = 1;
						FireTargets(STRING(pRescueArea->pev->target), NULL, NULL, USE_TOGGLE, 0);
						break;
					}
				}
			}
			while ((pRescueArea = UTIL_FindEntityByClassname(pRescueArea, "func_hostage_rescue")) != NULL);
		}
	}
}
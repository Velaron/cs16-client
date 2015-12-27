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
#include "game.h"
#include "pm_shared.h"

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

extern int gmsgCrosshair;
extern int gmsgCurWeapon;
extern int gmsgStatusIcon;
extern int gmsgSetFOV;
extern int gmsgSpecHealth2;
extern int gmsgNVGToggle;

int GetForceCamera(void)
{
	int retVal;

	if (!fadetoblack.value)
	{
		retVal = (int)CVAR_GET_FLOAT("mp_forcechasecam");

		if (retVal == FORCECAMERA_SPECTATE_ANYONE)
			retVal = (int)CVAR_GET_FLOAT("mp_forcecamera");
	}
	else
		retVal = FORCECAMERA_ONLY_FRIST_PERSON;

	return retVal;
}

CBaseEntity *CBasePlayer::Observer_IsValidTarget(int iTarget, bool bOnlyTeam)
{
	if (iTarget > gpGlobals->maxClients || iTarget < 1)
		return NULL;

	CBasePlayer *pEnt = (CBasePlayer *)UTIL_PlayerByIndex(iTarget);

	if (!pEnt || pEnt == this || pEnt->has_disconnected || pEnt->IsObserver() || (pEnt->pev->effects & EF_NODRAW) || pEnt->m_iTeam == TEAM_UNASSIGNED || (bOnlyTeam && pEnt->m_iTeam != m_iTeam))
		return NULL;

	return pEnt;
}

void UpdateClientEffects(CBasePlayer *pObserver, int oldMode);

void CBasePlayer::Observer_FindNextPlayer(bool bReverse, char *name)
{
	int iStart;
	int iCurrent;
	int iDir;
	bool bForceSameTeam;
	CBasePlayer *pPlayer;

	if (m_flNextFollowTime && gpGlobals->time < m_flNextFollowTime)
		return;

	m_flNextFollowTime = gpGlobals->time + 0.25;

	iStart = m_hObserverTarget ? ENTINDEX(m_hObserverTarget->edict()) : ENTINDEX(edict());
	iCurrent = iStart;

	m_hObserverTarget = NULL;

	iDir = bReverse ? -1 : 1;
	bForceSameTeam = (GetForceCamera() != FORCECAMERA_SPECTATE_ANYONE && m_iTeam != TEAM_SPECTATOR);

	do
	{
		iCurrent += iDir;

		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		else if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		m_hObserverTarget = Observer_IsValidTarget(iCurrent, bForceSameTeam);

		if (m_hObserverTarget)
		{
			if (!name)
				break;

			pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(ENTINDEX(m_hObserverTarget->edict()));

			if (!strcmp(name, STRING(pPlayer->pev->netname)))
				break;
		}
	}
	while (iCurrent != iStart);

	if (m_hObserverTarget)
	{
		UTIL_SetOrigin(pev, m_hObserverTarget->pev->origin);

		MESSAGE_BEGIN(MSG_ONE, gmsgSpecHealth2, NULL, pev);
		WRITE_BYTE(min(0, m_hObserverTarget->pev->health));
		WRITE_BYTE(ENTINDEX(m_hObserverTarget->edict()));
		MESSAGE_END();

		if (pev->iuser1 != OBS_ROAMING)
			pev->iuser2 = ENTINDEX(m_hObserverTarget->edict());

		UpdateClientEffects(this, pev->iuser1);
	}
}

void UpdateClientEffects(CBasePlayer *pObserver, int oldMode)
{
	bool clearProgress = false;
	bool clearBlindness = false;
	bool blindnessOk = fadetoblack.value == 0 ? true : false;
	bool clearNightvision = false;

	if (oldMode == OBS_IN_EYE && pObserver->pev->iuser1 != OBS_IN_EYE)
	{
		clearProgress = true;
		clearBlindness = true;
		clearNightvision = true;
	}

	if (pObserver->pev->iuser1 == OBS_IN_EYE)
	{
		clearProgress = true;
		clearBlindness = true;
		clearNightvision = true;

		if (pObserver->m_hObserverTarget->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(ENTINDEX(pObserver->m_hObserverTarget->edict()));

			if (pPlayer)
			{
				if (pPlayer->m_progressStart != 0)
				{
					if (pPlayer->m_progressEnd > pPlayer->m_progressStart)
					{
						if (pPlayer->m_progressEnd > gpGlobals->time)
						{
							float percentRemaining = gpGlobals->time - pPlayer->m_progressStart;
							pObserver->SetProgressBarTime2(pPlayer->m_progressEnd - pPlayer->m_progressStart, percentRemaining);
							clearProgress = false;
						}
					}
				}
			}

			if (blindnessOk)
			{
				if (pPlayer)
				{
					if (pPlayer->m_blindStartTime != 0)
					{
						if (pPlayer->m_blindFadeTime != 0)
						{
							float fadeTime, holdTime, alpha, ratio;
							float endTime = pPlayer->m_blindFadeTime + pPlayer->m_blindHoldTime + pPlayer->m_blindStartTime;

							if (endTime > gpGlobals->time)
							{
								clearBlindness = false;
								fadeTime = pPlayer->m_blindFadeTime;
								alpha = pPlayer->m_blindAlpha;
								holdTime = pPlayer->m_blindHoldTime + pPlayer->m_blindStartTime - gpGlobals->time;

								if (holdTime <= 0)
								{
									holdTime = 0;
									ratio = (endTime - gpGlobals->time) / pPlayer->m_blindFadeTime;
									alpha = pPlayer->m_blindAlpha * ratio;
									fadeTime = ratio * fadeTime;
								}

								UTIL_ScreenFade(pObserver, Vector(255, 255, 255), fadeTime, holdTime, alpha, 0);
							}
						}
					}
				}
			}

			clearNightvision = false;

			if (pPlayer->m_bNightVisionOn != pObserver->m_bNightVisionOn)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pObserver->pev);
				WRITE_BYTE(pPlayer->m_bNightVisionOn != false);
				MESSAGE_END();

				pObserver->m_bNightVisionOn = pPlayer->m_bNightVisionOn;
			}
		}
	}

	if (clearProgress)
		pObserver->SetProgressBarTime(0);

	if (blindnessOk && clearBlindness)
		UTIL_ScreenFade(pObserver, Vector(0, 0, 0), 0.001, 0, 0, 0);

	if (clearNightvision)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pObserver->pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		pObserver->m_bNightVisionOn = false;
	}
}

void CBasePlayer::Observer_HandleButtons(void)
{
	if (m_flNextObserverInput > gpGlobals->time)
		return;

	if (m_afButtonPressed & IN_JUMP)
	{
		if (pev->iuser1 == OBS_CHASE_LOCKED)
			Observer_SetMode(OBS_IN_EYE);
		else if (pev->iuser1 == OBS_CHASE_FREE)
			Observer_SetMode(OBS_IN_EYE);
		else if (pev->iuser1 == OBS_IN_EYE)
			Observer_SetMode(OBS_ROAMING);
		else if (pev->iuser1 == OBS_ROAMING)
			Observer_SetMode(OBS_MAP_FREE);
		else if (pev->iuser1 == OBS_MAP_FREE)
			Observer_SetMode(OBS_MAP_CHASE);
		else
			Observer_SetMode(m_bObserverAutoDirector ? OBS_CHASE_LOCKED : OBS_CHASE_FREE);

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	if (m_afButtonPressed & IN_ATTACK)
	{
		Observer_FindNextPlayer(false);
		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	if (m_afButtonPressed & IN_ATTACK2)
	{
		Observer_FindNextPlayer(true);
		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

void CBasePlayer::Observer_CheckTarget(void)
{
	int iPlayerIndex;
	CBasePlayer *pEnt;
	int lastMode;

	if (pev->iuser1 == OBS_ROAMING && m_bWasFollowing == false)
		return;

	if (m_bWasFollowing == true)
	{
		Observer_FindNextPlayer(false);

		if (m_hObserverTarget)
			Observer_SetMode(m_iObserverLastMode);

		return;
	}

	if (m_hObserverTarget == NULL)
		Observer_FindNextPlayer(false);

	if (m_hObserverTarget)
	{
		iPlayerIndex = ENTINDEX(ENT(m_hObserverTarget->pev));
		pEnt = (CBasePlayer *)UTIL_PlayerByIndex(iPlayerIndex);

		if (!pEnt || pEnt->pev->deadflag == DEAD_RESPAWNABLE || (pEnt->pev->effects & EF_NODRAW))
		{
			Observer_FindNextPlayer(false);
		}
		else
		{
			if (pEnt->pev->deadflag == DEAD_DEAD && gpGlobals->time > pEnt->m_fDeadTime + 2)
			{
				Observer_FindNextPlayer(false);

				if (m_hObserverTarget == NULL)
				{
					lastMode = pev->iuser1;
					Observer_SetMode(OBS_ROAMING);
					m_iObserverLastMode = lastMode;
					m_bWasFollowing = true;
				}
			}
		}
	}
	else
	{
		lastMode = pev->iuser1;
		Observer_SetMode(OBS_ROAMING);
		m_iObserverLastMode = lastMode;
	}
}

void CBasePlayer::Observer_CheckProperties(void)
{
	if (pev->iuser1 == OBS_IN_EYE && m_hObserverTarget != NULL)
	{
		CBasePlayer *target = (CBasePlayer *)UTIL_PlayerByIndex(ENTINDEX(ENT(m_hObserverTarget->pev)));

		if (!target)
			return;

		int weapon = target->m_pActiveItem ? target->m_pActiveItem->m_iId : 0;

		if (m_iFOV != target->m_iFOV || m_iObserverWeapon != weapon)
		{
			m_iClientFOV = m_iFOV = target->m_iFOV;

			MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
			WRITE_BYTE(m_iFOV);
			MESSAGE_END();

			m_iObserverWeapon = weapon;

			MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
			WRITE_BYTE(1);
			WRITE_BYTE(m_iObserverWeapon);
			WRITE_BYTE(0);
			MESSAGE_END();
		}

		int targetBombState = STATUSICON_HIDE;

		if (target->m_bHasC4)
		{
			if (target->m_signals.GetState() & SIGNAL_BOMB)
				targetBombState = STATUSICON_FLASH;
			else
				targetBombState = STATUSICON_SHOW;
		}

		if (m_iObserverC4State != targetBombState)
		{
			m_iObserverC4State = targetBombState;

			if (targetBombState)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(m_iObserverC4State);
				WRITE_STRING("c4");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_HIDE);
				WRITE_STRING("c4");
				MESSAGE_END();
			}
		}

		if (m_bObserverHasDefuser != target->m_bHasDefuser)
		{
			m_bObserverHasDefuser = target->m_bHasDefuser;

			if (target->m_bHasDefuser)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_SHOW);
				WRITE_STRING("defuser");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
				WRITE_BYTE(STATUSICON_HIDE);
				WRITE_STRING("defuser");
				MESSAGE_END();
			}
		}

		return;
	}

	m_iFOV = 90;

	if (m_iObserverWeapon)
	{
		m_iObserverWeapon = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
		WRITE_BYTE(1);
		WRITE_BYTE(m_iObserverWeapon);
		WRITE_BYTE(0);
		MESSAGE_END();
	}

	if (m_iObserverC4State)
	{
		m_iObserverC4State = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(0);
		WRITE_STRING("c4");
		MESSAGE_END();
	}

	if (m_bObserverHasDefuser)
	{
		m_bObserverHasDefuser = false;

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(0);
		WRITE_STRING("defuser");
		MESSAGE_END();
	}
}

void CBasePlayer::Observer_SetMode(int iMode)
{
	int forcecamera;
	int oldMode;
	char modemsg[16];

	if (iMode == pev->iuser1)
		return;

	forcecamera = GetForceCamera();

	if (iMode < OBS_CHASE_LOCKED || iMode > OBS_MAP_CHASE)
		iMode = OBS_IN_EYE;

	oldMode = pev->iuser1;

	if (m_iTeam != TEAM_SPECTATOR)
	{
		if (forcecamera == FORCECAMERA_SPECTATE_ONLY_TEAM)
		{
			if (iMode == OBS_ROAMING)
				iMode = OBS_MAP_FREE;
		}
		else if (forcecamera == FORCECAMERA_ONLY_FRIST_PERSON)
			iMode = OBS_IN_EYE;
	}

	if (m_hObserverTarget)
	{
		CBasePlayer *pEnt = (CBasePlayer *)((CBaseEntity *)m_hObserverTarget);

		if (pEnt == this || !pEnt || pEnt->has_disconnected || pEnt->IsObserver() || (pEnt->pev->effects & EF_NODRAW) || (forcecamera != FORCECAMERA_SPECTATE_ANYONE && pEnt->m_iTeam != m_iTeam))
			m_hObserverTarget = NULL;
	}

	pev->iuser1 = iMode;

	if (iMode != OBS_ROAMING)
	{
		if (m_hObserverTarget == NULL)
		{
			Observer_FindNextPlayer(false);

			if (m_hObserverTarget == NULL)
			{
				ClientPrint(pev, HUD_PRINTCENTER, "#Spec_NoTarget");
				pev->iuser1 = OBS_ROAMING;
			}
		}
	}

	if (pev->iuser1 != OBS_ROAMING)
		pev->iuser2 = ENTINDEX(ENT(m_hObserverTarget->pev));
	else
		pev->iuser2 = 0;

	pev->iuser3 = 0;

	if (m_hObserverTarget)
		UTIL_SetOrigin(pev, m_hObserverTarget->pev->origin);

	MESSAGE_BEGIN(MSG_ONE, gmsgCrosshair, NULL, pev);
	WRITE_BYTE((iMode == OBS_ROAMING) ? 1 : 0);
	MESSAGE_END();

	UpdateClientEffects(this, oldMode);

	sprintf(modemsg, "#Spec_Mode%i", pev->iuser1);
	ClientPrint(pev, HUD_PRINTCENTER, modemsg);

	m_iObserverLastMode = iMode;
	m_bWasFollowing = false;
}

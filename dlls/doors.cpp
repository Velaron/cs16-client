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
#include "doors.h"

extern void SetMovedir(entvars_t *ev);

#define noiseMoving noise1
#define noiseArrived noise2

class CBaseDoor : public CBaseToggle
{
public:
	void Spawn(void);
	void Restart(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Blocked(CBaseEntity *pOther);

	int ObjectCaps(void)
	{
		if (pev->spawnflags & SF_ITEM_USE_ONLY)
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
		else
			return (CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	};

	int Save(CSave &save);
	int Restore(CRestore &restore);
	void SetToggleState(int state);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	void EXPORT DoorTouch(CBaseEntity *pOther);
	int DoorActivate(void);
	void EXPORT DoorGoUp(void);
	void EXPORT DoorGoDown(void);
	void EXPORT DoorHitTop(void);
	void EXPORT DoorHitBottom(void);

public:
	BYTE m_bHealthValue;
	BYTE m_bMoveSnd;
	BYTE m_bStopSnd;
	locksound_t m_ls;
	BYTE m_bLockedSound;
	BYTE m_bLockedSentence;
	BYTE m_bUnlockedSound;
	BYTE m_bUnlockedSentence;
	float m_lastBlockedTimestamp;
};

TYPEDESCRIPTION CBaseDoor::m_SaveData[] =
{
	DEFINE_FIELD(CBaseDoor, m_bHealthValue, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bMoveSnd, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bStopSnd, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bLockedSound, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bLockedSentence, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bUnlockedSound, FIELD_CHARACTER),
	DEFINE_FIELD(CBaseDoor, m_bUnlockedSentence, FIELD_CHARACTER),
};

IMPLEMENT_SAVERESTORE(CBaseDoor, CBaseToggle);

#define DOOR_SENTENCEWAIT 6
#define DOOR_SOUNDWAIT 3
#define BUTTON_SOUNDWAIT 0.5

void PlayLockSounds(entvars_t *pev, locksound_t *pls, int flocked, int fbutton)
{
	float flsoundwait = fbutton ? BUTTON_SOUNDWAIT : DOOR_SOUNDWAIT;

	if (flocked)
	{
		int fplaysound = (pls->sLockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sLockedSentence && !pls->bEOFLocked && gpGlobals->time > pls->flwaitSentence);
		float fvol = (fplaysound && fplaysentence) ? 0.25 : 1;

		if (fplaysound)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, (char *)STRING(pls->sLockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		if (fplaysentence)
		{
			int iprev = pls->iLockedSentence;
			pls->iLockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sLockedSentence), 0.85, ATTN_NORM, 0, 100, pls->iLockedSentence, FALSE);
			pls->iUnlockedSentence = 0;
			pls->bEOFLocked = (iprev == pls->iLockedSentence);
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
	else
	{
		int fplaysound = (pls->sUnlockedSound && gpGlobals->time > pls->flwaitSound);
		int fplaysentence = (pls->sUnlockedSentence && !pls->bEOFUnlocked && gpGlobals->time > pls->flwaitSentence);
		float fvol = (fplaysound && fplaysentence) ? 0.25 : 1;

		if (fplaysound)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, (char *)STRING(pls->sUnlockedSound), fvol, ATTN_NORM);
			pls->flwaitSound = gpGlobals->time + flsoundwait;
		}

		if (fplaysentence)
		{
			int iprev = pls->iUnlockedSentence;
			pls->iUnlockedSentence = SENTENCEG_PlaySequentialSz(ENT(pev), STRING(pls->sUnlockedSentence), 0.85, ATTN_NORM, 0, 100, pls->iUnlockedSentence, FALSE);
			pls->iLockedSentence = 0;
			pls->bEOFUnlocked = (iprev == pls->iUnlockedSentence);
			pls->flwaitSentence = gpGlobals->time + DOOR_SENTENCEWAIT;
		}
	}
}

void CBaseDoor::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "skin"))
	{
		pev->skin = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		m_bStopSnd = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
		m_bHealthValue = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sound"))
	{
		m_bLockedSound = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "locked_sentence"))
	{
		m_bLockedSentence = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sound"))
	{
		m_bUnlockedSound = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "unlocked_sentence"))
	{
		m_bUnlockedSentence = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		pev->scale = atof(pkvd->szValue) * (1.0 / 8);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(func_door, CBaseDoor);
LINK_ENTITY_TO_CLASS(func_water, CBaseDoor);

void CBaseDoor::Spawn(void)
{
	Precache();
	SetMovedir(pev);

	if (pev->skin == 0)
	{
		if (FBitSet(pev->spawnflags, SF_DOOR_PASSABLE))
			pev->solid = SOLID_NOT;
		else
			pev->solid = SOLID_BSP;
	}
	else
	{
		pev->solid = SOLID_NOT;
		SetBits(pev->spawnflags, SF_DOOR_SILENT);
	}

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	m_vecPosition1 = pev->origin;
	m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x - 2)) + fabs(pev->movedir.y * (pev->size.y - 2)) + fabs(pev->movedir.z * (pev->size.z - 2)) - m_flLip));

	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{
		UTIL_SetOrigin(pev, m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
		SetTouch(NULL);
	else
		SetTouch(&CBaseDoor::DoorTouch);

	m_lastBlockedTimestamp = 0;
}

void CBaseDoor::Restart(void)
{
	SetMovedir(pev);
	m_toggle_state = TS_AT_BOTTOM;
	DoorGoDown();

	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
		SetTouch(NULL);
	else
		SetTouch(&CBaseDoor::DoorTouch);
}

void CBaseDoor::SetToggleState(int state)
{
	if (state == TS_AT_TOP)
		UTIL_SetOrigin(pev, m_vecPosition2);
	else
		UTIL_SetOrigin(pev, m_vecPosition1);
}

void CBaseDoor::Precache(void)
{
	switch (m_bMoveSnd)
	{
		case 0: pev->noiseMoving = ALLOC_STRING("common/null.wav"); break;
		case 1:
		{
			PRECACHE_SOUND("doors/doormove1.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove1.wav");
			break;
		}

		case 2:
		{
			PRECACHE_SOUND("doors/doormove2.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove2.wav");
			break;
		}

		case 3:
		{
			PRECACHE_SOUND("doors/doormove3.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove3.wav");
			break;
		}

		case 4:
		{
			PRECACHE_SOUND("doors/doormove4.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove4.wav");
			break;
		}

		case 5:
		{
			PRECACHE_SOUND("doors/doormove5.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove5.wav");
			break;
		}

		case 6:
		{
			PRECACHE_SOUND("doors/doormove6.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove6.wav");
			break;
		}

		case 7:
		{
			PRECACHE_SOUND("doors/doormove7.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove7.wav");
			break;
		}

		case 8:
		{
			PRECACHE_SOUND("doors/doormove8.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove8.wav");
			break;
		}

		case 9:
		{
			PRECACHE_SOUND("doors/doormove9.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove9.wav");
			break;
		}

		case 10:
		{
			PRECACHE_SOUND("doors/doormove10.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove10.wav");
			break;
		}

		default: pev->noiseMoving = ALLOC_STRING("common/null.wav"); break;
	}

	switch (m_bStopSnd)
	{
		case 0: pev->noiseArrived = ALLOC_STRING("common/null.wav"); break;
		case 1:
		{
			PRECACHE_SOUND("doors/doorstop1.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop1.wav");
			break;
		}

		case 2:
		{
			PRECACHE_SOUND("doors/doorstop2.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop2.wav");
			break;
		}

		case 3:
		{
			PRECACHE_SOUND("doors/doorstop3.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop3.wav");
			break;
		}

		case 4:
		{
			PRECACHE_SOUND("doors/doorstop4.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop4.wav");
			break;
		}

		case 5:
		{
			PRECACHE_SOUND("doors/doorstop5.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop5.wav");
			break;
		}

		case 6:
		{
			PRECACHE_SOUND("doors/doorstop6.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop6.wav");
			break;
		}

		case 7:
		{
			PRECACHE_SOUND("doors/doorstop7.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop7.wav");
			break;
		}

		case 8:
		{
			PRECACHE_SOUND("doors/doorstop8.wav");
			pev->noiseArrived = ALLOC_STRING("doors/doorstop8.wav");
			break;
		}

		default: pev->noiseArrived = ALLOC_STRING("common/null.wav"); break;
	}

	if (m_bLockedSound)
	{
		char *pszSound = ButtonSound((int)m_bLockedSound);
		PRECACHE_SOUND(pszSound);
		m_ls.sLockedSound = ALLOC_STRING(pszSound);
	}

	if (m_bUnlockedSound)
	{
		char *pszSound = ButtonSound((int)m_bUnlockedSound);
		PRECACHE_SOUND(pszSound);
		m_ls.sUnlockedSound = ALLOC_STRING(pszSound);
	}

	switch (m_bLockedSentence)
	{
		case 1: m_ls.sLockedSentence = ALLOC_STRING("NA"); break;
		case 2: m_ls.sLockedSentence = ALLOC_STRING("ND"); break;
		case 3: m_ls.sLockedSentence = ALLOC_STRING("NF"); break;
		case 4: m_ls.sLockedSentence = ALLOC_STRING("NFIRE"); break;
		case 5: m_ls.sLockedSentence = ALLOC_STRING("NCHEM"); break;
		case 6: m_ls.sLockedSentence = ALLOC_STRING("NRAD"); break;
		case 7: m_ls.sLockedSentence = ALLOC_STRING("NCON"); break;
		case 8: m_ls.sLockedSentence = ALLOC_STRING("NH"); break;
		case 9: m_ls.sLockedSentence = ALLOC_STRING("NG"); break;
		default: m_ls.sLockedSentence = 0; break;
	}

	switch (m_bUnlockedSentence)
	{
		case 1: m_ls.sUnlockedSentence = ALLOC_STRING("EA"); break;
		case 2: m_ls.sUnlockedSentence = ALLOC_STRING("ED"); break;
		case 3: m_ls.sUnlockedSentence = ALLOC_STRING("EF"); break;
		case 4: m_ls.sUnlockedSentence = ALLOC_STRING("EFIRE"); break;
		case 5: m_ls.sUnlockedSentence = ALLOC_STRING("ECHEM"); break;
		case 6: m_ls.sUnlockedSentence = ALLOC_STRING("ERAD"); break;
		case 7: m_ls.sUnlockedSentence = ALLOC_STRING("ECON"); break;
		case 8: m_ls.sUnlockedSentence = ALLOC_STRING("EH"); break;
		default: m_ls.sUnlockedSentence = 0; break;
	}
}

void CBaseDoor::DoorTouch(CBaseEntity *pOther)
{
	entvars_t *pevToucher = pOther->pev;

	if (pevToucher->deadflag != DEAD_NO)
		return;

	if (m_sMaster && !UTIL_IsMasterTriggered(m_sMaster, pOther))
		PlayLockSounds(pev, &m_ls, TRUE, FALSE);

	if (!FStringNull(pev->targetname))
	{
		PlayLockSounds(pev, &m_ls, TRUE, FALSE);
		return;
	}

	m_hActivator = pOther;

	if (DoorActivate())
		SetTouch(NULL);
}

void CBaseDoor::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	m_hActivator = pActivator;

	if (m_toggle_state == TS_AT_BOTTOM || FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
		DoorActivate();
}

int CBaseDoor::DoorActivate(void)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, m_hActivator))
		return 0;

	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN) && m_toggle_state == TS_AT_TOP)
	{
		DoorGoDown();
		return 1;
	}

	if (m_hActivator != 0 && m_hActivator->IsPlayer())
		m_hActivator->TakeHealth(m_bHealthValue, DMG_GENERIC);

	PlayLockSounds(pev, &m_ls, FALSE, FALSE);
	DoorGoUp();
	return 1;
}

extern Vector VecBModelOrigin(entvars_t *pevBModel);

void CBaseDoor::DoorGoUp(void)
{
	entvars_t *pevActivator;
	bool isReversing = m_toggle_state == TS_GOING_DOWN;

	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);

	if (m_toggle_state != TS_GOING_DOWN)
	{
		if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
			EMIT_SOUND(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseMoving), 1, ATTN_NORM);
	}

	m_toggle_state = TS_GOING_UP;
	SetMoveDone(&CBaseDoor::DoorHitTop);

	if (FClassnameIs(pev, "func_door_rotating"))
	{
		float sign = 1;

		if (m_hActivator != 0)
		{
			pevActivator = m_hActivator->pev;

			if (!FBitSet(pev->spawnflags, SF_DOOR_ONEWAY) && pev->movedir.y)
			{
				float loX = pev->mins.x + pev->origin.x;
				float loY = pev->mins.y + pev->origin.y;
				float hiX = pev->maxs.x + pev->origin.x;
				float hiY = pev->maxs.y + pev->origin.y;
				float momentArmX = pevActivator->origin.x - pev->origin.x;
				float momentArmY = pevActivator->origin.y - pev->origin.y;

				if (loX > pevActivator->origin.x)
				{
					if (pevActivator->origin.y < loY)
					{
						if (fabs(momentArmY) > fabs(momentArmX))
							sign = (momentArmY < 0) ? 1 : -1;
						else
							sign = (momentArmX > 0) ? 1 : -1;
					}
					else if (pevActivator->origin.y > hiY)
					{
						if (fabs(momentArmY) > fabs(momentArmX))
							sign = (momentArmY < 0) ? 1 : -1;
						else
							sign = (momentArmX < 0) ? 1 : -1;
					}
					else
						sign = (momentArmY < 0) ? 1 : -1;
				}
				else
				{
					if (pevActivator->origin.x <= hiX)
					{
						if (pevActivator->origin.y > loY)
							sign = (momentArmX > 0) ? 1 : -1;
						else if (pevActivator->origin.y > hiY)
							sign = (momentArmX < 0) ? 1 : -1;
					}
					else if (pevActivator->origin.y < loY)
					{
						if (fabs(momentArmY) > fabs(momentArmX))
							sign = (momentArmY > 0) ? 1 : -1;
						else
							sign = (momentArmX > 0) ? 1 : -1;
					}
					else if (pevActivator->origin.y > hiY)
					{
						if (fabs(momentArmY) > fabs(momentArmX))
							sign = (momentArmY > 0) ? 1 : -1;
						else
							sign = (momentArmX < 0) ? 1 : -1;
					}
					else
						sign = (momentArmY > 0) ? 1 : -1;
				}

				if (isReversing)
					sign = -sign;
			}
		}

		AngularMove(m_vecAngle2 * sign, pev->speed);
	}
	else
		LinearMove(m_vecPosition2, pev->speed);
}

void CBaseDoor::DoorHitTop(void)
{
	if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
	{
		STOP_SOUND(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseMoving));
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseArrived), 1, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;

	if (FBitSet(pev->spawnflags, SF_DOOR_NO_AUTO_RETURN))
	{
		if (!FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
			SetTouch(&CBaseDoor::DoorTouch);
	}
	else
	{
		pev->nextthink = pev->ltime + m_flWait;
		SetThink(&CBaseDoor::DoorGoDown);

		if (m_flWait == -1)
			pev->nextthink = -1;
	}

	if (pev->netname && (pev->spawnflags & SF_DOOR_START_OPEN))
		FireTargets(STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0);

	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);
}

void CBaseDoor::DoorGoDown(void)
{
	if (m_toggle_state != TS_GOING_UP)
	{
		if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
			EMIT_SOUND(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseMoving), VOL_NORM, ATTN_NORM);
	}

#ifdef DOOR_ASSERT
	ASSERT(m_toggle_state == TS_AT_TOP);
#endif
	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone(&CBaseDoor::DoorHitBottom);

	if (FClassnameIs(pev, "func_door_rotating"))
		AngularMove(m_vecAngle1, pev->speed);
	else
		LinearMove(m_vecPosition1, pev->speed);
}

void CBaseDoor::DoorHitBottom(void)
{
	if (!FBitSet(pev->spawnflags, SF_DOOR_SILENT))
	{
		STOP_SOUND(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseMoving));
		EMIT_SOUND(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseArrived), VOL_NORM, ATTN_NORM);
	}

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
		SetTouch(NULL);
	else
		SetTouch(&CBaseDoor::DoorTouch);

	SUB_UseTargets(m_hActivator, USE_TOGGLE, 0);

	if (pev->netname && !(pev->spawnflags & SF_DOOR_START_OPEN))
		FireTargets(STRING(pev->netname), m_hActivator, this, USE_TOGGLE, 0);
}

void CBaseDoor::Blocked(CBaseEntity *pOther)
{
	edict_t *pentTarget = NULL;

	if (pev->dmg)
		pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);

	if (gpGlobals->time - m_lastBlockedTimestamp >= 0.25)
	{
		m_lastBlockedTimestamp = gpGlobals->time;

		if (m_flWait >= 0)
		{
			if (m_toggle_state == TS_GOING_DOWN)
				DoorGoUp();
			else
				DoorGoDown();
		}

		if (!FStringNull(pev->targetname))
		{
			edict_t *pentTarget = NULL;

			while (1)
			{
				pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->targetname));

				if (VARS(pentTarget) != pev)
				{
					if (FNullEnt(pentTarget))
						break;

					if (FClassnameIs(pentTarget, "func_door") || FClassnameIs(pentTarget, "func_door_rotating"))
					{
						CBaseDoor *pDoor = GetClassPtr((CBaseDoor *)VARS(pentTarget));

						if (pDoor->m_flWait >= 0)
						{
							if (pDoor->pev->velocity == pev->velocity && pDoor->pev->avelocity == pev->velocity)
							{
								if (FClassnameIs(pentTarget, "func_door"))
								{
									pDoor->pev->origin = pev->origin;
									pDoor->pev->velocity = g_vecZero;
								}
								else
								{
									pDoor->pev->angles = pev->angles;
									pDoor->pev->avelocity = g_vecZero;
								}
							}

							if (pDoor->m_toggle_state == TS_GOING_DOWN)
								pDoor->DoorGoUp();
							else
								pDoor->DoorGoDown();
						}
					}
				}
			}
		}
	}
}

class CRotDoor : public CBaseDoor
{
public:
	void Restart(void);
	void Spawn(void);
	void SetToggleState(int state);
};

LINK_ENTITY_TO_CLASS(func_door_rotating, CRotDoor);

void CRotDoor::Restart(void)
{
	CBaseToggle::AxisDir(pev);

	if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS))
		pev->movedir = pev->movedir * -1;

	if (pev->speed == 0)
		pev->speed = 100;

	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{
		pev->angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1;
	}

	m_toggle_state = TS_AT_BOTTOM;
	DoorGoDown();
}

void CRotDoor::Spawn(void)
{
	Precache();
	CBaseToggle::AxisDir(pev);

	if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_BACKWARDS))
		pev->movedir = pev->movedir * -1;

	m_vecAngle1 = pev->angles;
	m_vecAngle2 = pev->angles + pev->movedir * m_flMoveDistance;

	if (FBitSet(pev->spawnflags, SF_DOOR_PASSABLE))
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{
		pev->angles = m_vecAngle2;
		Vector vecSav = m_vecAngle1;
		m_vecAngle2 = m_vecAngle1;
		m_vecAngle1 = vecSav;
		pev->movedir = pev->movedir * -1;
	}

	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
		SetTouch(NULL);
	else
		SetTouch(&CBaseDoor::DoorTouch);
}

void CRotDoor::SetToggleState(int state)
{
	if (state == TS_AT_TOP)
		pev->angles = m_vecAngle2;
	else
		pev->angles = m_vecAngle1;

	UTIL_SetOrigin(pev, pev->origin);
}

class CMomentaryDoor : public CBaseToggle
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	BYTE m_bMoveSnd;
};

LINK_ENTITY_TO_CLASS(momentary_door, CMomentaryDoor);

TYPEDESCRIPTION CMomentaryDoor::m_SaveData[] =
{
	DEFINE_FIELD(CMomentaryDoor, m_bMoveSnd, FIELD_CHARACTER),
};

IMPLEMENT_SAVERESTORE(CMomentaryDoor, CBaseToggle);

void CMomentaryDoor::Spawn(void)
{
	SetMovedir(pev);

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), STRING(pev->model));

	if (pev->speed == 0)
		pev->speed = 100;

	if (pev->dmg == 0)
		pev->dmg = 2;

	m_vecPosition1 = pev->origin;
	m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x - 2)) + fabs(pev->movedir.y * (pev->size.y - 2)) + fabs(pev->movedir.z * (pev->size.z - 2)) - m_flLip));

	if (FBitSet(pev->spawnflags, SF_DOOR_START_OPEN))
	{
		UTIL_SetOrigin(pev, m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	SetTouch(NULL);
	Precache();
}

void CMomentaryDoor::Precache(void)
{
	switch (m_bMoveSnd)
	{
		case 0: pev->noiseMoving = ALLOC_STRING("common/null.wav"); break;
		case 1:
		{
			PRECACHE_SOUND("doors/doormove1.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove1.wav");
			break;
		}

		case 2:
		{
			PRECACHE_SOUND("doors/doormove2.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove2.wav");
			break;
		}

		case 3:
		{
			PRECACHE_SOUND("doors/doormove3.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove3.wav");
			break;
		}

		case 4:
		{
			PRECACHE_SOUND("doors/doormove4.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove4.wav");
			break;
		}

		case 5:
		{
			PRECACHE_SOUND("doors/doormove5.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove5.wav");
			break;
		}

		case 6:
		{
			PRECACHE_SOUND("doors/doormove6.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove6.wav");
			break;
		}

		case 7:
		{
			PRECACHE_SOUND("doors/doormove7.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove7.wav");
			break;
		}

		case 8:
		{
			PRECACHE_SOUND("doors/doormove8.wav");
			pev->noiseMoving = ALLOC_STRING("doors/doormove8.wav");
			break;
		}

		default: pev->noiseMoving = ALLOC_STRING("common/null.wav"); break;
	}
}

void CMomentaryDoor::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "movesnd"))
	{
		m_bMoveSnd = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "stopsnd"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

void CMomentaryDoor::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (useType != USE_SET)
		return;

	if (value > 1)
		value = 1;

	Vector move = m_vecPosition1 + (value * (m_vecPosition2 - m_vecPosition1));
	Vector delta = move - pev->origin;
	float speed = delta.Length() * 10;

	if (speed)
	{
		if (pev->nextthink < pev->ltime || !pev->nextthink)
			EMIT_SOUND(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseMoving), VOL_NORM, ATTN_NORM);

		LinearMove(move, speed);
	}
}
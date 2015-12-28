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
#include "weapons.h"
#include "player.h"
#include "hostage.h"
#include "gamerules.h"
#include "training_gamerules.h"
#include "hltv.h"
#include "game.h"
#include "trains.h"
#include "vehicle.h"

extern int gmsgHostagePos;
extern int gmsgHostageK;

int g_iHostageNumber;

LINK_ENTITY_TO_CLASS(monster_scientist, CHostage);
LINK_ENTITY_TO_CLASS(hostage_entity, CHostage);

void CHostage::Spawn(void)
{
	Precache();

	if (pev->classname)
		RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->classname = MAKE_STRING("hostage_entity");
	AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags |= FL_MONSTER;
	pev->deadflag = DEAD_NO;
	pev->max_health = 100;
	pev->health = pev->max_health;
	pev->gravity = 1;
	pev->view_ofs = VEC_HOSTAGE_VIEW;
	pev->velocity = Vector(0, 0, 0);

	if (pev->spawnflags & SF_MONSTER_HITMONSTERCLIP)
		pev->flags |= FL_MONSTERCLIP;

	if (pev->skin < 0)
		pev->skin = 0;

	SET_MODEL(ENT(pev), STRING(pev->model));
	SetActivity(ACT_IDLE);

	m_flNextChange = 0;
	m_State = STAND;
	m_hTargetEnt = NULL;
	m_hStoppedTargetEnt = NULL;
	m_vPathToFollow[0] = Vector(0, 0, 0);
	m_flFlinchTime = 0;
	m_bRescueMe = 0;

	UTIL_SetSize(pev, VEC_HOSTAGE_HULL_MIN, VEC_HOSTAGE_HULL_MAX);
	UTIL_MakeVectors(pev->v_angle);

	SetBoneController(0, UTIL_VecToYaw(gpGlobals->v_forward));
	SetBoneController(1, 0);
	SetBoneController(2, 0);
	SetBoneController(3, 0);
	SetBoneController(4, 0);

	DROP_TO_FLOOR(ENT(pev));

	SetThink(&CHostage::IdleThink);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.2);

	m_flNextFullThink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.2);
	m_vStart = pev->origin;
	m_vStartAngles = pev->angles;
	m_vOldPos = Vector(9999, 9999, 9999);
	m_iHostageIndex = ++g_iHostageNumber;
	nTargetNode = -1;
	m_fHasPath = FALSE;
	m_flLastPathCheck = -1;
	m_flPathAcquired = -1;
	m_flPathCheckInterval = 0.1;
	m_flNextRadarTime = gpGlobals->time + RANDOM_FLOAT(0, 1);
	m_LocalNav = new CLocalNav(this);
	m_bStuck = FALSE;
	m_flStuckTime = 0;
	m_improv = NULL;
}

void CHostage::Precache(void)
{
	m_whichModel = 0;

	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/scientist.mdl");

	PRECACHE_MODEL((char *)STRING(pev->model));
	PRECACHE_SOUND("hostage/hos1.wav");
	PRECACHE_SOUND("hostage/hos2.wav");
	PRECACHE_SOUND("hostage/hos3.wav");
	PRECACHE_SOUND("hostage/hos4.wav");
	PRECACHE_SOUND("hostage/hos5.wav");
	PRECACHE_MODEL("sprites/smoke.spr");
}

void CHostage::SetActivity(Activity activity)
{
	if (m_Activity != activity)
	{
		int animDesired = LookupActivity(activity);

		if (animDesired != -1)
		{
			if (pev->sequence != animDesired)
			{
				if ((m_Activity != ACT_WALK && m_Activity != ACT_RUN) || (activity != ACT_WALK && activity != ACT_RUN))
					pev->frame = 0;

				pev->sequence = animDesired;
			}

			m_Activity = activity;
			ResetSequenceInfo();
		}
	}
}

void CHostage::SetFlinchActivity(void)
{
	switch (m_LastHitGroup)
	{
		case HITGROUP_GENERIC:
		case HITGROUP_HEAD:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH: SetActivity(ACT_SMALL_FLINCH); break;
		case HITGROUP_LEFTARM: SetActivity(ACT_FLINCH_LEFTARM); break;
		case HITGROUP_RIGHTARM: SetActivity(ACT_FLINCH_RIGHTARM); break;
		case HITGROUP_LEFTLEG: SetActivity(ACT_FLINCH_LEFTLEG); break;
		case HITGROUP_RIGHTLEG: SetActivity(ACT_FLINCH_RIGHTLEG); break;
	}
}

void CHostage::SetDeathActivity(void)
{
	if (m_improv)
		return;

	switch (m_LastHitGroup)
	{
		case HITGROUP_GENERIC:
		case HITGROUP_HEAD: SetActivity(ACT_DIE_HEADSHOT); break;
		case HITGROUP_CHEST: SetActivity(ACT_DIESIMPLE); break;
		case HITGROUP_STOMACH: SetActivity(ACT_DIEFORWARD); break;
		case HITGROUP_LEFTARM: SetActivity(ACT_DIEBACKWARD); break;
		case HITGROUP_RIGHTARM: SetActivity(ACT_DIESIMPLE); break;
		case HITGROUP_LEFTLEG: SetActivity(ACT_DIEBACKWARD); break;
		case HITGROUP_RIGHTLEG: SetActivity(ACT_DIEFORWARD); break;
	}
}

float CHostage::GetModifiedDamage(float flDamage)
{
	switch (m_LastHitGroup)
	{
		case HITGROUP_GENERIC: flDamage *= 1.75; break;
		case HITGROUP_HEAD: flDamage *= 2.5; break;
		case HITGROUP_CHEST: flDamage *= 1.5; break;
		case HITGROUP_STOMACH: flDamage *= 1.75; break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM: flDamage *= 0.75; break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG: flDamage *= 0.6; break;
	}

	return flDamage;
}

void CHostage::PlayPainSound(void)
{
	if (m_LastHitGroup == HITGROUP_HEAD)
	{
		int rand = RANDOM_FLOAT(0, 1);

		switch (rand)
		{
			case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot1.wav", VOL_NORM, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot2.wav", VOL_NORM, ATTN_NORM); break;
		}
	}
}

void CHostage::IdleThink(void)
{
	pev->nextthink = gpGlobals->time + (1.0 / 30);
	DispatchAnimEvents(StudioFrameAdvance());

	if (gpGlobals->time >= m_flNextFullThink)
	{
		m_flNextFullThink = gpGlobals->time + 0.1;

		if (pev->deadflag == DEAD_DEAD)
		{
			UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
			return;
		}

		if (m_hTargetEnt != NULL)
		{
			if ((m_bStuck != FALSE && gpGlobals->time - m_flStuckTime > 5) || m_hTargetEnt->pev->deadflag != DEAD_NO)
			{
				m_State = STAND;
				m_hTargetEnt = NULL;
				m_bStuck = FALSE;
			}
		}

		if (m_hTargetEnt != 0 || m_improv)
		{
			CBasePlayer *pPlayer;

			if (m_improv)
			{
			}
			else
				pPlayer = GetClassPtr((CBasePlayer *)m_hTargetEnt->pev);

			if (!pPlayer || pPlayer->m_iTeam == TEAM_CT)
			{
				if (!g_pGameRules->m_bMapHasRescueZone)
				{
					BOOL hasRescueZone = FALSE;

					if (UTIL_FindEntityByClassname(NULL, "info_hostage_rescue"))
						hasRescueZone = TRUE;

					CBaseEntity *pEntity = NULL;

					while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_hostage_rescue")) != NULL)
					{
						if ((pEntity->pev->origin - pev->origin).Length() < 256)
						{
							m_bRescueMe = TRUE;
							break;
						}
					}

					if (!hasRescueZone)
					{
						pEntity = NULL;

						while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_player_start")) != NULL)
						{
							if ((pEntity->pev->origin - pev->origin).Length() < 256)
							{
								m_bRescueMe = TRUE;
								break;
							}
						}
					}
				}

				if (m_bRescueMe)
				{
					if (g_pGameRules->IsCareer() && pPlayer && !pPlayer->IsBot())
					{
					}

					if (pPlayer)
					{
						pev->deadflag = DEAD_RESPAWNABLE;
						pPlayer->AddAccount(1000);
						UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Rescued_A_Hostage\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));
					}

					SendHostageEventMsg();

					MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
					WRITE_BYTE(9);
					WRITE_BYTE(DRC_CMD_EVENT);
					WRITE_SHORT(pPlayer ? ENTINDEX(pPlayer->edict()) : 0);
					WRITE_SHORT(ENTINDEX(edict()));
					WRITE_LONG(15);
					MESSAGE_END();

					pev->effects |= EF_NODRAW;
					Remove();

					g_pGameRules->m_iHostagesRescued++;
					g_pGameRules->CheckWinConditions();

					if (pPlayer)
						Broadcast("rescued");
					else
						Broadcast("escaped");
				}
			}
		}

		DoFollow();

		if (pev->deadflag != DEAD_DEAD && !(pev->effects & EF_NODRAW))
		{
			if (m_flNextRadarTime <= gpGlobals->time)
			{
				if ((m_vOldPos - pev->origin).Length() > 1)
				{
					m_vOldPos = pev->origin;

					if (!g_pGameRules->m_fTeamCount)
						SendHostagePositionMsg();
				}

				m_flNextRadarTime = gpGlobals->time + 1;
			}
		}

		if (m_flFlinchTime <= gpGlobals->time)
		{
			if (pev->velocity.Length() > 160)
				SetActivity(ACT_RUN);
			else if (pev->velocity.Length() > 15)
				SetActivity(ACT_WALK);
			else
				SetActivity(ACT_IDLE);
		}
	}
}

void CHostage::Remove(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pev->nextthink = -1;
	m_flNextFullThink = -1;
}

void CHostage::RePosition(void)
{
	pev->health = pev->max_health;
	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->deadflag = DEAD_NO;
	pev->velocity = Vector(0, 0, 0);
	pev->angles = m_vStartAngles;
	pev->effects &= ~EF_NODRAW;

	m_hTargetEnt = NULL;
	m_hStoppedTargetEnt = NULL;
	m_bTouched = FALSE;
	m_bRescueMe = FALSE;
	m_vOldPos = Vector(9999, 9999, 9999);
	m_flNextRadarTime = 0;

	UTIL_SetOrigin(pev, m_vStart);
	UTIL_SetSize(pev, VEC_HOSTAGE_HULL_MIN, VEC_HOSTAGE_HULL_MAX);
	DROP_TO_FLOOR(ENT(pev));
	SetActivity(ACT_IDLE);

	SetThink(&CHostage::IdleThink);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.2);

	m_fHasPath = FALSE;
	nTargetNode = -1;
	m_flLastPathCheck = -1;
	m_flPathAcquired = -1;
	m_flPathCheckInterval = 0.1;
	m_flNextFullThink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.2);
}

int CHostage::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	CBasePlayer *pAttacker = NULL;
	flDamage = GetModifiedDamage(flDamage);

	if (flDamage > pev->health)
		flDamage = pev->health;

	pev->health -= flDamage;

	if (m_improv)
	{
	}

	PlayPainSound();

	if (pevAttacker)
	{
		CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);

		if (pEntity->Classify() == CLASS_VEHICLE)
		{
			CBaseEntity *pDriver = ((CFuncVehicle *)pEntity)->m_pDriver;

			if (pDriver)
				pevAttacker = pDriver->pev;
		}

		if (pEntity->IsPlayer())
			pAttacker = GetClassPtr((CBasePlayer *)pevAttacker);
	}

	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->movetype = MOVETYPE_TOSS;
		ClearBits(pev->flags, FL_ONGROUND);
		SetDeathActivity();

		if (pAttacker)
		{
			pAttacker->AddAccount((-25 - flDamage) * 20);
			AnnounceDeath(pAttacker);
			ApplyHostagePenalty(pAttacker);
		}

		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->solid = SOLID_NOT;

		if (m_improv)
		{
		}

		g_pGameRules->CheckWinConditions();

		if (!g_pGameRules->m_fTeamCount)
			SendHostageEventMsg();

		pev->nextthink = gpGlobals->time + 3;
		SetThink(&CHostage::Remove);
		return 0;
	}

	m_flFlinchTime = gpGlobals->time + 0.75;

	if (m_improv)
	{
	}
	else
	{
		SetFlinchActivity();

		if (pAttacker)
		{
			pAttacker->AddAccount((int)flDamage * -20);
			ClientPrint(pAttacker->pev, HUD_PRINTCENTER, "#Injured_Hostage");

			if (!(pAttacker->m_flDisplayHistory & DHF_HOSTAGE_INJURED))
			{
				pAttacker->HintMessage("#Hint_careful_around_hostages");
				pAttacker->m_flDisplayHistory |= DHF_HOSTAGE_INJURED;
			}

			return 1;
		}
	}

	return 0;
}

void CHostage::AnnounceDeath(CBasePlayer *pAttacker)
{
	ClientPrint(pAttacker->pev, HUD_PRINTCENTER, "#Killed_Hostage");

	if (!(pAttacker->m_flDisplayHistory & DHF_HOSTAGE_KILLED))
	{
		pAttacker->HintMessage("#Hint_lost_money");
		pAttacker->m_flDisplayHistory |= DHF_HOSTAGE_KILLED;
	}

	if (!g_pGameRules->IsMultiplayer())
		CHalfLifeTraining::HostageDied();

	UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Killed_A_Hostage\"\n", STRING(pAttacker->pev->netname), GETPLAYERUSERID(pAttacker->edict()), GETPLAYERAUTHID(pAttacker->edict()), GetTeam(pAttacker->m_iTeam));

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);
	WRITE_BYTE(DRC_CMD_EVENT);
	WRITE_SHORT(ENTINDEX(pAttacker->edict()));
	WRITE_SHORT(ENTINDEX(edict()));
	WRITE_LONG(15);
	MESSAGE_END();
}

void CHostage::ApplyHostagePenalty(CBasePlayer *pAttacker)
{
	if (pAttacker->m_iTeam == TEAM_TERRORIST)
	{
		int iHostagePenalty = (int)CVAR_GET_FLOAT("mp_hostagepenalty");

		if (iHostagePenalty)
		{
			if (pAttacker->m_iHostagesKilled++ == iHostagePenalty)
				pAttacker->HintMessage("#Hint_removed_for_next_hostage_killed", TRUE);
			else if (pAttacker->m_iHostagesKilled >= iHostagePenalty)
				CLIENT_COMMAND(pAttacker->edict(), "disconnect\n");
		}
	}
}

void CHostage::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator->IsPlayer())
		return;

	if (pev->takedamage == DAMAGE_NO)
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;

	if (pPlayer->m_iTeam != TEAM_CT)
	{
		if (!(pPlayer->m_flDisplayHistory & DHF_HOSTAGE_CTMOVE))
		{
			pPlayer->m_flDisplayHistory |= DHF_HOSTAGE_CTMOVE;
			pPlayer->HintMessage("#Only_CT_Can_Move_Hostages", FALSE, TRUE);
		}

		return;
	}

	if (gpGlobals->time >= m_flNextChange)
	{
		m_flNextChange = gpGlobals->time + 1;

		if (m_improv)
		{
		}
		else
		{
			m_flPathAcquired = gpGlobals->time;

			if (pPlayer != m_hTargetEnt)
			{
				m_State = FOLLOW;
				m_hTargetEnt = pPlayer;
				m_hStoppedTargetEnt = NULL;
			}
			else if (m_State == FOLLOW)
			{
				m_State = STAND;
				m_hTargetEnt = NULL;
				m_hStoppedTargetEnt = pPlayer;
			}
			else
				m_State = FOLLOW;

			if (m_State == FOLLOW)
				PlayFollowRescueSound();
		}

		GiveCTTouchBonus(pPlayer);
		pPlayer->HostageUsed();
	}
}

void CHostage::GiveCTTouchBonus(CBasePlayer *pPlayer)
{
	if (m_bTouched == FALSE)
	{
		m_bTouched = TRUE;
		g_pGameRules->m_iAccountCT += 100;
		pPlayer->AddAccount(150);
		UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Touched_A_Hostage\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));
	}
}

void CHostage::PlayFollowRescueSound(void)
{
	switch (RANDOM_LONG(0, 4))
	{
		case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "hostage/hos1.wav", VOL_NORM, ATTN_NORM); break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "hostage/hos2.wav", VOL_NORM, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "hostage/hos3.wav", VOL_NORM, ATTN_NORM); break;
		case 3: EMIT_SOUND(ENT(pev), CHAN_VOICE, "hostage/hos4.wav", VOL_NORM, ATTN_NORM); break;
		case 4: EMIT_SOUND(ENT(pev), CHAN_VOICE, "hostage/hos5.wav", VOL_NORM, ATTN_NORM); break;
	}
}

int CHostage::ObjectCaps(void)
{
	return CBaseMonster::ObjectCaps() | FCAP_MUST_SPAWN | FCAP_ONOFF_USE;
}

void UTIL_DrawBeamPoints(Vector vecSrc, Vector vecDest, int iLifetime, byte bRed, byte bGreen, byte bBlue);

void CHostage::Touch(CBaseEntity *pOther)
{
	if (m_improv)
		return;

	if (pOther->IsPlayer() !=FALSE)
	{
		if (((CBasePlayer *)pOther)->m_iTeam != TEAM_CT)
			return;
	}
	else
	{
		if (!FClassnameIs(pOther->pev, "hostage_entity"))
			return;
	}

	Vector vecSrc, vecDest;
	vecDest = (pev->origin - pOther->pev->origin);
	ALERT(at_logged, "Check this! %s %s %i\n", __FILE__, __FUNCTION__, __LINE__);
	//vecDest.z = pev->origin.z - pev->origin.z;
	vecDest = vecDest.Normalize() * 50;
	pev->velocity = pev->velocity + vecDest;
	vecSrc = pev->origin;
	vecDest = vecSrc + (pev->velocity.Normalize() * 500);
	UTIL_DrawBeamPoints(vecSrc, vecDest, 10, 0, 255, 0);
}

void CHostage::PointAt(Vector &origin)
{
	pev->angles.x = 0;
	pev->angles.y = UTIL_VecToAngles(origin - pev->origin).y;
	pev->angles.z = 0;
}

void CHostage::DoFollow(void)
{
	if (m_hTargetEnt == NULL)
		return;

	CBaseEntity *pFollowing;
	Vector vecDest;
	float flRadius;
	int nindexPath;

	pFollowing = GetClassPtr((CBaseEntity *)m_hTargetEnt->pev);
	m_LocalNav->SetTargetEnt(pFollowing);

	vecDest = pFollowing->pev->origin;
	vecDest.z += pFollowing->pev->mins.z;
	flRadius = (vecDest - pev->origin).Length();

	if (flRadius < 80)
	{
		if (m_fHasPath)
			return;

		if (m_LocalNav->PathTraversable(pev->origin, vecDest, TRUE) != TRAVERSABLE_NO)
			return;
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (gpGlobals->time > m_flLastPathCheck + m_flPathCheckInterval)
		{
			if (!m_fHasPath || pFollowing->pev->velocity.Length2D() > 1)
			{
				m_flLastPathCheck = gpGlobals->time;
				m_LocalNav->RequestNav(this);
			}
		}
	}

	if (m_fHasPath)
	{
		nTargetNode = nindexPath = m_LocalNav->GetFurthestTraversableNode(pev->origin, vecNodes, m_nPathNodes, TRUE);

		if (!nindexPath)
		{
			if ((vecNodes[nTargetNode] - pev->origin).Length2D() < HOSTAGE_STEPSIZE)
				nTargetNode = -1;
		}

		if (nTargetNode == -1)
		{
			m_fHasPath = FALSE;
			m_flPathCheckInterval = 0.1;
		}
	}

	if (gpGlobals->time < m_flFlinchTime)
		return;

	if (nTargetNode != -1)
	{
		if (FBitSet(pev->flags, FL_ONGROUND))
			PointAt(vecNodes[nTargetNode]);

		if (IsOnLadder())
			pev->v_angle.x = -60;

		MoveToward(vecNodes[nTargetNode]);
		m_bStuck = FALSE;
	}

	if (pev->takedamage == DAMAGE_YES)
	{
		if (m_improv)
		{
		}

		if (m_hTargetEnt != NULL && m_State == FOLLOW)
		{
			if (!m_bStuck)
			{
				if (flRadius > 200)
				{
					m_bStuck = TRUE;
					m_flStuckTime = gpGlobals->time;
				}
			}
		}
	}

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (m_flPathAcquired != -1 && m_flPathAcquired + 2 > gpGlobals->time)
		{
			if (pev->velocity.Length2D() < 1 || nTargetNode == -1)
				Wiggle();
		}
	}
}

void CHostage::MoveToward(Vector &vecLoc)
{
	int nFwdMove;
	Vector vecFwd;
	Vector vecbigDest;
	Vector vecMove;
	CBaseEntity *pFollowing;
	Vector vecAng;
	float flDist;

	pFollowing = GetClassPtr((CBaseEntity *)m_hTargetEnt->pev);
	vecMove = vecLoc - pev->origin;
	vecAng = UTIL_VecToAngles(vecMove);
	vecAng = Vector(0, vecAng.y, 0);
	UTIL_MakeVectorsPrivate(vecAng, vecFwd, NULL, NULL);

	if ((vecFwd * m_LocalNav->s_flStepSize).Length2D() <= (vecLoc - pev->origin).Length2D())
		flDist = (vecFwd * m_LocalNav->s_flStepSize).Length2D();
	else
		flDist = (vecLoc - pev->origin).Length2D();

	vecbigDest = pev->origin + (vecFwd * flDist);
	nFwdMove = m_LocalNav->PathTraversable(pev->origin, vecbigDest, FALSE);

	if (nFwdMove != TRAVERSABLE_NO)
	{
		vecbigDest = pFollowing->pev->origin;
		vecbigDest.z += pFollowing->pev->mins.z;

		if (FBitSet(pev->flags, FL_ONGROUND))
		{
			flDist = (vecbigDest - pev->origin).Length();

			if (flDist >= 110)
			{
				if (flDist >= 250)
					flDist = 400;
				else
					flDist = 300;
			}
		}
		else
			flDist = 250;

		pev->velocity.x = vecFwd.x * flDist;
		pev->velocity.y = vecFwd.y * flDist;
		UTIL_DrawBeamPoints(pev->origin, pev->origin + (pev->velocity.Normalize() * 500), 10, 255, 0, 0);

		if (nFwdMove == TRAVERSABLE_STEPJUMPABLE)
		{
			if (FBitSet(pev->flags, FL_ONGROUND))
				pev->velocity.z = 270;
		}
	}
}

void CHostage::NavReady(void)
{
	CBaseEntity *pFollowing;
	Vector vecSrc;
	Vector vecDest;
	TraceResult tr;
	int nindexPath;

	if (m_hTargetEnt == NULL)
		return;

	pFollowing = GetClassPtr((CBaseEntity *)m_hTargetEnt->pev);
	vecSrc = pFollowing->pev->origin;
	vecDest = pFollowing->pev->origin;

	if (!FBitSet(pFollowing->pev->flags, FL_ONGROUND))
	{
		UTIL_TraceHull(vecSrc, vecDest - Vector(0, 0, 300), ignore_monsters, human_hull, pFollowing->edict(), &tr);

		if (tr.fStartSolid || tr.flFraction == 1)
			return;

		vecDest = tr.vecEndPos;
	}

	vecDest.z += pFollowing->pev->mins.z;
	m_LocalNav->SetTargetEnt(pFollowing);
	nindexPath = m_LocalNav->FindPath(pev->origin, vecDest, 40, TRUE);

	if (nindexPath != -1)
	{
		m_fHasPath = TRUE;
		nTargetNode = -1;
		m_flPathAcquired = gpGlobals->time;
		m_flPathCheckInterval = 0.5;
		m_nPathNodes = m_LocalNav->SetupPathNodes(nindexPath, vecNodes, TRUE);
	}
	else if (!m_fHasPath)
	{
		m_flPathCheckInterval += 0.1;

		if (m_flPathCheckInterval >= 0.5)
			m_flPathCheckInterval = 0.5;
	}
}

void CHostage::SendHostagePositionMsg(void)
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		if (!pEntity->IsPlayer())
			continue;

		if (pEntity->pev->flags == FL_DORMANT)
			continue;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pPlayer->pev->deadflag == DEAD_NO && pPlayer->m_iTeam == TEAM_CT)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgHostagePos, NULL, pPlayer->pev);
			WRITE_BYTE(0);
			WRITE_BYTE(m_iHostageIndex);
			WRITE_COORD(pev->origin.x);
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			MESSAGE_END();
		}
	}
}

void CHostage::SendHostageEventMsg(void)
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		if (!pEntity->IsPlayer())
			continue;

		if (pEntity->pev->flags == FL_DORMANT)
			continue;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pPlayer->pev->deadflag == DEAD_NO && pPlayer->m_iTeam == TEAM_CT)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgHostageK, NULL, pPlayer->pev);
			WRITE_BYTE(m_iHostageIndex);
			MESSAGE_END();
		}

		if (pPlayer->pev->deadflag == DEAD_NO)
			pPlayer->SendHostageIcons();
	}
}

void CHostage::Wiggle(void)
{
	TraceResult tr;
	Vector vec = Vector(0, 0, 0);
	Vector wiggle_directions[8] =
	{
		Vector(50, 0, 0),
		Vector(-50, 0, 0),
		Vector(0, 50, 0),
		Vector(0, -50, 0),
		Vector(50, 50, 0),
		Vector(50, -50, 0),
		Vector(-50, 50, 0),
		Vector(-50, -50, 0)
	};

	for (int i = 0; i < 8; i++)
	{
		vec=pev->origin + wiggle_directions[i];
		if (m_LocalNav->PathTraversable(pev->origin, vec, TRUE) == TRAVERSABLE_NO)
			vec = vec - wiggle_directions[i];
	}

	Vector vecSrc, vecDest;
	vec = vec + Vector(RANDOM_FLOAT(-3, 3), RANDOM_FLOAT(-3, 3), 0);
	pev->velocity = pev->velocity + (vec.Normalize() * 100);
	vecSrc = pev->origin;
	vecDest = vecSrc + (pev->velocity.Normalize() * 500);
	UTIL_DrawBeamPoints(vecSrc, vecDest, 10, 0, 0, 255);
}

void CHostage::PreThink(void)
{
	Vector vecSrc;
	Vector vecDest;
	TraceResult tr;
	float flOrigDist;
	float flRaisedDist;

	if (m_improv)
		return;

	if (!FBitSet(pev->flags, FL_ONGROUND))
		return;

	if (pev->velocity.Length2D() < 1)
		return;

	vecSrc = pev->origin;
	vecDest = vecSrc + pev->velocity * gpGlobals->frametime;
	vecDest.z = vecSrc.z;

	TRACE_MONSTER_HULL(edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr);

	if (tr.fStartSolid)
		return;

	if (tr.flFraction == 1)
		return;

	if (tr.vecPlaneNormal.z > 0.7)
		return;

	flOrigDist = (tr.vecEndPos - pev->origin).Length2D();
	vecSrc.z += m_LocalNav->s_flStepSize;
	vecDest = vecSrc + (pev->velocity.Normalize() * 0.1);
	vecDest.z = vecSrc.z;

	TRACE_MONSTER_HULL(edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr);

	if (tr.fStartSolid)
		return;

	vecSrc = tr.vecEndPos;
	vecDest = tr.vecEndPos;
	vecDest.z -= m_LocalNav->s_flStepSize;

	TRACE_MONSTER_HULL(edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr);

	if (tr.vecPlaneNormal.z < 0.7)
		return;

	flRaisedDist = (tr.vecEndPos - pev->origin).Length2D();

	if (flRaisedDist > flOrigDist)
	{
		Vector vecOrigin = pev->origin;
		vecOrigin.z = tr.vecEndPos.z;
		UTIL_SetOrigin(pev, vecOrigin);
		pev->velocity.z += pev->gravity * g_psv_gravity->value * gpGlobals->frametime;
	}
}

int CHostage::BloodColor(void)
{
	return DONT_BLEED;
}

void UTIL_DrawBeamPoints(Vector vecSrc, Vector vecDest, int iLifetime, byte bRed, byte bGreen, byte bBlue)
{
#ifdef _DEBUG
	static int s_iBeamSprite = 0;

	if (!s_iBeamSprite)
		s_iBeamSprite = PRECACHE_MODEL("sprites/smoke.spr");

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z);
	WRITE_COORD(vecDest.x);
	WRITE_COORD(vecDest.y);
	WRITE_COORD(vecDest.z);
	WRITE_SHORT(s_iBeamSprite);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(iLifetime);
	WRITE_BYTE(10);
	WRITE_BYTE(0);
	WRITE_BYTE(bRed);
	WRITE_BYTE(bGreen);
	WRITE_BYTE(bBlue);
	WRITE_BYTE(255);
	WRITE_BYTE(0);
	MESSAGE_END();
#endif
}

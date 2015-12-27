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
#include "soundent.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "pm_materials.h"
#include "player.h"
#include "game.h"

extern DLL_GLOBAL Vector g_vecAttackDir;
extern entvars_t *g_pevLastInflictor;

#define GERMAN_GIB_COUNT 4
#define HUMAN_GIB_COUNT 6
#define ALIEN_GIB_COUNT 4

void CGib::LimitVelocity(void)
{
	float length = pev->velocity.Length();

	if (length > 1500)
		pev->velocity = pev->velocity.Normalize() * 1500;
}

void CGib::SpawnStickyGibs(entvars_t *pevVictim, Vector vecOrigin, int cGibs)
{
	if (g_Language == LANGUAGE_GERMAN)
		return;

	for (int i = 0; i < cGibs; i++)
	{
		CGib *pGib = GetClassPtr((CGib *)NULL);
		pGib->Spawn("models/stickygib.mdl");
		pGib->pev->body = RANDOM_LONG(0, 2);

		if (pevVictim)
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT(-3, 3);
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT(-3, 3);
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT(-3, 3);
			pGib->pev->velocity = g_vecAttackDir * -1;
			pGib->pev->velocity.x += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity.y += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity.z += RANDOM_FLOAT(-0.15, 0.15);
			pGib->pev->velocity = pGib->pev->velocity * 900;
			pGib->pev->avelocity.x = RANDOM_FLOAT(250, 400);
			pGib->pev->avelocity.y = RANDOM_FLOAT(250, 400);
			pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();

			if (pevVictim->health > -50)
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			else if (pevVictim->health > -200)
				pGib->pev->velocity = pGib->pev->velocity * 2;
			else
				pGib->pev->velocity = pGib->pev->velocity * 4;

			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize(pGib->pev, Vector(0, 0, 0), Vector(0, 0, 0));
			pGib->SetTouch(&CGib::StickyGibTouch);
			pGib->SetThink(NULL);
		}

		pGib->LimitVelocity();
	}
}

void CGib::SpawnHeadGib(entvars_t *pevVictim)
{
	CGib *pGib = GetClassPtr((CGib *)NULL);

	if (g_Language == LANGUAGE_GERMAN)
	{
		pGib->Spawn("models/germangibs.mdl");
		pGib->pev->body = 0;
	}
	else
	{
		pGib->Spawn("models/hgibs.mdl");
		pGib->pev->body = 0;
	}

	if (pevVictim)
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;
		edict_t *pentPlayer = FIND_CLIENT_IN_PVS(pGib->edict());

		if (RANDOM_LONG(0, 100) <= 5 && pentPlayer)
		{
			entvars_t *pevPlayer = VARS(pentPlayer);
			pGib->pev->velocity = ((pevPlayer->origin + pevPlayer->view_ofs) - pGib->pev->origin).Normalize() * 300;
			pGib->pev->velocity.z += 100;
		}
		else
			pGib->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

		pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
		pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);
		pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();

		if (pevVictim->health > -50)
			pGib->pev->velocity = pGib->pev->velocity * 0.7;
		else if (pevVictim->health > -200)
			pGib->pev->velocity = pGib->pev->velocity * 2;
		else
			pGib->pev->velocity = pGib->pev->velocity * 4;
	}

	pGib->LimitVelocity();
}

void CGib::SpawnRandomGibs(entvars_t *pevVictim, int cGibs, int human)
{
	for (int cSplat = 0; cSplat < cGibs; cSplat++)
	{
		CGib *pGib = GetClassPtr((CGib *)NULL);

		if (g_Language == LANGUAGE_GERMAN)
		{
			pGib->Spawn("models/germangibs.mdl");
			pGib->pev->body = RANDOM_LONG(0, GERMAN_GIB_COUNT - 1);
		}
		else
		{
			if (human)
			{
				pGib->Spawn("models/hgibs.mdl");
				pGib->pev->body = RANDOM_LONG(1, HUMAN_GIB_COUNT - 1);
			}
			else
			{
				pGib->Spawn("models/agibs.mdl");
				pGib->pev->body = RANDOM_LONG(0, ALIEN_GIB_COUNT - 1);
			}
		}

		if (pevVictim)
		{
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * RANDOM_FLOAT(0, 1);
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * RANDOM_FLOAT(0, 1);
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * RANDOM_FLOAT(0, 1) + 1;
			pGib->pev->velocity = g_vecAttackDir * -1;
			pGib->pev->velocity.x += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.y += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity.z += RANDOM_FLOAT(-0.25, 0.25);
			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT(300, 400);
			pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
			pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);
			pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();

			if (pevVictim->health > -50)
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			else if (pevVictim->health > -200)
				pGib->pev->velocity = pGib->pev->velocity * 2;
			else
				pGib->pev->velocity = pGib->pev->velocity * 4;

			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize(pGib->pev, Vector(0, 0, 0), Vector(0, 0, 0));
		}

		pGib->LimitVelocity();
	}
}

BOOL CBaseMonster::HasHumanGibs(void)
{
	int myClass = Classify();

	if (myClass == CLASS_HUMAN_MILITARY || myClass == CLASS_PLAYER_ALLY || myClass == CLASS_HUMAN_PASSIVE || myClass == CLASS_PLAYER)
		return TRUE;

	return FALSE;
}

BOOL CBaseMonster::HasAlienGibs(void)
{
	int myClass = Classify();

	if (myClass == CLASS_ALIEN_MILITARY || myClass == CLASS_ALIEN_MONSTER || myClass == CLASS_ALIEN_PASSIVE || myClass == CLASS_INSECT || myClass == CLASS_ALIEN_PREDATOR || myClass == CLASS_ALIEN_PREY)
		return TRUE;

	return FALSE;
}

void CBaseMonster::FadeMonster(void)
{
	StopAnimation();
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

void CBaseMonster::GibMonster(void)
{
	BOOL gibbed = FALSE;
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);

	if (HasHumanGibs())
	{
		if (CVAR_GET_FLOAT("violence_hgibs") != 0)
		{
			CGib::SpawnHeadGib(pev);
			CGib::SpawnRandomGibs(pev, 4, 1);
		}

		gibbed = TRUE;
	}
	else if (HasAlienGibs())
	{
		if (CVAR_GET_FLOAT("violence_agibs") != 0)
			CGib::SpawnRandomGibs(pev, 4, 0);

		gibbed = TRUE;
	}

	if (!IsPlayer())
	{
		if (gibbed)
		{
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
		else
			FadeMonster();
	}
}

Activity CBaseMonster::GetDeathActivity(void)
{
	Activity deathActivity;
	BOOL fTriedDirection;
	float flDot;
	TraceResult tr;
	Vector vecSrc;

	if (pev->deadflag != DEAD_NO)
		return m_IdealActivity;

	vecSrc = Center();
	fTriedDirection = FALSE;
	deathActivity = ACT_DIESIMPLE;

	UTIL_MakeVectors(pev->angles);
	flDot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

	switch (m_LastHitGroup)
	{
		case HITGROUP_HEAD: deathActivity = ACT_DIE_HEADSHOT; break;
		case HITGROUP_STOMACH: deathActivity = ACT_DIE_GUTSHOT; break;

		case HITGROUP_GENERIC:
		default:
		{
			fTriedDirection = TRUE;

			if (flDot > 0.3)
				deathActivity = ACT_DIEFORWARD;
			else if (flDot <= -0.3)
				deathActivity = ACT_DIEBACKWARD;

			break;
		}
	}

	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
	{
		if (!fTriedDirection)
		{
			if (flDot > 0.3)
				deathActivity = ACT_DIEFORWARD;
			else if (flDot <= -0.3)
				deathActivity = ACT_DIEBACKWARD;
		}
		else
			deathActivity = ACT_DIESIMPLE;
	}

	if (LookupActivity(deathActivity) == ACTIVITY_NOT_AVAILABLE)
		deathActivity = ACT_DIESIMPLE;

	if (deathActivity == ACT_DIEFORWARD)
	{
		UTIL_TraceHull(vecSrc, vecSrc + gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);

		if (tr.flFraction != 1)
			deathActivity = ACT_DIESIMPLE;
	}

	if (deathActivity == ACT_DIEBACKWARD)
	{
		UTIL_TraceHull(vecSrc, vecSrc - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);

		if (tr.flFraction != 1)
			deathActivity = ACT_DIESIMPLE;
	}

	return deathActivity;
}

Activity CBaseMonster::GetSmallFlinchActivity(void)
{
	Activity flinchActivity;
	BOOL fTriedDirection;
	float flDot;

	fTriedDirection = FALSE;
	UTIL_MakeVectors(pev->angles);
	flDot = DotProduct(gpGlobals->v_forward, g_vecAttackDir * -1);

	switch (m_LastHitGroup)
	{
		case HITGROUP_HEAD: flinchActivity = ACT_FLINCH_HEAD; break;
		case HITGROUP_STOMACH: flinchActivity = ACT_FLINCH_STOMACH; break;
		case HITGROUP_LEFTARM: flinchActivity = ACT_FLINCH_LEFTARM; break;
		case HITGROUP_RIGHTARM: flinchActivity = ACT_FLINCH_RIGHTARM; break;
		case HITGROUP_LEFTLEG: flinchActivity = ACT_FLINCH_LEFTLEG; break;
		case HITGROUP_RIGHTLEG: flinchActivity = ACT_FLINCH_RIGHTLEG; break;

		case HITGROUP_GENERIC:
		default: flinchActivity = ACT_SMALL_FLINCH; break;
	}

	if (LookupActivity(flinchActivity) == ACTIVITY_NOT_AVAILABLE)
		flinchActivity = ACT_SMALL_FLINCH;

	return flinchActivity;
}

void CBaseMonster::BecomeDead(void)
{
	pev->takedamage = DAMAGE_YES;
	pev->health = pev->max_health / 2;
	pev->max_health = 5;
	pev->movetype = MOVETYPE_TOSS;
}

BOOL CBaseMonster::ShouldGibMonster(int iGib)
{
	if ((iGib == GIB_NORMAL && pev->health < GIB_HEALTH_VALUE) || iGib == GIB_ALWAYS)
		return TRUE;

	return FALSE;
}

void CBaseMonster::CallGibMonster(void)
{
	BOOL fade = FALSE;

	if (HasHumanGibs())
	{
		if (!CVAR_GET_FLOAT("violence_hgibs"))
			fade = TRUE;
	}
	else if (HasAlienGibs())
	{
		if (!CVAR_GET_FLOAT("violence_agibs"))
			fade = TRUE;
	}

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;

	if (!fade)
	{
		pev->effects = EF_NODRAW;
		GibMonster();
	}
	else
		FadeMonster();

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	if (pev->health < -99)
		pev->health = 0;

	if (ShouldFadeOnDeath() && !fade)
		UTIL_Remove(this);
}

void CBaseMonster::Killed(entvars_t *pevAttacker, int iGib)
{
	if (HasMemory(bits_MEMORY_KILLED))
	{
		if (ShouldGibMonster(iGib))
			CallGibMonster();

		return;
	}

	Remember(bits_MEMORY_KILLED);
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/null.wav", VOL_NORM, ATTN_NORM);
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	SetConditions(bits_COND_LIGHT_DAMAGE);

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);

	if (pOwner)
		pOwner->DeathNotice(pev);

	if (ShouldGibMonster(iGib))
	{
		CallGibMonster();
		return;
	}
	else if (pev->flags & FL_MONSTER)
	{
		SetTouch(NULL);
		BecomeDead();
	}

	if (pev->health < -99)
		pev->health = 0;

	m_IdealMonsterState = MONSTERSTATE_DEAD;
}

void CBaseEntity::SUB_StartFadeOut(void)
{
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CBaseEntity::SUB_FadeOut);
}

void CBaseEntity::SUB_FadeOut(void)
{
	if (pev->renderamt > 7)
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2;
		SetThink(&CBaseEntity::SUB_Remove);
	}
}

void CGib::WaitTillLand(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->velocity == g_vecZero)
	{
		SetThink(&CBaseEntity::SUB_StartFadeOut);
		pev->nextthink = gpGlobals->time + m_lifeTime;

		if (m_bloodColor != DONT_BLEED)
			CSoundEnt::InsertSound(bits_SOUND_MEAT, pev->origin, 384, 25);
	}
	else
		pev->nextthink = gpGlobals->time + 0.5;
}

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
//GCC почему-то не видит этот дефайн в extdll.h
void CGib::BounceGibTouch(CBaseEntity *pOther)
{
	if (pev->flags & FL_ONGROUND)
	{
		pev->velocity = pev->velocity * 0.9;
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->avelocity.x = 0;
		pev->avelocity.z = 0;
	}
	else
	{
		if (g_Language != LANGUAGE_GERMAN && m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED)
		{
			TraceResult tr;
			Vector vecSpot = pev->origin + Vector(0, 0, 8);
			UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -24), ignore_monsters, ENT(pev), &tr);
			UTIL_BloodDecalTrace(&tr, m_bloodColor);
			m_cBloodDecals--;
		}

		if (m_material != matNone && !RANDOM_LONG(0, 2))
		{
			float zvel = fabs(pev->velocity.z);
			float volume = 0.8 * min(1, zvel / 450);
			CBreakable::MaterialSoundRandom(edict(), (Materials)m_material, volume);
		}
	}
}

void CGib::StickyGibTouch(CBaseEntity *pOther)
{
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 10;

	if (!FClassnameIs(pOther->pev, "worldspawn"))
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 32, ignore_monsters, ENT(pev), &tr);
	UTIL_BloodDecalTrace(&tr, m_bloodColor);

	pev->velocity = tr.vecPlaneNormal * -1;
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

void CGib::Spawn(const char *szGibModel)
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55;
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX;

	if (pev->classname)
		RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->classname = MAKE_STRING("gib");
	AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	SET_MODEL(ENT(pev), szGibModel);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->nextthink = gpGlobals->time + 4;
	m_lifeTime = 25;
	SetThink(&CGib::WaitTillLand);
	SetTouch(&CGib::BounceGibTouch);
	m_material = matNone;
	m_cBloodDecals = 5;
}

int CBaseMonster::TakeHealth(float flHealth, int bitsDamageType)
{
	if (!pev->takedamage)
		return 0;

	m_bitsDamageType &= ~(bitsDamageType & ~DMG_TIMEBASED);
	return CBaseEntity::TakeHealth(flHealth, bitsDamageType);
}

int CBaseMonster::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flTake;
	Vector vecDir;

	if (!pev->takedamage)
		return 0;

	if (!IsAlive())
		return DeadTakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	if (pev->deadflag == DEAD_NO)
		PainSound();

	flTake = flDamage;
	m_bitsDamageType |= bitsDamageType;
	vecDir = Vector(0, 0, 0);

	if (!FNullEnt(pevInflictor))
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance(pevInflictor);

		if (pInflictor)
		{
			vecDir = (pInflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	if (IsPlayer())
	{
		if (pevInflictor)
			pev->dmg_inflictor = ENT(pevInflictor);

		pev->dmg_take += flTake;
	}

	pev->health -= flTake;

	if (m_MonsterState == MONSTERSTATE_SCRIPT)
	{
		SetConditions(bits_COND_LIGHT_DAMAGE);
		return 0;
	}

	if (pev->health <= 0)
	{
		g_pevLastInflictor = pevInflictor;

		if (bitsDamageType & DMG_ALWAYSGIB)
			Killed(pevAttacker, GIB_ALWAYS);
		else if (bitsDamageType & DMG_NEVERGIB)
			Killed(pevAttacker, GIB_NEVER);
		else
			Killed(pevAttacker, GIB_NORMAL);

		g_pevLastInflictor = NULL;
		return 0;
	}

	if ((pev->flags & FL_MONSTER) && !FNullEnt(pevAttacker))
	{
		if (pevAttacker->flags & (FL_MONSTER | FL_CLIENT))
		{
			if (pevInflictor)
			{
				if (m_hEnemy == NULL || pevInflictor == m_hEnemy->pev || !HasConditions(bits_COND_SEE_ENEMY))
					m_vecEnemyLKP = pevInflictor->origin;
			}
			else
				m_vecEnemyLKP = pev->origin + (g_vecAttackDir * 64);

			MakeIdealYaw(m_vecEnemyLKP);

			if (flDamage > 0)
				SetConditions(bits_COND_LIGHT_DAMAGE);

			if (flDamage >= 20)
				SetConditions(bits_COND_HEAVY_DAMAGE);
		}
	}

	return 1;
}

int CBaseMonster::DeadTakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	Vector vecDir = Vector(0, 0, 0);

	if (!FNullEnt(pevInflictor))
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance(pevInflictor);

		if (pInflictor)
		{
			vecDir = (pInflictor->Center() - Vector(0, 0, 10) - Center()).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	if (bitsDamageType & DMG_GIB_CORPSE)
	{
		if (pev->health <= flDamage)
		{
			pev->health = -50;
			Killed(pevAttacker, GIB_ALWAYS);
			return 0;
		}

		pev->health -= flDamage * 0.1;
	}

	return 1;
}

float CBaseMonster::DamageForce(float damage)
{
	float force = damage * ((32 * 32 * 72.0) / (pev->size.x * pev->size.y * pev->size.z)) * 5;

	if (force > 1000)
		force = 1000;

	return force;
}

void RadiusFlash(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage)
{
	CBaseEntity *pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;
	float flRadius = 1500;

	if (flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1;

	int bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1;

	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		TraceResult tr2;
		Vector vecLOS;
		float flDot;
		float fadeTime;
		float fadeHold;
		int alpha;
		CBasePlayer *pPlayer;
		float currentHoldTime;

		if (!pEntity->IsPlayer())
			break;

		pPlayer = (CBasePlayer *)pEntity;

		if (pPlayer->pev->takedamage == DAMAGE_NO || pPlayer->pev->deadflag != DEAD_NO)
			continue;

		if (bInWater && !pPlayer->pev->waterlevel)
			continue;

		if (!bInWater && pPlayer->pev->waterlevel == 3)
			continue;

		vecSpot = pPlayer->BodyTarget(vecSrc);
		UTIL_TraceLine(vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr);

		if (tr.flFraction == 1 || tr.pHit == pPlayer->edict())
		{
			UTIL_TraceLine(vecSpot, vecSrc, dont_ignore_monsters, tr.pHit, &tr2);

			if (tr2.flFraction < 1)
				continue;

			if (tr.fStartSolid)
			{
				tr.vecEndPos = vecSrc;
				tr.flFraction = 0;
			}

			flAdjustedDamage = (vecSrc - tr2.vecEndPos).Length() * falloff;
			flAdjustedDamage = flDamage - flAdjustedDamage;

			if (flAdjustedDamage < 0)
				flAdjustedDamage = 0;

			UTIL_MakeVectors(pPlayer->pev->v_angle);
			vecLOS = vecSrc - pPlayer->EarPosition();
			flDot = DotProduct(vecLOS, gpGlobals->v_forward);

			if (flDot >= 0)
			{
				fadeTime = flAdjustedDamage * 3;
				fadeHold = flAdjustedDamage / 1.5;
				alpha = 255;
			}
			else
			{
				fadeTime = flAdjustedDamage * 1.75;
				fadeHold = flAdjustedDamage / 3.5;
				alpha = 200;
			}

			currentHoldTime = pPlayer->m_blindStartTime + pPlayer->m_blindHoldTime - gpGlobals->time;

			if (currentHoldTime > 0 && alpha == 255)
				fadeHold += currentHoldTime;

			if (pPlayer->m_blindStartTime != 0 && pPlayer->m_blindFadeTime != 0)
			{
				if (pPlayer->m_blindStartTime + pPlayer->m_blindFadeTime + pPlayer->m_blindHoldTime > gpGlobals->time)
				{
					if (pPlayer->m_blindFadeTime > fadeTime)
						fadeTime = pPlayer->m_blindFadeTime;

					if (pPlayer->m_blindAlpha > alpha)
						alpha = pPlayer->m_blindAlpha;
				}
			}

			UTIL_ScreenFade(pPlayer, Vector(255, 255, 255), fadeTime, fadeHold, alpha, 0);
			currentHoldTime = 1;

			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CBasePlayer *pObserver = (CBasePlayer *)UTIL_PlayerByIndex(i);

				if (pObserver && pObserver->IsObservingPlayer(pPlayer) && fadetoblack.value == 0)
					UTIL_ScreenFade(pPlayer, Vector(255, 255, 255), fadeTime, fadeHold, alpha, 0);

				currentHoldTime++;
			}

			pPlayer->Blind(fadeTime / 3, fadeHold, fadeTime, alpha);
		}
	}
}

void RadiusDamage(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType)
{
	CBaseEntity *pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;

	if (flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1;

	int bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1;

	if (!pevAttacker)
		pevAttacker = pevInflictor;

	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		if (pEntity->pev->takedamage == DAMAGE_NO)
			continue;

		if (iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore)
			continue;

		if (bInWater && !pEntity->pev->waterlevel)
			continue;

		if (!bInWater && pEntity->pev->waterlevel == 3)
			continue;

		flAdjustedDamage = (vecSrc - pEntity->pev->origin).Length() * falloff;
		flAdjustedDamage = flDamage - flAdjustedDamage;

		if (flAdjustedDamage < 0)
			flAdjustedDamage = 0;

		pEntity->TakeDamage(pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType);
	}
}

void RadiusDamage2(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType)
{
	CBaseEntity *pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage, falloff;
	Vector vecSpot;

	if (flRadius)
		falloff = flDamage / flRadius;
	else
		falloff = 1;

	int bInWater = (UTIL_PointContents(vecSrc) == CONTENTS_WATER);

	vecSrc.z += 1;

	if (!pevAttacker)
		pevAttacker = pevInflictor;

	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecSrc, flRadius)) != NULL)
	{
		if (pEntity->pev->takedamage == DAMAGE_NO)
			continue;

		if (iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore)
			continue;

		if (bInWater && !pEntity->pev->waterlevel)
			continue;

		if (!bInWater && pEntity->pev->waterlevel == 3)
			continue;

		if (!pEntity->IsPlayer())
			break;

		vecSpot = pEntity->BodyTarget(vecSrc);
		UTIL_TraceLine(vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr);

		if (tr.flFraction == 1 || tr.pHit == pEntity->edict())
		{
			if (tr.fStartSolid)
			{
				tr.vecEndPos = vecSrc;
				tr.flFraction = 0;
			}

			flAdjustedDamage = (vecSrc - pEntity->pev->origin).Length() * falloff;
			flAdjustedDamage = flDamage - flAdjustedDamage;

			if (flAdjustedDamage < 0)
				flAdjustedDamage = 0;
			else if (flAdjustedDamage > 75)
				flAdjustedDamage = 75;

			if (tr.flFraction != 1)
			{
				ClearMultiDamage();
				pEntity->TraceAttack(pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize(), &tr, bitsDamageType);
				ApplyMultiDamage(pevInflictor, pevAttacker);
			}
			else
				pEntity->TakeDamage(pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType);
		}
	}
}

void CBaseMonster::RadiusDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType)
{
	if (flDamage <= 80)
		::RadiusDamage2(pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * (RANDOM_FLOAT(0.5, 1.5) + 3), iClassIgnore, bitsDamageType);
	else
		::RadiusDamage(pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * 3.5, iClassIgnore, bitsDamageType);
}

void CBaseMonster::RadiusDamage(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType)
{
	if (flDamage <= 80)
		::RadiusDamage2(vecSrc, pevInflictor, pevAttacker, flDamage, flDamage * (RANDOM_FLOAT(0.5, 1.5) + 3), iClassIgnore, bitsDamageType);
	else
		::RadiusDamage(vecSrc, pevInflictor, pevAttacker, flDamage, flDamage * 3.5, iClassIgnore, bitsDamageType);
}

CBaseEntity *CBaseMonster::CheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	if (IsPlayer())
		UTIL_MakeVectors(pev->angles);
	else
		UTIL_MakeAimVectors(pev->angles);

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist);

	TraceResult tr;
	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if (iDamage > 0)
			pEntity->TakeDamage(pev, pev, iDamage, iDmgType);

		return pEntity;
	}

	return NULL;
}

BOOL CBaseMonster::FInViewCone(CBaseEntity *pEntity)
{
	UTIL_MakeVectors(pev->angles);

	Vector2D vec2LOS = (pEntity->pev->origin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();
	float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > m_flFieldOfView)
		return TRUE;

	return FALSE;
}

BOOL CBaseMonster::FInViewCone(Vector *pOrigin)
{
	UTIL_MakeVectors(pev->angles);

	Vector2D vec2LOS = (*pOrigin - pev->origin).Make2D();
	vec2LOS = vec2LOS.Normalize();
	float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > m_flFieldOfView)
		return TRUE;

	return FALSE;
}

BOOL CBaseEntity::FVisible(CBaseEntity *pEntity)
{
	if (FBitSet(pEntity->pev->flags, FL_NOTARGET))
		return FALSE;

	if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) || (pev->waterlevel == 3 && !pEntity->pev->waterlevel))
		return FALSE;

	TraceResult tr;
	Vector vecLookerOrigin = pev->origin + pev->view_ofs;
	Vector vecTargetOrigin = pEntity->EyePosition();
	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(pev), &tr);

	if (tr.flFraction != 1)
		return FALSE;

	return TRUE;
}

BOOL CBaseEntity::FVisible(const Vector &vecOrigin)
{
	TraceResult tr;
	Vector vecLookerOrigin = EyePosition();
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT(pev), &tr);

	if (tr.flFraction != 1)
		return FALSE;

	return TRUE;
}

void CBaseEntity::TraceAttack(entvars_t *pevAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	if (!pev->takedamage)
		return;

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);

	int blood = BloodColor();

	if (blood != DONT_BLEED)
	{
		SpawnBlood(vecOrigin, blood, flDamage);
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
	}
}

void CBaseMonster::TraceAttack(entvars_t *pevAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (!pev->takedamage)
		return;

	m_LastHitGroup = ptr->iHitgroup;

	switch (ptr->iHitgroup)
	{
		case HITGROUP_GENERIC: break;
		case HITGROUP_HEAD: flDamage *= 3; break;
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH: flDamage *= 1.5; break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM: flDamage *= 1.0; break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG: flDamage *= 0.75; break;
		case HITGROUP_SHIELD: flDamage = 0; break;

		default: break;
	}

	CBaseEntity::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CBaseEntity::FireBullets(ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker)
{
	static int tracerCount;
	int tracer;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	if (!pevAttacker)
		pevAttacker = pev;

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for (ULONG iShot = 1; iShot <= cShots; iShot++)
	{
		float x, y, z;

		do
		{
			x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			z = x * x + y * y;
		}
		while (z > 1);

		TraceResult tr;
		Vector vecDir = vecDirShooting + x * vecSpread.x * vecRight + y * vecSpread.y * vecUp;
		Vector vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

		tracer = 0;

		if (iTracerFreq != 0 && !(tracerCount++ % iTracerFreq))
		{
			Vector vecTracerSrc;

			if (IsPlayer())
				vecTracerSrc = vecSrc + Vector(0, 0, -4) + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16;
			else
				vecTracerSrc = vecSrc;

			if (iTracerFreq != 1)
				tracer = 1;

			MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecTracerSrc);
			WRITE_BYTE(TE_TRACER);
			WRITE_COORD(vecTracerSrc.x);
			WRITE_COORD(vecTracerSrc.y);
			WRITE_COORD(vecTracerSrc.z);
			WRITE_COORD(tr.vecEndPos.x);
			WRITE_COORD(tr.vecEndPos.y);
			WRITE_COORD(tr.vecEndPos.z);
			MESSAGE_END();
		}

		if (tr.flFraction != 1)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if (iDamage)
			{
				pEntity->TraceAttack(pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB));
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot(&tr, iBulletType, FALSE, pev, FALSE);
			}
			else switch (iBulletType)
			{
				case BULLET_PLAYER_MP5:
				{
					pEntity->TraceAttack(pevAttacker, gSkillData.plrDmgMP5, vecDir, &tr, DMG_BULLET);
					break;
				}

				case BULLET_PLAYER_BUCKSHOT:
				{
					pEntity->TraceAttack(pevAttacker, (int)((1 - tr.flFraction) * 20), vecDir, &tr, DMG_BULLET);
					break;
				}

				case BULLET_PLAYER_357:
				{
					pEntity->TraceAttack(pevAttacker, gSkillData.plrDmg357, vecDir, &tr, DMG_BULLET);
					break;
				}

				case BULLET_MONSTER_9MM:
				{
					pEntity->TraceAttack(pevAttacker, gSkillData.monDmg9MM, vecDir, &tr, DMG_BULLET);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType, false, pev, false);
					break;
				}

				case BULLET_MONSTER_MP5:
				{
					pEntity->TraceAttack(pevAttacker, gSkillData.monDmgMP5, vecDir, &tr, DMG_BULLET);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
					DecalGunshot(&tr, iBulletType, false, pev, false);
					break;
				}

				case BULLET_MONSTER_12MM:
				{
					pEntity->TraceAttack(pevAttacker, gSkillData.monDmg12MM, vecDir, &tr, DMG_BULLET);

					if (!tracer)
					{
						TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
						DecalGunshot(&tr, iBulletType, false, pev, false);
					}

					break;
				}

				case BULLET_NONE:
				{
					pEntity->TraceAttack(pevAttacker, 50, vecDir, &tr, DMG_CLUB);
					TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);

					if (!FNullEnt(tr.pHit) && VARS(tr.pHit)->rendermode != 0)
						UTIL_DecalTrace(&tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0, 2));

					break;
				}

				default:
				{
					pEntity->TraceAttack(pevAttacker, gSkillData.monDmg9MM, vecDir, &tr, DMG_BULLET);
					break;
				}
			}
		}

		UTIL_BubbleTrail(vecSrc, tr.vecEndPos, (int)((flDistance * tr.flFraction) / 64));
	}

	ApplyMultiDamage(pev, pevAttacker);
}

#pragma optimize("", off)

Vector CBaseEntity::FireBullets3(Vector vecSrc, Vector vecDirShooting, float flSpread, float flDistance, int iPenetration, int iBulletType, int iDamage, float flRangeModifier, entvars_t *pevAttacker, bool bPistol, int shared_rand)
{
	int iOriginalPenetration = iPenetration;
	int iPenetrationPower;
	float flPenetrationDistance;
	int iCurrentDamage = iDamage;
	float flCurrentDistance;
	TraceResult tr, tr2;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	CBaseEntity *pEntity;
	bool bHitMetal = false;
	int iSparksAmount;

	switch (iBulletType)
	{
		case BULLET_PLAYER_9MM:
		{
			iPenetrationPower = 21;
			flPenetrationDistance = 800;
			iSparksAmount = 15;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		case BULLET_PLAYER_45ACP:
		{
			iPenetrationPower = 15;
			flPenetrationDistance = 500;
			iSparksAmount = 20;
			iCurrentDamage += (-2 + RANDOM_LONG(0, 4));
			break;
		}

		case BULLET_PLAYER_50AE:
		{
			iPenetrationPower = 30;
			flPenetrationDistance = 1000;
			iSparksAmount = 20;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		case BULLET_PLAYER_762MM:
		{
			iPenetrationPower = 39;
			flPenetrationDistance = 5000;
			iSparksAmount = 30;
			iCurrentDamage += (-2 + RANDOM_LONG(0, 4));
			break;
		}

		case BULLET_PLAYER_556MM:
		{
			iPenetrationPower = 35;
			flPenetrationDistance = 4000;
			iSparksAmount = 30;
			iCurrentDamage += (-3 + RANDOM_LONG(0, 6));
			break;
		}

		case BULLET_PLAYER_338MAG:
		{
			iPenetrationPower = 45;
			flPenetrationDistance = 8000;
			iSparksAmount = 30;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 8));
			break;
		}

		case BULLET_PLAYER_57MM:
		{
			iPenetrationPower = 30;
			flPenetrationDistance = 2000;
			iSparksAmount = 20;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		case BULLET_PLAYER_357SIG:
		{
			iPenetrationPower = 25;
			flPenetrationDistance = 800;
			iSparksAmount = 20;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		default:
		{
			iPenetrationPower = 0;
			flPenetrationDistance = 0;
			break;
		}
	}

	if (!pevAttacker)
		pevAttacker = pev;

	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	float x, y, z;

	if (IsPlayer())
	{
		x = UTIL_SharedRandomFloat(shared_rand, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + 1, -0.5, 0.5);
		y = UTIL_SharedRandomFloat(shared_rand + 2, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + 3, -0.5, 0.5);
	}
	else
	{
		do
		{
			x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			z = x * x + y * y;
		}
		while (z > 1);
	}

	Vector vecDir = vecDirShooting + x * flSpread * vecRight + y * flSpread * vecUp;
	Vector vecEnd = vecSrc + vecDir * flDistance;
	Vector vecOldSrc;
	Vector vecNewSrc;
	float flDamageModifier = 0.5;

	while (iPenetration != 0)
	{
		ClearMultiDamage();
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

		char cTextureType = UTIL_TextureHit(&tr, vecSrc, vecEnd);
		bool bSparks = false;

		switch (cTextureType)
		{
			case CHAR_TEX_METAL:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.15;
				flDamageModifier = 0.2;
				break;
			}

			case CHAR_TEX_CONCRETE:
			{
				iPenetrationPower *= 0.25;
				flDamageModifier = 0.25;
				break;
			}

			case CHAR_TEX_GRATE:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.5;
				flDamageModifier = 0.4;
				break;
			}

			case CHAR_TEX_VENT:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.5;
				flDamageModifier = 0.45;
				break;
			}

			case CHAR_TEX_TILE:
			{
				iPenetrationPower *= 0.65;
				flDamageModifier = 0.3;
				break;
			}

			case CHAR_TEX_COMPUTER:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.4;
				flDamageModifier = 0.45;
				break;
			}

			case CHAR_TEX_WOOD:
			{
				iPenetrationPower *= 1;
				flDamageModifier = 0.6;
				break;
			}

			default:
			{
				bSparks = false;
				break;
			}
		}

		if (tr.flFraction != 1.0)
		{
			pEntity = CBaseEntity::Instance(tr.pHit);
			iPenetration--;
			flCurrentDistance = tr.flFraction * flDistance;
			iCurrentDamage *= pow(flRangeModifier, flCurrentDistance / 500);

			if (flCurrentDistance > flPenetrationDistance)
				iPenetration = 0;

			if (tr.iHitgroup == HITGROUP_SHIELD)
			{
				iPenetration = 0;

				if (tr.flFraction != 1.0)
				{
					if (RANDOM_LONG(0, 1))
						EMIT_SOUND(pEntity->edict(), CHAN_VOICE, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
					else
						EMIT_SOUND(pEntity->edict(), CHAN_VOICE, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

					UTIL_Sparks(tr.vecEndPos);

					pEntity->pev->punchangle.x = iCurrentDamage * RANDOM_FLOAT(-0.15, 0.15);
					pEntity->pev->punchangle.z = iCurrentDamage * RANDOM_FLOAT(-0.15, 0.15);

					if (pEntity->pev->punchangle.x < 4)
						pEntity->pev->punchangle.x = 4;

					if (pEntity->pev->punchangle.z < -5)
						pEntity->pev->punchangle.z = -5;
					else if (pEntity->pev->punchangle.z > 5)
						pEntity->pev->punchangle.z = 5;
				}

				break;
			}

			if ((VARS(tr.pHit)->solid == SOLID_BSP) && (iPenetration != 0))
			{
				if (bPistol)
					DecalGunshot(&tr, iBulletType, false, pev, bHitMetal);
				else if (RANDOM_LONG(0, 3))
					DecalGunshot(&tr, iBulletType, true, pev, bHitMetal);

				vecSrc = tr.vecEndPos + (vecDir * iPenetrationPower);
				flDistance = (flDistance - flCurrentDistance) * 0.5;
				vecEnd = vecSrc + (vecDir * flDistance);

				pEntity->TraceAttack(pevAttacker, iCurrentDamage, vecDir, &tr, DMG_BULLET | DMG_NEVERGIB);

				iCurrentDamage *= flDamageModifier;
			}
			else
			{
				if (bPistol)
					DecalGunshot(&tr, iBulletType, false, pev, bHitMetal);
				else if (RANDOM_LONG(0, 3))
					DecalGunshot(&tr, iBulletType, true, pev, bHitMetal);

				vecSrc = tr.vecEndPos + (vecDir * 42);
				flDistance = (flDistance - flCurrentDistance) * 0.75;
				vecEnd = vecSrc + (vecDir * flDistance);

				pEntity->TraceAttack(pevAttacker, iCurrentDamage, vecDir, &tr, DMG_BULLET | DMG_NEVERGIB);

				iCurrentDamage *= 0.75;
			}
		}
		else
			iPenetration = 0;

		ApplyMultiDamage(pev, pevAttacker);
	}

	return Vector(x * flSpread, y * flSpread, 0);
}

#pragma optimize("", on)

void CBaseEntity::TraceBleed(float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (BloodColor() == DONT_BLEED)
		return;

	if (!flDamage)
		return;

	if (!(bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR)))
		return;

	float flNoise;
	int cCount;

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for (int i = 0; i < cCount; i++)
	{
		Vector vecTraceDir = vecDir * -1;
		vecTraceDir.x += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.y += RANDOM_FLOAT(-flNoise, flNoise);
		vecTraceDir.z += RANDOM_FLOAT(-flNoise, flNoise);

		TraceResult Bloodtr;
		UTIL_TraceLine(ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, ENT(pev), &Bloodtr);

		if (Bloodtr.flFraction != 1)
		{
			if (!RANDOM_LONG(0, 2))
				UTIL_BloodDecalTrace(&Bloodtr, BloodColor());
		}
	}
}

void CBaseMonster::BloodSplat(const Vector &vecPos, const Vector &vecDir, int hitgroup, int iDamage)
{
	if (hitgroup != HITGROUP_HEAD)
		return;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecPos);
	WRITE_BYTE(TE_BLOODSTREAM);
	WRITE_COORD(vecPos.x);
	WRITE_COORD(vecPos.y);
	WRITE_COORD(vecPos.z);
	WRITE_COORD(vecDir.x);
	WRITE_COORD(vecDir.y);
	WRITE_COORD(vecDir.z);
	WRITE_BYTE(223);
	WRITE_BYTE(iDamage + RANDOM_LONG(0, 100));
	MESSAGE_END();
}

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

extern DLL_GLOBAL Vector g_vecAttackDir;

#define SF_BRUSH_ACCDCC 16
#define SF_BRUSH_HURT 32
#define SF_ROTATING_NOT_SOLID 64

#define noiseStart noise1
#define noiseStop noise2
#define noiseRunning noise3

#define SF_PENDULUM_SWING 2

Vector VecBModelOrigin(entvars_t *pevBModel)
{
	return pevBModel->absmin + (pevBModel->size * 0.5);
}

class CFuncWall : public CBaseEntity
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(func_wall, CFuncWall);

void CFuncWall::Spawn(void)
{
	pev->angles = g_vecZero;
	pev->movetype = MOVETYPE_PUSH;
	pev->solid = SOLID_BSP;
	SET_MODEL(ENT(pev), STRING(pev->model));

	pev->flags |= FL_WORLDBRUSH;
}

void CFuncWall::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (ShouldToggle(useType, (int)(pev->frame)))
		pev->frame = 1 - pev->frame;
}

#define SF_WALL_START_OFF 0x0001

class CFuncWallToggle : public CFuncWall
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

public:
	void TurnOff(void);
	void TurnOn(void);
	BOOL IsOn(void);
};

LINK_ENTITY_TO_CLASS(func_wall_toggle, CFuncWallToggle);

void CFuncWallToggle::Spawn(void)
{
	CFuncWall::Spawn();

	if (pev->spawnflags & SF_WALL_START_OFF)
		TurnOff();
}

void CFuncWallToggle::TurnOff(void)
{
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	UTIL_SetOrigin(pev, pev->origin);
}

void CFuncWallToggle::TurnOn(void)
{
	pev->solid = SOLID_BSP;
	pev->effects &= ~EF_NODRAW;
	UTIL_SetOrigin(pev, pev->origin);
}

BOOL CFuncWallToggle::IsOn(void)
{
	if (pev->solid == SOLID_NOT)
		return FALSE;

	return TRUE;
}

void CFuncWallToggle::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int status = IsOn();

	if (ShouldToggle(useType, status))
	{
		if (status)
			TurnOff();
		else
			TurnOn();
	}
}

#define SF_CONVEYOR_VISUAL 0x0001
#define SF_CONVEYOR_NOTSOLID 0x0002

class CFuncConveyor : public CFuncWall
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void UpdateSpeed(float speed);
};

LINK_ENTITY_TO_CLASS(func_conveyor, CFuncConveyor);

void CFuncConveyor::Spawn(void)
{
	SetMovedir(pev);
	CFuncWall::Spawn();

	if (!(pev->spawnflags & SF_CONVEYOR_VISUAL))
		SetBits(pev->flags, FL_CONVEYOR);

	if (pev->spawnflags & SF_CONVEYOR_NOTSOLID)
	{
		pev->solid = SOLID_NOT;
		pev->skin = 0;
	}

	if (!pev->speed)
		pev->speed = 100;

	UpdateSpeed(pev->speed);
}

void CFuncConveyor::UpdateSpeed(float speed)
{
	int speedCode = (int)(fabs(speed) * 16.0);

	if (speed < 0)
		pev->rendercolor.x = 1;
	else
		pev->rendercolor.x = 0;

	pev->rendercolor.y = (speedCode >> 8);
	pev->rendercolor.z = (speedCode & 0xFF);
}

void CFuncConveyor::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	pev->speed = -pev->speed;
	UpdateSpeed(pev->speed);
}

class CFuncIllusionary : public CBaseToggle
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	int ObjectCaps(void){ return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

public:
	void EXPORT SloshTouch(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(func_illusionary, CFuncIllusionary);

void CFuncIllusionary::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "skin"))
	{
		pev->skin = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

void CFuncIllusionary::Spawn(void)
{
	pev->angles = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	SET_MODEL(ENT(pev), STRING(pev->model));
}

class CFuncMonsterClip : public CFuncWall
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {}
};

LINK_ENTITY_TO_CLASS(func_monsterclip, CFuncMonsterClip);

void CFuncMonsterClip::Spawn(void)
{
	CFuncWall::Spawn();

	if (CVAR_GET_FLOAT("showtriggers") == 0)
		pev->effects = EF_NODRAW;

	pev->flags |= FL_MONSTERCLIP;
}

class CFuncRotating : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	void RampPitchVol(int fUp);
	void Blocked(CBaseEntity *pOther);
	int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT SpinUp(void);
	void EXPORT SpinDown(void);
	void EXPORT HurtTouch(CBaseEntity *pOther);
	void EXPORT RotatingUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT Rotate(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_flFanFriction;
	float m_flAttenuation;
	float m_flVolume;
	float m_pitch;
	int m_sounds;
};

TYPEDESCRIPTION CFuncRotating::m_SaveData[] =
{
	DEFINE_FIELD(CFuncRotating, m_flFanFriction, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_flAttenuation, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_flVolume, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_pitch, FIELD_FLOAT),
	DEFINE_FIELD(CFuncRotating, m_sounds, FIELD_INTEGER)
};

IMPLEMENT_SAVERESTORE(CFuncRotating, CBaseEntity);
LINK_ENTITY_TO_CLASS(func_rotating, CFuncRotating);

void CFuncRotating::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "fanfriction"))
	{
		m_flFanFriction = atof(pkvd->szValue) / 100;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Volume"))
	{
		m_flVolume = atof(pkvd->szValue) / 10;

		if (m_flVolume > 1)
			m_flVolume = 1;

		if (m_flVolume < 0)
			m_flVolume = 0;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spawnorigin"))
	{
		Vector tmp;
		UTIL_StringToVector((float *)tmp, pkvd->szValue);

		if (tmp != g_vecZero)
			pev->origin = tmp;
	}
	else if (FStrEq(pkvd->szKeyName, "sounds"))
	{
		m_sounds = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CFuncRotating::Spawn(void)
{
	m_pitch = PITCH_NORM - 1;

	if (m_flVolume == 0.0)
		m_flVolume = 1;

	m_flAttenuation = ATTN_NORM;

	if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_SMALLRADIUS))
		m_flAttenuation = ATTN_IDLE;
	else if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_MEDIUMRADIUS))
		m_flAttenuation = ATTN_STATIC;
	else if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_LARGERADIUS))
		m_flAttenuation = ATTN_NORM;

	if (m_flFanFriction == 0)
		m_flFanFriction = 1;

	if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_Z_AXIS))
		pev->movedir = Vector(0, 0, 1);
	else if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_X_AXIS))
		pev->movedir = Vector(1, 0, 0);
	else
		pev->movedir = Vector(0, 1, 0);

	if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_BACKWARDS))
		pev->movedir = pev->movedir * -1;

	if (FBitSet(pev->spawnflags, SF_ROTATING_NOT_SOLID))
	{
		pev->solid = SOLID_NOT;
		pev->skin = CONTENTS_EMPTY;
		pev->movetype = MOVETYPE_PUSH;
	}
	else
	{
		pev->solid = SOLID_BSP;
		pev->movetype = MOVETYPE_PUSH;
	}

	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), STRING(pev->model));
	SetUse(&CFuncRotating::RotatingUse);

	if (pev->speed <= 0)
		pev->speed = 0;

	if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_INSTANT))
	{
		SetThink(&CBaseEntity::SUB_CallUseToggle);
		pev->nextthink = pev->ltime + 1.5;
	}

	if (FBitSet(pev->spawnflags, SF_BRUSH_HURT))
		SetTouch(&CFuncRotating::HurtTouch);

	Precache();
}

void CFuncRotating::Precache(void)
{
	char *szSoundFile = (char *)STRING(pev->message);

	if (!FStringNull(pev->message) && szSoundFile[0] != '\0')
	{
		PRECACHE_SOUND(szSoundFile);
		pev->noiseRunning = ALLOC_STRING(szSoundFile);
	}
	else
	{
		switch (m_sounds)
		{
			case 1:
			{
				PRECACHE_SOUND("fans/fan1.wav");
				pev->noiseRunning = ALLOC_STRING("fans/fan1.wav");
				break;
			}

			case 2:
			{
				PRECACHE_SOUND("fans/fan2.wav");
				pev->noiseRunning = ALLOC_STRING("fans/fan2.wav");
				break;
			}

			case 3:
			{
				PRECACHE_SOUND("fans/fan3.wav");
				pev->noiseRunning = ALLOC_STRING("fans/fan3.wav");
				break;
			}

			case 4:
			{
				PRECACHE_SOUND("fans/fan4.wav");
				pev->noiseRunning = ALLOC_STRING("fans/fan4.wav");
				break;
			}

			case 5:
			{
				PRECACHE_SOUND("fans/fan5.wav");
				pev->noiseRunning = ALLOC_STRING("fans/fan5.wav");
				break;
			}

			case 0:
			default:
			{
				if (!FStringNull(pev->message) && szSoundFile[0]!='\0')
				{
					PRECACHE_SOUND(szSoundFile);
					pev->noiseRunning = ALLOC_STRING(szSoundFile);
					break;
				}
				else
				{
					pev->noiseRunning = ALLOC_STRING("common/null.wav");
					break;
				}
			}
		}
	}

	if (pev->avelocity != g_vecZero)
	{
		SetThink(&CFuncRotating::SpinUp);
		pev->nextthink = pev->ltime + 1.5;
	}
}

void CFuncRotating::HurtTouch(CBaseEntity *pOther)
{
	entvars_t *pevOther = pOther->pev;

	if (!pevOther->takedamage)
		return;

	pev->dmg = pev->avelocity.Length() / 10;
	pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);
	pevOther->velocity = (pevOther->origin - VecBModelOrigin(pev)).Normalize() * pev->dmg;
}

#define FANPITCHMIN 30
#define FANPITCHMAX 100

void CFuncRotating::RampPitchVol(int fUp)
{
	Vector vecAVel = pev->avelocity;
	vec_t vecCur = abs(vecAVel.x != 0 ? (int)vecAVel.x : (int)(vecAVel.y != 0 ? vecAVel.y : vecAVel.z));
	vec_t vecFinal = (pev->movedir.x != 0 ? pev->movedir.x : (pev->movedir.y != 0 ? pev->movedir.y : pev->movedir.z));
	vecFinal *= pev->speed;
	vecFinal = abs((int)vecFinal);

	float fpct = vecCur / vecFinal;
	float fvol = m_flVolume * fpct;
	float fpitch = FANPITCHMIN + (FANPITCHMAX - FANPITCHMIN) * fpct;
	int pitch = (int)fpitch;

	if (pitch == PITCH_NORM)
		pitch = PITCH_NORM - 1;

	EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseRunning), fvol, m_flAttenuation, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
}

void CFuncRotating::SpinUp(void)
{
	pev->nextthink = pev->ltime + 0.1;
	pev->avelocity = pev->avelocity + (pev->movedir * (pev->speed * m_flFanFriction));

	Vector vecAVel = pev->avelocity;

	if (fabs((int)vecAVel.x) >= fabs((int)pev->movedir.x * pev->speed) && abs((int)vecAVel.y) >= fabs((int)pev->movedir.y * pev->speed) && abs((int)vecAVel.z) >= fabs((int)pev->movedir.z * pev->speed))
	{
		pev->avelocity = pev->movedir * pev->speed;
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseRunning), m_flVolume, m_flAttenuation, SND_CHANGE_PITCH | SND_CHANGE_VOL, FANPITCHMAX);
		SetThink(&CFuncRotating::Rotate);
		Rotate();
	}
	else
		RampPitchVol(TRUE);
}

void CFuncRotating::SpinDown(void)
{
	pev->nextthink = pev->ltime + 0.1;
	pev->avelocity = pev->avelocity - (pev->movedir *(pev->speed * m_flFanFriction));

	Vector vecAVel = pev->avelocity;
	vec_t vecdir = (pev->movedir.x != 0) ? vecdir = pev->movedir.x : (pev->movedir.y != 0) ? vecdir = pev->movedir.y : vecdir = pev->movedir.z;

	if (((vecdir > 0) && (vecAVel.x <= 0 && vecAVel.y <= 0 && vecAVel.z <= 0)) || ((vecdir < 0) && (vecAVel.x >= 0 && vecAVel.y >= 0 && vecAVel.z >= 0)))
	{
		pev->avelocity = g_vecZero;
		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseRunning), 0, 0, SND_STOP, (int)m_pitch);
		SetThink(&CFuncRotating::Rotate);
		Rotate();
	}
	else
		RampPitchVol(FALSE);
}

void CFuncRotating::Rotate(void)
{
	pev->nextthink = pev->ltime + 10;
}

void CFuncRotating::RotatingUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (FBitSet(pev->spawnflags, SF_BRUSH_ACCDCC))
	{
		if (pev->avelocity != g_vecZero)
		{
			SetThink(&CFuncRotating::SpinDown);
			pev->nextthink = pev->ltime + 0.1;
		}
		else
		{
			SetThink(&CFuncRotating::SpinUp);
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseRunning), 0.01, m_flAttenuation, 0, FANPITCHMIN);
			pev->nextthink = pev->ltime + 0.1;
		}
	}
	else if (!FBitSet(pev->spawnflags, SF_BRUSH_ACCDCC))
	{
		if (pev->avelocity != g_vecZero)
		{
			SetThink(&CFuncRotating::SpinDown);
			pev->nextthink = pev->ltime + 0.1;
		}
		else
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, (char *)STRING(pev->noiseRunning), m_flVolume, m_flAttenuation, 0, FANPITCHMAX);
			pev->avelocity = pev->movedir * pev->speed;
			SetThink(&CFuncRotating::Rotate);
			Rotate();
		}
	}
}

void CFuncRotating::Blocked(CBaseEntity *pOther)
{
	pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);
}

class CPendulum : public CBaseEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	void Touch(CBaseEntity *pOther);
	int ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void Blocked(CBaseEntity *pOther);

public:
	void EXPORT Swing(void);
	void EXPORT PendulumUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT Stop(void);
	void EXPORT RopeTouch(CBaseEntity *pOther);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_accel;
	float m_distance;
	float m_time;
	float m_damp;
	float m_maxSpeed;
	float m_dampSpeed;
	vec3_t m_center;
	vec3_t m_start;
};

LINK_ENTITY_TO_CLASS(func_pendulum, CPendulum);

TYPEDESCRIPTION CPendulum::m_SaveData[] =
{
	DEFINE_FIELD(CPendulum, m_accel, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_distance, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_time, FIELD_TIME),
	DEFINE_FIELD(CPendulum, m_damp, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_maxSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_dampSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CPendulum, m_center, FIELD_VECTOR),
	DEFINE_FIELD(CPendulum, m_start, FIELD_VECTOR),
};

IMPLEMENT_SAVERESTORE(CPendulum, CBaseEntity);

void CPendulum::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "distance"))
	{
		m_distance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damp"))
	{
		m_damp = atof(pkvd->szValue) * 0.001;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CPendulum::Spawn(void)
{
	CBaseToggle::AxisDir(pev);

	if (FBitSet(pev->spawnflags, SF_DOOR_PASSABLE))
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), STRING(pev->model));

	if (m_distance == 0)
		return;

	if (pev->speed == 0)
		pev->speed = 100;

	m_accel = (pev->speed * pev->speed) / (2 * fabs(m_distance));
	m_maxSpeed = pev->speed;
	m_start = pev->angles;
	m_center = pev->angles + (m_distance * 0.5) * pev->movedir;

	if (FBitSet(pev->spawnflags, SF_BRUSH_ROTATE_INSTANT))
	{
		SetThink(&CBaseEntity::SUB_CallUseToggle);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	pev->speed = 0;
	SetUse(&CPendulum::PendulumUse);

	if (FBitSet(pev->spawnflags, SF_PENDULUM_SWING))
		SetTouch(&CPendulum::RopeTouch);
}

void CPendulum::PendulumUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pev->speed)
	{
		if (FBitSet(pev->spawnflags, SF_PENDULUM_AUTO_RETURN))
		{
			float delta = CBaseToggle::AxisDelta(pev->spawnflags, pev->angles, m_start);
			pev->avelocity = m_maxSpeed * pev->movedir;
			pev->nextthink = pev->ltime + (delta / m_maxSpeed);
			SetThink(&CPendulum::Stop);
		}
		else
		{
			pev->speed = 0;
			SetThink(NULL);
			pev->avelocity = g_vecZero;
		}
	}
	else
	{
		pev->nextthink = pev->ltime + 0.1;
		m_time = gpGlobals->time;
		SetThink(&CPendulum::Swing);
		m_dampSpeed = m_maxSpeed;
	}
}

void CPendulum::Stop(void)
{
	pev->angles = m_start;
	pev->speed = 0;
	SetThink(NULL);
	pev->avelocity = g_vecZero;
}

void CPendulum::Blocked(CBaseEntity *pOther)
{
	m_time = gpGlobals->time;
}

void CPendulum::Swing(void)
{
	float delta = CBaseToggle::AxisDelta(pev->spawnflags, pev->angles, m_center);
	float dt = gpGlobals->time - m_time;
	m_time = gpGlobals->time;

	if (delta > 0 && m_accel > 0)
		pev->speed -= m_accel * dt;
	else
		pev->speed += m_accel * dt;

	if (pev->speed > m_maxSpeed)
		pev->speed = m_maxSpeed;
	else if (pev->speed < -m_maxSpeed)
		pev->speed = -m_maxSpeed;

	pev->avelocity = pev->speed * pev->movedir;
	pev->nextthink = pev->ltime + 0.1;

	if (m_damp)
	{
		m_dampSpeed -= m_damp * m_dampSpeed * dt;

		if (m_dampSpeed < 30)
		{
			pev->angles = m_center;
			pev->speed = 0;
			SetThink(NULL);
			pev->avelocity = g_vecZero;
		}
		else if (pev->speed > m_dampSpeed)
			pev->speed = m_dampSpeed;
		else if (pev->speed < -m_dampSpeed)
			pev->speed = -m_dampSpeed;
	}
}

void CPendulum::Touch(CBaseEntity *pOther)
{
	entvars_t *pevOther = pOther->pev;

	if (pev->dmg <= 0)
		return;

	if (!pevOther->takedamage)
		return;

	float damage = pev->dmg * pev->speed * 0.01;

	if (damage < 0)
		damage = -damage;

	pOther->TakeDamage(pev, pev, damage, DMG_CRUSH);
	pevOther->velocity = (pevOther->origin - VecBModelOrigin(pev)).Normalize() * damage;
}

void CPendulum::RopeTouch(CBaseEntity *pOther)
{
	entvars_t *pevOther = pOther->pev;

	if (!pOther->IsPlayer())
	{
		ALERT(at_console, "Not a client\n");
		return;
	}

	if (ENT(pevOther) == pev->enemy)
		return;

	pev->enemy = pOther->edict();
	pevOther->velocity = g_vecZero;
	pevOther->movetype = MOVETYPE_NONE;
}
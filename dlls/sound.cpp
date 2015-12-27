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
#include "talkmonster.h"
#include "gamerules.h"
#include <ctype.h>
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

static char *memfgets(byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize);

typedef struct dynpitchvol
{
	int preset;
	int pitchrun;
	int pitchstart;
	int spinup;
	int spindown;
	int volrun;
	int volstart;
	int fadein;
	int fadeout;
	int lfotype;
	int lforate;
	int lfomodpitch;
	int lfomodvol;
	int cspinup;
	int cspincount;
	int pitch;
	int spinupsav;
	int spindownsav;
	int pitchfrac;
	int vol;
	int fadeinsav;
	int fadeoutsav;
	int volfrac;
	int lfofrac;
	int lfomult;
}
dynpitchvol_t;

#define CDPVPRESETMAX 27

dynpitchvol_t rgdpvpreset[CDPVPRESETMAX] =
{
	{ 1, 255, 75, 95, 95, 10, 1, 50, 95, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 2, 255, 85, 70, 88, 10, 1, 20, 88, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 3, 255, 100, 50, 75, 10, 1, 10, 75, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 4, 100, 100, 0, 0, 10, 1, 90, 90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 5, 100, 100, 0, 0, 10, 1, 80, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 6, 100, 100, 0, 0, 10, 1, 50, 70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 7, 100, 100, 0, 0, 5, 1, 40, 50, 1, 50, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 8, 100, 100, 0, 0, 5, 1, 40, 50, 1, 150, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 9, 100, 100, 0, 0, 5, 1, 40, 50, 1, 750, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 10, 128, 100, 50, 75, 10, 1, 30, 40, 2, 8, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11, 128, 100, 50, 75, 10, 1, 30, 40, 2, 25, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 12, 128, 100, 50, 75, 10, 1, 30, 40, 2, 70, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13, 50, 50, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 14, 70, 70, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 15, 90, 90, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 16, 120, 120, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 17, 180, 180, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 18, 255, 255, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 19, 200, 75, 90, 90, 10, 1, 50, 90, 2, 100, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 20, 255, 75, 97, 90, 10, 1, 50, 90, 1, 40, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 21, 100, 100, 0, 0, 10, 1, 30, 50, 3, 15, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 22, 160, 160, 0, 0, 10, 1, 50, 50, 3, 500, 25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 23, 255, 75, 88, 0, 10, 1, 40, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 24, 200, 20, 95, 70, 10, 1, 70, 70, 3, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 25, 180, 100, 50, 60, 10, 1, 40, 60, 2, 90, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 26, 60, 60, 0, 0, 10, 1, 40, 70, 3, 80, 20, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 27, 128, 90, 10, 10, 10, 1, 20, 40, 1, 5, 10, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

class CAmbientGeneric : public CBaseEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Restart(void);
	void Precache(void);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

public:
	void EXPORT ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT RampThink(void);
	void InitModulationParms(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_flAttenuation;
	dynpitchvol_t m_dpv;
	BOOL m_fActive;
	BOOL m_fLooping;
};

LINK_ENTITY_TO_CLASS(ambient_generic, CAmbientGeneric);

TYPEDESCRIPTION CAmbientGeneric::m_SaveData[] =
{
	DEFINE_FIELD(CAmbientGeneric, m_flAttenuation, FIELD_FLOAT),
	DEFINE_FIELD(CAmbientGeneric, m_fActive, FIELD_BOOLEAN),
	DEFINE_FIELD(CAmbientGeneric, m_fLooping, FIELD_BOOLEAN),
	DEFINE_ARRAY(CAmbientGeneric, m_dpv, FIELD_CHARACTER, sizeof(dynpitchvol_t)),
};

IMPLEMENT_SAVERESTORE(CAmbientGeneric, CBaseEntity);

void CAmbientGeneric::Spawn(void)
{
	if (FBitSet(pev->spawnflags, AMBIENT_SOUND_EVERYWHERE))
		m_flAttenuation = ATTN_NONE;
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_SMALLRADIUS))
		m_flAttenuation = ATTN_IDLE;
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_MEDIUMRADIUS))
		m_flAttenuation = ATTN_STATIC;
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_LARGERADIUS))
		m_flAttenuation = ATTN_NORM;
	else
		m_flAttenuation = ATTN_STATIC;

	char *szSoundFile = (char *)STRING(pev->message);

	if (FStringNull(pev->message) || strlen(szSoundFile) < 1)
	{
		ALERT(at_error, "EMPTY AMBIENT AT: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CBaseEntity::SUB_Remove);
		return;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetThink(&CAmbientGeneric::RampThink);
	pev->nextthink = 0;

	SetUse(&CAmbientGeneric::ToggleUse);
	m_fActive = FALSE;

	if (FBitSet(pev->spawnflags, AMBIENT_SOUND_NOT_LOOPING))
		m_fLooping = FALSE;
	else
		m_fLooping = TRUE;

	Precache();
}

void CAmbientGeneric::Restart(void)
{
	if (FBitSet(pev->spawnflags, AMBIENT_SOUND_EVERYWHERE))
		m_flAttenuation = ATTN_NONE;
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_SMALLRADIUS))
		m_flAttenuation = ATTN_IDLE;
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_MEDIUMRADIUS))
		m_flAttenuation = ATTN_STATIC;
	else if (FBitSet(pev->spawnflags, AMBIENT_SOUND_LARGERADIUS))
		m_flAttenuation = ATTN_NORM;
	else
		m_flAttenuation = ATTN_STATIC;

	char *szSoundFile = (char *)STRING(pev->message);

	if (FStringNull(pev->message) || strlen(szSoundFile) < 1)
	{
		ALERT(at_error, "EMPTY AMBIENT AT: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CBaseEntity::SUB_Remove);
		return;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetThink(&CAmbientGeneric::RampThink);
	pev->nextthink = 0;

	SetUse(&CAmbientGeneric::ToggleUse);
	m_fActive = FALSE;

	UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);
	InitModulationParms();
	pev->nextthink = gpGlobals->time + 0.1;

	if (!FBitSet(pev->spawnflags, AMBIENT_SOUND_NOT_LOOPING))
	{
		m_fLooping = TRUE;
		m_fActive = TRUE;
	}
	else
		m_fLooping = FALSE;

	if (m_fActive)
		UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, (m_dpv.vol * 0.01), m_flAttenuation, 0, m_dpv.pitch);
}

void CAmbientGeneric::Precache(void)
{
	char *szSoundFile = (char *)STRING(pev->message);

	if (!FStringNull(pev->message) && strlen(szSoundFile) > 1)
	{
		if (*szSoundFile != '!')
			PRECACHE_SOUND(szSoundFile);
	}

	InitModulationParms();

	if (!FBitSet(pev->spawnflags, AMBIENT_SOUND_START_SILENT))
	{
		if (m_fLooping)
			m_fActive = TRUE;
	}

	if (m_fActive)
	{
		UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, (m_dpv.vol * 0.01), m_flAttenuation, SND_SPAWNING, m_dpv.pitch);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CAmbientGeneric::RampThink(void)
{
	char *szSoundFile = (char *)STRING(pev->message);
	int pitch = m_dpv.pitch;
	int vol = m_dpv.vol;
	int flags = 0;
	int fChanged = 0;
	int prev;

	if (!m_dpv.spinup && !m_dpv.spindown && !m_dpv.fadein && !m_dpv.fadeout && !m_dpv.lfotype)
		return;

	if (m_dpv.spinup || m_dpv.spindown)
	{
		prev = m_dpv.pitchfrac >> 8;

		if (m_dpv.spinup > 0)
			m_dpv.pitchfrac += m_dpv.spinup;
		else if (m_dpv.spindown > 0)
			m_dpv.pitchfrac -= m_dpv.spindown;

		pitch = m_dpv.pitchfrac >> 8;

		if (pitch > m_dpv.pitchrun)
		{
			pitch = m_dpv.pitchrun;
			m_dpv.spinup = 0;
		}

		if (pitch < m_dpv.pitchstart)
		{
			pitch = m_dpv.pitchstart;
			m_dpv.spindown = 0;

			UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);
			return;
		}

		if (pitch > 255)
			pitch = 255;

		if (pitch < 1)
			pitch = 1;

		m_dpv.pitch = pitch;
		fChanged |= (prev != pitch);
		flags |= SND_CHANGE_PITCH;
	}

	if (m_dpv.fadein || m_dpv.fadeout)
	{
		prev = m_dpv.volfrac >> 8;

		if (m_dpv.fadein > 0)
			m_dpv.volfrac += m_dpv.fadein;
		else if (m_dpv.fadeout > 0)
			m_dpv.volfrac -= m_dpv.fadeout;

		vol = m_dpv.volfrac >> 8;

		if (vol > m_dpv.volrun)
		{
			vol = m_dpv.volrun;
			m_dpv.fadein = 0;
		}

		if (vol < m_dpv.volstart)
		{
			vol = m_dpv.volstart;
			m_dpv.fadeout = 0;

			UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);
			return;
		}

		if (vol > 100)
			vol = 100;

		if (vol < 1)
			vol = 1;

		m_dpv.vol = vol;
		fChanged |= (prev != vol);
		flags |= SND_CHANGE_VOL;
	}

	if (m_dpv.lfotype)
	{
		int pos;

		if (m_dpv.lfofrac > 0x6FFFFFFF)
			m_dpv.lfofrac = 0;

		m_dpv.lfofrac += m_dpv.lforate;
		pos = m_dpv.lfofrac >> 8;

		if (m_dpv.lfofrac < 0)
		{
			m_dpv.lfofrac = 0;
			m_dpv.lforate = abs(m_dpv.lforate);
			pos = 0;
		}
		else if (pos > 255)
		{
			pos = 255;
			m_dpv.lfofrac = (255 << 8);
			m_dpv.lforate = -abs(m_dpv.lforate);
		}

		switch (m_dpv.lfotype)
		{
			case LFO_SQUARE:
			{
				if (pos < 128)
					m_dpv.lfomult = 255;
				else
					m_dpv.lfomult = 0;

				break;
			}

			case LFO_RANDOM:
			{
				if (pos == 255)
					m_dpv.lfomult = RANDOM_LONG(0, 255);

				break;
			}

			case LFO_TRIANGLE:
			default: m_dpv.lfomult = pos; break;
		}

		if (m_dpv.lfomodpitch)
		{
			prev = pitch;
			pitch += ((m_dpv.lfomult - 128) * m_dpv.lfomodpitch) / 100;

			if (pitch > 255)
				pitch = 255;

			if (pitch < 1)
				pitch = 1;

			fChanged |= (prev != pitch);
			flags |= SND_CHANGE_PITCH;
		}

		if (m_dpv.lfomodvol)
		{
			prev = vol;
			vol += ((m_dpv.lfomult - 128) * m_dpv.lfomodvol) / 100;

			if (vol > 100)
				vol = 100;

			if (vol < 0)
				vol = 0;

			fChanged |= (prev != vol);
			flags |= SND_CHANGE_VOL;
		}
	}

	if (flags && fChanged)
	{
		if (pitch == PITCH_NORM)
			pitch = PITCH_NORM + 1;

		UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, (vol * 0.01), m_flAttenuation, flags, pitch);
	}

	pev->nextthink = gpGlobals->time + 0.2;
	return;
}

void CAmbientGeneric::InitModulationParms(void)
{
	m_dpv.volrun = (int)(pev->health * 10);

	if (m_dpv.volrun > 100)
		m_dpv.volrun = 100;

	if (m_dpv.volrun < 0)
		m_dpv.volrun = 0;

	if (m_dpv.preset != 0 && m_dpv.preset <= CDPVPRESETMAX)
	{
		m_dpv = rgdpvpreset[m_dpv.preset - 1];

		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;

		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;

		m_dpv.volstart *= 10;
		m_dpv.volrun *= 10;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;

		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;

		m_dpv.lforate *= 256;
		m_dpv.fadeinsav = m_dpv.fadein;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		m_dpv.spinupsav = m_dpv.spinup;
		m_dpv.spindownsav = m_dpv.spindown;
	}

	m_dpv.fadein = m_dpv.fadeinsav;
	m_dpv.fadeout = 0;

	if (m_dpv.fadein)
		m_dpv.vol = m_dpv.volstart;
	else
		m_dpv.vol = m_dpv.volrun;

	m_dpv.spinup = m_dpv.spinupsav;
	m_dpv.spindown = 0;

	if (m_dpv.spinup)
		m_dpv.pitch = m_dpv.pitchstart;
	else
		m_dpv.pitch = m_dpv.pitchrun;

	if (m_dpv.pitch == 0)
		m_dpv.pitch = PITCH_NORM;

	m_dpv.pitchfrac = m_dpv.pitch << 8;
	m_dpv.volfrac = m_dpv.vol << 8;
	m_dpv.lfofrac = 0;
	m_dpv.lforate = abs(m_dpv.lforate);
	m_dpv.cspincount = 1;

	if (m_dpv.cspinup)
	{
		int pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;
		m_dpv.pitchrun = m_dpv.pitchstart + pitchinc;

		if (m_dpv.pitchrun > 255)
			m_dpv.pitchrun = 255;
	}

	if ((m_dpv.spinupsav || m_dpv.spindownsav || (m_dpv.lfotype && m_dpv.lfomodpitch)) && (m_dpv.pitch == PITCH_NORM))
		m_dpv.pitch = PITCH_NORM + 1;
}

void CAmbientGeneric::ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	char *szSoundFile = (char *)STRING(pev->message);

	if (useType != USE_TOGGLE)
	{
		if ((m_fActive && useType == USE_ON) || (!m_fActive && useType == USE_OFF))
			return;
	}

	if (useType == USE_SET && m_fActive)
	{
		float fraction = value;

		if (fraction > 1)
			fraction = 1;

		if (fraction < 0)
			fraction = 0.01;

		m_dpv.pitch = (int)(fraction * 255);
		UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, 0, 0, SND_CHANGE_PITCH, m_dpv.pitch);
		return;
	}

	if (m_fActive)
	{
		if (m_dpv.cspinup)
		{
			if (m_dpv.cspincount <= m_dpv.cspinup)
			{
				m_dpv.cspincount++;
				int pitchinc = (255 - m_dpv.pitchstart) / m_dpv.cspinup;
				m_dpv.spinup = m_dpv.spinupsav;
				m_dpv.spindown = 0;
				m_dpv.pitchrun = m_dpv.pitchstart + pitchinc * m_dpv.cspincount;

				if (m_dpv.pitchrun > 255)
					m_dpv.pitchrun = 255;

				pev->nextthink = gpGlobals->time + 0.1;
			}
		}
		else
		{
			m_fActive = FALSE;
			pev->spawnflags |= AMBIENT_SOUND_START_SILENT;

			if (m_dpv.spindownsav || m_dpv.fadeoutsav)
			{
				m_dpv.spindown = m_dpv.spindownsav;
				m_dpv.spinup = 0;
				m_dpv.fadeout = m_dpv.fadeoutsav;
				m_dpv.fadein = 0;
				pev->nextthink = gpGlobals->time + 0.1;
			}
			else
				UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);
		}
	}
	else
	{
		if (m_fLooping)
			m_fActive = TRUE;
		else
			UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, 0, 0, SND_STOP, 0);

		InitModulationParms();
		UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, (m_dpv.vol * 0.01), m_flAttenuation, 0, m_dpv.pitch);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CAmbientGeneric::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "preset"))
	{
		m_dpv.preset = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		m_dpv.pitchrun = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

		if (m_dpv.pitchrun > 255)
			m_dpv.pitchrun = 255;

		if (m_dpv.pitchrun < 0)
			m_dpv.pitchrun = 0;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchstart"))
	{
		m_dpv.pitchstart = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

		if (m_dpv.pitchstart > 255)
			m_dpv.pitchstart = 255;

		if (m_dpv.pitchstart < 0)
			m_dpv.pitchstart = 0;
	}
	else if (FStrEq(pkvd->szKeyName, "spinup"))
	{
		m_dpv.spinup = atoi(pkvd->szValue);

		if (m_dpv.spinup > 100)
			m_dpv.spinup = 100;

		if (m_dpv.spinup < 0)
			m_dpv.spinup = 0;

		if (m_dpv.spinup > 0)
			m_dpv.spinup = (101 - m_dpv.spinup) * 64;

		m_dpv.spinupsav = m_dpv.spinup;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spindown"))
	{
		m_dpv.spindown = atoi(pkvd->szValue);

		if (m_dpv.spindown > 100)
			m_dpv.spindown = 100;

		if (m_dpv.spindown < 0)
			m_dpv.spindown = 0;

		if (m_dpv.spindown > 0)
			m_dpv.spindown = (101 - m_dpv.spindown) * 64;

		m_dpv.spindownsav = m_dpv.spindown;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volstart"))
	{
		m_dpv.volstart = atoi(pkvd->szValue);

		if (m_dpv.volstart > 10)
			m_dpv.volstart = 10;

		if (m_dpv.volstart < 0)
			m_dpv.volstart = 0;

		m_dpv.volstart *= 10;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadein"))
	{
		m_dpv.fadein = atoi(pkvd->szValue);

		if (m_dpv.fadein > 100)
			m_dpv.fadein = 100;

		if (m_dpv.fadein < 0)
			m_dpv.fadein = 0;

		if (m_dpv.fadein > 0)
			m_dpv.fadein = (101 - m_dpv.fadein) * 64;

		m_dpv.fadeinsav = m_dpv.fadein;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fadeout"))
	{
		m_dpv.fadeout = atoi(pkvd->szValue);

		if (m_dpv.fadeout > 100)
			m_dpv.fadeout = 100;

		if (m_dpv.fadeout < 0)
			m_dpv.fadeout = 0;

		if (m_dpv.fadeout > 0)
			m_dpv.fadeout = (101 - m_dpv.fadeout) * 64;

		m_dpv.fadeoutsav = m_dpv.fadeout;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lfotype"))
	{
		m_dpv.lfotype = atoi(pkvd->szValue);

		if (m_dpv.lfotype > 4)
			m_dpv.lfotype = LFO_TRIANGLE;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lforate"))
	{
		m_dpv.lforate = atoi(pkvd->szValue);

		if (m_dpv.lforate > 1000)
			m_dpv.lforate = 1000;

		if (m_dpv.lforate < 0)
			m_dpv.lforate = 0;

		m_dpv.lforate *= 256;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lfomodpitch"))
	{
		m_dpv.lfomodpitch = atoi(pkvd->szValue);

		if (m_dpv.lfomodpitch > 100)
			m_dpv.lfomodpitch = 100;

		if (m_dpv.lfomodpitch < 0)
			m_dpv.lfomodpitch = 0;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "lfomodvol"))
	{
		m_dpv.lfomodvol = atoi(pkvd->szValue);

		if (m_dpv.lfomodvol > 100)
			m_dpv.lfomodvol = 100;

		if (m_dpv.lfomodvol < 0)
			m_dpv.lfomodvol = 0;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "cspinup"))
	{
		m_dpv.cspinup = atoi(pkvd->szValue);

		if (m_dpv.cspinup > 100)
			m_dpv.cspinup = 100;

		if (m_dpv.cspinup < 0)
			m_dpv.cspinup = 0;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

class CEnvSound : public CPointEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Think(void);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_flRadius;
	float m_flRoomtype;
};

LINK_ENTITY_TO_CLASS(env_sound, CEnvSound);

TYPEDESCRIPTION CEnvSound::m_SaveData[] =
{
	DEFINE_FIELD(CEnvSound, m_flRadius, FIELD_FLOAT),
	DEFINE_FIELD(CEnvSound, m_flRoomtype, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CEnvSound, CBaseEntity);

void CEnvSound::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_flRadius = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}

	if (FStrEq(pkvd->szKeyName, "roomtype"))
	{
		m_flRoomtype = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
}

BOOL FEnvSoundInRange(entvars_t *pev, entvars_t *pevTarget, float *pflRange)
{
	CEnvSound *pSound = GetClassPtr((CEnvSound *)pev);
	Vector vecSpot1 = pev->origin + pev->view_ofs;
	Vector vecSpot2 = pevTarget->origin + pevTarget->view_ofs;

	TraceResult tr;
	UTIL_TraceLine(vecSpot1, vecSpot2, ignore_monsters, ENT(pev), &tr);

	if ((tr.fInOpen && tr.fInWater) || tr.flFraction != 1)
		return FALSE;

	Vector vecRange = tr.vecEndPos - vecSpot1;
	float flRange = vecRange.Length();

	if (pSound->m_flRadius < flRange)
		return FALSE;

	if (pflRange)
		*pflRange = flRange;

	return TRUE;
}

void CEnvSound::Think(void)
{
	edict_t *pentPlayer = FIND_CLIENT_IN_PVS(edict());
	CBasePlayer *pPlayer = NULL;

	if (FNullEnt(pentPlayer))
		goto env_sound_Think_slow;

	float flRange;
	pPlayer = GetClassPtr((CBasePlayer *)VARS(pentPlayer));

	if (!FNullEnt(pPlayer->m_pentSndLast) && (pPlayer->m_pentSndLast == ENT(pev)))
	{
		if (pPlayer->m_flSndRoomtype != 0 && pPlayer->m_flSndRange != 0)
		{
			if (FEnvSoundInRange(pev, VARS(pentPlayer), &flRange))
			{
				pPlayer->m_flSndRange = flRange;
				goto env_sound_Think_fast;
			}
			else
			{
				pPlayer->m_flSndRange = 0;
				pPlayer->m_flSndRoomtype = 0;
				goto env_sound_Think_slow;
			}
		}
		else
			goto env_sound_Think_slow;
	}

	if (FEnvSoundInRange(pev, VARS(pentPlayer), &flRange))
	{
		if (flRange < pPlayer->m_flSndRange || pPlayer->m_flSndRange == 0)
		{
			pPlayer->m_pentSndLast = ENT(pev);
			pPlayer->m_flSndRoomtype = m_flRoomtype;
			pPlayer->m_flSndRange = flRange;

			MESSAGE_BEGIN(MSG_ONE, SVC_ROOMTYPE, NULL, pentPlayer);
			WRITE_SHORT((short)m_flRoomtype);
			MESSAGE_END();
		}
	}

env_sound_Think_fast:
	pev->nextthink = gpGlobals->time + 0.25;
	return;

env_sound_Think_slow:
	pev->nextthink = gpGlobals->time + 0.75;
	return;
}

void CEnvSound::Spawn(void)
{
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5);
}

#define CSENTENCE_LRU_MAX 32

typedef struct sentenceg
{
	char szgroupname[CBSENTENCENAME_MAX];
	int count;
	unsigned char rgblru[CSENTENCE_LRU_MAX];
}
SENTENCEG;

#define CSENTENCEG_MAX 200

SENTENCEG rgsentenceg[CSENTENCEG_MAX];
int fSentencesInit = FALSE;

char gszallsentencenames[CVOXFILESENTENCEMAX][CBSENTENCENAME_MAX];
int gcallsentences = 0;

void USENTENCEG_InitLRU(unsigned char *plru, int count)
{
	int i;
	unsigned char temp;

	if (!fSentencesInit)
		return;

	if (count > CSENTENCE_LRU_MAX)
		count = CSENTENCE_LRU_MAX;

	for (i = 0; i < count; i++)
		plru[i] = (unsigned char)i;

	for (i = 0; i < (count * 4); i++)
	{
		int j = RANDOM_LONG(0, count - 1);
		int k = RANDOM_LONG(0, count - 1);

		temp = plru[j];
		plru[j] = plru[k];
		plru[k] = temp;
	}
}

int USENTENCEG_PickSequential(int isentenceg, char *szfound, int ipick, int freset)
{
	if (!fSentencesInit)
		return -1;

	if (isentenceg < 0)
		return -1;

	char *szgroupname = rgsentenceg[isentenceg].szgroupname;
	unsigned char count = rgsentenceg[isentenceg].count;

	if (count == 0)
		return -1;

	if (ipick >= count)
		ipick = count-1;

	char sznum[8];
	strcpy(szfound, "!");
	strcat(szfound, szgroupname);
	sprintf(sznum, "%d", ipick);
	strcat(szfound, sznum);

	if (ipick >= count)
	{
		if (freset)
			return 0;
		else
			return count;
	}

	return ipick + 1;
}

int USENTENCEG_Pick(int isentenceg, char *szfound)
{
	unsigned char ipick;
	int ffound = FALSE;

	if (!fSentencesInit)
		return -1;

	if (isentenceg < 0)
		return -1;

	char *szgroupname = rgsentenceg[isentenceg].szgroupname;
	unsigned char count = rgsentenceg[isentenceg].count;
	unsigned char *plru = rgsentenceg[isentenceg].rgblru;

	while (!ffound)
	{
		for (unsigned char i = 0; i < count; i++)
		{
			if (plru[i] != 0xFF)
			{
				ipick = plru[i];
				plru[i] = 0xFF;
				ffound = TRUE;
				break;
			}
		}

		if (!ffound)
			USENTENCEG_InitLRU(plru, count);
		else
		{
			char sznum[8];
			strcpy(szfound, "!");
			strcat(szfound, szgroupname);
			sprintf(sznum, "%d", ipick);
			strcat(szfound, sznum);
			return ipick;
		}
	}

	return -1;
}

int SENTENCEG_GetIndex(const char *szgroupname)
{
	if (!fSentencesInit || !szgroupname)
		return -1;

	int i = 0;

	while (rgsentenceg[i].count)
	{
		if (!strcmp(szgroupname, rgsentenceg[i].szgroupname))
			return i;

		i++;
	}

	return -1;
}

int SENTENCEG_PlayRndI(edict_t *entity, int isentenceg, float volume, float attenuation, int flags, int pitch)
{
	if (!fSentencesInit)
		return -1;

	char name[64];
	name[0] = '\0';

	int ipick = USENTENCEG_Pick(isentenceg, name);

	if (ipick > 0 && *name)//ѕохоже, тут хотели проверить не указатель на name, а записалось ли туда что-нибудь. ѕоэтому *name будет правильнее, чем просто name.
		EMIT_SOUND_DYN(entity, CHAN_VOICE, name, volume, attenuation, flags, pitch);

	return ipick;
}

int SENTENCEG_PlayRndSz(edict_t *entity, const char *szgroupname, float volume, float attenuation, int flags, int pitch)
{
	if (!fSentencesInit)
		return -1;

	char name[64];
	name[0] = '\0';
	int isentenceg = SENTENCEG_GetIndex(szgroupname);

	if (isentenceg < 0)
	{
		ALERT(at_console, "No such sentence group %s\n", szgroupname);
		return -1;
	}

	int ipick = USENTENCEG_Pick(isentenceg, name);

	if (ipick >= 0 && name[0])
		EMIT_SOUND_DYN(entity, CHAN_VOICE, name, volume, attenuation, flags, pitch);

	return ipick;
}

int SENTENCEG_PlaySequentialSz(edict_t *entity, const char *szgroupname, float volume, float attenuation, int flags, int pitch, int ipick, int freset)
{
	if (!fSentencesInit)
		return -1;

	char name[64];
	name[0] = '\0';
	int isentenceg = SENTENCEG_GetIndex(szgroupname);

	if (isentenceg < 0)
		return -1;

	int ipicknext = USENTENCEG_PickSequential(isentenceg, name, ipick, freset);

	if (ipicknext >= 0 && name[0])
		EMIT_SOUND_DYN(entity, CHAN_VOICE, name, volume, attenuation, flags, pitch);

	return ipicknext;
}

void SENTENCEG_Stop(edict_t *entity, int isentenceg, int ipick)
{
	if (!fSentencesInit)
		return;

	if (isentenceg < 0 || ipick < 0)
		return;

	char buffer[64], sznum[8];
	strcpy(buffer, "!");
	strcat(buffer, rgsentenceg[isentenceg].szgroupname);
	sprintf(sznum, "%d", ipick);
	strcat(buffer, sznum);

	STOP_SOUND(entity, CHAN_VOICE, buffer);
}

void SENTENCEG_Init(void)
{
	int i, j;

	if (fSentencesInit)
		return;

	memset(gszallsentencenames, 0, CVOXFILESENTENCEMAX * CBSENTENCENAME_MAX);
	gcallsentences = 0;
	memset(rgsentenceg, 0, CSENTENCEG_MAX * sizeof(SENTENCEG));

	char buffer[512], szgroup[64];
	memset(buffer, 0, 512);
	memset(szgroup, 0, 64);

	int isentencegs = -1;
	int filePos = 0, fileSize;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe("sound/sentences.txt", &fileSize);

	if (!pMemFile)
		return;

	while (memfgets(pMemFile, fileSize, filePos, buffer, 511) != NULL)
	{
		i = 0;

		while (buffer[i] && buffer[i] == ' ')
			i++;

		if (!buffer[i])
			continue;

		if (buffer[i] == '/' || !isalpha(buffer[i]))
			continue;

		j = i;

		while (buffer[j] && buffer[j] != ' ')
			j++;

		if (!buffer[j])
			continue;

		if (gcallsentences > CVOXFILESENTENCEMAX)
		{
			ALERT(at_error, "Too many sentences in sentences.txt!\n");
			break;
		}

		buffer[j] = '\0';
		const char *pString = buffer + i;

		if (strlen(pString) >= CBSENTENCENAME_MAX)
			ALERT(at_warning, "Sentence %s longer than %d letters\n", pString, CBSENTENCENAME_MAX - 1);

		strcpy(gszallsentencenames[gcallsentences++], pString);
		j--;

		if (j <= i)
			continue;

		if (!isdigit(buffer[j]))
			continue;

		while (j > i && isdigit(buffer[j]))
			j--;

		if (j <= i)
			continue;

		buffer[j + 1] = '\0';

		if (strcmp(szgroup, &(buffer[i])))
		{
			isentencegs++;

			if (isentencegs >= CSENTENCEG_MAX)
			{
				ALERT(at_error, "Too many sentence groups in sentences.txt!\n");
				break;
			}

			strcpy(rgsentenceg[isentencegs].szgroupname, &(buffer[i]));
			rgsentenceg[isentencegs].count = 1;
			strcpy(szgroup, &(buffer[i]));
			continue;
		}
		else
		{
			if (isentencegs >= 0)
				rgsentenceg[isentencegs].count++;
		}
	}

	g_engfuncs.pfnFreeFile(pMemFile);
	fSentencesInit = TRUE;
	i = 0;

	while (rgsentenceg[i].count && i < CSENTENCEG_MAX)
	{
		USENTENCEG_InitLRU(&(rgsentenceg[i].rgblru[0]), rgsentenceg[i].count);
		i++;
	}
}

int SENTENCEG_Lookup(const char *sample, char *sentencenum)
{
	for (int i = 0; i < gcallsentences; i++)
	{
		if (!stricmp(gszallsentencenames[i], sample + 1))
		{
			if (sentencenum)
			{
				char sznum[8];
				strcpy(sentencenum, "!");
				sprintf(sznum, "%d", i);
				strcat(sentencenum, sznum);
			}

			return i;
		}
	}

	return -1;
}

void EMIT_SOUND_DYN(edict_t *entity, int channel, const char *sample, float volume, float attenuation, int flags, int pitch)
{
	if (sample && *sample == '!')
	{
		char name[32];

		if (SENTENCEG_Lookup(sample, name) >= 0)
			EMIT_SOUND_DYN2(entity, channel, name, volume, attenuation, flags, pitch);
		else
			ALERT(at_aiconsole, "Unable to find %s in sentences.txt\n", sample);
	}
	else
		EMIT_SOUND_DYN2(entity, channel, sample, volume, attenuation, flags, pitch);
}

void EMIT_SOUND_SUIT(edict_t *entity, const char *sample)
{
	int pitch = PITCH_NORM;
	float fvol = CVAR_GET_FLOAT("suitvolume");

	if (RANDOM_LONG(0, 1))
		pitch = RANDOM_LONG(0, 6) + 98;

	if (fvol > 0.05)
		EMIT_SOUND_DYN(entity, CHAN_STATIC, sample, fvol, ATTN_NORM, 0, pitch);
}

void EMIT_GROUPID_SUIT(edict_t *entity, int isentenceg)
{
	int pitch = PITCH_NORM;
	float fvol = CVAR_GET_FLOAT("suitvolume");

	if (RANDOM_LONG(0, 1))
		pitch = RANDOM_LONG(0, 6) + 98;

	if (fvol > 0.05)
		SENTENCEG_PlayRndI(entity, isentenceg, fvol, ATTN_NORM, 0, pitch);
}

void EMIT_GROUPNAME_SUIT(edict_t *entity, const char *groupname)
{
	int pitch = PITCH_NORM;
	float fvol = CVAR_GET_FLOAT("suitvolume");

	if (RANDOM_LONG(0, 1))
		pitch = RANDOM_LONG(0, 6) + 98;

	if (fvol > 0.05)
		SENTENCEG_PlayRndSz(entity, groupname, fvol, ATTN_NORM, 0, pitch);
}

int fTextureTypeInit = FALSE;

#define CTEXTURESMAX 1024

int gcTextures = 0;
char grgszTextureName[CTEXTURESMAX][CBTEXTURENAMEMAX];
char grgchTextureType[CTEXTURESMAX];

static char *memfgets(byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize)
{
	if (!pMemFile || !pBuffer)
		return NULL;

	if (filePos >= fileSize)
		return NULL;

	int i = filePos;
	int last = fileSize;

	if (last - filePos > (bufferSize - 1))
		last = filePos + (bufferSize - 1);

	int stop = 0;

	while (i < last && !stop)
	{
		if (pMemFile[i] == '\n')
			stop = 1;

		i++;
	}

	if (i != filePos)
	{
		int size = i - filePos;
		memcpy(pBuffer, pMemFile + filePos, sizeof(byte) * size);

		if (size < bufferSize)
			pBuffer[size] = '\0';

		filePos = i;
		return pBuffer;
	}

	return NULL;
}

void TEXTURETYPE_Init(void)
{
	int i, j;

	if (fTextureTypeInit)
		return;

	memset(&(grgszTextureName[0][0]), 0, CTEXTURESMAX * CBTEXTURENAMEMAX);
	memset(grgchTextureType, 0, CTEXTURESMAX);

	char buffer[512];
	gcTextures = 0;
	memset(buffer, 0, 512);

	int fileSize = 0, filePos = 0;
	byte *pMemFile = g_engfuncs.pfnLoadFileForMe("sound/materials.txt", &fileSize);

	if (!pMemFile)
		return;

	while (memfgets(pMemFile, fileSize, filePos, buffer, 511) != NULL && (gcTextures < CTEXTURESMAX))
	{
		i = 0;

		while (buffer[i] && isspace(buffer[i]))
			i++;

		if (!buffer[i])
			continue;

		if (buffer[i] == '/' || !isalpha(buffer[i]))
			continue;

		grgchTextureType[gcTextures] = toupper(buffer[i++]);

		while (buffer[i] && isspace(buffer[i]))
			i++;

		if (!buffer[i])
			continue;

		j = i;

		while (buffer[j] && !isspace(buffer[j]))
			j++;

		if (!buffer[j])
			continue;

		j = min (j, CBTEXTURENAMEMAX - 1 + i);
		buffer[j] = '\0';
		strcpy(&(grgszTextureName[gcTextures++][0]), &(buffer[i]));
	}

	g_engfuncs.pfnFreeFile(pMemFile);
	fTextureTypeInit = TRUE;
}

char TEXTURETYPE_Find(char *name)
{
	for (int i = 0; i < gcTextures; i++)
	{
		if (!strnicmp(name, &(grgszTextureName[i][0]), CBTEXTURENAMEMAX - 1))
			return (grgchTextureType[i]);
	}

	return CHAR_TEX_CONCRETE;
}

float TEXTURETYPE_PlaySound(TraceResult *ptr, Vector vecSrc, Vector vecEnd, int iBulletType)
{
	char *rgsz[4];
	float fvol, fvolbar;
	const char *pTextureName;
	int cnt;
	float fattn = ATTN_NORM;

	if (!g_pGameRules->PlayTextureSounds())
		return 0;

	CBaseEntity *pEntity = CBaseEntity::Instance(ptr->pHit);
	char chTextureType = 0;

	if (pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
	{
		chTextureType = CHAR_TEX_FLESH;
	}
	else
	{
		float rgfl1[3], rgfl2[3];
		vecSrc.CopyToArray(rgfl1);
		vecEnd.CopyToArray(rgfl2);

		if (pEntity)
			pTextureName = TRACE_TEXTURE(ENT(pEntity->pev), rgfl1, rgfl2);
		else
			pTextureName = TRACE_TEXTURE(ENT(0), rgfl1, rgfl2);

		if (pTextureName)
		{
			if (*pTextureName == '-' || *pTextureName == '+')
				pTextureName += 2;

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
				pTextureName++;

			char szbuffer[64];
			strcpy(szbuffer, pTextureName);
			szbuffer[CBTEXTURENAMEMAX - 1] = '\0';
			chTextureType = TEXTURETYPE_Find(szbuffer);
		}
	}

	switch (chTextureType)
	{
		default:
		case CHAR_TEX_CONCRETE:
		{
			fvol = 0.9;
			fvolbar = 0.6;
			rgsz[0] = "player/pl_step1.wav";
			rgsz[1] = "player/pl_step2.wav";
			cnt = 2;
			break;
		}

		case CHAR_TEX_METAL:
		{
			fvol = 0.9;
			fvolbar = 0.3;
			rgsz[0] = "player/pl_metal1.wav";
			rgsz[1] = "player/pl_metal2.wav";
			cnt = 2;
			break;
		}

		case CHAR_TEX_DIRT:
		{
			fvol = 0.9;
			fvolbar = 0.1;
			rgsz[0] = "player/pl_dirt1.wav";
			rgsz[1] = "player/pl_dirt2.wav";
			rgsz[2] = "player/pl_dirt3.wav";
			cnt = 3;
			break;
		}

		case CHAR_TEX_VENT:
		{
			fvol = 0.5;
			fvolbar = 0.3;
			rgsz[0] = "player/pl_duct1.wav";
			rgsz[1] = "player/pl_duct1.wav";
			cnt = 2;
			break;
		}

		case CHAR_TEX_GRATE:
		{
			fvol = 0.9;
			fvolbar = 0.5;
			rgsz[0] = "player/pl_grate1.wav";
			rgsz[1] = "player/pl_grate4.wav";
			cnt = 2;
			break;
		}

		case CHAR_TEX_TILE:
		{
			fvol = 0.8;
			fvolbar = 0.2;
			rgsz[0] = "player/pl_tile1.wav";
			rgsz[1] = "player/pl_tile3.wav";
			rgsz[2] = "player/pl_tile2.wav";
			rgsz[3] = "player/pl_tile4.wav";
			cnt = 4;
			break;
		}

		case CHAR_TEX_SLOSH:
		{
			fvol = 0.9;
			fvolbar = 0;
			rgsz[0] = "player/pl_slosh1.wav";
			rgsz[1] = "player/pl_slosh3.wav";
			rgsz[2] = "player/pl_slosh2.wav";
			rgsz[3] = "player/pl_slosh4.wav";
			cnt = 4;
			break;
		}

		case CHAR_TEX_SNOW:
		{
			fvol = 0.7;
			fvolbar = 0.4;
			rgsz[0] = "debris/pl_snow1.wav";
			rgsz[1] = "debris/pl_snow2.wav";
			rgsz[2] = "debris/pl_snow3.wav";
			rgsz[3] = "debris/pl_snow4.wav";
			cnt = 4;
			break;
		}

		case CHAR_TEX_WOOD:
		{
			fvol = 0.9;
			fvolbar = 0.2;
			rgsz[0] = "debris/wood1.wav";
			rgsz[1] = "debris/wood2.wav";
			rgsz[2] = "debris/wood3.wav";
			cnt = 3;
			break;
		}

		case CHAR_TEX_GLASS:
		case CHAR_TEX_COMPUTER:
		{
			fvol = 0.8;
			fvolbar = 0.2;
			rgsz[0] = "debris/glass1.wav";
			rgsz[1] = "debris/glass2.wav";
			rgsz[2] = "debris/glass3.wav";
			cnt = 3;
			break;
		}

		case CHAR_TEX_FLESH:
		{
			if (iBulletType == BULLET_PLAYER_CROWBAR)
				return 0;
			fvol = 1.0;fvolbar = 0.2;
			rgsz[0] = "weapons/bullet_hit1.wav";
			rgsz[1] = "weapons/bullet_hit2.wav";
			fattn = 1.0;
			cnt = 2;
			break;
		}
	}

	if (pEntity && FClassnameIs(pEntity->pev, "func_breakable"))
	{
		fvol /= 1.5;
		fvolbar /= 2;
	}
	else if (chTextureType == CHAR_TEX_COMPUTER)
	{
		if (ptr->flFraction != 1 && RANDOM_LONG(0, 1))
		{
			UTIL_Sparks(ptr->vecEndPos);
			float flVolume = RANDOM_FLOAT(0.7, 1);

			switch (RANDOM_LONG(0, 1))
			{
				case 0: UTIL_EmitAmbientSound(ENT(0), ptr->vecEndPos, "buttons/spark5.wav", flVolume, ATTN_NORM, 0, 100); break;
				case 1: UTIL_EmitAmbientSound(ENT(0), ptr->vecEndPos, "buttons/spark6.wav", flVolume, ATTN_NORM, 0, 100); break;
			}
		}
	}

	UTIL_EmitAmbientSound(ENT(0), ptr->vecEndPos, rgsz[RANDOM_LONG(0, cnt - 1)], fvol, fattn, 0, 96 + RANDOM_LONG(0, 0xF));
	return fvolbar;
}

class CSpeaker : public CBaseEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Spawn(void);
	void Precache(void);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

public:
	void EXPORT ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT SpeakerThink(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_preset;
};

LINK_ENTITY_TO_CLASS(speaker, CSpeaker);

TYPEDESCRIPTION CSpeaker::m_SaveData[] =
{
	DEFINE_FIELD(CSpeaker, m_preset, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CSpeaker, CBaseEntity);

void CSpeaker::Spawn(void)
{
	char *szSoundFile = (char *)STRING(pev->message);

	if (!m_preset && (FStringNull(pev->message) || strlen(szSoundFile) < 1))
	{
		ALERT(at_error, "SPEAKER with no Level/Sentence! at: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CBaseEntity::SUB_Remove);
		return;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	SetThink(&CSpeaker::SpeakerThink);
	pev->nextthink = 0;

	SetUse(&CSpeaker::ToggleUse);
	Precache();
}

#define ANNOUNCE_MINUTES_MIN 0.25
#define ANNOUNCE_MINUTES_MAX 2.25

void CSpeaker::Precache(void)
{
	if (!FBitSet(pev->spawnflags, SPEAKER_START_SILENT))
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(5, 15);
}

void CSpeaker::SpeakerThink(void)
{
	char *szSoundFile = NULL;
	float flvolume = pev->health * 0.1;
	float flattenuation = 0.3;
	int flags = 0;
	int pitch = 100;

	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
	{
		pev->nextthink = CTalkMonster::g_talkWaitTime + RANDOM_FLOAT(5, 10);
		return;
	}

	if (m_preset)
	{
		switch (m_preset)
		{
			case 1: szSoundFile = "C1A0_"; break;
			case 2: szSoundFile = "C1A1_"; break;
			case 3: szSoundFile = "C1A2_"; break;
			case 4: szSoundFile = "C1A3_"; break;
			case 5: szSoundFile = "C1A4_"; break;
			case 6: szSoundFile = "C2A1_"; break;
			case 7: szSoundFile = "C2A2_"; break;
			case 8: szSoundFile = "C2A3_"; break;
			case 9: szSoundFile = "C2A4_"; break;
			case 10: szSoundFile = "C2A5_"; break;
			case 11: szSoundFile = "C3A1_"; break;
			case 12: szSoundFile = "C3A2_"; break;
		}
	}
	else
		szSoundFile = (char *)STRING(pev->message);

	if (szSoundFile[0] == '!')
	{
		UTIL_EmitAmbientSound(ENT(pev), pev->origin, szSoundFile, flvolume, flattenuation, flags, pitch);
		pev->nextthink = 0;
	}
	else
	{
		if (SENTENCEG_PlayRndSz(ENT(pev), szSoundFile, flvolume, flattenuation, flags, pitch) < 0)
			ALERT(at_console, "Level Design Error!\nSPEAKER has bad sentence group name: %s\n", szSoundFile);

		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(ANNOUNCE_MINUTES_MIN * 60, ANNOUNCE_MINUTES_MAX * 60);
		CTalkMonster::g_talkWaitTime = gpGlobals->time + 5;
	}

	return;
}

void CSpeaker::ToggleUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	int fActive = (pev->nextthink > 0);

	if (useType != USE_TOGGLE)
	{
		if ((fActive && useType == USE_ON) || (!fActive && useType == USE_OFF))
			return;
	}

	if (useType == USE_ON)
	{
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	if (useType == USE_OFF)
	{
		pev->nextthink = 0;
		return;
	}

	if (fActive)
		pev->nextthink = 0;
	else
		pev->nextthink = gpGlobals->time + 0.1;
}

void CSpeaker::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "preset"))
	{
		m_preset = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

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
#include "trains.h"
#include "saverestore.h"

class CPathCorner : public CPointEntity
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	float GetDelay(void) { return m_flWait; }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_flWait;
};

LINK_ENTITY_TO_CLASS(path_corner, CPathCorner);

TYPEDESCRIPTION CPathCorner::m_SaveData[] =
{
	DEFINE_FIELD(CPathCorner, m_flWait, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CPathCorner, CPointEntity);

void CPathCorner::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CPathCorner::Spawn(void)
{

}

TYPEDESCRIPTION CPathTrack::m_SaveData[] =
{
	DEFINE_FIELD(CPathTrack, m_length, FIELD_FLOAT),
	DEFINE_FIELD(CPathTrack, m_pnext, FIELD_CLASSPTR),
	DEFINE_FIELD(CPathTrack, m_paltpath, FIELD_CLASSPTR),
	DEFINE_FIELD(CPathTrack, m_pprevious, FIELD_CLASSPTR),
	DEFINE_FIELD(CPathTrack, m_altName, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CPathTrack, CBaseEntity);
LINK_ENTITY_TO_CLASS(path_track, CPathTrack);

void CPathTrack::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "altpath"))
	{
		m_altName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CPathTrack::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_paltpath)
	{
		int on = !FBitSet(pev->spawnflags, SF_PATH_ALTERNATE);

		if (ShouldToggle(useType, on))
		{
			if (on)
				SetBits(pev->spawnflags, SF_PATH_ALTERNATE);
			else
				ClearBits(pev->spawnflags, SF_PATH_ALTERNATE);
		}
	}
	else
	{
		int on = !FBitSet(pev->spawnflags, SF_PATH_DISABLED);

		if (ShouldToggle(useType, on))
		{
			if (on)
				SetBits(pev->spawnflags, SF_PATH_DISABLED);
			else
				ClearBits(pev->spawnflags, SF_PATH_DISABLED);
		}
	}
}

void CPathTrack::Link(void)
{
	if (!FStringNull(pev->target))
	{
		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

		if (!FNullEnt(pentTarget))
		{
			m_pnext = CPathTrack::Instance(pentTarget);

			if (m_pnext)
				m_pnext->SetPrevious(this);
		}
		else
			ALERT(at_console, "Dead end link %s\n", STRING(pev->target));
	}

	if (m_altName)
	{
		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_altName));

		if (!FNullEnt(pentTarget))
		{
			m_paltpath = CPathTrack::Instance(pentTarget);

			if (m_paltpath)
				m_paltpath->SetPrevious(this);
		}
	}
}

void CPathTrack::Spawn(void)
{
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));

	m_pnext = NULL;
	m_pprevious = NULL;
}

void CPathTrack::Activate(void)
{
	if (!FStringNull(pev->targetname))
		Link();
}

CPathTrack *CPathTrack::ValidPath(CPathTrack *ppath, int testFlag)
{
	if (!ppath)
		return NULL;

	if (testFlag && FBitSet(ppath->pev->spawnflags, SF_PATH_DISABLED))
		return NULL;

	return ppath;
}

void CPathTrack::Project(CPathTrack *pstart, CPathTrack *pend, Vector *origin, float dist)
{
	if (pstart && pend)
	{
		Vector dir = (pend->pev->origin - pstart->pev->origin);
		dir = dir.Normalize();
		*origin = pend->pev->origin + dir * dist;
	}
}

CPathTrack *CPathTrack::GetNext(void)
{
	if (m_paltpath && FBitSet(pev->spawnflags, SF_PATH_ALTERNATE) && !FBitSet(pev->spawnflags, SF_PATH_ALTREVERSE))
		return m_paltpath;

	return m_pnext;
}

CPathTrack *CPathTrack::GetPrevious(void)
{
	if (m_paltpath && FBitSet(pev->spawnflags, SF_PATH_ALTERNATE) && FBitSet(pev->spawnflags, SF_PATH_ALTREVERSE))
		return m_paltpath;

	return m_pprevious;
}

void CPathTrack::SetPrevious(CPathTrack *pprev)
{
	if (pprev && !FStrEq(STRING(pprev->pev->targetname), STRING(m_altName)))
		m_pprevious = pprev;
}

CPathTrack *CPathTrack::LookAhead(Vector *origin, float dist, int move)
{
	float originalDist = dist;
	CPathTrack *pcurrent = this;
	Vector currentPos = *origin;

	if (dist < 0)
	{
		dist = -dist;

		while (dist > 0)
		{
			Vector dir = pcurrent->pev->origin - currentPos;
			float length = dir.Length();

			if (!length)
			{
				if (!ValidPath(pcurrent->GetPrevious(), move))
				{
					if (!move)
						Project(pcurrent->GetNext(), pcurrent, origin, dist);

					return NULL;
				}

				pcurrent = pcurrent->GetPrevious();
			}
			else if (length > dist)
			{
				*origin = currentPos + (dir * (dist / length));
				return pcurrent;
			}
			else
			{
				dist -= length;
				currentPos = pcurrent->pev->origin;
				*origin = currentPos;

				if (!ValidPath(pcurrent->GetPrevious(), move))
					return NULL;

				pcurrent = pcurrent->GetPrevious();
			}
		}

		*origin = currentPos;
		return pcurrent;
	}
	else
	{
		while (dist > 0)
		{
			if (!ValidPath(pcurrent->GetNext(), move))
			{
				if (!move)
					Project(pcurrent->GetPrevious(), pcurrent, origin, dist);

				return NULL;
			}

			Vector dir = pcurrent->GetNext()->pev->origin - currentPos;
			float length = dir.Length();

			if (!length && !ValidPath(pcurrent->GetNext()->GetNext(), move))
			{
				if (dist == originalDist)
					return NULL;

				return pcurrent;
			}

			if (length > dist)
			{
				*origin = currentPos + (dir * (dist / length));
				return pcurrent;
			}
			else
			{
				dist -= length;
				currentPos = pcurrent->GetNext()->pev->origin;
				pcurrent = pcurrent->GetNext();
				*origin = currentPos;
			}
		}

		*origin = currentPos;
	}

	return pcurrent;
}

CPathTrack *CPathTrack::Nearest(Vector origin)
{
	Vector delta = origin - pev->origin;
	delta.z = 0;
	float minDist = delta.Length();
	CPathTrack *pnearest = this;
	CPathTrack *ppath = GetNext();
	int deadCount = 0;

	while (ppath && ppath !=this)
	{
		deadCount++;

		if (deadCount > 9999)
		{
			ALERT(at_error, "Bad sequence of path_tracks from %s", STRING(pev->targetname));
			return NULL;
		}

		delta = origin - ppath->pev->origin;
		delta.z = 0;
		float dist = delta.Length();

		if (dist < minDist)
		{
			minDist = dist;
			pnearest = ppath;
		}

		ppath = ppath->GetNext();
	}

	return pnearest;
}

CPathTrack *CPathTrack::Instance(edict_t *pent)
{
	if (FClassnameIs(pent, "path_track"))
		return (CPathTrack *)GET_PRIVATE(pent);

	return NULL;
}
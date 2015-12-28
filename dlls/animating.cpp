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
#include "animation.h"
#include "saverestore.h"

TYPEDESCRIPTION CBaseAnimating::m_SaveData[] =
{
	DEFINE_FIELD(CBaseMonster, m_flFrameRate, FIELD_FLOAT),
	DEFINE_FIELD(CBaseMonster, m_flGroundSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CBaseMonster, m_flLastEventCheck, FIELD_TIME),
	DEFINE_FIELD(CBaseMonster, m_fSequenceFinished, FIELD_BOOLEAN),
	DEFINE_FIELD(CBaseMonster, m_fSequenceLoops, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CBaseAnimating, CBaseDelay);

float CBaseAnimating::StudioFrameAdvance(float flInterval)
{
	if (flInterval == 0)
	{
		flInterval = (gpGlobals->time - pev->animtime);

		if (flInterval <= 0.001)
		{
			pev->animtime = gpGlobals->time;
			return 0;
		}
	}

	if (!pev->animtime)
		flInterval = 0;

	pev->frame += flInterval * m_flFrameRate * pev->framerate;
	pev->animtime = gpGlobals->time;

	if (pev->frame < 0 || pev->frame >= 256)
	{
		if (m_fSequenceLoops)
			pev->frame -= (int)(pev->frame / 256.0) * 256.0;
		else
			pev->frame = (pev->frame < 0) ? 0 : 255;

		m_fSequenceFinished = TRUE;
	}

	return flInterval;
}

int CBaseAnimating::LookupActivity(int activity)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	return ::LookupActivity(pmodel, pev, activity);
}

int CBaseAnimating::LookupActivityHeaviest(int activity)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	return ::LookupActivityHeaviest(pmodel, pev, activity);
}

int CBaseAnimating::LookupSequence(const char *label)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	return ::LookupSequence(pmodel, label);
}

void CBaseAnimating::ResetSequenceInfo(void)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));

	GetSequenceInfo(pmodel, pev, &m_flFrameRate, &m_flGroundSpeed);
	m_fSequenceLoops = ((GetSequenceFlags() & STUDIO_LOOPING) != 0);
	pev->animtime = gpGlobals->time;
	pev->framerate = 1;
	m_fSequenceFinished = FALSE;
	m_flLastEventCheck = gpGlobals->time;
}

BOOL CBaseAnimating::GetSequenceFlags(void)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	return ::GetSequenceFlags(pmodel, pev);
}

void CBaseAnimating::DispatchAnimEvents(float flInterval)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));

	if (!pmodel)
	{
		ALERT(at_aiconsole, "Gibbed monster is thinking!\n");
		return;
	}

	flInterval = 0.1;

	float flStart = pev->frame + (m_flLastEventCheck - pev->animtime) * m_flFrameRate * pev->framerate;
	float flEnd = pev->frame + flInterval * m_flFrameRate * pev->framerate;
	m_flLastEventCheck = pev->animtime + flInterval;
	m_fSequenceFinished = FALSE;

	if (flEnd >= 256 || flEnd <= 0)
		m_fSequenceFinished = TRUE;

	int index = 0;
	MonsterEvent_t event;

	while ((index = GetAnimationEvent(pmodel, pev, &event, flStart, flEnd, index)) != 0)
		HandleAnimEvent(&event);
}

float CBaseAnimating::SetBoneController(int iController, float flValue)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	return SetController(pmodel, pev, iController, flValue);
}

void CBaseAnimating::InitBoneControllers(void)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));

	SetController(pmodel, pev, 0, 0);
	SetController(pmodel, pev, 1, 0);
	SetController(pmodel, pev, 2, 0);
	SetController(pmodel, pev, 3, 0);
}

float CBaseAnimating::SetBlending(int iBlender, float flValue)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));
	return ::SetBlending(pmodel, pev, iBlender, flValue);
}

void CBaseAnimating::GetBonePosition(int iBone, Vector &origin, Vector &angles)
{
	GET_BONE_POSITION(ENT(pev), iBone, origin, angles);
}

void CBaseAnimating::GetAttachment(int iAttachment, Vector &origin, Vector &angles)
{
	GET_ATTACHMENT(ENT(pev), iAttachment, origin, angles);
}

int CBaseAnimating::FindTransition(int iEndingSequence, int iGoalSequence, int *piDir)
{
	void *pmodel = GET_MODEL_PTR(ENT(pev));

	if (!piDir)
	{
		int iDir;
		int sequence = ::FindTransition(pmodel, iEndingSequence, iGoalSequence, &iDir);

		if (iDir != 1)
			return -1;

		return sequence;
	}

	return ::FindTransition(pmodel, iEndingSequence, iGoalSequence, piDir);
}

void CBaseAnimating::GetAutomovement(Vector &origin, Vector &angles, float flInterval)
{
}

void CBaseAnimating::SetBodygroup(int iGroup, int iValue)
{
	::SetBodygroup(GET_MODEL_PTR(ENT(pev)), pev, iGroup, iValue);
}

int CBaseAnimating::GetBodygroup(int iGroup)
{
	return ::GetBodygroup(GET_MODEL_PTR(ENT(pev)), pev, iGroup);
}

int CBaseAnimating::ExtractBbox(int sequence, float *mins, float *maxs)
{
	return ::ExtractBbox(GET_MODEL_PTR(ENT(pev)), sequence, mins, maxs);
}

void CBaseAnimating::SetSequenceBox(void)
{
	Vector mins, maxs;

	if (ExtractBbox(pev->sequence, mins, maxs))
	{
		float yaw = pev->angles.y * (M_PI / 180);

		Vector xvector, yvector;
		xvector.x = cos(yaw);
		xvector.y = sin(yaw);
		yvector.x = -sin(yaw);
		yvector.y = cos(yaw);

		Vector bounds[2] = { mins, maxs };
		Vector rmin(9999, 9999, 9999);
		Vector rmax(-9999, -9999, -9999);
		Vector base, transformed;

		for (int i = 0; i <= 1; i++)
		{
			base.x = bounds[i].x;

			for (int j = 0; j <= 1; j++)
			{
				base.y = bounds[j].y;

				for (int k = 0; k <= 1; k++)
				{
					base.z = bounds[k].z;
					transformed.x = xvector.x * base.x + yvector.x * base.y;
					transformed.y = xvector.y * base.x + yvector.y * base.y;
					transformed.z = base.z;

					if (transformed.x < rmin.x)
						rmin.x = transformed.x;

					if (transformed.x > rmax.x)
						rmax.x = transformed.x;

					if (transformed.y < rmin.y)
						rmin.y = transformed.y;

					if (transformed.y > rmax.y)
						rmax.y = transformed.y;

					if (transformed.z < rmin.z)
						rmin.z = transformed.z;

					if (transformed.z > rmax.z)
						rmax.z = transformed.z;
				}
			}
		}

		rmin.z = 0;
		rmax.z = rmin.z + 1;
		UTIL_SetSize(pev, rmin, rmax);
	}
}
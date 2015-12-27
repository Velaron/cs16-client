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
#include "game.h"

#define NUM_LATERAL_CHECKS 13
#define NUM_LATERAL_LOS_CHECKS 6

DLL_GLOBAL BOOL g_fDrawLines = FALSE;

BOOL FBoxVisible(entvars_t *pevLooker, entvars_t *pevTarget, Vector &vecTargetOrigin, float flSize)
{
	if ((pevLooker->waterlevel != 3 && pevTarget->waterlevel == 3) || (pevLooker->waterlevel == 3 && pevTarget->waterlevel == 0))
		return FALSE;

	Vector vecLookerOrigin = pevLooker->origin + pevLooker->view_ofs;

	for (int i = 0; i < 5; i++)
	{
		Vector vecTarget = pevTarget->origin;
		vecTarget.x += RANDOM_FLOAT(pevTarget->mins.x + flSize, pevTarget->maxs.x - flSize);
		vecTarget.y += RANDOM_FLOAT(pevTarget->mins.y + flSize, pevTarget->maxs.y - flSize);
		vecTarget.z += RANDOM_FLOAT(pevTarget->mins.z + flSize, pevTarget->maxs.z - flSize);

		TraceResult tr;
		UTIL_TraceLine(vecLookerOrigin, vecTarget, ignore_monsters, ignore_glass, ENT(pevLooker), &tr);

		if (tr.flFraction == 1)
		{
			vecTargetOrigin = vecTarget;
			return TRUE;
		}
	}

	return FALSE;
}

Vector VecCheckToss(entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj)
{
	float flGravity = CVAR_GET_FLOAT("sv_gravity") * flGravityAdj;

	if (vecSpot2.z - vecSpot1.z > 500)
		return g_vecZero;

	UTIL_MakeVectors(pev->angles);
	vecSpot2 = vecSpot2 + gpGlobals->v_right * (RANDOM_FLOAT(-8, 8) + RANDOM_FLOAT(-16, 16));
	vecSpot2 = vecSpot2 + gpGlobals->v_forward * (RANDOM_FLOAT(-8, 8) + RANDOM_FLOAT(-16, 16));

	TraceResult tr;
	Vector vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0, 0, 500), ignore_monsters, ENT(pev), &tr);
	vecMidPoint = tr.vecEndPos;
	vecMidPoint.z -= 50;

	if (vecMidPoint.z < vecSpot1.z || vecMidPoint.z < vecSpot2.z)
		return g_vecZero;

	float distance1 = (vecMidPoint.z - vecSpot1.z);
	float distance2 = (vecMidPoint.z - vecSpot2.z);
	float time1 = sqrt(distance1 / (0.5 * flGravity));
	float time2 = sqrt(distance2 / (0.5 * flGravity));

	if (time1 < 0.1)
		return g_vecZero;

	Vector vecGrenadeVel = (vecSpot2 - vecSpot1) / (time1 + time2);
	vecGrenadeVel.z = flGravity * time1;
	Vector vecApex = vecSpot1 + vecGrenadeVel * time1;
	vecApex.z = vecMidPoint.z;

	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, ENT(pev), &tr);

	if (tr.flFraction != 1)
		return g_vecZero;

	UTIL_TraceLine(vecSpot2, vecApex, ignore_monsters, ENT(pev), &tr);

	if (tr.flFraction != 1)
		return g_vecZero;

	return vecGrenadeVel;
}

Vector VecCheckThrow(entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj)
{
	float flGravity = CVAR_GET_FLOAT("sv_gravity") * flGravityAdj;
	Vector vecGrenadeVel = (vecSpot2 - vecSpot1);
	float time = vecGrenadeVel.Length() / flSpeed;
	vecGrenadeVel = vecGrenadeVel * (1 / time);
	vecGrenadeVel.z += flGravity * time * 0.5;

	Vector vecApex = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

	TraceResult tr;
	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, ENT(pev), &tr);

	if (tr.flFraction != 1)
		return g_vecZero;

	UTIL_TraceLine(vecSpot2, vecApex, ignore_monsters, ENT(pev), &tr);

	if (tr.flFraction != 1)
		return g_vecZero;

	return vecGrenadeVel;
}
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

float CLocalNav::s_flStepSize;
int CLocalNav::qptr;
EHANDLE CLocalNav::queue[MAX_HOSTAGES];
int CLocalNav::tot_inqueue;
float CLocalNav::nodeval;
float CLocalNav::flNextCvarCheck;
float CLocalNav::flLastThinkTime;
EHANDLE CLocalNav::hostages[MAX_HOSTAGES];
int CLocalNav::tot_hostages;

CLocalNav::CLocalNav(CHostage *pOwner)
{
	m_pOwner = pOwner;
	m_pTargetEnt = NULL;
	m_nodeArr = new node_index_t[MAX_NODES];

#ifdef _DEBUG
	CONSOLE_ECHO("Allocated m_nodeArr: %d nodes, %lu bytes\n", MAX_NODES, sizeof(node_index_t) * MAX_NODES);
#endif

	if (tot_hostages >= MAX_HOSTAGES)
	{
#ifdef _DEBUG
		CONSOLE_ECHO("queue full (creation)\n");
#endif
		return;
	}

	hostages[tot_hostages++] = pOwner;
}

CLocalNav::~CLocalNav(void)
{
	delete m_nodeArr;
}

void CLocalNav::Reset(void)
{
	flNextCvarCheck = 0;
	flLastThinkTime = 0;
	tot_inqueue = 0;
	qptr = 0;
	nodeval = 0;
	tot_hostages = 0;
}

void CLocalNav::SetTargetEnt(CBaseEntity *pTargetEnt)
{
	if (pTargetEnt)
		m_pTargetEnt = pTargetEnt->edict();
	else
		m_pTargetEnt = NULL;
}

void UTIL_DrawBeamPoints(Vector vecStart, Vector vecEnd, int iLifetime, byte bRed, byte bGreen, byte bBlue);

int CLocalNav::AddNode(int nindexParent, Vector &vecLoc, int offsetX, int offsetY, BOOL bDepth)
{
	node_index_t *nodeNew;

	if (m_nindexAvailableNode == MAX_NODES)
		return -1;

	nodeNew = GetNode(m_nindexAvailableNode);
	nodeNew->vecLoc = vecLoc;
	nodeNew->offsetX = offsetX;
	nodeNew->offsetY = offsetY;
	nodeNew->bDepth = bDepth;
	nodeNew->fSearched = FALSE;
	nodeNew->nindexParent = nindexParent;
	return m_nindexAvailableNode++;
}

node_index_t *CLocalNav::GetNode(int nindexCurrent)
{
	return &m_nodeArr[nindexCurrent];
}

int CLocalNav::NodeExists(int offsetX, int offsetY)
{
	int nindexCurrent;
	node_index_t *node = GetNode(m_nindexAvailableNode);

	if (m_nindexAvailableNode)
	{
		nindexCurrent = m_nindexAvailableNode - 1;

		while (1)
		{
			node--;
			nindexCurrent--;

			if (node->offsetX == offsetX && node->offsetY == offsetY)
				return nindexCurrent;

			if (!nindexCurrent)
				break;
		}
	}

	return -1;
}

void CLocalNav::AddPathNode(int nindexSource, int offsetX, int offsetY, BOOL fNoMonsters)
{
	BOOL bDepth;
	Vector vecSource, vecDest;
	int offsetXAbs, offsetYAbs;
	int xRevDir, yRevDir;

	if (nindexSource == -1)
	{
		offsetXAbs = offsetX;
		offsetYAbs = offsetY;
		bDepth = 1;
		vecSource = m_vecStartingLoc;
		vecDest.x = vecSource.x + (offsetX * HOSTAGE_STEPSIZE);
		vecDest.y = vecSource.y + (offsetY * HOSTAGE_STEPSIZE);
		vecDest.z = vecSource.z;
	}
	else
	{
		node_index_t *nodeSource;
		node_index_t *nodeCurrent;
		int nindexCurrent;

		nodeCurrent = GetNode(nindexSource);
		offsetXAbs = offsetX + nodeCurrent->offsetX;
		offsetYAbs = offsetY + nodeCurrent->offsetY;
		nodeSource = GetNode(m_nindexAvailableNode);

		if (NodeExists(offsetXAbs, offsetYAbs) != -1)
			return;

		vecSource = nodeCurrent->vecLoc;
		vecDest.x = vecSource.x + (offsetX * HOSTAGE_STEPSIZE);
		vecDest.y = vecSource.y + (offsetY * HOSTAGE_STEPSIZE);
		vecDest.z = vecSource.z;

		if (m_nindexAvailableNode)
		{
			nindexCurrent = m_nindexAvailableNode;

			do
			{
				nodeSource--;
				nindexCurrent--;
				xRevDir = nodeSource->offsetX - offsetXAbs;

				if (xRevDir >= 0)
				{
					if (xRevDir > 1)
						continue;
				}
				else
				{
					if (-xRevDir > 1)
						continue;
				}

				yRevDir = nodeSource->offsetY - offsetYAbs;

				if (yRevDir >= 0)
				{
					if (yRevDir > 1)
						continue;
				}
				else
				{
					if (-yRevDir > 1)
						continue;
				}

				if (PathTraversable(nodeSource->vecLoc, vecDest, fNoMonsters) != TRAVERSABLE_NO)
				{
					nodeCurrent = nodeSource;
					nindexSource = nindexCurrent;
				}
			}
			while (nindexCurrent);
		}

		vecSource = nodeCurrent->vecLoc;
		bDepth = nodeCurrent->bDepth + 1;
	}

	if (PathTraversable(vecSource, vecDest, fNoMonsters) != TRAVERSABLE_NO)
		AddNode(nindexSource, vecDest, offsetXAbs, offsetYAbs, bDepth);
}

void CLocalNav::AddPathNodes(int nindexSource, BOOL fNoMonsters)
{
	AddPathNode(nindexSource, 1, 0, fNoMonsters);
	AddPathNode(nindexSource, -1, 0, fNoMonsters);
	AddPathNode(nindexSource, 0, 1, fNoMonsters);
	AddPathNode(nindexSource, 0, -1, fNoMonsters);
	AddPathNode(nindexSource, 1, 1, fNoMonsters);
	AddPathNode(nindexSource, 1, -1, fNoMonsters);
	AddPathNode(nindexSource, -1, 1, fNoMonsters);
	AddPathNode(nindexSource, -1, -1, fNoMonsters);
}

int CLocalNav::SetupPathNodes(int nindex, Vector *vecNodes, BOOL fNoMonsters)
{
	int nCurrentIndex;
	int nNodeCount;
	node_index_t *nodeCurrent;
	Vector vecCurrentLoc;

	nNodeCount = 0;
	nCurrentIndex = nindex;

	while (nCurrentIndex != -1)
	{
		nodeCurrent = GetNode(nCurrentIndex);
		vecCurrentLoc = nodeCurrent->vecLoc;
		vecNodes[nNodeCount++] = vecCurrentLoc;
		nCurrentIndex = nodeCurrent->nindexParent;
	}

	return nNodeCount;
}

int CLocalNav::GetFurthestTraversableNode(Vector &vecStartingLoc, Vector *vecNodes, int nTotalNodes, BOOL fNoMonsters)
{
	int nCount = 0;

	while (nCount < nTotalNodes)
	{
		if (PathTraversable(vecStartingLoc, vecNodes[nCount], fNoMonsters) != TRAVERSABLE_NO)
			return nCount;

		nCount++;
	}

	return -1;
}

int CLocalNav::FindPath(Vector &vecStart, Vector &vecDest, float flTargetRadius, BOOL fNoMonsters)
{
	int nIndexBest;
	node_index_t *node;
	Vector vecNodeLoc;
	float flDistToDest;

#ifdef _DEBUG
	CONSOLE_ECHO("findpath: %f\n", gpGlobals->time);
#endif
	nIndexBest = FindDirectPath(vecStart, vecDest, flTargetRadius, fNoMonsters);

	if (nIndexBest != -1)
		return nIndexBest;

	m_vecStartingLoc = vecStart;
	m_nindexAvailableNode = 0;
	AddPathNodes(-1, fNoMonsters);

	vecNodeLoc = vecStart;
	nIndexBest = GetBestNode(vecNodeLoc, vecDest);

	while (nIndexBest != -1)
	{
		node = GetNode(nIndexBest);
		vecNodeLoc = node->vecLoc;
		node->fSearched = TRUE;
		flDistToDest = (vecDest - node->vecLoc).Length2D();

		if (flDistToDest <= flTargetRadius)
			break;

		if (flDistToDest <= HOSTAGE_STEPSIZE)
			break;

		if ((flDistToDest - flTargetRadius) > (MAX_NODES - m_nindexAvailableNode) * HOSTAGE_STEPSIZE || m_nindexAvailableNode == MAX_NODES)
		{
			nIndexBest = -1;
			break;
		}

		AddPathNodes(nIndexBest, fNoMonsters);
		nIndexBest = GetBestNode(vecNodeLoc, vecDest);
	}

	if (m_nindexAvailableNode <= 10)
		nodeval += 2;
	else if (m_nindexAvailableNode <= 20)
		nodeval += 4;
	else if (m_nindexAvailableNode <= 30)
		nodeval += 8;
	else if (m_nindexAvailableNode <= 40)
		nodeval += 13;
	else if (m_nindexAvailableNode <= 50)
		nodeval += 19;
	else if (m_nindexAvailableNode <= 60)
		nodeval += 26;
	else if (m_nindexAvailableNode <= 70)
		nodeval += 34;
	else if (m_nindexAvailableNode <= 80)
		nodeval += 43;
	else if (m_nindexAvailableNode <= 90)
		nodeval += 53;
	else if (m_nindexAvailableNode <= 100)
		nodeval += 64;
	else if (m_nindexAvailableNode <= 110)
		nodeval += 76;
	else if (m_nindexAvailableNode <= 120)
		nodeval += 89;
	else if (m_nindexAvailableNode <= 130)
		nodeval += 103;
	else if (m_nindexAvailableNode <= 140)
		nodeval += 118;
	else if (m_nindexAvailableNode <= 150)
		nodeval += 134;
	else if (m_nindexAvailableNode <= 160)
		nodeval += 151;
	else
		nodeval += 169;

	return nIndexBest;
}

int CLocalNav::FindDirectPath(Vector &vecStepStart, Vector &vecDest, float flTargetRadius, BOOL fNoMonsters)
{
	Vector vecActualDest;
	Vector vecPathDir;
	Vector vecNodeLoc;
	int nindexLast;

	vecPathDir = (vecDest - vecStepStart).Normalize();
	vecActualDest = vecDest - (vecPathDir * flTargetRadius);

	if (PathTraversable(vecStepStart, vecActualDest, fNoMonsters) == TRAVERSABLE_NO)
		return -1;

	nindexLast = -1;
	vecNodeLoc = vecStepStart;
	m_nindexAvailableNode = 0;

	while ((vecNodeLoc - vecActualDest).Length2D() >= HOSTAGE_STEPSIZE)
	{
		vecNodeLoc = vecNodeLoc + (vecPathDir * HOSTAGE_STEPSIZE);
		nindexLast = AddNode(nindexLast, vecNodeLoc, 0, 0, 0);

		if (nindexLast == -1)
			break;
	}

	return nindexLast;
}

int CLocalNav::PathClear(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters)
{
	TraceResult tr;
	return PathClear(vecSource, vecDest, fNoMonsters, tr);
}

int CLocalNav::PathClear(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr)
{
	TRACE_MONSTER_HULL(m_pOwner->edict(), vecSource, vecDest, fNoMonsters, m_pOwner->edict(), &tr);

	if (tr.fStartSolid)
		return 0;

	if (tr.flFraction == 1)
		return 1;

	if (tr.pHit == m_pTargetEnt)
	{
#ifdef _DEBUG
		CONSOLE_ECHO("target edict hit!\n");
#endif
		m_fTargetEntHit = TRUE;
		return 1;
	}

	return 0;
}

int CLocalNav::GetBestNode(Vector &vecOrigin, Vector &vecDest)
{
	int nindexCurrent;
	node_index_t *nodeCurrent;
	int nindexBest;
	float flBestVal;
	float flCurrentVal;
	float flDistFromStart;
	float flDistToDest;
	float flZDiff;
	int zDir;
	Vector vecDown;

	nindexBest = -1;
	nindexCurrent = 0;
	flBestVal = 1000000;

	while (nindexCurrent < m_nindexAvailableNode)
	{
		nodeCurrent = GetNode(nindexCurrent);

		if (nodeCurrent->fSearched != TRUE)
		{
			zDir = -1;
			vecDown = nodeCurrent->vecLoc - vecDest;
			flDistFromStart = vecDown.Length();

			if (vecDown.z >= 0)
				zDir = 1;

			flDistToDest = vecDown.z * zDir;

			if (flDistToDest <= s_flStepSize)
				flZDiff = 1;
			else
				flZDiff = 1.25;

			flCurrentVal = ((nodeCurrent->bDepth * HOSTAGE_STEPSIZE) + flDistFromStart) * flZDiff;

			if (flBestVal > flCurrentVal)
			{
				flBestVal = flCurrentVal;
				nindexBest = nindexCurrent;
			}
		}

		nindexCurrent++;
	}

	return nindexBest;
}

int CLocalNav::PathTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters)
{
	TraceResult tr;
	Vector vecSrcTmp;
	Vector vecDestTmp;
	Vector vecDir;
	Vector vecTmp;
	float flTotal;
	int retval;

	retval = TRAVERSABLE_NO;
	vecSrcTmp = vecSource;
	vecDestTmp = vecDest - vecSource;
	vecDir = vecDestTmp.Normalize();
	vecDir.z = 0;
	flTotal = vecDestTmp.Length2D();

	while (flTotal > 1)
	{
		if (flTotal >= s_flStepSize)
			vecDestTmp = vecSrcTmp + (vecDir * s_flStepSize);
		else
			vecDestTmp = vecDest;

		m_fTargetEntHit = FALSE;

		if (PathClear(vecSrcTmp, vecDestTmp, fNoMonsters, tr))
		{
			vecDestTmp = tr.vecEndPos;

			if (retval == TRAVERSABLE_NO)
				retval = TRAVERSABLE_SLOPE;
		}
		else
		{
			if (tr.fStartSolid)
				return TRAVERSABLE_NO;

			if (tr.pHit && !fNoMonsters && !FStringNull(tr.pHit->v.classname) && FClassnameIs(tr.pHit, "hostage_entity"))
				return TRAVERSABLE_NO;

			vecSrcTmp = tr.vecEndPos;

			if (tr.vecPlaneNormal.z > 0.7)
			{
				if (SlopeTraversable(vecSrcTmp, vecDestTmp, fNoMonsters, tr))
				{
					if (retval == TRAVERSABLE_NO)
						retval = TRAVERSABLE_SLOPE;
				}
				else
					return TRAVERSABLE_NO;
			}
			else
			{
				if (StepTraversable(vecSrcTmp, vecDestTmp, fNoMonsters, tr))
				{
					if (retval == TRAVERSABLE_NO)
						retval = TRAVERSABLE_STEP;
				}
				else if (StepJumpable(vecSrcTmp, vecDestTmp, fNoMonsters, tr))
				{
					if (retval == TRAVERSABLE_NO)
						retval = TRAVERSABLE_STEPJUMPABLE;
				}
				else
					return TRAVERSABLE_NO;
			}
		}
		vecTmp = vecDestTmp - Vector(0, 0, 300);
		if (PathClear(vecDestTmp, vecTmp, fNoMonsters, tr))
			return TRAVERSABLE_NO;

		if (!tr.fStartSolid)
			vecDestTmp = tr.vecEndPos;

		vecSrcTmp = vecDestTmp;

		if (!m_fTargetEntHit)
			flTotal = (vecDest - vecDestTmp).Length2D();
		else
			flTotal = 0;
	}

	vecDest = vecDestTmp;
	return retval;
}

int CLocalNav::SlopeTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr)
{
	Vector vecSlopeStart;
	Vector vecSlopeEnd;
	Vector vecDown;
	Vector vecAngles;

	vecSlopeStart = vecSource;
	vecSlopeEnd = vecDest;
	vecDown = vecDest - vecSource;
	vecAngles = UTIL_VecToAngles(tr.vecPlaneNormal);
	vecSlopeEnd.z = vecDown.Length2D() * tan((90 - vecAngles.x) * (M_PI / 180)) + vecSource.z;

	if (!PathClear(vecSlopeStart, vecSlopeEnd, fNoMonsters, tr))
	{
		if (tr.fStartSolid)
			return 0;

		if ((tr.vecEndPos - vecSlopeStart).Length() < 1)
			return 0;
	}

	vecSlopeStart = tr.vecEndPos;
	vecSlopeEnd = vecSlopeStart;
	vecSlopeEnd.z -= s_flStepSize;

	if (!PathClear(vecSlopeStart, vecSlopeEnd, fNoMonsters, tr))
	{
		if (tr.fStartSolid)
		{
			vecDest = vecSlopeStart;
			return 1;
		}
	}

	vecDest = tr.vecEndPos;
	return 1;
}

int CLocalNav::StepTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr)
{
	Vector vecStepStart;
	Vector vecStepDest;
	BOOL fFwdTrace;
	float flFwdFraction;

	vecStepStart = vecSource;
	vecStepDest = vecDest;
	vecStepStart.z += s_flStepSize;
	vecStepDest.z = vecStepStart.z;
	fFwdTrace = FALSE;

	if (!PathClear(vecStepStart, vecStepDest, fNoMonsters, tr))
	{
		if (tr.fStartSolid)
			return 0;

		flFwdFraction = (tr.vecEndPos - vecStepStart).Length();

		if (flFwdFraction < 1)
			return 0;
	}

	vecStepStart = tr.vecEndPos;
	vecStepDest = vecStepStart;
	vecStepDest.z -= s_flStepSize;

	if (!PathClear(vecStepStart, vecStepDest, fNoMonsters, tr))
	{
		if (tr.fStartSolid)
		{
			vecDest = vecStepStart;
			return 1;
		}
	}

	vecDest = tr.vecEndPos;
	return 1;
}

int CLocalNav::StepJumpable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr)
{
	Vector vecStepStart;
	Vector vecStepDest;
	BOOL fFwdTrace;
	float flFwdFraction;
	float flJumpHeight;
	BOOL fJumpClear;

	flJumpHeight = s_flStepSize + 1;
	vecStepStart = vecSource;
	vecStepStart.z += flJumpHeight;
	fFwdTrace = FALSE;
	fJumpClear = FALSE;

	while (vecStepStart.z < 40)
	{
		vecStepDest = vecDest;
		vecStepDest.z = vecStepStart.z;

		if (!PathClear(vecStepStart, vecStepDest, fNoMonsters, tr))
		{
			if (tr.fStartSolid)
				return 0;

			flFwdFraction = (tr.vecEndPos - vecStepStart).Length();

			if (flFwdFraction < 1)
			{
				flJumpHeight += 10;
				vecStepStart.z += 10;
				continue;
			}
		}

		vecStepStart = tr.vecEndPos;
		vecStepDest = vecStepStart;
		vecStepDest.z -= s_flStepSize;

		if (!PathClear(vecStepStart, vecStepDest, fNoMonsters, tr))
		{
			if (tr.fStartSolid)
			{
				vecDest = vecStepStart;
				return 1;
			}
		}

		vecDest = tr.vecEndPos;
		return 1;
	}

	return 0;
}

int CLocalNav::LadderTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr)
{
	Vector vecStepStart;
	Vector vecStepDest;

	vecStepStart = tr.vecEndPos;
	vecStepDest = vecStepStart;
	vecStepDest.z += HOSTAGE_STEPSIZE;

	if (!PathClear(vecStepStart, vecStepDest, fNoMonsters, tr))
	{
		if (tr.fStartSolid)
			return 0;

		if ((tr.vecEndPos - vecStepStart).Length() < 1)
			return 0;
	}

	vecStepStart = tr.vecEndPos;
	vecDest.z = tr.vecEndPos.z;
	return PathTraversable(vecStepStart, vecDest, fNoMonsters);
}

bool CLocalNav::LadderHit(Vector &vecSource, Vector &vecDest, TraceResult &tr)
{
	Vector vecFwd, vecRight, vecUp;
	Vector vecAngles;
	Vector vecOrigin;

	vecAngles = UTIL_VecToAngles(-tr.vecPlaneNormal);
	UTIL_MakeVectorsPrivate(vecAngles, vecFwd, vecRight, vecUp);
	vecOrigin = tr.vecEndPos + (vecFwd * 15) + (vecUp * 36);

	if (UTIL_PointContents(vecOrigin) == CONTENTS_LADDER)
		return true;

	vecOrigin = tr.vecEndPos + (vecFwd * 15) - (vecUp * 36);

	if (UTIL_PointContents(vecOrigin) == CONTENTS_LADDER)
		return true;

	vecOrigin = tr.vecEndPos + (vecFwd * 15) + (vecRight * 16) + (vecUp * 36);

	if (UTIL_PointContents(vecOrigin) == CONTENTS_LADDER)
		return true;

	vecOrigin = tr.vecEndPos + (vecFwd * 15) - (vecRight * 16) + (vecUp * 36);

	if (UTIL_PointContents(vecOrigin) == CONTENTS_LADDER)
		return true;

	vecOrigin = tr.vecEndPos + (vecFwd * 15) + (vecRight * 16) - (vecUp * 36);

	if (UTIL_PointContents(vecOrigin) == CONTENTS_LADDER)
		return true;

	vecOrigin = tr.vecEndPos + (vecFwd * 15) - (vecRight * 16) + (vecUp * 36);

	if (UTIL_PointContents(vecOrigin) == CONTENTS_LADDER)
		return true;

	return false;
}

void CLocalNav::HostagePrethink(void)
{
	for (int i = 0; i < tot_hostages; i++)
	{
		if (hostages[i] != NULL)
			GetClassPtr((CHostage *)hostages[i]->pev)->PreThink();
	}
}

void CLocalNav::Think(void)
{
	EHANDLE hostage;
	static cvar_t *sv_stepsize = NULL;

	if (flNextCvarCheck <= gpGlobals->time)
	{
		if (sv_stepsize)
		{
			flNextCvarCheck = gpGlobals->time + 1;
			s_flStepSize = sv_stepsize->value;
		}
		else
		{
			sv_stepsize = CVAR_GET_POINTER("sv_stepsize");
			flNextCvarCheck = gpGlobals->time + 1;
			s_flStepSize = s_flStepSize ? sv_stepsize->value : 18;
		}
	}

	HostagePrethink();
	nodeval -= (gpGlobals->time - flLastThinkTime) * 250;
	flLastThinkTime = gpGlobals->time;

	if (nodeval < 0)
		nodeval = 0;
	else if (nodeval > 17)
		return;

	if (tot_inqueue)
	{
		hostage = NULL;
		hostage = queue[qptr];

		if (hostage == NULL)
		{
			while (tot_inqueue > 0)
			{
				if (++qptr == MAX_HOSTAGES)
					qptr = 0;

				tot_inqueue--;

				if (!tot_inqueue)
				{
					hostage = NULL;
					break;
				}

				hostage = queue[qptr];

				if (hostage)
					break;
			}
		}

		if (hostage)
		{
			CHostage *pHostage = GetClassPtr((CHostage *)hostage->pev);

			if (++qptr == MAX_HOSTAGES)
				qptr = 0;

			tot_inqueue--;
			pHostage->NavReady();
		}
	}
}

void CLocalNav::RequestNav(CHostage *pOwner)
{
	if (nodeval > 17 || tot_inqueue)
	{
		if (tot_inqueue >= MAX_HOSTAGES)
		{
			CONSOLE_ECHO("queue full\n");
			return;
		}

		int i = qptr;

		for (int j = 0; j < tot_inqueue; j++)
		{
			CHostage *pHostage = GetClassPtr((CHostage *)queue[i]->pev);

			if (pHostage == pOwner)
				return;

			if (++i == MAX_HOSTAGES)
				i = 0;
		}

		queue[i] = pOwner;
		tot_inqueue++;
	}
	else
		pOwner->NavReady();
}

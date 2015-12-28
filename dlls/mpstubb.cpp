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
#include "nodes.h"
#include "talkmonster.h"

float CTalkMonster::g_talkWaitTime = 0;

CGraph WorldGraph;
void CGraph::InitGraph(void) {}
int CGraph::FLoadGraph(char *szMapName) { return FALSE; }
int CGraph::AllocNodes(void) { return FALSE; }
int CGraph::CheckNODFile(char *szMapName) { return FALSE; }
int CGraph::FSetGraphPointers(void) { return 0; }
void CGraph::ShowNodeConnections(int iNode) {}
int CGraph::FindNearestNode(const Vector &vecOrigin, int afNodeTypes) { return 0; }

void CBaseMonster::ReportAIState(void) {}
float CBaseMonster::ChangeYaw(int speed) { return 0; }
void CBaseMonster::MakeIdealYaw(Vector vecTarget) {}

void CBaseMonster::CorpseFallThink(void)
{
	if (pev->flags & FL_ONGROUND)
	{
		SetThink(NULL);
		SetSequenceBox();
		UTIL_SetOrigin(pev, pev->origin);
	}
	else
		pev->nextthink = gpGlobals->time + 0.1;
}

void CBaseMonster::MonsterInitDead(void)
{
	InitBoneControllers();

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS;

	pev->frame = 0;
	ResetSequenceInfo();
	pev->framerate = 0;

	pev->max_health = pev->health;
	pev->deadflag = DEAD_DEAD;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);

	BecomeDead();
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.5;
}

BOOL CBaseMonster::ShouldFadeOnDeath(void)
{
	return FALSE;
}

BOOL CBaseMonster::FCheckAITrigger(void)
{
	return FALSE;
}

void CBaseMonster::KeyValue(KeyValueData *pkvd)
{
	CBaseToggle::KeyValue(pkvd);
}

int CBaseMonster::IRelationship(CBaseEntity *pTarget)
{
	static int iEnemy[14][14] =
	{
		{ R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO },
		{ R_NO, R_NO, R_DL, R_DL, R_NO, R_DL, R_DL, R_DL, R_DL, R_DL, R_NO, R_DL, R_DL, R_DL },
		{ R_NO, R_DL, R_NO, R_NO, R_DL, R_DL, R_DL, R_DL, R_DL, R_DL, R_NO, R_NO, R_DL, R_DL },
		{ R_NO, R_NO, R_AL, R_AL, R_HT, R_FR, R_NO, R_HT, R_DL, R_FR, R_NO, R_AL, R_NO, R_NO },
		{ R_NO, R_NO, R_HT, R_DL, R_NO, R_HT, R_DL, R_DL, R_DL, R_DL, R_NO, R_HT, R_NO, R_NO },
		{ R_NO, R_DL, R_HT, R_DL, R_HT, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_DL, R_NO, R_NO },
		{ R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO },
		{ R_NO, R_DL, R_DL, R_DL, R_DL, R_NO, R_NO, R_NO, R_NO, R_NO, R_NO, R_DL, R_NO, R_NO },
		{ R_NO, R_NO, R_DL, R_DL, R_DL, R_NO, R_NO, R_NO, R_NO, R_FR, R_NO, R_DL, R_NO, R_NO },
		{ R_NO, R_NO, R_DL, R_DL, R_DL, R_NO, R_NO, R_NO, R_HT, R_DL, R_NO, R_DL, R_NO, R_NO },
		{ R_FR, R_FR, R_FR, R_FR, R_FR, R_NO, R_FR, R_FR, R_FR, R_FR, R_NO, R_FR, R_NO, R_NO },
		{ R_NO, R_DL, R_AL, R_AL, R_DL, R_DL, R_DL, R_DL, R_DL, R_DL, R_NO, R_NO, R_NO, R_NO },
		{ R_NO, R_NO, R_DL, R_DL, R_DL, R_DL, R_DL, R_DL, R_DL, R_DL, R_NO, R_DL, R_NO, R_DL },
		{ R_NO, R_NO, R_DL, R_DL, R_DL, R_AL, R_NO, R_DL, R_DL, R_NO, R_NO, R_DL, R_DL, R_NO }
	};

	return iEnemy[Classify()][pTarget->Classify()];
}

void CBaseMonster::Look(int iDistance)
{
	int iSighted = 0;
	ClearConditions(bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR | bits_COND_SEE_NEMESIS | bits_COND_SEE_CLIENT);
	m_pLink = NULL;

	CBaseEntity *pSightEnt = NULL;
	CBaseEntity *pList[100];
	Vector delta = Vector(iDistance, iDistance, iDistance);
	int count = UTIL_EntitiesInBox(pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT | FL_MONSTER);

	for (int i = 0; i < count; i++)
	{
		pSightEnt = pList[i];

		if (pSightEnt != this && pSightEnt->pev->health > 0)
		{
			if (IRelationship(pSightEnt) != R_NO && FInViewCone(pSightEnt) && !FBitSet(pSightEnt->pev->flags, FL_NOTARGET) && FVisible(pSightEnt))
			{
				if (pSightEnt->IsPlayer())
					iSighted |= bits_COND_SEE_CLIENT;

				pSightEnt->m_pLink = m_pLink;
				m_pLink = pSightEnt;

				if (pSightEnt == m_hEnemy)
					iSighted |= bits_COND_SEE_ENEMY;

				switch (IRelationship(pSightEnt))
				{
					case R_NM: iSighted |= bits_COND_SEE_NEMESIS; break;
					case R_HT: iSighted |= bits_COND_SEE_HATE; break;
					case R_DL: iSighted |= bits_COND_SEE_DISLIKE; break;
					case R_FR: iSighted |= bits_COND_SEE_FEAR; break;
					case R_AL: break;
				}
			}
		}
	}

	SetConditions(iSighted);
}

CBaseEntity *CBaseMonster::BestVisibleEnemy(void)
{
	int iNearest = 8192;
	CBaseEntity *pNextEnt = m_pLink;
	CBaseEntity *pReturn = NULL;
	int iBestRelationship = R_NO;

	while (pNextEnt)
	{
		if (pNextEnt->IsAlive())
		{
			if (IRelationship(pNextEnt) > iBestRelationship)
			{
				iBestRelationship = IRelationship(pNextEnt);
				iNearest = (int)((pNextEnt->pev->origin - pev->origin).Length());
				pReturn = pNextEnt;
			}
			else if (IRelationship(pNextEnt) == iBestRelationship)
			{
				int iDist = (int)((pNextEnt->pev->origin - pev->origin).Length());

				if (iDist <= iNearest)
				{
					iNearest = iDist;
					iBestRelationship = IRelationship(pNextEnt);
					pReturn = pNextEnt;
				}
			}
		}

		pNextEnt = pNextEnt->m_pLink;
	}

	return pReturn;
}
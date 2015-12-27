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
#define MAX_HOSTAGES 20
#define HOSTAGE_STEPSIZE 26.0
#define MAX_NODES 100

#define VEC_HOSTAGE_VIEW Vector(0, 0, 12)
#define VEC_HOSTAGE_HULL_MIN Vector(-10, -10, 0)
#define VEC_HOSTAGE_HULL_MAX Vector(10, 10, 62)

class CLocalNav;

class CHostage : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void EXPORT IdleThink(void);
	void EXPORT Remove(void);
	int Classify(void) { return CLASS_HUMAN_PASSIVE; }
	int ObjectCaps(void);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Touch(CBaseEntity *pOther);
	int BloodColor(void);
	void NavReady(void);
	void PreThink(void);
	void RePosition(void);
	void PointAt(Vector &origin);
	void DoFollow(void);
	void MoveToward(Vector &vecLoc);
	void Wiggle(void);
	void SendHostageEventMsg(void);
	void SendHostagePositionMsg(void);
	void SetActivity(Activity activity);
	void SetFlinchActivity(void);
	void SetDeathActivity(void);
	float GetModifiedDamage(float flDamage);
	void PlayPainSound(void);
	void PlayFollowRescueSound(void);
	void AnnounceDeath(CBasePlayer *pKiller);
	void ApplyHostagePenalty(CBasePlayer *pKiller);
	void GiveCTTouchBonus(CBasePlayer *pPlayer);
	BOOL IsOnLadder(void) { return pev->movetype == MOVETYPE_TOSS; }
	Activity GetActivity(void) { return m_Activity; }

public:
	typedef enum { FOLLOW, STAND, DUCK, SCARED, IDLE, FOLLOWPATH } state;

public:
	//Activity m_Activity;//Already exists in CBaseMonster
	BOOL m_bTouched;
	BOOL m_bRescueMe;
	float m_flFlinchTime;
	float m_flNextChange;
	float m_flMarkPosition;
	int m_iModel;
	int m_iSkin;
	float m_flNextRadarTime;
	state m_State;
	Vector m_vStart;
	Vector m_vStartAngles;
	Vector m_vPathToFollow[20];
	int m_iWaypoint;
	CBaseEntity *m_target;
	CLocalNav *m_LocalNav;
	int nTargetNode;
	Vector vecNodes[MAX_NODES];
	EHANDLE m_hStoppedTargetEnt;
	float m_flNextFullThink;
	float m_flPathCheckInterval;
	float m_flLastPathCheck;
	int m_nPathNodes;
	BOOL m_fHasPath;
	float m_flPathAcquired;
	Vector m_vOldPos;
	int m_iHostageIndex;
	BOOL m_bStuck;
	float m_flStuckTime;
	void *m_improv;
	int m_whichModel;
};

extern int g_iHostageNumber;

typedef struct localnode_s
{
	Vector vecLoc;
	int offsetX, offsetY;
	BOOL bDepth;
	BOOL fSearched;
	int nindexParent;
}
node_index_t;

#define TRAVERSABLE_NO 0
#define TRAVERSABLE_SLOPE 1
#define TRAVERSABLE_STEP 2
#define TRAVERSABLE_STEPJUMPABLE 3

class CLocalNav
{
public:
	CLocalNav(CHostage *pOwner);
	~CLocalNav(void);

public:
	static void Reset(void);
	static void HostagePrethink(void);
	static void Think(void);
	static void RequestNav(CHostage *pOwner);

public:
	int AddNode(int nindexParent, Vector &vecLoc, int offsetX, int offsetY, BOOL bDepth);
	node_index_t *GetNode(int nindexCurrent);
	void AddPathNode(int nindexSource, int offsetX, int offsetY, BOOL fNoMonsters);
	void AddPathNodes(int nindexSource, BOOL fNoMonsters);
	int NodeExists(int offsetX, int offsetY);
	int PathClear(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters);
	int PathClear(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr);
	int GetBestNode(Vector &vecOrigin, Vector &vecDest);
	int SetupPathNodes(int nindex, Vector *vecNodes, BOOL fNoMonsters);
	int StepTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr);
	int StepJumpable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr);
	int SlopeTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr);
	int LadderTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters, TraceResult &tr);
	int PathTraversable(Vector &vecSource, Vector &vecDest, BOOL fNoMonsters);
	int GetFurthestTraversableNode(Vector &vecStartingLoc, Vector *vecNodes, int nTotalNodes, BOOL fNoMonsters);
	bool LadderHit(Vector &vecSource, Vector &vecDest, TraceResult &tr);
	int FindPath(Vector &vecStart, Vector &vecDest, float flTargetRadius, BOOL fNoMonsters);
	int FindDirectPath(Vector &vecStart, Vector &vecDest, float flTargetRadius, BOOL fNoMonsters);
	void SetTargetEnt(CBaseEntity *pTargetEnt);

public:
	static float s_flStepSize;
	static int qptr;
	static EHANDLE queue[MAX_HOSTAGES];
	static int tot_inqueue;
	static float nodeval;
	static float flNextCvarCheck;
	static float flLastThinkTime;
	static EHANDLE hostages[MAX_HOSTAGES];
	static int tot_hostages;

private:
	CHostage *m_pOwner;
	edict_t *m_pTargetEnt;
	int m_fTargetEntHit;
	node_index_t *m_nodeArr;
	int m_nindexAvailableNode;
	Vector m_vecStartingLoc;
};
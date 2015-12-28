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
#ifndef TRAINS_H
#define TRAINS_H

#define SF_TRACKTRAIN_NOPITCH 0x0001
#define SF_TRACKTRAIN_NOCONTROL 0x0002
#define SF_TRACKTRAIN_FORWARDONLY 0x0004
#define SF_TRACKTRAIN_PASSABLE 0x0008

#define SF_PATH_DISABLED 0x00000001
#define SF_PATH_FIREONCE 0x00000002
#define SF_PATH_ALTREVERSE 0x00000004
#define SF_PATH_DISABLE_TRAIN 0x00000008
#define SF_PATH_ALTERNATE 0x00008000

#define SF_CORNER_WAITFORTRIG 0x001
#define SF_CORNER_TELEPORT 0x002
#define SF_CORNER_FIREONCE 0x004

class CPathTrack : public CPointEntity
{
public:
	void Spawn(void);
	void Activate(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void SetPrevious(CPathTrack *pprevious);
	void Link(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	CPathTrack *ValidPath(CPathTrack *ppath, int testFlag);
	void Project(CPathTrack *pstart, CPathTrack *pend, Vector *origin, float dist);

public:
	static CPathTrack *Instance(edict_t *pent);

public:
	CPathTrack *LookAhead(Vector *origin, float dist, int move);
	CPathTrack *Nearest(Vector origin);
	CPathTrack *GetNext(void);
	CPathTrack *GetPrevious(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_length;
	string_t m_altName;
	CPathTrack *m_pnext;
	CPathTrack *m_pprevious;
	CPathTrack *m_paltpath;
};

class CFuncTrackTrain : public CBaseEntity
{
public:
	void Spawn(void);
	void Restart(void);
	void Precache(void);
	void Blocked(CBaseEntity *pOther);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DIRECTIONAL_USE; }
	void OverrideReset(void);

public:
	void EXPORT Next(void);
	void EXPORT Find(void);
	void EXPORT NearestPath(void);
	void EXPORT DeadEnd(void);

public:
	void NextThink(float thinkTime, BOOL alwaysThink);
	void SetTrack(CPathTrack *track) { m_ppath = track->Nearest(pev->origin); }
	void SetControls(entvars_t *pevControls);
	BOOL OnControls(entvars_t *pev);
	void StopSound(void);
	void UpdateSound(void);

public:
	static CFuncTrackTrain *Instance(edict_t *pent);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	CPathTrack *m_ppath;
	float m_length;
	float m_height;
	float m_speed;
	float m_dir;
	float m_startSpeed;
	Vector m_controlMins;
	Vector m_controlMaxs;
	int m_soundPlaying;
	int m_sounds;
	float m_flVolume;
	float m_flBank;
	float m_oldSpeed;

private:
	unsigned short m_usAdjustPitch;
};

#endif
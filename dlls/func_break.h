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
#ifndef FUNC_BREAK_H
#define FUNC_BREAK_H

typedef enum { expRandom, expDirected } Explosions;
typedef enum { matGlass = 0, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile, matComputer, matUnbreakableGlass, matRocks, matNone, matLastMaterial } Materials;

#define NUM_SHARDS 6

class CBreakable : public CBaseDelay
{
public:
	void Spawn(void);
	void Restart(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void DamageSound(void);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	BOOL IsBreakable(void);
	BOOL SparkWhenHit(void);
	int DamageDecal(int bitsDamageType);
	int ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT Die(void);
	void EXPORT BreakTouch(CBaseEntity *pOther);

public:
	inline BOOL Explodable(void) { return ExplosionMagnitude() > 0; }
	inline int ExplosionMagnitude(void) { return pev->impulse; }
	inline void ExplosionSetMagnitude(int magnitude) { pev->impulse = magnitude; }

public:
	static void MaterialSoundPrecache(Materials precacheMaterial);
	static void MaterialSoundRandom(edict_t *pEdict, Materials soundMaterial, float volume);
	static const char **MaterialSoundList(Materials precacheMaterial, int &soundCount);

public:
	static const char *pSoundsWood[];
	static const char *pSoundsFlesh[];
	static const char *pSoundsGlass[];
	static const char *pSoundsMetal[];
	static const char *pSoundsConcrete[];
	static const char *pSpawnObjects[];

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	Materials m_Material;
	Explosions m_Explosion;
	int m_idShard;
	float m_angle;
	int m_iszGibModel;
	int m_iszSpawnObject;
	float m_flSaveHealth;
};

#endif
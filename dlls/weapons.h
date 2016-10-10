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
#ifndef WEAPONS_H
#define WEAPONS_H

#include "effects.h"

class CBasePlayer;
extern int gmsgWeapPickup;
extern int gmsgReloadSound;

class CGrenade : public CBaseMonster
{
public:
	void Spawn(void);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	virtual void BounceSound(void);
	int ObjectCaps(void) { return m_bIsC4 != false ? FCAP_CONTINUOUS_USE : 0; }
	int BloodColor(void) { return DONT_BLEED; }
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Killed(entvars_t *pevAttacker, int iGib);

public:
	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;

public:
#ifndef CLIENT_DLL
	static CGrenade *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time);
	static CGrenade *ShootTimed2(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam, unsigned short usEvent);
	static CGrenade *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	static CGrenade *ShootSmokeGrenade(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, unsigned short usEvent);
	static CGrenade *ShootSatchelCharge(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
#else
	static CGrenade *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time) { return NULL; }
	static CGrenade *ShootTimed2(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam, unsigned short usEvent) { return NULL; }
	static CGrenade *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity) { return NULL; }
	static CGrenade *ShootSmokeGrenade(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, unsigned short usEvent) { return NULL; }
	static CGrenade *ShootSatchelCharge(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity) { return NULL; }
#endif
	static void UseSatchelCharges(entvars_t *pevOwner, SATCHELCODE code);

public:
	void Explode(Vector vecSrc, Vector vecAim);
	void Explode(TraceResult *pTrace, int bitsDamageType);
	void Explode2(TraceResult *pTrace, int bitsDamageType);
	void Explode3(TraceResult *pTrace, int bitsDamageType);
	void EXPORT Smoke(void);
	void EXPORT SG_Smoke(void);
	void EXPORT Smoke2(void);
	void EXPORT Smoke3_A(void);
	void EXPORT Smoke3_B(void);
	void EXPORT Smoke3_C(void);
	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT SlideTouch(CBaseEntity *pOther);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);
	void EXPORT DangerSoundThink(void);
	void EXPORT PreDetonate(void);
	void EXPORT Detonate(void);
	void EXPORT SG_Detonate(void);
	void EXPORT Detonate2(void);
	void EXPORT Detonate3(void);
	void EXPORT DetonateUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TumbleThink(void);
	void EXPORT SG_TumbleThink(void);
	void EXPORT C4Think(void);
	void EXPORT C4Touch(CBaseEntity *pOther) {}

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	bool m_bStartDefuse;
	bool m_bIsC4;
	EHANDLE m_pBombDefuser;
	float m_flDefuseCountDown;
	float m_flC4Blow;
	float m_flNextFreqInterval;
	float m_flNextBeep;
	float m_flNextFreq;
	char *m_sBeepName;
	float m_fAttenu;
	float m_flNextBlink;
	float m_fNextDefuse;
	bool m_bJustBlew;
	int m_iTeam;
	int m_iCurWave;
	edict_t *m_pentCurBombTarget;
	int m_SGSmoke;
	int m_angle;
	unsigned short m_usEvent;
	bool m_bLightSmoke;
	bool m_bDetonated;
	Vector m_vSmokeDetonate;
	int m_iBounceCount;
	BOOL m_fRegisteredSound;
};

#define ITEM_HEALTHKIT 1
#define ITEM_ANTIDOTE 2
#define ITEM_SECURITY 3
#define ITEM_BATTERY 4

enum WeaponIdType
{
	WEAPON_NONE,
	WEAPON_P228,
	WEAPON_GLOCK,
	WEAPON_SCOUT,
	WEAPON_HEGRENADE,
	WEAPON_XM1014,
	WEAPON_C4,
	WEAPON_MAC10,
	WEAPON_AUG,
	WEAPON_SMOKEGRENADE,
	WEAPON_ELITE,
	WEAPON_FIVESEVEN,
	WEAPON_UMP45,
	WEAPON_SG550,
	WEAPON_GALIL,
	WEAPON_FAMAS,
	WEAPON_USP,
	WEAPON_GLOCK18,
	WEAPON_AWP,
	WEAPON_MP5N,
	WEAPON_M249,
	WEAPON_M3,
	WEAPON_M4A1,
	WEAPON_TMP,
	WEAPON_G3SG1,
	WEAPON_FLASHBANG,
	WEAPON_DEAGLE,
	WEAPON_SG552,
	WEAPON_AK47,
	WEAPON_KNIFE,
	WEAPON_P90,
	WEAPON_SHIELDGUN = 99
};

#define WEAPON_ALLWEAPONS (~(1 << WEAPON_SUIT))
#define WEAPON_SUIT 31
#define MAX_WEAPONS 32

#define AK47_WEIGHT 25
#define AUG_WEIGHT 25
#define AWP_WEIGHT 30
#define C4_WEIGHT 3
#define DEAGLE_WEIGHT 7
#define ELITE_WEIGHT 5
#define FAMAS_WEIGHT 75
#define FIVESEVEN_WEIGHT 5
#define FLASHBANG_WEIGHT 1
#define G3SG1_WEIGHT 20
#define GALIL_WEIGHT 25
#define GLOCK18_WEIGHT 5
#define HEGRENADE_WEIGHT 2
#define KNIFE_WEIGHT 0
#define M249_WEIGHT 25
#define M3_WEIGHT 20
#define M4A1_WEIGHT 25
#define MAC10_WEIGHT 25
#define MP5NAVY_WEIGHT 25
#define P228_WEIGHT 5
#define P90_WEIGHT 26
#define SCOUT_WEIGHT 30
#define SG550_WEIGHT 20
#define SG552_WEIGHT 25
#define SMOKEGRENADE_WEIGHT 1
#define TMP_WEIGHT 25
#define UMP45_WEIGHT 25
#define USP_WEIGHT 5
#define XM1014_WEIGHT 20

#define MAX_NORMAL_BATTERY 100

#define WEAPON_NOCLIP -1

#define AK47_DEFAULT_GIVE 30
#define AUG_DEFAULT_GIVE 30
#define AWP_DEFAULT_GIVE 10
#define C4_DEFAULT_GIVE 1
#define DEAGLE_DEFAULT_GIVE 7
#define ELITE_DEFAULT_GIVE 30
#define FAMAS_DEFAULT_GIVE 25
#define FIVESEVEN_DEFAULT_GIVE 20
#define FLASHBANG_DEFAULT_GIVE 1
#define G3SG1_DEFAULT_GIVE 20
#define GALIL_DEFAULT_GIVE 35
#define GLOCK18_DEFAULT_GIVE 20
#define HEGRENADE_DEFAULT_GIVE 1
#define M249_DEFAULT_GIVE 100
#define M3_DEFAULT_GIVE 8
#define M4A1_DEFAULT_GIVE 30
#define MAC10_DEFAULT_GIVE 30
#define MP5NAVY_DEFAULT_GIVE 30
#define P228_DEFAULT_GIVE 13
#define P90_DEFAULT_GIVE 50
#define SCOUT_DEFAULT_GIVE 10
#define SG550_DEFAULT_GIVE 30
#define SG552_DEFAULT_GIVE 30
#define MAC10_DEFAULT_GIVE 30
#define SMOKEGRENADE_DEFAULT_GIVE 1
#define TMP_DEFAULT_GIVE 30
#define UMP45_DEFAULT_GIVE 25
#define USP_DEFAULT_GIVE 12
#define XM1014_DEFAULT_GIVE 7

#define AMMO_9MM_GIVE 30
#define AMMO_BUCKSHOT_GIVE 8
#define AMMO_556NATO_GIVE 30
#define AMMO_556NATOBOX_GIVE 30
#define AMMO_762NATO_GIVE 30
#define AMMO_45ACP_GIVE 12
#define AMMO_50AE_GIVE 7
#define AMMO_338MAGNUM_GIVE 10
#define AMMO_57MM_GIVE 50
#define AMMO_357SIG_GIVE 13

#define _9MM_MAX_CARRY MAX_AMMO_9MM
#define BUCKSHOT_MAX_CARRY MAX_AMMO_BUCKSHOT
#define _556NATO_MAX_CARRY MAX_AMMO_556NATO
#define _556NATOBOX_MAX_CARRY MAX_AMMO_556NATOBOX
#define _762NATO_MAX_CARRY MAX_AMMO_762NATO
#define _45ACP_MAX_CARRY MAX_AMMO_45ACP
#define _50AE_MAX_CARRY MAX_AMMO_50AE
#define _338MAGNUM_MAX_CARRY MAX_AMMO_338MAGNUM
#define _57MM_MAX_CARRY MAX_AMMO_57MM
#define _357SIG_MAX_CARRY MAX_AMMO_357SIG

#define HEGRENADE_MAX_CARRY 1
#define FLASHBANG_MAX_CARRY 2
#define SMOKEGRENADE_MAX_CARRY 1
#define C4_MAX_CARRY 1

enum AmmoCostType
{
	AMMO_338MAG_PRICE = 125,
	AMMO_357SIG_PRICE = 50,
	AMMO_45ACP_PRICE = 25,
	AMMO_50AE_PRICE = 40,
	AMMO_556NATO_PRICE = 60,
	AMMO_57MM_PRICE = 50,
	AMMO_762NATO_PRICE = 80,
	AMMO_9MM_PRICE = 20,
	AMMO_BUCKSHOT_PRICE = 65
};

enum WeaponCostType
{
	AK47_PRICE = 2500,
	AWP_PRICE = 4750,
	DEAGLE_PRICE = 650,
	G3SG1_PRICE = 5000,
	SG550_PRICE = 4200,
	GLOCK18_PRICE = 400,
	M249_PRICE = 5750,
	M3_PRICE = 1700,
	M4A1_PRICE = 3100,
	AUG_PRICE = 3500,
	MP5NAVY_PRICE = 1500,
	P228_PRICE = 600,
	P90_PRICE = 2350,
	UMP45_PRICE = 1700,
	MAC10_PRICE = 1400,
	SCOUT_PRICE = 2750,
	SG552_PRICE = 3500,
	TMP_PRICE = 1250,
	USP_PRICE = 500,
	ELITE_PRICE = 800,
	FIVESEVEN_PRICE = 750,
	XM1014_PRICE = 3000,
	GALIL_PRICE = 2000,
	FAMAS_PRICE = 2250,
	SHIELDGUN_PRICE = 2200
};

enum ItemCostType
{
	ASSAULTSUIT_PRICE = 1000,
	FLASHBANG_PRICE = 200,
	HEGRENADE_PRICE = 300,
	SMOKEGRENADE_PRICE = 300,
	KEVLAR_PRICE = 650,
	HELMET_PRICE = 350,
	NVG_PRICE = 1250,
	DEFUSEKIT_PRICE = 200
};

enum ClipSizeType
{
	P228_MAX_CLIP = 13,
	GLOCK18_MAX_CLIP = 20,
	SCOUT_MAX_CLIP = 10,
	XM1014_MAX_CLIP = 7,
	MAC10_MAX_CLIP = 30,
	AUG_MAX_CLIP = 30,
	ELITE_MAX_CLIP = 30,
	FIVESEVEN_MAX_CLIP = 20,
	UMP45_MAX_CLIP = 25,
	SG550_MAX_CLIP = 30,
	GALIL_MAX_CLIP = 35,
	FAMAS_MAX_CLIP = 25,
	USP_MAX_CLIP = 12,
	AWP_MAX_CLIP = 10,
	MP5N_MAX_CLIP = 30,
	M249_MAX_CLIP = 100,
	M3_MAX_CLIP = 8,
	M4A1_MAX_CLIP = 30,
	TMP_MAX_CLIP = 30,
	G3SG1_MAX_CLIP = 20,
	DEAGLE_MAX_CLIP = 7,
	SG552_MAX_CLIP = 30,
	AK47_MAX_CLIP = 30,
	P90_MAX_CLIP = 50
};

enum MaxAmmoType
{
	MAX_AMMO_BUCKSHOT = 32,
	MAX_AMMO_9MM = 120,
	MAX_AMMO_556NATO = 90,
	MAX_AMMO_556NATOBOX = 200,
	MAX_AMMO_762NATO = 90,
	MAX_AMMO_45ACP = 100,
	MAX_AMMO_50AE = 35,
	MAX_AMMO_338MAGNUM = 30,
	MAX_AMMO_57MM = 100,
	MAX_AMMO_357SIG = 52
};

enum AmmoType
{
	AMMO_BUCKSHOT,
	AMMO_9MM,
	AMMO_556NATO,
	AMMO_556NATOBOX,
	AMMO_762NATO,
	AMMO_45ACP,
	AMMO_50AE,
	AMMO_338MAGNUM,
	AMMO_57MM,
	AMMO_357SIG,
	AMMO_MAX_TYPES
};

typedef struct
{
	int id;
	int cost;
	int clipCost;
	int buyClipSize;
	int gunClipSize;
	int maxRounds;
	int ammoType;
	char *entityName;
}
WeaponInfoStruct;

enum WeaponClassType
{
	WEAPONCLASS_NONE,
	WEAPONCLASS_KNIFE,
	WEAPONCLASS_PISTOL,
	WEAPONCLASS_GRENADE,
	WEAPONCLASS_SUBMACHINEGUN,
	WEAPONCLASS_SHOTGUN,
	WEAPONCLASS_MACHINEGUN,
	WEAPONCLASS_RIFLE,
	WEAPONCLASS_SNIPERRIFLE,
	WEAPONCLASS_MAX
};

enum AutoBuyClassType
{
	AUTOBUYCLASS_PRIMARY = 1,
	AUTOBUYCLASS_SECONDARY = 2,
	AUTOBUYCLASS_AMMO = 4,
	AUTOBUYCLASS_ARMOR = 8,
	AUTOBUYCLASS_DEFUSER = 16,
	AUTOBUYCLASS_PISTOL = 32,
	AUTOBUYCLASS_SMG = 64,
	AUTOBUYCLASS_RIFLE = 128,
	AUTOBUYCLASS_SNIPERRIFLE = 256,
	AUTOBUYCLASS_SHOTGUN = 512,
	AUTOBUYCLASS_MACHINEGUN = 1024,
	AUTOBUYCLASS_GRENADE = 2048,
	AUTOBUYCLASS_NIGHTVISION = 4096,
	AUTOBUYCLASS_SHIELD = 8192
};

typedef struct
{
	int m_class;
	char *m_command;
	char *m_classname;
}
AutoBuyInfoStruct;

typedef struct
{
	char *alias;
	int id;
}
WeaponAliasInfo;

typedef struct
{
	char *alias;
	int id;
	char *failName;
}
WeaponBuyAliasInfo;

typedef struct
{
	char *alias;
	int id;
}
WeaponClassAliasInfo;

enum AmmoBuyAmount
{
	AMMO_338MAG_BUY = 10,
	AMMO_357SIG_BUY = 13,
	AMMO_45ACP_BUY = 12,
	AMMO_50AE_BUY = 7,
	AMMO_556NATO_BUY = 30,
	AMMO_556NATOBOX_BUY = 30,
	AMMO_57MM_BUY = 50,
	AMMO_762NATO_BUY = 30,
	AMMO_9MM_BUY = 30,
	AMMO_BUCKSHOT_BUY = 8
};

enum shieldgun_e
{
	SHIELDGUN_IDLE,
	SHIELDGUN_SHOOT1,
	SHIELDGUN_SHOOT2,
	SHIELDGUN_SHOOT_EMPTY,
	SHIELDGUN_RELOAD,
	SHIELDGUN_DRAW,
	SHIELDGUN_DRAWN_IDLE,
	SHIELDGUN_UP,
	SHIELDGUN_DOWN
};

enum shieldren_e
{
	SHIELDREN_IDLE = 4,
	SHIELDREN_UP,
	SHIELDREN_DOWN
};

typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM,
	BULLET_PLAYER_MP5,
	BULLET_PLAYER_357,
	BULLET_PLAYER_BUCKSHOT,
	BULLET_PLAYER_CROWBAR,

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,

	BULLET_PLAYER_45ACP,
	BULLET_PLAYER_338MAG,
	BULLET_PLAYER_762MM,
	BULLET_PLAYER_556MM,
	BULLET_PLAYER_50AE,
	BULLET_PLAYER_57MM,
	BULLET_PLAYER_357SIG
}
Bullet;

#define ITEM_FLAG_SELECTONEMPTY 1
#define ITEM_FLAG_NOAUTORELOAD 2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY 4
#define ITEM_FLAG_LIMITINWORLD 8
#define ITEM_FLAG_EXHAUSTIBLE 16

#define WPNSLOT_PRIMARY 1
#define WPNSLOT_SECONDARY 2
#define WPNSLOT_KNIFE 3
#define WPNSLOT_GRENADE 4
#define WPNSLOT_C4 5

#define WPNSTATE_USP_SILENCED (1<<0)
#define WPNSTATE_GLOCK18_BURST_MODE (1<<1)
#define WPNSTATE_M4A1_SILENCED (1<<2)
#define WPNSTATE_ELITE_LEFT (1<<3)
#define WPNSTATE_FAMAS_BURST_MODE (1<<4)
#define WPNSTATE_SHIELD_DRAWN (1<<5)

#define WEAPON_IS_ONTARGET 0x40

typedef struct
{
	int iSlot;
	int iPosition;
	const char *pszAmmo1;
	int iMaxAmmo1;
	const char *pszAmmo2;
	int iMaxAmmo2;
	const char *pszName;
	int iMaxClip;
	int iId;
	int iFlags;
	int iWeight;
}
ItemInfo;

typedef struct
{
	const char *pszName;
	int iId;
}
AmmoInfo;

class CBasePlayerItem : public CBaseAnimating
{
public:
	virtual int Save(CSave &save) { return 1; }
	virtual int Restore(CRestore &restore) { return 1; }
	virtual void SetObjectCollisionBox(void) { }
	virtual int AddToPlayer(CBasePlayer *pPlayer) { return false; }
	virtual int AddDuplicate(CBasePlayerItem *pItem) { return FALSE; }
	virtual int GetItemInfo(ItemInfo *p) { return 0; }
	virtual BOOL CanDeploy(void) { return TRUE; }
	virtual BOOL CanDrop(void) { return TRUE; }
	virtual BOOL Deploy(void) { return TRUE; }
	virtual BOOL IsWeapon(void) { return FALSE; }
	virtual BOOL CanHolster(void) { return TRUE; }
	virtual void Holster(int skiplocal = 0) {}
	virtual void UpdateItemInfo(void) {}
	virtual void ItemPreFrame(void) {}
	virtual void ItemPostFrame(void) {}
	virtual void Drop(void) {}
	virtual void Kill(void) {}
	virtual void AttachToPlayer(CBasePlayer *pPlayer) {}
	virtual int PrimaryAmmoIndex(void) { return -1; }
	virtual int SecondaryAmmoIndex(void) { return -1; }
	virtual int UpdateClientData(CBasePlayer *pPlayer) { return 0; }
	virtual CBasePlayerItem *GetWeaponPtr(void) { return NULL; }
	virtual float GetMaxSpeed(void) { return 260; }
	virtual int iItemSlot(void) { return 0; }

public:
	void EXPORT DestroyItem(void) {}
	void EXPORT DefaultTouch(CBaseEntity *pOther) {}
	void EXPORT FallThink(void) {}
	void EXPORT Materialize(void) {}
	void EXPORT AttemptToMaterialize(void) {}
	CBaseEntity *Respawn(void) { return this; }
	void FallInit(void) { }
	void CheckRespawn(void) {}

public:
	static TYPEDESCRIPTION m_SaveData[];
	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];

public:
	CBasePlayer *m_pPlayer;
	CBasePlayerItem *m_pNext;
	int m_iId;

public:
	int iItemPosition(void) { return ItemInfoArray[m_iId].iPosition; }
	const char *pszAmmo1(void) { return ItemInfoArray[m_iId].pszAmmo1; }
	int iMaxAmmo1(void) { return ItemInfoArray[m_iId].iMaxAmmo1; }
	const char *pszAmmo2(void) { return ItemInfoArray[m_iId].pszAmmo2; }
	int iMaxAmmo2(void) { return ItemInfoArray[m_iId].iMaxAmmo2; }
	const char *pszName(void) { return ItemInfoArray[m_iId].pszName; }
	int iMaxClip(void) { return ItemInfoArray[m_iId].iMaxClip; }
	int iWeight(void) { return ItemInfoArray[m_iId].iWeight; }
	int iFlags(void) { return ItemInfoArray[m_iId].iFlags; }
};

class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	virtual int Save(CSave &save) { return 1; }
	virtual int Restore(CRestore &restore) { return 1; }
	virtual int AddToPlayer(CBasePlayer *pPlayer) { return 0; }
	virtual int AddDuplicate(CBasePlayerItem *pItem) { return 0; }
	virtual int ExtractAmmo(CBasePlayerWeapon *pWeapon) { return 0; }
	virtual int ExtractClipAmmo(CBasePlayerWeapon *pWeapon) { return 0; }
	virtual int AddWeapon(void) { ExtractAmmo(this); return TRUE; }
	virtual void UpdateItemInfo(void) {}
	virtual BOOL PlayEmptySound(void);
	virtual void ResetEmptySound(void);
	virtual void SendWeaponAnim(int iAnim, int skiplocal = 0);
	virtual BOOL CanDeploy(void);
	virtual BOOL IsWeapon(void) { return TRUE; }
	virtual BOOL IsUseable(void) { return true; }
	virtual void ItemPostFrame(void);
	virtual void PrimaryAttack(void) {}
	virtual void SecondaryAttack(void) {}
	virtual void Reload(void) {}
	virtual void WeaponIdle(void) {}
	virtual int UpdateClientData(CBasePlayer *pPlayer) { return 0; }
	virtual void RetireWeapon(void);
	virtual BOOL ShouldWeaponIdle(void) { return FALSE; }
	virtual void Holster(int skiplocal = 0);
	virtual BOOL UseDecrement(void) { return FALSE; }
	virtual CBasePlayerItem *GetWeaponPtr(void) { return (CBasePlayerItem *)this; }

public:
	BOOL DefaultDeploy(const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int skiplocal = 0);
	int DefaultReload(int iClipSize, int iAnim, float fDelay, int body = 0);
	void ReloadSound(void);
	BOOL AddPrimaryAmmo(int iCount, char *szName, int iMaxClip, int iMaxCarry) { return true; }
	BOOL AddSecondaryAmmo(int iCount, char *szName, int iMaxCarry) { return true; }
	int PrimaryAmmoIndex(void) { return -1; }
	int SecondaryAmmoIndex(void) { return -1; }
	void EjectBrassLate(void);
	void KickBack(float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change);
	void FireRemaining(int &shotsFired, float &shootTime, BOOL isGlock18);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);
	bool HasSecondaryAttack(void);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	int m_iPlayEmptySound;
	int m_fFireOnEmpty;
	float m_flNextPrimaryAttack;
	float m_flNextSecondaryAttack;
	float m_flTimeWeaponIdle;
	int m_iPrimaryAmmoType;
	int m_iSecondaryAmmoType;
	int m_iClip;
	int m_iClientClip;
	int m_iClientWeaponState;
	int m_fInReload;
	int m_fInSpecialReload;
	int m_iDefaultAmmo;
	int m_iShellId;
	int m_fMaxSpeed;
	bool m_bDelayFire;
	int m_iDirection;
	bool m_bSecondarySilencerOn;
	float m_flAccuracy;
	float m_flLastFire;
	int m_iShotsFired;
	Vector m_vVecAiming;
	string_t model_name;
	float m_flGlock18Shoot;
	int m_iGlock18ShotsFired;
	float m_flFamasShoot;
	int m_iFamasShotsFired;
	float m_fBurstSpread;
	int m_iWeaponState;
	float m_flNextReload;
	float m_flDecreaseShotsFired;
	unsigned short m_usFireGlock18;
	unsigned short m_usFireFamas;
};

class CBasePlayerAmmo : public CBaseEntity
{
public:
	virtual void Spawn(void){}
	virtual BOOL AddAmmo(CBaseEntity *pOther) { return TRUE; }

public:
	void EXPORT Materialize(void) { }
	void EXPORT DefaultTouch(CBaseEntity *pOther) { }
	CBaseEntity *Respawn(void) { return this; }
};

extern DLL_GLOBAL short g_sModelIndexLaser;
extern DLL_GLOBAL const char *g_pModelNameLaser;
extern DLL_GLOBAL short g_sModelIndexLaserDot;
extern DLL_GLOBAL short g_sModelIndexFireball;
extern DLL_GLOBAL short g_sModelIndexFireball2;
extern DLL_GLOBAL short g_sModelIndexFireball3;
extern DLL_GLOBAL short g_sModelIndexFireball4;
extern DLL_GLOBAL short g_sModelIndexSmoke;
extern DLL_GLOBAL short g_sModelIndexSmokePuff;
extern DLL_GLOBAL short g_sModelIndexWExplosion;
extern DLL_GLOBAL short g_sModelIndexBubbles;
extern DLL_GLOBAL short g_sModelIndexBloodDrop;
extern DLL_GLOBAL short g_sModelIndexBloodSpray;
extern DLL_GLOBAL short g_sModelIndexRadio;
extern DLL_GLOBAL short g_sModelIndexCTGhost;
extern DLL_GLOBAL short g_sModelIndexTGhost;
extern DLL_GLOBAL short g_sModelIndexC4Glow;

#ifndef CLIENT_WEAPONS
extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker);
extern void AddMultiDamage(entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);
extern void DecalGunshot(TraceResult *pTrace, int iBulletType, bool ClientOnly, entvars_t *pShooter, bool bHitMetal);
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern int DamageDecal(CBaseEntity *pEntity, int bitsDamageType);
extern void RadiusFlash(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage);
extern void RadiusDamage(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType);
#else
inline void ClearMultiDamage(void) { }
inline void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker) { }
inline void DecalGunshot(TraceResult *pTrace, int iBulletType, bool ClientOnly, entvars_t *pShooter, bool bHitMetal) { }
#endif
typedef struct
{
	CBaseEntity *pEntity;
	float amount;
	int type;
}
MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

#define LOUD_GUN_VOLUME 1000
#define NORMAL_GUN_VOLUME 600
#define QUIET_GUN_VOLUME 200

#define BRIGHT_GUN_FLASH 512
#define NORMAL_GUN_FLASH 256
#define DIM_GUN_FLASH 128

#define BIG_EXPLOSION_VOLUME 2048
#define NORMAL_EXPLOSION_VOLUME 1024
#define SMALL_EXPLOSION_VOLUME 512

#define WEAPON_ACTIVITY_VOLUME 64

#define VECTOR_CONE_1DEGREES Vector(0.00873, 0.00873, 0.00873)
#define VECTOR_CONE_2DEGREES Vector(0.01745, 0.01745, 0.01745)
#define VECTOR_CONE_3DEGREES Vector(0.02618, 0.02618, 0.02618)
#define VECTOR_CONE_4DEGREES Vector(0.03490, 0.03490, 0.03490)
#define VECTOR_CONE_5DEGREES Vector(0.04362, 0.04362, 0.04362)
#define VECTOR_CONE_6DEGREES Vector(0.05234, 0.05234, 0.05234)
#define VECTOR_CONE_7DEGREES Vector(0.06105, 0.06105, 0.06105)
#define VECTOR_CONE_8DEGREES Vector(0.06976, 0.06976, 0.06976)
#define VECTOR_CONE_9DEGREES Vector(0.07846, 0.07846, 0.07846)
#define VECTOR_CONE_10DEGREES Vector(0.08716, 0.08716, 0.08716)
#define VECTOR_CONE_15DEGREES Vector(0.13053, 0.13053, 0.13053)
#define VECTOR_CONE_20DEGREES Vector(0.17365, 0.17365, 0.17365)

class CWeaponBox : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	void Touch(CBaseEntity *pOther);
	void SetObjectCollisionBox(void);

public:
	BOOL IsEmpty(void);
	int GiveAmmo(int iCount, char *szName, int iMax, int *pIndex = NULL);
	void EXPORT Kill(void);
	BOOL HasWeapon(CBasePlayerItem *pCheckItem);
	BOOL PackWeapon(CBasePlayerItem *pWeapon);
	BOOL PackAmmo(int iszName, int iCount);

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	CBasePlayerItem *m_rgpPlayerItems[MAX_ITEM_TYPES];
	int m_rgiszAmmo[MAX_AMMO_SLOTS];
	int m_rgAmmo[MAX_AMMO_SLOTS];
	int m_cAmmoTypes;
};

class CWShield : public CBaseEntity
{
public:
	void Spawn(void);
	void EXPORT Touch(CBaseEntity *pOther);
	void SetCantBePickedUpByUser(CBaseEntity *pEntity, float time);

public:
	EHANDLE m_hEntToIgnoreTouchesFrom;
	float m_flTimeToIgnoreTouches;
};

#ifdef PLAYER_H

class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 221; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void AK47Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAK47;
};

class CAUG : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	void AUGFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAug;
};

class CAWP : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void AWPFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireAWP;
};

class CC4 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_C4; }
	void PrimaryAttack(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	bool m_bStartedArming;
	bool m_bBombPlacedAnimation;
	float m_fArmedTime;
	bool m_bHasShield;
};

class CDEAGLE : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void DEAGLEFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireDeagle;
};

class CELITE : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void ELITEFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireELITE_LEFT;
	unsigned short m_usFireELITE_RIGHT;
};

class CFamas : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void FamasFire(float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL bFireBurst);

private:
	int m_iShell;
	int iShellOn;
};

class CFiveSeven : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void FiveSevenFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireFiveSeven;
};

class CFlashbang : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDeploy(void);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_GRENADE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
};

class CG3SG1 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void G3SG1Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireG3SG1;
};

class CGalil : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void GalilFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireGalil;
};

class CGLOCK18 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void GLOCK18Fire(float flSpread, float flCycleTime, BOOL fUseBurstMode);

private:
	int m_iShell;
	bool m_bBurstFire;
};

class CHEGrenade : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDeploy(void);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	BOOL CanHolster(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_GRENADE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	unsigned short m_usCreateExplosion;
};

class CKnife : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_KNIFE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	void WeaponAnimation(int iAnimation);
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);
	int Swing(int fFirst);
	int Stab(int fFirst);

private:
	//int m_iSwing; //Already exists in CBaseDelay
	TraceResult m_trHit;
	unsigned short m_usKnife;
};

class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 220; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void M249Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM249;
};

class CM3 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 230; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireM3;
};

class CM4A1 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 230; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void M4A1Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM4A1;
};

class CMAC10 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void MAC10Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMAC10;
};

class CMP5N : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void MP5NFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMP5N;
};

class CP228 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void P228Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireP228;
};

class CP90 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 245; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void P90Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireP90;
};

class CSCOUT : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void SCOUTFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireScout;
};

class CSG550 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void SG550Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireSG550;
};

class CSG552 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void);
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	void SG552Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireSG552;
};

class CSmokeGrenade : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL CanDeploy(void);
	BOOL CanDrop(void) { return FALSE; }
	BOOL Deploy(void);
	void Holster(int skiplocal);
	float GetMaxSpeed(void) { return m_fMaxSpeed; }
	int iItemSlot(void) { return WPNSLOT_GRENADE; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void WeaponIdle(void);
	void SetPlayerShieldAnim(void);
	void ResetPlayerShieldAnim(void);
	bool ShieldSecondaryFire(int up_anim, int down_anim);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

public:
	unsigned short m_usCreateSmoke;
};

class CTMP : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void TMPFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireTMP;
};

class CUMP45 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void UMP45Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireUMP45;
};

class CUSP : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 250; }
	int iItemSlot(void) { return WPNSLOT_SECONDARY; }
	void PrimaryAttack(void);
	void SecondaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	void USPFire(float flSpread, float flCycleTime, BOOL fUseAutoAim);

private:
	int m_iShell;
	unsigned short m_usFireUSP;
};

class CXM1014 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy(void);
	float GetMaxSpeed(void) { return 240; }
	int iItemSlot(void) { return WPNSLOT_PRIMARY; }
	void PrimaryAttack(void);
	void Reload(void);
	void WeaponIdle(void);

	BOOL UseDecrement(void)
	{
#ifdef CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireXM1014;
};

#endif

#define ARMOURY_MP5NAVY 0
#define ARMOURY_TMP 1
#define ARMOURY_P90 2
#define ARMOURY_MAC10 3
#define ARMOURY_AK47 4
#define ARMOURY_SG552 5
#define ARMOURY_M4A1 6
#define ARMOURY_AUG 7
#define ARMOURY_SCOUT 8
#define ARMOURY_G3SG1 9
#define ARMOURY_AWP 10
#define ARMOURY_M3 11
#define ARMOURY_XM1014 12
#define ARMOURY_M249 13
#define ARMOURY_FLASHBANG 14
#define ARMOURY_HEGRENADE 15
#define ARMOURY_KEVLAR 16
#define ARMOURY_ASSAULT 17
#define ARMOURY_SMOKEGRENADE 18

class CArmoury : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);
	void Restart(void);
	void KeyValue(KeyValueData *pkvd);

public:
	void EXPORT ArmouryTouch(CBaseEntity *pOther);

public:
	int m_iItem;
	int m_iCount;
	int m_iInitialCount;
	bool m_bAlreadyCounted;
};

#endif

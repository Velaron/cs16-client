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
#include "player.h"
#include "weapons.h"

#ifdef CLIENT_WEAPONS
extern bool g_bGlockBurstMode;
#endif

enum glock18_e
{
	GLOCK18_IDLE1,
	GLOCK18_IDLE2,
	GLOCK18_IDLE3,
	GLOCK18_SHOOT,
	GLOCK18_SHOOT2,
	GLOCK18_SHOOT3,
	GLOCK18_SHOOT_EMPTY,
	GLOCK18_RELOAD,
	GLOCK18_DRAW,
	GLOCK18_HOLSTER,
	GLOCK18_ADD_SILENCER,
	GLOCK18_DRAW2,
	GLOCK18_RELOAD2
};

enum glock18_shield_e
{
	GLOCK18_SHIELD_IDLE1,
	GLOCK18_SHIELD_SHOOT,
	GLOCK18_SHIELD_SHOOT2,
	GLOCK18_SHIELD_SHOOT_EMPTY,
	GLOCK18_SHIELD_RELOAD,
	GLOCK18_SHIELD_DRAW,
	GLOCK18_SHIELD_IDLE,
	GLOCK18_SHIELD_UP,
	GLOCK18_SHIELD_DOWN
};

LINK_ENTITY_TO_CLASS(weapon_glock18, CGLOCK18)

void CGLOCK18::Spawn(void)
{
	pev->classname = MAKE_STRING("weapon_glock18");

	Precache();
	m_iId = WEAPON_GLOCK18;
	SET_MODEL(ENT(pev), "models/w_glock18.mdl");

	m_bBurstFire = 0;
	m_iGlock18ShotsFired = 0;
	m_flGlock18Shoot = 0;
	m_iDefaultAmmo = GLOCK18_DEFAULT_GIVE;
	m_flAccuracy = 0.9;
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;

	FallInit();
}

void CGLOCK18::Precache(void)
{
	PRECACHE_MODEL("models/v_glock18.mdl");
	PRECACHE_MODEL("models/w_glock18.mdl");
	PRECACHE_MODEL("models/shield/v_shield_glock18.mdl");

	PRECACHE_SOUND("weapons/glock18-1.wav");
	PRECACHE_SOUND("weapons/glock18-2.wav");
	PRECACHE_SOUND("weapons/clipout1.wav");
	PRECACHE_SOUND("weapons/clipin1.wav");
	PRECACHE_SOUND("weapons/sliderelease1.wav");
	PRECACHE_SOUND("weapons/slideback1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("weapons/de_clipin.wav");
	PRECACHE_SOUND("weapons/de_clipout.wav");

	m_iShellId = m_iShell = PRECACHE_MODEL("models/pshell.mdl");
	m_usFireGlock18 = PRECACHE_EVENT(1, "events/glock18.sc");
}

int CGLOCK18::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK18_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_GLOCK18;
	p->iFlags = 0;
	p->iWeight = GLOCK18_WEIGHT;

	return 1;
}

BOOL CGLOCK18::Deploy(void)
{
	m_bBurstFire = 0;
	m_iGlock18ShotsFired = 0;
	m_flGlock18Shoot = 0;
	m_flAccuracy = 0.9;
	m_fMaxSpeed = 250;
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
	m_pPlayer->m_bShieldDrawn = false;

	if (m_pPlayer->HasShield() != false)
		return DefaultDeploy("models/shield/v_shield_glock18.mdl", "models/shield/p_shield_glock18.mdl", GLOCK18_SHIELD_DRAW, "shieldgun", UseDecrement() != FALSE);
	else
		return DefaultDeploy("models/v_glock18.mdl", "models/p_glock18.mdl", RANDOM_LONG(0, 1) ? GLOCK18_DRAW : GLOCK18_DRAW2, "onehanded", UseDecrement() != FALSE);
}

void CGLOCK18::SecondaryAttack(void)
{
	if (ShieldSecondaryFire(GLOCK18_SHIELD_UP, GLOCK18_SHIELD_DOWN) == true)
		return;
	//return;

	if (m_iWeaponState & WPNSTATE_GLOCK18_BURST_MODE)
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_SemiAuto");
		m_iWeaponState &= ~WPNSTATE_GLOCK18_BURST_MODE;
#ifdef CLIENT_WEAPONS
		g_bGlockBurstMode = false;
#endif
	}
	else
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_BurstFire");
		m_iWeaponState |= WPNSTATE_GLOCK18_BURST_MODE;
#ifdef CLIENT_WEAPONS
		g_bGlockBurstMode = true;
#endif
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CGLOCK18::PrimaryAttack(void)
{
	if (m_iWeaponState & WPNSTATE_GLOCK18_BURST_MODE)
	{
		if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
			GLOCK18Fire((1.2) * (1 - m_flAccuracy), 0.5, TRUE);
		else if (m_pPlayer->pev->velocity.Length2D() > 0)
			GLOCK18Fire((0.185) * (1 - m_flAccuracy), 0.5, TRUE);
		else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
			GLOCK18Fire((0.095) * (1 - m_flAccuracy), 0.5, TRUE);
		else
			GLOCK18Fire((0.3) * (1 - m_flAccuracy), 0.5, TRUE);
	}
	else
	{
		if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
			GLOCK18Fire((1.0) * (1 - m_flAccuracy), 0.2, FALSE);
		else if (m_pPlayer->pev->velocity.Length2D() > 0)
			GLOCK18Fire((0.165) * (1 - m_flAccuracy), 0.2, FALSE);
		else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
			GLOCK18Fire((0.075) * (1 - m_flAccuracy), 0.2, FALSE);
		else
			GLOCK18Fire((0.1) * (1 - m_flAccuracy), 0.2, FALSE);
	}
}

void CGLOCK18::GLOCK18Fire(float flSpread, float flCycleTime, BOOL fUseBurstMode)
{
	if (fUseBurstMode != FALSE)
	{
		m_iGlock18ShotsFired = 0;
	}
	else
	{
		m_iShotsFired++;
		flCycleTime -= 0.05;

		if (m_iShotsFired > 1)
			return;
	}

	if (m_flLastFire)
	{
		m_flAccuracy -= (0.325 - (gpGlobals->time - m_flLastFire)) * 0.275;

		if (m_flAccuracy > 0.9)
			m_flAccuracy = 0.9;
		else if (m_flAccuracy < 0.6)
			m_flAccuracy = 0.6;
	}

	m_flLastFire = gpGlobals->time;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	SetPlayerShieldAnim();

#ifndef CLIENT_DLL
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBullets3(vecSrc, gpGlobals->v_forward, flSpread, 8192, 1, BULLET_PLAYER_9MM, 25, 0.75, m_pPlayer->pev, TRUE, m_pPlayer->random_seed);

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireGlock18, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 100), (int)(m_pPlayer->pev->punchangle.y * 100), m_iClip != 0, FALSE);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

#ifndef CLIENT_DLL
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;

	if (fUseBurstMode != FALSE)
	{
		m_iGlock18ShotsFired++;
		m_flGlock18Shoot = gpGlobals->time + 0.1;
	}

	ResetPlayerShieldAnim();
}

void CGLOCK18::Reload(void)
{
	if (m_pPlayer->ammo_9mm <= 0)
		return;

	int iAnim;

	if (m_pPlayer->HasShield() != false)
		iAnim = GLOCK18_SHIELD_RELOAD;
	else if (RANDOM_LONG(0, 1))
		iAnim = GLOCK18_RELOAD;
	else
		iAnim = GLOCK18_RELOAD2;

	if (DefaultReload(GLOCK18_MAX_CLIP, iAnim, 2.2))
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD);
#endif
		m_flAccuracy = 0.9;
	}
}

void CGLOCK18::WeaponIdle(void)
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_pPlayer->HasShield() != false)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

		if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
			SendWeaponAnim(GLOCK18_SHIELD_IDLE, UseDecrement() != FALSE);

		return;
	}

	if (m_iClip)
	{
		float flRand = RANDOM_FLOAT(0, 1);

		if (flRand < 0.3)
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0625;
			SendWeaponAnim(GLOCK18_IDLE3, UseDecrement() != FALSE);
		}
		else if (flRand < 0.6)
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.75;
			SendWeaponAnim(GLOCK18_IDLE1, UseDecrement() != FALSE);
		}
		else
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
			SendWeaponAnim(GLOCK18_IDLE2, UseDecrement() != FALSE);
		}
	}
}

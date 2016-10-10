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

enum famas_e
{
	FAMAS_IDLE1,
	FAMAS_RELOAD,
	FAMAS_DRAW,
	FAMAS_SHOOT1,
	FAMAS_SHOOT2,
	FAMAS_SHOOT3
};

LINK_ENTITY_TO_CLASS(weapon_famas, CFamas)

void CFamas::Spawn(void)
{
	pev->classname = MAKE_STRING("weapon_famas");

	Precache();
	m_iId = WEAPON_FAMAS;
	SET_MODEL(ENT(pev), "models/w_famas.mdl");

	m_iDefaultAmmo = FAMAS_DEFAULT_GIVE;
	m_iFamasShotsFired = 0;
	m_flFamasShoot = 0;

	FallInit();
}

void CFamas::Precache(void)
{
	PRECACHE_MODEL("models/v_famas.mdl");
	PRECACHE_MODEL("models/w_famas.mdl");

	PRECACHE_SOUND("weapons/famas-1.wav");
	PRECACHE_SOUND("weapons/famas-2.wav");
	PRECACHE_SOUND("weapons/famas_clipout.wav");
	PRECACHE_SOUND("weapons/famas_clipin.wav");
	PRECACHE_SOUND("weapons/famas_boltpull.wav");
	PRECACHE_SOUND("weapons/famas_boltslap.wav");
	PRECACHE_SOUND("weapons/famas_forearm.wav");
	PRECACHE_SOUND("weapons/famas-burst.wav");

	m_iShell = PRECACHE_MODEL("models/rshell.mdl");
	m_usFireFamas = PRECACHE_EVENT(1, "events/famas.sc");
}

int CFamas::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = _556NATO_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = FAMAS_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 18;
	p->iId = m_iId = WEAPON_FAMAS;
	p->iFlags = 0;
	p->iWeight = FAMAS_WEIGHT;

	return 1;
}

BOOL CFamas::Deploy(void)
{
	m_iShotsFired = 0;
	m_iFamasShotsFired = 0;
	m_flFamasShoot = 0;
	m_flAccuracy = 0.2;
	iShellOn = 1;

	return DefaultDeploy("models/v_famas.mdl", "models/p_famas.mdl", FAMAS_DRAW, "carbine", UseDecrement() != FALSE);
}

void CFamas::SecondaryAttack(void)
{
	if (m_iWeaponState & WPNSTATE_FAMAS_BURST_MODE)
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_FullAuto");
		m_iWeaponState &= ~WPNSTATE_FAMAS_BURST_MODE;
	}
	else
	{
		ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_BurstFire");
		m_iWeaponState |= WPNSTATE_FAMAS_BURST_MODE;
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CFamas::PrimaryAttack(void)
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	BOOL bBurstMode = FBitSet(m_iWeaponState, WPNSTATE_FAMAS_BURST_MODE) ? TRUE : FALSE;

	if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		FamasFire(0.030 + (0.3) * m_flAccuracy, 0.0825, FALSE, bBurstMode);
	else if (m_pPlayer->pev->velocity.Length2D() > 140)
		FamasFire(0.030 + (0.07) * m_flAccuracy, 0.0825, FALSE, bBurstMode);
	else
		FamasFire((0.02) * m_flAccuracy, 0.0825, FALSE, bBurstMode);
}

void CFamas::FamasFire(float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL bFireBurst)
{
	if (bFireBurst != FALSE)
	{
		m_iFamasShotsFired = 0;
		flCycleTime = 0.55;
	}
	else
		flSpread += 0.01;

	m_bDelayFire = true;
	m_iShotsFired++;
	m_flAccuracy = ((float)(m_iShotsFired * m_iShotsFired * m_iShotsFired) / 215.0) + 0.3;

	if (m_flAccuracy > 1)
		m_flAccuracy = 1;

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
#ifndef CLIENT_DLL
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBullets3(vecSrc, gpGlobals->v_forward, flSpread, 8192, 2, BULLET_PLAYER_556MM, bFireBurst ? 34 : 30, 0.96, m_pPlayer->pev, FALSE, m_pPlayer->random_seed);

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireFamas, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000000), (int)(m_pPlayer->pev->punchangle.y * 10000000), m_iClip != 0, FALSE);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

#ifndef CLIENT_DLL
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
		KickBack(1, 0.45, 0.275, 0.05, 4, 2.5, 7);
	else if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		KickBack(1.25, 0.45, 0.22, 0.18, 5.5, 4, 5);
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		KickBack(0.575, 0.325, 0.2, 0.011, 3.25, 2, 8);
	else
		KickBack(0.625, 0.375, 0.25, 0.0125, 3.5, 2.25, 8);

	if (bFireBurst != FALSE)
	{
		m_fBurstSpread = flSpread;
		m_iFamasShotsFired++;
		m_flFamasShoot = gpGlobals->time + 0.05;
	}
}

void CFamas::Reload(void)
{
	if (m_pPlayer->ammo_556nato <= 0)
		return;

	if (DefaultReload(FAMAS_MAX_CLIP, FAMAS_RELOAD, 3.3))
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD);
#endif
		if (m_pPlayer->m_iFOV != 90)
			SecondaryAttack();

		m_flAccuracy = 0;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CFamas::WeaponIdle(void)
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
	SendWeaponAnim(FAMAS_IDLE1, UseDecrement() != FALSE);
}

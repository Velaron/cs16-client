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

class C9MMAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_9mm, C9MMAmmo);

void C9MMAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C9MMAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C9MMAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_9MM_GIVE, "9mm", _9MM_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class CBuckShotAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_buckshot, CBuckShotAmmo);

void CBuckShotAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_shotbox.mdl");
	CBasePlayerAmmo::Spawn();
}

void CBuckShotAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_shotbox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL CBuckShotAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_BUCKSHOT_GIVE, "buckshot", BUCKSHOT_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C556NatoAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_556nato, C556NatoAmmo);

void C556NatoAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C556NatoAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C556NatoAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_556NATO_GIVE, "556Nato", _556NATO_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C556NatoBox : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_556natobox, C556NatoBox);

void C556NatoBox::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C556NatoBox::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C556NatoBox::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_556NATOBOX_GIVE, "556NatoBox", _556NATOBOX_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C762NatoAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_762nato, C762NatoAmmo);

void C762NatoAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C762NatoAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C762NatoAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_762NATO_GIVE, "762Nato", _762NATO_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C45ACPAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_45acp, C45ACPAmmo);

void C45ACPAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C45ACPAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C45ACPAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_45ACP_GIVE, "45acp", _45ACP_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C50AEAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_50ae, C50AEAmmo);

void C50AEAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C50AEAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C50AEAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_50AE_GIVE, "50AE", _50AE_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C338MagnumAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_338magnum, C338MagnumAmmo);

void C338MagnumAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C338MagnumAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C338MagnumAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_338MAGNUM_GIVE, "338Magnum", _338MAGNUM_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C57MMAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_57mm, C57MMAmmo);

void C57MMAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C57MMAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C57MMAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_57MM_GIVE, "57mm", _57MM_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}

class C357SIGAmmo : public CBasePlayerAmmo
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL AddAmmo(CBaseEntity *pOther);
};

LINK_ENTITY_TO_CLASS(ammo_357sig, C357SIGAmmo);

void C357SIGAmmo::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
	CBasePlayerAmmo::Spawn();
}

void C357SIGAmmo::Precache(void)
{
	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL C357SIGAmmo::AddAmmo(CBaseEntity *pOther)
{
	if (pOther->GiveAmmo(AMMO_357SIG_GIVE, "357SIG", _357SIG_MAX_CARRY) == -1)
		return FALSE;

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	return TRUE;
}
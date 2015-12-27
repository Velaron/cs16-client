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
#ifndef EXPLODE_H
#define EXPLODE_H

#define SF_ENVEXPLOSION_NODAMAGE (1<<0)
#define SF_ENVEXPLOSION_REPEATABLE (1<< 1)
#define SF_ENVEXPLOSION_NOFIREBALL (1<< 2)
#define SF_ENVEXPLOSION_NOSMOKE (1<<3)
#define SF_ENVEXPLOSION_NODECAL (1<<4)
#define SF_ENVEXPLOSION_NOSPARKS (1<<5)

extern DLL_GLOBAL short g_sModelIndexFireball;
extern DLL_GLOBAL short g_sModelIndexSmoke;

extern void ExplosionCreate(const Vector &center, const Vector &angles, edict_t *pOwner, int magnitude, BOOL doDamage);
#endif
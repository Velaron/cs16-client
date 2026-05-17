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

#pragma once
#ifndef PARTICLEMAN_INTERNAL_H
#define PARTICLEMAN_INTERNAL_H

/**
*	@file
*
*	Internal header for particle manager
*/

#include <algorithm>
#include <cstddef>

#include "CFrustum.h"

constexpr std::size_t MaxForceElements = 128;

extern CFrustum g_cFrustum;
extern float g_flGravity;
extern float g_flOldTime;
extern Vector g_vViewAngles;

inline bool IsGamePaused()
{
	return gEngfuncs.GetClientTime() == g_flOldTime;
}

struct ForceMember
{
	Vector m_vOrigin;
	Vector m_vDirection;
	float m_flRadius;
	float m_flStrength;
	float m_flDieTime;
};
#endif

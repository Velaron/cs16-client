//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once
#if !defined( HUD_IFACEH )
#define HUD_IFACEH
#ifdef _WIN32
#define EXPORT		_declspec( dllexport )
#define _DLLEXPORT __declspec( dllexport )
#else
#define EXPORT
#define _DLLEXPORT
#endif
typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);
#include "wrect.h"
#include "../engine/cdll_int.h"
extern cl_enginefunc_t gEngfuncs;

#endif

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

#include <assert.h>
#include "mathlib.h"
#include "const.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_movevars.h"
#include "pm_debug.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef CLIENT_DLL
	int iJumpSpectator;
	float vJumpOrigin[3];
	float vJumpAngles[3];
#endif

static int pm_shared_initialized = 0;

#ifdef _MSC_VER
#pragma warning(disable:4101)
#pragma warning(disable:4305)
#endif

typedef enum
{
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
}
modtype_t;

playermove_t *pmove = NULL;

typedef struct
{
	int planenum;
	short children[2];
}
dclipnode_t;

typedef struct mplane_s
{
	vec3_t normal;
	float dist;
	byte type;
	byte signbits;
	byte pad[2];
}
mplane_t;

typedef struct hull_s
{
	dclipnode_t *clipnodes;
	mplane_t *planes;
	int firstclipnode;
	int lastclipnode;
	vec3_t clip_mins;
	vec3_t clip_maxs;
}
hull_t;

#define TIME_TO_DUCK 0.4
#define VEC_DUCK_HULL_MIN -18
#define VEC_DUCK_HULL_MAX 32
#define VEC_DUCK_VIEW 12
#define PM_DEAD_VIEWHEIGHT -8
#define MAX_CLIMB_SPEED	200
#define STUCK_MOVEUP 1
#define STUCK_MOVEDOWN -1
#define VEC_HULL_MIN -36
#define VEC_HULL_MAX 36

#define VEC_VIEW 17
#define STOP_EPSILON 0.1

#define CTEXTURESMAX 1024

#define CBTEXTURENAMEMAX 17

#define CHAR_TEX_CONCRETE 'C'
#define CHAR_TEX_METAL 'M'
#define CHAR_TEX_DIRT 'D'
#define CHAR_TEX_VENT 'V'
#define CHAR_TEX_GRATE 'G'
#define CHAR_TEX_TILE 'T'
#define CHAR_TEX_SLOSH 'S'
#define CHAR_TEX_WOOD 'W'
#define CHAR_TEX_COMPUTER 'P'
#define CHAR_TEX_GLASS 'Y'
#define CHAR_TEX_FLESH 'F'
#define CHAR_TEX_SNOW 'N'

#define STEP_CONCRETE 0
#define STEP_METAL 1
#define STEP_DIRT 2
#define STEP_VENT 3
#define STEP_GRATE 4
#define STEP_TILE 5
#define STEP_SLOSH 6
#define STEP_WADE 7
#define STEP_LADDER 8
#define STEP_SNOW 9

#define PLAYER_FATAL_FALL_SPEED 1024
#define PLAYER_MAX_SAFE_FALL_SPEED 580
#define DAMAGE_FOR_FALL_SPEED (float)100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED)

#define PLAYER_MIN_BOUNCE_SPEED 350

#define PLAYER_FALL_PUNCH_THRESHHOLD (float)250

#define PLAYER_LONGJUMP_SPEED 350

#pragma warning(disable : 4244)

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define PITCH 0
#define YAW 1
#define ROLL 2 

#define MAX_CLIENTS 32

#define CONTENTS_CURRENT_0 -9
#define CONTENTS_CURRENT_90 -10
#define CONTENTS_CURRENT_180 -11
#define CONTENTS_CURRENT_270 -12
#define CONTENTS_CURRENT_UP -13
#define CONTENTS_CURRENT_DOWN -14

#define CONTENTS_TRANSLUCENT -15

static vec3_t rgv3tStuckTable[54];
static int rgStuckLast[MAX_CLIENTS][2];

static int gcTextures = 0;
static char grgszTextureName[CTEXTURESMAX][CBTEXTURENAMEMAX];
static char grgchTextureType[CTEXTURESMAX];

int g_onladder = 0;

void PM_SwapTextures(int i, int j)
{
	char chTemp;
	char szTemp[CBTEXTURENAMEMAX];

	strcpy(szTemp, grgszTextureName[i]);
	chTemp = grgchTextureType[i];

	strcpy(grgszTextureName[i], grgszTextureName[j]);
	grgchTextureType[i] = grgchTextureType[j];

	strcpy(grgszTextureName[j], szTemp);
	grgchTextureType[j] = chTemp;
}

void PM_SortTextures(void)
{
	int i, j;

	for (i = 0 ; i < gcTextures; i++)
	{
		for (j = i + 1; j < gcTextures; j++)
		{
			if (stricmp(grgszTextureName[i], grgszTextureName[j]) > 0)
				PM_SwapTextures(i, j);
		}
	}
}

void PM_InitTextureTypes(void)
{
	char buffer[512];
	int i, j;
	byte *pMemFile;
	int fileSize, filePos;
	static qboolean bTextureTypeInit = false;

	if (bTextureTypeInit)
		return;

	memset(&(grgszTextureName[0][0]), 0, CTEXTURESMAX * CBTEXTURENAMEMAX);
	memset(grgchTextureType, 0, CTEXTURESMAX);

	gcTextures = 0;
	memset(buffer, 0, 512);

	fileSize = pmove->COM_FileSize("sound/materials.txt");
	pMemFile = pmove->COM_LoadFile("sound/materials.txt", 5, NULL);

	if (!pMemFile)
		return;

	filePos = 0;

	while (pmove->memfgets(pMemFile, fileSize, &filePos, buffer, 511) != NULL && (gcTextures < CTEXTURESMAX))
	{
		i = 0;

		while (buffer[i] && isspace(buffer[i]))
			i++;

		if (!buffer[i])
			continue;

		if (buffer[i] == '/' || !isalpha(buffer[i]))
			continue;

		grgchTextureType[gcTextures] = toupper(buffer[i++]);

		while (buffer[i] && isspace(buffer[i]))
			i++;

		if (!buffer[i])
			continue;

		j = i;

		while (buffer[j] && !isspace(buffer[j]))
			j++;

		if (!buffer[j])
			continue;

		j = min (j, CBTEXTURENAMEMAX - 1 + i);
		buffer[j] = 0;
		strcpy(&(grgszTextureName[gcTextures++][0]), &(buffer[i]));
	}

	pmove->COM_FreeFile(pMemFile);

	PM_SortTextures();

	bTextureTypeInit = true;
}

char PM_FindTextureType(char *name)
{
	int left, right, pivot;
	int val;

	assert(pm_shared_initialized);

	left = 0;
	right = gcTextures - 1;

	while (left <= right)
	{
		pivot = (left + right) / 2;

		val = strnicmp(name, grgszTextureName[pivot], CBTEXTURENAMEMAX - 1);

		if (val == 0)
		{
			return grgchTextureType[pivot];
		}
		else if (val > 0)
		{
			left = pivot + 1;
		}
		else if (val < 0)
		{
			right = pivot - 1;
		}
	}

	return CHAR_TEX_CONCRETE;
}

void PM_PlayStepSound(int step, float fvol)
{
	static int iSkipStep = 0;
	int irand;

	pmove->iStepLeft = !pmove->iStepLeft;

	if (!pmove->runfuncs)
		return;

	irand = pmove->RandomLong(0, 1) + (pmove->iStepLeft * 2);

	if (pmove->multiplayer && !pmove->movevars->footsteps)
		return;

	switch (step)
	{
		default:
		case STEP_CONCRETE:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_step1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_step3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_step2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_step4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_METAL:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_metal1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_metal3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_metal2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_metal4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_DIRT:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_dirt1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_dirt3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_dirt2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_dirt4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_VENT:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_duct1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_duct3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_duct2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_duct4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_GRATE:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_grate1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_grate3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_grate2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_grate4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_TILE:
		{
			if (!pmove->RandomLong(0, 4))
				irand = 4;

			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_tile1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_tile3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_tile2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_tile4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 4: pmove->PM_PlaySound(CHAN_BODY, "player/pl_tile5.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_SLOSH:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_slosh1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_slosh3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_slosh2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_slosh4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_WADE:
		{
			if (iSkipStep == 0)
			{
				iSkipStep++;
				break;
			}

			if (iSkipStep++ == 3)
				iSkipStep = 0;

			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_LADDER:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_ladder1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_ladder3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_ladder2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_ladder4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}

		case STEP_SNOW:
		{
			switch (irand)
			{
				case 0: pmove->PM_PlaySound(CHAN_BODY, "player/pl_snow1.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 1: pmove->PM_PlaySound(CHAN_BODY, "player/pl_snow3.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 2: pmove->PM_PlaySound(CHAN_BODY, "player/pl_snow2.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
				case 3: pmove->PM_PlaySound(CHAN_BODY, "player/pl_snow4.wav", fvol, ATTN_NORM, 0, PITCH_NORM); break;
			}

			break;
		}
	}
}

int PM_MapTextureTypeStepType(chTextureType)
{
	switch (chTextureType)
	{
		default:
		case CHAR_TEX_CONCRETE: return STEP_CONCRETE;
		case CHAR_TEX_METAL: return STEP_METAL;
		case CHAR_TEX_DIRT: return STEP_DIRT;
		case CHAR_TEX_VENT: return STEP_VENT;
		case CHAR_TEX_GRATE: return STEP_GRATE;
		case CHAR_TEX_TILE: return STEP_TILE;
		case CHAR_TEX_SLOSH: return STEP_SLOSH;
		case CHAR_TEX_SNOW: return STEP_SNOW;
	}
}

void PM_CatagorizeTextureType(void)
{
	vec3_t start, end;
	const char *pTextureName;

	VectorCopy(pmove->origin, start);
	VectorCopy(pmove->origin, end);

	end[2] -= 64;

	pmove->sztexturename[0] = '\0';
	pmove->chtexturetype = CHAR_TEX_CONCRETE;

	pTextureName = pmove->PM_TraceTexture(pmove->onground, start, end);

	if (!pTextureName)
		return;

	if (*pTextureName == '-' || *pTextureName == '+')
		pTextureName += 2;

	if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
		pTextureName++;

	strcpy(pmove->sztexturename, pTextureName);
	pmove->sztexturename[CBTEXTURENAMEMAX - 1] = 0;

	pmove->chtexturetype = PM_FindTextureType(pmove->sztexturename);
}

void PM_UpdateStepSound(void)
{
	float fvol;
	vec3_t knee;
	vec3_t feet;
	vec3_t center;
	float height;
	float speed;
	int fLadder;
	int step;

	if (pmove->flTimeStepSound > 0)
		return;

	if (pmove->flags & FL_FROZEN)
		return;

	speed = Length(pmove->velocity);

	if (speed < 150)
	{
		pmove->flTimeStepSound = 400;
		return;
	}

	fLadder = (pmove->movetype == MOVETYPE_FLY);

	if (fLadder || (pmove->onground != -1))
	{
		PM_CatagorizeTextureType();

		VectorCopy(pmove->origin, center);
		VectorCopy(pmove->origin, knee);
		VectorCopy(pmove->origin, feet);

		height = pmove->player_maxs[pmove->usehull][2] - pmove->player_mins[pmove->usehull][2];

		knee[2] = pmove->origin[2] - 0.3 * height;
		feet[2] = pmove->origin[2] - 0.5 * height;

		if (fLadder)
		{
			step = STEP_LADDER;
			fvol = 0.35;
			pmove->flTimeStepSound = 350;
		}
		else if (pmove->PM_PointContents(knee, NULL) == CONTENTS_WATER)
		{
			step = STEP_WADE;
			fvol = 0.65;
			pmove->flTimeStepSound = 600;
		}
		else if (pmove->PM_PointContents(feet, NULL) == CONTENTS_WATER)
		{
			step = STEP_SLOSH;
			fvol = 0.5;
			pmove->flTimeStepSound = 300;
		}
		else
		{
			step = PM_MapTextureTypeStepType(pmove->chtexturetype);

			switch (pmove->chtexturetype)
			{
				case CHAR_TEX_CONCRETE:
				{
					fvol = 0.5;
					pmove->flTimeStepSound = 300;
					break;
				}

				case CHAR_TEX_METAL:
				{
					fvol = 0.5;
					pmove->flTimeStepSound = 300;
					break;
				}

				case CHAR_TEX_DIRT:
				{
					fvol = 0.55;
					pmove->flTimeStepSound = 300;
					break;
				}

				case CHAR_TEX_VENT:
				{
					fvol = 0.7;
					pmove->flTimeStepSound = 300;
					break;
				}

				default:
				{
					fvol = 0.5;
					break;
				}

				case CHAR_TEX_GRATE:
				{
					fvol = 0.5;
					pmove->flTimeStepSound = 300;
					break;
				}

				case CHAR_TEX_TILE:
				{
					fvol = 0.5;
					pmove->flTimeStepSound = 300;
					break;
				}

				case CHAR_TEX_SLOSH:
				{
					fvol = 0.5;
					pmove->flTimeStepSound = 300;
					break;
				}

				case CHAR_TEX_SNOW:
				{
					fvol = 0.5;
					pmove->flTimeStepSound = 300;
					break;
				}

				pmove->flTimeStepSound = 300;
			}
		}

		if (pmove->flags & FL_DUCKING || fLadder)
		{
			pmove->flTimeStepSound += 100;

			if (pmove->flags & FL_DUCKING && pmove->flDuckTime < 950.0)
				fvol *= 0.35;
		}

		PM_PlayStepSound(step, fvol);
	}
}

qboolean PM_AddToTouched(pmtrace_t tr, vec3_t impactvelocity)
{
	int i;

	for (i = 0; i < pmove->numtouch; i++)
	{
		if (pmove->touchindex[i].ent == tr.ent)
			break;
	}

	if (i != pmove->numtouch)
		return false;

	VectorCopy(impactvelocity, tr.deltavelocity);

	if (pmove->numtouch >= MAX_PHYSENTS)
		pmove->Con_DPrintf("Too many entities were touched!\n");

	pmove->touchindex[pmove->numtouch++] = tr;
	return true;
}

void PM_CheckVelocity(void)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (IS_NAN(pmove->velocity[i]))
		{
			pmove->Con_Printf("PM  Got a NaN velocity %i\n", i);
			pmove->velocity[i] = 0;
		}
		if (IS_NAN(pmove->origin[i]))
		{
			pmove->Con_Printf("PM  Got a NaN origin on %i\n", i);
			pmove->origin[i] = 0;
		}

		if (pmove->velocity[i] > pmove->movevars->maxvelocity) 
		{
			pmove->Con_DPrintf("PM  Got a velocity too high on %i\n", i);
			pmove->velocity[i] = pmove->movevars->maxvelocity;
		}
		else if (pmove->velocity[i] < -pmove->movevars->maxvelocity)
		{
			pmove->Con_DPrintf("PM  Got a velocity too low on %i\n", i);
			pmove->velocity[i] = -pmove->movevars->maxvelocity;
		}
	}
}

int PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float backoff;
	float change;
	float angle;
	int i, blocked;

	angle = normal[2];

	blocked = 0x00;

	if (angle > 0)
		blocked |= 0x01;

	if (!angle)
		blocked |= 0x02;

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;

		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}

void PM_AddCorrectGravity(void)
{
	float ent_gravity;

	if (pmove->waterjumptime)
		return;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * 0.5 * pmove->frametime);
	pmove->velocity[2] += pmove->basevelocity[2] * pmove->frametime;
	pmove->basevelocity[2] = 0;

	PM_CheckVelocity();
}

void PM_FixupGravityVelocity(void)
{
	float ent_gravity;

	if (pmove->waterjumptime)
		return;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * pmove->frametime * 0.5);

	PM_CheckVelocity();
}

int PM_FlyMove(void)
{
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int numplanes;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity, original_velocity;
	vec3_t new_velocity;
	int i, j;
	pmtrace_t trace;
	vec3_t end;
	float time_left, allFraction;
	int blocked;

	numbumps = 4;

	blocked = 0;
	numplanes = 0;

	VectorCopy(pmove->velocity, original_velocity);
	VectorCopy(pmove->velocity, primal_velocity);

	allFraction = 0;
	time_left = pmove->frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (!pmove->velocity[0] && !pmove->velocity[1] && !pmove->velocity[2])
			break;

		for (i = 0; i < 3; i++)
			end[i] = pmove->origin[i] + time_left * pmove->velocity[i];

		trace = pmove->PM_PlayerTrace(pmove->origin, end, PM_NORMAL, -1);

		allFraction += trace.fraction;

		if (trace.allsolid)
		{
			VectorCopy(vec3_origin, pmove->velocity);
			return 4;
		}

		if (trace.fraction > 0)
		{
			VectorCopy(trace.endpos, pmove->origin);
			VectorCopy(pmove->velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;

		PM_AddToTouched(trace, pmove->velocity);

		if (trace.plane.normal[2] > 0.7)
			blocked |= 1;

		if (!trace.plane.normal[2])
			blocked |= 2;

		time_left -= time_left * trace.fraction;

		if (numplanes >= MAX_CLIP_PLANES)
		{
			VectorCopy(vec3_origin, pmove->velocity);
			break;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		if (pmove->movetype == MOVETYPE_WALK && ((pmove->onground == -1) || (pmove->friction != 1)))
		{
			for (i = 0; i < numplanes; i++)
			{
				if (planes[i][2] > 0.7)
				{
					PM_ClipVelocity(original_velocity, planes[i], new_velocity, 1);
					VectorCopy(new_velocity, original_velocity);
				}
				else
					PM_ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + pmove->movevars->bounce * (1 - pmove->friction));
			}

			VectorCopy(new_velocity, pmove->velocity);
			VectorCopy(new_velocity, original_velocity);
		}
		else
		{
			for (i = 0; i < numplanes; i++)
			{
				PM_ClipVelocity(original_velocity, planes[i], pmove->velocity, 1);

				for (j = 0; j <numplanes; j++)
				{
					if (j != i)
					{
						if (DotProduct(pmove->velocity, planes[j]) < 0)
							break;
					}
				}

				if (j == numplanes)
					break;
			}

			if (i != numplanes)
			{
			}
			else
			{
				if (numplanes != 2)
				{
					VectorCopy(vec3_origin, pmove->velocity);
					break;
				}

				CrossProduct(planes[0], planes[1], dir);
				d = DotProduct(dir, pmove->velocity);
				VectorScale(dir, d, pmove->velocity);
			}

			if (DotProduct(pmove->velocity, primal_velocity) <= 0)
			{
				VectorCopy(vec3_origin, pmove->velocity);
				break;
			}
		}
	}

	if (allFraction == 0)
		VectorCopy(vec3_origin, pmove->velocity);

	return blocked;
}

void PM_Accelerate(vec3_t wishdir, float wishspeed, float accel)
{
	int i;
	float addspeed, accelspeed, currentspeed;

	if (pmove->dead)
		return;

	if (pmove->waterjumptime)
		return;

	currentspeed = DotProduct(pmove->velocity, wishdir);

	addspeed = wishspeed - currentspeed;

	if (addspeed <= 0)
		return;

	accelspeed = accel * pmove->frametime * wishspeed * pmove->friction;

	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmove->velocity[i] += accelspeed * wishdir[i];
}

void PM_WalkMove(void)
{
	int clip;
	int oldonground;
	int i;

	vec3_t wishvel;
	float spd;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;

	vec3_t dest, start;
	vec3_t original, originalvel;
	vec3_t down, downvel;
	float downdist, updist;

	pmtrace_t trace;

	if (pmove->fuser2 > 0.0)
	{
		float factor;
		factor = (100.0 - pmove->fuser2 * 0.001 * 19.0) * 0.01;
		pmove->velocity[0] *= factor;
		pmove->velocity[1] *= factor;
	}

	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;

	pmove->forward[2] = 0;
	pmove->right[2] = 0;

	VectorNormalize(pmove->forward);
	VectorNormalize(pmove->right);

	for (i = 0; i < 2; i++)
		wishvel[i] = pmove->forward[i] * fmove + pmove->right[i] * smove;

	wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pmove->maxspeed)
	{
		VectorScale(wishvel, pmove->maxspeed/wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}

	pmove->velocity[2] = 0;

	PM_Accelerate(wishdir, wishspeed, pmove->movevars->accelerate);
	pmove->velocity[2] = 0;

	VectorAdd(pmove->velocity, pmove->basevelocity, pmove->velocity);

	spd = Length(pmove->velocity);

	if (spd < 1.0f)
	{
		VectorClear(pmove->velocity);
		return;
	}

	oldonground = pmove->onground;

	dest[0] = pmove->origin[0] + pmove->velocity[0]*pmove->frametime;
	dest[1] = pmove->origin[1] + pmove->velocity[1]*pmove->frametime;
	dest[2] = pmove->origin[2];

	VectorCopy(dest, start);
	trace = pmove->PM_PlayerTrace(pmove->origin, dest, PM_NORMAL, -1);

	if (trace.fraction == 1)
	{
		VectorCopy(trace.endpos, pmove->origin);
		return;
	}

	if (oldonground == -1 && pmove->waterlevel == 0)
		return;

	if (pmove->waterjumptime)
		return;

	VectorCopy(pmove->origin, original);
	VectorCopy(pmove->velocity, originalvel);

	clip = PM_FlyMove();

	VectorCopy(pmove->origin, down);
	VectorCopy(pmove->velocity, downvel);

	VectorCopy(original, pmove->origin);
	VectorCopy(originalvel, pmove->velocity);

	VectorCopy(pmove->origin, dest);
	dest[2] += pmove->movevars->stepsize;

	trace = pmove->PM_PlayerTrace(pmove->origin, dest, PM_NORMAL, -1);

	if (!trace.startsolid && !trace.allsolid)
		VectorCopy(trace.endpos, pmove->origin);

	clip = PM_FlyMove();

	VectorCopy(pmove->origin, dest);
	dest[2] -= pmove->movevars->stepsize;

	trace = pmove->PM_PlayerTrace(pmove->origin, dest, PM_NORMAL, -1);

	if (trace.plane.normal[2] < 0.7)
		goto usedown;

	if (!trace.startsolid && !trace.allsolid)
		VectorCopy(trace.endpos, pmove->origin);

	VectorCopy(pmove->origin, pmove->up);

	downdist = (down[0] - original[0])*(down[0] - original[0]) + (down[1] - original[1])*(down[1] - original[1]);
	updist = (pmove->up[0] - original[0])*(pmove->up[0] - original[0]) + (pmove->up[1] - original[1])*(pmove->up[1] - original[1]);

	if (downdist > updist)
	{
usedown:
		VectorCopy(down, pmove->origin);
		VectorCopy(downvel, pmove->velocity);
	}
	else
		pmove->velocity[2] = downvel[2];
}

void PM_Friction(void)
{
	float *vel;
	float speed, newspeed, control;
	float friction;
	float drop;
	vec3_t newvel;

	if (pmove->waterjumptime)
		return;

	vel = pmove->velocity;

	speed = sqrt(vel[0] * vel[0] +vel[1] * vel[1] + vel[2] * vel[2]);

	if (speed < 0.1f)
		return;

	drop = 0;

	if (pmove->onground != -1)
	{
		vec3_t start, stop;
		pmtrace_t trace;

		start[0] = stop[0] = pmove->origin[0] + vel[0] / speed * 16;
		start[1] = stop[1] = pmove->origin[1] + vel[1] / speed * 16;
		start[2] = pmove->origin[2] + pmove->player_mins[pmove->usehull][2];
		stop[2] = start[2] - 34;

		trace = pmove->PM_PlayerTrace(start, stop, PM_NORMAL, -1);

		if (trace.fraction == 1.0)
			friction = pmove->movevars->friction * pmove->movevars->edgefriction;
		else
			friction = pmove->movevars->friction;

		friction *= pmove->friction;

		control = (speed < pmove->movevars->stopspeed) ? pmove->movevars->stopspeed : speed;
		drop += control*friction*pmove->frametime;
	}

	newspeed = speed - drop;

	if (newspeed < 0)
		newspeed = 0;

	newspeed /= speed;

	newvel[0] = vel[0] * newspeed;
	newvel[1] = vel[1] * newspeed;
	newvel[2] = vel[2] * newspeed;

	VectorCopy(newvel, pmove->velocity);
}

void PM_AirAccelerate(vec3_t wishdir, float wishspeed, float accel)
{
	int i;
	float addspeed, accelspeed, currentspeed, wishspd = wishspeed;

	if (pmove->dead)
		return;

	if (pmove->waterjumptime)
		return;

	if (wishspd > 30)
		wishspd = 30;

	currentspeed = DotProduct(pmove->velocity, wishdir);
	addspeed = wishspd - currentspeed;

	if (addspeed <= 0)
		return;

	accelspeed = accel * wishspeed * pmove->frametime * pmove->friction;

	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pmove->velocity[i] += accelspeed * wishdir[i];
}

void PM_WaterMove(void)
{
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	vec3_t start, dest;
	vec3_t temp;
	pmtrace_t trace;

	float speed, newspeed, addspeed, accelspeed;

	for (i = 0; i < 3; i++)
		wishvel[i] = pmove->forward[i] * pmove->cmd.forwardmove + pmove->right[i] * pmove->cmd.sidemove;

	if (!pmove->cmd.forwardmove && !pmove->cmd.sidemove && !pmove->cmd.upmove)
		wishvel[2] -= 60;
	else
		wishvel[2] += pmove->cmd.upmove;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pmove->maxspeed)
	{
		VectorScale(wishvel, pmove->maxspeed / wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}

	wishspeed *= 0.8;

	VectorAdd(pmove->velocity, pmove->basevelocity, pmove->velocity);
	VectorCopy(pmove->velocity, temp);
	speed = VectorNormalize(temp);

	if (speed)
	{
		newspeed = speed - pmove->frametime * speed * pmove->movevars->friction * pmove->friction;

		if (newspeed < 0)
			newspeed = 0;

		VectorScale(pmove->velocity, newspeed / speed, pmove->velocity);
	}
	else
		newspeed = 0;

	if (wishspeed < 0.1f)
		return;

	addspeed = wishspeed - newspeed;

	if (addspeed > 0)
	{
		VectorNormalize(wishvel);
		accelspeed = pmove->movevars->accelerate * wishspeed * pmove->frametime * pmove->friction;

		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (i = 0; i < 3; i++)
			pmove->velocity[i] += accelspeed * wishvel[i];
	}

	VectorMA(pmove->origin, pmove->frametime, pmove->velocity, dest);
	VectorCopy(dest, start);
	start[2] += pmove->movevars->stepsize + 1;
	trace = pmove->PM_PlayerTrace(start, dest, PM_NORMAL, -1);

	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(trace.endpos, pmove->origin);
		return;
	}

	PM_FlyMove();
}

void PM_AirMove(void)
{
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;

	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;

	pmove->forward[2] = 0;
	pmove->right[2] = 0;

	VectorNormalize(pmove->forward);
	VectorNormalize(pmove->right);

	for (i = 0; i < 2; i++)
	{
		wishvel[i] = pmove->forward[i] * fmove + pmove->right[i] * smove;
	}

	wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pmove->maxspeed)
	{
		VectorScale(wishvel, pmove->maxspeed / wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}

	PM_AirAccelerate(wishdir, wishspeed, pmove->movevars->airaccelerate);

	VectorAdd(pmove->velocity, pmove->basevelocity, pmove->velocity);

	PM_FlyMove();
}

qboolean PM_InWater(void)
{
	return (pmove->waterlevel > 1);
}

qboolean PM_CheckWater(void)
{
	vec3_t point;
	int cont;
	int truecont;
	float height;
	float heightover2;

	point[0] = pmove->origin[0] + (pmove->player_mins[pmove->usehull][0] + pmove->player_maxs[pmove->usehull][0]) * 0.5;
	point[1] = pmove->origin[1] + (pmove->player_mins[pmove->usehull][1] + pmove->player_maxs[pmove->usehull][1]) * 0.5;
	point[2] = pmove->origin[2] + pmove->player_mins[pmove->usehull][2] + 1;

	pmove->waterlevel = 0;
	pmove->watertype = CONTENTS_EMPTY;

	cont = pmove->PM_PointContents(point, &truecont);

	if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT)
	{
		pmove->watertype = cont;
		pmove->waterlevel = 1;

		height = (pmove->player_mins[pmove->usehull][2] + pmove->player_maxs[pmove->usehull][2]);
		heightover2 = height * 0.5;

		point[2] = pmove->origin[2] + heightover2;
		cont = pmove->PM_PointContents(point, NULL);

		if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT)
		{
			pmove->waterlevel = 2;

			point[2] = pmove->origin[2] + pmove->view_ofs[2];

			cont = pmove->PM_PointContents(point, NULL);

			if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT)
				pmove->waterlevel = 3;
		}

		if ((truecont <= CONTENTS_CURRENT_0) && (truecont >= CONTENTS_CURRENT_DOWN))
		{
			static vec3_t current_table[] =
			{
				{ 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 },
				{ 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
			};

			VectorMA(pmove->basevelocity, 50.0 * pmove->waterlevel, current_table[CONTENTS_CURRENT_0 - truecont], pmove->basevelocity);
		}
	}

	return pmove->waterlevel > 1;
}

void PM_CatagorizePosition(void)
{
	vec3_t point;
	pmtrace_t tr;

	PM_CheckWater();

	point[0] = pmove->origin[0];
	point[1] = pmove->origin[1];
	point[2] = pmove->origin[2] - 2;

	if (pmove->velocity[2] > 180)
	{
		pmove->onground = -1;
	}
	else
	{
		tr = pmove->PM_PlayerTrace(pmove->origin, point, PM_NORMAL, -1);

		if (tr.plane.normal[2] < 0.7)
			pmove->onground = -1;
		else
			pmove->onground = tr.ent;

		if (pmove->onground != -1)
		{
			pmove->waterjumptime = 0;

			if (pmove->waterlevel < 2 && !tr.startsolid && !tr.allsolid)
				VectorCopy(tr.endpos, pmove->origin);
		}

		if (tr.ent > 0)
		{
			PM_AddToTouched(tr, pmove->velocity);
		}
	}
}

int PM_GetRandomStuckOffsets(int nIndex, int server, vec3_t offset)
{
	int idx;
	idx = rgStuckLast[nIndex][server]++;

	VectorCopy(rgv3tStuckTable[idx % 54], offset);

	return (idx % 54);
}

void PM_ResetStuckOffsets(int nIndex, int server)
{
	rgStuckLast[nIndex][server] = 0;
}

#define PM_CHECKSTUCK_MINTIME 0.05

int PM_CheckStuck(void)
{
	vec3_t base;
	vec3_t offset;
	vec3_t test;
	int hitent;
	int idx;
	float fTime;
	int i;
	pmtrace_t traceresult;

	static float rgStuckCheckTime[MAX_CLIENTS][2];

	hitent = pmove->PM_TestPlayerPosition(pmove->origin, &traceresult);

	if (hitent == -1)
	{
		PM_ResetStuckOffsets(pmove->player_index, pmove->server);
		return 0;
	}

	VectorCopy(pmove->origin, base);

	if (!pmove->server)
	{
		if ((hitent == 0) || (pmove->physents[hitent].model != NULL))
		{
			int nReps = 0;

			PM_ResetStuckOffsets(pmove->player_index, pmove->server);

			do 
			{
				i = PM_GetRandomStuckOffsets(pmove->player_index, pmove->server, offset);

				VectorAdd(base, offset, test);

				if (pmove->PM_TestPlayerPosition(test, &traceresult) == -1)
				{
					PM_ResetStuckOffsets(pmove->player_index, pmove->server);

					VectorCopy(test, pmove->origin);
					return 0;
				}

				nReps++;
			}
			while (nReps < 54);
		}
	}

	if (pmove->server)
		idx = 0;
	else
		idx = 1;

	fTime = pmove->Sys_FloatTime();

	if (rgStuckCheckTime[pmove->player_index][idx] >= (fTime - PM_CHECKSTUCK_MINTIME))
		return 1;

	rgStuckCheckTime[pmove->player_index][idx] = fTime;

	pmove->PM_StuckTouch(hitent, &traceresult);

	i = PM_GetRandomStuckOffsets(pmove->player_index, pmove->server, offset);

	VectorAdd(base, offset, test);

	if ((hitent = pmove->PM_TestPlayerPosition(test, NULL)) == -1)
	{
		PM_ResetStuckOffsets(pmove->player_index, pmove->server);

		if (i >= 27)
			VectorCopy(test, pmove->origin);

		return 0;
	}

	if (pmove->cmd.buttons & (IN_JUMP | IN_DUCK | IN_ATTACK) && ( pmove->physents[hitent].player != 0))
	{
		float x, y, z;
		float xystep = 8.0;
		float zstep = 18.0;
		float xyminmax = xystep;
		float zminmax = 4 * zstep;

		for (z = 0; z <= zminmax; z += zstep)
		{
			for (x = -xyminmax; x <= xyminmax; x += xystep)
			{
				for (y = -xyminmax; y <= xyminmax; y += xystep)
				{
					VectorCopy(base, test);
					test[0] += x;
					test[1] += y;
					test[2] += z;

					if (pmove->PM_TestPlayerPosition(test, NULL) == -1)
					{
						VectorCopy(test, pmove->origin);
						return 0;
					}
				}
			}
		}
	}

	return 1;
}

void PM_SpectatorMove(void)
{
	float speed, drop, friction, control, newspeed;
	float accel;
	float currentspeed, addspeed, accelspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;

	if (pmove->iuser1 == OBS_ROAMING)
	{
#ifdef CLIENT_DLL
		if (iJumpSpectator)
		{
			VectorCopy(vJumpOrigin, pmove->origin);
			VectorCopy(vJumpAngles, pmove->angles);
			VectorCopy(vec3_origin, pmove->velocity);
			iJumpSpectator = 0;
			return;
		}
#endif
		speed = Length(pmove->velocity);

		if (speed < 1)
		{
			VectorCopy(vec3_origin, pmove->velocity)
		}
		else
		{
			drop = 0;

			friction = pmove->movevars->friction * 1.5;
			control = speed < pmove->movevars->stopspeed ? pmove->movevars->stopspeed : speed;
			drop += control*friction*pmove->frametime;

			newspeed = speed - drop;

			if (newspeed < 0)
				newspeed = 0;

			newspeed /= speed;

			VectorScale(pmove->velocity, newspeed, pmove->velocity);
		}

		fmove = pmove->cmd.forwardmove;
		smove = pmove->cmd.sidemove;

		VectorNormalize(pmove->forward);
		VectorNormalize(pmove->right);

		for (i = 0; i < 3; i++)
			wishvel[i] = pmove->forward[i] * fmove + pmove->right[i] * smove;

		wishvel[2] += pmove->cmd.upmove;

		VectorCopy(wishvel, wishdir);
		wishspeed = VectorNormalize(wishdir);

		if (wishspeed > pmove->movevars->spectatormaxspeed)
		{
			VectorScale(wishvel, pmove->movevars->spectatormaxspeed / wishspeed, wishvel);
			wishspeed = pmove->movevars->spectatormaxspeed;
		}

		currentspeed = DotProduct(pmove->velocity, wishdir);
		addspeed = wishspeed - currentspeed;

		if (addspeed <= 0)
			return;

		accelspeed = pmove->movevars->accelerate * pmove->frametime * wishspeed;

		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (i = 0; i < 3; i++)
			pmove->velocity[i] += accelspeed * wishdir[i];

		VectorMA(pmove->origin, pmove->frametime, pmove->velocity, pmove->origin);
	}
	else
	{
		int target;

		if (pmove->iuser2 <= 0)
			return;

		for (target = 0; target < pmove->numphysent; target++)
		{
			if (pmove->physents[target].info == pmove->iuser2)
				break;
		}

		if (target == pmove->numphysent)
			return;

		VectorCopy(pmove->physents[target].angles, pmove->angles);
		VectorCopy(pmove->physents[target].origin, pmove->origin);

		VectorCopy(vec3_origin, pmove->velocity);
	}
}

float PM_SplineFraction(float value, float scale)
{
	float valueSquared;

	value = scale * value;
	valueSquared = value * value;

	return 3 * valueSquared - 2 * valueSquared * value;
}

void PM_FixPlayerCrouchStuck(int direction)
{
	int hitent;
	int i;
	vec3_t test;

	hitent = pmove->PM_TestPlayerPosition(pmove->origin, NULL);

	if (hitent == -1)
		return;

	VectorCopy(pmove->origin, test);

	for (i = 0; i < 36; i++)
	{
		pmove->origin[2] += direction;
		hitent = pmove->PM_TestPlayerPosition(pmove->origin, NULL);

		if (hitent == -1)
			return;
	}

	VectorCopy(test, pmove->origin);
}

void PM_UnDuck(void)
{
	int i;
	pmtrace_t trace;
	vec3_t newOrigin;

	VectorCopy(pmove->origin, newOrigin);

	if (pmove->onground != -1)
	{
		newOrigin[2] += 18.0;
	}

	trace = pmove->PM_PlayerTrace(newOrigin, newOrigin, PM_NORMAL, -1);

	if (!trace.startsolid)
	{
		pmove->usehull = 0;

		trace = pmove->PM_PlayerTrace(newOrigin, newOrigin, PM_NORMAL, -1);

		if (trace.startsolid)
		{
			pmove->usehull = 1;
			return;
		}

		pmove->flags &= ~FL_DUCKING;
		pmove->bInDuck = false;
		pmove->view_ofs[2] = VEC_VIEW;
		pmove->flDuckTime = 0;

		pmove->flTimeStepSound -= 100;

		if (pmove->flTimeStepSound < 0)
		{
			pmove->flTimeStepSound = 0;
		}

		VectorCopy(newOrigin, pmove->origin);

		PM_CatagorizePosition();
	}
}

void PM_Duck(void)
{
	int i;
	float time;
	float duckFraction;

	int buttonsChanged = (pmove->oldbuttons ^ pmove->cmd.buttons);
	int nButtonPressed = buttonsChanged & pmove->cmd.buttons;

	int duckchange = buttonsChanged & IN_DUCK ? 1 : 0;
	int duckpressed = nButtonPressed & IN_DUCK ? 1 : 0;

	if (pmove->cmd.buttons & IN_DUCK)
	{
		pmove->oldbuttons |= IN_DUCK;
	}
	else
	{
		pmove->oldbuttons &= ~IN_DUCK;
	}

	if (pmove->dead)
		return;

	if ((pmove->cmd.buttons & IN_DUCK) || (pmove->bInDuck) || (pmove->flags & FL_DUCKING))
	{
		pmove->cmd.forwardmove *= 0.333;
		pmove->cmd.sidemove *= 0.333;
		pmove->cmd.upmove *= 0.333;

		if (pmove->cmd.buttons & IN_DUCK)
		{
			if ((nButtonPressed & IN_DUCK) && !(pmove->flags & FL_DUCKING))
			{
				pmove->flDuckTime = 1000;
				pmove->bInDuck = true;
			}

			time = max(0.0, ( 1.0 - (float)pmove->flDuckTime / 1000.0));

			if (pmove->bInDuck)
			{
				if (((float)pmove->flDuckTime / 1000.0 <= (1.0 - TIME_TO_DUCK)) || (pmove->onground == -1))
				{
					pmove->usehull = 1;
					pmove->view_ofs[2] = VEC_DUCK_VIEW;

					pmove->flags |= FL_DUCKING;
					pmove->bInDuck = false;

					if (pmove->onground != -1)
					{
						pmove->origin[2] = pmove->origin[2] - 18.0;

						PM_FixPlayerCrouchStuck(STUCK_MOVEUP);

						PM_CatagorizePosition();
					}
				}
				else
				{
					float fMore = (VEC_DUCK_HULL_MIN - VEC_HULL_MIN);

					duckFraction = PM_SplineFraction(time, (1.0 / TIME_TO_DUCK));
					pmove->view_ofs[2] = ((VEC_DUCK_VIEW - fMore) * duckFraction) + (VEC_VIEW * (1 - duckFraction));
				}
			}
		}
		else
		{
			PM_UnDuck();
		}
	}
}

void PM_LadderMove(physent_t *pLadder)
{
	vec3_t ladderCenter;
	trace_t trace;
	qboolean onFloor;
	vec3_t floor;
	vec3_t modelmins, modelmaxs;

	if (pmove->movetype == MOVETYPE_NOCLIP)
		return;

	pmove->PM_GetModelBounds(pLadder->model, modelmins, modelmaxs);

	VectorAdd(modelmins, modelmaxs, ladderCenter);
	VectorScale(ladderCenter, 0.5, ladderCenter);

	pmove->movetype = MOVETYPE_FLY;

	VectorCopy(pmove->origin, floor);
	floor[2] += pmove->player_mins[pmove->usehull][2] - 1;

	if (pmove->PM_PointContents(floor, NULL) == CONTENTS_SOLID)
		onFloor = true;
	else
		onFloor = false;

	pmove->gravity = 0;
	pmove->PM_TraceModel(pLadder, pmove->origin, ladderCenter, &trace);

	if (trace.fraction != 1.0)
	{
		float forward = 0, right = 0;
		vec3_t vpn, v_right;

		float flSpeed = MAX_CLIMB_SPEED;

		if (pmove->maxspeed < MAX_CLIMB_SPEED)
			flSpeed = pmove->maxspeed;

		AngleVectors(pmove->angles, vpn, v_right, NULL);

		if (pmove->flags & FL_DUCKING)
		{
			flSpeed *= 0.333;
		}

		if (pmove->cmd.buttons & IN_BACK)
			forward -= flSpeed;

		if (pmove->cmd.buttons & IN_FORWARD)
			forward += flSpeed;

		if (pmove->cmd.buttons & IN_MOVELEFT)
			right -= flSpeed;

		if (pmove->cmd.buttons & IN_MOVERIGHT)
			right += flSpeed;

		if (pmove->cmd.buttons & IN_JUMP)
		{
			pmove->movetype = MOVETYPE_WALK;
			VectorScale(trace.plane.normal, 270, pmove->velocity);
		}
		else
		{
			if (forward != 0 || right != 0)
			{
				vec3_t velocity, perp, cross, lateral, tmp;
				float normal;

				VectorScale(vpn, forward, velocity);
				VectorMA(velocity, right, v_right, velocity);

				VectorClear(tmp);
				tmp[2] = 1;
				CrossProduct(tmp, trace.plane.normal, perp);
				VectorNormalize(perp);

				normal = DotProduct(velocity, trace.plane.normal);
				VectorScale(trace.plane.normal, normal, cross);

				VectorSubtract(velocity, cross, lateral);

				CrossProduct(trace.plane.normal, perp, tmp);
				VectorMA(lateral, -normal, tmp, pmove->velocity);

				if (onFloor && normal > 0)
				{
					VectorMA(pmove->velocity, MAX_CLIMB_SPEED, trace.plane.normal, pmove->velocity);
				}
			}
			else
			{
				VectorClear(pmove->velocity);
			}
		}
	}
}

physent_t *PM_Ladder(void)
{
	int i;
	physent_t *pe;
	hull_t *hull;
	int num;
	vec3_t test;

	for (i = 0; i < pmove->nummoveent; i++)
	{
		pe = &pmove->moveents[i];

		if (pe->model && (modtype_t)pmove->PM_GetModelType(pe->model) == mod_brush && pe->skin == CONTENTS_LADDER)
		{
			hull = (hull_t *)pmove->PM_HullForBsp(pe, test);
			num = hull->firstclipnode;

			VectorSubtract(pmove->origin, test, test);

			if (pmove->PM_HullPointContents (hull, num, test) == CONTENTS_EMPTY)
				continue;

			return pe;
		}
	}

	return NULL;
}

void PM_WaterJump(void)
{
	if (pmove->waterjumptime > 10000)
		pmove->waterjumptime = 10000;

	if (!pmove->waterjumptime)
		return;

	pmove->waterjumptime -= pmove->cmd.msec;

	if (pmove->waterjumptime < 0 || !pmove->waterlevel)
	{
		pmove->waterjumptime = 0;
		pmove->flags &= ~FL_WATERJUMP;
	}

	pmove->velocity[0] = pmove->movedir[0];
	pmove->velocity[1] = pmove->movedir[1];
}

void PM_AddGravity(void)
{
	float ent_gravity;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * pmove->frametime);
	pmove->velocity[2] += pmove->basevelocity[2] * pmove->frametime;
	pmove->basevelocity[2] = 0;
	PM_CheckVelocity();
}

pmtrace_t PM_PushEntity(vec3_t push)
{
	pmtrace_t trace;
	vec3_t end;

	VectorAdd(pmove->origin, push, end);

	trace = pmove->PM_PlayerTrace(pmove->origin, end, PM_NORMAL, -1);

	VectorCopy(trace.endpos, pmove->origin);

	if (trace.fraction < 1.0 && !trace.allsolid)
	{
		PM_AddToTouched(trace, pmove->velocity);
	}

	return trace;
}

void PM_Physics_Toss(void)
{
	pmtrace_t trace;
	vec3_t move;
	float backoff;

	PM_CheckWater();

	if (pmove->velocity[2] > 0)
		pmove->onground = -1;

	if (pmove->onground != -1)
	{
		if (VectorCompare(pmove->basevelocity, vec3_origin) && VectorCompare(pmove->velocity, vec3_origin))
			return;
	}

	PM_CheckVelocity();

	if (pmove->movetype != MOVETYPE_FLY && pmove->movetype != MOVETYPE_BOUNCEMISSILE && pmove->movetype != MOVETYPE_FLYMISSILE)
		PM_AddGravity();

	VectorAdd(pmove->velocity, pmove->basevelocity, pmove->velocity);

	PM_CheckVelocity();
	VectorScale(pmove->velocity, pmove->frametime, move);
	VectorSubtract(pmove->velocity, pmove->basevelocity, pmove->velocity);

	trace = PM_PushEntity(move);

	PM_CheckVelocity();

	if (trace.allsolid)
	{
		pmove->onground = trace.ent;
		VectorCopy(vec3_origin, pmove->velocity);
		return;
	}

	if (trace.fraction == 1)
	{
		PM_CheckWater();
		return;
	}

	if (pmove->movetype == MOVETYPE_BOUNCE)
		backoff = 2.0 - pmove->friction;
	else if (pmove->movetype == MOVETYPE_BOUNCEMISSILE)
		backoff = 2.0;
	else
		backoff = 1;

	PM_ClipVelocity(pmove->velocity, trace.plane.normal, pmove->velocity, backoff);

	if (trace.plane.normal[2] > 0.7)
	{
		float vel;
		vec3_t base;

		VectorClear(base);

		if (pmove->velocity[2] < pmove->movevars->gravity * pmove->frametime)
		{
			pmove->onground = trace.ent;
			pmove->velocity[2] = 0;
		}

		vel = DotProduct(pmove->velocity, pmove->velocity);

		if (vel < (30 * 30) || (pmove->movetype != MOVETYPE_BOUNCE && pmove->movetype != MOVETYPE_BOUNCEMISSILE))
		{
			pmove->onground = trace.ent;
			VectorCopy(vec3_origin, pmove->velocity);
		}
		else
		{
			VectorScale(pmove->velocity, (1.0 - trace.fraction) * pmove->frametime * 0.9, move);

			trace = PM_PushEntity(move);
		}

		VectorSubtract(pmove->velocity, base, pmove->velocity)
	}

	PM_CheckWater();
}

void PM_NoClip(void)
{
	int i;
	vec3_t wishvel;
	float fmove, smove;
	float currentspeed, addspeed, accelspeed;

	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;

	VectorNormalize(pmove->forward);
	VectorNormalize(pmove->right);

	for (i = 0; i < 3; i++)
	{
		wishvel[i] = pmove->forward[i] * fmove + pmove->right[i] * smove;
	}

	wishvel[2] += pmove->cmd.upmove;

	VectorMA(pmove->origin, pmove->frametime, wishvel, pmove->origin);

	VectorClear(pmove->velocity);
}

#define BUNNYJUMP_MAX_SPEED_FACTOR 1.2f

void PM_PreventMegaBunnyJumping(void)
{
	float spd;
	float fraction;
	float maxscaledspeed;

	maxscaledspeed = BUNNYJUMP_MAX_SPEED_FACTOR * pmove->maxspeed;

	if (maxscaledspeed <= 0.0f)
		return;

	spd = Length(pmove->velocity);

	if (spd <= maxscaledspeed)
		return;

	fraction = (maxscaledspeed / spd) * 0.8;

	VectorScale(pmove->velocity, fraction, pmove->velocity);
}

void PM_Jump(void)
{
	int i;
	qboolean tfc = false;

	qboolean cansuperjump = false;

	if (pmove->dead)
	{
		pmove->oldbuttons |= IN_JUMP;
		return;
	}

	if (pmove->waterjumptime)
	{
		pmove->waterjumptime -= pmove->cmd.msec;

		if (pmove->waterjumptime < 0)
		{
			pmove->waterjumptime = 0;
		}

		return;
	}

	if (pmove->waterlevel >= 2)
	{
		pmove->onground = -1;

		if (pmove->watertype == CONTENTS_WATER)
			pmove->velocity[2] = 100;
		else if (pmove->watertype == CONTENTS_SLIME)
			pmove->velocity[2] = 80;
		else
			pmove->velocity[2] = 50;

		if (pmove->flSwimTime <= 0)
		{
			pmove->flSwimTime = 1000;

			switch (pmove->RandomLong(0, 3))
			{ 
				case 0:
				{
					pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
					break;
				}

				case 1:
				{
					pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
					break;
				}

				case 2:
				{
					pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade3.wav", 1, ATTN_NORM, 0, PITCH_NORM);
					break;
				}

				case 3:
				{
					pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade4.wav", 1, ATTN_NORM, 0, PITCH_NORM);
					break;
				}
			}
		}

		return;
	}

 	if (pmove->onground == -1)
	{
		pmove->oldbuttons |= IN_JUMP;
		return;
	}

	if (~pmove->oldbuttons & IN_JUMP && (!pmove->bInDuck || ~pmove->flags & FL_DUCKING))
	{
		pmove->onground = -1;

		PM_CatagorizeTextureType();
		PM_PreventMegaBunnyJumping();

		if (Length(pmove->velocity) >= 150.0) 
		{
			PM_PlayStepSound(PM_MapTextureTypeStepType(pmove->chtexturetype), 1.0);
		}

		pmove->velocity[2] = sqrt(2 * 800 * 45.0);

		if (pmove->fuser2 > 0.0)
			pmove->velocity[2] *= (100.0 - pmove->fuser2 * 0.001 * 19.0) * 0.01;

		pmove->fuser2 = 1315.7894;

		PM_FixupGravityVelocity();

		pmove->oldbuttons |= IN_JUMP;
	}
}

#define WJ_HEIGHT 8

void PM_CheckWaterJump(void)
{
	vec3_t vecStart, vecEnd;
	vec3_t flatforward;
	vec3_t flatvelocity;
	float curspeed;
	pmtrace_t tr;
	int savehull;

	if (pmove->waterjumptime)
		return;

	if (pmove->velocity[2] < -180)
		return;

	flatvelocity[0] = pmove->velocity[0];
	flatvelocity[1] = pmove->velocity[1];
	flatvelocity[2] = 0;

	curspeed = VectorNormalize(flatvelocity);

	flatforward[0] = pmove->forward[0];
	flatforward[1] = pmove->forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	if (curspeed != 0.0 && (DotProduct(flatvelocity, flatforward) < 0.0))
		return;

	VectorCopy(pmove->origin, vecStart);
	vecStart[2] += WJ_HEIGHT;

	VectorMA(vecStart, 24, flatforward, vecEnd);

	savehull = pmove->usehull;
	pmove->usehull = 2;
	tr = pmove->PM_PlayerTrace(vecStart, vecEnd, PM_NORMAL, -1);

	if (tr.fraction < 1.0 && fabs(tr.plane.normal[2]) < 0.1f)
	{
		vecStart[2] += pmove->player_maxs[savehull][2] - WJ_HEIGHT;
		VectorMA(vecStart, 24, flatforward, vecEnd);
		VectorMA(vec3_origin, -50, tr.plane.normal, pmove->movedir);

		tr = pmove->PM_PlayerTrace(vecStart, vecEnd, PM_NORMAL, -1);

		if (tr.fraction == 1.0)
		{
			pmove->waterjumptime = 2000;
			pmove->velocity[2] = 225;
			pmove->oldbuttons |= IN_JUMP;
			pmove->flags |= FL_WATERJUMP;
		}
	}

	pmove->usehull = savehull;
}

void PM_CheckFalling(void)
{
	if (pmove->onground != -1 && !pmove->dead && pmove->flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		float fvol = 0.5;

		if (pmove->waterlevel > 0)
		{
		}
		else if (pmove->flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{
			fvol = 1.0;
		}
		else if (pmove->flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED / 2)
		{
			fvol = 0.85;
		}
		else if (pmove->flFallVelocity < PLAYER_MIN_BOUNCE_SPEED)
		{
			fvol = 0;
		}

		if (fvol > 0.0)
		{
			PM_CatagorizeTextureType();
			PM_PlayStepSound(PM_MapTextureTypeStepType(pmove->chtexturetype), fvol); 

			pmove->flTimeStepSound = 300;

			pmove->punchangle[2] = pmove->flFallVelocity * 0.013;

			if (pmove->punchangle[0] > 8)
				pmove->punchangle[0] = 8;
		}
	}

	if (pmove->onground != -1) 
	{
		pmove->flFallVelocity = 0;
	}
}

void PM_PlayWaterSounds(void)
{
	if ((pmove->oldwaterlevel == 0 && pmove->waterlevel != 0) || (pmove->oldwaterlevel != 0 && pmove->waterlevel == 0))
	{
		switch (pmove->RandomLong(0, 3))
		{
			case 0:
			{
				pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			}

			case 1:
			{
				pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade2.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			}

			case 2:
			{
				pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade3.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			}

			case 3:
			{
				pmove->PM_PlaySound(CHAN_BODY, "player/pl_wade4.wav", 1, ATTN_NORM, 0, PITCH_NORM);
				break;
			}
		}
	}
}

float PM_CalcRoll(vec3_t angles, vec3_t velocity, float rollangle, float rollspeed)
{
	float sign;
	float side;
	float value;
	vec3_t forward, right, up;

	AngleVectors(angles, forward, right, up);

	side = DotProduct(velocity, right);

	sign = side < 0 ? -1 : 1;

	side = fabs(side);

	value = rollangle;

	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
	else
	{
		side = value;
	}

	return side * sign;
}

void PM_DropPunchAngle(vec3_t punchangle)
{
	float len;

	len = VectorNormalize (punchangle);
	len -= (10.0 + len * 0.5) * pmove->frametime;
	len = max(len, 0.0);
	VectorScale(punchangle, len, punchangle);
}

void PM_CheckParamters(void)
{
	float spd;
	float maxspeed;
	vec3_t v_angle;

	spd = (pmove->cmd.forwardmove * pmove->cmd.forwardmove) + (pmove->cmd.sidemove * pmove->cmd.sidemove) + (pmove->cmd.upmove * pmove->cmd.upmove);
	spd = sqrt(spd);

	maxspeed = pmove->clientmaxspeed;

	if (maxspeed != 0.0)
		pmove->maxspeed = min(maxspeed, pmove->maxspeed);

	if ((spd != 0.0) && (spd > pmove->maxspeed))
	{
		float fRatio = pmove->maxspeed / spd;
		pmove->cmd.forwardmove *= fRatio;
		pmove->cmd.sidemove *= fRatio;
		pmove->cmd.upmove *= fRatio;
	}

	if (pmove->flags & FL_FROZEN || pmove->flags & FL_ONTRAIN || pmove->dead)
	{
		pmove->cmd.forwardmove = 0;
		pmove->cmd.sidemove = 0;
		pmove->cmd.upmove = 0;
	}

	PM_DropPunchAngle(pmove->punchangle);

	if (!pmove->dead)
	{
		VectorCopy(pmove->cmd.viewangles, v_angle); 
		VectorAdd(v_angle, pmove->punchangle, v_angle);

		pmove->angles[ROLL] = PM_CalcRoll(v_angle, pmove->velocity, pmove->movevars->rollangle, pmove->movevars->rollspeed) * 4;
		pmove->angles[PITCH] = v_angle[PITCH];
		pmove->angles[YAW] = v_angle[YAW];
	}
	else
	{
		VectorCopy(pmove->oldangles, pmove->angles);
	}

	if (pmove->dead)
	{
		pmove->view_ofs[2] = PM_DEAD_VIEWHEIGHT;
	}

	if (pmove->angles[YAW] > 180.0f)
	{
		pmove->angles[YAW] -= 360.0f;
	}
}

void PM_ReduceTimers(void)
{
	if (pmove->flTimeStepSound > 0)
	{
		pmove->flTimeStepSound -= pmove->cmd.msec;

		if (pmove->flTimeStepSound < 0)
		{
			pmove->flTimeStepSound = 0;
		}
	}

	if (pmove->flDuckTime > 0)
	{
		pmove->flDuckTime -= pmove->cmd.msec;

		if (pmove->flDuckTime < 0)
		{
			pmove->flDuckTime = 0;
		}
	}

	if (pmove->flSwimTime > 0)
	{
		pmove->flSwimTime -= pmove->cmd.msec;

		if (pmove->flSwimTime < 0)
		{
			pmove->flSwimTime = 0;
		}
	}

	if (pmove->fuser2 > 0.0)
	{
		pmove->fuser2 -= pmove->cmd.msec;

		if (pmove->fuser2 < 0)
			pmove->fuser2 = 0;
	}
}

void PM_PlayerMove(qboolean server)
{
	physent_t *pLadder = NULL;

	pmove->server = server;

	PM_CheckParamters();

	pmove->numtouch = 0;
	pmove->frametime = pmove->cmd.msec * 0.001;

	PM_ReduceTimers();

	AngleVectors(pmove->angles, pmove->forward, pmove->right, pmove->up);

	if ((pmove->spectator || pmove->iuser1 > 0) && (pmove->iuser3 <= 0 || pmove->deadflag == DEAD_DEAD))
	{
		PM_SpectatorMove();
		PM_CatagorizePosition();
		return;
	}

	if (pmove->movetype != MOVETYPE_NOCLIP && pmove->movetype != MOVETYPE_NONE)
	{
		if (PM_CheckStuck())
		{
			return;
		}
	}

	PM_CatagorizePosition();

	pmove->oldwaterlevel = pmove->waterlevel;

	if (pmove->onground == -1)
	{
		pmove->flFallVelocity = -pmove->velocity[2];
	}

	g_onladder = 0;

	if (!pmove->dead && !(pmove->flags & FL_ONTRAIN))
	{
		pLadder = PM_Ladder();

		if (pLadder)
		{
			g_onladder = 1;
		}
	}

	PM_Duck();
	PM_UpdateStepSound();

	if (!pmove->dead && !(pmove->flags & FL_ONTRAIN))
	{
		if (pLadder)
		{
			PM_LadderMove(pLadder);
		}
		else if (pmove->movetype != MOVETYPE_WALK && pmove->movetype != MOVETYPE_NOCLIP)
		{
			pmove->movetype = MOVETYPE_WALK;
		}
	}

	switch (pmove->movetype)
	{
		default:
		{
			pmove->Con_DPrintf("Bogus pmove player movetype %i on (%i) 0=cl 1=sv\n", pmove->movetype, pmove->server);
			break;
		}

		case MOVETYPE_NONE:
		{
			break;
		}

		case MOVETYPE_NOCLIP:
		{
			PM_NoClip();
			break;
		}

		case MOVETYPE_TOSS:
		case MOVETYPE_BOUNCE:
		{
			PM_Physics_Toss();
			break;
		}

		case MOVETYPE_FLY:
		{
			PM_CheckWater();

			if (pmove->cmd.buttons & IN_JUMP)
			{
				if (!pLadder)
				{
					PM_Jump();
				}
			}
			else
			{
				pmove->oldbuttons &= ~IN_JUMP;
			}

			VectorAdd(pmove->velocity, pmove->basevelocity, pmove->velocity);
			PM_FlyMove();
			VectorSubtract(pmove->velocity, pmove->basevelocity, pmove->velocity);
			break;
		}

		case MOVETYPE_WALK:
		{
			if (!PM_InWater())
			{
				PM_AddCorrectGravity();
			}

			if (pmove->waterjumptime)
			{
				PM_WaterJump();
				PM_FlyMove();

				PM_CheckWater();
				return;
			}

			if (pmove->waterlevel >= 2)
			{
				if (pmove->waterlevel == 2)
				{
					PM_CheckWaterJump();
				}

				if (pmove->velocity[2] < 0 && pmove->waterjumptime)
				{
					pmove->waterjumptime = 0;
				}

				if (pmove->cmd.buttons & IN_JUMP)
				{
					PM_Jump();
				}
				else
				{
					pmove->oldbuttons &= ~IN_JUMP;
				}

				PM_WaterMove();

				VectorSubtract(pmove->velocity, pmove->basevelocity, pmove->velocity);

				PM_CatagorizePosition();
			}
			else
			{
				if (pmove->cmd.buttons & IN_JUMP)
				{
					if (!pLadder)
					{
						PM_Jump();
					}
				}
				else
				{
					pmove->oldbuttons &= ~IN_JUMP;
				}

				if (pmove->onground != -1)
				{
					pmove->velocity[2] = 0.0;
					PM_Friction();
				}

				PM_CheckVelocity();

				if (pmove->onground != -1)
				{
					PM_WalkMove();
				}
				else
				{
					PM_AirMove();
				}

				PM_CatagorizePosition();

				VectorSubtract(pmove->velocity, pmove->basevelocity, pmove->velocity);

				PM_CheckVelocity();

				if (!PM_InWater())
				{
					PM_FixupGravityVelocity();
				}

				if (pmove->onground != -1)
				{
					pmove->velocity[2] = 0;
				}

				PM_CheckFalling();
			}

			PM_PlayWaterSounds();
			break;
		}
	}
}

void PM_CreateStuckTable(void)
{
	float x, y, z;
	int idx;
	int i;
	float zi[3];

	memset(rgv3tStuckTable, 0, sizeof(rgv3tStuckTable));

	idx = 0;
	x = y = 0;

	for (z = -0.125; z <= 0.125; z += 0.125, idx++)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
	}

	x = z = 0;

	for (y = -0.125; y <= 0.125; y += 0.125, idx++)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
	}

	y = z = 0;

	for (x = -0.125; x <= 0.125; x += 0.125, idx++)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
	}

	for (x = -0.125; x <= 0.125; x += 0.250)
	{
		for (y = -0.125; y <= 0.125; y += 0.250)
		{
			for (z = -0.125; z <= 0.125; z += 0.250, idx++)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
			}
		}
	}

	x = y = 0;
	zi[0] = 0;
	zi[1] = 1;
	zi[2] = 6;

	for (i = 0; i < 3; i++, idx++)
	{
		z = zi[i];
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
	}

	x = z = 0;

	for (y = -2; y <= 2; y += 2, idx++)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
	}

	y = z = 0;

	for (x = -2; x <= 2; x += 2, idx++)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
	}

	for (i = 0; i < 3; i++)
	{
		z = zi[i];

		for (y = -2; y <= 2; y += 2)
		{
			for (z = -2; z <= 2; z += 2, idx++)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
			}
		}
	}
}

void PM_Move (struct playermove_s *ppmove, int server)
{
	assert(pm_shared_initialized);

	pmove = ppmove;

	PM_PlayerMove((server != 0) ? true : false);

	if (pmove->onground != -1)
	{
		pmove->flags |= FL_ONGROUND;
	}
	else
	{
		pmove->flags &= ~FL_ONGROUND;
	}

	if (!pmove->multiplayer && (pmove->movetype == MOVETYPE_WALK))
	{
		pmove->friction = 1.0f;
	}
}

int PM_GetVisEntInfo(int ent)
{
	if (ent >= 0 && ent <= pmove->numvisent)
	{
		return pmove->visents[ent].info;
	}

	return -1;
}

int PM_GetPhysEntInfo(int ent)
{
	if (ent >= 0 && ent <= pmove->numphysent)
		return pmove->physents[ent].info;

	return -1;
}

void PM_Init(struct playermove_s *ppmove)
{
	assert(!pm_shared_initialized);

	pmove = ppmove;

	PM_CreateStuckTable();
	PM_InitTextureTypes();

	pm_shared_initialized = 1;
}
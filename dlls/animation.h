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

#ifndef ANIMATION_H
#define ANIMATION_H
#ifdef _WIN32
#pragma once
#endif

#define ANIM_SWIM_1			8
#define ANIM_SWIM_2			9
#define ANIM_FIRST_DEATH_SEQUENCE	101

#include "com_model.h"
#include "studio.h"
#include "r_studioint.h"

#define NOXREF
#define C_DLLEXPORT
#define ACTIVITY_NOT_AVAILABLE -1

int ExtractBbox(void *pmodel, int sequence, float *mins, float *maxs);
int LookupActivity(void *pmodel, entvars_t *pev, int activity);
int LookupActivityHeaviest(void *pmodel, entvars_t *pev, int activity);
NOXREF void GetEyePosition(void *pmodel, float *vecEyePosition);
int LookupSequence(void *pmodel, const char *label);
int IsSoundEvent(int eventNumber);
NOXREF void SequencePrecache(void *pmodel, const char *pSequenceName);
void GetSequenceInfo(void *pmodel, entvars_t *pev, float *pflFrameRate, float *pflGroundSpeed);
int GetSequenceFlags(void *pmodel, entvars_t *pev);
int GetAnimationEvent(void *pmodel, entvars_t *pev, MonsterEvent_t *pMonsterEvent, float flStart, float flEnd, int index);
float SetController(void *pmodel, entvars_t *pev, int iController, float flValue);
float SetBlending(void *pmodel, entvars_t *pev, int iBlender, float flValue);
int FindTransition(void *pmodel, int iEndingAnim, int iGoalAnim, int *piDir);
void SetBodygroup(void *pmodel, entvars_t *pev, int iGroup, int iValue);
int GetBodygroup(void *pmodel, entvars_t *pev, int iGroup);
C_DLLEXPORT int Server_GetBlendingInterface(int version, struct sv_blending_interface_s **ppinterface, struct engine_studio_api_s *pstudio, float *rotationmatrix, float *bonetransform);
void AngleQuaternion(vec_t *angles, vec_t *quaternion);
void QuaternionSlerp(vec_t *p, vec_t *q, float t, vec_t *qt);
void QuaternionMatrix(vec_t *quaternion, float (*matrix)[4]);
mstudioanim_t *StudioGetAnim(model_t *m_pSubModel, mstudioseqdesc_t *pseqdesc);
mstudioanim_t *LookupAnimation(model_t *model, mstudioseqdesc_t *pseqdesc, int index);
void StudioCalcBoneAdj(float dadt, float *adj, const byte *pcontroller1, const byte *pcontroller2, byte mouthopen);
void StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q);
void StudioCalcBonePosition(int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos);
void StudioSlerpBones(vec4_t *q1, float pos1[][3], vec4_t *q2, float pos2[][3], float s);
void StudioCalcRotations(mstudiobone_t *pbones, int *chain, int chainlength, float *adj, float pos[128][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f, float s);
void ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);
void SV_StudioSetupBones(struct model_s *pModel, float frame, int sequence, const vec3_t angles, const vec3_t origin, const byte *pcontroller, const byte *pblending,int iBone, const edict_t *pEdict);


extern struct sv_blending_interface_s svBlending;
extern server_studio_api_t IEngineStudio;
extern studiohdr_t *g_pstudiohdr;
extern float (*g_pRotationMatrix)[3][4];
extern float (*g_pBoneTransform)[128][3][4];
extern float omega;
extern float cosom;
extern float sinom;
extern float sclp;
extern float sclq;

#endif // ANIMATION_H


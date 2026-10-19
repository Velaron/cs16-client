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

// Big thanks to Chicken Fortress developers
// for this code.
#include <assert.h>
#include "hud.h"
#include "aimbot.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "pm_defs.h"
#include "camera.h"
#include "eventscripts.h"

// Note: do not undef max/min here — this file still uses old bare min() in several places.

extern cvar_t *cl_player_outline;  // clean visibility silhouette - only on visible player models (respects walls/depth)
extern cvar_t *cl_hitbox_outline;   // hitbox wireframe outlines for visible players (clean aiming reference)
extern cvar_t *cl_hitbox_outline_head_only; // only head hitbox (much cleaner, recommended)
extern cvar_t *cl_hitbox_outline_max_dist;   // max distance for hitbox drawing (perf)
extern cvar_t *cl_esp;   // 1 = all hitboxes through walls, 2 = head only

// ====================================================================
// MODIFICADO: EspIsOffline() agora sempre retorna 1 (verdadeiro).
// Isso faz o ESP funcionar apenas com cl_esp, sem precisar de
// esp_offline. Original:
//     return gEngfuncs.pfnGetCvarFloat("esp_offline") > 1.0f;
// ====================================================================
static int EspIsOffline(void)
{
	return 1;
}

#define ANIM_WALK_SEQUENCE 3
#define ANIM_JUMP_SEQUENCE 6
#define ANIM_SWIM_1 8
#define ANIM_SWIM_2 9
#define ANIM_FIRST_DEATH_SEQUENCE 101
#define ANIM_LAST_DEATH_SEQUENCE 159
#define ANIM_FIRST_EMOTION_SEQUENCE 198
#define ANIM_LAST_EMOTION_SEQUENCE 207
#define STUDIO_HITGROUP_HEAD 1

#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define ESP_HB_MAGIC 0x58424831u
#define ESP_HB_MAX   384
#define ESP_HB_PATH  "/tmp/cs16_hitbox.bin"

struct esp_hb_t
{
	float c[8][3];
	int32_t group;
	int32_t player;
};

struct esp_hb_frame_t
{
	uint32_t magic;
	uint32_t seq;
	uint32_t time_ms;
	int32_t count;
	esp_hb_t box[ESP_HB_MAX];
};

static esp_hb_frame_t *g_espHb;
static int g_espHbFd = -1;
static uint32_t g_espSeen;
static int g_espCount;
static esp_hb_t g_espBoxes[ESP_HB_MAX];
static int g_espBoxPart[ESP_HB_MAX];

/* ------------------------------------------------------------------
 * Classificacao do hitbox em uma das 20 partes do menu (padrao Sakura).
 * Usa o nome do bone porque o grupo do studio junta varios boxes.
 * ------------------------------------------------------------------ */
static void PartLowerCopy( char *dst, const char *src, int max )
{
	int i = 0;

	if( !dst || max <= 0 )
		return;
	if( src )
	{
		for( ; src[i] && i < max - 1; i++ )
		{
			char c = src[i];

			if( c >= 'A' && c <= 'Z' )
				c = (char)( c - 'A' + 'a' );
			dst[i] = c;
		}
	}
	dst[i] = 0;
}

static int PartSide( const char *n )
{
	if( strstr( n, " l " ) || strstr( n, "left" ) || strncmp( n, "l ", 2 ) == 0 )
		return 1;
	if( strstr( n, " r " ) || strstr( n, "right" ) || strncmp( n, "r ", 2 ) == 0 )
		return 2;
	return 0;
}

static int PartFromBone( studiohdr_t *hdr, int bone, int group )
{
	char name[48];
	int side;

	name[0] = 0;
	if( hdr && bone >= 0 && bone < hdr->numbones )
	{
		mstudiobone_t *bones = (mstudiobone_t *)( (byte *)hdr + hdr->boneindex );

		PartLowerCopy( name, bones[bone].name, sizeof( name ) );
	}

	side = PartSide( name );

	if( strstr( name, "head" ) )
		return AIM_PART_HEAD;
	if( strstr( name, "neck" ) )
		return AIM_PART_NECK;
	if( strstr( name, "spine4" ) || strstr( name, "spine3" ) || strstr( name, "thorax" ) )
		return AIM_PART_UPPER_CHEST;
	if( strstr( name, "spine2" ) )
		return AIM_PART_LOWER_CHEST;
	if( strstr( name, "spine1" ) )
		return AIM_PART_STOMACH;
	if( strstr( name, "spine" ) || strstr( name, "pelvis" ) || strstr( name, "hip" ) )
		return AIM_PART_BELT;

	if( strstr( name, "thigh" ) )
		return side == 2 ? AIM_PART_R_THIGH : AIM_PART_L_THIGH;
	if( strstr( name, "calf" ) || strstr( name, "tibia" ) || strstr( name, "shin" ) )
		return side == 2 ? AIM_PART_R_TIBIA : AIM_PART_L_TIBIA;
	if( strstr( name, "foot" ) || strstr( name, "ankle" ) || strstr( name, "toe" ) )
		return side == 2 ? AIM_PART_R_FOOT : AIM_PART_L_FOOT;

	if( strstr( name, "clavicle" ) || strstr( name, "shoulder" ) )
		return side == 2 ? AIM_PART_R_SHOULDER : AIM_PART_L_SHOULDER;
	if( strstr( name, "upperarm" ) )
		return side == 2 ? AIM_PART_R_SHOULDER : AIM_PART_L_SHOULDER;
	if( strstr( name, "forearm" ) || strstr( name, "elbow" ) )
		return side == 2 ? AIM_PART_R_ELBOW : AIM_PART_L_ELBOW;
	if( strstr( name, "hand" ) )
		return side == 2 ? AIM_PART_R_HAND : AIM_PART_L_HAND;

	/* fallback pelo grupo do studio */
	switch( group )
	{
	case 1: return AIM_PART_HEAD;
	case 2: return AIM_PART_UPPER_CHEST;
	case 3: return AIM_PART_STOMACH;
	case 4: return AIM_PART_L_SHOULDER;
	case 5: return AIM_PART_R_SHOULDER;
	case 6: return AIM_PART_L_THIGH;
	case 7: return AIM_PART_R_THIGH;
	}

	return AIM_PART_GENERIC;
}

static void EspHitbox_Open(void)
{
	if (g_espHb)
		return;

	g_espHbFd = open(ESP_HB_PATH, O_RDWR | O_CREAT, 0600);
	if (g_espHbFd < 0)
		return;
	if (ftruncate(g_espHbFd, sizeof(*g_espHb)) < 0)
		return;

	void *map = mmap(NULL, sizeof(*g_espHb), PROT_READ | PROT_WRITE, MAP_SHARED, g_espHbFd, 0);
	if (map == MAP_FAILED)
		return;

	g_espHb = (esp_hb_frame_t *)map;
	memset(g_espHb, 0, sizeof(*g_espHb));
	g_espHb->magic = ESP_HB_MAGIC;
}

void EspHitbox_BeginFrame(void)
{
	g_espSeen = 0;
	g_espCount = 0;
}

void EspHitbox_Flush(void)
{
	int i, maxclients;
	cl_entity_t *local;

	EspHitbox_Open();

	local = gEngfuncs.GetLocalPlayer();
	maxclients = gEngfuncs.GetMaxClients();
	if (maxclients > 128)
		maxclients = 128;

	if (!EspIsOffline())
		g_espCount = 1;
	else
	{
		for (i = 1; i <= maxclients; i++)
		{
			cl_entity_t *ent;

			if (g_espSeen & (1u << i))
				continue;
			if (local && local->index == i)
				continue;

			ent = gEngfuncs.GetEntityByIndex(i);
			if (!ent || !ent->player || !ent->model)
				continue;

			g_StudioRenderer.ExportPlayerHitboxes(ent);
		}
	}

	{
		aim_box_t snap[ESP_HB_MAX];
		int n = 0;
		int b;

		for (b = 0; b < g_espCount && n < ESP_HB_MAX; b++)
		{
			int k;
			float o[3];

			if (g_espBoxes[b].player <= 0)
				continue;
			o[0] = o[1] = o[2] = 0.f;
			for (k = 0; k < 8; k++)
			{
				o[0] += g_espBoxes[b].c[k][0];
				o[1] += g_espBoxes[b].c[k][1];
				o[2] += g_espBoxes[b].c[k][2];
			}
			snap[n].player = g_espBoxes[b].player;
			snap[n].group = g_espBoxes[b].group;
			snap[n].part = g_espBoxPart[b];
			snap[n].origin[0] = o[0] * 0.125f;
			snap[n].origin[1] = o[1] * 0.125f;
			snap[n].origin[2] = o[2] * 0.125f;
			for (k = 0; k < 8; k++)
			{
				snap[n].c[k][0] = g_espBoxes[b].c[k][0];
				snap[n].c[k][1] = g_espBoxes[b].c[k][1];
				snap[n].c[k][2] = g_espBoxes[b].c[k][2];
			}
			n++;
		}
		Aimbot_SetBoxes(snap, n);
	}

	if (!g_espHb)
		return;

	struct timespec ts;
	uint32_t odd;
	esp_hb_frame_t localFrame;

	memset(&localFrame, 0, sizeof(localFrame));
	localFrame.magic = ESP_HB_MAGIC;
	localFrame.count = g_espCount;
	if (g_espCount > 0)
		memcpy(localFrame.box, g_espBoxes, sizeof(esp_hb_t) * g_espCount);

	clock_gettime(CLOCK_MONOTONIC, &ts);
	localFrame.time_ms = (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);

	odd = (g_espHb->seq + 1u) | 1u;
	g_espHb->seq = odd;
	__sync_synchronize();
	memcpy((char *)g_espHb + 8, (char *)&localFrame + 8, sizeof(localFrame) - 8);
	__sync_synchronize();
	g_espHb->seq = odd + 1u;
}

CGameStudioModelRenderer g_StudioRenderer;

int g_rseq;
int g_gaitseq;
vec3_t g_clorg;
vec3_t g_clang;

void CounterStrike_GetSequence(int *seq, int *gaitseq)
{
	*seq = g_rseq;
	*gaitseq = g_gaitseq;
}

void CounterStrike_GetOrientation(float *o, float *a)
{
	VectorCopy(g_clorg, o);
	VectorCopy(g_clang, a);
}

float g_flStartScaleTime;
int iPrevRenderState;
int iRenderStateChanged;

engine_studio_api_t IEngineStudio;

static client_anim_state_t g_state;
static client_anim_state_t g_clientstate;

static void DrawStudioHitboxEdges( Vector corners[8] )
{
	gEngfuncs.pTriAPI->Vertex3fv( corners[0] ); gEngfuncs.pTriAPI->Vertex3fv( corners[1] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[1] ); gEngfuncs.pTriAPI->Vertex3fv( corners[2] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[2] ); gEngfuncs.pTriAPI->Vertex3fv( corners[3] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[3] ); gEngfuncs.pTriAPI->Vertex3fv( corners[0] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[4] ); gEngfuncs.pTriAPI->Vertex3fv( corners[5] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[5] ); gEngfuncs.pTriAPI->Vertex3fv( corners[6] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[6] ); gEngfuncs.pTriAPI->Vertex3fv( corners[7] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[7] ); gEngfuncs.pTriAPI->Vertex3fv( corners[4] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[0] ); gEngfuncs.pTriAPI->Vertex3fv( corners[4] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[1] ); gEngfuncs.pTriAPI->Vertex3fv( corners[5] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[2] ); gEngfuncs.pTriAPI->Vertex3fv( corners[6] );
	gEngfuncs.pTriAPI->Vertex3fv( corners[3] ); gEngfuncs.pTriAPI->Vertex3fv( corners[7] );
}

CGameStudioModelRenderer::CGameStudioModelRenderer(void)
{
	m_bLocal = false;
}

mstudioanim_t *CGameStudioModelRenderer::LookupAnimation(mstudioseqdesc_t *pseqdesc, int index)
{
	mstudioanim_t *panim = NULL;

	panim = StudioGetAnim(m_pRenderModel, pseqdesc);

	if (index < 0)
		return panim;

	if (index > (pseqdesc->numblends - 1))
		return panim;

	panim += index * m_pStudioHeader->numbones;
	return panim;
}

void CGameStudioModelRenderer::StudioSetupBones(void)
{
	int i;
	double f;

	mstudiobone_t *pbones;
	mstudioseqdesc_t *pseqdesc;
	mstudioanim_t *panim;

	static float pos[MAXSTUDIOBONES][3];
	static vec4_t q[MAXSTUDIOBONES];
	float bonematrix[3][4];

	static float pos2[MAXSTUDIOBONES][3];
	static vec4_t q2[MAXSTUDIOBONES];
	static float pos3[MAXSTUDIOBONES][3];
	static vec4_t q3[MAXSTUDIOBONES];
	static float pos4[MAXSTUDIOBONES][3];
	static vec4_t q4[MAXSTUDIOBONES];

	if (!m_pCurrentEntity->player)
	{
		CStudioModelRenderer::StudioSetupBones();
		return;
	}

	if (m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq)
		m_pCurrentEntity->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;
	panim = StudioGetAnim(m_pRenderModel, pseqdesc);

	f = StudioEstimateFrame(pseqdesc);

	if (m_pPlayerInfo->gaitsequence == ANIM_WALK_SEQUENCE)
	{
		if (m_pCurrentEntity->curstate.blending[0] <= 26)
		{
			m_pCurrentEntity->curstate.blending[0] = 0;
			m_pCurrentEntity->latched.prevseqblending[0] = m_pCurrentEntity->curstate.blending[0];
		}
		else
		{
			m_pCurrentEntity->curstate.blending[0] -= 26;
			m_pCurrentEntity->latched.prevseqblending[0] = m_pCurrentEntity->curstate.blending[0];
		}
	}

	if (pseqdesc->numblends == 9)
	{
		float s = m_pCurrentEntity->curstate.blending[0];
		float t = m_pCurrentEntity->curstate.blending[1];

		if (s <= 127.0)
		{
			s = (s * 2.0);

			if (t <= 127.0)
			{
				t = (t * 2.0);

				StudioCalcRotations(pos, q, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 1);
				StudioCalcRotations(pos2, q2, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 3);
				StudioCalcRotations(pos3, q3, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 4);
				StudioCalcRotations(pos4, q4, pseqdesc, panim, f);
			}
			else
			{
				t = 2.0 * (t - 127.0);

				panim = LookupAnimation(pseqdesc, 3);
				StudioCalcRotations(pos, q, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 4);
				StudioCalcRotations(pos2, q2, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 6);
				StudioCalcRotations(pos3, q3, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 7);
				StudioCalcRotations(pos4, q4, pseqdesc, panim, f);
			}
		}
		else
		{
			s = 2.0 * (s - 127.0);

			if (t <= 127.0)
			{
				t = (t * 2.0);

				panim = LookupAnimation(pseqdesc, 1);
				StudioCalcRotations(pos, q, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 2);
				StudioCalcRotations(pos2, q2, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 4);
				StudioCalcRotations(pos3, q3, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 5);
				StudioCalcRotations(pos4, q4, pseqdesc, panim, f);
			}
			else
			{
				t = 2.0 * (t - 127.0);

				panim = LookupAnimation(pseqdesc, 4);
				StudioCalcRotations(pos, q, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 5);
				StudioCalcRotations(pos2, q2, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 7);
				StudioCalcRotations(pos3, q3, pseqdesc, panim, f);
				panim = LookupAnimation(pseqdesc, 8);
				StudioCalcRotations(pos4, q4, pseqdesc, panim, f);
			}
		}

		s /= 255.0;
		t /= 255.0;

		StudioSlerpBones(q, pos, q2, pos2, s);
		StudioSlerpBones(q3, pos3, q4, pos4, s);
		StudioSlerpBones(q, pos, q3, pos3, t);
	}
	else
	{
		StudioCalcRotations(pos, q, pseqdesc, panim, f);
	}

	if (m_fDoInterp && m_pCurrentEntity->latched.sequencetime && (m_pCurrentEntity->latched.sequencetime + 0.2 > m_clTime) && (m_pCurrentEntity->latched.prevsequence < m_pStudioHeader->numseq))
	{
		static float pos1b[MAXSTUDIOBONES][3];
		static vec4_t q1b[MAXSTUDIOBONES];
		float s = m_pCurrentEntity->latched.prevseqblending[0];
		float t = m_pCurrentEntity->latched.prevseqblending[1];

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->latched.prevsequence;
		panim = StudioGetAnim(m_pRenderModel, pseqdesc);

		if (pseqdesc->numblends == 9)
		{
			if (s <= 127.0)
			{
				s = (s * 2.0);

				if (t <= 127.0)
				{
					t = (t * 2.0);

					StudioCalcRotations(pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 1);
					StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 3);
					StudioCalcRotations(pos3, q3, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 4);
					StudioCalcRotations(pos4, q4, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
				}
				else
				{
					t = 2.0 * (t - 127.0);

					panim = LookupAnimation(pseqdesc, 3);
					StudioCalcRotations(pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 4);
					StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 6);
					StudioCalcRotations(pos3, q3, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 7);
					StudioCalcRotations(pos4, q4, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
				}
			}
			else
			{
				s = 2.0 * (s - 127.0);

				if (t <= 127.0)
				{
					t = (t * 2.0);

					panim = LookupAnimation(pseqdesc, 1);
					StudioCalcRotations(pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 2);
					StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 4);
					StudioCalcRotations(pos3, q3, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 5);
					StudioCalcRotations(pos4, q4, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
				}
				else
				{
					t = 2.0 * (t - 127.0);

					panim = LookupAnimation(pseqdesc, 4);
					StudioCalcRotations(pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 5);
					StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 7);
					StudioCalcRotations(pos3, q3, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
					panim = LookupAnimation(pseqdesc, 8);
					StudioCalcRotations(pos4, q4, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
				}
			}

			s /= 255.0;
			t /= 255.0;

			StudioSlerpBones(q1b, pos1b, q2, pos2, s);
			StudioSlerpBones(q3, pos3, q4, pos4, s);
			StudioSlerpBones(q1b, pos1b, q3, pos3, t);
		}
		else
		{
			StudioCalcRotations(pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe);
		}

		s = 1.0 - (m_clTime - m_pCurrentEntity->latched.sequencetime) / 0.2;
		StudioSlerpBones(q, pos, q1b, pos1b, s);
	}
	else
	{
		m_pCurrentEntity->latched.prevframe = f;
	}

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	if (m_pPlayerInfo && (m_pCurrentEntity->curstate.sequence < ANIM_FIRST_DEATH_SEQUENCE || m_pCurrentEntity->curstate.sequence > ANIM_LAST_DEATH_SEQUENCE) && (m_pCurrentEntity->curstate.sequence < ANIM_FIRST_EMOTION_SEQUENCE || m_pCurrentEntity->curstate.sequence > ANIM_LAST_EMOTION_SEQUENCE) && m_pCurrentEntity->curstate.sequence != ANIM_SWIM_1 && m_pCurrentEntity->curstate.sequence != ANIM_SWIM_2)
	{
		int copy = 1;

		if (m_pPlayerInfo->gaitsequence >= m_pStudioHeader->numseq)
			m_pPlayerInfo->gaitsequence = 0;

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex ) + m_pPlayerInfo->gaitsequence;

		panim = StudioGetAnim(m_pRenderModel, pseqdesc);
		StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pPlayerInfo->gaitframe);

		for (i = 0; i < m_pStudioHeader->numbones; i++)
		{
			if (!strcmp(pbones[i].name, "Bip01 Spine"))
				copy = 0;
			else if (!strcmp(pbones[pbones[i].parent].name, "Bip01 Pelvis"))
				copy = 1;

			if (copy)
			{
				memcpy(pos[i], pos2[i], sizeof(pos[i]));
				memcpy(q[i], q2[i], sizeof(q[i]));
			}
		}
	}

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		QuaternionMatrix(q[i], bonematrix);

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1)
		{
			if (IEngineStudio.IsHardware())
			{
				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
				MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);
			}
			else
			{
				ConcatTransforms((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
			}

			StudioFxTransform(m_pCurrentEntity, (*m_pbonetransform)[i]);
		}
		else
		{
			ConcatTransforms((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
			ConcatTransforms((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
		}
	}
}

void CGameStudioModelRenderer::StudioEstimateGait(entity_state_t *pplayer)
{
	float dt;
	vec3_t est_velocity;

	dt = (m_clTime - m_clOldTime);
	dt = max(0.0, dt);
	dt = min(1.0, dt);

	if (dt == 0 || m_pPlayerInfo->renderframe == m_nFrameCount)
	{
		m_flGaitMovement = 0;
		return;
	}

	if (m_fGaitEstimation)
	{
		VectorSubtract(m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin, est_velocity);
		VectorCopy(m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin);
		m_flGaitMovement = est_velocity.Length();

		if (dt <= 0 || m_flGaitMovement / dt < 5)
		{
			m_flGaitMovement = 0;
			est_velocity[0] = 0;
			est_velocity[1] = 0;
		}
	}
	else
	{
		VectorCopy(pplayer->velocity, est_velocity);
		m_flGaitMovement = est_velocity.Length() * dt;
	}

	if (est_velocity[1] == 0 && est_velocity[0] == 0)
	{
		float flYawDiff = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;

		if (flYawDiff > 180)
			flYawDiff -= 360;

		if (flYawDiff < -180)
			flYawDiff += 360;

		if (dt < 0.25)
			flYawDiff *= dt * 4;
		else
			flYawDiff *= dt;

		m_pPlayerInfo->gaityaw += flYawDiff;
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - (int)(m_pPlayerInfo->gaityaw / 360) * 360;

		m_flGaitMovement = 0;
	}
	else
	{
		m_pPlayerInfo->gaityaw = (atan2(est_velocity[1], est_velocity[0]) * 180 / M_PI);

		if (m_pPlayerInfo->gaityaw > 180)
			m_pPlayerInfo->gaityaw = 180;

		if (m_pPlayerInfo->gaityaw < -180)
			m_pPlayerInfo->gaityaw = -180;
	}
}

void CGameStudioModelRenderer::StudioPlayerBlend(mstudioseqdesc_t *pseqdesc, int *pBlend, float *pPitch)
{
	float range = 45.0;

	*pBlend = (*pPitch * 3);

	if (*pBlend <= -range)
		*pBlend = 255;
	else if (*pBlend >= range)
		*pBlend = 0;
	else
		*pBlend = 255 * (range - *pBlend) / (2 * range);

	*pPitch = 0;
}

void CGameStudioModelRenderer::CalculatePitchBlend(entity_state_t *pplayer)
{
	mstudioseqdesc_t *pseqdesc;
	int iBlend;

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	StudioPlayerBlend(pseqdesc, &iBlend, &m_pCurrentEntity->angles[PITCH]);

	m_pCurrentEntity->latched.prevangles[PITCH] = m_pCurrentEntity->angles[PITCH];
	m_pCurrentEntity->curstate.blending[1] = iBlend;
	m_pCurrentEntity->latched.prevblending[1] = m_pCurrentEntity->curstate.blending[1];
	m_pCurrentEntity->latched.prevseqblending[1] = m_pCurrentEntity->curstate.blending[1];
}

void CGameStudioModelRenderer::CalculateYawBlend(entity_state_t *pplayer)
{
	float flYaw;

	StudioEstimateGait(pplayer);

	flYaw = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
	flYaw = fmod(flYaw, 360.0f);

	if (flYaw < -180)
		flYaw = flYaw + 360;
	else if (flYaw > 180)
		flYaw = flYaw - 360;

	float maxyaw = 120.0;

	if (flYaw > maxyaw)
	{
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - 180;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw - 180;
	}
	else if (flYaw < -maxyaw)
	{
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw + 180;
		m_flGaitMovement = -m_flGaitMovement;
		flYaw = flYaw + 180;
	}

	float blend_yaw = (flYaw / 90.0) * 128.0 + 127.0;

	blend_yaw = 255.0 - bound( 0.0, blend_yaw, 255.0 );

	m_pCurrentEntity->curstate.blending[0] = (int)(blend_yaw);
	m_pCurrentEntity->latched.prevblending[0] = m_pCurrentEntity->curstate.blending[0];
	m_pCurrentEntity->latched.prevseqblending[0] = m_pCurrentEntity->curstate.blending[0];

	m_pCurrentEntity->angles[YAW] = m_pPlayerInfo->gaityaw;

	if (m_pCurrentEntity->angles[YAW] < -0)
		m_pCurrentEntity->angles[YAW] += 360;

	m_pCurrentEntity->latched.prevangles[YAW] = m_pCurrentEntity->angles[YAW];
}

void CGameStudioModelRenderer::StudioProcessGait(entity_state_t *pplayer)
{
	mstudioseqdesc_t *pseqdesc;

	CalculateYawBlend(pplayer);
	CalculatePitchBlend(pplayer);


	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + pplayer->gaitsequence;

	if (pseqdesc->linearmovement[0] > 0)
		m_pPlayerInfo->gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	else
	{
		float dt = bound( 0.0, (m_clTime - m_clOldTime), 1.0 );
		m_pPlayerInfo->gaitframe += pseqdesc->fps * dt * m_pCurrentEntity->curstate.framerate;
	}

	m_pPlayerInfo->gaitframe = m_pPlayerInfo->gaitframe - (int)(m_pPlayerInfo->gaitframe / pseqdesc->numframes) * pseqdesc->numframes;

	if (m_pPlayerInfo->gaitframe < 0)
		m_pPlayerInfo->gaitframe += pseqdesc->numframes;
}

void CGameStudioModelRenderer::SavePlayerState(entity_state_t *pplayer)
{
	client_anim_state_t *st;
	cl_entity_t *ent = IEngineStudio.GetCurrentEntity();

	if (!ent)
		return;

	st = &g_state;

	st->angles = ent->curstate.angles;
	st->origin = ent->curstate.origin;

	st->realangles = ent->angles;

	st->sequence = ent->curstate.sequence;
	st->gaitsequence = pplayer->gaitsequence;
	st->animtime = ent->curstate.animtime;
	st->frame = ent->curstate.frame;
	st->framerate = ent->curstate.framerate;

	memcpy(st->blending, ent->curstate.blending, 2);
	memcpy(st->controller, ent->curstate.controller, 4);

	st->lv = ent->latched;
}

void GetSequenceInfo(void *pmodel, client_anim_state_t *pev, float *pflFrameRate, float *pflGroundSpeed)
{
	studiohdr_t *pstudiohdr;
	pstudiohdr = (studiohdr_t *)pmodel;

	if (!pstudiohdr)
		return;

	mstudioseqdesc_t *pseqdesc;

	if (pev->sequence >= pstudiohdr->numseq)
	{
		*pflFrameRate = 0.0;
		*pflGroundSpeed = 0.0;
		return;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + (int)pev->sequence;

	if (pseqdesc->numframes > 1)
	{
		*pflFrameRate = 256 * pseqdesc->fps / (pseqdesc->numframes - 1);
		*pflGroundSpeed = sqrt(pseqdesc->linearmovement[0] * pseqdesc->linearmovement[0] + pseqdesc->linearmovement[1] * pseqdesc->linearmovement[1] + pseqdesc->linearmovement[2] * pseqdesc->linearmovement[2]);
		*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	}
	else
	{
		*pflFrameRate = 256.0;
		*pflGroundSpeed = 0.0;
	}
}

int GetSequenceFlags(void *pmodel, client_anim_state_t *pev)
{
	studiohdr_t *pstudiohdr;
	pstudiohdr = (studiohdr_t *)pmodel;

	if (!pstudiohdr || pev->sequence >= pstudiohdr->numseq)
		return 0;

	mstudioseqdesc_t *pseqdesc;
	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + (int)pev->sequence;

	return pseqdesc->flags;
}

float StudioFrameAdvance(client_anim_state_t *st, float framerate, float flInterval)
{
	if (flInterval == 0.0)
	{
		flInterval = (gEngfuncs.GetClientTime() - st->animtime);

		if (flInterval <= 0.001)
		{
			st->animtime = gEngfuncs.GetClientTime();
			return 0.0;
		}
	}

	if (!st->animtime)
		flInterval = 0.0;

	st->frame += flInterval * framerate * st->framerate;
	st->animtime = gEngfuncs.GetClientTime();

	if (st->frame < 0.0 || st->frame >= 256.0)
	{
		if (st->m_fSequenceLoops)
			st->frame -= (int)(st->frame / 256.0) * 256.0;
		else
			st->frame = (st->frame < 0.0) ? 0 : 255;

		st->m_fSequenceFinished = TRUE;
	}

	return flInterval;
}

void CGameStudioModelRenderer::SetupClientAnimation(entity_state_t *pplayer)
{
	static double oldtime;
	double curtime, dt;

	client_anim_state_t *st;
	float fr, gs;

	cl_entity_t *ent = IEngineStudio.GetCurrentEntity();

	if (!ent)
		return;

	curtime = gEngfuncs.GetClientTime();
	dt = bound( 0.0, (curtime - oldtime), 1.0 );

	oldtime = curtime;
	st = &g_clientstate;

	st->framerate = 1.0;

	int oldseq = st->sequence;
	CounterStrike_GetSequence(&st->sequence, &st->gaitsequence);
	CounterStrike_GetOrientation((float *)&st->origin, (float *)&st->angles);
	VectorCopy(st->angles, st->realangles);

	if (st->sequence != oldseq)
	{
		st->frame = 0.0;
		st->lv.prevsequence = oldseq;
		st->lv.sequencetime = st->animtime;

		memcpy(st->lv.prevseqblending, st->blending, 2);
		memcpy(st->lv.prevcontroller, st->controller, 4);
	}

	void *pmodel = (studiohdr_t *)IEngineStudio.Mod_Extradata(ent->model);

	if( !pmodel )
		return;


	GetSequenceInfo(pmodel, st, &fr, &gs);
	st->m_fSequenceLoops = ((GetSequenceFlags(pmodel, st) & STUDIO_LOOPING) != 0);
	StudioFrameAdvance(st, fr, dt);

	ent->angles = st->realangles;

	ent->curstate.angles = st->angles;
	ent->curstate.origin = st->origin;

	ent->curstate.sequence = st->sequence;
	pplayer->gaitsequence = st->gaitsequence;
	ent->curstate.animtime = st->animtime;
	ent->curstate.frame = st->frame;
	ent->curstate.framerate = st->framerate;

	memcpy(ent->curstate.blending, st->blending, 2);
	memcpy(ent->curstate.controller, st->controller, 4);

	ent->latched = st->lv;
}

void CGameStudioModelRenderer::RestorePlayerState(entity_state_t *pplayer)
{
	client_anim_state_t *st;
	cl_entity_t *ent = IEngineStudio.GetCurrentEntity();

	if (!ent)
		return;

	st = &g_clientstate;

	st->angles = ent->curstate.angles;
	st->origin = ent->curstate.origin;

	st->realangles = ent->angles;

	st->sequence = ent->curstate.sequence;
	st->gaitsequence = pplayer->gaitsequence;
	st->animtime = ent->curstate.animtime;
	st->frame = ent->curstate.frame;
	st->framerate = ent->curstate.framerate;

	memcpy(st->blending, ent->curstate.blending, 2);
	memcpy(st->controller, ent->curstate.controller, 4);

	st->lv = ent->latched;

	st = &g_state;

	ent->angles = st->realangles;

	ent->curstate.angles = st->angles;
	ent->curstate.origin = st->origin;

	ent->curstate.sequence = st->sequence;
	pplayer->gaitsequence = st->gaitsequence;
	ent->curstate.animtime = st->animtime;
	ent->curstate.frame = st->frame;
	ent->curstate.framerate = st->framerate;

	memcpy(ent->curstate.blending, st->blending, 2);
	memcpy(ent->curstate.controller, st->controller, 4);

	ent->latched = st->lv;
}

int CGameStudioModelRenderer::StudioDrawPlayer(int flags, entity_state_t *pplayer)
{
	int iret = 0;
	bool isLocalPlayer = false;

	m_pplayer = pplayer;

	if (m_bLocal && IEngineStudio.GetCurrentEntity() == gEngfuncs.GetLocalPlayer())
		isLocalPlayer = true;

	if (isLocalPlayer)
	{
		SavePlayerState(pplayer);
		SetupClientAnimation(pplayer);
	}

	iret = _StudioDrawPlayer(flags, pplayer);

	if (isLocalPlayer)
		RestorePlayerState(pplayer);

	if( m_pCvarShadows->value != 0.0f )
	{
		Vector chestpos;

		for( int i = 0; i < m_nCachedBones; i++ )
		{
			if( !strcmp(m_nCachedBoneNames[i], "Bip01 Spine3") )
			{
				chestpos.x = m_rgCachedBoneTransform[i][0][3];
				chestpos.y = m_rgCachedBoneTransform[i][1][3];
				chestpos.z = m_rgCachedBoneTransform[i][2][3];
				StudioDrawShadow(chestpos, 20.0f);
				break;
			}
		}
	}

	m_pplayer = NULL;

	return iret;
}

bool WeaponHasAttachments(entity_state_t *pplayer)
{
	studiohdr_t *modelheader = NULL;
	model_t *pweaponmodel;

	if (!pplayer)
		return false;

	pweaponmodel = IEngineStudio.GetModelByIndex(pplayer->weaponmodel);
	modelheader = (studiohdr_t *)IEngineStudio.Mod_Extradata(pweaponmodel);

	if( !modelheader )
		return false;

	return (modelheader->numattachments != 0);
}

void CGameStudioModelRenderer::WriteCurrentHitboxes(void)
{
	int index, i, j;
	studiohdr_t *hdr;
	mstudiobbox_t *hit;

	if (!EspIsOffline())
		return;
	if (!m_pCurrentEntity || !m_pStudioHeader || !m_pbonetransform)
		return;

	index = m_pCurrentEntity->index;
	if (index <= 0 || index > 31)
		return;
	if (g_espSeen & (1u << index))
		return;

	hdr = m_pStudioHeader;
	if (hdr->numhitboxes <= 0)
		return;

	hit = (mstudiobbox_t *)((byte *)hdr + hdr->hitboxindex);
	for (i = 0; i < hdr->numhitboxes && g_espCount < ESP_HB_MAX; i++)
	{
		int bone = hit[i].bone;
		float mins[3], maxs[3];
		float src[8][3];
		float (*mat)[4];
		esp_hb_t *out;

		if (bone < 0 || bone >= MAXSTUDIOBONES)
			continue;

		mins[0] = hit[i].bbmin[0];
		mins[1] = hit[i].bbmin[1];
		mins[2] = hit[i].bbmin[2];
		maxs[0] = hit[i].bbmax[0];
		maxs[1] = hit[i].bbmax[1];
		maxs[2] = hit[i].bbmax[2];

		src[0][0] = mins[0]; src[0][1] = mins[1]; src[0][2] = mins[2];
		src[1][0] = maxs[0]; src[1][1] = mins[1]; src[1][2] = mins[2];
		src[2][0] = maxs[0]; src[2][1] = maxs[1]; src[2][2] = mins[2];
		src[3][0] = mins[0]; src[3][1] = maxs[1]; src[3][2] = mins[2];
		src[4][0] = mins[0]; src[4][1] = mins[1]; src[4][2] = maxs[2];
		src[5][0] = maxs[0]; src[5][1] = mins[1]; src[5][2] = maxs[2];
		src[6][0] = maxs[0]; src[6][1] = maxs[1]; src[6][2] = maxs[2];
		src[7][0] = mins[0]; src[7][1] = maxs[1]; src[7][2] = maxs[2];

		mat = (*m_pbonetransform)[bone];
		out = &g_espBoxes[g_espCount];
		for (j = 0; j < 8; j++)
		{
			out->c[j][0] = src[j][0] * mat[0][0] + src[j][1] * mat[0][1] + src[j][2] * mat[0][2] + mat[0][3];
			out->c[j][1] = src[j][0] * mat[1][0] + src[j][1] * mat[1][1] + src[j][2] * mat[1][2] + mat[1][3];
			out->c[j][2] = src[j][0] * mat[2][0] + src[j][1] * mat[2][1] + src[j][2] * mat[2][2] + mat[2][3];
		}
		out->group = hit[i].group;
		out->player = index;
		g_espBoxPart[g_espCount] = PartFromBone( hdr, bone, hit[i].group );
		g_espCount++;
	}

	g_espSeen |= 1u << index;
}

void CGameStudioModelRenderer::ExportPlayerHitboxes(cl_entity_t *ent)
{
	cl_entity_t *saved;

	if (!ent || !ent->player || !ent->model)
		return;
	if (ent->index <= 0 || ent->index > 31)
		return;
	if (g_espSeen & (1u << ent->index))
		return;

	saved = m_pCurrentEntity;
	m_pCurrentEntity = ent;

	IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
	m_nPlayerIndex = ent->curstate.number - 1;
	if (m_nPlayerIndex < 0 || m_nPlayerIndex >= gEngfuncs.GetMaxClients())
	{
		m_pCurrentEntity = saved;
		return;
	}

	m_pRenderModel = IEngineStudio.SetupPlayerModel(m_nPlayerIndex);
	if (!m_pRenderModel)
		m_pRenderModel = ent->model;
	if (!m_pRenderModel)
	{
		m_pCurrentEntity = saved;
		return;
	}

	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(m_pRenderModel);
	if (!m_pStudioHeader)
	{
		m_pCurrentEntity = saved;
		return;
	}

	IEngineStudio.StudioSetHeader(m_pStudioHeader);
	IEngineStudio.SetRenderModel(m_pRenderModel);

	if (m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq)
		m_pCurrentEntity->curstate.sequence = 0;

	m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);
	if (m_pPlayerInfo)
		m_pPlayerInfo->gaitsequence = ent->curstate.gaitsequence;

	StudioSetUpTransform(0);
	if (m_pPlayerInfo)
		StudioSetupBones();
	m_pPlayerInfo = NULL;

	WriteCurrentHitboxes();
	m_pCurrentEntity = saved;
}

int CGameStudioModelRenderer::_StudioDrawPlayer(int flags, entity_state_t *pplayer)
{
	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();

	IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
	IEngineStudio.GetViewInfo(m_vRenderOrigin, m_vUp, m_vRight, m_vNormal);
	IEngineStudio.GetAliasScale(&m_fSoftwareXScale, &m_fSoftwareYScale);

	m_nPlayerIndex = pplayer->number - 1;

	if (m_nPlayerIndex < 0 || m_nPlayerIndex >= gEngfuncs.GetMaxClients())
		return 0;

	/*m_pRenderModel = IEngineStudio.SetupPlayerModel(m_nPlayerIndex);

	if (m_pRenderModel == NULL)
		return 0;*/

	extra_player_info_t *pExtra = g_PlayerExtraInfo + pplayer->number;

	if( gHUD.cl_minmodels && gHUD.cl_minmodels->value )
	{
		int team = pExtra->teamnumber;
		if( team == TEAM_TERRORIST )
		{
			// set leet if model isn't valid
			int modelIdx = gHUD.cl_min_t && BIsValidTModelIndex(gHUD.cl_min_t->value) ? gHUD.cl_min_t->value : 1;

			m_pRenderModel = gEngfuncs.CL_LoadModel( sPlayerModelFiles[ modelIdx ], NULL );
		}
		else if( team == TEAM_CT )
		{
			if( pExtra->vip )
				m_pRenderModel = gEngfuncs.CL_LoadModel( sPlayerModelFiles[3], NULL );
			else
			{
				// set gign, if model isn't valud
				int modelIdx = gHUD.cl_min_ct && BIsValidCTModelIndex(gHUD.cl_min_ct->value) ? gHUD.cl_min_ct->value : 2;

				m_pRenderModel = gEngfuncs.CL_LoadModel( sPlayerModelFiles[ modelIdx ], NULL );
			}
		}
	}
	else
	{
		m_pRenderModel = IEngineStudio.SetupPlayerModel( m_nPlayerIndex );
	}

	if( !m_pRenderModel )
	{
		return 0;
	}

	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(m_pRenderModel);

	if( !m_pStudioHeader )
		return 0;

	IEngineStudio.StudioSetHeader(m_pStudioHeader);
	IEngineStudio.SetRenderModel(m_pRenderModel);

	if (m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq)
		m_pCurrentEntity->curstate.sequence = 0;

	if (pplayer->sequence >= m_pStudioHeader->numseq)
		pplayer->sequence = 0;

	if (m_pCurrentEntity->curstate.gaitsequence >= m_pStudioHeader->numseq)
		m_pCurrentEntity->curstate.gaitsequence = 0;

	if (pplayer->gaitsequence >= m_pStudioHeader->numseq)
		pplayer->gaitsequence = 0;

	if (pplayer->gaitsequence)
	{
		vec3_t orig_angles(m_pCurrentEntity->angles);
		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

		StudioProcessGait(pplayer);

		m_pPlayerInfo->gaitsequence = pplayer->gaitsequence;
		m_pPlayerInfo = NULL;

		StudioSetUpTransform(0);
		m_pCurrentEntity->angles = orig_angles;
	}
	else
	{
		m_pCurrentEntity->curstate.controller[0] = 127;
		m_pCurrentEntity->curstate.controller[1] = 127;
		m_pCurrentEntity->curstate.controller[2] = 127;
		m_pCurrentEntity->curstate.controller[3] = 127;
		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];

		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

		CalculatePitchBlend(pplayer);
		CalculateYawBlend(pplayer);

		m_pPlayerInfo->gaitsequence = 0;
		StudioSetUpTransform(0);
	}

	if (flags & STUDIO_RENDER)
	{
		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++;

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

	StudioSetupBones();
	StudioSaveBones();
	WriteCurrentHitboxes();

	m_pPlayerInfo->renderframe = m_nFrameCount;
	m_pPlayerInfo = NULL;

	if (flags & STUDIO_EVENTS && (!(flags & STUDIO_RENDER) || !pplayer->weaponmodel || !WeaponHasAttachments(pplayer)))
	{
		StudioCalcAttachments();
		IEngineStudio.StudioClientEvents();

		if (m_pCurrentEntity->index > 0)
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index);
			memcpy(ent->attachment, m_pCurrentEntity->attachment, sizeof(vec3_t) * 4);
		}
	}

	if (flags & STUDIO_RENDER)
	{
		alight_t lighting;
		vec3_t dir;

		lighting.plightvec = dir;

		IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting);
		IEngineStudio.StudioEntityLight(&lighting);
		IEngineStudio.StudioSetupLighting(&lighting);

		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);
		m_nTopColor = m_pPlayerInfo->topcolor;

		if (m_nTopColor < 0) m_nTopColor = 0;
		if (m_nTopColor > 360) m_nTopColor = 360;

		m_nBottomColor = m_pPlayerInfo->bottomcolor;

		// ====================== CLEAN VISIBILITY: PLAYER SILHOUETTE + HITBOX OUTLINES ======================
		// 3D outlines drawn only for visible players (respects walls, depth, frustum). Legitimate visibility aid only.
		// Silhouette: polygon offset cyan "edge highlight" around the actual model.
		// Hitboxes: precise wireframe (head bright, body subtle). Head-only default for clean look.
		// Performance: distance culling + early outs.

		cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
		bool isOtherPlayer = (m_pCurrentEntity->player && localPlayer &&
		                      m_pCurrentEntity->index != localPlayer->index);

		float playerDistSq = 0.0f;
		bool withinDrawDist = true;
		if (isOtherPlayer && localPlayer)
		{
			Vector delta = m_pCurrentEntity->origin - localPlayer->origin;
			playerDistSq = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z; // LengthSqr equivalent
		}

		// Silhouette outline (cyan edge) - only if enabled and close enough
		if (cl_player_outline && cl_player_outline->value > 0.0f && isOtherPlayer)
		{
			// Reuse the hitbox max dist cvar (or slightly more generous default) for silhouette
			float maxSilDist = cl_hitbox_outline_max_dist ? cl_hitbox_outline_max_dist->value * 1.4f : 0.0f;
			if (maxSilDist <= 0.0f || playerDistSq <= (maxSilDist * maxSilDist))
			{
				int origRenderFx = m_pCurrentEntity->curstate.renderfx;
				int origRenderMode = m_pCurrentEntity->curstate.rendermode;
				int origRenderAmt = m_pCurrentEntity->curstate.renderamt;
				color24 origColor = m_pCurrentEntity->curstate.rendercolor;

				m_pCurrentEntity->curstate.renderfx = kRenderFxNone;
				m_pCurrentEntity->curstate.rendermode = kRenderNormal;
				m_pCurrentEntity->curstate.renderamt = 255;
				m_pCurrentEntity->curstate.rendercolor.r = 0;
				m_pCurrentEntity->curstate.rendercolor.g = 255;
				m_pCurrentEntity->curstate.rendercolor.b = 210;  // cyan

				glPushAttrib(GL_POLYGON_BIT);
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(1.5f, 4.0f);

				StudioRenderModel(dir);  // offset cyan pass (visible surfaces only)

				glPopAttrib();

				// restore state
				m_pCurrentEntity->curstate.renderfx = origRenderFx;
				m_pCurrentEntity->curstate.rendermode = origRenderMode;
				m_pCurrentEntity->curstate.renderamt = origRenderAmt;
				m_pCurrentEntity->curstate.rendercolor = origColor;
			}
		}

		if (m_nBottomColor < 0) m_nBottomColor = 0;
		if (m_nBottomColor > 360) m_nBottomColor = 360;

		IEngineStudio.StudioSetRemapColors(m_nTopColor, m_nBottomColor);

		StudioRenderModel(dir);   // normal textured render on top

		// Native ESP: same bones as the model, drawn without the depth test so the
		// wireframe stays visible through walls. Offline listen-server only.
		// (MODIFICADO: EspIsOffline() agora sempre retorna 1; basta cl_esp)
		if (EspIsOffline() && cl_esp && cl_esp->value > 1.0f)
		{
			mstudiobbox_t *pHitbox = (mstudiobbox_t *)((byte *)m_pStudioHeader + m_pStudioHeader->hitboxindex);
			bool headOnly = cl_esp->value >= 2.0f;
			GLboolean depthWas = glIsEnabled(GL_DEPTH_TEST);

			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			for (int i = 0; i < m_pStudioHeader->numhitboxes; i++)
			{
				bool isHead = (pHitbox[i].group == STUDIO_HITGROUP_HEAD);
				if (headOnly && !isHead)
					continue;

				int bone = pHitbox[i].bone;
				if (bone < 0 || bone >= MAXSTUDIOBONES)
					continue;

				float mins[3] = { pHitbox[i].bbmin[0], pHitbox[i].bbmin[1], pHitbox[i].bbmin[2] };
				float maxs[3] = { pHitbox[i].bbmax[0], pHitbox[i].bbmax[1], pHitbox[i].bbmax[2] };
				Vector corners[8] = {
					{mins[0], mins[1], mins[2]},
					{maxs[0], mins[1], mins[2]},
					{maxs[0], maxs[1], mins[2]},
					{mins[0], maxs[1], mins[2]},
					{mins[0], mins[1], maxs[2]},
					{maxs[0], mins[1], maxs[2]},
					{maxs[0], maxs[1], maxs[2]},
					{mins[0], maxs[1], maxs[2]}
				};

				float (*mat)[4] = (*m_pbonetransform)[bone];
				for (int j = 0; j < 8; j++)
				{
					Vector src = corners[j];
					corners[j][0] = src[0]*mat[0][0] + src[1]*mat[0][1] + src[2]*mat[0][2] + mat[0][3];
					corners[j][1] = src[0]*mat[1][0] + src[1]*mat[1][1] + src[2]*mat[1][2] + mat[1][3];
					corners[j][2] = src[0]*mat[2][0] + src[1]*mat[2][1] + src[2]*mat[2][2] + mat[2][3];
				}

				glLineWidth(isHead ? 2.0f : 1.0f);
				gEngfuncs.pTriAPI->Begin(TRI_LINES);
				if (isHead)
					gEngfuncs.pTriAPI->Color4ub(80, 255, 255, 255);
				else
					gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 220);
				DrawStudioHitboxEdges(corners);
				gEngfuncs.pTriAPI->End();

				if (isHead)
				{
					Vector mid(0, 0, 0);
					for (int j = 0; j < 8; j++)
						mid = mid + corners[j];
					mid = mid * 0.125f;
					glPointSize(6.0f);
					gEngfuncs.pTriAPI->Begin(TRI_POINTS);
					gEngfuncs.pTriAPI->Color4ub(80, 255, 255, 255);
					gEngfuncs.pTriAPI->Vertex3fv(mid);
					gEngfuncs.pTriAPI->End();
					glPointSize(1.0f);
				}
			}

			glDepthMask(GL_TRUE);
			if (depthWas)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
			glPopAttrib();
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
		}

		// Hitbox outlines (wireframe for aim practice) - clean, head-only by default
		if (cl_hitbox_outline && cl_hitbox_outline->value > 0.0f && isOtherPlayer)
		{
			float maxDist = cl_hitbox_outline_max_dist ? cl_hitbox_outline_max_dist->value : 0.0f;
			if (maxDist <= 0.0f || playerDistSq <= (maxDist * maxDist))
			{
				studiohdr_t *pStudioHdr = m_pStudioHeader;
				if (pStudioHdr && pStudioHdr->numhitboxes > 0 && m_pbonetransform)
				{
					mstudiobbox_t *pHitbox = (mstudiobbox_t *)((byte *)pStudioHdr + pStudioHdr->hitboxindex);

					gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
					glPushAttrib( GL_LINE_BIT );

					for (int i = 0; i < pStudioHdr->numhitboxes; i++)
					{
						bool isHead = (pHitbox[i].group == STUDIO_HITGROUP_HEAD);

						if (cl_hitbox_outline_head_only && cl_hitbox_outline_head_only->value > 0.0f && !isHead)
							continue;

						int bone = pHitbox[i].bone;
						if (bone < 0 || bone >= MAXSTUDIOBONES) continue;

						float mins[3] = { pHitbox[i].bbmin[0], pHitbox[i].bbmin[1], pHitbox[i].bbmin[2] };
						float maxs[3] = { pHitbox[i].bbmax[0], pHitbox[i].bbmax[1], pHitbox[i].bbmax[2] };

						if (isHead)
						{
							for (int axis = 0; axis < 3; axis++)
							{
								float center = (mins[axis] + maxs[axis]) * 0.5f;
								float halfSize = (maxs[axis] - mins[axis]) * 0.6f; // 20% larger than the model hitbox
								mins[axis] = center - halfSize;
								maxs[axis] = center + halfSize;
							}
						}

						Vector corners[8] = {
							{mins[0], mins[1], mins[2]},
							{maxs[0], mins[1], mins[2]},
							{maxs[0], maxs[1], mins[2]},
							{mins[0], maxs[1], mins[2]},
							{mins[0], mins[1], maxs[2]},
							{maxs[0], mins[1], maxs[2]},
							{maxs[0], maxs[1], maxs[2]},
							{mins[0], maxs[1], maxs[2]}
						};

						float (*mat)[4] = (*m_pbonetransform)[bone];
						for (int j = 0; j < 8; j++)
						{
							Vector &src = corners[j];
							Vector out;
							out[0] = src[0]*mat[0][0] + src[1]*mat[0][1] + src[2]*mat[0][2] + mat[0][3];
							out[1] = src[0]*mat[1][0] + src[1]*mat[1][1] + src[2]*mat[1][2] + mat[1][3];
							out[2] = src[0]*mat[2][0] + src[1]*mat[2][1] + src[2]*mat[2][2] + mat[2][3];
							corners[j] = out;
						}

						if (isHead)
						{
							glLineWidth( 6.0f );
							gEngfuncs.pTriAPI->Begin( TRI_LINES );
							gEngfuncs.pTriAPI->Color4ub( 0, 255, 255, 120 );
							DrawStudioHitboxEdges( corners );
							gEngfuncs.pTriAPI->End();

							glLineWidth( 3.0f );
							gEngfuncs.pTriAPI->Begin( TRI_LINES );
							gEngfuncs.pTriAPI->Color4ub( 140, 255, 255, 255 );
							DrawStudioHitboxEdges( corners );
							gEngfuncs.pTriAPI->End();
						}
						else
						{
							glLineWidth( 1.0f );
							gEngfuncs.pTriAPI->Begin( TRI_LINES );
							gEngfuncs.pTriAPI->Color4ub( 180, 180, 180, 70 );
							DrawStudioHitboxEdges( corners );
							gEngfuncs.pTriAPI->End();
						}
					}

					glPopAttrib();
					gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
				}
			}
		}

		// Note: 3D silhouette + hitbox outlines implemented directly here (inside visible model render pass)
		// for perfect depth adherence ("on the model, not through walls"). Canvas 2D available for HUD overlays.

		// Note: 3D silhouette + hitbox outlines are implemented directly here (inside visible model render)
		// for perfect depth adherence ("on the model, not through walls"). Canvas 2D is available
		// for other HUD / projected overlays when the engine Canvas ABI is wired in.

		m_pPlayerInfo = NULL;

		if (pplayer->weaponmodel)
		{
			studiohdr_t *saveheader = m_pStudioHeader;
			cl_entity_t saveent = *m_pCurrentEntity;

			model_t *pweaponmodel = IEngineStudio.GetModelByIndex(pplayer->weaponmodel);

			m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(pweaponmodel);
			if( !m_pStudioHeader )
				return 0;

			IEngineStudio.StudioSetHeader(m_pStudioHeader);

			StudioMergeBones(pweaponmodel);

			IEngineStudio.StudioSetupLighting(&lighting);

			StudioRenderModel(dir);

			StudioCalcAttachments();

			if (m_pCurrentEntity->index > 0)
				memcpy(saveent.attachment, m_pCurrentEntity->attachment, sizeof(vec3_t) * m_pStudioHeader->numattachments);

			*m_pCurrentEntity = saveent;
			m_pStudioHeader = saveheader;
			IEngineStudio.StudioSetHeader(m_pStudioHeader);

			if (flags & STUDIO_EVENTS)
				IEngineStudio.StudioClientEvents();
		}
	}

	return 1;
}


void CGameStudioModelRenderer::StudioFxTransform(cl_entity_t *ent, float transform[3][4])
{
	switch (ent->curstate.renderfx)
	{
	case kRenderFxDistort:
	case kRenderFxHologram:
	{
		if (Com_RandomLong(0, 49) == 0)
		{
			int axis = Com_RandomLong(0, 1);

			if (axis == 1)
				axis = 2;

			VectorScale( transform[axis], gEngfuncs.pfnRandomFloat( 1, 1.484 ), transform[axis] );
		}
		else if (Com_RandomLong(0, 49) == 0)
		{
			float offset;

			offset = gEngfuncs.pfnRandomFloat(-10, 10);
			transform[Com_RandomLong(0, 2)][3] += offset;
		}

		break;
	}

	case kRenderFxExplode:
	{
		if (iRenderStateChanged)
		{
			g_flStartScaleTime = m_clTime;
			iRenderStateChanged = FALSE;
		}

		float flTimeDelta = m_clTime - g_flStartScaleTime;

		if (flTimeDelta > 0)
		{
			float flScale = 0.001;

			if (flTimeDelta <= 2.0)
				flScale = 1.0 - (flTimeDelta / 2.0);

			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
					transform[i][j] *= flScale;
			}
		}

		break;
	}
	}
}

void R_StudioInit(void)
{
	g_StudioRenderer.Init();
}

int R_StudioDrawPlayer(int flags, entity_state_t *pplayer)
{
#ifndef NDEBUG
	if( g_StudioRenderer.m_pCvarDebug->value >= 8 )
	{
			cl_entity_t *pCurrentEntity = IEngineStudio.GetCurrentEntity();
			int ret = 0;

			if( pCurrentEntity )
			{
					cvar_t *drawEntites = g_StudioRenderer.m_pCvarDrawEntities;
					g_StudioRenderer.m_pCvarDebug->value -= 8;
					g_StudioRenderer.m_pCvarDrawEntities = g_StudioRenderer.m_pCvarDebug;

					// first draw interpolated
					ret = g_StudioRenderer.StudioDrawPlayer( flags, pplayer );

					// then draw non-interpolated
					/*{
						Vector saveOrigin = pCurrentEntity->origin;
						int savefx = pCurrentEntity->curstate.renderfx;
						int saveamt = pCurrentEntity->curstate.renderamt;
						color24 savecolor = pCurrentEntity->curstate.rendercolor;

						pCurrentEntity->curstate.renderfx = kRenderFxGlowShell;
						pCurrentEntity->curstate.rendercolor.r = 0;
						pCurrentEntity->curstate.rendercolor.g = 0;
						pCurrentEntity->curstate.rendercolor.b = 255;
						pCurrentEntity->curstate.renderamt = 255;
						pCurrentEntity->origin = pCurrentEntity->curstate.origin;

						g_StudioRenderer.StudioDrawPlayer( flags, pplayer );
						pCurrentEntity->origin = saveOrigin;
						pCurrentEntity->curstate.renderfx = savefx;
						pCurrentEntity->curstate.renderamt   = saveamt;
						pCurrentEntity->curstate.rendercolor = savecolor;
					}

					// then draw non-interpolated
					{
						Vector saveOrigin = pCurrentEntity->origin;
						int savefx = pCurrentEntity->curstate.renderfx;
						int saveamt = pCurrentEntity->curstate.renderamt;
						color24 savecolor = pCurrentEntity->curstate.rendercolor;

						pCurrentEntity->curstate.renderfx = kRenderFxGlowShell;
						pCurrentEntity->curstate.rendercolor.r = 255;
						pCurrentEntity->curstate.rendercolor.g = 0;
						pCurrentEntity->curstate.rendercolor.b = 0;
						pCurrentEntity->curstate.renderamt = 255;
						pCurrentEntity->origin = pCurrentEntity->prevstate.origin;

						g_StudioRenderer.StudioDrawPlayer( flags, pplayer );
						pCurrentEntity->origin = saveOrigin;
						pCurrentEntity->curstate.renderfx    = savefx;
						pCurrentEntity->curstate.renderamt   = saveamt;
						pCurrentEntity->curstate.rendercolor = savecolor;
					}*/


					g_StudioRenderer.m_pCvarDrawEntities = drawEntites;
					g_StudioRenderer.m_pCvarDebug->value += 8;
			}

			return ret;
	}
	else
#endif
	return g_StudioRenderer.StudioDrawPlayer(flags, pplayer);
}

int R_StudioDrawModel(int flags)
{
	return g_StudioRenderer.StudioDrawModel(flags);
}
// The simple drawing interface we'll pass back to the engine
r_studio_interface_t studio =
{
	STUDIO_INTERFACE_VERSION,
	R_StudioDrawModel,
	R_StudioDrawPlayer,
};

/*
====================
HUD_GetStudioModelInterface
Export this function for the engine to use the studio renderer class to render objects.
====================
*/
int DLLEXPORT HUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio )
{
	if ( version != STUDIO_INTERFACE_VERSION )
		return 0;

	// Point the engine to our callbacks
	*ppinterface = &studio;

	// Copy in engine helper functions
	IEngineStudio = *pstudio;

	// Initialize local variables, etc.
	R_StudioInit();

	// Success
	return 1;
}

void DrawTransparentTriangles( void );

void CHud::DrawTransparentTriangles( void )
{
	// Visibility features (player silhouette + hitbox outlines) are rendered inside
	// the Studio model path (GameStudioModelRenderer) for correct depth and wall respect.

	// === Canvas 2D example (engine Canvas API) ===
	// To use 2D drawing (HUD overlays, projected outlines, etc.), do this:
	//
	//   #include "engine_canvas_api.h"
	//   canvas_t *c = ...; // supplied by the engine/renderer integration point
	//   if (c && c->BeginFrame) {
	//       c->BeginFrame( ScreenWidth, ScreenHeight );
	//       c->SetColor(255, 255, 0, 200);
	//       c->Line(100, 100, 200, 150, 2.0f);
	//       c->EndFrame();
	//   }
	//
	// Note: Call while 2D projection is active (usually safe in HUD_Redraw or here).
	// The local gCanvas (C++) wrapper is also available as an alternative.
}

// ====================== WRAPPER GLOBAL OBRIGATÓRIO PARA O XASH/CS16Client ======================
// O engine do Xash3D procura essa função GLOBAL (não a da classe CHud).
void DrawTransparentTriangles(void)
{
    // não desenha se HUD estiver escondido
    if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
        return;

    gHUD.DrawTransparentTriangles();
}
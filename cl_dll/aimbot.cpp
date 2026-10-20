/*
 * aimbot.cpp - mira/trigger nativos do client.
 *
 * Reescrito a partir do zero para trazer as opcoes do Sakura para dentro do
 * engine: alvo por hitbox (20 partes), varios modos de smooth, silent e
 * perfect silent por comando, recoil compensation, reaction time, flash
 * check, triggerbot com burst e accuracy boost.
 *
 * Fluxo:
 *   HUD_PostRunCmd -> Aimbot_PostRunCmd  (punchangle, arma, municao, seed)
 *   CL_CreateMove  -> Aimbot_Apply       (escolhe alvo e escreve viewangles
 *                                         e/ou cmd->viewangles)
 *   HUD_Redraw     -> Aimbot_Draw        (circulo de FOV, caixa do alvo)
 */

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "pm_defs.h"
#include "in_buttons.h"
#include "keydefs.h"
#include "entity_state.h"
#include "usercmd.h"
#include "com_weapons.h"
#include "triangleapi.h"
#include "draw_util.h"
#include "aimbot.h"
#include "backtrack.h"
#include "spread_predictor.h"
#include "imgui.h"
#include "imgui_host.h"
#include "imgui_menu.h"
#include "ui/hub_menu.h"

#define AIM_PI       3.14159265358979323846f
#define AIM_DEG2RAD  ( AIM_PI / 180.0f )
#define AIM_RAD2DEG  ( 180.0f / AIM_PI )
#define AIM_BOX_MAX  384

/* ids de arma do client (dlls/weapontype.h) */
#define AIM_WEAPON_SCOUT     3
#define AIM_WEAPON_AUG       8
#define AIM_WEAPON_SG550     13
#define AIM_WEAPON_AWP       18
#define AIM_WEAPON_G3SG1     24
#define AIM_WEAPON_SG552     27
#define AIM_WEAPON_KNIFE     29
#define AIM_WEAPON_C4        6
#define AIM_WEAPON_HEGRENADE 4
#define AIM_WEAPON_FLASHBANG 25
#define AIM_WEAPON_SMOKE     9

extern int g_iTeamNumber;
extern int g_iUser1;
extern cvar_t *cl_aim_assist;
extern cvar_t *cl_aim_smooth;
extern cvar_t *cl_aim_fov;

void VectorAngles( const float *forward, float *angles );

/* ------------------------------------------------------------------ */
/* estado                                                             */
/* ------------------------------------------------------------------ */

static aim_box_t g_boxes[AIM_BOX_MAX];
static int       g_boxCount;
static double    g_boxTime;

static float g_mouse_pitch;
static float g_mouse_yaw;

static double g_lastCmdTime;
static double g_lastPredTime;

/* estado da predicao local */
static float  g_punch[3];
static int    g_weaponId;
static int    g_clip;
static int    g_inReload;
static int    g_inZoom;
static int    g_shots;
static int    g_shotsPrev;
static float  g_nextAttack;
static float  g_nextPrimary;
static float  g_velocity[3];
static int    g_flags;
static unsigned int g_seed;
static int    g_health;

/* alvo do frame */
static int   g_target;
static int   g_targetPart;
static int   g_targetPartMode = 0;   /* AIM_PART_MODE_AIM */
static float g_targetAim[3];
static float g_targetAngle;
static float g_targetDist;
static double g_acquireTime;
static int   g_prevTarget;
static double g_blockAttackUntil;

/* triggerbot */
static int    g_trigTarget;
static double g_trigAcquire;
static double g_trigNextShot;
static int    g_trigFired;
static int    g_triggerState;

/* tecla de mira / captura de bind */
static int   g_keyHeld;
static int   g_captureKey;

/* ------------------------------------------------------------------ */
/* cvars                                                              */
/* ------------------------------------------------------------------ */

static cvar_t *cv_team;
static cvar_t *cv_walls;
static cvar_t *cv_draw;
static cvar_t *cv_drawfov;
static cvar_t *cv_mode;       /* 0 = sempre, 1 = so ao atirar, 2 = tecla */
static cvar_t *cv_key;
static cvar_t *cv_smooth_mode;
static cvar_t *cv_auto_speed;
static cvar_t *cv_reaction;
static cvar_t *cv_speed_fov;
static cvar_t *cv_humanize;
static cvar_t *cv_spiral_x;
static cvar_t *cv_spiral_y;
static cvar_t *cv_blockkill;
static cvar_t *cv_flashcheck;
static cvar_t *cv_flash_pct;
static cvar_t *cv_fastzoom;
static cvar_t *cv_hit_mode;
static cvar_t *cv_debug;

static cvar_t *cv_silent;
static cvar_t *cv_psilent;
static cvar_t *cv_psilent_fov;
static cvar_t *cv_psilent_spread;

static cvar_t *cv_rc_pitch;
static cvar_t *cv_rc_yaw;
static cvar_t *cv_rc_fov;
static cvar_t *cv_rc_smooth;
static cvar_t *cv_rc_after;
static cvar_t *cv_accuracy;

static cvar_t *cv_trig_enable;
static cvar_t *cv_trig_team;
static cvar_t *cv_trig_wall;
static cvar_t *cv_trig_zoom;
static cvar_t *cv_trig_delay;
static cvar_t *cv_trig_fire;
static cvar_t *cv_trig_mode;   /* 0 = count, 1 = random */
static cvar_t *cv_trig_count;
static cvar_t *cv_trig_randmax;
static cvar_t *cv_trig_fov;
static cvar_t *cv_trig_draw;
static cvar_t *cv_trig_accuracy;

/* perfis de hitbox: cada modo de mira tem o seu conjunto de partes */
enum
{
	AIM_PART_MODE_AIM = 0,
	AIM_PART_MODE_SILENT,
	AIM_PART_MODE_PSILENT,
	AIM_PART_MODE_COUNT
};

static cvar_t *cv_part[AIM_PART_MODE_COUNT][AIM_PART_COUNT];

/* compatibilidade com os cvars antigos (cl_aim_head, cl_aim_chest, ...) */
static cvar_t *cv_old_head;
static cvar_t *cv_old_chest;
static cvar_t *cv_old_stomach;
static cvar_t *cv_old_arms;
static cvar_t *cv_old_legs;
static cvar_t *cv_old_body;

static const char *kPartSuffix[AIM_PART_COUNT] =
{
	"belt",
	"lthigh", "ltibia", "lfoot",
	"rthigh", "rtibia", "rfoot",
	"stomach", "lchest", "uchest",
	"neck", "head",
	"lshoulder", "lelbow", "larm", "lhand",
	"rshoulder", "relbow", "rarm", "rhand",
	"generic"
};

static const char *kPartPrefix[AIM_PART_MODE_COUNT] = { "cl_aim_", "cl_silent_", "cl_psilent_" };

static const char *kPartLabel[AIM_PART_COUNT] =
{
	"Cintura", "Coxa esquerda", "Tibia esquerda", "Pe esquerdo",
	"Coxa direita", "Tibia direita", "Pe direito",
	"Barriga", "Peito inferior", "Peito superior",
	"Pescoco", "Cabeca",
	"Ombro esquerdo", "Cotovelo esquerdo", "Braco esquerdo", "Mao esquerda",
	"Ombro direito", "Cotovelo direito", "Braco direito", "Mao direita",
	"Resto do corpo"
};

/* prioridade: menor = preferido quando cl_aim_hit_mode = 0 */
static const int kPartRank[AIM_PART_COUNT] =
{
	5, 8, 9, 10, 8, 9, 10, 4, 2, 1, 0, 0, 3, 6, 6, 7, 3, 6, 6, 7, 11
};

/* ------------------------------------------------------------------ */
/* utilidades                                                         */
/* ------------------------------------------------------------------ */

static int On( cvar_t *cv, int fallback )
{
	if( !cv )
		return fallback;
	return cv->value > 0.5f;
}

static float CV( cvar_t *cv, float fallback )
{
	return cv ? cv->value : fallback;
}

static float NormAngle( float a )
{
	while( a > 180.f ) a -= 360.f;
	while( a < -180.f ) a += 360.f;
	return a;
}

static float Clampf( float v, float lo, float hi )
{
	if( v < lo ) return lo;
	if( v > hi ) return hi;
	return v;
}

static void SetCvar( const char *name, float v )
{
	gEngfuncs.Cvar_SetValue( name, v );
}

static int GunWeapon( int id )
{
	switch( id )
	{
	case AIM_WEAPON_KNIFE:
	case AIM_WEAPON_C4:
	case AIM_WEAPON_HEGRENADE:
	case AIM_WEAPON_FLASHBANG:
	case AIM_WEAPON_SMOKE:
		return 0;
	}
	return id > 0 && id < 32;
}

static int SniperWeapon( int id )
{
	return id == AIM_WEAPON_SCOUT || id == AIM_WEAPON_AWP ||
		id == AIM_WEAPON_G3SG1 || id == AIM_WEAPON_SG550 ||
		id == AIM_WEAPON_AUG || id == AIM_WEAPON_SG552;
}

/* WorldToScreen pela engine. 1 se estiver na tela. */
static int ToScreen( const float *world, float *x, float *y )
{
	float tmp[3];
	tmp[0] = world[0];
	tmp[1] = world[1];
	tmp[2] = world[2];
	return gEngfuncs.pTriAPI->WorldToScreen( tmp, tmp ) == 0 ? 0 : 1;
}

/* ------------------------------------------------------------------ */
/* init                                                               */
void Aimbot_Init( void )
{
	int i;

	cl_aim_assist = gEngfuncs.pfnRegisterVariable( "cl_aim_assist", "0", FCVAR_ARCHIVE );
	cl_aim_fov    = gEngfuncs.pfnRegisterVariable( "cl_aim_fov", "12", FCVAR_ARCHIVE );
	cl_aim_smooth = gEngfuncs.pfnRegisterVariable( "cl_aim_smooth", "8", FCVAR_ARCHIVE );

	cv_team        = gEngfuncs.pfnRegisterVariable( "cl_aim_team", "0", FCVAR_ARCHIVE );
	cv_walls       = gEngfuncs.pfnRegisterVariable( "cl_aim_walls", "0", FCVAR_ARCHIVE );
	cv_draw        = gEngfuncs.pfnRegisterVariable( "cl_aim_draw", "0", FCVAR_ARCHIVE );
	cv_drawfov     = gEngfuncs.pfnRegisterVariable( "cl_aim_drawfov", "1", FCVAR_ARCHIVE );
	cv_mode        = gEngfuncs.pfnRegisterVariable( "cl_aim_mode", "1", FCVAR_ARCHIVE );
	cv_key         = gEngfuncs.pfnRegisterVariable( "cl_aim_key", "0", FCVAR_ARCHIVE );
	cv_smooth_mode = gEngfuncs.pfnRegisterVariable( "cl_aim_smooth_mode", "1", FCVAR_ARCHIVE );
	cv_auto_speed  = gEngfuncs.pfnRegisterVariable( "cl_aim_auto_speed", "0", FCVAR_ARCHIVE );
	cv_reaction    = gEngfuncs.pfnRegisterVariable( "cl_aim_reaction", "0", FCVAR_ARCHIVE );
	cv_speed_fov   = gEngfuncs.pfnRegisterVariable( "cl_aim_speed_fov", "0", FCVAR_ARCHIVE );
	cv_humanize    = gEngfuncs.pfnRegisterVariable( "cl_aim_humanize", "0", FCVAR_ARCHIVE );
	cv_spiral_x    = gEngfuncs.pfnRegisterVariable( "cl_aim_spiral_x", "1.3", FCVAR_ARCHIVE );
	cv_spiral_y    = gEngfuncs.pfnRegisterVariable( "cl_aim_spiral_y", "3.7", FCVAR_ARCHIVE );
	cv_blockkill   = gEngfuncs.pfnRegisterVariable( "cl_aim_blockkill", "0", FCVAR_ARCHIVE );
	cv_flashcheck  = gEngfuncs.pfnRegisterVariable( "cl_aim_flashcheck", "0", FCVAR_ARCHIVE );
	cv_flash_pct   = gEngfuncs.pfnRegisterVariable( "cl_aim_flash_pct", "30", FCVAR_ARCHIVE );
	cv_fastzoom    = gEngfuncs.pfnRegisterVariable( "cl_aim_fastzoom", "0", FCVAR_ARCHIVE );
	cv_hit_mode    = gEngfuncs.pfnRegisterVariable( "cl_aim_hit_mode", "0", FCVAR_ARCHIVE );
	cv_debug       = gEngfuncs.pfnRegisterVariable( "cl_aim_debug", "0", FCVAR_ARCHIVE );

	cv_silent        = gEngfuncs.pfnRegisterVariable( "cl_aim_silent", "0", FCVAR_ARCHIVE );
	cv_psilent       = gEngfuncs.pfnRegisterVariable( "cl_aim_psilent", "0", FCVAR_ARCHIVE );
	cv_psilent_fov   = gEngfuncs.pfnRegisterVariable( "cl_aim_psilent_fov", "1.5", FCVAR_ARCHIVE );
	cv_psilent_spread= gEngfuncs.pfnRegisterVariable( "cl_aim_psilent_spread", "0", FCVAR_ARCHIVE );

	cv_rc_pitch  = gEngfuncs.pfnRegisterVariable( "cl_aim_rc_pitch", "0", FCVAR_ARCHIVE );
	cv_rc_yaw    = gEngfuncs.pfnRegisterVariable( "cl_aim_rc_yaw", "0", FCVAR_ARCHIVE );
	cv_rc_fov    = gEngfuncs.pfnRegisterVariable( "cl_aim_rc_fov", "0", FCVAR_ARCHIVE );
	cv_rc_smooth = gEngfuncs.pfnRegisterVariable( "cl_aim_rc_smooth", "0", FCVAR_ARCHIVE );
	cv_rc_after  = gEngfuncs.pfnRegisterVariable( "cl_aim_rc_after", "0", FCVAR_ARCHIVE );
	cv_accuracy  = gEngfuncs.pfnRegisterVariable( "cl_aim_accuracy", "0", FCVAR_ARCHIVE );

	cv_trig_enable  = gEngfuncs.pfnRegisterVariable( "cl_trig_enable", "0", FCVAR_ARCHIVE );
	cv_trig_team    = gEngfuncs.pfnRegisterVariable( "cl_trig_team", "0", FCVAR_ARCHIVE );
	cv_trig_wall    = gEngfuncs.pfnRegisterVariable( "cl_trig_wall", "0", FCVAR_ARCHIVE );
	cv_trig_zoom    = gEngfuncs.pfnRegisterVariable( "cl_trig_zoom", "0", FCVAR_ARCHIVE );
	cv_trig_delay   = gEngfuncs.pfnRegisterVariable( "cl_trig_delay", "60", FCVAR_ARCHIVE );
	cv_trig_fire    = gEngfuncs.pfnRegisterVariable( "cl_trig_fire", "120", FCVAR_ARCHIVE );
	cv_trig_mode    = gEngfuncs.pfnRegisterVariable( "cl_trig_mode", "0", FCVAR_ARCHIVE );
	cv_trig_count   = gEngfuncs.pfnRegisterVariable( "cl_trig_count", "1", FCVAR_ARCHIVE );
	cv_trig_randmax = gEngfuncs.pfnRegisterVariable( "cl_trig_randmax", "3", FCVAR_ARCHIVE );
	cv_trig_fov     = gEngfuncs.pfnRegisterVariable( "cl_trig_fov", "1.2", FCVAR_ARCHIVE );
	cv_trig_draw    = gEngfuncs.pfnRegisterVariable( "cl_trig_draw", "0", FCVAR_ARCHIVE );
	cv_trig_accuracy= gEngfuncs.pfnRegisterVariable( "cl_trig_accuracy", "0", FCVAR_ARCHIVE );

	{
		int mode;

		for( mode = 0; mode < AIM_PART_MODE_COUNT; mode++ )
		{
			for( i = 0; i < AIM_PART_COUNT; i++ )
			{
				char name[64];
				const char *dir;

				if( mode == AIM_PART_MODE_AIM )
				{
					/* padrao antigo do aimbot: peito, pescoco e cabeca */
					dir = ( i == AIM_PART_UPPER_CHEST || i == AIM_PART_LOWER_CHEST ||
						i == AIM_PART_NECK || i == AIM_PART_HEAD ) ? "1" : "0";
				}
				else
				{
					/* silents: so a cabeca por padrao */
					dir = ( i == AIM_PART_HEAD ) ? "1" : "0";
				}

				/* AIM_PART_GENERIC e o fallback: sempre ligado */
				if( i == AIM_PART_GENERIC )
					dir = "1";

				snprintf( name, sizeof( name ), "%s%s", kPartPrefix[mode], kPartSuffix[i] );
				cv_part[mode][i] = gEngfuncs.pfnRegisterVariable( name, dir, FCVAR_ARCHIVE );
			}
		}
	}

	/*
	 * Nomes antigos: continuam existindo para o config.cfg velho nao
	 * reclamar, e ainda ligam o grupo correspondente.
	 */
	cv_old_head    = gEngfuncs.pfnRegisterVariable( "cl_aim_head", "0", FCVAR_ARCHIVE );
	cv_old_chest   = gEngfuncs.pfnRegisterVariable( "cl_aim_chest", "0", FCVAR_ARCHIVE );
	cv_old_stomach = gEngfuncs.pfnRegisterVariable( "cl_aim_stomach", "0", FCVAR_ARCHIVE );
	cv_old_arms    = gEngfuncs.pfnRegisterVariable( "cl_aim_arms", "0", FCVAR_ARCHIVE );
	cv_old_legs    = gEngfuncs.pfnRegisterVariable( "cl_aim_legs", "0", FCVAR_ARCHIVE );
	cv_old_body    = gEngfuncs.pfnRegisterVariable( "cl_aim_body", "0", FCVAR_ARCHIVE );


	/* migracao do smooth antigo: valores <= 1 eram "ganho direto" */
	if( cl_aim_smooth && cl_aim_smooth->value > 0.f && cl_aim_smooth->value < 1.f )
		SetCvar( "cl_aim_smooth", 1.f / cl_aim_smooth->value );
	if( cl_aim_smooth && cl_aim_smooth->value < 1.f )
		SetCvar( "cl_aim_smooth", 1.f );
}

void Aimbot_NoteMouse( float pitch, float yaw )
{
	g_mouse_pitch = pitch;
	g_mouse_yaw = yaw;
}

const aim_box_t *Aimbot_GetBoxes( int *count )
{
	if( count )
		*count = g_boxCount;
	return g_boxes;
}

void Aimbot_SetBoxes( const aim_box_t *boxes, int count )
{
	if( !boxes || count <= 0 )
	{
		g_boxCount = 0;
		return;
	}
	if( count > AIM_BOX_MAX )
		count = AIM_BOX_MAX;
	memcpy( g_boxes, boxes, sizeof( aim_box_t ) * count );
	g_boxCount = count;
	g_boxTime = gEngfuncs.GetClientTime();
}

/* ------------------------------------------------------------------ */
/* estado vindo da predicao                                           */
/* ------------------------------------------------------------------ */

void Aimbot_PostRunCmd( local_state_s *from, local_state_s *to,
	struct usercmd_s *cmd, int runfuncs, unsigned int seed )
{
	local_state_s *st;
	int id, clip;

	(void)runfuncs;
	if( !to )
		return;

	st = to;
	g_punch[0] = st->client.punchangle[0];
	g_punch[1] = st->client.punchangle[1];
	g_punch[2] = st->client.punchangle[2];
	g_flags = st->client.flags;
	g_velocity[0] = st->client.velocity[0];
	g_velocity[1] = st->client.velocity[1];
	g_velocity[2] = st->client.velocity[2];
	g_health = (int)st->client.health;
	g_nextAttack = st->client.m_flNextAttack;
	g_seed = seed;
	(void)cmd;

	id = st->client.m_iId;
	if( id <= 0 && from )
		id = from->client.m_iId;
	clip = 0;
	if( id > 0 && id < 32 )
	{
		clip = st->weapondata[id].m_iClip;
		g_inReload = st->weapondata[id].m_fInReload != 0;
		g_inZoom = st->weapondata[id].m_fInZoom != 0;
		g_nextPrimary = st->weapondata[id].m_flNextPrimaryAttack;
	}
	else
	{
		g_inReload = 0;
		g_inZoom = 0;
		g_nextPrimary = 0.f;
	}

	if( id != g_weaponId )
	{
		g_shots = 0;
		g_shotsPrev = clip;
	}
	else if( clip < g_shotsPrev )
	{
		g_shots += g_shotsPrev - clip;
		g_shotsPrev = clip;
	}
	else if( clip > g_shotsPrev )
	{
		/* recarregou / trocou carregador */
		g_shots = 0;
		g_shotsPrev = clip;
	}

	g_weaponId = id;
	g_clip = clip;
	g_lastPredTime = gEngfuncs.GetClientTime();
}

/* ------------------------------------------------------------------ */
/* alvo                                                               */
/* ------------------------------------------------------------------ */

static int PartEnabled( int mode, int part )
{
	if( mode < 0 || mode >= AIM_PART_MODE_COUNT )
		mode = AIM_PART_MODE_AIM;
	if( part < 0 || part >= AIM_PART_COUNT )
		part = AIM_PART_GENERIC;
	if( cv_part[mode][part] && cv_part[mode][part]->value > 0.5f )
		return 1;

	/* os grupos do menu antigo so valem para o aimbot */
	if( mode != AIM_PART_MODE_AIM )
		return 0;

	/* grupos do menu antigo */
	switch( part )
	{
	case AIM_PART_HEAD:
		return On( cv_old_head, 0 );
	case AIM_PART_NECK:
	case AIM_PART_UPPER_CHEST:
	case AIM_PART_LOWER_CHEST:
		return On( cv_old_chest, 0 );
	case AIM_PART_STOMACH:
	case AIM_PART_BELT:
		return On( cv_old_stomach, 0 );
	case AIM_PART_L_SHOULDER:
	case AIM_PART_L_ELBOW:
	case AIM_PART_L_ARM:
	case AIM_PART_L_HAND:
	case AIM_PART_R_SHOULDER:
	case AIM_PART_R_ELBOW:
	case AIM_PART_R_ARM:
	case AIM_PART_R_HAND:
		return On( cv_old_arms, 0 );
	case AIM_PART_L_THIGH:
	case AIM_PART_L_TIBIA:
	case AIM_PART_L_FOOT:
	case AIM_PART_R_THIGH:
	case AIM_PART_R_TIBIA:
	case AIM_PART_R_FOOT:
		return On( cv_old_legs, 0 );
	}

	return On( cv_old_body, 0 );
}

static int EnemyAlive( int index, int allowTeam )
{
	cl_entity_t *local;
	cl_entity_t *ent;
	int team;

	local = gEngfuncs.GetLocalPlayer();
	if( !local || index <= 0 || index == local->index )
		return 0;
	if( index > gEngfuncs.GetMaxClients() )
		return 0;

	ent = gEngfuncs.GetEntityByIndex( index );
	if( !ent || !ent->player || !ent->model )
		return 0;
	if( g_PlayerExtraInfo[index].dead )
		return 0;

	team = g_PlayerExtraInfo[index].teamnumber;
	if( team != 1 && team != 2 )
		return 0;
	if( !allowTeam && g_iTeamNumber != 0 && team == g_iTeamNumber )
		return 0;
	return 1;
}

/* 1 quando a linha de visao chega ao ponto (ou quando "atravessa parede"). */
static int PointVisible( const float *eye, const float *point, int player, int ignoreWalls )
{
	pmtrace_t tr;

	gEngfuncs.pEventAPI->EV_SetTraceHull( 0 );
	if( ignoreWalls )
	{
		gEngfuncs.pEventAPI->EV_PlayerTrace( (float *)eye, (float *)point, PM_WORLD_ONLY, -1, &tr );
		return !tr.startsolid && tr.fraction >= 0.97f;
	}

	gEngfuncs.pEventAPI->EV_PlayerTrace( (float *)eye, (float *)point, PM_GLASS_IGNORE, -1, &tr );
	if( tr.startsolid )
		return 0;
	if( tr.fraction >= 0.97f )
		return 1;
	return gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ) == player;
}

static void AngleToPoint( const float *eye, const float *point, float *angles )
{
	float d[3];
	float flat;

	d[0] = point[0] - eye[0];
	d[1] = point[1] - eye[1];
	d[2] = point[2] - eye[2];

	flat = sqrtf( d[0] * d[0] + d[1] * d[1] );
	angles[1] = atan2f( d[1], d[0] ) * AIM_RAD2DEG;
	angles[0] = -atan2f( d[2], flat ) * AIM_RAD2DEG;
	angles[2] = 0.f;
}

static float AngleDelta( const float *a, const float *b )
{
	float dp = NormAngle( b[0] - a[0] );
	float dy = NormAngle( b[1] - a[1] );
	return sqrtf( dp * dp + dy * dy );
}

static int FindTarget( const float *eye, const float *view, float fov, int allowTeam,
	int ignoreWalls, int partMode, int *outPlayer, float *outAim, float *outAngle, float *outDist )
{
	int i;
	int best = 0;
	float bestAngle = fov;
	int bestRank = 0x7fff;
	int bestPart = AIM_PART_GENERIC;
	float bestDist = 0.f;
	float bestAim[3] = { 0.f, 0.f, 0.f };
	int useRank = !On( cv_hit_mode, 0 );

	for( i = 0; i < g_boxCount; i++ )
	{
		float aim[3];
		float ang;
		float dist;
		int rank;

		if( !PartEnabled( partMode, g_boxes[i].part ) )
			continue;
		if( !EnemyAlive( g_boxes[i].player, allowTeam ) )
			continue;
		if( !PointVisible( eye, g_boxes[i].origin, g_boxes[i].player, ignoreWalls ) )
			continue;

		AngleToPoint( eye, g_boxes[i].origin, aim );
		ang = AngleDelta( view, aim );
		if( ang > fov )
			continue;

		rank = kPartRank[( g_boxes[i].part >= 0 && g_boxes[i].part < AIM_PART_COUNT )
			? g_boxes[i].part : AIM_PART_GENERIC];

		if( useRank )
		{
			if( rank > bestRank )
				continue;
			if( rank == bestRank && ang >= bestAngle )
				continue;
		}
		else
		{
			if( ang >= bestAngle )
				continue;
		}

		dist = sqrtf( ( g_boxes[i].origin[0] - eye[0] ) * ( g_boxes[i].origin[0] - eye[0] ) +
			( g_boxes[i].origin[1] - eye[1] ) * ( g_boxes[i].origin[1] - eye[1] ) +
			( g_boxes[i].origin[2] - eye[2] ) * ( g_boxes[i].origin[2] - eye[2] ) );

		best = g_boxes[i].player;
		bestRank = rank;
		bestPart = g_boxes[i].part;
		bestAngle = ang;
		bestDist = dist;
		bestAim[0] = aim[0];
		bestAim[1] = aim[1];
		bestAim[2] = 0.f;
	}

	if( !best )
		return 0;

	*outPlayer = best;
	outAim[0] = bestAim[0];
	outAim[1] = bestAim[1];
	outAim[2] = 0.f;
	*outAngle = bestAngle;
	*outDist = bestDist;
	g_targetPart = bestPart;
	g_targetPartMode = partMode;
	return 1;
}

/* ------------------------------------------------------------------ */
/* smooth                                                             */
/* ------------------------------------------------------------------ */

static float SmoothAlpha( float smooth, float dt, float angRatio )
{
	float s = smooth;
	float alpha;
	float scale;

	if( s < 1.f )
		s = 1.f;

	scale = CV( cv_speed_fov, 0.f );
	if( scale > 0.f )
	{
		float k = Clampf( angRatio, 0.f, 1.f );
		s *= 1.f - ( k * scale * 0.01f );
		if( s < 1.f )
			s = 1.f;
	}

	if( s <= 1.0001f )
		return 1.f;

	switch( (int)CV( cv_smooth_mode, 1.f ) )
	{
	case 0: /* linear, igual ao Sakura: delta / smooth por comando */
		alpha = 1.f / s;
		break;
	case 2: /* instantaneo */
		alpha = 1.f;
		break;
	default: /* tempo: tau proporcional ao slider */
	{
		float tau = s * 0.0045f;
		alpha = 1.f - expf( -dt / tau );
		break;
	}
	}

	return Clampf( alpha, 0.0f, 1.f );
}

static void SmoothStep( const float *from, const float *to, float *out,
	float alpha, int humanize, float sx, float sy )
{
	float dp = NormAngle( to[0] - from[0] );
	float dy = NormAngle( to[1] - from[1] );

	if( humanize )
	{
		float t = dp;
		float u = dy;

		dp = t + u / ( sx > 1.f ? sx : 1.f );
		dy = u + t / ( sy > 1.f ? sy : 1.f );
	}

	out[0] = Clampf( from[0] + dp * alpha, -89.f, 89.f );
	out[1] = from[1] + dy * alpha;
	out[2] = 0.f;
}

/* ------------------------------------------------------------------ */
/* flash / accuracy                                                   */
/* ------------------------------------------------------------------ */

static float FlashAlpha( void )
{
	static cvar_t *cv;

	/*
	 * A engine registra cl_flash_alpha no primeiro screenfade; nao guardar
	 * o resultado negativo porque no primeiro frame ela ainda nao existe.
	 */
	if( !cv )
		cv = (cvar_t *)gEngfuncs.pfnGetCvarPointer( "cl_flash_alpha" );
	return cv ? cv->value : 0.f;
}

static int Flashed( void )
{
	if( !On( cv_flashcheck, 0 ) )
		return 0;
	return FlashAlpha() > Clampf( CV( cv_flash_pct, 30.f ), 0.f, 100.f ) * 2.55f;
}

/* Inaccuracy atual em graus (estimate do spread_predictor). */
static float SpreadDegrees( void )
{
	float spread = SpreadDot_GetSpreadDegrees();

	if( spread < 0.f )
		spread = 0.f;
	return spread;
}

/* 1 = o tiro deve sair (accuracy boost nao bloqueou). */
static int AccuracyAllows( float angleToTarget, int forTrigger )
{
	int mode = forTrigger ? (int)CV( cv_trig_accuracy, 0.f ) : (int)CV( cv_accuracy, 0.f );
	float limit;

	if( mode <= 0 )
		return 1;

	switch( mode )
	{
	case 1: /* so mirando: exige que o alvo esteja na cone de spread */
		break;
	case 2: /* + recoil */
		break;
	default: /* + spread */
		break;
	}

	limit = SpreadDegrees() * 2.f + 0.35f;
	if( limit < 0.6f )
		limit = 0.6f;

	return angleToTarget <= limit;
}

/* ------------------------------------------------------------------ */
/* triggerbot                                                         */
/* ------------------------------------------------------------------ */

static double NowSeconds( void )
{
	return gEngfuncs.GetClientTime();
}

static int WeaponCanFire( void )
{
	double now = NowSeconds();

	if( !GunWeapon( g_weaponId ) )
		return 0;
	if( g_clip <= 0 )
		return 0;
	if( g_inReload )
		return 0;
	if( g_nextAttack > 0.f && (float)now < g_nextAttack )
		return 0;
	if( g_nextPrimary > 0.f && (float)now < g_nextPrimary )
		return 0;
	if( On( cv_trig_zoom, 0 ) )
	{
		if( !SniperWeapon( g_weaponId ) && !g_inZoom )
			return 0;
		if( SniperWeapon( g_weaponId ) && !g_inZoom )
			return 0;
	}
	return 1;
}

static void Trigger_Reset( void )
{
	g_trigTarget = 0;
	g_trigAcquire = 0.0;
	g_trigFired = 0;
}

static void Trigger_Run( usercmd_t *cmd, const float *eye, const float *view )
{
	float fov, aim[3], dist, ang;
	int player = 0;
	double now = NowSeconds();
	int burst;

	if( On( cv_trig_enable, 0 ) == 0 )
	{
		Trigger_Reset();
		return;
	}

	if( g_iUser1 != 0 || !WeaponCanFire() )
	{
		Trigger_Reset();
		return;
	}

	if( Flashed() )
	{
		Trigger_Reset();
		return;
	}

	fov = Clampf( CV( cv_trig_fov, 1.2f ), 0.05f, 45.f );

	if( !FindTarget( eye, view, fov, On( cv_trig_team, 0 ), On( cv_trig_wall, 0 ),
		AIM_PART_MODE_AIM, &player, aim, &ang, &dist ) )
	{
		Trigger_Reset();
		return;
	}

	/*
	 * Backtrack no gatilho: se o ponto historico do hitbox esta sob a mira,
	 * libera o tiro como se o alvo estivesse ali. So relaxa o angulo, nunca
	 * bloqueia o alvo atual.
	 */
	if( Backtrack_Enabled() )
	{
		float bt[3];
		float btAim[3];
		int btms = 0;

		if( Backtrack_GetPoint( player, g_targetPart, bt, &btms ))
		{
			float btAng;

			AngleToPoint( eye, bt, btAim );
			btAim[2] = 0.f;
			btAng = AngleDelta( view, btAim );

			if( ang <= 0.f || btAng < ang )
				ang = btAng;
		}
	}

	if( !AccuracyAllows( ang, 1 ) )
		return;

	if( g_trigTarget != player )
	{
		g_trigTarget = player;
		g_trigAcquire = now;
		g_trigFired = 0;
		g_triggerState = 0;
	}

	if( now - g_trigAcquire < Clampf( CV( cv_trig_delay, 60.f ), 0.f, 1000.f ) * 0.001 )
		return;

	if( now < g_trigNextShot )
		return;

	burst = (int)CV( cv_trig_mode, 0.f ) == 0
		? (int)Clampf( CV( cv_trig_count, 1.f ), 1.f, 5.f )
		: (int)Clampf( CV( cv_trig_randmax, 3.f ), 2.f, 5.f );

	if( g_trigFired >= burst )
	{
		/* rajada terminou: espera 250 ms antes da proxima */
		if( now - g_trigNextShot < 0.25 )
			return;
		g_trigFired = 0;
	}

	cmd->buttons |= IN_ATTACK;
	g_trigFired++;
	g_trigNextShot = now + Clampf( CV( cv_trig_fire, 120.f ), 10.f, 1000.f ) * 0.001;
}

/* ------------------------------------------------------------------ */
/* perfect silent                                                     */
/* ------------------------------------------------------------------ */

static void SpreadOffset( float *pitch, float *yaw )
{
	float dx = 0.f, dy = 0.f;

	*pitch = 0.f;
	*yaw = 0.f;

	if( !SpreadDot_GetSpreadOffset( g_seed, &dx, &dy ) )
		return;

	/* tangente -> graus: x = direita (yaw), y = cima (pitch) */
	*yaw = atanf( dx ) * AIM_RAD2DEG;
	*pitch = atanf( dy ) * AIM_RAD2DEG;
}

/* ------------------------------------------------------------------ */
/* apply                                                              */
/* ------------------------------------------------------------------ */

int Aimbot_Apply( float viewangles[3], usercmd_t *cmd )
{
	float eye[3], ofs[3];
	float view[3];
	float aim[3];
	float dist = 0.f;
	float fov, smooth, dt, ratio;
	float ang = 0.f;
	int player = 0;
	int hasTarget;
	int silent, psilent;
	int attacking;
	int partMode;
	int i;
	cl_entity_t *local;
	double now;

	if( !cmd )
		return 0;

	if( g_iUser1 != 0 )
		return 0;

	now = NowSeconds();
	dt = 1.f / 72.f;
	if( g_lastCmdTime > 0.0 && now > g_lastCmdTime )
		dt = (float)( now - g_lastCmdTime );
	dt = Clampf( dt, 0.001f, 0.1f );
	g_lastCmdTime = now;

	attacking = ( cmd->buttons & IN_ATTACK ) != 0;
	Backtrack_SetContext( attacking );
	view[0] = viewangles[0];
	view[1] = viewangles[1];
	view[2] = 0.f;

	local = gEngfuncs.GetLocalPlayer();
	if( !local )
		return 0;

	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( ofs );
	eye[0] = local->origin[0] + ofs[0];
	eye[1] = local->origin[1] + ofs[1];
	eye[2] = local->origin[2] + ofs[2];

	/*
	 * Triggerbot roda mesmo com a mira desligada: ele so aperta o gatilho
	 * quando o hitbox escolhido esta sob a mira.
	 */
	Trigger_Run( cmd, eye, view );

	/* o triggerbot pode ter apertado o gatilho neste mesmo comando */
	attacking = ( cmd->buttons & IN_ATTACK ) != 0;
	Backtrack_SetContext( attacking );

	if( !On( cl_aim_assist, 0 ) || g_boxCount <= 0 )
	{
		g_target = 0;
		return 0;
	}

	/* modo de ativacao */
	switch( (int)CV( cv_mode, 1.f ) )
	{
	case 0:
		break;
	case 2:
		if( !g_keyHeld )
			return 0;
		break;
	default:
		if( !attacking )
		{
			/* "auto aim speed": continua puxando devagar sem atirar */
			if( CV( cv_auto_speed, 0.f ) <= 0.f )
				return 0;
		}
		break;
	}

	if( Flashed() )
		return 0;

	if( CL_IsDead() )
		return 0;

	fov = Clampf( CV( cl_aim_fov, 12.f ), 0.5f, 180.f );

	/* recoil compensation amplia o FOV quando ha recuo acumulado */
	if( (int)CV( cv_rc_fov, 0.f ) > 0 && g_shots > (int)CV( cv_rc_after, 0.f ) )
		fov += fov * CV( cv_rc_fov, 0.f ) * 0.01f;

	/*
	 * O perfil de hitbox acompanha o mecanismo que vai ser usado:
	 * psilent so age atirando, silent age sempre, senao e o aimbot normal.
	 */
	partMode = AIM_PART_MODE_AIM;
	if( On( cv_psilent, 0 ) && attacking )
		partMode = AIM_PART_MODE_PSILENT;
	else if( On( cv_silent, 0 ) )
		partMode = AIM_PART_MODE_SILENT;

	hasTarget = FindTarget( eye, view, fov, On( cv_team, 0 ), On( cv_walls, 0 ),
		partMode, &player, aim, &ang, &dist );

	if( !hasTarget )
	{
		/*
		 * "Block attack after kill": quando o alvo sai da lista (morreu) no
		 * meio do spray, solta o gatilho por N ms para nao gastar bala.
		 */
		if( CV( cv_blockkill, 0.f ) > 0.f )
		{
			if( g_prevTarget )
				g_blockAttackUntil = now + CV( cv_blockkill, 0.f ) * 0.001;
			if( attacking && now < g_blockAttackUntil )
				cmd->buttons &= ~IN_ATTACK;
		}

		g_target = 0;
		g_prevTarget = 0;
		g_acquireTime = 0.0;
		return 0;
	}

	/* reaction time: espera ao trocar de alvo */
	if( g_target != player )
	{
		g_target = player;
		g_acquireTime = now;
	}
	if( now - g_acquireTime < Clampf( CV( cv_reaction, 0.f ), 0.f, 1000.f ) * 0.001 )
		return 0;

	/*
	 * Backtrack: mira onde o hitbox estava N ms atras. Se o ponto historico
	 * sair do FOV, volta para o ponto atual que o FindTarget ja calculou.
	 */
	if( Backtrack_Enabled() )
	{
		float bt[3];
		float keep[3];
		int btms = 0;

		keep[0] = aim[0];
		keep[1] = aim[1];
		keep[2] = aim[2];

		if( Backtrack_GetPoint( player, g_targetPart, bt, &btms ))
		{
			float btAng;

			AngleToPoint( eye, bt, aim );
			aim[2] = 0.f;
			btAng = AngleDelta( view, aim );

			if( btAng > fov )
			{
				aim[0] = keep[0];
				aim[1] = keep[1];
				aim[2] = keep[2];
			}
			else
			{
				ang = btAng;
				dist = sqrtf( ( bt[0] - eye[0] ) * ( bt[0] - eye[0] ) +
					( bt[1] - eye[1] ) * ( bt[1] - eye[1] ) +
					( bt[2] - eye[2] ) * ( bt[2] - eye[2] ) );
			}
		}
	}

	g_prevTarget = player;
	g_targetAim[0] = aim[0];
	g_targetAim[1] = aim[1];
	g_targetAim[2] = 0.f;
	g_targetAngle = ang;
	g_targetDist = dist;

	/* recoil compensation nos angulos de destino */
	if( CV( cv_rc_pitch, 0.f ) > 0.f || CV( cv_rc_yaw, 0.f ) > 0.f )
	{
		if( g_shots > (int)CV( cv_rc_after, 0.f ) )
		{
			aim[0] -= g_punch[0] * CV( cv_rc_pitch, 0.f ) * 0.01f;
			aim[1] -= g_punch[1] * CV( cv_rc_yaw, 0.f ) * 0.01f;
		}
	}

	silent = On( cv_silent, 0 );
	psilent = On( cv_psilent, 0 );

	if( psilent && attacking )
	{
		float sp, sy;

		SpreadOffset( &sp, &sy );
		if( On( cv_psilent_spread, 0 ) )
		{
			aim[0] += sp - g_punch[0];
			aim[1] += sy - g_punch[1];
		}
		else
		{
			aim[0] -= g_punch[0];
			aim[1] -= g_punch[1];
		}

		if( ang <= Clampf( CV( cv_psilent_fov, 1.5f ), 0.1f, 15.f ) )
		{
			cmd->viewangles[0] = Clampf( aim[0], -89.f, 89.f );
			cmd->viewangles[1] = aim[1];
			cmd->viewangles[2] = 0.f;
			return 0; /* camera intacta */
		}
	}

	/* accuracy boost da mira normal */
	if( !AccuracyAllows( ang, 0 ) && !silent )
		return 0;

	/* smooth */
	smooth = CV( cl_aim_smooth, 8.f );
	if( !attacking && CV( cv_auto_speed, 0.f ) > 0.f )
		smooth = 101.f - Clampf( CV( cv_auto_speed, 0.f ), 0.f, 100.f );
	if( smooth < 1.f )
		smooth = 1.f;

	/* recuo ativo deixa o passo mais lento (opcional) */
	if( CV( cv_rc_smooth, 0.f ) > 0.f && g_shots > (int)CV( cv_rc_after, 0.f ) )
		smooth += smooth * CV( cv_rc_smooth, 0.f ) * 0.01f;

	ratio = fov > 0.01f ? ang / fov : 0.f;

	if( silent )
	{
		/* silent: o comando vai para o alvo, a camera fica onde o jogador deixou */
		SmoothStep( view, aim, cmd->viewangles, SmoothAlpha( smooth, dt, ratio ),
			On( cv_humanize, 0 ), CV( cv_spiral_x, 1.3f ), CV( cv_spiral_y, 3.7f ) );
		return 0;
	}

	SmoothStep( view, aim, viewangles, SmoothAlpha( smooth, dt, ratio ),
		On( cv_humanize, 0 ), CV( cv_spiral_x, 1.3f ), CV( cv_spiral_y, 3.7f ) );

	viewangles[2] = 0.f;

	if( On( cv_debug, 0 ) )
	{
		static double lastPrint;

		if( now - lastPrint > 0.25 )
		{
			lastPrint = now;
			gEngfuncs.Con_Printf( "aim: alvo %d parte %d ang %.2f fov %.1f dist %.0f smooth %.0f\n",
				player, g_targetPart, ang, fov, dist, smooth );
		}
	}

	/* fast zoom: sniper sem scope, aperta o zoom automaticamente */
	if( On( cv_fastzoom, 0 ) && SniperWeapon( g_weaponId ) && !g_inZoom && attacking )
		cmd->buttons |= IN_ATTACK2;

	for( i = 0; i < 3; i++ )
		cmd->viewangles[i] = viewangles[i];

	return 1;
}

/* ------------------------------------------------------------------ */
/* desenho                                                            */
/* ------------------------------------------------------------------ */

static void DrawBox( const float c[8][3], int r, int g, int b )
{
	static const int edges[12][2] =
	{
		{0,1},{1,2},{2,3},{3,0},
		{4,5},{5,6},{6,7},{7,4},
		{0,4},{1,5},{2,6},{3,7}
	};
	float s[8][2];
	int ok[8];
	int i;

	for( i = 0; i < 8; i++ )
		ok[i] = ToScreen( c[i], &s[i][0], &s[i][1] );

	for( i = 0; i < 12; i++ )
	{
		int a = edges[i][0], d = edges[i][1];
		int x1, y1, x2, y2, n, steps;

		if( !ok[a] || !ok[d] )
			continue;

		steps = (int)( fabsf( s[d][0] - s[a][0] ) + fabsf( s[d][1] - s[a][1] ) ) / 6;
		if( steps < 1 )
			steps = 1;
		if( steps > 48 )
			steps = 48;

		for( n = 0; n <= steps; n++ )
		{
			float t = (float)n / (float)steps;

			x1 = (int)( s[a][0] + ( s[d][0] - s[a][0] ) * t );
			y1 = (int)( s[a][1] + ( s[d][1] - s[a][1] ) * t );
			(void)x2;
			(void)y2;
			gEngfuncs.pfnFillRGBA( x1, y1, 1, 1, r, g, b, 170 );
		}
	}
}

static void DrawFovCircle( void )
{
	float fov, vfov, radius;
	int i, cx, cy;
	const int steps = 96;

	if( !On( cv_drawfov, 1 ) )
		return;
	if( !On( cl_aim_assist, 0 ) && !On( cv_trig_enable, 0 ) )
		return;

	fov = On( cv_trig_enable, 0 ) && !On( cl_aim_assist, 0 )
		? CV( cv_trig_fov, 1.2f )
		: CV( cl_aim_fov, 12.f );

	fov = Clampf( fov, 0.2f, 180.f );

	vfov = (float)gHUD.m_iFOV;
	if( vfov < 10.f )
		vfov = 90.f;

	radius = tanf( fov * AIM_DEG2RAD ) / tanf( ( vfov * 0.5f ) * AIM_DEG2RAD );
	radius *= ScreenHeight * 0.5f;
	if( radius < 3.f )
		radius = 3.f;
	if( radius > ScreenHeight )
		radius = (float)ScreenHeight;

	cx = ScreenWidth / 2;
	cy = ScreenHeight / 2;
	for( i = 0; i < steps; i++ )
	{
		float a = (float)i * ( 2.f * AIM_PI / (float)steps );
		int x = cx + (int)( cosf( a ) * radius );
		int y = cy + (int)( sinf( a ) * radius );

		gEngfuncs.pfnFillRGBA( x - 1, y - 1, 3, 3, 255, 176, 32, 210 );
	}
}

static void DrawTargetOverlay( void )
{
	int i;

	if( !On( cv_draw, 0 ) && !On( cv_trig_draw, 0 ) )
		return;

	if( g_target )
	{
		for( i = 0; i < g_boxCount; i++ )
		{
			if( g_boxes[i].player != g_target )
				continue;
			if( !PartEnabled( g_targetPartMode, g_boxes[i].part ) )
				continue;
			DrawBox( g_boxes[i].c, 255, 60, 60 );
		}
	}

	if( g_trigTarget && On( cv_trig_draw, 0 ) )
	{
		for( i = 0; i < g_boxCount; i++ )
		{
			if( g_boxes[i].player != g_trigTarget )
				continue;
			DrawBox( g_boxes[i].c, 80, 200, 255 );
		}
	}

	/* linha do centro da tela ate o alvo */
	if( g_target )
	{
		float sx, sy;
		float org[3];
		int found = 0;

		for( i = 0; i < g_boxCount && !found; i++ )
		{
			if( g_boxes[i].player != g_target )
				continue;
			if( !PartEnabled( g_targetPartMode, g_boxes[i].part ) )
				continue;
			org[0] = g_boxes[i].origin[0];
			org[1] = g_boxes[i].origin[1];
			org[2] = g_boxes[i].origin[2];
			found = 1;
		}

		if( found && ToScreen( org, &sx, &sy ) )
		{
			float cx = ScreenWidth * 0.5f, cy = ScreenHeight * 0.5f;
			int steps = (int)( fabsf( sx - cx ) + fabsf( sy - cy ) ) / 6;

			if( steps < 1 ) steps = 1;
			if( steps > 96 ) steps = 96;
			for( i = 0; i <= steps; i++ )
			{
				float t = (float)i / (float)steps;
				int x = (int)( cx + ( sx - cx ) * t );
				int y = (int)( cy + ( sy - cy ) * t );

				gEngfuncs.pfnFillRGBA( x, y, 1, 1, 255, 90, 90, 120 );
			}
		}
	}
}

void Aimbot_Draw( void )
{
	DrawFovCircle();
	DrawTargetOverlay();

	if( On( cl_aim_assist, 0 ) && !Hub_IsOpen() )
	{
		char buf[96];
		const char *mode = On( cv_silent, 0 ) ? ( On( cv_psilent, 0 ) ? "PSILENT" : "SILENT" ) : "AIM";

		snprintf( buf, sizeof( buf ), "%s", mode );
		DrawUtils::DrawHudString( XRES( 8 ), YRES( 8 ), ScreenWidth - 8, buf, 255, 220, 80 );
	}
	if( On( cv_trig_enable, 0 ) && !Hub_IsOpen() )
		DrawUtils::DrawHudString( XRES( 8 ), YRES( 20 ), ScreenWidth - 8, "TRIG", 80, 200, 255 );
}

/* ------------------------------------------------------------------ */
/* teclas                                                             */
/* ------------------------------------------------------------------ */

int Aimbot_Key( int down, int keynum )
{
	if( g_captureKey && down )
	{
		SetCvar( "cl_aim_key", (float)keynum );
		g_captureKey = 0;
		return 0;
	}

	if( keynum > 0 && keynum == (int)CV( cv_key, 0.f ) )
	{
		g_keyHeld = down ? 1 : 0;
		return 1; /* a tecla continua chegando no jogo */
	}

	return 1;
}

/* ------------------------------------------------------------------ */
/* acesso para o backtrack                                            */
/* ------------------------------------------------------------------ */

int Aimbot_CurrentTarget( void )
{
	return g_target;
}

int Aimbot_KeyHeld( void )
{
	return g_keyHeld;
}

int Aimbot_PointVisible( const float *eye, const float *point, int player, int ignoreWalls )
{
	return PointVisible( eye, point, player, ignoreWalls );
}

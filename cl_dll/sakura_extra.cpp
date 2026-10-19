/*
 * sakura_extra.cpp - features complementares do HUB.
 *
 *   chams      : pinta o modelo do inimigo com uma cor solida
 *   knifebot   : stab pelas costas, slash na frente
 *   killsound  : som ao matar / ao acertar
 *   nightmode  : luz do mapa por cvar do engine
 *   skins      : usa os comandos skin_<arma> do skin_model
 *   cores      : cl_hub_accent e cl_hub_bg lidas pelo ui_theme
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "cvardef.h"
#include "usercmd.h"
#include "in_buttons.h"
#include "entity_state.h"
#include "aimbot.h"
#include "sakura_extra.h"

static cvar_t *cv_chams;
static cvar_t *cv_chams_color;
static cvar_t *cv_knifebot;
static cvar_t *cv_knife_range;
static cvar_t *cv_killsound;
static cvar_t *cv_hitsound;
static cvar_t *cv_night;
static cvar_t *cv_skin;
static cvar_t *cv_skin_wpn;

static cvar_t *cv_hub_accent;
static cvar_t *cv_hub_accent_g;
static cvar_t *cv_hub_accent_b;
static cvar_t *cv_hub_bg;
static cvar_t *cv_hub_bg_g;
static cvar_t *cv_hub_bg_b;

static int g_nightApplied = -1;
static int g_lastKill;
static int g_lastHealth[33];
static int g_lastSkin = -1;
static int g_lastSkinWpn = -1;

static const char *kSkinWeapons[] =
{
	"ak47", "m4a1", "awp", "deagle", "usp", "glock", "knife", NULL
};

static int On( cvar_t *cv )
{
	return cv && cv->value > 0.5f;
}

void SakuraExtra_Init( void )
{
	cv_chams       = gEngfuncs.pfnRegisterVariable( "cl_sak_chams", "0", FCVAR_ARCHIVE );
	cv_chams_color = gEngfuncs.pfnRegisterVariable( "cl_sak_chams_color", "0", FCVAR_ARCHIVE );
	cv_knifebot    = gEngfuncs.pfnRegisterVariable( "cl_sak_knifebot", "0", FCVAR_ARCHIVE );
	cv_knife_range = gEngfuncs.pfnRegisterVariable( "cl_sak_knifebot_range", "48", FCVAR_ARCHIVE );
	cv_killsound   = gEngfuncs.pfnRegisterVariable( "cl_sak_killsound", "0", FCVAR_ARCHIVE );
	cv_hitsound    = gEngfuncs.pfnRegisterVariable( "cl_sak_hitsound", "0", FCVAR_ARCHIVE );
	cv_night       = gEngfuncs.pfnRegisterVariable( "cl_sak_world_night", "0", FCVAR_ARCHIVE );
	cv_skin        = gEngfuncs.pfnRegisterVariable( "cl_sak_skin", "0", FCVAR_ARCHIVE );
	cv_skin_wpn    = gEngfuncs.pfnRegisterVariable( "cl_sak_skin_weapon", "0", FCVAR_ARCHIVE );

	/* cores do menu; o ui_theme le com fallback na paleta do Sakura */
	cv_hub_accent   = gEngfuncs.pfnRegisterVariable( "cl_hub_accent", "1.0", FCVAR_ARCHIVE );
	cv_hub_accent_g = gEngfuncs.pfnRegisterVariable( "cl_hub_accent_g", "0.0784", FCVAR_ARCHIVE );
	cv_hub_accent_b = gEngfuncs.pfnRegisterVariable( "cl_hub_accent_b", "0.5764", FCVAR_ARCHIVE );
	cv_hub_bg       = gEngfuncs.pfnRegisterVariable( "cl_hub_bg", "0.0980", FCVAR_ARCHIVE );
	cv_hub_bg_g     = gEngfuncs.pfnRegisterVariable( "cl_hub_bg_g", "0.1176", FCVAR_ARCHIVE );
	cv_hub_bg_b     = gEngfuncs.pfnRegisterVariable( "cl_hub_bg_b", "0.1372", FCVAR_ARCHIVE );
}

/* ------------------------------------------------------------------ */
/* chams                                                              */
/* ------------------------------------------------------------------ */

static int      g_chamsSaved;
static int      g_chamsRenderMode;
static int      g_chamsRenderAmt;
static color24  g_chamsColor;

static void Chams_SetColor( cl_entity_t *ent, int r, int g, int b )
{
	ent->curstate.rendercolor.r = (byte)r;
	ent->curstate.rendercolor.g = (byte)g;
	ent->curstate.rendercolor.b = (byte)b;
}

void Chams_Begin( cl_entity_t *ent )
{
	int team;

	if( !ent || !On( cv_chams ) )
		return;

	if( ent->player && ent->index == gEngfuncs.GetLocalPlayer()->index )
		return;

	g_chamsSaved = 1;
	g_chamsRenderMode = ent->curstate.rendermode;
	g_chamsRenderAmt = ent->curstate.renderamt;
	g_chamsColor = ent->curstate.rendercolor;

	team = ( ent->index > 0 && ent->index <= MAX_PLAYERS ) ? g_PlayerExtraInfo[ent->index].teamnumber : 0;

	if( (int)cv_chams_color->value == 0 )
	{
		/* 0 = por time */
		if( team == 1 )
			Chams_SetColor( ent, 255, 60, 60 );
		else if( team == 2 )
			Chams_SetColor( ent, 70, 140, 255 );
		else
			Chams_SetColor( ent, 255, 20, 147 );
	}
	else
	{
		switch( (int)cv_chams_color->value )
		{
		case 1: Chams_SetColor( ent, 255, 40, 40 ); break;
		case 2: Chams_SetColor( ent, 40, 255, 40 ); break;
		case 3: Chams_SetColor( ent, 40, 40, 255 ); break;
		default: Chams_SetColor( ent, 255, 255, 255 ); break;
		}
	}

	ent->curstate.rendermode = kRenderTransColor;
	ent->curstate.renderamt = 255;
}

void Chams_End( cl_entity_t *ent )
{
	if( !ent || !g_chamsSaved )
		return;

	ent->curstate.rendermode = g_chamsRenderMode;
	ent->curstate.renderamt = g_chamsRenderAmt;
	ent->curstate.rendercolor = g_chamsColor;
	g_chamsSaved = 0;
}

/* ------------------------------------------------------------------ */
/* knifebot                                                           */
/* ------------------------------------------------------------------ */

static int HoldingKnife( void )
{
	cl_entity_t *vm = gEngfuncs.GetViewModel();

	return vm && vm->model && vm->model->name && strstr( vm->model->name, "knife" ) != NULL;
}

static float NormalizeYaw( float ang )
{
	while( ang > 180.f )
		ang -= 360.f;
	while( ang < -180.f )
		ang += 360.f;
	return ang;
}

void SakuraExtra_PostMove( usercmd_t *cmd, const float viewangles[3] )
{
	const aim_box_t *boxes;
	int count = 0;
	cl_entity_t *local;
	float eye[3];
	float fwd[3];
	int i;

	if( !cmd || !On( cv_knifebot ) )
		return;

	if( !HoldingKnife() )
		return;

	local = gEngfuncs.GetLocalPlayer();
	if( !local )
		return;

	boxes = Aimbot_GetBoxes( &count );
	if( !boxes || count <= 0 )
		return;

	eye[0] = local->origin[0];
	eye[1] = local->origin[1];
	eye[2] = local->origin[2] + 28.f;

	fwd[0] = cosf( viewangles[1] * 3.14159265f / 180.f );
	fwd[1] = sinf( viewangles[1] * 3.14159265f / 180.f );

	for( i = 0; i < count; i++ )
	{
		float dx, dy, dz, dist, len;
		float angle;
		float targetYaw;

		if( boxes[i].player == local->index )
			continue;

		/* so o peito serve de referencia */
		if( boxes[i].part != AIM_PART_UPPER_CHEST && boxes[i].part != AIM_PART_LOWER_CHEST )
			continue;

		dx = boxes[i].origin[0] - eye[0];
		dy = boxes[i].origin[1] - eye[1];
		dz = boxes[i].origin[2] - eye[2];
		dist = sqrtf( dx * dx + dy * dy + dz * dz );

		if( dist > ( cv_knife_range ? cv_knife_range->value : 48.f ) )
			continue;

		if( fabsf( dz ) > 48.f )
			continue;

		targetYaw = atan2f( dy, dx ) * 180.f / 3.14159265f;
		angle = NormalizeYaw( targetYaw - viewangles[1] );

		if( fabsf( angle ) > 60.f )
			continue;

		len = sqrtf( dx * dx + dy * dy );
		if( len > 0.1f )
		{
			/* alvo de costas para a nossa mira -> stab */
			if( ( fwd[0] * dx + fwd[1] * dy ) / len > 0.5f )
				cmd->buttons |= IN_ATTACK2;
			else
				cmd->buttons |= IN_ATTACK;
		}
		return;
	}
}

/* ------------------------------------------------------------------ */
/* sons e mundo                                                       */
/* ------------------------------------------------------------------ */

static void TickKills( void )
{
	int i;
	int maxclients = gEngfuncs.GetMaxClients();

	for( i = 1; i <= maxclients && i < 33; i++ )
	{
		int dead = g_PlayerExtraInfo[i].dead ? 1 : 0;
		int hp = g_PlayerExtraInfo[i].health;

		/* quem estava vivo e morreu */
		if( On( cv_killsound ) && dead && !( g_lastKill & ( 1 << i ) ) )
			gEngfuncs.pfnPlaySoundByName( "sound/buttons/blip2.wav", 0.5f );

		/* vida caiu sem morrer = acertamos */
		if( On( cv_hitsound ) && !dead && g_lastHealth[i] > 0 && hp > 0 && hp < g_lastHealth[i] )
			gEngfuncs.pfnPlaySoundByName( "sound/buttons/blip1.wav", 0.4f );

		g_lastHealth[i] = hp;

		if( dead )
			g_lastKill |= ( 1 << i );
		else
			g_lastKill &= ~( 1 << i );
	}
}

static void TickNight( void )
{
	cvar_t *light;
	int want;

	if( !cv_night )
		return;

	want = On( cv_night ) ? 1 : 0;
	if( want == g_nightApplied )
		return;

	/* r_fullbright quando existir; senao r_lightmap 1 */
	light = gEngfuncs.pfnGetCvarPointer( "r_fullbright" );
	if( light )
	{
		gEngfuncs.Cvar_SetValue( "r_fullbright", (float)want );
		g_nightApplied = want;
		return;
	}

	light = gEngfuncs.pfnGetCvarPointer( "r_lightmap" );
	if( light )
	{
		gEngfuncs.Cvar_SetValue( "r_lightmap", (float)want );
		g_nightApplied = want;
	}
}

static void TickSkin( void )
{
	int n, w;

	if( !cv_skin || !cv_skin_wpn )
		return;

	n = (int)cv_skin->value;
	w = (int)cv_skin_wpn->value;

	if( n == g_lastSkin && w == g_lastSkinWpn )
		return;

	g_lastSkin = n;
	g_lastSkinWpn = w;

	{
		const char *name = kSkinWeapons[w];
		char cmd[64];

		if( !name )
			return;

		snprintf( cmd, sizeof( cmd ), "skin_%s %d\n", name, n );
		gEngfuncs.pfnClientCmd( cmd );
	}
}

void SakuraExtra_Tick( void )
{
	TickKills();
	TickNight();
	TickSkin();
}

/*
 * sakura_features.cpp - features do Sakura portadas para o client.
 *
 * Tudo aqui e client-side: movimento (bhop/fastrun/groundstrafe/jumpbug),
 * rage (norecoil/nospread/antiaim), misc (namestealer/chatspammer/antiafk),
 * visuals (crosshair/fov/removals) e ESP textual.
 *
 * Fluxo:
 *   HUD_PostRunCmd -> Sakura_PostRunCmd  (punch, flags, velocidade, seed)
 *   CL_CreateMove  -> Sakura_PreMove     (movimento)
 *   CL_CreateMove  -> Sakura_PostMove    (AA, norecoil, nospread no cmd)
 *   V_CalcRefdef   -> Sakura_CalcRefdef  (fov)
 *   HUD_Redraw     -> Sakura_Draw        (crosshair e ESP)
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "pm_defs.h"
#include "event_api.h"
#include "in_buttons.h"
#include "keydefs.h"
#include "entity_state.h"
#include "usercmd.h"
#include "draw_util.h"
#include "triangleapi.h"
#include "aimbot.h"
#include "spread_predictor.h"
#include "sakura_features.h"
#include "imgui.h"
#include "imgui_host.h"
#include "imgui_menu.h"

#define SAK_PI       3.14159265358979323846f
#define SAK_DEG2RAD  ( SAK_PI / 180.0f )
#define SAK_RAD2DEG  ( 180.0f / SAK_PI )

extern int CL_IsDead( void );
extern int g_iUser1;

/* ------------------------------------------------------------------ */
/* cvars                                                              */
/* ------------------------------------------------------------------ */

/* movimento */
static cvar_t *cv_bhop;
static cvar_t *cv_fastrun;
static cvar_t *cv_groundstrafe;
static cvar_t *cv_jumpbug;

/* rage */
static cvar_t *cv_norecoil;
static cvar_t *cv_nospread;
static cvar_t *cv_antiaim;
static cvar_t *cv_aa_pitch;
static cvar_t *cv_aa_yaw;
static cvar_t *cv_aa_speed;

/* misc */
static cvar_t *cv_namestealer;
static cvar_t *cv_chatspam;
static cvar_t *cv_chatspam_delay;
static cvar_t *cv_chatspam_text;
static cvar_t *cv_antiafk;

/* visuals */
static cvar_t *cv_crosshair;
static cvar_t *cv_crosshair_size;
static cvar_t *cv_crosshair_gap;
static cvar_t *cv_remove_hands;
static cvar_t *cv_fov;

/* esp */
static cvar_t *cv_esp_box;
static cvar_t *cv_esp_name;
static cvar_t *cv_esp_health;
static cvar_t *cv_esp_dist;
static cvar_t *cv_esp_snapline;
static cvar_t *cv_esp_radar;
static cvar_t *cv_esp_team;
static cvar_t *cv_esp_maxdist;

/* debug */
static cvar_t *cv_debug;

/* ------------------------------------------------------------------ */
/* estado local (vindo da predicao)                                   */
/* ------------------------------------------------------------------ */

static float g_punch[3];
static float g_velocity[3];
static int   g_flags;
static float g_origin[3];
static int   g_health = 100;
static int   g_waterlevel;
static unsigned int g_seed;
static double g_lastPred;

/* teclas seguradas */
static int g_keyJump;
static int g_keyDuck;
static int g_keyForward;
static int g_keyBack;
static int g_keyLeft;
static int g_keyRight;

/* movimento */
static int   g_groundFrames;
static int   g_strafeDir = 1;

/* antiafk */
static double g_afkNext;
static int    g_afkSide;

/* chatspammer */
static double g_chatNext;

/* namestealer */
static int    g_nameStealKey;

/* menu */
static int g_menu;
static int g_panel;

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

static double Now( void )
{
	return gEngfuncs.GetClientTime();
}

static int ToScreen( const float *world, float *x, float *y )
{
	float tmp[3];

	tmp[0] = world[0];
	tmp[1] = world[1];
	tmp[2] = world[2];
	return gEngfuncs.pTriAPI->WorldToScreen( tmp, tmp ) == 0 ? 0 : 1;
}

static void Fill( int x, int y, int w, int h, int r, int g, int b, int a )
{
	gEngfuncs.pfnFillRGBA( x, y, w, h, r, g, b, a );
}

static void Line2D( int x0, int y0, int x1, int y1, int r, int g, int b, int a )
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int steps = abs( dx ) > abs( dy ) ? abs( dx ) : abs( dy );
	int i;

	if( steps <= 0 )
	{
		Fill( x0, y0, 1, 1, r, g, b, a );
		return;
	}
	if( steps > 400 )
		steps = 400;

	for( i = 0; i <= steps; i++ )
	{
		float t = (float)i / (float)steps;

		Fill( (int)( x0 + dx * t ), (int)( y0 + dy * t ), 1, 1, r, g, b, a );
	}
}

/* ------------------------------------------------------------------ */
/* init                                                               */
/* ------------------------------------------------------------------ */

static void DrawSakuraMenu( void *user );
static void Cmd_SakuraMenu( void );

static void SetSakuraMenu( int open )
{
	g_menu = open ? 1 : 0;
	ImGui_SetMenuOpen( ( g_menu || ImGuiMenu_IsOpen() ) ? 1 : 0 );
}

static void Cmd_SakuraMenu( void )
{
	SetSakuraMenu( !g_menu );
}

void Sakura_Init( void )
{
	cv_bhop         = gEngfuncs.pfnRegisterVariable( "cl_sak_bhop", "0", FCVAR_ARCHIVE );
	cv_fastrun      = gEngfuncs.pfnRegisterVariable( "cl_sak_fastrun", "0", FCVAR_ARCHIVE );
	cv_groundstrafe = gEngfuncs.pfnRegisterVariable( "cl_sak_groundstrafe", "0", FCVAR_ARCHIVE );
	cv_jumpbug      = gEngfuncs.pfnRegisterVariable( "cl_sak_jumpbug", "0", FCVAR_ARCHIVE );

	cv_norecoil     = gEngfuncs.pfnRegisterVariable( "cl_sak_norecoil", "0", FCVAR_ARCHIVE );
	cv_nospread     = gEngfuncs.pfnRegisterVariable( "cl_sak_nospread", "0", FCVAR_ARCHIVE );
	cv_antiaim      = gEngfuncs.pfnRegisterVariable( "cl_sak_antiaim", "0", FCVAR_ARCHIVE );
	cv_aa_pitch     = gEngfuncs.pfnRegisterVariable( "cl_sak_aa_pitch", "0", FCVAR_ARCHIVE );
	cv_aa_yaw       = gEngfuncs.pfnRegisterVariable( "cl_sak_aa_yaw", "0", FCVAR_ARCHIVE );
	cv_aa_speed     = gEngfuncs.pfnRegisterVariable( "cl_sak_aa_speed", "6", FCVAR_ARCHIVE );

	cv_namestealer  = gEngfuncs.pfnRegisterVariable( "cl_sak_namestealer", "0", FCVAR_ARCHIVE );
	cv_chatspam     = gEngfuncs.pfnRegisterVariable( "cl_sak_chatspam", "0", FCVAR_ARCHIVE );
	cv_chatspam_delay = gEngfuncs.pfnRegisterVariable( "cl_sak_chatspam_delay", "3", FCVAR_ARCHIVE );
	cv_chatspam_text = gEngfuncs.pfnRegisterVariable( "cl_sak_chatspam_text", "sakura on top", FCVAR_ARCHIVE );
	cv_antiafk      = gEngfuncs.pfnRegisterVariable( "cl_sak_antiafk", "0", FCVAR_ARCHIVE );

	cv_crosshair    = gEngfuncs.pfnRegisterVariable( "cl_sak_crosshair", "0", FCVAR_ARCHIVE );
	cv_crosshair_size = gEngfuncs.pfnRegisterVariable( "cl_sak_crosshair_size", "6", FCVAR_ARCHIVE );
	cv_crosshair_gap = gEngfuncs.pfnRegisterVariable( "cl_sak_crosshair_gap", "3", FCVAR_ARCHIVE );
	cv_remove_hands = gEngfuncs.pfnRegisterVariable( "cl_sak_remove_hands", "0", FCVAR_ARCHIVE );
	cv_fov          = gEngfuncs.pfnRegisterVariable( "cl_sak_fov", "0", FCVAR_ARCHIVE );

	cv_esp_box      = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_box", "0", FCVAR_ARCHIVE );
	cv_esp_name     = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_name", "0", FCVAR_ARCHIVE );
	cv_esp_health   = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_health", "0", FCVAR_ARCHIVE );
	cv_esp_dist     = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_dist", "0", FCVAR_ARCHIVE );
	cv_esp_snapline = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_snapline", "0", FCVAR_ARCHIVE );
	cv_esp_radar    = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_radar", "0", FCVAR_ARCHIVE );
	cv_esp_team     = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_team", "0", FCVAR_ARCHIVE );
	cv_esp_maxdist  = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_maxdist", "3000", FCVAR_ARCHIVE );

	cv_debug        = gEngfuncs.pfnRegisterVariable( "cl_sak_debug", "0", FCVAR_ARCHIVE );

	{
		cvar_t *open = gEngfuncs.pfnRegisterVariable( "cl_sak_open", "0", FCVAR_ARCHIVE );

		if( open && open->value > 0.5f )
			SetSakuraMenu( 1 );
	}

	gEngfuncs.Con_Printf( "sakura init: esp_box=%d cross=%d bhop=%d norecoil=%d nospread=%d fov=%.0f menu=%d\n",
		cv_esp_box ? (int)cv_esp_box->value : -1,
		cv_crosshair ? (int)cv_crosshair->value : -1,
		cv_bhop ? (int)cv_bhop->value : -1,
		cv_norecoil ? (int)cv_norecoil->value : -1,
		cv_nospread ? (int)cv_nospread->value : -1,
		cv_fov ? cv_fov->value : -1.f,
		g_menu ? 1 : 0 );

	gEngfuncs.pfnAddCommand( "sakura_menu", Cmd_SakuraMenu );
	gEngfuncs.pfnAddCommand( "menu_sakura", Cmd_SakuraMenu );
	if( !g_panel )
		g_panel = ImGui_RegisterPanel( "sakura", DrawSakuraMenu, NULL );
}

/* ------------------------------------------------------------------ */
/* estado da predicao                                                 */
/* ------------------------------------------------------------------ */

void Sakura_PostRunCmd( local_state_s *from, local_state_s *to,
	usercmd_s *cmd, unsigned int seed )
{
	(void)from;
	(void)cmd;

	if( !to )
		return;

	g_punch[0] = to->client.punchangle[0];
	g_punch[1] = to->client.punchangle[1];
	g_punch[2] = to->client.punchangle[2];
	g_flags = to->client.flags;
	g_waterlevel = to->client.waterlevel;
	g_health = (int)to->client.health;
	g_velocity[0] = to->client.velocity[0];
	g_velocity[1] = to->client.velocity[1];
	g_velocity[2] = to->client.velocity[2];
	g_origin[0] = to->client.origin[0];
	g_origin[1] = to->client.origin[1];
	g_origin[2] = to->client.origin[2];
	g_seed = seed;
	g_lastPred = gEngfuncs.GetClientTime();

	if( g_flags & FL_ONGROUND )
		g_groundFrames++;
	else
		g_groundFrames = 0;
}

/* ------------------------------------------------------------------ */
/* movimento                                                          */
/* ------------------------------------------------------------------ */

static int GroundBelow( float *dist )
{
	pmtrace_t tr;
	float end[3];

	end[0] = g_origin[0];
	end[1] = g_origin[1];
	end[2] = g_origin[2] - 64.0f;

	gEngfuncs.pEventAPI->EV_SetTraceHull( 0 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( g_origin, end, PM_WORLD_ONLY, -1, &tr );

	if( dist )
		*dist = tr.fraction * 64.0f;
	return tr.fraction < 0.999f || tr.startsolid;
}

void Sakura_PreMove( usercmd_s *cmd, float frametime )
{
	float maxspeed;
	int onground;

	(void)frametime;
	if( !cmd )
		return;

	onground = ( g_flags & FL_ONGROUND ) != 0;
	maxspeed = gEngfuncs.GetClientMaxspeed();
	if( maxspeed <= 0.f )
		maxspeed = 250.f;

	/* bhop: reaperta o pulo enquanto a tecla estiver segurada */
	if( On( cv_bhop, 0 ) && g_keyJump )
	{
		if( onground )
			cmd->buttons |= IN_JUMP;
		/* evita o "duck jump" acidental do engine */
		cmd->buttons &= ~IN_DUCK;
	}

	/* fastrun: mantem velocidade cheia em diagonal, sem perder no clamp */
	if( On( cv_fastrun, 0 ) && g_keyForward && onground )
	{
		float f = maxspeed * 0.7071f;

		if( cmd->forwardmove < f )
			cmd->forwardmove = f;
		if( fabsf( cmd->sidemove ) < f && ( g_keyLeft || g_keyRight ) )
			cmd->sidemove = ( cmd->sidemove < 0.f ? -f : f );
	}

	/* groundstrafe: alterna o sidemove para manter a aceleracao no chao */
	if( On( cv_groundstrafe, 0 ) && onground && ( g_keyForward || g_keyBack ) )
	{
		static int frame;

		frame++;
		if( frame % 2 == 0 )
			g_strafeDir = -g_strafeDir;

		cmd->sidemove = maxspeed * 0.6f * (float)g_strafeDir;
		cmd->forwardmove = maxspeed * 0.6f;
	}

	/* jumpbug: duck+jump pouco antes de tocar o chao em queda forte */
	if( On( cv_jumpbug, 0 ) && !onground && g_velocity[2] < -200.f )
	{
		float dist = 999.f;

		if( GroundBelow( &dist ) && dist < 12.f )
		{
			cmd->buttons |= IN_DUCK | IN_JUMP;
		}
	}

	/* antiafk: pequenos movimentos para o servidor nao kickar */
	if( On( cv_antiafk, 0 ) )
	{
		double now = Now();

		if( now >= g_afkNext )
		{
			g_afkNext = now + 20.0;
			g_afkSide = -g_afkSide;
			if( g_afkSide == 0 )
				g_afkSide = 1;
		}
		if( cmd->forwardmove == 0.f && cmd->sidemove == 0.f )
			cmd->sidemove = 60.f * (float)g_afkSide;
	}
}

/* ------------------------------------------------------------------ */
/* antiaim + norecoil + nospread                                      */
/* ------------------------------------------------------------------ */

static void ApplyAntiAim( usercmd_s *cmd, double now )
{
	int pitchMode = (int)CV( cv_aa_pitch, 0.f );
	int yawMode = (int)CV( cv_aa_yaw, 0.f );
	float speed = Clampf( CV( cv_aa_speed, 6.f ), 0.1f, 90.f );
	float spin;
	float jitterP = 0.f, jitterY = 0.f;

	if( pitchMode <= 0 && yawMode <= 0 )
		return;

	spin = (float)now * speed * 90.0f;

	switch( pitchMode )
	{
	case 1: cmd->viewangles[0] = -89.f; break;
	case 2: cmd->viewangles[0] = 89.f; break;
	case 3: cmd->viewangles[0] = ( sinf( (float)now * speed * 3.f ) > 0.f ) ? 89.f : 0.f; break;
	case 4: cmd->viewangles[0] = ( sinf( (float)now * speed * 3.f ) > 0.f ) ? -89.f : 0.f; break;
	case 5: jitterP = ( sinf( (float)now * speed * 8.f ) > 0.f ) ? 30.f : -30.f; break;
	case 6: jitterP = (float)( (int)( now * 1000.0 ) % 120 ) - 60.f; break;
	}

	switch( yawMode )
	{
	case 1: cmd->viewangles[1] += 180.f; break;
	case 2: cmd->viewangles[1] += ( ( (int)( now * 1000.0 ) / 40 ) & 1 ) ? 180.f : 0.f; break;
	case 3: cmd->viewangles[1] += spin * 0.35f; break;
	case 4: cmd->viewangles[1] += spin * 2.5f; break;
	case 5: jitterY = ( sinf( (float)now * speed * 8.f ) > 0.f ) ? 60.f : -60.f; break;
	case 6: cmd->viewangles[1] += 90.f; break;
	case 7: jitterY = (float)( (int)( now * 1000.0 ) % 360 ) - 180.f; break;
	case 8: cmd->viewangles[1] += 180.f; break;
	}

	cmd->viewangles[0] = Clampf( cmd->viewangles[0] + jitterP, -89.f, 89.f );
	cmd->viewangles[1] += jitterY;

	while( cmd->viewangles[1] > 180.f ) cmd->viewangles[1] -= 360.f;
	while( cmd->viewangles[1] < -180.f ) cmd->viewangles[1] += 360.f;
}

void Sakura_PostMove( usercmd_s *cmd, const float viewangles[3] )
{
	double now;

	if( !cmd || !viewangles )
		return;
	if( g_iUser1 != 0 || CL_IsDead() )
		return;

	now = Now();

	/* norecoil: tira o punch do comando (o tiro sai reto) */
	if( On( cv_norecoil, 0 ) )
	{
		cmd->viewangles[0] -= g_punch[0];
		cmd->viewangles[1] -= g_punch[1];
	}

	/* nospread: corrige o offset aleatorio do proximo tiro */
	if( On( cv_nospread, 0 ) )
	{
		float dx = 0.f, dy = 0.f;

		if( SpreadDot_GetSpreadOffset( g_seed, &dx, &dy ) )
		{
			cmd->viewangles[1] += dx * SAK_RAD2DEG;
			cmd->viewangles[0] += dy * SAK_RAD2DEG;

			if( On( cv_debug, 0 ) )
			{
				static double lastPrint;

				if( now - lastPrint > 0.5 )
				{
					lastPrint = now;
					gEngfuncs.Con_Printf( "sak nospread: dx %.4f dy %.4f graus (%.2f / %.2f)\n",
						dx, dy, dx * SAK_RAD2DEG, dy * SAK_RAD2DEG );
				}
			}
		}
	}

	/* antiaim so age quando o aimbot nao trocou os angulos do comando */
	if( On( cv_antiaim, 0 ) )
	{
		float dp = fabsf( cmd->viewangles[0] - viewangles[0] );
		float dyw = fabsf( cmd->viewangles[1] - viewangles[1] );

		while( dyw > 180.f ) dyw = fabsf( dyw - 360.f );
		if( dp < 0.01f && dyw < 0.01f )
			ApplyAntiAim( cmd, now );
	}

	cmd->viewangles[0] = Clampf( cmd->viewangles[0], -89.f, 89.f );
}

/* ------------------------------------------------------------------ */
/* misc                                                               */
/* ------------------------------------------------------------------ */

static void RunMisc( void )
{
	double now = Now();

	/* namestealer: copia o nome do jogador sob a mira (ou do primeiro) */
	if( On( cv_namestealer, 0 ) && g_nameStealKey )
	{
		int i;
		int best = 0;
		float bestDist = 99999.f;
		cl_entity_t *local = gEngfuncs.GetLocalPlayer();

		g_nameStealKey = 0;
		for( i = 1; i <= gEngfuncs.GetMaxClients(); i++ )
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex( i );
			float d;

			if( !ent || !ent->player || !ent->model || !local || i == local->index )
				continue;
			d = ( ent->origin[0] - local->origin[0] ) * ( ent->origin[0] - local->origin[0] ) +
				( ent->origin[1] - local->origin[1] ) * ( ent->origin[1] - local->origin[1] ) +
				( ent->origin[2] - local->origin[2] ) * ( ent->origin[2] - local->origin[2] );
			if( d < bestDist )
			{
				bestDist = d;
				best = i;
			}
		}

		if( best && g_PlayerInfoList[best].name && g_PlayerInfoList[best].name[0] )
		{
			char buf[256];

			snprintf( buf, sizeof( buf ), "name \"%s\"\n", g_PlayerInfoList[best].name );
			gEngfuncs.pfnClientCmd( buf );
			gEngfuncs.Con_Printf( "sak namestealer: nome copiado de %s\n", g_PlayerInfoList[best].name );
		}
	}

	/* chatspammer */
	if( On( cv_chatspam, 0 ) && now >= g_chatNext )
	{
		char buf[320];
		const char *text = cv_chatspam_text && cv_chatspam_text->string
			? cv_chatspam_text->string : "sakura";

		g_chatNext = now + Clampf( CV( cv_chatspam_delay, 3.f ), 0.5f, 120.f );
		snprintf( buf, sizeof( buf ), "say %s\n", text );
		gEngfuncs.pfnClientCmd( buf );
	}
}

/* ------------------------------------------------------------------ */
/* draw: crosshair e esp                                              */
/* ------------------------------------------------------------------ */

static void DrawCrosshair( void )
{
	int cx = ScreenWidth / 2;
	int cy = ScreenHeight / 2;
	int size = (int)Clampf( CV( cv_crosshair_size, 6.f ), 1.f, 40.f );
	int gap = (int)Clampf( CV( cv_crosshair_gap, 3.f ), 0.f, 40.f );
	int a = 220;

	Fill( cx - gap - size, cy, size, 1, 0, 255, 120, a );
	Fill( cx + gap + 1, cy, size, 1, 0, 255, 120, a );
	Fill( cx, cy - gap - size, 1, size, 0, 255, 120, a );
	Fill( cx, cy + gap + 1, 1, size, 0, 255, 120, a );
	Fill( cx, cy, 1, 1, 255, 255, 255, a );
}

static void DrawBox2D( const float c[8][3], int r, int g, int b, int a )
{
	float s[8][2];
	int ok[8];
	int i;
	float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;

	for( i = 0; i < 8; i++ )
	{
		ok[i] = ToScreen( c[i], &s[i][0], &s[i][1] );
		if( ok[i] )
		{
			if( s[i][0] < minx ) minx = s[i][0];
			if( s[i][1] < miny ) miny = s[i][1];
			if( s[i][0] > maxx ) maxx = s[i][0];
			if( s[i][1] > maxy ) maxy = s[i][1];
		}
	}

	if( minx > maxx || miny > maxy )
		return;

	/* limita caixas gigantes (jogador muito perto) */
	if( maxx - minx > ScreenWidth * 1.5f || maxy - miny > ScreenHeight * 1.5f )
		return;

	Line2D( (int)minx, (int)miny, (int)maxx, (int)miny, r, g, b, a );
	Line2D( (int)maxx, (int)miny, (int)maxx, (int)maxy, r, g, b, a );
	Line2D( (int)maxx, (int)maxy, (int)minx, (int)maxy, r, g, b, a );
	Line2D( (int)minx, (int)maxy, (int)minx, (int)miny, r, g, b, a );
}

static void DrawESP( void )
{
	const aim_box_t *boxes;
	int count = 0;
	int i, j;
	int drawn[65];
	float maxdist;
	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	int localTeam = local ? g_PlayerExtraInfo[local->index].teamnumber : 0;
	int showTeam = On( cv_esp_team, 0 );

	memset( drawn, 0, sizeof( drawn ) );
	boxes = Aimbot_GetBoxes( &count );
	if( !boxes || count <= 0 )
		return;

	maxdist = Clampf( CV( cv_esp_maxdist, 3000.f ), 100.f, 20000.f );

	for( i = 0; i < count; i++ )
	{
		int p = boxes[i].player;
		float dist;
		float sx, sy;
		int r = 255, g = 60, b = 60;

		if( p <= 0 || p > 64 || drawn[p] )
			continue;
		if( g_PlayerExtraInfo[p].dead )
			continue;
		if( !showTeam && localTeam != 0 && g_PlayerExtraInfo[p].teamnumber == localTeam )
			continue;

		dist = sqrtf( ( boxes[i].origin[0] - g_origin[0] ) * ( boxes[i].origin[0] - g_origin[0] ) +
			( boxes[i].origin[1] - g_origin[1] ) * ( boxes[i].origin[1] - g_origin[1] ) +
			( boxes[i].origin[2] - g_origin[2] ) * ( boxes[i].origin[2] - g_origin[2] ) );
		if( dist > maxdist )
			continue;

		drawn[p] = 1;

		if( On( cv_esp_box, 0 ) )
		{
			/* desenha a caixa de cada parte habilitada */
			for( j = i; j < count; j++ )
			{
				if( boxes[j].player == p )
					DrawBox2D( boxes[j].c, r, g, b, 170 );
			}
		}

		if( !ToScreen( boxes[i].origin, &sx, &sy ) )
			continue;

		if( On( cv_esp_snapline, 0 ) )
			Line2D( ScreenWidth / 2, ScreenHeight, (int)sx, (int)sy, r, g, b, 90 );

		if( On( cv_esp_name, 0 ) && g_PlayerInfoList[p].name )
			DrawUtils::DrawHudString( (int)sx - 24, (int)sy - 34, (int)sx + 160,
				g_PlayerInfoList[p].name, 255, 255, 255 );

		if( On( cv_esp_health, 0 ) )
		{
			char hp[32];

			snprintf( hp, sizeof( hp ), "HP %d", g_PlayerExtraInfo[p].health );
			DrawUtils::DrawHudString( (int)sx - 24, (int)sy - 22, (int)sx + 160, hp, 80, 255, 120 );
		}

		if( On( cv_esp_dist, 0 ) )
		{
			char ds[32];

			snprintf( ds, sizeof( ds ), "%d m", (int)( dist / 39.37f ) );
			DrawUtils::DrawHudString( (int)sx - 24, (int)sy - 10, (int)sx + 160, ds, 200, 220, 255 );
		}
	}

	/* radar circular no canto superior direito */
	if( On( cv_esp_radar, 0 ) && local )
	{
		int radius = 70;
		int cx = ScreenWidth - radius - 12;
		int cy = radius + 12;
		int n;
		float yaw = -gHUD.m_vecAngles[1] * SAK_DEG2RAD;
		float cyaw = cosf( yaw ), syaw = sinf( yaw );

		for( n = 0; n < 48; n++ )
		{
			float a = (float)n * ( 2.f * SAK_PI / 48.f );

			Fill( cx + (int)( cosf( a ) * radius ), cy + (int)( sinf( a ) * radius ), 1, 1, 80, 255, 140, 120 );
		}

		for( i = 0; i < count; i++ )
		{
			int p = boxes[i].player;
			float dx, dy, px, py;

			if( p <= 0 || p > 64 )
				continue;
			if( g_PlayerExtraInfo[p].dead )
				continue;
			if( !showTeam && localTeam != 0 && g_PlayerExtraInfo[p].teamnumber == localTeam )
				continue;

			drawn[p] = 1;
			dx = boxes[i].origin[0] - g_origin[0];
			dy = boxes[i].origin[1] - g_origin[1];
			px = dx * cyaw - dy * syaw;
			py = dx * syaw + dy * cyaw;

			px = Clampf( px / 24.f, -(float)radius, (float)radius );
			py = Clampf( py / 24.f, -(float)radius, (float)radius );

			Fill( cx + (int)px - 1, cy - (int)py - 1, 3, 3, 255, 60, 60, 220 );
		}
	}
}

static void ApplyFov( void );

void Sakura_Draw( void )
{
	static int openChecked;

	if( !openChecked )
	{
		cvar_t *o = (cvar_t *)gEngfuncs.pfnGetCvarPointer( "cl_sak_open" );

		if( o )
		{
			openChecked = 1;
			if( o->value > 0.5f )
				SetSakuraMenu( 1 );
		}
	}

	ApplyFov();

	if( On( cv_crosshair, 0 ) )
		DrawCrosshair();

	if( On( cv_esp_box, 0 ) || On( cv_esp_name, 0 ) || On( cv_esp_health, 0 ) ||
		On( cv_esp_dist, 0 ) || On( cv_esp_snapline, 0 ) || On( cv_esp_radar, 0 ) )
		DrawESP();

	RunMisc();
}

/* ------------------------------------------------------------------ */
/* teclas, fov, viewmodel                                             */
/* ------------------------------------------------------------------ */

int Sakura_Key( int down, int keynum )
{
	if( keynum == K_SPACE ) g_keyJump = down;
	else if( keynum == K_CTRL ) g_keyDuck = down;
	else if( keynum == K_UPARROW ) g_keyForward = down;
	else if( keynum == K_DOWNARROW ) g_keyBack = down;
	else if( keynum == K_LEFTARROW ) g_keyLeft = down;
	else if( keynum == K_RIGHTARROW ) g_keyRight = down;

	return 1;
}

/* fov custom: mexe no default_fov do HUD (o engine le esse valor) */
static void ApplyFov( void )
{
	static float last = -1.f;
	float fov = CV( cv_fov, 0.f );

	if( fov < 1.f )
	{
		if( last > 0.f )
		{
			gEngfuncs.Cvar_SetValue( "default_fov", 90.f );
			last = -1.f;
		}
		return;
	}

	fov = Clampf( fov, 30.f, 140.f );
	if( fabsf( fov - last ) > 0.01f )
	{
		gEngfuncs.Cvar_SetValue( "default_fov", fov );
		last = fov;
	}
}

int Sakura_DrawViewModel( void )
{
	return On( cv_remove_hands, 0 ) ? 0 : 1;
}

/* ------------------------------------------------------------------ */
/* menu                                                               */
/* ------------------------------------------------------------------ */

static void CheckCVar( const char *label, cvar_t *cv )
{
	bool on = On( cv, 0 ) != 0;

	if( ImGui::Checkbox( label, &on ) && cv && cv->name )
		SetCvar( cv->name, on ? 1.f : 0.f );
}

static void SliderCVar( const char *label, cvar_t *cv, float lo, float hi, const char *fmt )
{
	float v = CV( cv, lo );

	if( ImGui::SliderFloat( label, &v, lo, hi, fmt ) && cv && cv->name )
		SetCvar( cv->name, v );
}

static void ComboCVar( const char *label, cvar_t *cv, const char *items )
{
	int v = (int)CV( cv, 0.f );

	if( ImGui::Combo( label, &v, items ) && cv && cv->name )
		SetCvar( cv->name, (float)v );
}

static void TabMovement( void )
{
	ImGui::TextColored( ImVec4( 1.f, 0.45f, 0.75f, 1.f ), "HNS / movimento" );
	CheckCVar( "Bunnyhop", cv_bhop );
	CheckCVar( "Fastrun", cv_fastrun );
	CheckCVar( "Groundstrafe", cv_groundstrafe );
	CheckCVar( "Jumpbug", cv_jumpbug );
	ImGui::Separator();
	ImGui::TextUnformatted( "Bunnyhop reaperta o pulo enquanto SPACE estiver segurado." );
	ImGui::TextUnformatted( "Jumpbug usa trace no chao e duck+jump no ultimo instante." );
}

static void TabRage( void )
{
	ImGui::TextColored( ImVec4( 1.f, 0.45f, 0.75f, 1.f ), "Rage" );
	CheckCVar( "No recoil", cv_norecoil );
	CheckCVar( "No spread", cv_nospread );
	ImGui::Separator();
	CheckCVar( "Anti-aim", cv_antiaim );
	ComboCVar( "Pitch", cv_aa_pitch, "Off\0Up\0Down\0Fake down\0Fake up\0Jitter\0Random\0" );
	ComboCVar( "Yaw", cv_aa_yaw, "Off\0180\0180 jitter\0Slow spin\0Fast spin\0Jitter\0Sideways\0Random\0Static\0" );
	SliderCVar( "Velocidade do spin", cv_aa_speed, 0.5f, 30.f, "%.1f" );
	ImGui::Separator();
	ImGui::TextUnformatted( "Anti-aim so atua quando o aimbot nao escreve no comando." );
}

static void TabMisc( void )
{
	char buf[128];

	ImGui::TextColored( ImVec4( 1.f, 0.45f, 0.75f, 1.f ), "Misc" );
	CheckCVar( "Anti-AFK", cv_antiafk );
	CheckCVar( "Chat spammer", cv_chatspam );
	SliderCVar( "Intervalo do chat", cv_chatspam_delay, 0.5f, 30.f, "%.1f s" );

	snprintf( buf, sizeof( buf ), "%s", cv_chatspam_text && cv_chatspam_text->string
		? cv_chatspam_text->string : "" );
	if( ImGui::InputText( "Mensagem", buf, sizeof( buf ) ) )
		gEngfuncs.Cvar_Set( "cl_sak_chatspam_text", buf );

	ImGui::Separator();
	CheckCVar( "Name stealer (F7 copia o mais perto)", cv_namestealer );
	if( ImGui::Button( "Copiar nome do mais proximo" ) )
		g_nameStealKey = 1;
	ImGui::Separator();
	CheckCVar( "Debug no console", cv_debug );
}

static void TabVisuals( void )
{
	ImGui::TextColored( ImVec4( 1.f, 0.45f, 0.75f, 1.f ), "Visuals" );
	CheckCVar( "Crosshair custom", cv_crosshair );
	SliderCVar( "Tamanho", cv_crosshair_size, 1.f, 20.f, "%.0f" );
	SliderCVar( "Espaco", cv_crosshair_gap, 0.f, 20.f, "%.0f" );
	ImGui::Separator();
	CheckCVar( "Remover maos (viewmodel)", cv_remove_hands );
	SliderCVar( "FOV (0 = padrao)", cv_fov, 0.f, 140.f, "%.0f" );
}

static void TabESP( void )
{
	ImGui::TextColored( ImVec4( 1.f, 0.45f, 0.75f, 1.f ), "ESP" );
	CheckCVar( "Caixa", cv_esp_box );
	CheckCVar( "Nome", cv_esp_name );
	CheckCVar( "Vida", cv_esp_health );
	CheckCVar( "Distancia", cv_esp_dist );
	CheckCVar( "Snapline", cv_esp_snapline );
	CheckCVar( "Radar", cv_esp_radar );
	ImGui::Separator();
	CheckCVar( "Mostrar time", cv_esp_team );
	SliderCVar( "Distancia maxima", cv_esp_maxdist, 100.f, 8000.f, "%.0f" );
}

static void DrawSakuraMenu( void * )
{
	static int logged;

	if( !g_menu )
		return;
	if( !logged )
	{
		logged = 1;
		gEngfuncs.Con_Printf( "sakura menu: painel desenhando\n" );
	}

	ImGui::SetNextWindowPos( ImVec2( 500.0f, 48.0f ), ImGuiCond_FirstUseEver );
	ImGui::SetNextWindowSize( ImVec2( 460.0f, 0.0f ), ImGuiCond_Always );
	ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
	ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.02f, 0.0f, 0.05f, 0.86f ) );
	ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.16f, 0.10f, 0.18f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_SliderGrab, ImVec4( 1.0f, 0.45f, 0.75f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_CheckMark, ImVec4( 1.0f, 0.45f, 0.75f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_Tab, ImVec4( 0.20f, 0.08f, 0.24f, 1.0f ) );
	ImGui::PushStyleColor( ImGuiCol_TabActive, ImVec4( 0.55f, 0.16f, 0.42f, 1.0f ) );

	if( !ImGui::Begin( "Sakura", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize ) )
	{
		ImGui::End();
		ImGui::PopStyleColor( 7 );
		ImGui::PopStyleVar();
		return;
	}

	ImGui::TextColored( ImVec4( 1.0f, 0.45f, 0.75f, 1.0f ),
		"sakura_menu / menu_sakura  |  ESC fecha" );

	if( ImGui::BeginTabBar( "##saktabs" ) )
	{
		if( ImGui::BeginTabItem( "Movemento" ) ) { TabMovement(); ImGui::EndTabItem(); }
		if( ImGui::BeginTabItem( "Rage" ) ) { TabRage(); ImGui::EndTabItem(); }
		if( ImGui::BeginTabItem( "Misc" ) ) { TabMisc(); ImGui::EndTabItem(); }
		if( ImGui::BeginTabItem( "Visuals" ) ) { TabVisuals(); ImGui::EndTabItem(); }
		if( ImGui::BeginTabItem( "ESP" ) ) { TabESP(); ImGui::EndTabItem(); }
		ImGui::EndTabBar();
	}

	if( ImGui::Button( "Fechar" ) )
		SetSakuraMenu( 0 );

	ImGui::End();
	ImGui::PopStyleColor( 7 );
	ImGui::PopStyleVar();
}

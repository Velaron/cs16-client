/*
 * sakura_more.cpp - strafe automatico, notify (toasts) e route (waypoints).
 *
 * Regras do projeto:
 *   - a UI do HUB so escreve cvars; a feature vive aqui;
 *   - nada de chamada de engine fora da thread principal;
 *   - comentarios em portugues sem acento.
 */

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <vector>

#include "hud.h"
#include "cl_util.h"
#include "keydefs.h"
#include "pm_defs.h"
#include "in_buttons.h"
#include "usercmd.h"
#include "entity_state.h"
#include "triangleapi.h"
#include "sakura_features.h"
#include "sakura_more.h"

/* ------------------------------------------------------------------ */
/* estado                                                             */
/* ------------------------------------------------------------------ */

static cvar_t *cv_strafe;
static cvar_t *cv_strafe_speed;
static cvar_t *cv_notify;
static cvar_t *cv_notify_time;
static cvar_t *cv_route_draw;
static cvar_t *cv_route_record;

struct toast_t
{
	char text[128];
	double until;
};

static std::vector<toast_t> g_toasts;

struct wp_t
{
	float p[3];
};

static std::vector<wp_t> g_route;
static float g_lastWp[3];
static int g_haveLastWp;
static double g_lastSave;

/* ------------------------------------------------------------------ */
/* utilidades de desenho                                              */
/* ------------------------------------------------------------------ */

static void Fill( int x, int y, int w, int h, int r, int g, int b, int a )
{
	if( w <= 0 || h <= 0 )
		return;
	gEngfuncs.pfnFillRGBA( x, y, w, h, r, g, b, a );
}

static void Line2D( int x0, int y0, int x1, int y1, int r, int g, int b, int a )
{
	int dx = x1 - x0;
	int dy = y1 - y0;
	int steps, i;

	if( abs( dx ) > abs( dy ) )
		steps = abs( dx );
	else
		steps = abs( dy );

	if( steps <= 0 )
	{
		Fill( x0, y0, 1, 1, r, g, b, a );
		return;
	}

	for( i = 0; i <= steps; i++ )
	{
		int x = x0 + dx * i / steps;
		int y = y0 + dy * i / steps;

		Fill( x, y, 2, 2, r, g, b, a );
	}
}

static int WorldToScreen( const float *world, float *sx, float *sy )
{
	float tmp[3];

	tmp[0] = world[0];
	tmp[1] = world[1];
	tmp[2] = world[2];

	if( gEngfuncs.pTriAPI->WorldToScreen( tmp, tmp ) == 0 )
		return 0;

	*sx = tmp[0];
	*sy = tmp[1];
	return 1;
}

/* ------------------------------------------------------------------ */
/* notify (toasts)                                                    */
/* ------------------------------------------------------------------ */

void Notify_Push( const char *fmt, ... )
{
	toast_t t;
	va_list ap;

	if( cv_notify && cv_notify->value < 0.5f )
		return;
	if( !fmt )
		return;

	va_start( ap, fmt );
	vsnprintf( t.text, sizeof( t.text ), fmt, ap );
	va_end( ap );

	t.until = gEngfuncs.GetClientTime() + ( cv_notify_time ? cv_notify_time->value : 3.f );

	g_toasts.push_back( t );

	/* fila curta: no maximo 5 mensagens na tela */
	if( g_toasts.size() > 5 )
		g_toasts.erase( g_toasts.begin() );
}

static void DrawToasts( void )
{
	double now = gEngfuncs.GetClientTime();
	int y = 24;
	size_t i;

	for( i = 0; i < g_toasts.size(); )
	{
		toast_t &t = g_toasts[i];
		int len;

		if( now >= t.until )
		{
			g_toasts.erase( g_toasts.begin() + i );
			continue;
		}

		len = (int)strlen( t.text );
		Fill( ScreenWidth - 16 - len * 7, y - 3, len * 7 + 8, 16, 10, 12, 15, 200 );
		gEngfuncs.pfnDrawString( ScreenWidth - 12 - len * 7, y, t.text, 255, 220, 120 );
		y += 20;
		i++;
	}
}

/* ------------------------------------------------------------------ */
/* route (waypoints)                                                  */
/* ------------------------------------------------------------------ */

static void RouteClear( void )
{
	g_route.clear();
	g_haveLastWp = 0;
}

static void RouteRecord( const float *org )
{
	wp_t w;

	if( !g_haveLastWp )
	{
		g_lastWp[0] = org[0];
		g_lastWp[1] = org[1];
		g_lastWp[2] = org[2];
		g_haveLastWp = 1;
	}

	{
		float dx = org[0] - g_lastWp[0];
		float dy = org[1] - g_lastWp[1];
		float dz = org[2] - g_lastWp[2];

		if( ( dx * dx + dy * dy + dz * dz ) < 64.f * 64.f )
			return;
	}

	w.p[0] = org[0];
	w.p[1] = org[1];
	w.p[2] = org[2];
	g_route.push_back( w );

	g_lastWp[0] = org[0];
	g_lastWp[1] = org[1];
	g_lastWp[2] = org[2];

	if( g_route.size() > 4096 )
		g_route.erase( g_route.begin() );
}

static void RouteSave( void )
{
	const char *level = gEngfuncs.pfnGetLevelName();
	const char *gamedir = gEngfuncs.pfnGetGameDirectory();
	char name[64];
	char path[512];
	FILE *f;
	size_t i;

	if( !level || !gamedir || g_route.empty() )
		return;

	/* "maps/cs_747.bsp" -> "cs_747" */
	{
		const char *base = strrchr( level, '/' );
		const char *dot;

		base = base ? base + 1 : level;
		strncpy( name, base, sizeof( name ) - 1 );
		name[sizeof( name ) - 1] = 0;
		dot = strrchr( name, '.' );
		if( dot )
			( (char *)dot )[0] = 0;
	}

	snprintf( path, sizeof( path ), "%s/routes/%s.txt", gamedir, name );
	f = fopen( path, "w" );
	if( !f )
		return;

	for( i = 0; i < g_route.size(); i++ )
		fprintf( f, "%.1f %.1f %.1f\n", g_route[i].p[0], g_route[i].p[1], g_route[i].p[2] );
	fclose( f );

	Notify_Push( "rota salva: %d pontos", (int)g_route.size() );
}

static void DrawRoute( void )
{
	size_t i;

	if( g_route.size() < 2 )
		return;

	for( i = 1; i < g_route.size(); i++ )
	{
		float ax, ay, bx, by;

		if( !WorldToScreen( g_route[i - 1].p, &ax, &ay ) )
			continue;
		if( !WorldToScreen( g_route[i].p, &bx, &by ) )
			continue;

		Line2D( (int)ax, (int)ay, (int)bx, (int)by, 255, 20, 147, 200 );
	}
}

/* ------------------------------------------------------------------ */
/* strafe automatico                                                  */
/* ------------------------------------------------------------------ */

static void DoStrafe( usercmd_s *cmd )
{
	float vel[3], org[3];
	float fwd[3], right[3];
	float vr, vf;
	float speed, limit;
	int flags;

	if( !cv_strafe || cv_strafe->value < 0.5f )
		return;

	Sakura_GetState( vel, org, &flags );

	/* so no ar: no chao o strafe normal e melhor */
	if( flags & FL_ONGROUND )
		return;

	/* se o usuario esta strafando na mao, nao atrapalha */
	if( cmd->buttons & ( IN_MOVELEFT | IN_MOVERIGHT ) )
		return;

	speed = sqrtf( vel[0] * vel[0] + vel[1] * vel[1] );
	if( speed < 30.f )
		return;

	AngleVectors( cmd->viewangles, fwd, right, NULL );

	vf = vel[0] * fwd[0] + vel[1] * fwd[1];
	vr = vel[0] * right[0] + vel[1] * right[1];

	limit = cv_strafe_speed ? cv_strafe_speed->value : 400.f;
	if( limit < 100.f ) limit = 100.f;
	if( limit > 600.f ) limit = 600.f;

	/* empurra para o lado oposto ao desvio lateral: wishdir ~ perpendicular */
	cmd->sidemove = ( vr > 0.f ) ? -limit : limit;

	/* sem isso o wishdir nao muda e a aceleracao nao acontece */
	if( fabs( vf ) < 5.f )
		cmd->forwardmove = 0.f;
}

/* ------------------------------------------------------------------ */
/* api                                                                */
/* ------------------------------------------------------------------ */

static void Cmd_RouteClear( void )
{
	RouteClear();
	Notify_Push( "rota limpa" );
}

void SakuraMore_Init( void )
{
	cv_strafe         = gEngfuncs.pfnRegisterVariable( "cl_sak_strafe", "0", FCVAR_ARCHIVE );
	cv_strafe_speed   = gEngfuncs.pfnRegisterVariable( "cl_sak_strafe_speed", "400", FCVAR_ARCHIVE );
	cv_notify         = gEngfuncs.pfnRegisterVariable( "cl_sak_notify", "1", FCVAR_ARCHIVE );
	cv_notify_time    = gEngfuncs.pfnRegisterVariable( "cl_sak_notify_time", "3", FCVAR_ARCHIVE );
	cv_route_draw     = gEngfuncs.pfnRegisterVariable( "cl_sak_route", "0", FCVAR_ARCHIVE );
	cv_route_record   = gEngfuncs.pfnRegisterVariable( "cl_sak_route_record", "0", FCVAR_ARCHIVE );

	gEngfuncs.pfnAddCommand( "route_clear", Cmd_RouteClear );
}

void SakuraMore_PreMove( usercmd_s *cmd )
{
	float vel[3], org[3];
	int flags;

	DoStrafe( cmd );

	Sakura_GetState( vel, org, &flags );

	if( cv_route_record && cv_route_record->value > 0.5f )
	{
		RouteRecord( org );

		/* salva a cada 5 s para nao perder a rota num crash */
		if( gEngfuncs.GetClientTime() - g_lastSave > 5.0 )
		{
			g_lastSave = gEngfuncs.GetClientTime();
			RouteSave();
		}
	}
}

void SakuraMore_Draw( void )
{
	if( cv_route_draw && cv_route_draw->value > 0.5f )
		DrawRoute();

	DrawToasts();
}

/*
aimassist.cpp -- debug overlay + target highlight for the gamepad aim assist
*/

#define _USE_MATH_DEFINES // for M_PI (used by DEG2RAD on MSVC)
#include "stdio.h"
#include "math.h"
#include "hud.h"
#include "cl_util.h"
#include "draw_util.h"
#include "triangleapi.h"
#include "cl_entity.h"
#include "aimassist.h"

// Project a world point to screen pixels. Returns false if behind the viewer.
static bool AA_Project( float *world, int &sx, int &sy )
{
	vec3_t screen;
	if( gEngfuncs.pTriAPI->WorldToScreen( world, screen ) )
		return false;
	sx = (int)XPROJECT( screen[0] );
	sy = (int)YPROJECT( screen[1] );
	return true;
}

int CHudAimAssist::Init( void )
{
	gHUD.AddHudElem( this );
	m_iFlags = HUD_DRAW;
	return 1;
}

int CHudAimAssist::VidInit( void )
{
	return 1;
}

int CHudAimAssist::Draw( float flTime )
{
	if( !aim_assist_debug || !aim_assist_debug->value )
		return 1;

	int lh = gHUD.m_iFontHeight + 1;
	int x  = XRES( 8 );
	int y  = YRES( 70 );
	char line[128];

	// background box
	FillRGBA( x - 4, y - 4, XRES( 240 ), lh * 5 + 8, 0, 0, 0, 150 );

	DrawUtils::DrawHudString( x, y, ScreenWidth, "AIM ASSIST DEBUG", 255, 200, 0 );
	y += lh;

	snprintf( line, sizeof( line ), "enabled: %s   key: %s",
		( aim_assist && aim_assist->value ) ? "ON" : "OFF",
		g_bAimAssistKey ? "HELD" : "released" );
	DrawUtils::DrawHudString( x, y, ScreenWidth, line, 255, 255, 255 );
	y += lh;

	if( g_iAimAssistTarget )
	{
		const char *name = g_PlayerInfoList[g_iAimAssistTarget].name;
		snprintf( line, sizeof( line ), "target: %s (#%d)  d=%.0f a=%.1f",
			name ? name : "?", g_iAimAssistTarget, g_flAimAssistDist, g_flAimAssistAngle );
		DrawUtils::DrawHudString( x, y, ScreenWidth, line, 0, 255, 0 );
	}
	else
	{
		DrawUtils::DrawHudString( x, y, ScreenWidth, "target: none", 255, 100, 100 );
	}
	y += lh;

	if( g_iAimAssistNearestIdx )
		snprintf( line, sizeof( line ), "nearest enemy: %.1f deg (lock_fov %.0f)",
			g_flAimAssistNearestAngle, aim_assist_lock_fov ? aim_assist_lock_fov->value : 0.0f );
	else
		snprintf( line, sizeof( line ), "nearest enemy: none (team/alive/range)" );
	DrawUtils::DrawHudString( x, y, ScreenWidth, line, 200, 200, 200 );
	y += lh;

	snprintf( line, sizeof( line ), "applying: %s", g_bAimAssistApplying ? "YES" : "no" );
	DrawUtils::DrawHudString( x, y, ScreenWidth, line, 255, 255, 255 );

	// --- view cone + computed aim direction (to diagnose eye/forward) ---
	// magenta dot = where the assist thinks you aim (eye + fwd). It MUST sit on
	// your crosshair (screen center); if it is off-center, eye/forward is wrong.
	{
		vec3_t p;
		int px, py;
		for( int j = 0; j < 3; j++ )
			p[j] = g_vecAimEye[j] + g_vecAimFwd[j] * 300.0f;
		if( AA_Project( p, px, py ) )
			FillRGBA( px - 3, py - 3, 6, 6, 255, 0, 255, 255 );

		// cone edge ring at the acquisition fov, projected into the world
		float fov = aim_assist_lock_fov ? aim_assist_lock_fov->value : 45.0f;
		float cf = cos( DEG2RAD( fov ) ), sf = sin( DEG2RAD( fov ) );
		for( int k = 0; k < 24; k++ )
		{
			float a  = DEG2RAD( k * 15.0f );
			float ca = cos( a ), sa = sin( a );
			vec3_t dir, q;
			int qx, qy;
			for( int j = 0; j < 3; j++ )
			{
				dir[j] = g_vecAimFwd[j] * cf + ( g_vecAimRight[j] * ca + g_vecAimUp[j] * sa ) * sf;
				q[j]   = g_vecAimEye[j] + dir[j] * 300.0f;
			}
			if( AA_Project( q, qx, qy ) )
				FillRGBA( qx - 1, qy - 1, 3, 3, 0, 200, 255, 220 );
		}
	}

	// nearest enemy (yellow) even if outside the cone -- shows where it projects
	if( g_iAimAssistNearestIdx )
	{
		cl_entity_t *n = gEngfuncs.GetEntityByIndex( g_iAimAssistNearestIdx );
		int nx, ny;
		if( n && AA_Project( n->curstate.origin, nx, ny ) )
			FillRGBA( nx - 4, ny - 4, 8, 8, 255, 255, 0, 220 );
	}

	// floating marker over the target's head
	if( g_iAimAssistTarget )
	{
		cl_entity_t *t = gEngfuncs.GetEntityByIndex( g_iAimAssistTarget );
		if( t )
		{
			vec3_t head, screen;
			VectorCopy( t->origin, head );
			head[2] += 40.0f; // approx. above the head

			// WorldToScreen returns non-zero when the point is behind the viewer
			if( gEngfuncs.pTriAPI->WorldToScreen( head, screen ) == 0 )
			{
				int sx = XPROJECT( screen[0] );
				int sy = YPROJECT( screen[1] );
				int s  = XRES( 5 );
				int r  = g_bAimAssistApplying ? 0   : 255;
				int g  = g_bAimAssistApplying ? 255 : 200;
				FillRGBA( sx - s, sy - s, s * 2, s * 2, r, g, 0, 220 );
			}
		}
	}

	return 1;
}

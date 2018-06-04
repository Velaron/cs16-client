// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
#include "events.h"
#include "triangleapi.h"

#include "com_model.h"
#include "com_weapons.h"
#include "cloud.tga.h"
#define SMOKE_CLOUDS 20

struct smokecloud_t
{
	Vector origin;
	Vector direction;
	colorVec color;
	float fadetime;
	float fadespeed;
	bool active;
};

static smokecloud_t g_clouds[SMOKE_CLOUDS];
static int g_hCloudTexture, width, height;
static float left, right, up, down, scale;

void EV_InitSmokeRenderer( void )
{
	g_hCloudTexture = gRenderAPI.GL_LoadTexture( "cloud.tga", cloud1_tga, cloud1_tga_len, 0 );

	width = gRenderAPI.RenderGetParm( PARM_TEX_WIDTH, g_hCloudTexture );
	height = gRenderAPI.RenderGetParm( PARM_TEX_HEIGHT, g_hCloudTexture );

	left = -width / 2.0f;
	right = left + width;

	down = -height / 2.0f;
	up = down + height;

	scale = 1.0f;
}

void EV_ShutdownSmokeRenderer( void )
{
	gRenderAPI.GL_FreeTexture( g_hCloudTexture );
}

bool EV_IsSmokeCulled( smokecloud_t *cloud )
{
	Vector mins, maxs;

	mins[0] = mins[1] = -width / 2;
	maxs[0] = maxs[1] = width / 2;
	mins[2] = -height / 2;
	maxs[2] = height / 2;

	mins = mins + cloud->origin;
	maxs = maxs + cloud->origin;

	return !gEngfuncs.pTriAPI->BoxInPVS( mins, maxs );
}

void EV_RenderSmoke( void )
{
	Vector f, r, u, point;
	cl_entity_t *cl;

	gRenderAPI.GL_Bind( 0, g_hCloudTexture );

	cl = gEngfuncs.GetLocalPlayer();

	if( !cl )
		return;

	for( int i = 0; i < SMOKE_CLOUDS; i++ )
	{
		if( !g_clouds[i].active )
			continue;

		if( EV_IsSmokeCulled( g_clouds + i ))
		{
			// gEngfuncs.Con_NPrintf( i, "cloud is not visible" );
			continue;
		}
		// gEngfuncs.Con_NPrintf( i, "cloud is visible" );

		AngleVectors( v_angles, f, r, u );


		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
		gEngfuncs.pTriAPI->Brightness( 1.0f );
		gEngfuncs.pTriAPI->Color4ub( g_clouds[i].color.r, g_clouds[i].color.g, g_clouds[i].color.b, g_clouds[i].color.a );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		gEngfuncs.pTriAPI->TexCoord2f( 0.0f, 1.0f );
		VectorMA( g_clouds[i].origin, down * scale, u, point );
		VectorMA( point, left * scale, r, point );
		gEngfuncs.pTriAPI->Vertex3fv( point );

		gEngfuncs.pTriAPI->TexCoord2f( 0.0f, 0.0f );
		VectorMA( g_clouds[i].origin, up * scale, u, point );
		VectorMA( point, left * scale, r, point );
		gEngfuncs.pTriAPI->Vertex3fv( point );

		gEngfuncs.pTriAPI->TexCoord2f( 1.0f, 0.0f );
		VectorMA( g_clouds[i].origin, up * scale, u, point );
		VectorMA( point, right * scale, r, point );
		gEngfuncs.pTriAPI->Vertex3fv( point );

		gEngfuncs.pTriAPI->TexCoord2f( 1.0f, 1.0f );
		VectorMA( g_clouds[i].origin, down * scale, u, point );
		VectorMA( point, right * scale, r, point );
		gEngfuncs.pTriAPI->Vertex3fv( point );

		gEngfuncs.pTriAPI->End();
	}
}

void EV_AnimateSmoke( void )
{
	float currenttime = gHUD.m_flTime;

	for( int i = 0; i < SMOKE_CLOUDS; i++ )
	{
		if( !g_clouds[i].active )
			continue;

		if( g_clouds[i].color.a > 0 && currenttime >= g_clouds[i].fadetime )
		{
			g_clouds[i].color.a = 150 - (currenttime - g_clouds[i].fadetime) * g_clouds[i].fadespeed;
			if( g_clouds[i].color.a <= 0 )
			{
				g_clouds[i].color.a = 0;
				g_clouds[i].active = false;
				continue;
			}
		}

		// move
		g_clouds[i].origin = g_clouds[i].origin + g_clouds[i].direction * gHUD.m_flTimeDelta;
	}
}

void EV_CleanupSmoke( void )
{
	for( int i = 0; i < SMOKE_CLOUDS; i++ )
		g_clouds[i].active = false;
}

void EV_CreateSmoke(event_args_s *args)
{
	if( !args->bparam2 ) //first explosion
	{
		for( int i = 0; i < SMOKE_CLOUDS; i++ )
		{
			// randomize smoke cloud position
			Vector org = args->origin;
			if( i != 0 )
			{
				org.x += Com_RandomFloat(-100.0f, 100.0f);
				org.y += Com_RandomFloat(-100.0f, 100.0f);
			}
			org.z += 30;

			g_clouds[i].active = true;
			g_clouds[i].origin = org;
			g_clouds[i].color.b =
					g_clouds[i].color.g =
					g_clouds[i].color.r = Com_RandomLong(210, 230);
			g_clouds[i].color.a = 150;
			g_clouds[i].direction.x = Com_RandomFloat(-5, 5);
			g_clouds[i].direction.y = Com_RandomFloat(-5, 5);
			g_clouds[i].direction.z = Com_RandomFloat(-5, 5);
			g_clouds[i].fadetime = gEngfuncs.GetClientTime() + 5.0f;
			g_clouds[i].fadespeed = Com_RandomFloat( 5, 7 );
		}
	}
	else // second and other
	{
		int g = gEngfuncs.pfnRandomLong(155, 175);

		Vector dir( args->fparam1, args->fparam2, 0.0f );
		Vector vel( 0.0f, 0.0f, 0.0f );

		EV_CS16Client_CreateSmoke( SMOKE_BLACK, args->origin, dir, args->iparam1 / 100, 1.0f, g, g, g, true, vel, 25 );
	}
}

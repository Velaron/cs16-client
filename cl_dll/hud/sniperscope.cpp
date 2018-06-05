// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
hud_overlays.cpp - HUD Overlays
Copyright (C) 2015-2016 a1batross

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.

*/

#include "hud.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "cl_util.h"

#include "draw_util.h"

int CHudSniperScope::Init()
{
	if( g_iXash )
		gHUD.AddHudElem(this);

	m_iFlags = HUD_DRAW;
	m_iScopeArc[0] = m_iScopeArc[1] =m_iScopeArc[2] = m_iScopeArc[3]  = 0;
	return 1;
}

void DrawCircle( byte *buf, int radius, int x0, int y0, int w, int h )
{
	int x = radius-1, y = 0, dx = 1, dy = 1;
	int err = dx - (radius << 1);

	while( x >= y )
	{
#define Draw( x, y ) \
			buf[ ((y) * w + (x)) * 4 + 0 ] = 0; \
			buf[ ((y) * w + (x)) * 4 + 1 ] = 0; \
			buf[ ((y) * w + (x)) * 4 + 2 ] = 0; \
			buf[ ((y) * w + (x)) * 4 + 3 ] = 255;

		Draw( x0 + x, y0 + y );
		Draw( x0 + y, y0 + x );
		Draw( x0 - y, y0 + x );
		Draw( x0 - x, y0 + y );
		Draw( x0 - x, y0 - y );
		Draw( x0 - y, y0 - x );
		Draw( x0 + y, y0 - x );
		Draw( x0 + x, y0 - y );
#undef Draw
		if( err <= 0 )
		{
			y++;
			err += dy;
			dy += 2;
		}

		if( err > 0 )
		{
			x--;
			dx += 2;
			err += dx - (radius << 1);
		}
	}
}

void SniperScope_InitBitmap( int w, int h, byte *buf )
{
	int radius = ( w - 1 ) * 0.45;

	// TODO: optimize!
	memset( buf, 0, w * h * 4 );
	DrawCircle( buf, radius, w / 2, w / 2, w, h );

	for( int i = 0; i < h; i++ )
	{
		// from left to right
		for( int j = 0; j < w; j++ )
		{
			if( buf[ (i * w + j) * 4 + 3] )
				break;

			buf[ (i * w + j) * 4 + 0 ] = 0;
			buf[ (i * w + j) * 4 + 1 ] = 0;
			buf[ (i * w + j) * 4 + 2 ] = 0;
			buf[ (i * w + j) * 4 + 3 ] = 255;
		}

		// from right to left
		for( int j = w - 1; j >= 0; j-- )
		{
			if( buf[ (i * w + j) * 4 + 3] )
				break;

			buf[ (i * w + j) * 4 + 0 ] = 0;
			buf[ (i * w + j) * 4 + 1 ] = 0;
			buf[ (i * w + j) * 4 + 2 ] = 0;
			buf[ (i * w + j) * 4 + 3 ] = 255;
		}
	}
}

int CHudSniperScope::InitBuiltinTextures( void )
{
	texFlags_t defFlags = (texFlags_t)(TF_HAS_ALPHA);

	if( m_bBuiltinInitialized )
		return 1;

	char *buffer = (char*)malloc(512*512*4);

	int w = min( TrueWidth, 512 );

	const struct
	{
		const char	*name;
		byte	*buf;
		int		*texnum;
		int		w, h;
		void	(*init)( int w, int h, byte *buf );
	}
	textures[] =
	{
	{ "scope", (byte*)buffer, &m_iScopeArc[0], w, w, SniperScope_InitBitmap },
	};
	size_t	i, num_builtin_textures = sizeof( textures ) / sizeof( textures[0] );

	for( i = 0; i < num_builtin_textures; i++ )
	{
		textures[i].init( textures[i].w, textures[i].h, textures[i].buf );
		*textures[i].texnum = gRenderAPI.GL_CreateTexture( textures[i].name, textures[i].w, textures[i].h, textures[i].buf, defFlags );
		if( *textures[i].texnum == 0 )
		{
			// it's maybe safer to leave texture render uninitialized and use classic fillrgba
			for( size_t j = 0; j < i; j++ )
			{
				gRenderAPI.GL_FreeTexture( *textures[j].texnum );
			}

			free(buffer);
			return 0;
		}
	}

	m_bBuiltinInitialized = true;

	free( buffer );
	return 1;
}

int CHudSniperScope::VidInit()
{
	if( g_iXash == 0 )
	{
		ConsolePrint("^3No Xash Found Warning^7: CHudSniperScope is disabled!\n");
		m_iFlags = 0;
		return 0;
	}

	if( !InitBuiltinTextures() )
	{
		m_iScopeArc[0] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_nw.tga", NULL, 0, TF_NEAREST |TF_NOMIPMAP|TF_CLAMP);
		m_iScopeArc[1] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_ne.tga", NULL, 0, TF_NEAREST |TF_NOMIPMAP|TF_CLAMP);
		m_iScopeArc[2] = gRenderAPI.GL_LoadTexture("sprites/scope_arc.tga",    NULL, 0, TF_NEAREST |TF_NOMIPMAP|TF_CLAMP);
		m_iScopeArc[3] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_sw.tga", NULL, 0, TF_NEAREST |TF_NOMIPMAP|TF_CLAMP);

		if( !m_iScopeArc[0] || !m_iScopeArc[1] || !m_iScopeArc[2] || !m_iScopeArc[3] )
		{
			gRenderAPI.Host_Error( "^3Error^7: Cannot load Sniper Scope arcs. Check sprites/scope_arc*.tga files\n" );
		}

		m_bUseBuiltin = false;
	}
	else m_bUseBuiltin = true;

	left = (TrueWidth - TrueHeight)/2.0;
	right = left + TrueHeight;
	centerx = TrueWidth/2.0;
	centery = TrueHeight/2.0;
	return 1;
}

inline void DrawTexture( int tex, float x1, float y1, float x2, float y2 )
{
	gRenderAPI.GL_Bind( 0, tex );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	DrawUtils::Draw2DQuad( x1, y1, x2, y2 );
	gEngfuncs.pTriAPI->End();
}

int CHudSniperScope::Draw(float flTime)
{
	if( gHUD.m_iFOV > 40 )
		return 1;

	gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
	gEngfuncs.pTriAPI->Brightness(1.0);
	gEngfuncs.pTriAPI->Color4ub(0, 0, 0, 255);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	gRenderAPI.GL_SelectTexture(0);

	if( m_bUseBuiltin )
	{
		DrawTexture( m_iScopeArc[0], left, 0, right, TrueHeight );
	}
	else
	{
		DrawTexture( m_iScopeArc[0], left, 0, centerx, centery );
		DrawTexture( m_iScopeArc[1], centerx, 0, right, centery );
		DrawTexture( m_iScopeArc[2], centerx, centery, right, TrueHeight );
		DrawTexture( m_iScopeArc[3], left, centery, centerx, TrueHeight );
	}

	gRenderAPI.GL_Bind( 0, gHUD.m_WhiteTex );
	DrawUtils::Draw2DQuad( 0, 0, left + 2, TrueHeight );
	DrawUtils::Draw2DQuad( right, 0, right + ( TrueWidth - right ), TrueHeight );

	// default crosshair pixel perfect lines
	DrawUtils::Draw2DQuad( left, centery + 1, right, centery + 2 );
	DrawUtils::Draw2DQuad( centerx - 1, 0, centerx, TrueHeight );

	return 0;
}

void CHudSniperScope::Shutdown()
{
	if( m_bUseBuiltin )
	{
		gRenderAPI.GL_FreeTexture( m_iScopeArc[0] );
	}
	else
	{
		for( int i = 0; i < 4; i++ )
			gRenderAPI.GL_FreeTexture( m_iScopeArc[i] );
	}
}

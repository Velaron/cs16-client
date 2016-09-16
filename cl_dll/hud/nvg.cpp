/*
nvg.cpp - Night Vision Googles
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
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "dlight.h"

DECLARE_MESSAGE(m_NVG, NVGToggle)
DECLARE_COMMAND(m_NVG, NVGAdjustDown)
DECLARE_COMMAND(m_NVG, NVGAdjustUp)

int CHudNVG::Init()
{
	HOOK_MESSAGE(NVGToggle)
	HOOK_COMMAND("+nvgadjust", NVGAdjustUp);
	HOOK_COMMAND("-nvgadjust", NVGAdjustDown);

	gHUD.AddHudElem(this);
	m_iFlags = HUD_DRAW;
	m_iEnable = 0;
	m_iAlpha = 110; // 220 is max, 30 is min

   return 0;
}

int CHudNVG::Draw(float flTime)
{
	if ( !m_iEnable || gEngfuncs.IsSpectateOnly() )
	{
		return 1;
	}
	gEngfuncs.pfnFillRGBABlend(0, 0, ScreenWidth, ScreenHeight, 50, 225, 50, m_iAlpha);

	// draw a dynamic light on player's origin
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight ( 0 );
	VectorCopy ( gHUD.m_vecOrigin, dl->origin );
	dl->radius = gEngfuncs.pfnRandomFloat( 750, 800 );
	dl->die = flTime + 0.1f;
	dl->color.r = 50;
	dl->color.g = 255;
	dl->color.b = 50;
	return 1;
}

int CHudNVG::MsgFunc_NVGToggle(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	m_iEnable = reader.ReadByte();
	return 1;
}

void CHudNVG::UserCmd_NVGAdjustDown()
{
	m_iAlpha += 20;
	if( m_iAlpha > 220 ) m_iAlpha = 220;
}

void CHudNVG::UserCmd_NVGAdjustUp()
{
	m_iAlpha -= 20;
	if( m_iAlpha < 30 ) m_iAlpha = 30;
}

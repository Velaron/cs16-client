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

#include "com_model.h"


void EV_CreateSmoke(event_args_s *args)
{
	static bool init = false;
	static const model_t *pGasModel;
	TEMPENTITY *pTemp;

	if(!init)
	{
		int iGasSprite = SPR_Load("sprites/gas_puff_01.spr");
		pGasModel = gEngfuncs.GetSpritePointer(iGasSprite);
		init = true;
	}

	if( !args->bparam2 ) // first explosion
	{
		Vector org;

		for( int i = 0; i < 10; i++ )
		{
			// randomize smoke cloud position
			org = args->origin;
			org.x += gEngfuncs.pfnRandomFloat(-100.0f, 100.0f);
			org.y += gEngfuncs.pfnRandomFloat(-100.0f, 100.0f);
			org.z += 30; 

			pTemp = gEngfuncs.pEfxAPI->CL_TempEntAlloc( org, (model_s*)pGasModel );
			if( pTemp )
			{
				// don't die when animation is ended
				pTemp->flags |= (FTENT_FADEOUT | FTENT_SPRANIMATELOOP | FTENT_COLLIDEWORLD);
				pTemp->die = gEngfuncs.GetClientTime() + 40.0f;
				pTemp->entity.curstate.framerate = 4.0f;
				pTemp->entity.curstate.rendermode = kRenderTransTexture;
				pTemp->entity.curstate.renderfx = kRenderFxNone;
				pTemp->entity.curstate.scale = 5.0f;
				// make it move slowly
				pTemp->entity.baseline.origin.x = gEngfuncs.pfnRandomLong(-5, 5);
				pTemp->entity.baseline.origin.y = gEngfuncs.pfnRandomLong(-5, 5);
			}
		}
	}
	else // second and other
	{
		static int iSmokeSprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/black_smoke4.spr" );

		pTemp = gEngfuncs.pEfxAPI->R_DefaultSprite( args->origin, iSmokeSprite, 4.0f );

		if( pTemp )
		{
			pTemp->entity.curstate.rendermode = kRenderTransTexture;
			pTemp->entity.curstate.renderfx = kRenderFxNone;
			pTemp->entity.curstate.rendercolor.r = gEngfuncs.pfnRandomLong(210, 230);
			pTemp->entity.curstate.rendercolor.g = gEngfuncs.pfnRandomLong(210, 230);
			pTemp->entity.curstate.rendercolor.b = gEngfuncs.pfnRandomLong(210, 230);
			pTemp->entity.curstate.renderamt = gEngfuncs.pfnRandomLong(180, 200);

			pTemp->entity.baseline.origin[0] = gEngfuncs.pfnRandomLong(0, 10);
		}
	}
}

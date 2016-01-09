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
				pTemp->flags |= (FTENT_SPRANIMATELOOP | FTENT_COLLIDEWORLD);
				pTemp->die = 80.0f;
				pTemp->entity.curstate.framerate = 4.0f;
				pTemp->entity.curstate.rendermode = kRenderTransTexture;
				pTemp->entity.curstate.renderfx = kRenderFxNone;
				pTemp->entity.curstate.scale = 7.0f;
				// make it move slowly
				pTemp->entity.baseline.origin.x = gEngfuncs.pfnRandomLong(-10, 10);
				pTemp->entity.baseline.origin.y = gEngfuncs.pfnRandomLong(-10, 10);
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

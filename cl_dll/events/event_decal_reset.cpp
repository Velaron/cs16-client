#include "events.h"

#include <math.h>

void EV_DecalReset(event_args_s *args)
{
	int decalnum = floor(gEngfuncs.pfnGetCvarFloat("r_decals"));

	for( int i = 0; i < decalnum; i++ )
		gEngfuncs.pEfxAPI->R_DecalRemoveAll(1);
}

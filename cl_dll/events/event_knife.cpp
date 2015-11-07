#include "events.h"

enum knife_e
{
	KNIFE_IDLE1 = 0,
	KNIFE_SLASH1,
	KNIFE_SLASH2,
	KNIFE_DRAW,
	KNIFE_STAB,
	KNIFE_STAB_MISS,
	KNIFE_MIDSLASH1,
	KNIFE_MIDSLASH2
};

void EV_Knife( struct event_args_s *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/knife_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM);
}

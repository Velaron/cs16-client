#include "events.h"

void EV_CreateSmoke(event_args_s *args)
{
	EV_HLDM_SmokeGrenade( args->origin[0], args->origin[1], args->origin[2] );
}

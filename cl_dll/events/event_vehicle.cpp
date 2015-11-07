#include "events.h"

#include <string.h>

void EV_Vehicle(event_args_s *args)
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;

	char sz[ 256 ];

	idx = args->entindex;

	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/vehicle1.wav"); break;
	case 2: strcpy( sz, "plats/vehicle2.wav"); break;
	case 3: strcpy( sz, "plats/vehicle3.wav"); break;
	case 4: strcpy( sz, "plats/vehicle4.wav"); break;
	case 5: strcpy( sz, "plats/vehicle6.wav"); break;
	case 6: strcpy( sz, "plats/vehicle7.wav"); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, 0, pitch );
	}
}

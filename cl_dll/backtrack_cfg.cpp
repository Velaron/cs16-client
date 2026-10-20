#include <string.h>

#include "backtrack_cfg.h"

const bt_cvar_def_s kBtCvars[] =
{
#define BT_X( var, name, def, lo, hi, ar, desc ) { name, def, lo, hi, ar, desc },
	BT_CVAR_LIST( BT_X )
#undef BT_X
};

const int kBtCvarCount = (int)( sizeof( kBtCvars ) / sizeof( kBtCvars[0] ));

int BtCfg_Find( const char *name )
{
	int i;

	if( !name )
		return -1;

	for( i = 0; i < kBtCvarCount; i++ )
	{
		if( strcmp( kBtCvars[i].name, name ) == 0 )
			return i;
	}

	return -1;
}

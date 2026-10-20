/*
 * Valida a tabela de cvars do backtrack sem engine e sem jogo.
 * Roda com: ./tools/bt_check.sh
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backtrack_cfg.h"

#ifndef BT_ROOT
#define BT_ROOT "."
#endif

static int g_ok, g_fail;

#define CHECK( cond, msg ) do { \
	if( cond ) { g_ok++; } \
	else { g_fail++; printf( "FAIL: %s (linha %d)\n", msg, __LINE__ ); } \
} while( 0 )

/* procura "cl_bt_xxx" numa linha do manifesto */
static int InManifest( const char *name )
{
	char path[1024];
	char line[256];
	FILE *f;
	int found = 0;

	snprintf( path, sizeof( path ), "%s/tools/hub_cvars.txt", BT_ROOT );
	f = fopen( path, "r" );
	if( !f )
	{
		printf( "FAIL: nao abriu %s\n", path );
		g_fail++;
		return 0;
	}

	while( fgets( line, sizeof( line ), f ))
	{
		char *p = line;
		size_t n;

		while( *p == ' ' || *p == '\t' ) p++;
		if( *p == '#' || *p == '\n' || *p == 0 )
			continue;

		n = strlen( p );
		while( n > 0 && ( p[n-1] == '\n' || p[n-1] == '\r' ))
			p[--n] = 0;

		if( strcmp( p, name ) == 0 )
		{
			found = 1;
			break;
		}
	}

	fclose( f );
	return found;
}

int main( void )
{
	int i, j;

	CHECK( kBtCvarCount >= 37, "tabela tem pelo menos 37 cvars" );

	for( i = 0; i < kBtCvarCount; i++ )
	{
		const bt_cvar_def_s *d = &kBtCvars[i];
		float def;

		CHECK( d->name != 0 && strncmp( d->name, "cl_bt_", 6 ) == 0, "nome com prefixo cl_bt_" );
		CHECK( d->desc != 0 && d->desc[0] != 0, "descricao preenchida" );
		CHECK( d->archive == 0 || d->archive == 1, "archive e 0 ou 1" );
		CHECK( d->lo <= d->hi, "faixa lo <= hi" );

		def = (float)atof( d->def );
		CHECK( def >= d->lo - 0.0001f && def <= d->hi + 0.0001f, "default dentro da faixa" );

		for( j = i + 1; j < kBtCvarCount; j++ )
			CHECK( strcmp( d->name, kBtCvars[j].name ) != 0, "nome unico" );

		CHECK( InManifest( d->name ), "cvar presente no manifesto" );
		CHECK( BtCfg_Find( d->name ) == i, "BtCfg_Find devolve o indice" );
	}

	CHECK( BtCfg_Find( "cl_bt_nao_existe" ) == -1, "BtCfg_Find ignora desconhecida" );

	printf( "BT CONFIG TEST: %d ok, %d fail\n", g_ok, g_fail );
	return g_fail ? 1 : 0;
}

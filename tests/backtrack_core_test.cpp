/* Teste host-side da logica pura do backtrack. Sem engine, sem ImGui. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../cl_dll/backtrack_core.h"

static int g_ok, g_fail;

#define CHECK( cond, msg ) do { \
	if( cond ) { g_ok++; } \
	else { g_fail++; printf( "FAIL: %s (linha %d)\n", msg, __LINE__ ); } \
} while( 0 )

static void Put( bt_buffer_s *b, double ago_ms, int player, int part, float x, float y, float z )
{
	bt_frame_s *f;
	bt_sample_s *s;
	int i;

	f = Bt_Push( b, 100.0 - ago_ms / 1000.0 );
	if( !f )
	{
		g_fail++;
		return;
	}

	for( i = 0; i < f->count; i++ )
		if( f->s[i].player == player )
			break;

	if( i == f->count )
	{
		f->s[i].player = player;
		f->count++;
	}

	s = &f->s[i];
	s->valid[part] = 1;
	s->part[part][0] = x;
	s->part[part][1] = y;
	s->part[part][2] = z;
	s->origin[0] = x;
	s->origin[1] = y;
	s->origin[2] = z;
}

int main( void )
{
	bt_buffer_s b;
	bt_query_s q;
	float out[3];
	int ms, i;

	/* 1. reset deixa o buffer vazio */
	Bt_Reset( &b );
	CHECK( Bt_FrameCount( &b ) == 0, "reset zera o buffer" );

	/* 2. push conta frames */
	Put( &b, 0, 1, 0, 0, 0, 0 );
	Put( &b, 10, 1, 0, 10, 0, 0 );
	Put( &b, 20, 1, 0, 20, 0, 0 );
	CHECK( Bt_FrameCount( &b ) == 3, "tres frames gravados" );

	/* 3. FindSample acha o jogador e ignora jogador ausente */
	CHECK( Bt_FindSample( &b.frames[0], 1 ) != 0, "FindSample acha jogador 1" );
	CHECK( Bt_FindSample( &b.frames[0], 9 ) == 0, "FindSample ignora jogador 9" );

	memset( &q, 0, sizeof( q ));
	q.now = 100.0;
	q.min_ms = 0;
	q.max_ms = 200;
	q.prefer = 0;

	/* 4. prefer 0 = mais recente */
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "lookup recente acha" );
	CHECK( fabsf( out[0] - 0.f ) < 0.01f, "recente = frame de 0 ms" );
	CHECK( ms == 0, "recente reporta 0 ms" );

	/* 5. prefer 1 = mais antigo */
	q.prefer = 1;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "lookup antigo acha" );
	CHECK( fabsf( out[0] - 20.f ) < 0.01f, "antigo = frame de 20 ms" );

	/* 6. janela filtra: so frames com ago >= 10 */
	q.prefer = 0;
	q.min_ms = 10;
	q.max_ms = 200;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "janela [10,200] acha" );
	CHECK( fabsf( out[0] - 10.f ) < 0.01f, "janela exclui o frame de 0 ms" );

	/* 7. parte inexistente e jogador inexistente nao retornam nada */
	q.min_ms = 0;
	CHECK( Bt_Lookup( &b, &q, 1, 5, out, &ms ) == 0, "parte sem dado = 0" );
	CHECK( Bt_Lookup( &b, &q, 9, 0, out, &ms ) == 0, "jogador ausente = 0" );

	/* 8. passo quantiza: 0/10/20/30 com step 20 e min 0 -> so 0 e 20 */
	Bt_Reset( &b );
	for( i = 0; i < 4; i++ )
		Put( &b, (double)( i * 10 ), 1, 0, (float)( i * 10 ), 0, 0 );

	q.min_ms = 0; q.max_ms = 40; q.step_ms = 20; q.prefer = 0;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "step acha candidato" );
	CHECK( ms == 0 || ms == 20 || ms == 40, "step so aceita a grade" );

	/* 9. interpolacao faz a media com o quadro vizinho */
	Bt_Reset( &b );
	Put( &b, 0, 1, 0, 100, 0, 0 );
	Put( &b, 50, 1, 0, 200, 0, 0 );

	q.min_ms = 0; q.max_ms = 100; q.step_ms = 0; q.prefer = 0; q.interp = 1;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "interp acha" );
	CHECK( fabsf( out[0] - 150.f ) < 0.01f, "interp = media (150)" );

	q.interp = 0;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "sem interp acha" );
	CHECK( fabsf( out[0] - 100.f ) < 0.01f, "sem interp = 100" );

	/* 10. prefer 2 escolhe o ponto mais alinhado com a mira */
	Bt_Reset( &b );
	Put( &b, 0, 1, 0, 0, 100, 0 );      /* angulo ~90 graus do yaw 0 */
	Put( &b, 50, 1, 0, 100, 0, 0 );     /* alinhado com yaw 0 */
	q.prefer = 2; q.interp = 0; q.min_ms = 0; q.max_ms = 100;
	q.eye[0] = 0; q.eye[1] = 0; q.eye[2] = 0;
	q.view[0] = 0; q.view[1] = 0; q.view[2] = 0;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "prefer angulo acha" );
	CHECK( fabsf( out[0] - 100.f ) < 0.01f, "prefer angulo escolhe o alinhado" );

	/* 11. prefer 3 escolhe o mais perto do olho */
	q.prefer = 3;
	q.eye[0] = 0; q.eye[1] = 90; q.eye[2] = 0;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "prefer perto acha" );
	CHECK( fabsf( out[0] - 0.f ) < 0.01f, "prefer perto escolhe o vizinho" );

	/* 12. wrap-around mantem o maximo de frames */
	Bt_Reset( &b );
	for( i = 0; i < BT_MAX_FRAMES + 10; i++ )
		Put( &b, (double)i, 1, 0, (float)i, 0, 0 );
	CHECK( Bt_FrameCount( &b ) == BT_MAX_FRAMES, "wrap limita ao maximo" );

	q.min_ms = 0; q.max_ms = 1000; q.step_ms = 0; q.prefer = 0; q.interp = 0;
	q.now = 100.0;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "wrap ainda responde" );
	/* foram 106 pushes e o buffer guarda 96: os 10 mais velhos (0..9) saem,
	 * entao o quadro mais novo que sobrou e o de indice 10 */
	CHECK( fabsf( out[0] - 10.f ) < 0.01f, "wrap guarda os 96 mais novos" );

	/* 13. buffer vazio nao responde */
	Bt_Reset( &b );
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 0, "buffer vazio = 0" );

	/* 14. jitter real (16.7 ms por quadro) com janela [60,180] precisa achar */
	Bt_Reset( &b );
	for( i = 0; i < 12; i++ )
		Put( &b, 16.7 * i, 1, 0, (float)i, 0, 0 );

	q.min_ms = 60; q.max_ms = 180; q.step_ms = 20; q.prefer = 0;
	q.interp = 0; q.now = 100.0;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "jitter acha candidato" );
	CHECK( ms >= 60 && ms <= 180, "jitter respeita a janela" );

	q.prefer = 1;
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "jitter antigo acha" );
	CHECK( ms >= 150, "prefer antigo pega o mais velho da janela" );

	/* 15. limite de quadros: so os ultimos gravados contam */
	Bt_Reset( &b );
	for( i = 0; i < 10; i++ )
		Put( &b, (double)i, 1, 0, (float)i, 0, 0 );

	Bt_SetLimit( &b, 3 );
	q.min_ms = 0; q.max_ms = 100; q.step_ms = 0; q.prefer = 0;
	q.interp = 0; q.now = 100.0;

	CHECK( Bt_FrameCount( &b ) == 10, "limit nao mexe no total gravado" );
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "limit ainda responde" );
	/* os 3 quadros validos sao i=7,8,9 (ago 7,8,9); o mais novo e o de 7 ms */
	CHECK( fabsf( out[0] - 7.f ) < 0.01f, "limit usa os ultimos quadros" );

	Bt_SetLimit( &b, 0 );
	CHECK( Bt_Lookup( &b, &q, 1, 0, out, &ms ) == 1, "limit 0 usa tudo" );
	CHECK( fabsf( out[0] - 0.f ) < 0.01f, "limit 0 volta ao mais novo" );

	printf( "BT CORE TEST: %d ok, %d fail\n", g_ok, g_fail );
	return g_fail ? 1 : 0;
}

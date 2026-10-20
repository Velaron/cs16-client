#include <math.h>
#include <string.h>

#include "backtrack_core.h"

void Bt_Reset( bt_buffer_s *b )
{
	if( !b )
		return;

	memset( b, 0, sizeof( *b ));
}

bt_frame_s *Bt_Push( bt_buffer_s *b, double time )
{
	bt_frame_s *f;

	if( !b )
		return 0;

	f = &b->frames[b->head];
	memset( f, 0, sizeof( *f ));
	f->time = time;

	b->head = ( b->head + 1 ) % BT_MAX_FRAMES;
	if( b->count < BT_MAX_FRAMES )
		b->count++;

	return f;
}

int Bt_FrameCount( const bt_buffer_s *b )
{
	return b ? b->count : 0;
}

void Bt_SetLimit( bt_buffer_s *b, int limit )
{
	if( !b )
		return;

	if( limit < 1 ) limit = 0;
	if( limit > BT_MAX_FRAMES ) limit = BT_MAX_FRAMES;

	b->limit = limit;
}

/* quantidade de quadros que vale consultar (respeita o limite) */
static int Bt_ValidCount( const bt_buffer_s *b )
{
	int n = b->count;

	if( b->limit > 0 && b->limit < n )
		n = b->limit;

	return n;
}

const bt_sample_s *Bt_FindSample( const bt_frame_s *f, int player )
{
	int i;

	if( !f )
		return 0;

	for( i = 0; i < f->count; i++ )
	{
		if( f->s[i].player == player )
			return &f->s[i];
	}

	return 0;
}

/* index 0 = frame mais antigo, count-1 = mais novo */
static const bt_frame_s *Bt_FrameAt( const bt_buffer_s *b, int i )
{
	int start;

	if( !b || i < 0 || i >= Bt_ValidCount( b ))
		return 0;

	start = ( b->head - Bt_ValidCount( b ) + BT_MAX_FRAMES ) % BT_MAX_FRAMES;

	return &b->frames[( start + i ) % BT_MAX_FRAMES];
}

static float Bt_Dist( const float a[3], const float b[3] )
{
	float dx = a[0] - b[0];
	float dy = a[1] - b[1];
	float dz = a[2] - b[2];

	return sqrtf( dx * dx + dy * dy + dz * dz );
}

static float Bt_AngleTo( const float eye[3], const float point[3], const float view[3] )
{
	float d[3];
	float len, yaw, pitch, dy, dp;

	d[0] = point[0] - eye[0];
	d[1] = point[1] - eye[1];
	d[2] = point[2] - eye[2];
	len = sqrtf( d[0] * d[0] + d[1] * d[1] + d[2] * d[2] );
	if( len < 0.001f )
		return 0.f;

	yaw = atan2f( d[1], d[0] ) * 57.29578f;
	pitch = -asinf( d[2] / len ) * 57.29578f;

	dy = yaw - view[1];
	while( dy > 180.f ) dy -= 360.f;
	while( dy < -180.f ) dy += 360.f;
	dp = pitch - view[0];

	return sqrtf( dp * dp + dy * dy );
}

int Bt_Lookup( const bt_buffer_s *b, const bt_query_s *q, int player, int part,
	float out[3], int *out_ms )
{
	int i, have = 0, best = -1, best_ms = 0;
	double best_score = 0.0;
	int min_ms, max_ms, step, target_ms;

	if( !b || !q || !out || player <= 0 || part < 0 || part >= BT_PARTS )
		return 0;
	if( b->count <= 0 )
		return 0;

	min_ms = q->min_ms < 0 ? 0 : q->min_ms;
	max_ms = q->max_ms < min_ms ? min_ms : q->max_ms;
	step = q->step_ms < 0 ? 0 : q->step_ms;

	/*
	 * Alvo temporal para os modos por tempo (prefer 0 = mais recente,
	 * prefer 1 = mais antigo). O passo arredonda esse alvo para a grade,
	 * sem descartar quadros: o mais proximo do alvo sempre serve.
	 */
	target_ms = ( q->prefer == 1 ) ? max_ms : min_ms;
	if( step > 0 )
	{
		int k = ( target_ms - min_ms + step / 2 ) / step;

		target_ms = min_ms + k * step;
		if( target_ms > max_ms ) target_ms = max_ms;
		if( target_ms < min_ms ) target_ms = min_ms;
	}

	for( i = 0; i < Bt_ValidCount( b ); i++ )
	{
		const bt_frame_s *f = Bt_FrameAt( b, i );
		const bt_sample_s *s;
		double ago, score;

		if( !f )
			continue;

		ago = ( q->now - f->time ) * 1000.0;
		if( ago < (double)min_ms - 0.5 || ago > (double)max_ms + 0.5 )
			continue;

		s = Bt_FindSample( f, player );
		if( !s || !s->valid[part] )
			continue;

		switch( q->prefer )
		{
		case 2: score = -Bt_AngleTo( q->eye, s->part[part], q->view ); break;
		case 3: score = -Bt_Dist( q->eye, s->part[part] ); break;
		default: score = -fabs( ago - (double)target_ms ); break;
		}

		if( !have || score > best_score )
		{
			have = 1;
			best_score = score;
			best = i;
			best_ms = (int)( ago + 0.5 );
		}
	}

	if( !have || best < 0 )
		return 0;

	{
		const bt_frame_s *f = Bt_FrameAt( b, best );
		const bt_sample_s *s = Bt_FindSample( f, player );
		int k;

		for( k = 0; k < 3; k++ )
			out[k] = s->part[part][k];

		if( q->interp )
		{
			/* media com o quadro vizinho: o seguinte ou, se nao houver, o anterior */
			const bt_frame_s *f2 = Bt_FrameAt( b, best + 1 );
			const bt_sample_s *s2 = f2 ? Bt_FindSample( f2, player ) : 0;

			if( !s2 || !s2->valid[part] )
			{
				f2 = Bt_FrameAt( b, best - 1 );
				s2 = f2 ? Bt_FindSample( f2, player ) : 0;
			}

			if( s2 && s2->valid[part] )
			{
				for( k = 0; k < 3; k++ )
					out[k] = ( out[k] + s2->part[part][k] ) * 0.5f;
			}
		}
	}

	if( out_ms )
		*out_ms = best_ms;

	return 1;
}

int Bt_SelfTest( void )
{
	bt_buffer_s b;
	bt_query_s q;
	float out[3];
	int ms, i, fail = 0;

	Bt_Reset( &b );
	if( Bt_FrameCount( &b ) != 0 )
		fail++;

	for( i = 0; i < 3; i++ )
	{
		bt_frame_s *f = Bt_Push( &b, 100.0 - i * 0.01 );

		if( !f )
		{
			fail++;
			continue;
		}

		f->s[0].player = 1;
		f->s[0].valid[0] = 1;
		f->s[0].part[0][0] = (float)( i * 10 );
		f->count = 1;
	}
	if( Bt_FrameCount( &b ) != 3 )
		fail++;

	memset( &q, 0, sizeof( q ));
	q.now = 100.0;
	q.max_ms = 200;

	if( !Bt_Lookup( &b, &q, 1, 0, out, &ms ))
		fail++;
	else if( fabsf( out[0] - 0.f ) > 0.01f )
		fail++;   /* mais recente */

	q.prefer = 1;
	if( !Bt_Lookup( &b, &q, 1, 0, out, &ms ))
		fail++;
	else if( fabsf( out[0] - 20.f ) > 0.01f )
		fail++;   /* mais antigo */

	if( Bt_Lookup( &b, &q, 9, 0, out, &ms ))
		fail++;   /* jogador ausente */
	if( Bt_Lookup( &b, &q, 1, 5, out, &ms ))
		fail++;   /* parte ausente */

	return fail;
}

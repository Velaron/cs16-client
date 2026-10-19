/*
 * sound_esp.cpp - sound ESP.
 *
 * A engine grava cada som dinamico (passos, tiro, reload) em
 * /tmp/cs16_sound_esp.bin via mmap. Aqui a gente le o mesmo arquivo e
 * desenha um circulo que expande e some no lugar de onde o som veio.
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "cvardef.h"
#include "sound_esp.h"

#define SND_ESP_MAGIC 0x444E5331u
#define SND_ESP_MAX   64
#define SND_ESP_PATH  "/tmp/cs16_sound_esp.bin"

#pragma pack(push, 1)
typedef struct
{
	float    origin[3];
	int32_t  entnum;
	int32_t  channel;
	uint32_t time_ms;
} snd_esp_rec_t;

typedef struct
{
	uint32_t magic;
	uint32_t seq;
	uint32_t time_ms;
	int32_t  count;
	snd_esp_rec_t rec[SND_ESP_MAX];
} snd_esp_frame_t;
#pragma pack(pop)

static const snd_esp_frame_t *g_frame;
static int g_fd = -1;
static int g_tried;

static cvar_t *cv_sound;
static cvar_t *cv_range;
static cvar_t *cv_time;

static uint32_t NowMs( void )
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	return (uint32_t)( tv.tv_sec * 1000u + tv.tv_usec / 1000u );
}

void SoundEsp_Init( void )
{
	cv_sound = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_sound", "0", FCVAR_ARCHIVE );
	cv_range = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_sound_range", "3000", FCVAR_ARCHIVE );
	cv_time  = gEngfuncs.pfnRegisterVariable( "cl_sak_esp_sound_time", "1.2", FCVAR_ARCHIVE );
}

static void OpenMap( void )
{
	if( g_frame || g_tried )
		return;

	g_tried = 1;
	g_fd = open( SND_ESP_PATH, O_RDONLY );
	if( g_fd < 0 )
		return;

	g_frame = (const snd_esp_frame_t *)mmap( NULL, sizeof( snd_esp_frame_t ),
		PROT_READ, MAP_SHARED, g_fd, 0 );

	if( g_frame == MAP_FAILED )
	{
		g_frame = NULL;
		close( g_fd );
		g_fd = -1;
	}
}

static int ToScreen( const float *world, float *x, float *y )
{
	float tmp[3];

	tmp[0] = world[0];
	tmp[1] = world[1];
	tmp[2] = world[2];
	return gEngfuncs.pTriAPI->WorldToScreen( tmp, tmp ) == 0 ? 0 : 1;
}

static void Dot( int x, int y, int r, int g, int b, int a )
{
	gEngfuncs.pfnFillRGBA( x, y, 2, 2, r, g, b, a );
}

/* circulo aproximado com 16 pontos */
static void Ring( int cx, int cy, int radius, int r, int g, int b, int a )
{
	int i;

	if( radius < 2 )
	{
		Dot( cx, cy, r, g, b, a );
		return;
	}

	for( i = 0; i < 16; i++ )
	{
		float ang = (float)i * 3.14159265f / 8.f;
		int x = cx + (int)( cosf( ang ) * radius );
		int y = cy + (int)( sinf( ang ) * radius );

		Dot( x, y, r, g, b, a );
	}
}

void SoundEsp_Draw( void )
{
	uint32_t now;
	float life;
	float range;
	float local[3];
	int localIndex;
	int i;

	if( !cv_sound || cv_sound->value < 0.5f )
		return;

	OpenMap();
	if( !g_frame || g_frame->magic != SND_ESP_MAGIC )
		return;

	/* frame sendo escrito: espera o proximo */
	if( g_frame->seq & 1u )
		return;

	now = NowMs();
	life = cv_time ? cv_time->value : 1.2f;
	range = cv_range ? cv_range->value : 3000.f;

	if( life < 0.1f )
		life = 0.1f;

	localIndex = gEngfuncs.GetLocalPlayer() ? gEngfuncs.GetLocalPlayer()->index : 0;

	if( localIndex > 0 )
	{
		local[0] = g_PlayerExtraInfo[localIndex].origin[0];
		local[1] = g_PlayerExtraInfo[localIndex].origin[1];
		local[2] = g_PlayerExtraInfo[localIndex].origin[2];
	}
	else
	{
		local[0] = local[1] = local[2] = 0.f;
	}

	for( i = 0; i < g_frame->count && i < SND_ESP_MAX; i++ )
	{
		const snd_esp_rec_t *rec = &g_frame->rec[i];
		float age;
		float dx, dy, dz, dist;
		float sx, sy;
		float t;
		int r = 255, g = 80, b = 80, a;

		if( rec->entnum <= 0 || rec->entnum == localIndex )
			continue;

		age = (float)( now - rec->time_ms ) / 1000.f;
		if( age < 0.f || age > life )
			continue;

		dx = rec->origin[0] - local[0];
		dy = rec->origin[1] - local[1];
		dz = rec->origin[2] - local[2];
		dist = sqrtf( dx * dx + dy * dy + dz * dz );

		if( dist > range )
			continue;

		if( !ToScreen( rec->origin, &sx, &sy ) )
			continue;

		if( sx < -64.f || sy < -64.f || sx > ScreenWidth + 64.f || sy > ScreenHeight + 64.f )
			continue;

		/* cor por time: TT vermelho, CT azul */
		if( rec->entnum > 0 && rec->entnum <= MAX_PLAYERS )
		{
			int team = g_PlayerExtraInfo[rec->entnum].teamnumber;

			if( team == 2 )
			{
				r = 80; g = 160; b = 255;
			}
		}

		t = age / life;
		a = (int)( 230.f * ( 1.f - t ) );

		Ring( (int)sx, (int)sy, (int)( 4.f + 30.f * t ), r, g, b, a );
		Ring( (int)sx, (int)sy, (int)( 2.f + 12.f * t ), r, g, b, a / 2 );
	}
}

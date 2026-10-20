#pragma once

/*
 * backtrack_core.h - logica pura do backtrack.
 *
 * Guarda um historico circular de posicoes de hitbox por jogador, gravado a
 * cada frame pelo HUD, e responde "onde este hitbox estava N ms atras?".
 * Sem engine, sem cvars: compila no host e roda em tests/backtrack_core_test.cpp.
 */

#define BT_PARTS        21   /* AIM_PART_COUNT */
#define BT_MAX_PLAYERS  33   /* 0 nao usado, 1..32 */
#define BT_MAX_FRAMES   96   /* ~1 s a 10 ms de intervalo */

struct bt_sample_s
{
	int player;
	unsigned char valid[BT_PARTS];
	float part[BT_PARTS][3];
	float origin[3];
};

struct bt_frame_s
{
	double time;
	int count;
	bt_sample_s s[BT_MAX_PLAYERS];
};

struct bt_buffer_s
{
	bt_frame_s frames[BT_MAX_FRAMES];
	int head;
	int count;
	int limit;    /* quadros usados (0 = todos) */
};

struct bt_query_s
{
	double now;
	int min_ms;
	int max_ms;
	int step_ms;
	int prefer;    /* 0 recente, 1 antigo, 2 melhor angulo, 3 mais perto */
	int interp;    /* 1 = media com o quadro vizinho */
	float view[3];
	float eye[3];
};

void Bt_Reset( bt_buffer_s *b );
bt_frame_s *Bt_Push( bt_buffer_s *b, double time );
int Bt_FrameCount( const bt_buffer_s *b );
void Bt_SetLimit( bt_buffer_s *b, int limit );
const bt_sample_s *Bt_FindSample( const bt_frame_s *f, int player );
int Bt_Lookup( const bt_buffer_s *b, const bt_query_s *q, int player, int part,
	float out[3], int *out_ms );

/* roda os casos basicos e devolve a quantidade de falhas */
int Bt_SelfTest( void );

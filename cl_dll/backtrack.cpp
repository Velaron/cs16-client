/*
 * backtrack.cpp - glue do backtrack: cvars, gravacao por frame, mira no
 * ponto historico, sincronizacao com o fakelag e desenho dos ghosts.
 *
 * O historico em si (ring buffer, janela, passo, escolha) vive em
 * backtrack_core.cpp, que nao depende da engine e roda em teste no host.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "triangleapi.h"
#include "com_weapons.h"
#include "backtrack.h"
#include "backtrack_core.h"
#include "aimbot.h"
#include "sakura_features.h"

static cvar_t *cv_enable;
static cvar_t *cv_mode;
static cvar_t *cv_trigger_mode;
static cvar_t *cv_key;
static cvar_t *cv_team;
static cvar_t *cv_min_ms;
static cvar_t *cv_max_ms;
static cvar_t *cv_step_ms;
static cvar_t *cv_prefer;
static cvar_t *cv_visible;
static cvar_t *cv_walls;
static cvar_t *cv_max_dist;
static cvar_t *cv_ping_min;
static cvar_t *cv_sync_fl;
static cvar_t *cv_fl_extra;
static cvar_t *cv_draw;
static cvar_t *cv_draw_ms;
static cvar_t *cv_draw_line;
static cvar_t *cv_draw_target;
static cvar_t *cv_draw_box;
static cvar_t *cv_interp;
static cvar_t *cv_record_ms;
static cvar_t *cv_max_frames;
static cvar_t *cv_clear_round;
static cvar_t *cv_debug;

static bt_buffer_s g_bt;
static double g_lastRecord;
static double g_lastAimTime;
static int g_lastDelay;
static int g_flOwned;
static float g_flOldEnable, g_flOldLimit, g_flOldType;
static int g_attacking;

static void FillQuery( bt_query_s *q, double now );

static int On( cvar_t *cv, int fallback )
{
	if( !cv )
		return fallback;

	return cv->value != 0.f;
}

static float CV( cvar_t *cv, float fallback )
{
	return cv ? cv->value : fallback;
}

static float BtClamp( float v, float lo, float hi )
{
	if( v < lo ) return lo;
	if( v > hi ) return hi;

	return v;
}

static double Now( void )
{
	return gEngfuncs.GetClientTime();
}

static int LocalPing( void )
{
	hud_player_info_t info;
	cl_entity_t *local = gEngfuncs.GetLocalPlayer();

	if( !local )
		return 0;

	memset( &info, 0, sizeof( info ));
	gEngfuncs.pfnGetPlayerInfo( local->index, &info );

	return info.ping;
}

void Backtrack_Reset( void )
{
	Bt_Reset( &g_bt );
	g_lastRecord = 0.0;
	g_lastAimTime = 0.0;
	g_lastDelay = 0;
}

void Backtrack_SetContext( int attacking )
{
	g_attacking = attacking;
}

int Backtrack_Enabled( void )
{
	if( !On( cv_enable, 0 ))
		return 0;
	if( (int)CV( cv_mode, 1.f ) <= 0 )
		return 0;

	return 1;
}

void Backtrack_Init( void )
{
	cv_enable       = gEngfuncs.pfnRegisterVariable( "cl_bt_enable", "0", FCVAR_ARCHIVE );
	cv_mode         = gEngfuncs.pfnRegisterVariable( "cl_bt_mode", "1", FCVAR_ARCHIVE );
	cv_trigger_mode = gEngfuncs.pfnRegisterVariable( "cl_bt_trigger_mode", "0", FCVAR_ARCHIVE );
	cv_key          = gEngfuncs.pfnRegisterVariable( "cl_bt_key", "0", FCVAR_ARCHIVE );
	cv_team         = gEngfuncs.pfnRegisterVariable( "cl_bt_team", "0", FCVAR_ARCHIVE );
	cv_min_ms       = gEngfuncs.pfnRegisterVariable( "cl_bt_min_ms", "40", FCVAR_ARCHIVE );
	cv_max_ms       = gEngfuncs.pfnRegisterVariable( "cl_bt_max_ms", "200", FCVAR_ARCHIVE );
	cv_step_ms      = gEngfuncs.pfnRegisterVariable( "cl_bt_step_ms", "20", FCVAR_ARCHIVE );
	cv_prefer       = gEngfuncs.pfnRegisterVariable( "cl_bt_prefer", "0", FCVAR_ARCHIVE );
	cv_visible      = gEngfuncs.pfnRegisterVariable( "cl_bt_visible_only", "0", FCVAR_ARCHIVE );
	cv_walls        = gEngfuncs.pfnRegisterVariable( "cl_bt_ignore_walls", "0", FCVAR_ARCHIVE );
	cv_max_dist     = gEngfuncs.pfnRegisterVariable( "cl_bt_max_dist", "2500", FCVAR_ARCHIVE );
	cv_ping_min     = gEngfuncs.pfnRegisterVariable( "cl_bt_ping_min", "0", FCVAR_ARCHIVE );
	cv_sync_fl      = gEngfuncs.pfnRegisterVariable( "cl_bt_sync_fakelag", "0", FCVAR_ARCHIVE );
	cv_fl_extra     = gEngfuncs.pfnRegisterVariable( "cl_bt_fakelag_extra", "0", FCVAR_ARCHIVE );
	cv_draw         = gEngfuncs.pfnRegisterVariable( "cl_bt_draw", "0", FCVAR_ARCHIVE );
	cv_draw_ms      = gEngfuncs.pfnRegisterVariable( "cl_bt_draw_ms", "120", FCVAR_ARCHIVE );
	cv_draw_line    = gEngfuncs.pfnRegisterVariable( "cl_bt_draw_line", "1", FCVAR_ARCHIVE );
	cv_draw_target  = gEngfuncs.pfnRegisterVariable( "cl_bt_draw_target_only", "0", FCVAR_ARCHIVE );
	cv_draw_box     = gEngfuncs.pfnRegisterVariable( "cl_bt_draw_box", "1", FCVAR_ARCHIVE );
	cv_interp       = gEngfuncs.pfnRegisterVariable( "cl_bt_interp", "1", FCVAR_ARCHIVE );
	cv_record_ms    = gEngfuncs.pfnRegisterVariable( "cl_bt_record_ms", "10", FCVAR_ARCHIVE );
	cv_max_frames   = gEngfuncs.pfnRegisterVariable( "cl_bt_max_frames", "96", FCVAR_ARCHIVE );
	cv_clear_round  = gEngfuncs.pfnRegisterVariable( "cl_bt_clear_round", "1", FCVAR_ARCHIVE );
	cv_debug        = gEngfuncs.pfnRegisterVariable( "cl_bt_debug", "0", FCVAR_ARCHIVE );

	Backtrack_Reset();
	Backtrack_SelfTest();
}

void Backtrack_Tick( void )
{
	const aim_box_t *boxes;
	int count = 0;
	int i;
	double now = Now();
	float interval;
	bt_frame_s *f;

	/* so grava quando alguma coisa vai usar o historico */
	if( !On( cv_enable, 0 ) && !On( cv_draw, 0 ))
		return;

	if( g_lastRecord > 0.0 && now < g_lastRecord )
		Backtrack_Reset();   /* mapa novo / tempo voltou */

	interval = BtClamp( CV( cv_record_ms, 10.f ), 5.f, 50.f ) * 0.001f;
	if( g_lastRecord > 0.0 && now - g_lastRecord < interval )
		return;

	Bt_SetLimit( &g_bt, (int)BtClamp( CV( cv_max_frames, 96.f ), 8.f, BT_MAX_FRAMES ));

	boxes = Aimbot_GetBoxes( &count );
	if( !boxes || count <= 0 )
		return;

	f = Bt_Push( &g_bt, now );
	if( !f )
		return;

	g_lastRecord = now;

	for( i = 0; i < count; i++ )
	{
		int p = boxes[i].player;
		int part = boxes[i].part;
		bt_sample_s *s = 0;
		int j;

		if( p <= 0 || p >= BT_MAX_PLAYERS )
			continue;
		if( part < 0 || part >= BT_PARTS )
			continue;

		for( j = 0; j < f->count; j++ )
		{
			if( f->s[j].player == p )
			{
				s = &f->s[j];
				break;
			}
		}

		if( !s )
		{
			if( f->count >= BT_MAX_PLAYERS )
				continue;

			s = &f->s[f->count++];
			s->player = p;
		}

		s->valid[part] = 1;
		s->part[part][0] = boxes[i].origin[0];
		s->part[part][1] = boxes[i].origin[1];
		s->part[part][2] = boxes[i].origin[2];
		s->origin[0] = boxes[i].origin[0];
		s->origin[1] = boxes[i].origin[1];
		s->origin[2] = boxes[i].origin[2];
	}

	/* limpa quando o jogador local morre: o historico da vida antiga nao serve */
	{
		static int lastAlive = -1;
		int alive = CL_IsDead() ? 0 : 1;

		if( On( cv_debug, 0 ))
		{
			static double lastPrint;

			if( now - lastPrint > 2.0 )
			{
				int p, part, hits = 0;
				float tmp[3];
				bt_query_s q;

				lastPrint = now;
				FillQuery( &q, now );

				for( p = 1; p < BT_MAX_PLAYERS; p++ )
					for( part = 0; part < BT_PARTS; part++ )
						if( Bt_Lookup( &g_bt, &q, p, part, tmp, 0 ))
							hits++;

				{
					cl_entity_t *me = gEngfuncs.GetLocalPlayer();
					float ox = me ? me->origin[0] : 0.f;
					float oy = me ? me->origin[1] : 0.f;

					gEngfuncs.Con_Printf( "bt: boxes=%d aceitos=%d frames=%d hp=%d alive=%d hits=%d org=%.1f,%.1f\n",
						count, f->count, Bt_FrameCount( &g_bt ), gHUD.m_Health.m_iHealth, alive, hits, ox, oy );
				}
			}
		}

		if( On( cv_clear_round, 1 ) && lastAlive == 1 && alive == 0 )
			Backtrack_Reset();   /* morreu */

		lastAlive = alive;
	}

	/*
	 * mantem o cl_fakelag acompanhando o delay escolhido.
	 * So liga enquanto esta atirando: fakelag parado a toa deixa o
	 * movimento impreciso. Ao sair, devolve os valores originais.
	 */
	{
		int delay = Backtrack_LastDelay();
		int target;

		if( On( cv_sync_fl, 0 ) && delay > 0 && g_attacking )
		{
			if( !g_flOwned )
			{
				g_flOldEnable = CVAR_GET_FLOAT( "cl_fakelag" );
				g_flOldLimit = CVAR_GET_FLOAT( "cl_fakelag_limit" );
				g_flOldType = CVAR_GET_FLOAT( "cl_fakelag_type" );
			}

			target = delay + (int)BtClamp( CV( cv_fl_extra, 0.f ), 0.f, 100.f );
			if( target > 200 )
				target = 200;

			gEngfuncs.Cvar_SetValue( "cl_fakelag_limit", (float)target );
			gEngfuncs.Cvar_SetValue( "cl_fakelag_type", 1.f );
			gEngfuncs.Cvar_SetValue( "cl_fakelag", 1.f );
			g_flOwned = 1;
		}
		else if( g_flOwned )
		{
			gEngfuncs.Cvar_SetValue( "cl_fakelag", g_flOldEnable );
			gEngfuncs.Cvar_SetValue( "cl_fakelag_limit", g_flOldLimit );
			gEngfuncs.Cvar_SetValue( "cl_fakelag_type", g_flOldType );
			g_flOwned = 0;
		}
	}
}

static void FillQuery( bt_query_s *q, double now )
{
	memset( q, 0, sizeof( *q ));

	q->now = now;
	q->min_ms = (int)BtClamp( CV( cv_min_ms, 40.f ), 0.f, 400.f );
	q->max_ms = (int)BtClamp( CV( cv_max_ms, 200.f ), 0.f, 800.f );
	q->step_ms = (int)BtClamp( CV( cv_step_ms, 20.f ), 0.f, 100.f );
	q->prefer = (int)CV( cv_prefer, 0.f );
	q->interp = On( cv_interp, 1 );

	/* rage usa sempre o delay maximo da janela */
	if( (int)CV( cv_mode, 1.f ) == 2 )
		q->prefer = 1;

	gEngfuncs.GetViewAngles( q->view );

	{
		cl_entity_t *local = gEngfuncs.GetLocalPlayer();
		float ofs[3];

		if( local )
		{
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( ofs );
			q->eye[0] = local->origin[0] + ofs[0];
			q->eye[1] = local->origin[1] + ofs[1];
			q->eye[2] = local->origin[2] + ofs[2];
		}
	}
}

int Backtrack_GetPoint( int player, int part, float out[3], int *out_ms )
{
	bt_query_s q;
	float dx, dy, dz, dist;

	if( !Backtrack_Enabled() || player <= 0 )
		return 0;

	/* ativacao propria: 0 sempre, 1 so ao atirar, 2 tecla */
	switch( (int)CV( cv_trigger_mode, 0.f ))
	{
	case 1:
		if( !g_attacking )
			return 0;
		break;
	case 2:
		if( !Aimbot_KeyHeld() )
			return 0;
		break;
	default:
		break;
	}

	/* time: nao mirar no proprio time */
	if( !On( cv_team, 0 ) && g_iTeamNumber != 0 )
	{
		if( g_PlayerExtraInfo[player].teamnumber == g_iTeamNumber )
			return 0;
	}

	if( CV( cv_ping_min, 0.f ) > 0.f && LocalPing() < (int)CV( cv_ping_min, 0.f ))
		return 0;

	FillQuery( &q, Now() );

	if( !Bt_Lookup( &g_bt, &q, player, part, out, out_ms ))
		return 0;

	dx = out[0] - q.eye[0];
	dy = out[1] - q.eye[1];
	dz = out[2] - q.eye[2];
	dist = sqrtf( dx * dx + dy * dy + dz * dz );

	if( dist > BtClamp( CV( cv_max_dist, 2500.f ), 100.f, 20000.f ))
		return 0;

	if( On( cv_visible, 0 ) && !Aimbot_PointVisible( q.eye, out, player, On( cv_walls, 0 )))
		return 0;

	g_lastDelay = out_ms ? *out_ms : 0;
	g_lastAimTime = q.now;

	if( On( cv_debug, 0 ))
	{
		static double lastLog;

		if( q.now - lastLog > 0.25 )
		{
			lastLog = q.now;
			gEngfuncs.Con_Printf( "bt: player %d part %d -> %d ms (%.1f %.1f %.1f)\n",
				player, part, g_lastDelay, out[0], out[1], out[2] );
		}
	}

	return 1;
}

int Backtrack_LastDelay( void )
{
	if( g_lastAimTime <= 0.0 )
		return 0;
	if( Now() - g_lastAimTime > 0.5 )
		return 0;

	return g_lastDelay;
}

void Backtrack_Draw( void )
{
	const aim_box_t *boxes;
	int count = 0;
	int i;
	int target;
	int r = 255, g = 20, b = 147, a = 200;
	int delay;

	if( !On( cv_draw, 0 ))
		return;

	boxes = Aimbot_GetBoxes( &count );
	if( !boxes || count <= 0 )
		return;

	delay = (int)BtClamp( CV( cv_draw_ms, 120.f ), 0.f, 800.f );
	target = Aimbot_CurrentTarget();

	{
		bt_query_s q;

		FillQuery( &q, Now() );
		q.min_ms = delay;
		q.max_ms = delay;
		q.step_ms = 0;
		q.prefer = 0;
		q.interp = 0;

		for( i = 0; i < count; i++ )
		{
			float hist[3];
			float delta[3];
			float corners[8][3];
			int ms = 0;
			int k;

			if( boxes[i].player <= 0 || boxes[i].part < 0 || boxes[i].part >= BT_PARTS )
				continue;
			if( On( cv_draw_target, 0 ) && target > 0 && boxes[i].player != target )
				continue;
			if( !Bt_Lookup( &g_bt, &q, boxes[i].player, boxes[i].part, hist, &ms ))
				continue;

			delta[0] = hist[0] - boxes[i].origin[0];
			delta[1] = hist[1] - boxes[i].origin[1];
			delta[2] = hist[2] - boxes[i].origin[2];

			/* ghost = caixa atual deslocada para a posicao historica */
			if( On( cv_draw_box, 1 ))
			{
				for( k = 0; k < 8; k++ )
				{
					corners[k][0] = boxes[i].c[k][0] + delta[0];
					corners[k][1] = boxes[i].c[k][1] + delta[1];
					corners[k][2] = boxes[i].c[k][2] + delta[2];
				}

				Esp_DrawBoxCorners( corners, r, g, b, a );
			}

			/* marca o ponto historico */
			if( On( cv_draw_line, 1 ))
			{
				float p3[3], s2[2];

				p3[0] = hist[0];
				p3[1] = hist[1];
				p3[2] = hist[2];

				if( gEngfuncs.pTriAPI->WorldToScreen( p3, s2 ))
					gEngfuncs.pfnFillRGBA( (int)s2[0], (int)s2[1], 2, 2, r, g, b, a );
			}
		}
	}
}

void Backtrack_SelfTest( void )
{
	int fail = Bt_SelfTest();

	gEngfuncs.Con_Printf( "BT SELFTEST: total=6 fail=%d %s\n", fail, fail ? "FAIL" : "PASS" );
}

static void Cmd_BtSelfTest( void )
{
	Backtrack_SelfTest();
}

static void Cmd_BtClear( void )
{
	Backtrack_Reset();
	gEngfuncs.Con_Printf( "bt: historico limpo\n" );
}

void Backtrack_Commands( void )
{
	gEngfuncs.pfnAddCommand( "bt_selftest", Cmd_BtSelfTest );
	gEngfuncs.pfnAddCommand( "bt_clear", Cmd_BtClear );
}

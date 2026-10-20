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
#include "sakura_more.h"
#include "cl_util.h"
#include "backtrack_cfg.h"

#define BT_X( var, name, def, lo, hi, ar, desc ) static cvar_t *cv_##var;
BT_CVAR_LIST( BT_X )
#undef BT_X

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

/* mascara de partes conforme cl_bt_parts */
static unsigned int PartMask( void )
{
	switch( (int)CV( cv_parts, 0.f ))
	{
	case 1:
		return ( 1u << AIM_PART_HEAD ) | ( 1u << AIM_PART_NECK );
	case 2:
		return ( 1u << AIM_PART_BELT ) | ( 1u << AIM_PART_STOMACH ) |
			( 1u << AIM_PART_LOWER_CHEST ) | ( 1u << AIM_PART_UPPER_CHEST );
	case 3:
		return ( 1u << AIM_PART_HEAD ) | ( 1u << AIM_PART_NECK ) |
			( 1u << AIM_PART_BELT ) | ( 1u << AIM_PART_STOMACH ) |
			( 1u << AIM_PART_LOWER_CHEST ) | ( 1u << AIM_PART_UPPER_CHEST );
	default:
		return 0xFFFFFFFFu;
	}
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
#define BT_X( var, name, def, lo, hi, ar, desc ) \
	cv_##var = gEngfuncs.pfnRegisterVariable( (char *)name, (char *)def, (ar) ? FCVAR_ARCHIVE : 0 );
	BT_CVAR_LIST( BT_X )
#undef BT_X

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
	Bt_TrimOlder( &g_bt, now, (int)BtClamp( CV( cv_history_ms, 1200.f ), 100.f, 3000.f ));

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

		if( (int)CV( cv_debug, 0.f ) >= 1 )
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

		if( On( cv_sync_fakelag, 0 ) && delay > 0 && g_attacking )
		{
			if( !g_flOwned )
			{
				g_flOldEnable = CVAR_GET_FLOAT( "cl_fakelag" );
				g_flOldLimit = CVAR_GET_FLOAT( "cl_fakelag_limit" );
				g_flOldType = CVAR_GET_FLOAT( "cl_fakelag_type" );
			}

			target = delay + (int)BtClamp( CV( cv_fakelag_extra, 0.f ), 0.f, 100.f );
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
	q->smooth = BtClamp( CV( cv_smooth, 0.5f ), 0.f, 1.f );

	/* janela automatica pelo ping (ida e volta) */
	q->max_ms = Bt_LagWindow( LocalPing(), q->min_ms, q->max_ms, On( cv_lag_comp, 0 ));

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
	bt_filter_s flt;
	float dx, dy, dz;

	if( !Backtrack_Enabled() || player <= 0 )
		return 0;

	if( !Bt_PartAllowed( PartMask(), part ))
		return 0;

	FillQuery( &q, Now() );

	if( !Bt_Lookup( &g_bt, &q, player, part, out, out_ms ))
		return 0;

	dx = out[0] - q.eye[0];
	dy = out[1] - q.eye[1];
	dz = out[2] - q.eye[2];

	memset( &flt, 0, sizeof( flt ));
	flt.team_ok = ( On( cv_team, 0 ) || g_iTeamNumber == 0 ||
		g_PlayerExtraInfo[player].teamnumber != g_iTeamNumber );
	flt.ping = LocalPing();
	flt.ping_min = (int)CV( cv_ping_min, 0.f );
	flt.dist = sqrtf( dx * dx + dy * dy + dz * dz );
	flt.max_dist = BtClamp( CV( cv_max_dist, 2500.f ), 100.f, 20000.f );
	flt.visible_only = On( cv_visible_only, 0 );
	flt.visible = flt.visible_only ? Aimbot_PointVisible( q.eye, out, player, On( cv_ignore_walls, 0 )) : 1;
	flt.attacking = g_attacking;
	flt.trigger_mode = (int)CV( cv_trigger_mode, 0.f );
	flt.key_held = Aimbot_KeyHeld();

	if( !Bt_FilterAccepts( &flt ))
		return 0;

	/* fov proprio: 0 usa o fov do aimbot */
	{
		float fov = CV( cv_fov, 0.f );

		if( fov > 0.f )
		{
			float a[3], view[3];

			Aimbot_AngleToPoint( q.eye, out, a );
			gEngfuncs.GetViewAngles( view );

			if( Aimbot_AngleDelta( view, a ) > BtClamp( fov, 0.5f, 180.f ))
				return 0;
		}
	}

	g_lastDelay = out_ms ? *out_ms : 0;
	g_lastAimTime = q.now;

	if( On( cv_notify, 0 ))
		Notify_Push( "bt: %d ms", g_lastDelay );

	if( (int)CV( cv_debug, 0.f ) >= 2 )
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
	int r = (int)BtClamp( CV( cv_color_r, 255.f ), 0.f, 255.f );
	int g = (int)BtClamp( CV( cv_color_g, 20.f ), 0.f, 255.f );
	int b = (int)BtClamp( CV( cv_color_b, 147.f ), 0.f, 255.f );
	int a = (int)BtClamp( CV( cv_draw_alpha, 200.f ), 0.f, 255.f );
	int style = (int)CV( cv_draw_style, 0.f );
	int wantDraw = On( cv_draw, 0 );
	int delay;

	/* indicador de delay: aparece mesmo sem desenhar o historico */
	if( On( cv_hud, 0 ))
	{
		delay = Backtrack_LastDelay();

		if( delay > 0 )
		{
			char buf[64];

			snprintf( buf, sizeof( buf ), "BT %d ms", delay );
			gEngfuncs.pfnDrawString( ScreenWidth - 90, 120, buf, r, g, b );
		}
	}

	if( !wantDraw )
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
			if( On( cv_draw_target_only, 0 ) && target > 0 && boxes[i].player != target )
				continue;
			if( !Bt_Lookup( &g_bt, &q, boxes[i].player, boxes[i].part, hist, &ms ))
				continue;

			delta[0] = hist[0] - boxes[i].origin[0];
			delta[1] = hist[1] - boxes[i].origin[1];
			delta[2] = hist[2] - boxes[i].origin[2];

			/* ghost = caixa atual deslocada para a posicao historica */
			if( style == 0 || style == 2 )
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
			if( style == 1 || style == 2 || On( cv_draw_line, 1 ))
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

	gEngfuncs.Con_Printf( "BT SELFTEST: total=10 fail=%d %s\n", fail, fail ? "FAIL" : "PASS" );
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

int Backtrack_Key( int down, int keynum )
{
	if( down && keynum > 0 && keynum == (int)CV( cv_reset_key, 0.f ))
	{
		Backtrack_Reset();
		Notify_Push( "bt: historico limpo" );
	}

	return 1;
}

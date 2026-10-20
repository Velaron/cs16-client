#pragma once

/*
 * Tabela unica das cvars do backtrack.
 *
 * Campos: sufixo da variavel em backtrack.cpp, nome da cvar, default,
 * minimo, maximo, archive (1 = vai pro config.cfg), descricao curta.
 *
 * Usada pelo registro (backtrack.cpp), pelo teste de host
 * (tests/bt_config_test.cpp) e como referencia do manifesto
 * tools/hub_cvars.txt. Nao inclui nada da engine.
 */

struct bt_cvar_def_s
{
	const char *name;
	const char *def;
	float lo;
	float hi;
	int archive;
	const char *desc;
};

#define BT_CVAR_LIST( X ) \
	X( enable,        "cl_bt_enable",           "0",     0.f,   1.f, 1, "liga o backtrack" ) \
	X( mode,          "cl_bt_mode",             "1",     0.f,   3.f, 1, "0 off, 1 legit, 2 rage, 3 adaptativo" ) \
	X( trigger_mode,  "cl_bt_trigger_mode",     "0",     0.f,   2.f, 1, "0 sempre, 1 so ao atirar, 2 tecla" ) \
	X( key,           "cl_bt_key",              "0",     0.f, 255.f, 1, "tecla de ativacao" ) \
	X( team,          "cl_bt_team",             "0",     0.f,   1.f, 1, "mirar no proprio time" ) \
	X( min_ms,        "cl_bt_min_ms",           "40",    0.f, 400.f, 1, "delay minimo" ) \
	X( max_ms,        "cl_bt_max_ms",           "200",   0.f, 800.f, 1, "delay maximo" ) \
	X( step_ms,       "cl_bt_step_ms",          "20",    0.f, 100.f, 1, "resolucao do delay" ) \
	X( prefer,        "cl_bt_prefer",           "0",     0.f,   3.f, 1, "0 recente, 1 antigo, 2 angulo, 3 perto" ) \
	X( visible_only,  "cl_bt_visible_only",     "0",     0.f,   1.f, 1, "so ponto visivel" ) \
	X( ignore_walls,  "cl_bt_ignore_walls",     "0",     0.f,   1.f, 1, "ignorar paredes na visibilidade" ) \
	X( max_dist,      "cl_bt_max_dist",         "2500", 100.f, 20000.f, 1, "distancia maxima" ) \
	X( ping_min,      "cl_bt_ping_min",         "0",     0.f, 300.f, 1, "ping minimo para usar" ) \
	X( sync_fakelag,  "cl_bt_sync_fakelag",     "0",     0.f,   1.f, 1, "sincronizar com cl_fakelag" ) \
	X( fakelag_extra, "cl_bt_fakelag_extra",    "0",     0.f, 100.f, 1, "ms extras no fakelag" ) \
	X( draw,          "cl_bt_draw",             "0",     0.f,   1.f, 1, "desenhar historico" ) \
	X( draw_ms,       "cl_bt_draw_ms",          "120",   0.f, 800.f, 1, "janela do desenho" ) \
	X( draw_line,     "cl_bt_draw_line",        "1",     0.f,   1.f, 1, "marcar pontos" ) \
	X( draw_target_only, "cl_bt_draw_target_only", "0",     0.f,   1.f, 1, "so o alvo atual" ) \
	X( interp,        "cl_bt_interp",           "1",     0.f,   1.f, 1, "interpolar com o quadro vizinho" ) \
	X( record_ms,     "cl_bt_record_ms",        "10",    5.f,  50.f, 1, "intervalo de gravacao" ) \
	X( max_frames,    "cl_bt_max_frames",       "96",    8.f,  96.f, 1, "quadros usados" ) \
	X( clear_round,   "cl_bt_clear_round",      "1",     0.f,   1.f, 1, "limpar ao morrer" ) \
	X( debug,         "cl_bt_debug",            "0",     0.f,   3.f, 1, "0 off, 1 resumo, 2 alvo, 3 tudo" ) \
	X( fov,           "cl_bt_fov",              "0",     0.f, 180.f, 1, "fov proprio (0 = fov do aimbot)" ) \
	X( parts,         "cl_bt_parts",            "0",     0.f,   3.f, 1, "0 todos, 1 cabeca, 2 corpo, 3 cabeca+corpo" ) \
	X( lag_comp,      "cl_bt_lag_comp",         "0",     0.f,   1.f, 1, "janela automatica pelo ping" ) \
	X( smooth,        "cl_bt_smooth",           "0.5",   0.f,   1.f, 1, "peso da interpolacao" ) \
	X( color_r,       "cl_bt_color_r",          "255",   0.f, 255.f, 1, "cor do ghost - vermelho" ) \
	X( color_g,       "cl_bt_color_g",          "20",    0.f, 255.f, 1, "cor do ghost - verde" ) \
	X( color_b,       "cl_bt_color_b",          "147",   0.f, 255.f, 1, "cor do ghost - azul" ) \
	X( draw_alpha,    "cl_bt_draw_alpha",       "200",   0.f, 255.f, 1, "transparencia do ghost" ) \
	X( draw_style,    "cl_bt_draw_style",       "0",     0.f,   2.f, 1, "0 caixa, 1 pontos, 2 ambos" ) \
	X( hud,           "cl_bt_hud",              "0",     0.f,   1.f, 1, "indicador de delay na tela" ) \
	X( history_ms,    "cl_bt_history_ms",       "1200",100.f,3000.f, 1, "idade maxima dos quadros" ) \
	X( reset_key,     "cl_bt_reset_key",        "0",     0.f, 255.f, 1, "tecla que limpa o historico" ) \
	X( notify,        "cl_bt_notify",           "0",     0.f,   1.f, 1, "avisar quando aplica rewind" )

extern const bt_cvar_def_s kBtCvars[];
extern const int kBtCvarCount;

/* devolve o indice da cvar na tabela ou -1 */
int BtCfg_Find( const char *name );

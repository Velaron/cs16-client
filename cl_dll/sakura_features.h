#pragma once

/*
 * sakura_features.h - features do Sakura que faltavam no client.
 *
 * Modulos:
 *   movimento : bhop, fastrun, groundstrafe, jumpbug
 *   rage      : norecoil, nospread, antiaim
 *   misc      : namestealer, chatspammer, antiafk
 *   visuals   : crosshair, removals (maos), fov
 *   esp       : box, nome, vida, distancia, snapline, radar
 *
 * Todos os cvars usam o prefixo cl_sak_ para nao colidir com os cvars
 * antigos (cl_chams, cl_bhop, cl_norecoil...) que sobraram no config.
 */

struct usercmd_s;
struct local_state_s;
struct ref_params_s;

void Sakura_Init( void );

/* HUD_PostRunCmd: guarda estado local (punch, flags, velocidade, seed). */
void Sakura_PostRunCmd( struct local_state_s *from, struct local_state_s *to,
	struct usercmd_s *cmd, unsigned int seed );

/* CL_CreateMove, antes do aimbot: movimento e antiafk. */
void Sakura_PreMove( struct usercmd_s *cmd, float frametime );

/* CL_CreateMove, depois do aimbot: antiaim, norecoil e nospread. */
void Sakura_PostMove( struct usercmd_s *cmd, const float viewangles[3] );

/* HUD_Redraw: crosshair e ESP. */
void Sakura_Draw( void );

/* 0 = tecla consumida, 1 = passa para a engine. */
int  Sakura_Key( int down, int keynum );

/* Remocao do viewmodel (maos). */
int  Sakura_DrawViewModel( void );

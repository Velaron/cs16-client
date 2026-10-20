#pragma once

/*
 * backtrack.h - glue do backtrack com a engine e o HUD.
 * A logica pura (buffer, janela, escolha) fica em backtrack_core.h.
 */

void Backtrack_Init( void );
void Backtrack_Reset( void );

/* grava um frame; chamar por frame depois de EspHitbox_Flush() */
void Backtrack_Tick( void );

/* desenha os ghosts (chamar no HUD_Redraw, depois do ESP) */
void Backtrack_Draw( void );

int  Backtrack_Enabled( void );

/* ponto historico de (jogador, parte); 1 = achou */
int  Backtrack_GetPoint( int player, int part, float out[3], int *out_ms );

/* ultimo delay usado na mira, em ms (0 = nenhum) */
int  Backtrack_LastDelay( void );

void Backtrack_SelfTest( void );

/* contexto do comando atual, enviado pelo aimbot antes de mirar */
void Backtrack_SetContext( int attacking );

/* registra os comandos de console (bt_selftest, bt_clear) */
void Backtrack_Commands( void );
/* tecla propria (reset do historico). Retorna 1 = passa pra engine. */
int  Backtrack_Key( int down, int keynum );

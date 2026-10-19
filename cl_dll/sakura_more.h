#pragma once

/*
 * sakura_more.h - funcoes do Sakura que faltavam no client:
 *
 *   strafe  : auto-strafe no ar (HNS)
 *   notify  : mensagens no canto da tela (toasts)
 *   route   : gravacao e desenho de waypoints do mapa
 */

struct usercmd_s;

void SakuraMore_Init( void );

/* CL_CreateMove, logo depois de Sakura_PreMove. */
void SakuraMore_PreMove( struct usercmd_s *cmd );

/* HUD_Redraw, depois do ESP. */
void SakuraMore_Draw( void );

/* Fila de mensagens (usada por dump, killsound, skins...). */
void Notify_Push( const char *fmt, ... );

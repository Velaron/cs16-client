#pragma once

/*
 * sakura_extra.h - features do Sakura que faltavam:
 * chams, knifebot, som de kill/hit, nightmode, skins e cores do menu.
 */

struct usercmd_s;
struct cl_entity_s;

void SakuraExtra_Init( void );

/* chamado no CL_CreateMove depois do aimbot (knifebot). */
void SakuraExtra_PostMove( struct usercmd_s *cmd, const float viewangles[3] );

/* chamado por frame no HUD_Redraw (sons, nightmode, skins). */
void SakuraExtra_Tick( void );

/* chams: aplica antes do draw do modelo e restaura depois. */
void Chams_Begin( struct cl_entity_s *ent );
void Chams_End( struct cl_entity_s *ent );

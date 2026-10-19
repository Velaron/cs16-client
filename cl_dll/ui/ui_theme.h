#pragma once

#include "cvardef.h"

/*
 * Tema e widgets do HUB no estilo Sakura.
 * Nao conhece nenhuma feature: so desenha e escreve cvars.
 */

namespace UI
{
	/* paleta extraida do menu do Sakura */
	extern const float kHeader[3];   /* 0.0745 0.0862 0.1019 */
	extern const float kBody[3];     /* 0.0980 0.1176 0.1372 */
	extern const float kFooter[3];   /* 0.1764 0.1960 0.2156 */
	extern const float kAccent[3];   /* 1.0000 0.0784 0.5764 */

	/* geometria da janela */
	const float kWinW     = 720.f;
	const float kWinH     = 490.f;
	const float kHeaderH  = 50.f;
	const float kFooterH  = 20.f;
	const float kSubTabW  = 150.f;
	const float kSubTabH  = 25.f;
	const float kContentX = 170.f;
	const float kContentY = 60.f;
	const float kContentW = 530.f;
	const float kContentH = 400.f;

	void Init( void );

	/* Apply empilha 4 style vars e 8 cores; Pop desempilha tudo. */
	void Apply( void );
	void Pop( void );

	/* widgets */
	bool SubTab( const char *label, bool active );
	bool Check( const char *cvar, const char *label );
	bool Slider( const char *cvar, const char *label, float lo, float hi, const char *fmt );
	bool Combo( const char *cvar, const char *label, const char *items );
	bool KeyBind( const char *cvar, const char *label );
	void Section( const char *title );

	/* cvars */
	cvar_t *Cvar( const char *name );
	float   Get( const char *name, float fallback );
	void    Set( const char *name, float value );
	const char *Str( const char *name, const char *fallback );
}

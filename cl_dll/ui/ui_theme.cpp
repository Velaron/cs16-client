#include <string.h>
#include <stdio.h>

#include "hud.h"
#include "cl_util.h"
#include "imgui.h"
#include "ui_theme.h"

namespace UI
{
	const float kHeader[3] = { 0.0745f, 0.0862f, 0.1019f };
	const float kBody[3]   = { 0.0980f, 0.1176f, 0.1372f };
	const float kFooter[3] = { 0.1764f, 0.1960f, 0.2156f };
	const float kAccent[3] = { 1.0000f, 0.0784f, 0.5764f };

	struct cvar_cache_t
	{
		char name[64];
		cvar_t *cv;
	};

	static cvar_cache_t s_cache[192];
	static int s_cacheCount;

	void Init( void )
	{
		memset( s_cache, 0, sizeof( s_cache ) );
		s_cacheCount = 0;
	}

	cvar_t *Cvar( const char *name )
	{
		int i;

		if( !name || !name[0] )
			return NULL;

		for( i = 0; i < s_cacheCount; i++ )
		{
			if( !strcmp( s_cache[i].name, name ) )
				return s_cache[i].cv;
		}

		/* nao cacheia ponteiro nulo: a cvar pode ser registrada depois */
		{
			cvar_t *cv = (cvar_t *)gEngfuncs.pfnGetCvarPointer( name );

			if( !cv )
				return NULL;

			if( s_cacheCount < (int)( sizeof( s_cache ) / sizeof( s_cache[0] ) ) )
			{
				strncpy( s_cache[s_cacheCount].name, name, sizeof( s_cache[0].name ) - 1 );
				s_cache[s_cacheCount].name[sizeof( s_cache[0].name ) - 1] = 0;
				s_cache[s_cacheCount].cv = cv;
				s_cacheCount++;
			}
			return cv;
		}
	}

	float Get( const char *name, float fallback )
	{
		cvar_t *cv = Cvar( name );

		return cv ? cv->value : fallback;
	}

	void Set( const char *name, float value )
	{
		if( Cvar( name ) )
			gEngfuncs.Cvar_SetValue( name, value );
	}

	const char *Str( const char *name, const char *fallback )
	{
		cvar_t *cv = Cvar( name );

		return ( cv && cv->string ) ? cv->string : fallback;
	}

	void Apply( void )
	{
		/* cores vem dos cvars (tab Menu); fallback na paleta do Sakura */
		const float ar = Get( "cl_hub_accent", kAccent[0] );
		const float ag = Get( "cl_hub_accent_g", kAccent[1] );
		const float ab = Get( "cl_hub_accent_b", kAccent[2] );
		const float br = Get( "cl_hub_bg", kBody[0] );
		const float bg = Get( "cl_hub_bg_g", kBody[1] );
		const float bb = Get( "cl_hub_bg_b", kBody[2] );

		ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
		ImGui::PushStyleVar( ImGuiStyleVar_WindowRounding, 0.0f );
		ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 3.0f );
		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 6, 6 ) );

		ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 1.f, 1.f, 1.f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( br + 0.06f, bg + 0.06f, bb + 0.06f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_SliderGrab, ImVec4( ar, ag, ab, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_SliderGrabActive, ImVec4( ar, ag, ab, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_CheckMark, ImVec4( ar, ag, ab, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( br + 0.08f, bg + 0.08f, bb + 0.08f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( ar * 0.55f, ag * 0.55f, ab * 0.55f, 1.f ) );
		ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( ar, ag, ab, 1.f ) );
	}

	void Pop( void )
	{
		ImGui::PopStyleColor( 8 );
		ImGui::PopStyleVar( 4 );
	}

	void Section( const char *title )
	{
		ImGui::Spacing();
		ImGui::TextColored( ImVec4( kAccent[0], kAccent[1], kAccent[2], 1.f ), "%s", title );
		ImGui::Separator();
	}

	bool SubTab( const char *label, bool active )
	{
		bool pressed;

		if( active )
			ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( kAccent[0], kAccent[1], kAccent[2], 1.f ) );

		pressed = ImGui::Button( label, ImVec2( kSubTabW, kSubTabH ) );

		if( active )
			ImGui::PopStyleColor();

		return pressed;
	}

	bool Check( const char *cvar, const char *label )
	{
		bool on = Get( cvar, 0 ) > 0.5f;

		if( ImGui::Checkbox( label, &on ) )
		{
			Set( cvar, on ? 1.f : 0.f );
			return true;
		}
		return false;
	}

	bool Slider( const char *cvar, const char *label, float lo, float hi, const char *fmt )
	{
		char id[96];
		float v = Get( cvar, lo );

		snprintf( id, sizeof( id ), "##%s", label );

		ImGui::TextUnformatted( label );
		ImGui::SameLine( 190 );
		ImGui::SetNextItemWidth( -1 );

		if( ImGui::SliderFloat( id, &v, lo, hi, fmt ) )
		{
			Set( cvar, v );
			return true;
		}
		return false;
	}

	bool Combo( const char *cvar, const char *label, const char *items )
	{
		char id[96];
		int v = (int)Get( cvar, 0 );

		snprintf( id, sizeof( id ), "##%s", label );

		ImGui::TextUnformatted( label );
		ImGui::SameLine( 190 );
		ImGui::SetNextItemWidth( -1 );

		if( ImGui::Combo( id, &v, items ) )
		{
			Set( cvar, (float)v );
			return true;
		}
		return false;
	}

	bool KeyBind( const char *cvar, const char *label )
	{
		char buf[32];
		int key = (int)Get( cvar, 0 );

		if( key > 0 )
			snprintf( buf, sizeof( buf ), "tecla %d", key );
		else
			snprintf( buf, sizeof( buf ), "definir" );

		ImGui::TextUnformatted( label );
		ImGui::SameLine( 190 );
		ImGui::SetNextItemWidth( -1 );

		return ImGui::Button( buf, ImVec2( -1, 0 ) );
	}
} /* namespace UI */

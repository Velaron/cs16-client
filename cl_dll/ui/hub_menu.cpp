/*
 * hub_menu.cpp - HUB_MENU unico do client, na tecla F6.
 *
 * Reproduz a anatomia do menu do Sakura: janela 720x490, header com as
 * categorias, coluna de sub-tabs 150x25 e area de conteudo 530x400.
 *
 * Este arquivo NAO implementa feature nenhuma: ele so desenha e escreve
 * cvars. A logica de cada cheat vive no modulo dela (aimbot.cpp,
 * sakura_features.cpp, sound_esp.cpp, ...).
 */

#include <string.h>
#include <stdio.h>

#include "hud.h"
#include "cl_util.h"
#include "keydefs.h"
#include "imgui.h"
#include "imgui_host.h"
#include "ui_theme.h"
#include "hub_menu.h"
#include "hub_cvars_list.h"
#include "server_dump.h"

/* ------------------------------------------------------------------ */
/* navegacao                                                          */
/* ------------------------------------------------------------------ */

enum
{
	CAT_RAGE = 0,
	CAT_LEGIT,
	CAT_VISUALS,
	CAT_MISC,
	CAT_COUNT
};

static const char *kCatName[CAT_COUNT] = { "Rage", "Legit", "Visuals", "Misc" };

static const char *kSubRage[]    = { "Aimbot", "Anti Aim", "Fake Lag", "Speedhack", NULL };
static const char *kSubLegit[]   = { "Aimbot", "Triggerbot", "Perfect Silent", "Recoil", "Knifebot", NULL };
static const char *kSubVisuals[] = { "Player", "Local", "World", "Radar", "Skins", "Removals", "Sound", "Menu", NULL };
static const char *kSubMisc[]    = { "Movement", "Servers", "Keybindings", "Player List", "Config", "AntiAFK", NULL };

static const char **kSubTabs[CAT_COUNT] = { kSubRage, kSubLegit, kSubVisuals, kSubMisc };

static int g_open;
static int g_cat = CAT_LEGIT;
static int g_sub[CAT_COUNT];

/* ------------------------------------------------------------------ */
/* partes do corpo (ordem do menu do Sakura)                          */
/* ------------------------------------------------------------------ */

static const char *kPartCvar[21] =
{
	"cl_aim_belt",
	"cl_aim_lthigh", "cl_aim_ltibia", "cl_aim_lfoot",
	"cl_aim_rthigh", "cl_aim_rtibia", "cl_aim_rfoot",
	"cl_aim_stomach", "cl_aim_lchest", "cl_aim_uchest",
	"cl_aim_neck", "cl_aim_head",
	"cl_aim_lshoulder", "cl_aim_lelbow", "cl_aim_larm", "cl_aim_lhand",
	"cl_aim_rshoulder", "cl_aim_relbow", "cl_aim_rarm", "cl_aim_rhand",
	"cl_aim_generic"
};

static const char *kPartLabel[21] =
{
	"Cintura",
	"Coxa esquerda", "Tibia esquerda", "Pe esquerdo",
	"Coxa direita", "Tibia direita", "Pe direito",
	"Barriga", "Peito inferior", "Peito superior",
	"Pescoco", "Cabeca",
	"Ombro esquerdo", "Cotovelo esquerdo", "Braco esquerdo", "Mao esquerda",
	"Ombro direito", "Cotovelo direito", "Braco direito", "Mao direita",
	"Resto do corpo"
};

/* ------------------------------------------------------------------ */
/* helpers                                                            */
/* ------------------------------------------------------------------ */

static int On( const char *cvar )
{
	return UI::Get( cvar, 0 ) > 0.5f;
}

static void Hint( const char *text )
{
	ImGui::TextDisabled( "%s", text );
}

static void TabAimbotRage( void );
static void TabAimbotLegit( void );
static void TabTrigger( void );
static void TabPerfectSilent( void );
static void TabRecoil( void );
static void TabKnifebot( void );
static void TabAntiAim( void );
static void TabPlayer( void );
static void TabLocal( void );
static void TabWorld( void );
static void TabRadar( void );
static void TabSkins( void );
static void TabRemovals( void );
static void TabSound( void );
static void TabMenu( void );
static void TabMovement( void );
static void TabServers( void );
static void TabKeybinds( void );
static void TabPlayerList( void );
static void TabConfig( void );
static void TabAntiAfk( void );

/* ------------------------------------------------------------------ */
/* tabs: legit                                                        */
/* ------------------------------------------------------------------ */

static void TabAimbotLegit( void )
{
	UI::Section( "Aimbot legit" );
	UI::Check( "cl_aim_assist", "Ligado" );
	UI::Check( "cl_aim_team", "Mirar no time" );
	UI::Check( "cl_aim_walls", "Atravessar parede" );
	UI::Check( "cl_aim_draw", "Desenhar alvo" );
	UI::Check( "cl_aim_drawfov", "Circulo de FOV" );

	UI::Section( "Ativacao" );
	UI::Combo( "cl_aim_mode", "Modo", "Sempre\0So ao atirar\0Tecla\0" );
	if( (int)UI::Get( "cl_aim_mode", 1 ) == 2 )
		UI::KeyBind( "cl_aim_key", "Tecla" );

	UI::Section( "Mira" );
	UI::Slider( "cl_aim_fov", "FOV", 0.5f, 45.f, "%.1f" );
	UI::Slider( "cl_aim_smooth", "Smooth", 1.f, 250.f, "%.0f" );
	UI::Combo( "cl_aim_smooth_mode", "Modo do smooth", "Linear\0Tempo\0Instantaneo\0" );
	UI::Slider( "cl_aim_auto_speed", "Auto aim speed", 0.f, 100.f, "%.0f" );
	UI::Slider( "cl_aim_reaction", "Reaction time", 0.f, 1000.f, "%.0f ms" );
	UI::Slider( "cl_aim_speed_fov", "Speed scale FOV", 0.f, 100.f, "%.0f %%" );
	UI::Slider( "cl_aim_blockkill", "Block attack", 0.f, 1000.f, "%.0f ms" );

	UI::Section( "Humanizacao" );
	UI::Check( "cl_aim_humanize", "Humanize (spiral)" );
	UI::Slider( "cl_aim_spiral_x", "Spiral X", 0.1f, 10.f, "%.1f" );
	UI::Slider( "cl_aim_spiral_y", "Spiral Y", 0.1f, 10.f, "%.1f" );

	UI::Section( "Extras" );
	UI::Check( "cl_aim_flashcheck", "Flash check" );
	UI::Slider( "cl_aim_flash_pct", "Flash limite", 0.f, 100.f, "%.0f %%" );
	UI::Check( "cl_aim_fastzoom", "Fast zoom" );
	UI::Combo( "cl_aim_hit_mode", "Escolha do hitbox", "Prioridade\0Mais perto da mira\0" );
	UI::Check( "cl_aim_debug", "Debug no console" );

	Hint( "Smooth 1 = instantaneo, 250 = bem lento." );
}

static void TabTrigger( void )
{
	UI::Section( "Triggerbot" );
	UI::Check( "cl_trig_enable", "Ligado" );
	UI::Check( "cl_trig_team", "Mirar no time" );
	UI::Check( "cl_trig_wall", "Atirar na parede" );
	UI::Check( "cl_trig_zoom", "So com zoom" );
	UI::Check( "cl_trig_draw", "Desenhar alvo" );

	UI::Section( "Tempo" );
	UI::Slider( "cl_trig_fov", "FOV do gatilho", 0.05f, 10.f, "%.2f" );
	UI::Slider( "cl_trig_delay", "Delay", 0.f, 1000.f, "%.0f ms" );
	UI::Slider( "cl_trig_fire", "Intervalo do tiro", 10.f, 1000.f, "%.0f ms" );

	UI::Section( "Rajada" );
	if( (int)UI::Get( "cl_trig_mode", 0 ) == 0 )
	{
		UI::Combo( "cl_trig_mode", "Tipo", "Rajada fixa\0Rajada aleatoria\0" );
		UI::Slider( "cl_trig_count", "Tiros", 1.f, 5.f, "%.0f" );
	}
	else
	{
		UI::Combo( "cl_trig_mode", "Tipo", "Rajada fixa\0Rajada aleatoria\0" );
		UI::Slider( "cl_trig_randmax", "Maximo aleatorio", 2.f, 5.f, "%.0f" );
	}

	UI::Section( "Precisao" );
	UI::Combo( "cl_trig_accuracy", "Accuracy boost", "Desligado\0So mirando\0Recuo\0Recuo + spread\0" );
}

static void TabPerfectSilent( void )
{
	UI::Section( "Silent" );
	UI::Check( "cl_aim_silent", "Silent aim" );
	Hint( "O comando vai para o alvo e a camera nao se move." );

	UI::Section( "Perfect silent" );
	UI::Check( "cl_aim_psilent", "Perfect silent" );
	UI::Slider( "cl_aim_psilent_fov", "FOV", 0.1f, 15.f, "%.1f" );
	UI::Check( "cl_aim_psilent_spread", "Compensar spread" );
	Hint( "Perfect silent so age quando o gatilho esta apertado." );
}

static void TabRecoil( void )
{
	UI::Section( "Recoil compensation" );
	UI::Slider( "cl_aim_rc_pitch", "Pitch", 0.f, 100.f, "%.0f %%" );
	UI::Slider( "cl_aim_rc_yaw", "Yaw", 0.f, 100.f, "%.0f %%" );
	UI::Slider( "cl_aim_rc_fov", "FOV adicional", 0.f, 150.f, "%.0f %%" );
	UI::Slider( "cl_aim_rc_smooth", "Smooth adicional", 0.f, 150.f, "%.0f %%" );
	UI::Slider( "cl_aim_rc_after", "Depois de tiros", 0.f, 10.f, "%.0f" );

	UI::Section( "Rage" );
	UI::Check( "cl_sak_norecoil", "Sem recuo" );
	UI::Check( "cl_sak_nospread", "Sem spread" );

	UI::Section( "Precisao" );
	UI::Combo( "cl_aim_accuracy", "Accuracy boost", "Desligado\0Mirando\0Recuo\0Recuo + spread\0" );
}

static void TabKnifebot( void )
{
	UI::Section( "Knifebot" );
	UI::Check( "cl_sak_knifebot", "Ligado" );
	UI::Slider( "cl_sak_knifebot_range", "Alcance", 16.f, 96.f, "%.0f" );
	Hint( "Da stab quando o alvo esta de costas, slash na frente." );
}

/* ------------------------------------------------------------------ */
/* tabs: rage                                                         */
/* ------------------------------------------------------------------ */

static void TabAimbotRage( void )
{
	int i;
	int half = 11;

	UI::Section( "Rage aimbot" );
	UI::Check( "cl_aim_assist", "Ligado" );
	UI::Check( "cl_aim_walls", "Atravessar parede" );
	UI::Check( "cl_aim_team", "Mirar no time" );
	UI::Slider( "cl_aim_fov", "FOV", 0.5f, 180.f, "%.1f" );
	UI::Slider( "cl_aim_smooth", "Smooth", 1.f, 20.f, "%.0f" );
	UI::Check( "cl_aim_silent", "Silent" );
	UI::Check( "cl_aim_psilent", "Perfect silent" );

	UI::Section( "Hitboxes" );
	if( ImGui::BeginChild( "##hit_l", ImVec2( 240, 260 ), false ) )
	{
		for( i = 0; i < half; i++ )
			UI::Check( kPartCvar[i], kPartLabel[i] );
	}
	ImGui::EndChild();
	ImGui::SameLine();
	if( ImGui::BeginChild( "##hit_r", ImVec2( 240, 260 ), false ) )
	{
		for( i = half; i < 21; i++ )
			UI::Check( kPartCvar[i], kPartLabel[i] );
	}
	ImGui::EndChild();
}

static void TabAntiAim( void )
{
	UI::Section( "Anti aim" );
	UI::Check( "cl_sak_antiaim", "Ligado" );
	UI::Combo( "cl_sak_aa_pitch", "Pitch", "Nenhum\0Cima\0Baixo\0Fake\0" );
	UI::Combo( "cl_sak_aa_yaw", "Yaw", "Nenhum\0Esquerda\0Direita\0Jitter\0" );
	UI::Slider( "cl_sak_aa_speed", "Velocidade", 1.f, 30.f, "%.0f" );
	Hint( "Anti aim mexe nos angulos enviados ao servidor." );
}

/* ------------------------------------------------------------------ */
/* tabs: visuals                                                      */
/* ------------------------------------------------------------------ */

static void TabPlayer( void )
{
	UI::Section( "ESP" );
	UI::Check( "cl_sak_esp_box", "Caixa" );
	UI::Check( "cl_sak_esp_name", "Nome" );
	UI::Check( "cl_sak_esp_health", "Vida" );
	UI::Check( "cl_sak_esp_dist", "Distancia" );
	UI::Check( "cl_sak_esp_snapline", "Snapline" );
	UI::Check( "cl_sak_esp_team", "Mostrar time" );
	UI::Slider( "cl_sak_esp_maxdist", "Distancia maxima", 100.f, 8000.f, "%.0f" );

	UI::Section( "Chams" );
	UI::Check( "cl_sak_chams", "Ligado" );
	UI::Combo( "cl_sak_chams_color", "Cor", "Time\0Vermelho\0Verde\0Azul\0Branco\0" );
	Hint( "Chams colorido. Atravessar parede exige mudanca no renderer." );
}

static void TabLocal( void )
{
	UI::Section( "Camera" );
	UI::Slider( "cl_sak_fov", "FOV", 0.f, 140.f, "%.0f" );
	Hint( "0 usa o FOV do jogo." );

	UI::Section( "Crosshair" );
	UI::Check( "cl_sak_crosshair", "Crosshair propria" );
	UI::Slider( "cl_sak_crosshair_size", "Tamanho", 1.f, 20.f, "%.0f" );
	UI::Slider( "cl_sak_crosshair_gap", "Espaco", 0.f, 20.f, "%.0f" );
}

static void TabWorld( void )
{
	UI::Section( "Mundo" );
	UI::Check( "cl_sak_world_night", "Nightmode" );
	Hint( "Nightmode usa os cvars de luz do engine quando existirem." );
}

static void TabRadar( void )
{
	UI::Section( "Radar" );
	UI::Check( "cl_sak_esp_radar", "Radar circular" );
	UI::Slider( "cl_sak_esp_maxdist", "Distancia maxima", 100.f, 8000.f, "%.0f" );
	Hint( "Pontos vermelhos = inimigos vistos pelo ESP." );
}

static void TabSkins( void )
{
	UI::Section( "Skins" );
	UI::Combo( "cl_sak_skin_weapon", "Arma", "ak47\0m4a1\0awp\0deagle\0usp\0glock\0knife\0" );
	UI::Combo( "cl_sak_skin", "Skin", "Original\0Skin 1\0Skin 2\0Skin 3\0Skin 4\0" );
	Hint( "Usa os comandos skin_<arma> que o client ja carrega." );
}

static void TabRemovals( void )
{
	UI::Section( "Remocoes" );
	UI::Check( "cl_sak_remove_hands", "Remover maos" );
	UI::Check( "cl_aim_drawfov", "Circulo de FOV" );
	UI::Check( "cl_aim_draw", "Caixa do alvo" );
}

static void TabSound( void )
{
	UI::Section( "Sound ESP" );
	UI::Check( "cl_sak_esp_sound", "Ligado" );
	UI::Slider( "cl_sak_esp_sound_range", "Alcance", 200.f, 8000.f, "%.0f" );
	UI::Slider( "cl_sak_esp_sound_time", "Duracao", 0.2f, 4.f, "%.1f s" );
	Hint( "Desenha um circulo no lugar de cada som de inimigo." );

	UI::Section( "Sons" );
	UI::Check( "cl_sak_killsound", "Som ao matar" );
	UI::Check( "cl_sak_hitsound", "Som ao acertar" );
}

static void TabMenu( void )
{
	float accent[3];
	float bg[3];

	UI::Section( "Cores do menu" );
	accent[0] = UI::Get( "cl_hub_accent", 1.0f );
	accent[1] = UI::Get( "cl_hub_accent_g", 0.0784f );
	accent[2] = UI::Get( "cl_hub_accent_b", 0.5764f );
	if( ImGui::ColorEdit3( "Destaque", accent ) )
	{
		UI::Set( "cl_hub_accent", accent[0] );
		UI::Set( "cl_hub_accent_g", accent[1] );
		UI::Set( "cl_hub_accent_b", accent[2] );
	}

	bg[0] = UI::Get( "cl_hub_bg", 0.0980f );
	bg[1] = UI::Get( "cl_hub_bg_g", 0.1176f );
	bg[2] = UI::Get( "cl_hub_bg_b", 0.1372f );
	if( ImGui::ColorEdit3( "Fundo", bg ) )
	{
		UI::Set( "cl_hub_bg", bg[0] );
		UI::Set( "cl_hub_bg_g", bg[1] );
		UI::Set( "cl_hub_bg_b", bg[2] );
	}

	UI::Section( "Teclas" );
	Hint( "F6 abre e fecha o HUB." );
	Hint( "ESC fecha o HUB." );
	Hint( "Setas trocam de categoria e sub-tab." );
}

/* ------------------------------------------------------------------ */
/* tabs: misc                                                         */
/* ------------------------------------------------------------------ */

static void TabMovement( void )
{
	UI::Section( "Movimento" );
	UI::Check( "cl_sak_bhop", "Bhop" );
	UI::Check( "cl_sak_fastrun", "Fast run" );
	UI::Check( "cl_sak_groundstrafe", "Ground strafe" );
	UI::Check( "cl_sak_jumpbug", "Jump bug" );
	Hint( "Bhop segura o pulo enquanto a tecla de pular estiver pressionada." );
}

static void TabServers( void )
{
	/* o painel antigo virou conteudo desta tab */
	ServerDump_DrawContent();
}

static void TabKeybinds( void )
{
	UI::Section( "Teclas do HUB" );
	UI::KeyBind( "cl_aim_key", "Tecla da mira" );
	UI::Combo( "cl_aim_mode", "Modo da mira", "Sempre\0So ao atirar\0Tecla\0" );
	Hint( "A captura de tecla sera habilitada na proxima fase." );
}

static void TabPlayerList( void )
{
	int i;
	int maxclients = gEngfuncs.GetMaxClients();

	UI::Section( "Jogadores" );

	if( ImGui::BeginChild( "##plist", ImVec2( -1, -1 ), false ) )
	{
		for( i = 1; i <= maxclients && i < 33; i++ )
		{
			const char *name = g_PlayerInfoList[i].name;

			if( !name || !name[0] )
				continue;

			ImGui::Text( "%2d  %-20s  time %d  ping %d", i, name,
				g_PlayerExtraInfo[i].teamnumber, g_PlayerInfoList[i].ping );
		}
	}
	ImGui::EndChild();
}

static void TabConfig( void )
{
	UI::Section( "Configuracao" );

	if( ImGui::Button( "Salvar config.cfg", ImVec2( -1, 0 ) ) )
		gEngfuncs.pfnClientCmd( "host_writeconfig\n" );

	if( ImGui::Button( "Recarregar config.cfg", ImVec2( -1, 0 ) ) )
		gEngfuncs.pfnClientCmd( "exec config.cfg\n" );

	Hint( "As cvars do HUB usam FCVAR_ARCHIVE e vao para o config.cfg." );
}

static void TabAntiAfk( void )
{
	static char chat[128];
	static int loaded;

	UI::Section( "Anti AFK" );
	UI::Check( "cl_sak_antiafk", "Ligado" );

	UI::Section( "Nome" );
	UI::Check( "cl_sak_namestealer", "Name stealer" );

	UI::Section( "Chat spam" );
	UI::Check( "cl_sak_chatspam", "Ligado" );
	UI::Slider( "cl_sak_chatspam_delay", "Intervalo", 1.f, 60.f, "%.0f s" );

	if( !loaded )
	{
		strncpy( chat, UI::Str( "cl_sak_chatspam_text", "" ), sizeof( chat ) - 1 );
		chat[sizeof( chat ) - 1] = 0;
		loaded = 1;
	}
	ImGui::TextUnformatted( "Texto" );
	ImGui::SameLine( 190 );
	ImGui::SetNextItemWidth( -1 );
	if( ImGui::InputText( "##chatspam", chat, sizeof( chat ) ) )
		gEngfuncs.Cvar_Set( "cl_sak_chatspam_text", chat );
}

/* ------------------------------------------------------------------ */
/* dispatch                                                           */
/* ------------------------------------------------------------------ */

static void DrawTabBody( void )
{
	switch( g_cat )
	{
	case CAT_RAGE:
		switch( g_sub[CAT_RAGE] )
		{
		case 0: TabAimbotRage(); break;
		case 1: TabAntiAim(); break;
		case 2:
			UI::Section( "Fake lag" );
			Hint( "Requer segurar pacotes na engine (fase 8 do plano)." );
			break;
		default:
			UI::Section( "Speedhack" );
			Hint( "O servidor simula o movimento. Nao e possivel no cliente." );
			break;
		}
		break;

	case CAT_LEGIT:
		switch( g_sub[CAT_LEGIT] )
		{
		case 0: TabAimbotLegit(); break;
		case 1: TabTrigger(); break;
		case 2: TabPerfectSilent(); break;
		case 3: TabRecoil(); break;
		default: TabKnifebot(); break;
		}
		break;

	case CAT_VISUALS:
		switch( g_sub[CAT_VISUALS] )
		{
		case 0: TabPlayer(); break;
		case 1: TabLocal(); break;
		case 2: TabWorld(); break;
		case 3: TabRadar(); break;
		case 4: TabSkins(); break;
		case 5: TabRemovals(); break;
		case 6: TabSound(); break;
		default: TabMenu(); break;
		}
		break;

	default:
		switch( g_sub[CAT_MISC] )
		{
		case 0: TabMovement(); break;
		case 1: TabServers(); break;
		case 2: TabKeybinds(); break;
		case 3: TabPlayerList(); break;
		case 4: TabConfig(); break;
		default: TabAntiAfk(); break;
		}
		break;
	}
}

/* ------------------------------------------------------------------ */
/* janela                                                             */
/* ------------------------------------------------------------------ */

static void DrawHub( void * )
{
	if( !g_open )
		return;

	ImGui::SetNextWindowPos( ImVec2( 40, 40 ), ImGuiCond_Once );
	ImGui::SetNextWindowSize( ImVec2( UI::kWinW, UI::kWinH ), ImGuiCond_Always );

	UI::Apply();

	{
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		if( !ImGui::Begin( "##hub", NULL, flags ) )
		{
			ImGui::End();
			UI::Pop();
			return;
		}
	}

	{
		ImDrawList *dl = ImGui::GetWindowDrawList();
		const ImVec2 p = ImGui::GetWindowPos();
		const float bodyY = p.y + UI::kHeaderH;
		const float footY = p.y + UI::kWinH - UI::kFooterH;
		int i;

		/* fundo: header, corpo e rodape */
		dl->AddRectFilled( ImVec2( p.x, bodyY ), ImVec2( p.x + UI::kWinW, p.y + UI::kWinH ),
			ImColor( UI::kBody[0], UI::kBody[1], UI::kBody[2], 1.f ), 6.f );
		dl->AddRectFilled( ImVec2( p.x, p.y ), ImVec2( p.x + UI::kWinW, bodyY ),
			ImColor( UI::kHeader[0], UI::kHeader[1], UI::kHeader[2], 1.f ), 6.f,
			ImDrawFlags_RoundCornersTop );
		dl->AddRectFilled( ImVec2( p.x, footY ), ImVec2( p.x + UI::kWinW, p.y + UI::kWinH ),
			ImColor( UI::kFooter[0], UI::kFooter[1], UI::kFooter[2], 1.f ), 6.f,
			ImDrawFlags_RoundCornersBottom );

		/* logo e titulo */
		ImGui::SetCursorPos( ImVec2( 20, 16 ) );
		ImGui::TextColored( ImVec4( UI::kAccent[0], UI::kAccent[1], UI::kAccent[2], 1.f ), "S" );
		ImGui::SameLine();
		ImGui::SetCursorPosX( 50 );
		ImGui::TextUnformatted( "Sakura" );

		/* categorias no header */
		ImGui::SetCursorPos( ImVec2( 200, 12 ) );
		for( i = 0; i < CAT_COUNT; i++ )
		{
			if( i > 0 )
				ImGui::SameLine();

			if( g_cat == i )
				ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( UI::kAccent[0], UI::kAccent[1], UI::kAccent[2], 1.f ) );

			if( ImGui::Button( kCatName[i], ImVec2( 110, 26 ) ) )
				g_cat = i;

			if( g_cat == i )
				ImGui::PopStyleColor();
		}

		/* sub-tabs a esquerda */
		ImGui::SetCursorPos( ImVec2( 10, UI::kContentY ) );
		for( i = 0; kSubTabs[g_cat][i]; i++ )
		{
			if( UI::SubTab( kSubTabs[g_cat][i], g_sub[g_cat] == i ) )
				g_sub[g_cat] = i;
		}

		/* conteudo */
		ImGui::SetCursorPos( ImVec2( UI::kContentX, UI::kContentY ) );
		ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4( UI::kBody[0] + 0.02f, UI::kBody[1] + 0.02f,
			UI::kBody[2] + 0.02f, 0.85f ) );
		if( ImGui::BeginChild( "##hub_body", ImVec2( UI::kContentW, UI::kContentH ), false ) )
		{
			ImGui::SetCursorPos( ImVec2( 12, 10 ) );
			ImGui::BeginGroup();
			DrawTabBody();
			ImGui::EndGroup();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();

		/* rodape */
		dl->AddText( ImVec2( p.x + 10, footY + 4 ), ImColor( 1.f, 1.f, 1.f, 0.75f ),
			"F6 fecha   |   ESC fecha   |   setas navegam" );

		/* borda */
		dl->AddRect( ImVec2( p.x, p.y ), ImVec2( p.x + UI::kWinW, p.y + UI::kWinH ),
			ImColor( UI::kAccent[0], UI::kAccent[1], UI::kAccent[2], 1.f ), 6.f );
	}

	ImGui::End();
	UI::Pop();
}

/* ------------------------------------------------------------------ */
/* ciclo de vida                                                      */
/* ------------------------------------------------------------------ */

void Hub_Open( int open )
{
	g_open = open ? 1 : 0;

	if( g_open )
	{
		ImGui_SetOpen( 1 );
		ImGui_SetMenuOpen( 1 );
	}
	else
	{
		ImGui_SetMenuOpen( 0 );
		ImGui_SetOpen( 0 );
	}
}

int Hub_IsOpen( void )
{
	return g_open;
}

void Hub_ShowTab( int cat, int sub )
{
	if( cat < 0 || cat >= CAT_COUNT )
		return;
	if( sub < 0 )
		return;

	g_cat = cat;
	g_sub[cat] = sub;
}

static void Cmd_HubMenu( void )
{
	Hub_Open( !g_open );
}

/* hub_selftest: confere que toda cvar do manifesto existe de verdade no
   runtime. Roda no console e imprime PASS/FAIL, verificavel no log. */
static void Cmd_HubSelfTest( void )
{
	int i;
	int fail = 0;

	for( i = 0; i < kHubCvarCount; i++ )
	{
		if( !gEngfuncs.pfnGetCvarPointer( kHubCvars[i] ) )
		{
			gEngfuncs.Con_Printf( "HUB SELFTEST FAIL: cvar ausente %s\n", kHubCvars[i] );
			fail++;
		}
	}

	gEngfuncs.Con_Printf( "HUB SELFTEST: total=%d fail=%d %s\n",
		kHubCvarCount, fail, fail ? "FAIL" : "PASS" );
}

/* hub_panels: quantos paineis ImGui estao registrados (diagnostico). */
static void Cmd_HubPanels( void )
{
	gEngfuncs.Con_Printf( "paineis ImGui registrados: %d (hub, menus, scoreboard)\n", ImGui_PanelCount() );
}

int Hub_Key( int down, int keynum )
{
	if( keynum == K_F6 )
	{
		if( down )
			Hub_Open( !g_open );
		return 0;
	}

	if( !g_open )
		return 1;

	if( keynum == K_ESCAPE )
	{
		if( down )
			Hub_Open( 0 );
		return 0;
	}

	if( down && keynum == K_LEFTARROW )
	{
		g_sub[g_cat] = ( g_sub[g_cat] + 15 ) % 16;
		return 0;
	}
	if( down && keynum == K_RIGHTARROW )
	{
		g_sub[g_cat] = ( g_sub[g_cat] + 1 ) % 16;
		return 0;
	}
	if( down && keynum == K_UPARROW )
	{
		g_cat = ( g_cat + CAT_COUNT - 1 ) % CAT_COUNT;
		return 0;
	}
	if( down && keynum == K_DOWNARROW )
	{
		g_cat = ( g_cat + 1 ) % CAT_COUNT;
		return 0;
	}

	return 1;
}

void Hub_Init( void )
{
	int i;

	UI::Init();

	/* a tecla pode cair em qualquer sub-tab; limita ao numero real da categoria */
	for( i = 0; i < CAT_COUNT; i++ )
		g_sub[i] = 0;

	g_sub[CAT_LEGIT] = 0;

	ImGui_RegisterPanel( "hub", DrawHub, NULL );
	gEngfuncs.pfnAddCommand( "hub_menu", Cmd_HubMenu );
	gEngfuncs.pfnAddCommand( "hub_selftest", Cmd_HubSelfTest );
	gEngfuncs.pfnAddCommand( "hub_panels", Cmd_HubPanels );

	/* abre o HUB no inicio quando cl_hub_open 1 (util para teste e cfg) */
	{
		cvar_t *open = gEngfuncs.pfnRegisterVariable( "cl_hub_open", "0", 0 );

		if( open && open->value > 0.5f )
			Hub_Open( 1 );
	}
}

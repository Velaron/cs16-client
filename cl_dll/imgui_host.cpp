#include "imgui.h"
#include "imgui_impl_opengl2.h"

#include <GL/gl.h>

#include "hud.h"
#include "cl_util.h"
#include "keydefs.h"
#include "imgui_host.h"
#include "imgui_menu.h"
#include "imgui_scoreboard.h"

extern int g_iVisibleMouse;

namespace {

struct Panel
{
	int id;
	int alive;
	const char *title;
	ImGuiPanelFn fn;
	void *user;
};

const int kMaxPanels = 32;

Panel g_panels[kMaxPanels];
int g_next_id = 1;
int g_open = 0;
int g_menu_open = 0;
int g_ready = 0;
int g_gl = 0;
float g_last_time = 0.0f;
bool g_shift = false;
bool g_ctrl = false;
bool g_alt = false;

void SyncCursorCvar( void )
{
	int active = g_open || g_menu_open;
	if( !gEngfuncs.Cvar_SetValue )
		return;
	gEngfuncs.Cvar_SetValue( "cl_imgui_mouse", active ? 1.0f : 0.0f );
	g_iVisibleMouse = active ? 1 : 0;
}

int DisplayWidth( void )
{
	float w = gEngfuncs.pfnGetCvarFloat ? gEngfuncs.pfnGetCvarFloat( "width" ) : 0.0f;
	if( w < 64.0f )
		w = (float)ScreenWidth;
	return (int)w;
}

int DisplayHeight( void )
{
	float h = gEngfuncs.pfnGetCvarFloat ? gEngfuncs.pfnGetCvarFloat( "height" ) : 0.0f;
	if( h < 64.0f )
		h = (float)ScreenHeight;
	return (int)h;
}

ImGuiKey MapKey( int keynum )
{
	if( keynum >= 'a' && keynum <= 'z' )
		return (ImGuiKey)( ImGuiKey_A + ( keynum - 'a' ) );
	if( keynum >= 'A' && keynum <= 'Z' )
		return (ImGuiKey)( ImGuiKey_A + ( keynum - 'A' ) );
	if( keynum >= '0' && keynum <= '9' )
		return (ImGuiKey)( ImGuiKey_0 + ( keynum - '0' ) );

	switch( keynum )
	{
	case K_TAB: return ImGuiKey_Tab;
	case K_ENTER: return ImGuiKey_Enter;
	case K_ESCAPE: return ImGuiKey_Escape;
	case K_SPACE: return ImGuiKey_Space;
	case K_BACKSPACE: return ImGuiKey_Backspace;
	case K_UPARROW: return ImGuiKey_UpArrow;
	case K_DOWNARROW: return ImGuiKey_DownArrow;
	case K_LEFTARROW: return ImGuiKey_LeftArrow;
	case K_RIGHTARROW: return ImGuiKey_RightArrow;
	case K_SHIFT: return ImGuiKey_LeftShift;
	case K_CTRL: return ImGuiKey_LeftCtrl;
	case K_ALT: return ImGuiKey_LeftAlt;
	case K_INS: return ImGuiKey_Insert;
	case K_DEL: return ImGuiKey_Delete;
	case K_PGDN: return ImGuiKey_PageDown;
	case K_PGUP: return ImGuiKey_PageUp;
	case K_HOME: return ImGuiKey_Home;
	case K_END: return ImGuiKey_End;
	case K_F1: return ImGuiKey_F1;
	case K_F2: return ImGuiKey_F2;
	case K_F3: return ImGuiKey_F3;
	case K_F4: return ImGuiKey_F4;
	case K_F5: return ImGuiKey_F5;
	case K_F6: return ImGuiKey_F6;
	case K_F7: return ImGuiKey_F7;
	case K_F8: return ImGuiKey_F8;
	case K_F9: return ImGuiKey_F9;
	case K_F10: return ImGuiKey_F10;
	case K_F11: return ImGuiKey_F11;
	case K_F12: return ImGuiKey_F12;
	case '`':
	case '~': return ImGuiKey_GraveAccent;
	case '-': return ImGuiKey_Minus;
	case '=': return ImGuiKey_Equal;
	case '[': return ImGuiKey_LeftBracket;
	case ']': return ImGuiKey_RightBracket;
	case '\\': return ImGuiKey_Backslash;
	case ';': return ImGuiKey_Semicolon;
	case '\'': return ImGuiKey_Apostrophe;
	case ',': return ImGuiKey_Comma;
	case '.': return ImGuiKey_Period;
	case '/': return ImGuiKey_Slash;
	default: return ImGuiKey_None;
	}
}

void FeedKey( int down, int keynum )
{
	ImGuiIO &io = ImGui::GetIO();

	if( keynum == K_MWHEELUP )
	{
		if( down )
			io.AddMouseWheelEvent( 0.0f, 1.0f );
		return;
	}
	if( keynum == K_MWHEELDOWN )
	{
		if( down )
			io.AddMouseWheelEvent( 0.0f, -1.0f );
		return;
	}
	if( keynum >= K_MOUSE1 && keynum <= K_MOUSE5 )
	{
		io.AddMouseButtonEvent( keynum - K_MOUSE1, down != 0 );
		return;
	}

	if( keynum == K_SHIFT )
		g_shift = down != 0;
	else if( keynum == K_CTRL )
		g_ctrl = down != 0;
	else if( keynum == K_ALT )
		g_alt = down != 0;

	io.AddKeyEvent( ImGuiMod_Shift, g_shift );
	io.AddKeyEvent( ImGuiMod_Ctrl, g_ctrl );
	io.AddKeyEvent( ImGuiMod_Alt, g_alt );

	ImGuiKey mapped = MapKey( keynum );
	if( mapped != ImGuiKey_None )
		io.AddKeyEvent( mapped, down != 0 );

	if( !down || keynum < 32 || keynum >= 127 || keynum == '`' || keynum == '~' )
		return;

	unsigned int ch = (unsigned int)keynum;
	if( g_shift && ch >= 'a' && ch <= 'z' )
		ch = ch - 'a' + 'A';
	io.AddInputCharacter( ch );
}

void RestoreEngineGl( void )
{
	if( gRenderAPI.GL_CleanUpTextureUnits )
		gRenderAPI.GL_CleanUpTextureUnits( 0 );
}

bool EnsureContext( void )
{
	if( g_ready )
		return true;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.IniFilename = "cs16_imgui.ini";
	io.ConfigWindowsResizeFromEdges = true;

	ImGui::StyleColorsDark();
	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowRounding = 4.0f;
	style.FrameRounding = 3.0f;
	style.ScrollbarRounding = 3.0f;

	if( !ImGui_ImplOpenGL2_Init() )
	{
		ImGui::DestroyContext();
		gEngfuncs.Con_Printf( "ImGui: falha ao iniciar o backend OpenGL.\n" );
		return false;
	}

	g_ready = 1;
	g_gl = 1;
	gEngfuncs.Con_Printf( "ImGui nativo pronto. Abra com F6 (HUB).\n" );
	return true;
}

} // namespace

int ImGui_PanelCount( void )
{
	int n = 0;
	for( int i = 0; i < kMaxPanels; ++i )
		if( g_panels[i].alive )
			++n;
	return n;
}

int ImGui_RegisterPanel( const char *title, ImGuiPanelFn fn, void *user )
{
	if( !fn )
		return 0;

	for( int i = 0; i < kMaxPanels; ++i )
	{
		if( g_panels[i].alive )
			continue;
		g_panels[i].id = g_next_id++;
		g_panels[i].alive = 1;
		g_panels[i].title = title ? title : "";
		g_panels[i].fn = fn;
		g_panels[i].user = user;
		return g_panels[i].id;
	}

	gEngfuncs.Con_Printf( "ImGui: limite de %d paineis.\n", kMaxPanels );
	return 0;
}

void ImGui_UnregisterPanel( int id )
{
	if( id <= 0 )
		return;
	for( int i = 0; i < kMaxPanels; ++i )
	{
		if( g_panels[i].alive && g_panels[i].id == id )
			g_panels[i].alive = 0;
	}
}

void ImGui_SetOpen( int open )
{
	int next = open ? 1 : 0;

	g_open = next;
	if( !g_open )
		g_shift = g_ctrl = g_alt = false;
	SyncCursorCvar();
}

int ImGui_IsOpen( void )
{
	return g_open || g_menu_open;
}

void ImGui_SetMenuOpen( int open )
{
	g_menu_open = open ? 1 : 0;
	if( !g_menu_open && !g_open )
		g_shift = g_ctrl = g_alt = false;
	SyncCursorCvar();
}

void ImGui_Init( void )
{
	/* sem comando de console: a interface so abre pelos paineis (HUB/F6) */
	EnsureContext();
	SyncCursorCvar();
}

void ImGui_Shutdown( void )
{
	ImGui_SetOpen( 0 );
	if( !g_ready )
		return;
	if( g_gl )
		ImGui_ImplOpenGL2_Shutdown();
	ImGui::DestroyContext();
	g_ready = 0;
	g_gl = 0;
}

void ImGui_VidInit( void )
{
	if( g_ready && g_gl )
		ImGui_ImplOpenGL2_DestroyDeviceObjects();
}

void ImGui_Frame( void )
{
	if( !g_open && !g_menu_open && !ImGuiScore_Active() )
	{
		SyncCursorCvar();
		return;
	}
	if( !EnsureContext() )
		return;

	const GLubyte *gl_version = glGetString( GL_VERSION );
	if( !gl_version )
		return;

	int w = DisplayWidth();
	int h = DisplayHeight();
	if( w < 1 || h < 1 )
		return;

	ImGuiIO &io = ImGui::GetIO();
	io.DisplaySize = ImVec2( (float)w, (float)h );
	io.DisplayFramebufferScale = ImVec2( 1.0f, 1.0f );

	float now = gEngfuncs.GetClientTime();
	float dt = now - g_last_time;
	g_last_time = now;
	if( dt <= 0.0f || dt > 0.25f )
		dt = 1.0f / 60.0f;
	io.DeltaTime = dt;

	float scale = 1.0f;
	if( g_open )
	{
		scale = h / 720.0f;
		if( scale < 1.0f )
			scale = 1.0f;
		if( scale > 2.0f )
			scale = 2.0f;
	}
	ImGui::GetStyle().FontScaleMain = scale;

	int mx = 0;
	int my = 0;
	gEngfuncs.GetMousePosition( &mx, &my );
	io.AddMousePosEvent( (float)mx, (float)my );
	// A seta visivel e a do motor (evdev), a mesma da interface nativa.
	io.MouseDrawCursor = false;

	ImGui_ImplOpenGL2_NewFrame();
	ImGui::NewFrame();

	for( int i = 0; i < kMaxPanels; ++i )
	{
		if( g_panels[i].alive && g_panels[i].fn )
			g_panels[i].fn( g_panels[i].user );
	}

	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData( ImGui::GetDrawData() );
	RestoreEngineGl();
	SyncCursorCvar();
}

int ImGui_KeyEvent( int down, int keynum )
{
	if( keynum == '`' || keynum == '~' )
		return 1;

	if( keynum == K_INS )
	{
		if( down )
			ImGui_SetOpen( !g_open );
		return 0;
	}

	if( !g_open && !g_menu_open )
		return 1;

	if( !g_ready && !EnsureContext() )
		return 1;

	FeedKey( down, keynum );

	if( keynum == K_ESCAPE )
	{
		if( down )
		{
			if( ImGuiMenu_IsOpen() )
				ImGuiMenu_OnKey( down, keynum );
			else
				ImGui_SetOpen( 0 );
		}
		return 0;
	}

	if( ImGuiMenu_IsOpen() && ImGuiMenu_OnKey( down, keynum ) )
		return 0;

	if( keynum == K_MWHEELUP || keynum == K_MWHEELDOWN || ( keynum >= K_MOUSE1 && keynum <= K_MOUSE5 ) )
		return 0;

	if( ImGui::GetIO().WantCaptureKeyboard )
		return 0;

	return 1;
}

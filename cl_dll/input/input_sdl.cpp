//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: Client input — botoes, cvars do mouse e stubs do joystick.
//          Sem SDL. O look via mouse e feito pelo motor (evdev).
//
// $NoKeywords: $
//=============================================================================

#include "port.h"
#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "../engine/keydefs.h"
#include "view.h"
#include "input.h"
#include "in_defs.h"
#include "aimbot.h"

#define MOUSE_BUTTON_COUNT 5

int g_iVisibleMouse = 0;
extern int iMouseInUse;

int mouse_buttons;
int mouse_oldbuttonstate;
int mouseinitialized = 0;

#define JOY_ABSOLUTE_AXIS   0x00000000
#define JOY_RELATIVE_AXIS   0x00000010

#define JOY_MAX_AXES 6
#define JOY_AXIS_X 0
#define JOY_AXIS_Y 1
#define JOY_AXIS_Z 2
#define JOY_AXIS_R 3
#define JOY_AXIS_U 4
#define JOY_AXIS_V 5

enum _ControlList
{
	AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn
};

DWORD dwAxisMap[JOY_MAX_AXES];
DWORD dwControlMap[JOY_MAX_AXES];
int pdwRawValue[JOY_MAX_AXES];
DWORD joy_oldbuttonstate, joy_oldpovstate;
int joy_id;
DWORD joy_numbuttons;
int joy_avail = 0, joy_advancedinit = 0, joy_haspov = 0;

// cvars do mouse-look
cvar_t *sensitivity;
cvar_t *m_filter;

// cvars do joystick
extern cvar_t *in_joystick;
cvar_t *joy_name;
cvar_t *joy_advanced;
cvar_t *joy_advaxisx;
cvar_t *joy_advaxisy;
cvar_t *joy_advaxisz;
cvar_t *joy_advaxisr;
cvar_t *joy_advaxisu;
cvar_t *joy_advaxisv;
cvar_t *joy_forwardthreshold;
cvar_t *joy_sidethreshold;
cvar_t *joy_pitchthreshold;
cvar_t *joy_yawthreshold;
cvar_t *joy_forwardsensitivity;
cvar_t *joy_sidesensitivity;
cvar_t *joy_pitchsensitivity;
cvar_t *joy_yawsensitivity;
cvar_t *joy_wwhack1;
cvar_t *joy_wwhack2;

void Force_CenterView_f (void)
{
	vec3_t viewangles;

	if (!iMouseInUse)
	{
		gEngfuncs.GetViewAngles( (float *)viewangles );
		viewangles[PITCH] = 0;
		gEngfuncs.SetViewAngles( (float *)viewangles );
	}
}

void DLLEXPORT IN_ActivateMouse (void)
{
}

void DLLEXPORT IN_DeactivateMouse (void)
{
}

void IN_StartupMouse (void)
{
	if ( gEngfuncs.CheckParm ("-nomouse", NULL ) )
		return;

	mouseinitialized = 1;
	mouse_buttons = MOUSE_BUTTON_COUNT;
}

void IN_Shutdown (void)
{
	IN_DeactivateMouse();
}

void IN_GetMousePos( int *mx, int *my )
{
	gEngfuncs.GetMousePosition( mx, my );
}

void IN_ResetMouse( void )
{
}

void DLLEXPORT IN_MouseEvent (int mstate)
{
	int i;

	if ( iMouseInUse || g_iVisibleMouse )
		return;

	for (i=0 ; i<mouse_buttons ; i++)
	{
		if ( (mstate & (1<<i)) &&
			!(mouse_oldbuttonstate & (1<<i)) )
		{
			gEngfuncs.Key_Event (K_MOUSE1 + i, 1);
		}

		if ( !(mstate & (1<<i)) &&
			(mouse_oldbuttonstate & (1<<i)) )
		{
			gEngfuncs.Key_Event (K_MOUSE1 + i, 0);
		}
	}

	mouse_oldbuttonstate = mstate;
}

void IN_MouseMove ( float frametime, usercmd_t *cmd)
{
	static cvar_t *evdev_dx;
	static cvar_t *evdev_dy;
	static float old_mouse_x, old_mouse_y;
	vec3_t viewangles;
	float mx, my, mouse_x, mouse_y, sens;

	(void)frametime;

	if( !mouseinitialized )
	{
		Aimbot_NoteMouse( 0.f, 0.f );
		return;
	}

	// Third-person camera reads the same sample later in the frame.
	if( iMouseInUse )
	{
		Aimbot_NoteMouse( 0.f, 0.f );
		return;
	}

	if( !evdev_dx )
		evdev_dx = gEngfuncs.pfnGetCvarPointer( "evdev_dx" );
	if( !evdev_dy )
		evdev_dy = gEngfuncs.pfnGetCvarPointer( "evdev_dy" );
	if( !evdev_dx || !evdev_dy || !m_yaw || !m_pitch )
	{
		Aimbot_NoteMouse( 0.f, 0.f );
		return;
	}

	mx = evdev_dx->value;
	my = evdev_dy->value;
	evdev_dx->value = 0;
	evdev_dy->value = 0;

	if( g_iVisibleMouse || gHUD.m_iIntermission )
	{
		Aimbot_NoteMouse( 0.f, 0.f );
		return;
	}

	if( m_filter && m_filter->value )
	{
		mouse_x = ( mx + old_mouse_x ) * 0.5f;
		mouse_y = ( my + old_mouse_y ) * 0.5f;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	if( mouse_x == 0.0f && mouse_y == 0.0f )
	{
		Aimbot_NoteMouse( 0.f, 0.f );
		return;
	}

	sens = gHUD.GetSensitivity();
	if( sens == 0.0f && sensitivity )
		sens = sensitivity->value;
	if( sens == 0.0f )
		sens = 1.0f;

	mouse_x *= sens;
	mouse_y *= sens;

	gEngfuncs.GetViewAngles( (float *)viewangles );
	{
		float pitch_before = viewangles[PITCH];
		float yaw_before = viewangles[YAW];

	if( (in_strafe.state & 1) || (lookstrafe && lookstrafe->value) )
	{
		if( m_side )
			cmd->sidemove += m_side->value * mouse_x;
	}
	else
		viewangles[YAW] -= m_yaw->value * mouse_x;

	if( in_strafe.state & 1 )
	{
		if( m_forward )
		{
			if( gEngfuncs.IsNoClipping() )
				cmd->upmove -= m_forward->value * mouse_y;
			else
				cmd->forwardmove -= m_forward->value * mouse_y;
		}
	}
	else
	{
		viewangles[PITCH] += m_pitch->value * mouse_y;
		if( cl_pitchdown && viewangles[PITCH] > cl_pitchdown->value )
			viewangles[PITCH] = cl_pitchdown->value;
		if( cl_pitchup && viewangles[PITCH] < -cl_pitchup->value )
			viewangles[PITCH] = -cl_pitchup->value;
	}

	gEngfuncs.SetViewAngles( (float *)viewangles );
	Aimbot_NoteMouse( viewangles[PITCH] - pitch_before, viewangles[YAW] - yaw_before );
	}
}

void DLLEXPORT IN_Accumulate (void)
{
}

void DLLEXPORT IN_ClearStates (void)
{
	mouse_oldbuttonstate = 0;
}

/* ===========
IN_Move — chamado por CL_CreateMove() (input.cpp) a cada frame
=========== */
void IN_Move ( float frametime, usercmd_t *cmd )
{
	IN_MouseMove( frametime, cmd );
}

void IN_StartupJoystick (void)
{
	if ( gEngfuncs.CheckParm ("-nojoy", NULL ) )
		return;

	joy_avail = 0;
}

int RawValuePointer (int axis)
{
	(void)axis;
	return 0;
}

void Joy_AdvancedUpdate_f (void)
{
	int i;
	DWORD dwTemp;

	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer(i);
	}

	if( joy_advanced->value == 0.0)
	{
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
	}
	else
	{
		if ( strcmp ( joy_name->string, "joystick") != 0 )
		{
			gEngfuncs.Con_Printf ("\n%s configured\n\n", joy_name->string);
		}

		dwTemp = (DWORD) joy_advaxisx->value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy->value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz->value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr->value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu->value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv->value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}
}

void IN_Commands (void)
{
	// Joystick gerenciado pelo motor.
}

void IN_Init( void )
{
	sensitivity = gEngfuncs.pfnRegisterVariable ( "sensitivity", "3", FCVAR_ARCHIVE );
	m_filter    = gEngfuncs.pfnRegisterVariable ( "m_filter",    "0", FCVAR_ARCHIVE );

	joy_name = gEngfuncs.pfnRegisterVariable ( "joyname", "joystick", 0 );
	joy_advanced = gEngfuncs.pfnRegisterVariable ( "joyadvanced", "0", 0 );
	joy_advaxisx = gEngfuncs.pfnRegisterVariable ( "joyadvaxisx", "0", 0 );
	joy_advaxisy = gEngfuncs.pfnRegisterVariable ( "joyadvaxisy", "0", 0 );
	joy_advaxisz = gEngfuncs.pfnRegisterVariable ( "joyadvaxisz", "0", 0 );
	joy_advaxisr = gEngfuncs.pfnRegisterVariable ( "joyadvaxisr", "0", 0 );
	joy_advaxisu = gEngfuncs.pfnRegisterVariable ( "joyadvaxisu", "0", 0 );
	joy_advaxisv = gEngfuncs.pfnRegisterVariable ( "joyadvaxisv", "0", 0 );
	joy_forwardthreshold = gEngfuncs.pfnRegisterVariable ( "joyforwardthreshold", "0.15", 0 );
	joy_sidethreshold = gEngfuncs.pfnRegisterVariable ( "joysidethreshold", "0.15", 0 );
	joy_pitchthreshold = gEngfuncs.pfnRegisterVariable ( "joypitchthreshold", "0.15", 0 );
	joy_yawthreshold = gEngfuncs.pfnRegisterVariable ( "joyyawthreshold", "0.15", 0 );
	joy_forwardsensitivity = gEngfuncs.pfnRegisterVariable ( "joyforwardsensitivity", "-1.0", 0 );
	joy_sidesensitivity = gEngfuncs.pfnRegisterVariable ( "joysidesensitivity", "-1.0", 0 );
	joy_pitchsensitivity = gEngfuncs.pfnRegisterVariable ( "joypitchsensitivity", "1.0", 0 );
	joy_yawsensitivity = gEngfuncs.pfnRegisterVariable ( "joyyawsensitivity", "-1.0", 0 );
	joy_wwhack1 = gEngfuncs.pfnRegisterVariable ( "joywwhack1", "0.0", 0 );
	joy_wwhack2 = gEngfuncs.pfnRegisterVariable ( "joywwhack2", "0.0", 0 );

	IN_StartupMouse();
	IN_StartupJoystick();
}
#define _USE_MATH_DEFINES // for M_PI (used by DEG2RAD/RAD2DEG on MSVC)
#include "hud.h"
#include "usercmd.h"
#include "cvardef.h"
#include "kbutton.h"
#include "keydefs.h"
#include "input.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "const.h"
#include "pmtrace.h"
#include "event_api.h"
#include "pm_defs.h"
#include <math.h>

#define	PITCH	0
#define	YAW		1
#define	ROLL	2 

cvar_t	*cl_laddermode;
cvar_t	*sensitivity;
cvar_t	*in_joystick;
cvar_t	*evdev_grab;

// Aim assist (gamepad soft-lock, Max Payne 3 style)
bool g_bAimAssistKey = false;		// hold state of the dedicated aim button (shared)
cvar_t	*aim_assist;			// master on/off
cvar_t	*aim_assist_fov;		// (legacy) tight cone, kept for compatibility
cvar_t	*aim_assist_lock_fov;		// acquisition cone half-angle while the button is held (degrees)
cvar_t	*aim_assist_pull;		// magnetism strength 0..1
cvar_t	*aim_assist_slow;		// sticky slowdown factor applied to stick input
cvar_t	*aim_assist_range;		// max target distance (units)
cvar_t	*aim_assist_wallcheck;		// require line of sight to the target
cvar_t	*aim_assist_debug;		// debug overlay (text + head marker)
cvar_t	*aim_assist_highlight;		// glow shell over the target model

// Shared state for the debug overlay (cl_dll/hud/aimassist.cpp) and highlight (entity.cpp)
int   g_iAimAssistTarget = 0;		// entity index of the chosen target (0 = none)
bool  g_bAimAssistApplying = false;	// assist actively steering (key held + target)
float g_flAimAssistDist = 0.0f;		// distance to the chosen target
float g_flAimAssistAngle = 0.0f;	// angular separation (deg) to the chosen target
int   g_iAimAssistNearestIdx = 0;	// nearest visible enemy by angle, ignoring the cone
float g_flAimAssistNearestAngle = 0.0f;	// its angle (diagnose a too-tight cone)

// view basis used by the assist, stored each frame for the debug cone visualization
vec3_t g_vecAimEye   = { 0, 0, 0 };
vec3_t g_vecAimFwd   = { 0, 0, 0 };
vec3_t g_vecAimRight = { 0, 0, 0 };
vec3_t g_vecAimUp    = { 0, 0, 0 };


float ac_forwardmove;
float ac_sidemove;
int ac_movecount;
float rel_yaw;
float rel_pitch;
bool bMouseInUse = false;

extern Vector dead_viewangles;
extern bool evdev_open;
extern vec3_t v_origin; // actual camera/eye origin, set each frame in view.cpp

#define F 1U<<0	// Forward
#define B 1U<<1	// Back
#define L 1U<<2	// Left
#define R 1U<<3	// Right
#define T 1U<<4	// Forward stop
#define S 1U<<5	// Side stop

#define BUTTON_DOWN		1
#define IMPULSE_DOWN	2
#define IMPULSE_UP		4

bool CL_IsDead();

void IN_ToggleButtons( float forwardmove, float sidemove )
{
	static unsigned int moveflags = T | S;

	if( forwardmove )
		moveflags &= ~T;
	else
	{
		//if( in_forward.state || in_back.state ) gEngfuncs.Con_Printf("Buttons pressed f%d b%d\n", in_forward.state, in_back.state);
		if( !( moveflags & T ) )
		{
			//IN_ForwardUp();
			//IN_BackUp();
			//gEngfuncs.Con_Printf("Reset forwardmove state f%d b%d\n", in_forward.state, in_back.state);
			in_forward.state &= ~BUTTON_DOWN;
			in_back.state &= ~BUTTON_DOWN;
			moveflags |= T;
		}
	}
	if( sidemove )
		moveflags &= ~S;
	else
	{
		//gEngfuncs.Con_Printf("l%d r%d\n", in_moveleft.state, in_moveright.state);
		//if( in_moveleft.state || in_moveright.state ) gEngfuncs.Con_Printf("Buttons pressed l%d r%d\n", in_moveleft.state, in_moveright.state);
		if( !( moveflags & S ) )
		{
			//IN_MoverightUp();
			//IN_MoveleftUp();
			//gEngfuncs.Con_Printf("Reset sidemove state f%d b%d\n", in_moveleft.state, in_moveright.state);
			in_moveleft.state &= ~BUTTON_DOWN;
			in_moveright.state &= ~BUTTON_DOWN;
			moveflags |= S;
		}
	}

	if ( forwardmove > 0.7 && !( moveflags & F ))
	{
		moveflags |= F;
		in_forward.state |= BUTTON_DOWN;
	}
	if ( forwardmove < 0.7 && ( moveflags & F ))
	{
		moveflags &= ~F;
		in_forward.state &= ~BUTTON_DOWN;
	}
	if ( forwardmove < -0.7 && !( moveflags & B ))
	{
		moveflags |= B;
		in_back.state |= BUTTON_DOWN;
	}
	if ( forwardmove > -0.7 && ( moveflags & B ))
	{
		moveflags &= ~B;
		in_back.state &= ~BUTTON_DOWN;
	}
	if ( sidemove > 0.9 && !( moveflags & R ))
	{
		moveflags |= R;
		in_moveright.state |= BUTTON_DOWN;
	}
	if ( sidemove < 0.9 && ( moveflags & R ))
	{
		moveflags &= ~R;
		in_moveright.state &= ~BUTTON_DOWN;
	}
	if ( sidemove < -0.9 && !( moveflags & L ))
	{
		moveflags |= L;
		in_moveleft.state |= BUTTON_DOWN;
	}
	if ( sidemove > -0.9 && ( moveflags & L ))
	{
		moveflags &= ~L;
		in_moveleft.state &= ~BUTTON_DOWN;
	}

}

void IN_ClientMoveEvent( float forwardmove, float sidemove )
{
	//gEngfuncs.Con_Printf("IN_MoveEvent\n");

	ac_forwardmove += forwardmove;
	ac_sidemove += sidemove;
	ac_movecount++;
}

void IN_ClientLookEvent( float relyaw, float relpitch )
{
	rel_yaw += relyaw;
	rel_pitch += relpitch;
}

// Console commands bound through the Controls menu (+aimassist / -aimassist)
void IN_AimAssistDown( void ) { g_bAimAssistKey = true;  }
void IN_AimAssistUp( void )   { g_bAimAssistKey = false; }

// Returns true if no world geometry blocks the line from start to end.
// We trace against the world ONLY (PM_WORLD_ONLY): tracing against solid players
// would stop the ray at the very enemy we are checking and report "not visible".
static bool AimAssist_Visible( float *start, float *end )
{
	pmtrace_t tr;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( -1 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( start, end, PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
	gEngfuncs.pEventAPI->EV_PopPMStates();

	return tr.fraction >= 0.95f; // ~1.0 means nothing solid in the world blocks the view
}

// Picks the enemy closest to the crosshair within the assist cone, alive and visible.
static cl_entity_t *AimAssist_FindTarget( float *eye, float *fwd )
{
	cl_entity_t *local = gEngfuncs.GetLocalPlayer();
	int maxc = gEngfuncs.GetMaxClients();
	int me = local ? local->index : 0;
	float fov = aim_assist_lock_fov->value; // wide acquisition cone (grab nearest target in front)
	float maxRange = aim_assist_range->value;
	float bestAngle = 9999.0f;
	float nearestAngle = 9999.0f;
	cl_entity_t *best = NULL;

	g_iAimAssistNearestIdx = 0;
	g_flAimAssistNearestAngle = 0.0f;

	for( int i = 1; i <= maxc; i++ )
	{
		if( i == me )
			continue;

		cl_entity_t *e = gEngfuncs.GetEntityByIndex( i );
		if( !e || !e->player )
			continue;
		if( e->curstate.solid == SOLID_NOT || g_PlayerExtraInfo[i].dead )
			continue; // dead / non-solid
		if( g_iTeamNumber != 0 && g_PlayerExtraInfo[i].teamnumber == g_iTeamNumber )
			continue; // teammate (skip filter in FFA where team is 0)

		vec3_t dir;
		VectorSubtract( e->curstate.origin, eye, dir );
		float dist = sqrt( dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2] );
		if( dist < 1.0f || dist > maxRange )
			continue;

		float inv = 1.0f / dist;
		dir[0] *= inv; dir[1] *= inv; dir[2] *= inv;

		float d = DotProduct( dir, fwd );
		if( d > 1.0f ) d = 1.0f;
		float angle = RAD2DEG( acos( d ) );

		// diagnostics: nearest enemy by angle, ignoring the cone and LOS
		if( angle < nearestAngle )
		{
			nearestAngle = angle;
			g_iAimAssistNearestIdx = i;
			g_flAimAssistNearestAngle = angle;
		}

		// distance-adjusted cone: a closer enemy subtends a larger angle, so the
		// assist window grows when you're near them (fixes "target none" up close).
		float effFov = fov + RAD2DEG( atan2( 32.0f, dist ) );
		if( angle > effFov )
			continue; // crosshair not on the enemy
		if( angle >= bestAngle )
			continue; // keep the one closest to the crosshair

		if( aim_assist_wallcheck->value )
		{
			vec3_t tgt;
			VectorCopy( e->curstate.origin, tgt );
			if( !AimAssist_Visible( eye, tgt ) )
				continue;
		}

		bestAngle = angle;
		best = e;
	}

	return best;
}

// Rotate camera and add move values to usercmd
void IN_Move( float frametime, usercmd_t *cmd )
{
	Vector viewangles;
	bool bLadder = false;

	if( gHUD.m_iIntermission )
		return; // we can't move during intermission


	if( cl_laddermode->value != 2 )
	{
		cl_entity_t *pplayer = gEngfuncs.GetLocalPlayer();
		if( pplayer )
			bLadder = pplayer->curstate.movetype == MOVETYPE_FLY;
	}
	//if(ac_forwardmove || ac_sidemove)
	//gEngfuncs.Con_Printf("Move: %f %f %f %f\n", ac_forwardmove, ac_sidemove, rel_pitch, rel_yaw);
#if 0
	if( in_mlook.state & 1 )
	{
		V_StopPitchDrift();
	}
#endif

	if( CL_IsDead( ) )
	{
		viewangles = dead_viewangles; // HACKHACK: see below
	}
	else
	{
		gEngfuncs.GetViewAngles( viewangles );
	}

	if( gHUD.GetSensitivity() != 0 )
	{
		rel_yaw *= gHUD.GetSensitivity();
		rel_pitch *= gHUD.GetSensitivity();
	}
	else
	{
		rel_yaw *= sensitivity->value;
		rel_pitch *= sensitivity->value;
	}

	// --- Aim assist: find target (for steering and/or the debug overlay) ---
	cl_entity_t *aaTarget = NULL;
	vec3_t aaDesired = { 0, 0, 0 };
	g_iAimAssistTarget = 0;
	g_bAimAssistApplying = false;
	g_flAimAssistDist = g_flAimAssistAngle = 0.0f;

	// store the view basis every frame so the debug cone can be drawn (even while dead)
	VectorCopy( v_origin, g_vecAimEye );
	AngleVectors( viewangles, g_vecAimFwd, g_vecAimRight, g_vecAimUp );

	// scan when steering (key held) OR when a debug/highlight view wants the target
	bool aaScan = aim_assist->value && !CL_IsDead()
		&& ( g_bAimAssistKey || aim_assist_debug->value || aim_assist_highlight->value )
		&& !( gHUD.m_MOTD.cl_hide_motd->value == 0.0f && gHUD.m_MOTD.m_bShow );
	if( aaScan )
	{
		cl_entity_t *local = gEngfuncs.GetLocalPlayer();
		if( local )
		{
			aaTarget = AimAssist_FindTarget( g_vecAimEye, g_vecAimFwd );
			if( aaTarget )
			{
				vec3_t dir;
				VectorSubtract( aaTarget->curstate.origin, g_vecAimEye, dir );
				float dist = sqrt( dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2] );
				float hyp = sqrt( dir[0] * dir[0] + dir[1] * dir[1] );
				aaDesired[YAW]   = RAD2DEG( atan2( dir[1], dir[0] ) );
				aaDesired[PITCH] = -RAD2DEG( atan2( dir[2], hyp ) ); // GoldSrc: positive pitch looks down

				g_iAimAssistTarget = aaTarget->index;
				g_flAimAssistDist  = dist;
				if( dist > 0.0f )
				{
					float dot = DotProduct( dir, g_vecAimFwd ) / dist;
					g_flAimAssistAngle = RAD2DEG( acos( dot > 1.0f ? 1.0f : dot ) );
				}

				// sticky: dampen manual rotation, but only while actually steering (key held)
				if( g_bAimAssistKey )
				{
					g_bAimAssistApplying = true;
					rel_yaw   *= aim_assist_slow->value;
					rel_pitch *= aim_assist_slow->value;
				}
			}
		}
	}

	if(gHUD.m_MOTD.cl_hide_motd->value == 0.0f && gHUD.m_MOTD.m_bShow)
	{
		gHUD.m_MOTD.scroll += rel_pitch;
	}
	else
	{
		viewangles[PITCH] += rel_pitch;
		viewangles[YAW] += rel_yaw;
		if( bLadder )
		{
			if( ( cl_laddermode->value == 1 ) )
				viewangles[YAW] -= ac_sidemove * 5;
			ac_sidemove = 0;
		}
	}

	// --- Aim assist: magnetism pull toward the target (only while steering) ---
	if( aaTarget && g_bAimAssistKey )
	{
		float dyaw   = aaDesired[YAW]   - viewangles[YAW];
		float dpitch = aaDesired[PITCH] - viewangles[PITCH];
		while( dyaw > 180.0f )  dyaw -= 360.0f; // shortest way around
		while( dyaw < -180.0f ) dyaw += 360.0f;
		float t = aim_assist_pull->value * frametime * 60.0f;
		if( t > 1.0f ) t = 1.0f;
		if( t < 0.0f ) t = 0.0f;
		viewangles[YAW]   += dyaw * t;
		viewangles[PITCH] += dpitch * t;
	}

	if (viewangles[PITCH] > cl_pitchdown->value)
		viewangles[PITCH] = cl_pitchdown->value;
	if (viewangles[PITCH] < -cl_pitchup->value)
		viewangles[PITCH] = -cl_pitchup->value;


	if( !CL_IsDead( ) )
	{
		gEngfuncs.SetViewAngles( viewangles );
	}

	dead_viewangles = viewangles;
	
	if( ac_movecount )
	{
		IN_ToggleButtons( ac_forwardmove / ac_movecount, ac_sidemove / ac_movecount );
		if( ac_forwardmove ) cmd->forwardmove  = ac_forwardmove * cl_forwardspeed->value / ac_movecount;
		if( ac_sidemove ) cmd->sidemove  = ac_sidemove * cl_sidespeed->value / ac_movecount;
		if (in_speed.state & 1)
		{
			cmd->forwardmove *= cl_movespeedkey->value;
			cmd->sidemove *= cl_movespeedkey->value;
		}
	}
	
	ac_sidemove = ac_forwardmove = rel_pitch = rel_yaw = 0;
	ac_movecount = 0;
}

void DLLEXPORT IN_MouseEvent( int mstate )
{
	static int mouse_oldbuttonstate;
	// perform button actions
	for( int i = 0; i < 5; i++ )
	{
		if(( mstate & (1 << i)) && !( mouse_oldbuttonstate & (1 << i)))
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 1 );
		}

		if( !( mstate & (1 << i)) && ( mouse_oldbuttonstate & (1 << i)))
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 0 );
		}
	}	
	
	mouse_oldbuttonstate = mstate;
	bMouseInUse = true;
}

// Stubs

void DLLEXPORT IN_ClearStates ( void )
{
	//gEngfuncs.Con_Printf("IN_ClearStates\n");
}

void  DLLEXPORT IN_ActivateMouse ( void )
{
	//gEngfuncs.Con_Printf("IN_ActivateMouse\n");
}

void DLLEXPORT  IN_DeactivateMouse ( void )
{
	//gEngfuncs.Con_Printf("IN_DeactivateMouse\n");
}

void DLLEXPORT IN_Accumulate ( void )
{
	//gEngfuncs.Con_Printf("IN_Accumulate\n");
}

void IN_Commands ( void )
{
	//gEngfuncs.Con_Printf("IN_Commands\n");
}

void IN_Shutdown ( void )
{
}
// Register cvars and reset data
void IN_Init( void )
{
	sensitivity = gEngfuncs.pfnRegisterVariable ( "sensitivity", "3", FCVAR_ARCHIVE );
	in_joystick = gEngfuncs.pfnRegisterVariable ( "joystick", "0", FCVAR_ARCHIVE );
	cl_laddermode = gEngfuncs.pfnRegisterVariable ( "cl_laddermode", "2", FCVAR_ARCHIVE );
	evdev_grab = gEngfuncs.pfnGetCvarPointer("evdev_grab");

	// Aim assist (bindable via Controls menu as "+aimassist")
	gEngfuncs.pfnAddCommand( "+aimassist", IN_AimAssistDown );
	gEngfuncs.pfnAddCommand( "-aimassist", IN_AimAssistUp );
	aim_assist           = gEngfuncs.pfnRegisterVariable( "aim_assist",           "0",    FCVAR_ARCHIVE );
	aim_assist_fov       = gEngfuncs.pfnRegisterVariable( "aim_assist_fov",       "10",   FCVAR_ARCHIVE );
	aim_assist_lock_fov  = gEngfuncs.pfnRegisterVariable( "aim_assist_lock_fov",  "45",   FCVAR_ARCHIVE );
	aim_assist_pull      = gEngfuncs.pfnRegisterVariable( "aim_assist_pull",      "0.25", FCVAR_ARCHIVE );
	aim_assist_slow      = gEngfuncs.pfnRegisterVariable( "aim_assist_slow",      "0.4",  FCVAR_ARCHIVE );
	aim_assist_range     = gEngfuncs.pfnRegisterVariable( "aim_assist_range",     "1500", FCVAR_ARCHIVE );
	aim_assist_wallcheck = gEngfuncs.pfnRegisterVariable( "aim_assist_wallcheck", "1",    FCVAR_ARCHIVE );
	aim_assist_debug     = gEngfuncs.pfnRegisterVariable( "aim_assist_debug",     "0",    FCVAR_ARCHIVE );
	aim_assist_highlight = gEngfuncs.pfnRegisterVariable( "aim_assist_highlight", "0",    FCVAR_ARCHIVE );

	ac_forwardmove = ac_sidemove = rel_yaw = rel_pitch = 0;
}

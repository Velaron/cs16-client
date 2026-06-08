/*
aimassist.h - debug overlay + target highlight for the gamepad aim assist
*/
#pragma once
#ifndef AIMASSIST_H
#define AIMASSIST_H

// Shared state produced by the aim assist in input_xash3d.cpp and consumed by
// the HUD overlay (this element) and the model highlight (entity.cpp).
extern bool   g_bAimAssistKey;        // dedicated aim button is held
extern int    g_iAimAssistTarget;     // entity index of the chosen target (0 = none)
extern bool   g_bAimAssistApplying;   // assist actively steering (key held + target)
extern float  g_flAimAssistDist;      // distance to the chosen target
extern float  g_flAimAssistAngle;     // angular separation (deg) to the chosen target
extern int    g_iAimAssistNearestIdx; // nearest visible enemy by angle, ignoring the cone
extern float  g_flAimAssistNearestAngle; // its angle (to diagnose a too-tight cone)

extern vec3_t g_vecAimEye;   // camera origin used by the assist (for the debug cone)
extern vec3_t g_vecAimFwd;   // view forward
extern vec3_t g_vecAimRight; // view right
extern vec3_t g_vecAimUp;    // view up

extern cvar_t *aim_assist;            // master on/off (shown in the overlay)
extern cvar_t *aim_assist_debug;      // text overlay + floating head marker
extern cvar_t *aim_assist_highlight;  // glow shell over the target model
extern cvar_t *aim_assist_fov;        // legacy tight cone
extern cvar_t *aim_assist_lock_fov;   // acquisition cone (shown in the overlay / drawn as the ring)

class CHudAimAssist : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
};

#endif // AIMASSIST_H

#pragma once

// Predictive spread/recoil HUD dot for CS16Client/Xash3D.
// Intended for offline/LAN/training use in a client build you control.
//
// V5 notes:
// - Stays client-only: do not include dlls/weapons.h from this module.
// - Keeps the V4 anti-jitter behavior for Deagle/AWP unscoped idle.
// - Fixes the dot staying away from the crosshair after shooting: V5 freezes
//   only the random spread component, not the final HUD screen position.
// - cl_spreaddot_freeze_idle defaults to 1.
// - cl_spreaddot_idle_random defaults to 0: no random spread while idle.

struct local_state_s;
typedef struct local_state_s local_state_t;
struct usercmd_s;
struct client_data_s;
typedef struct client_data_s client_data_t;

void SpreadDot_Capture(local_state_t *from, local_state_t *to, struct usercmd_s *cmd,
                       double time, unsigned int random_seed);
void SpreadDot_UpdateClientData(client_data_t *cdata, float time);
void SpreadDot_Draw(float flTime);
/* Inaccuracy atual em graus (0 quando a arma nao tem spread modelado). */
float SpreadDot_GetSpreadDegrees(void);
/* Offset tangente (radianos) do proximo tiro para a seed do comando. */
int SpreadDot_GetSpreadOffset(unsigned int seed, float *dx, float *dy);

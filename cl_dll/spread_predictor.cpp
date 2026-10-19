/***
 * Predictive spread HUD dot for CS16Client/Xash3D.
 *
 * V5 center-return fix:
 * - keeps the dot cached while idle instead of recomputing a new random spread
 *   offset every rendered frame.
 * - by default, the random part of spread is zeroed while idle; the real
 *   command random_seed is used only on attack edge / shot / recoil updates.
 * - this fixes Deagle/AWP unscoped "random wandering" while still allowing
 *   small shot flicks when a bullet is actually predicted.
 * - kept this module client-only; do not include dlls/weapons.h here.
 *
 * Intended for offline/LAN/training builds you control.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hud.h"
#include "cl_util.h"
#include "entity_state.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "com_weapons.h"
#include "spread_predictor.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef FCVAR_ARCHIVE
#define FCVAR_ARCHIVE 1
#endif

#ifndef FL_ONGROUND
#define FL_ONGROUND (1 << 9)
#endif

#ifndef FL_DUCKING
#define FL_DUCKING (1 << 14)
#endif

#ifndef IN_ATTACK
#define IN_ATTACK (1 << 0)
#endif

// Local weapon ids only. Avoid including dlls/weapons.h in client-only code.
enum SpreadDotWeaponId
{
    SP_WEAPON_P228       = 1,
    SP_WEAPON_SCOUT      = 3,
    SP_WEAPON_XM1014     = 5,
    SP_WEAPON_MAC10      = 7,
    SP_WEAPON_AUG        = 8,
    SP_WEAPON_ELITE      = 10,
    SP_WEAPON_FIVESEVEN  = 11,
    SP_WEAPON_UMP45      = 12,
    SP_WEAPON_SG550      = 13,
    SP_WEAPON_GALIL      = 14,
    SP_WEAPON_FAMAS      = 15,
    SP_WEAPON_USP        = 16,
    SP_WEAPON_GLOCK18    = 17,
    SP_WEAPON_AWP        = 18,
    SP_WEAPON_MP5N       = 19,
    SP_WEAPON_M249       = 20,
    SP_WEAPON_M3         = 21,
    SP_WEAPON_M4A1       = 22,
    SP_WEAPON_TMP        = 23,
    SP_WEAPON_G3SG1      = 24,
    SP_WEAPON_DEAGLE     = 26,
    SP_WEAPON_SG552      = 27,
    SP_WEAPON_AK47       = 28,
    SP_WEAPON_P90        = 30,
    SP_MAX_WEAPONS       = 32
};

struct SpreadDotVec2
{
    float x;
    float y;
};

struct SpreadDotState
{
    int weaponId;
    int weaponState;
    int shotsFired;
    int clip;
    int flags;
    int waterlevel;
    int buttons;
    float time;
    float lastFire;
    float nextAttack;
    float nextPrimaryAttack;
    float fov;
    unsigned int randomSeed;
    Vector velocity;
    Vector punchangle;
    bool valid;
    bool hasCamera;
    bool refreshDot;
    bool useLiveSeed;
    bool hasCachedDot;
    int cachedX;
    int cachedY;
    float cachedSpread;
    unsigned int captureCount;
    unsigned int drawCount;
};

static SpreadDotState g_spreadDot;
static bool g_spreadDotCvarsReady = false;
static cvar_t *cl_spreaddot = NULL;
static cvar_t *cl_spreaddot_mode = NULL;
static cvar_t *cl_spreaddot_size = NULL;
static cvar_t *cl_spreaddot_color = NULL;
static cvar_t *cl_spreaddot_max_px = NULL;
static cvar_t *cl_spreaddot_debug = NULL;
static cvar_t *cl_spreaddot_fallback = NULL;
static cvar_t *cl_spreaddot_freeze_idle = NULL;
static cvar_t *cl_spreaddot_idle_random = NULL;

static cvar_t *SP_RegisterCvar(const char *name, const char *value, int flags)
{
    return gEngfuncs.pfnRegisterVariable((char *)name, (char *)value, flags);
}

static float SP_Clamp(float v, float lo, float hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

static float SP_Abs(float v)
{
    return v < 0.0f ? -v : v;
}

static float SP_DegToRad(float v)
{
    return v * (float)(M_PI / 180.0);
}

static float SP_Vec2DLength(const Vector &v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

static bool SP_OnGround(const SpreadDotState &s)
{
    return (s.flags & FL_ONGROUND) != 0;
}

static bool SP_Ducking(const SpreadDotState &s)
{
    return (s.flags & FL_DUCKING) != 0;
}

static bool SP_IsBulletWeapon(int id)
{
    switch (id)
    {
    case SP_WEAPON_P228:
    case SP_WEAPON_SCOUT:
    case SP_WEAPON_XM1014:
    case SP_WEAPON_MAC10:
    case SP_WEAPON_AUG:
    case SP_WEAPON_ELITE:
    case SP_WEAPON_FIVESEVEN:
    case SP_WEAPON_UMP45:
    case SP_WEAPON_SG550:
    case SP_WEAPON_GALIL:
    case SP_WEAPON_FAMAS:
    case SP_WEAPON_USP:
    case SP_WEAPON_GLOCK18:
    case SP_WEAPON_AWP:
    case SP_WEAPON_MP5N:
    case SP_WEAPON_M249:
    case SP_WEAPON_M3:
    case SP_WEAPON_M4A1:
    case SP_WEAPON_TMP:
    case SP_WEAPON_G3SG1:
    case SP_WEAPON_DEAGLE:
    case SP_WEAPON_SG552:
    case SP_WEAPON_AK47:
    case SP_WEAPON_P90:
        return true;
    default:
        return false;
    }
}

static void SP_RegisterCVars()
{
    if (g_spreadDotCvarsReady)
        return;

    // Default ON in V3, so after rebuilding you immediately see whether the
    // HUD draw hook is working. Set cl_spreaddot 0 in-game to disable.
    cl_spreaddot          = SP_RegisterCvar("cl_spreaddot", "1", FCVAR_ARCHIVE);
    cl_spreaddot_mode     = SP_RegisterCvar("cl_spreaddot_mode", "2", FCVAR_ARCHIVE);
    cl_spreaddot_size     = SP_RegisterCvar("cl_spreaddot_size", "5", FCVAR_ARCHIVE);
    cl_spreaddot_color    = SP_RegisterCvar("cl_spreaddot_color", "0 255 255 230", FCVAR_ARCHIVE);
    cl_spreaddot_max_px   = SP_RegisterCvar("cl_spreaddot_max_px", "9999", FCVAR_ARCHIVE);
    cl_spreaddot_debug       = SP_RegisterCvar("cl_spreaddot_debug", "0", FCVAR_ARCHIVE);
    cl_spreaddot_fallback    = SP_RegisterCvar("cl_spreaddot_fallback", "1", FCVAR_ARCHIVE);
    cl_spreaddot_freeze_idle = SP_RegisterCvar("cl_spreaddot_freeze_idle", "1", FCVAR_ARCHIVE);

    // 0 = no random spread while idle, only recoil/punch. Best visual behavior.
    // 1 = stable deterministic preview while idle.
    // 2 = old behavior: live command random_seed every recompute.
    cl_spreaddot_idle_random = SP_RegisterCvar("cl_spreaddot_idle_random", "0", FCVAR_ARCHIVE);

    g_spreadDotCvarsReady = true;
}

// Same RNG consumption order used by CBaseEntity::FireBullets3 in cs_wpn/cs_weapons.cpp.
static SpreadDotVec2 SP_ComputeSharedSpreadOffset(unsigned int seed, float spread)
{
    SpreadDotVec2 out;
    const float x = UTIL_SharedRandomFloat(seed,     -0.5f, 0.5f) +
                    UTIL_SharedRandomFloat(seed + 1, -0.5f, 0.5f);
    const float y = UTIL_SharedRandomFloat(seed + 2, -0.5f, 0.5f) +
                    UTIL_SharedRandomFloat(seed + 3, -0.5f, 0.5f);
    out.x = x * spread;
    out.y = y * spread;
    return out;
}

static unsigned int SP_MakeStableSeed(const SpreadDotState &s)
{
    // Small deterministic hash for a stable idle preview. It is not used as
    // "truth" for a fired bullet; real random_seed is used for actual fire.
    unsigned int h = 2166136261u;
    h = (h ^ (unsigned int)s.weaponId) * 16777619u;
    h = (h ^ (unsigned int)s.weaponState) * 16777619u;
    h = (h ^ (unsigned int)(s.shotsFired + 31)) * 16777619u;
    h = (h ^ (unsigned int)(s.flags + 131)) * 16777619u;
    h = (h ^ (unsigned int)(s.fov * 10.0f)) * 16777619u;
    return h;
}

static bool SP_PunchChangedEnough(const Vector &a, const Vector &b)
{
    return SP_Abs(a.x - b.x) > 0.010f ||
           SP_Abs(a.y - b.y) > 0.010f ||
           SP_Abs(a.z - b.z) > 0.010f;
}

static float SP_AccuracyByShots(int shots, float div, float base, float maxAcc)
{
    if (shots < 0)
        shots = 0;
    float f = ((float)(shots * shots * shots) / div) + base;
    return SP_Clamp(f, 0.0f, maxAcc);
}

static float SP_TimeAccuracy(const SpreadDotState &s, float start, float recover, float lo, float hi)
{
    float acc = start - (s.time - s.lastFire) * recover;
    return SP_Clamp(acc, lo, hi);
}

static float SP_ComputeWeaponSpread(const SpreadDotState &s, int simulatedShots)
{
    const float speed = SP_Vec2DLength(s.velocity);
    const bool onGround = SP_OnGround(s);
    const bool duck = SP_Ducking(s);

    switch (s.weaponId)
    {
    case SP_WEAPON_AK47:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 200.0f, 0.35f, 1.25f);
        if (!onGround) return 0.0400f * acc;
        if (speed > 140.0f) return 0.0700f * acc;
        if (duck) return 0.0200f * acc;
        return 0.0275f * acc;
    }

    case SP_WEAPON_M4A1:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 220.0f, 0.30f, 1.00f);
        if (!onGround) return 0.0400f * acc;
        if (speed > 140.0f) return 0.0700f * acc;
        if (duck) return 0.0250f * acc;
        return 0.0300f * acc;
    }

    case SP_WEAPON_GALIL:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 200.0f, 0.35f, 1.25f);
        if (!onGround) return 0.0400f * acc;
        if (speed > 140.0f) return 0.0700f * acc;
        if (duck) return 0.0250f * acc;
        return 0.0375f * acc;
    }

    case SP_WEAPON_FAMAS:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 215.0f, 0.30f, 1.00f);
        if (!onGround) return 0.0300f * acc;
        if (speed > 140.0f) return 0.0700f * acc;
        if (duck) return 0.0200f * acc;
        return 0.0300f * acc;
    }

    case SP_WEAPON_AUG:
    case SP_WEAPON_SG552:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 215.0f, 0.30f, 1.00f);
        if (!onGround) return 0.0350f * acc;
        if (speed > 140.0f) return 0.0750f * acc;
        if (duck) return 0.0200f * acc;
        return 0.0300f * acc;
    }

    case SP_WEAPON_M249:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 175.0f, 0.40f, 1.25f);
        if (!onGround) return 0.0950f * acc;
        if (speed > 140.0f) return 0.0950f * acc;
        if (duck) return 0.0300f * acc;
        return 0.0500f * acc;
    }

    case SP_WEAPON_P90:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 175.0f, 0.45f, 1.00f);
        if (!onGround) return 0.30f * acc;
        if (speed > 170.0f) return 0.115f * acc;
        if (duck) return 0.045f * acc;
        return 0.060f * acc;
    }

    case SP_WEAPON_MAC10:
        if (!onGround) return 0.375f;
        return speed > 140.0f ? 0.070f : 0.030f;

    case SP_WEAPON_MP5N:
        if (!onGround) return 0.200f;
        return speed > 140.0f ? 0.070f : 0.040f;

    case SP_WEAPON_TMP:
        if (!onGround) return 0.250f;
        return speed > 140.0f ? 0.070f : 0.030f;

    case SP_WEAPON_UMP45:
        if (!onGround) return 0.240f;
        return speed > 140.0f ? 0.070f : 0.040f;

    case SP_WEAPON_DEAGLE:
    {
        float acc = SP_TimeAccuracy(s, 0.90f, 0.35f, 0.55f, 0.90f);
        if (!onGround) return 1.50f * (1.0f - acc);
        if (speed > 0.0f) return 0.25f * (1.0f - acc);
        if (duck) return 0.115f * (1.0f - acc);
        return 0.130f * (1.0f - acc);
    }

    case SP_WEAPON_USP:
    {
        float acc = SP_TimeAccuracy(s, 0.92f, 0.275f, 0.60f, 0.92f);
        if (!onGround) return 1.30f * (1.0f - acc);
        if (speed > 0.0f) return 0.225f * (1.0f - acc);
        if (duck) return 0.08f * (1.0f - acc);
        return 0.10f * (1.0f - acc);
    }

    case SP_WEAPON_GLOCK18:
    {
        float acc = SP_TimeAccuracy(s, 0.90f, 0.275f, 0.60f, 0.90f);
        if (!onGround) return 1.00f * (1.0f - acc);
        if (speed > 0.0f) return 0.185f * (1.0f - acc);
        if (duck) return 0.075f * (1.0f - acc);
        return 0.10f * (1.0f - acc);
    }

    case SP_WEAPON_P228:
    {
        float acc = SP_TimeAccuracy(s, 0.90f, 0.30f, 0.60f, 0.90f);
        if (!onGround) return 1.50f * (1.0f - acc);
        if (speed > 0.0f) return 0.255f * (1.0f - acc);
        if (duck) return 0.075f * (1.0f - acc);
        return 0.150f * (1.0f - acc);
    }

    case SP_WEAPON_FIVESEVEN:
    {
        float acc = SP_TimeAccuracy(s, 0.92f, 0.275f, 0.725f, 0.92f);
        if (!onGround) return 1.50f * (1.0f - acc);
        if (speed > 0.0f) return 0.255f * (1.0f - acc);
        if (duck) return 0.075f * (1.0f - acc);
        return 0.150f * (1.0f - acc);
    }

    case SP_WEAPON_ELITE:
    {
        float acc = SP_TimeAccuracy(s, 0.88f, 0.275f, 0.55f, 0.88f);
        if (!onGround) return 1.30f * (1.0f - acc);
        if (speed > 0.0f) return 0.175f * (1.0f - acc);
        if (duck) return 0.080f * (1.0f - acc);
        return 0.100f * (1.0f - acc);
    }

    case SP_WEAPON_AWP:
    {
        float spread = 0.001f;
        if (!onGround) spread = 0.85f;
        else if (speed > 140.0f) spread = 0.25f;
        else if (speed > 10.0f) spread = 0.10f;
        else if (duck) spread = 0.000f;
        if (s.fov >= 90.0f) spread += 0.08f;
        return spread;
    }

    case SP_WEAPON_SCOUT:
    {
        float spread = 0.000f;
        if (!onGround) spread = 0.20f;
        else if (speed > 170.0f) spread = 0.075f;
        else if (speed > 10.0f) spread = 0.025f;
        if (s.fov >= 90.0f) spread += 0.025f;
        return spread;
    }

    case SP_WEAPON_G3SG1:
    case SP_WEAPON_SG550:
    {
        float acc = SP_AccuracyByShots(simulatedShots, 200.0f, 0.55f, 0.98f);
        float spread = 0.025f * acc;
        if (!onGround) spread = 0.45f * acc;
        else if (speed > 140.0f) spread = 0.15f * acc;
        else if (speed > 10.0f) spread = 0.075f * acc;
        if (s.fov >= 90.0f) spread += 0.025f;
        return spread;
    }

    case SP_WEAPON_M3:
        return 0.0675f;

    case SP_WEAPON_XM1014:
        return 0.0725f;

    default:
        return 0.0f;
    }
}

static SpreadDotVec2 SP_ProjectToScreen(const SpreadDotState &s, const SpreadDotVec2 &tangentXY)
{
    SpreadDotVec2 out;
    const float cx = ScreenWidth * 0.5f;
    const float cy = ScreenHeight * 0.5f;
    float fov = s.fov;
    if (fov <= 1.0f || fov > 179.0f)
        fov = 90.0f;

    const float fovX = SP_DegToRad(fov);
    const float aspect = (ScreenHeight > 0) ? ((float)ScreenWidth / (float)ScreenHeight) : 1.333333f;
    const float fovY = 2.0f * atanf(tanf(fovX * 0.5f) / aspect);

    const float focalX = cx / tanf(fovX * 0.5f);
    const float focalY = cy / tanf(fovY * 0.5f);

    out.x = cx + tangentXY.x * focalX;
    out.y = cy - tangentXY.y * focalY;
    return out;
}

static void SP_DrawFilledDot(int cx, int cy, int radius, int r, int g, int b, int a)
{
    if (radius < 1)
        radius = 1;

    for (int y = -radius; y <= radius; ++y)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            if (x * x + y * y <= radius * radius)
                gEngfuncs.pfnFillRGBA(cx + x, cy + y, 1, 1, r, g, b, a);
        }
    }
}

static void SP_ReadColor(int &r, int &g, int &b, int &a)
{
    r = 0;
    g = 255;
    b = 255;
    a = 230;

    if (cl_spreaddot_color && cl_spreaddot_color->string)
    {
        int rr = r, gg = g, bb = b, aa = a;
        int read = sscanf(cl_spreaddot_color->string, "%d %d %d %d", &rr, &gg, &bb, &aa);
        if (read >= 3)
        {
            r = rr;
            g = gg;
            b = bb;
            a = (read >= 4) ? aa : 230;
        }
    }

    r = (int)SP_Clamp((float)r, 0.0f, 255.0f);
    g = (int)SP_Clamp((float)g, 0.0f, 255.0f);
    b = (int)SP_Clamp((float)b, 0.0f, 255.0f);
    a = (int)SP_Clamp((float)a, 0.0f, 255.0f);
}

static void SP_DrawFallbackDot(int radius)
{
    if (!cl_spreaddot_fallback || cl_spreaddot_fallback->value <= 0.0f)
        return;
    if (ScreenWidth <= 0 || ScreenHeight <= 0)
        return;

    const int cx = ScreenWidth / 2;
    const int cy = ScreenHeight / 2;

    // Orange fallback: Draw() is hooked, but Capture() has not produced a valid
    // bullet-weapon state yet. Cyan means real predicted state.
    SP_DrawFilledDot(cx, cy, radius + 1, 0, 0, 0, 180);
    SP_DrawFilledDot(cx, cy, radius, 255, 128, 0, 230);

    if (cl_spreaddot_debug && cl_spreaddot_debug->value > 0.0f)
    {
        gEngfuncs.pfnFillRGBA(cx - 8, cy, 17, 1, 255, 255, 255, 140);
        gEngfuncs.pfnFillRGBA(cx, cy - 8, 1, 17, 255, 255, 255, 140);
    }
}

void SpreadDot_Capture(local_state_t *from, local_state_t *to, struct usercmd_s *cmd,
                       double time, unsigned int random_seed)
{
    SP_RegisterCVars();
    g_spreadDot.captureCount++;

    const int oldWeaponId = g_spreadDot.weaponId;
    const int oldShotsFired = g_spreadDot.shotsFired;
    const int oldButtons = g_spreadDot.buttons;
    const float oldFov = g_spreadDot.fov;
    const Vector oldPunch = g_spreadDot.punchangle;
    const bool oldValid = g_spreadDot.valid;

    if (!from || !to || !cmd)
    {
        g_spreadDot.valid = false;
        g_spreadDot.refreshDot = true;
        g_spreadDot.hasCachedDot = false;
        return;
    }

    int id = to->client.m_iId;
    if (id <= 0)
        id = from->client.m_iId;

    if (id <= 0 || id >= SP_MAX_WEAPONS || !SP_IsBulletWeapon(id))
    {
        g_spreadDot.weaponId = id;
        g_spreadDot.valid = false;
        g_spreadDot.refreshDot = true;
        g_spreadDot.hasCachedDot = false;
        return;
    }

    // Use predicted output state when available. HUD_WeaponsPostThink writes
    // m_iShotsFired to m_fInZoom and m_flLastFire to m_fAimedDamage.
    const weapon_data_t *wd = to->weapondata + id;

    const int newShotsFired = (int)wd->m_fInZoom;
    const int newButtons = cmd->buttons;
    const float newFov = to->client.fov > 1.0f ? to->client.fov : from->client.fov;
    const Vector newPunch = to->client.punchangle;

    const bool attackNow = (newButtons & IN_ATTACK) != 0;
    const bool attackWasDown = (oldButtons & IN_ATTACK) != 0;
    const bool attackEdge = attackNow && !attackWasDown;
    const bool weaponChanged = !oldValid || id != oldWeaponId;
    const bool shotsChanged = oldValid && newShotsFired != oldShotsFired;
    const bool fovChanged = oldValid && SP_Abs(newFov - oldFov) > 0.5f;
    const bool punchChanged = oldValid && SP_PunchChangedEnough(newPunch, oldPunch);

    g_spreadDot.weaponId = id;
    g_spreadDot.weaponState = wd->m_iWeaponState;
    g_spreadDot.shotsFired = newShotsFired;
    g_spreadDot.clip = wd->m_iClip;
    g_spreadDot.flags = to->client.flags;
    g_spreadDot.waterlevel = to->client.waterlevel;
    g_spreadDot.buttons = newButtons;
    g_spreadDot.time = (float)time;
    g_spreadDot.lastFire = wd->m_fAimedDamage;
    g_spreadDot.nextAttack = to->client.m_flNextAttack;
    g_spreadDot.nextPrimaryAttack = wd->m_flNextPrimaryAttack;
    g_spreadDot.fov = newFov;
    g_spreadDot.randomSeed = random_seed;
    g_spreadDot.velocity = to->client.velocity;
    g_spreadDot.punchangle = newPunch;
    g_spreadDot.valid = true;

    // Do not recompute the random component every frame while idle. Recompute
    // only when state actually changed or when a shot/attack is being predicted.
    g_spreadDot.refreshDot = weaponChanged || fovChanged || attackEdge || shotsChanged || punchChanged;

    // Use the live command seed only when a shot/attack event happens.
    // Recoil decay changes punchangle every frame; it should move the cached dot
    // back to center, but must not generate a new random spread point each frame.
    g_spreadDot.useLiveSeed = attackEdge || shotsChanged;

    if (weaponChanged)
        g_spreadDot.hasCachedDot = false;
}

/* Inaccuracy atual da arma em graus (usada pelo accuracy boost da mira). */
float SpreadDot_GetSpreadDegrees(void)
{
    if (!g_spreadDot.valid || !SP_IsBulletWeapon(g_spreadDot.weaponId))
        return 0.0f;

    const int simulatedShots = g_spreadDot.shotsFired + 1;
    float spread = SP_ComputeWeaponSpread(g_spreadDot, simulatedShots);

    if (spread < 0.0f)
        spread = 0.0f;

    return atanf(spread) * (float)(180.0 / M_PI);
}

/* Offset tangente (radianos) que o proximo tiro recebe com esta seed.
   Retorna 0 quando a arma nao tem spread modelado. */
int SpreadDot_GetSpreadOffset(unsigned int seed, float *dx, float *dy)
{
    if (!dx || !dy)
        return 0;
    *dx = 0.0f;
    *dy = 0.0f;

    if (!g_spreadDot.valid || !SP_IsBulletWeapon(g_spreadDot.weaponId))
        return 0;

    const int simulatedShots = g_spreadDot.shotsFired + 1;
    const float spread = SP_ComputeWeaponSpread(g_spreadDot, simulatedShots);
    const SpreadDotVec2 off = SP_ComputeSharedSpreadOffset(seed, spread);

    *dx = off.x;
    *dy = off.y;
    return 1;
}

void SpreadDot_UpdateClientData(client_data_t *cdata, float time)
{
    SP_RegisterCVars();

    if (!cdata)
        return;

    g_spreadDot.hasCamera = true;
    if (cdata->fov > 1.0f)
        g_spreadDot.fov = cdata->fov;
    if (time > 0.0f)
        g_spreadDot.time = time;
}

void SpreadDot_Draw(float flTime)
{
    SP_RegisterCVars();
    g_spreadDot.drawCount++;

    if (!cl_spreaddot || cl_spreaddot->value <= 0.0f)
        return;
    if (ScreenWidth <= 0 || ScreenHeight <= 0)
        return;

    int radius = cl_spreaddot_size ? (int)cl_spreaddot_size->value : 5;
    radius = (int)SP_Clamp((float)radius, 1.0f, 20.0f);

    if (!g_spreadDot.valid || !SP_IsBulletWeapon(g_spreadDot.weaponId))
    {
        SP_DrawFallbackDot(radius);
        return;
    }

    if (flTime > 0.0f)
        g_spreadDot.time = flTime;

    int mode = cl_spreaddot_mode ? (int)cl_spreaddot_mode->value : 2;
    int simulatedShots = g_spreadDot.shotsFired;
    if (mode >= 2)
        simulatedShots++;

    const int idleRandomMode = cl_spreaddot_idle_random ? (int)cl_spreaddot_idle_random->value : 0;
    float spread = SP_ComputeWeaponSpread(g_spreadDot, simulatedShots);

    SpreadDotVec2 offset;
    offset.x = 0.0f;
    offset.y = 0.0f;

    if (g_spreadDot.useLiveSeed)
    {
        // Use the real predicted command seed only on shot/update.
        // This gives the small fire flick without idle jitter.
        offset = SP_ComputeSharedSpreadOffset(g_spreadDot.randomSeed, spread);
    }
    else if (idleRandomMode == 1)
    {
        // Stable preview while idle. It is intentionally not centered.
        offset = SP_ComputeSharedSpreadOffset(SP_MakeStableSeed(g_spreadDot), spread);
    }
    else if (idleRandomMode >= 2)
    {
        // Old live-preview behavior. Useful only for debugging; causes jitter.
        offset = SP_ComputeSharedSpreadOffset(g_spreadDot.randomSeed, spread);
    }
    else
    {
        // Default V5: while idle, do not keep or invent a random bullet offset.
        // The dot follows punch/recoil and returns to the crosshair center.
        offset.x = 0.0f;
        offset.y = 0.0f;
    }

    // Mode 0: spread only. Mode 1/2: spread plus current predicted punch angle.
    // Important V5 fix: recompute the final screen position every draw.
    // We freeze/zero the random component, not the final HUD coordinates.
    if (mode >= 1)
    {
        offset.x += tanf(SP_DegToRad(g_spreadDot.punchangle.y));
        offset.y += tanf(SP_DegToRad(-g_spreadDot.punchangle.x));
    }

    SpreadDotVec2 px = SP_ProjectToScreen(g_spreadDot, offset);

    int cx = (int)(px.x + 0.5f);
    int cy = (int)(px.y + 0.5f);
    const int screenCx = ScreenWidth / 2;
    const int screenCy = ScreenHeight / 2;

    int maxPx = cl_spreaddot_max_px ? (int)cl_spreaddot_max_px->value : 9999;
    if (maxPx < 10)
        maxPx = 10;

    if (SP_Abs((float)(cx - screenCx)) > maxPx)
        cx = screenCx + (cx > screenCx ? maxPx : -maxPx);
    if (SP_Abs((float)(cy - screenCy)) > maxPx)
        cy = screenCy + (cy > screenCy ? maxPx : -maxPx);

    g_spreadDot.cachedX = cx;
    g_spreadDot.cachedY = cy;
    g_spreadDot.cachedSpread = spread;
    g_spreadDot.hasCachedDot = true;
    g_spreadDot.refreshDot = false;
    g_spreadDot.useLiveSeed = false;

    int r, g, b, a;
    SP_ReadColor(r, g, b, a);

    SP_DrawFilledDot(cx, cy, radius + 1, 0, 0, 0, a > 180 ? 180 : a);
    SP_DrawFilledDot(cx, cy, radius, r, g, b, a);

    if (cl_spreaddot_debug && cl_spreaddot_debug->value > 0.0f)
    {
        gEngfuncs.pfnFillRGBA(screenCx - 3, screenCy, 7, 1, 255, 255, 255, 120);
        gEngfuncs.pfnFillRGBA(screenCx, screenCy - 3, 1, 7, 255, 255, 255, 120);
    }
}

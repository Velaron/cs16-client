/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "stdafx.h"
#include "eiface.h"
#include "util.h"
#include "game.h"

cvar_t *g_psv_gravity, *g_psv_aim;
cvar_t *g_footsteps;
cvar_t *g_psv_accelerate, *g_psv_friction, *g_psv_stopspeed;

cvar_t displaysoundlist = { "displaysoundlist", "0" };

cvar_t timelimit = { "mp_timelimit", "0", FCVAR_SERVER };
cvar_t friendlyfire = { "mp_friendlyfire", "0", FCVAR_SERVER };
cvar_t flashlight = { "mp_flashlight", "0", FCVAR_SERVER };

cvar_t decalfrequency = { "decalfrequency", "30", FCVAR_SERVER };

cvar_t allowmonsters = { "mp_allowmonsters", "0", FCVAR_SERVER };
cvar_t roundtime = { "mp_roundtime", "5", FCVAR_SERVER };
cvar_t buytime = { "mp_buytime", "1.5", FCVAR_SERVER };
#ifdef _DEBUG
cvar_t freezetime = { "mp_freezetime", "0", FCVAR_SERVER };
#else
cvar_t freezetime = { "mp_freezetime", "6", FCVAR_SERVER };
#endif
cvar_t c4timer = { "mp_c4timer", "45", FCVAR_SERVER };

cvar_t ghostfrequency = { "mp_ghostfrequency", "0.1", FCVAR_SERVER };
cvar_t autokick = { "mp_autokick", "1", FCVAR_SERVER };

cvar_t restartround = { "sv_restartround", "0", FCVAR_SERVER };
cvar_t sv_restart = { "sv_restart", "0", FCVAR_SERVER };

cvar_t limitteams = { "mp_limitteams", "2", FCVAR_SERVER };
cvar_t autoteambalance = { "mp_autoteambalance", "1", FCVAR_SERVER };
cvar_t tkpunish = { "mp_tkpunish", "0", FCVAR_SERVER };
cvar_t hostagepenalty = { "mp_hostagepenalty", "13", FCVAR_SERVER };
cvar_t mirrordamage = { "mp_mirrordamage", "0", FCVAR_SERVER };
cvar_t logmessages = { "mp_logmessages", "0", FCVAR_SERVER };
cvar_t forcecamera = { "mp_forcecamera", "0", FCVAR_SERVER };
cvar_t forcechasecam = { "mp_forcechasecam", "0", FCVAR_SERVER };
cvar_t mapvoteratio = { "mp_mapvoteratio", "0.66", FCVAR_SERVER };
cvar_t logdetail = { "mp_logdetail", "0", FCVAR_SERVER };
#ifdef _DEBUG
cvar_t startmoney = { "mp_startmoney", "160000", FCVAR_SERVER };
#else
cvar_t startmoney = { "mp_startmoney", "800", FCVAR_SERVER };
#endif
cvar_t maxrounds = { "mp_maxrounds", "0", FCVAR_SERVER };
cvar_t fadetoblack = { "mp_fadetoblack", "0", FCVAR_SERVER };
cvar_t windifference = { "mp_windifference", "1", FCVAR_SERVER };
cvar_t playerid = { "mp_playerid", "0", FCVAR_SERVER };
cvar_t winlimit = { "mp_winlimit", "0", FCVAR_SERVER };
cvar_t allow_spectators = { "allow_spectators", "1.0", FCVAR_SERVER };
cvar_t mp_chattime = { "mp_chattime", "10", FCVAR_SERVER };
cvar_t kick_percent = { "mp_kickpercent", "0.66", FCVAR_SERVER };

cvar_t fragsleft = { "mp_fragsleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED };
cvar_t timeleft = { "mp_timeleft", "0", FCVAR_SERVER | FCVAR_UNLOGGED };

cvar_t humans_join_team = { "humans_join_team", "any", FCVAR_SERVER };

cvar_t sk_agrunt_health1 = { "sk_agrunt_health1", "0" };
cvar_t sk_agrunt_health2 = { "sk_agrunt_health2", "0" };
cvar_t sk_agrunt_health3 = { "sk_agrunt_health3", "0" };

cvar_t sk_agrunt_dmg_punch1 = { "sk_agrunt_dmg_punch1", "0" };
cvar_t sk_agrunt_dmg_punch2 = { "sk_agrunt_dmg_punch2", "0" };
cvar_t sk_agrunt_dmg_punch3 = { "sk_agrunt_dmg_punch3", "0" };

cvar_t sk_apache_health1 = { "sk_apache_health1", "0" };
cvar_t sk_apache_health2 = { "sk_apache_health2", "0" };
cvar_t sk_apache_health3 = { "sk_apache_health3", "0" };

cvar_t sk_barney_health1 = { "sk_barney_health1", "0" };
cvar_t sk_barney_health2 = { "sk_barney_health2", "0" };
cvar_t sk_barney_health3 = { "sk_barney_health3", "0" };

cvar_t sk_bullsquid_health1 = { "sk_bullsquid_health1", "0" };
cvar_t sk_bullsquid_health2 = { "sk_bullsquid_health2", "0" };
cvar_t sk_bullsquid_health3 = { "sk_bullsquid_health3", "0" };

cvar_t sk_bullsquid_dmg_bite1 = { "sk_bullsquid_dmg_bite1", "0" };
cvar_t sk_bullsquid_dmg_bite2 = { "sk_bullsquid_dmg_bite2", "0" };
cvar_t sk_bullsquid_dmg_bite3 = { "sk_bullsquid_dmg_bite3", "0" };

cvar_t sk_bullsquid_dmg_whip1 = { "sk_bullsquid_dmg_whip1", "0" };
cvar_t sk_bullsquid_dmg_whip2 = { "sk_bullsquid_dmg_whip2", "0" };
cvar_t sk_bullsquid_dmg_whip3 = { "sk_bullsquid_dmg_whip3", "0" };

cvar_t sk_bullsquid_dmg_spit1 = { "sk_bullsquid_dmg_spit1", "0" };
cvar_t sk_bullsquid_dmg_spit2 = { "sk_bullsquid_dmg_spit2", "0" };
cvar_t sk_bullsquid_dmg_spit3 = { "sk_bullsquid_dmg_spit3", "0" };

cvar_t sk_bigmomma_health_factor1 = { "sk_bigmomma_health_factor1", "1.0" };
cvar_t sk_bigmomma_health_factor2 = { "sk_bigmomma_health_factor2", "1.0" };
cvar_t sk_bigmomma_health_factor3 = { "sk_bigmomma_health_factor3", "1.0" };

cvar_t sk_bigmomma_dmg_slash1 = { "sk_bigmomma_dmg_slash1", "50" };
cvar_t sk_bigmomma_dmg_slash2 = { "sk_bigmomma_dmg_slash2", "50" };
cvar_t sk_bigmomma_dmg_slash3 = { "sk_bigmomma_dmg_slash3", "50" };

cvar_t sk_bigmomma_dmg_blast1 = { "sk_bigmomma_dmg_blast1", "100" };
cvar_t sk_bigmomma_dmg_blast2 = { "sk_bigmomma_dmg_blast2", "100" };
cvar_t sk_bigmomma_dmg_blast3 = { "sk_bigmomma_dmg_blast3", "100" };

cvar_t sk_bigmomma_radius_blast1 = { "sk_bigmomma_radius_blast1", "250" };
cvar_t sk_bigmomma_radius_blast2 = { "sk_bigmomma_radius_blast2", "250" };
cvar_t sk_bigmomma_radius_blast3 = { "sk_bigmomma_radius_blast3", "250" };

cvar_t sk_gargantua_health1 = { "sk_gargantua_health1", "0" };
cvar_t sk_gargantua_health2 = { "sk_gargantua_health2", "0" };
cvar_t sk_gargantua_health3 = { "sk_gargantua_health3", "0" };

cvar_t sk_gargantua_dmg_slash1 = { "sk_gargantua_dmg_slash1", "0" };
cvar_t sk_gargantua_dmg_slash2 = { "sk_gargantua_dmg_slash2", "0" };
cvar_t sk_gargantua_dmg_slash3 = { "sk_gargantua_dmg_slash3", "0" };

cvar_t sk_gargantua_dmg_fire1 = { "sk_gargantua_dmg_fire1", "0" };
cvar_t sk_gargantua_dmg_fire2 = { "sk_gargantua_dmg_fire2", "0" };
cvar_t sk_gargantua_dmg_fire3 = { "sk_gargantua_dmg_fire3", "0" };

cvar_t sk_gargantua_dmg_stomp1 = { "sk_gargantua_dmg_stomp1", "0" };
cvar_t sk_gargantua_dmg_stomp2 = { "sk_gargantua_dmg_stomp2", "0" };
cvar_t sk_gargantua_dmg_stomp3 = { "sk_gargantua_dmg_stomp3", "0" };

cvar_t sk_hassassin_health1 = { "sk_hassassin_health1", "0" };
cvar_t sk_hassassin_health2 = { "sk_hassassin_health2", "0" };
cvar_t sk_hassassin_health3 = { "sk_hassassin_health3", "0" };

cvar_t sk_headcrab_health1 = { "sk_headcrab_health1", "0" };
cvar_t sk_headcrab_health2 = { "sk_headcrab_health2", "0" };
cvar_t sk_headcrab_health3 = { "sk_headcrab_health3", "0" };

cvar_t sk_headcrab_dmg_bite1 = { "sk_headcrab_dmg_bite1", "0" };
cvar_t sk_headcrab_dmg_bite2 = { "sk_headcrab_dmg_bite2", "0" };
cvar_t sk_headcrab_dmg_bite3 = { "sk_headcrab_dmg_bite3", "0" };

cvar_t sk_hgrunt_health1 = { "sk_hgrunt_health1", "0" };
cvar_t sk_hgrunt_health2 = { "sk_hgrunt_health2", "0" };
cvar_t sk_hgrunt_health3 = { "sk_hgrunt_health3", "0" };

cvar_t sk_hgrunt_kick1 = { "sk_hgrunt_kick1", "0" };
cvar_t sk_hgrunt_kick2 = { "sk_hgrunt_kick2", "0" };
cvar_t sk_hgrunt_kick3 = { "sk_hgrunt_kick3", "0" };

cvar_t sk_hgrunt_pellets1 = { "sk_hgrunt_pellets1", "0" };
cvar_t sk_hgrunt_pellets2 = { "sk_hgrunt_pellets2", "0" };
cvar_t sk_hgrunt_pellets3 = { "sk_hgrunt_pellets3", "0" };

cvar_t sk_hgrunt_gspeed1 = { "sk_hgrunt_gspeed1", "0" };
cvar_t sk_hgrunt_gspeed2 = { "sk_hgrunt_gspeed2", "0" };
cvar_t sk_hgrunt_gspeed3 = { "sk_hgrunt_gspeed3", "0" };

cvar_t sk_houndeye_health1 = { "sk_houndeye_health1", "0" };
cvar_t sk_houndeye_health2 = { "sk_houndeye_health2", "0" };
cvar_t sk_houndeye_health3 = { "sk_houndeye_health3", "0" };

cvar_t sk_houndeye_dmg_blast1 = { "sk_houndeye_dmg_blast1", "0" };
cvar_t sk_houndeye_dmg_blast2 = { "sk_houndeye_dmg_blast2", "0" };
cvar_t sk_houndeye_dmg_blast3 = { "sk_houndeye_dmg_blast3", "0" };

cvar_t sk_islave_health1 = { "sk_islave_health1", "0" };
cvar_t sk_islave_health2 = { "sk_islave_health2", "0" };
cvar_t sk_islave_health3 = { "sk_islave_health3", "0" };

cvar_t sk_islave_dmg_claw1 = { "sk_islave_dmg_claw1", "0" };
cvar_t sk_islave_dmg_claw2 = { "sk_islave_dmg_claw2", "0" };
cvar_t sk_islave_dmg_claw3 = { "sk_islave_dmg_claw3", "0" };

cvar_t sk_islave_dmg_clawrake1 = { "sk_islave_dmg_clawrake1", "0" };
cvar_t sk_islave_dmg_clawrake2 = { "sk_islave_dmg_clawrake2", "0" };
cvar_t sk_islave_dmg_clawrake3 = { "sk_islave_dmg_clawrake3", "0" };

cvar_t sk_islave_dmg_zap1 = { "sk_islave_dmg_zap1", "0" };
cvar_t sk_islave_dmg_zap2 = { "sk_islave_dmg_zap2", "0" };
cvar_t sk_islave_dmg_zap3 = { "sk_islave_dmg_zap3", "0" };

cvar_t sk_ichthyosaur_health1 = { "sk_ichthyosaur_health1", "0" };
cvar_t sk_ichthyosaur_health2 = { "sk_ichthyosaur_health2", "0" };
cvar_t sk_ichthyosaur_health3 = { "sk_ichthyosaur_health3", "0" };

cvar_t sk_ichthyosaur_shake1 = { "sk_ichthyosaur_shake1", "0" };
cvar_t sk_ichthyosaur_shake2 = { "sk_ichthyosaur_shake2", "0" };
cvar_t sk_ichthyosaur_shake3 = { "sk_ichthyosaur_shake3", "0" };

cvar_t sk_leech_health1 = { "sk_leech_health1", "0" };
cvar_t sk_leech_health2 = { "sk_leech_health2", "0" };
cvar_t sk_leech_health3 = { "sk_leech_health3", "0" };

cvar_t sk_leech_dmg_bite1 = { "sk_leech_dmg_bite1", "0" };
cvar_t sk_leech_dmg_bite2 = { "sk_leech_dmg_bite2", "0" };
cvar_t sk_leech_dmg_bite3 = { "sk_leech_dmg_bite3", "0" };

cvar_t sk_controller_health1 = { "sk_controller_health1", "0" };
cvar_t sk_controller_health2 = { "sk_controller_health2", "0" };
cvar_t sk_controller_health3 = { "sk_controller_health3", "0" };

cvar_t sk_controller_dmgzap1 = { "sk_controller_dmgzap1", "0" };
cvar_t sk_controller_dmgzap2 = { "sk_controller_dmgzap2", "0" };
cvar_t sk_controller_dmgzap3 = { "sk_controller_dmgzap3", "0" };

cvar_t sk_controller_speedball1 = { "sk_controller_speedball1", "0" };
cvar_t sk_controller_speedball2 = { "sk_controller_speedball2", "0" };
cvar_t sk_controller_speedball3 = { "sk_controller_speedball3", "0" };

cvar_t sk_controller_dmgball1 = { "sk_controller_dmgball1", "0" };
cvar_t sk_controller_dmgball2 = { "sk_controller_dmgball2", "0" };
cvar_t sk_controller_dmgball3 = { "sk_controller_dmgball3", "0" };

cvar_t sk_nihilanth_health1 = { "sk_nihilanth_health1", "0" };
cvar_t sk_nihilanth_health2 = { "sk_nihilanth_health2", "0" };
cvar_t sk_nihilanth_health3 = { "sk_nihilanth_health3", "0" };

cvar_t sk_nihilanth_zap1 = { "sk_nihilanth_zap1", "0" };
cvar_t sk_nihilanth_zap2 = { "sk_nihilanth_zap2", "0" };
cvar_t sk_nihilanth_zap3 = { "sk_nihilanth_zap3", "0" };

cvar_t sk_scientist_health1 = { "sk_scientist_health1", "0" };
cvar_t sk_scientist_health2 = { "sk_scientist_health2", "0" };
cvar_t sk_scientist_health3 = { "sk_scientist_health3", "0" };

cvar_t sk_snark_health1 = { "sk_snark_health1", "0" };
cvar_t sk_snark_health2 = { "sk_snark_health2", "0" };
cvar_t sk_snark_health3 = { "sk_snark_health3", "0" };

cvar_t sk_snark_dmg_bite1 = { "sk_snark_dmg_bite1", "0" };
cvar_t sk_snark_dmg_bite2 = { "sk_snark_dmg_bite2", "0" };
cvar_t sk_snark_dmg_bite3 = { "sk_snark_dmg_bite3", "0" };

cvar_t sk_snark_dmg_pop1 = { "sk_snark_dmg_pop1", "0" };
cvar_t sk_snark_dmg_pop2 = { "sk_snark_dmg_pop2", "0" };
cvar_t sk_snark_dmg_pop3 = { "sk_snark_dmg_pop3", "0" };

cvar_t sk_zombie_health1 = { "sk_zombie_health1", "0" };
cvar_t sk_zombie_health2 = { "sk_zombie_health2", "0" };
cvar_t sk_zombie_health3 = { "sk_zombie_health3", "0" };

cvar_t sk_zombie_dmg_one_slash1 = { "sk_zombie_dmg_one_slash1", "0" };
cvar_t sk_zombie_dmg_one_slash2 = { "sk_zombie_dmg_one_slash2", "0" };
cvar_t sk_zombie_dmg_one_slash3 = { "sk_zombie_dmg_one_slash3", "0" };

cvar_t sk_zombie_dmg_both_slash1 = { "sk_zombie_dmg_both_slash1", "0" };
cvar_t sk_zombie_dmg_both_slash2 = { "sk_zombie_dmg_both_slash2", "0" };
cvar_t sk_zombie_dmg_both_slash3 = { "sk_zombie_dmg_both_slash3", "0" };

cvar_t sk_turret_health1 = { "sk_turret_health1", "0" };
cvar_t sk_turret_health2 = { "sk_turret_health2", "0" };
cvar_t sk_turret_health3 = { "sk_turret_health3", "0" };

cvar_t sk_miniturret_health1 = { "sk_miniturret_health1", "0" };
cvar_t sk_miniturret_health2 = { "sk_miniturret_health2", "0" };
cvar_t sk_miniturret_health3 = { "sk_miniturret_health3", "0" };

cvar_t sk_sentry_health1 = { "sk_sentry_health1", "0" };
cvar_t sk_sentry_health2 = { "sk_sentry_health2", "0" };
cvar_t sk_sentry_health3 = { "sk_sentry_health3", "0" };

cvar_t sk_plr_crowbar1 = { "sk_plr_crowbar1", "0" };
cvar_t sk_plr_crowbar2 = { "sk_plr_crowbar2", "0" };
cvar_t sk_plr_crowbar3 = { "sk_plr_crowbar3", "0" };

cvar_t sk_plr_9mm_bullet1 = { "sk_plr_9mm_bullet1", "0" };
cvar_t sk_plr_9mm_bullet2 = { "sk_plr_9mm_bullet2", "0" };
cvar_t sk_plr_9mm_bullet3 = { "sk_plr_9mm_bullet3", "0" };

cvar_t sk_plr_357_bullet1 = { "sk_plr_357_bullet1", "0" };
cvar_t sk_plr_357_bullet2 = { "sk_plr_357_bullet2", "0" };
cvar_t sk_plr_357_bullet3 = { "sk_plr_357_bullet3", "0" };

cvar_t sk_plr_9mmAR_bullet1 = { "sk_plr_9mmAR_bullet1", "0" };
cvar_t sk_plr_9mmAR_bullet2 = { "sk_plr_9mmAR_bullet2", "0" };
cvar_t sk_plr_9mmAR_bullet3 = { "sk_plr_9mmAR_bullet3", "0" };

cvar_t sk_plr_9mmAR_grenade1 = { "sk_plr_9mmAR_grenade1", "0" };
cvar_t sk_plr_9mmAR_grenade2 = { "sk_plr_9mmAR_grenade2", "0" };
cvar_t sk_plr_9mmAR_grenade3 = { "sk_plr_9mmAR_grenade3", "0" };

cvar_t sk_plr_buckshot1 = { "sk_plr_buckshot1", "0" };
cvar_t sk_plr_buckshot2 = { "sk_plr_buckshot2", "0" };
cvar_t sk_plr_buckshot3 = { "sk_plr_buckshot3", "0" };

cvar_t sk_plr_xbow_bolt_client1 = { "sk_plr_xbow_bolt_client1", "0" };
cvar_t sk_plr_xbow_bolt_client2 = { "sk_plr_xbow_bolt_client2", "0" };
cvar_t sk_plr_xbow_bolt_client3 = { "sk_plr_xbow_bolt_client3", "0" };

cvar_t sk_plr_xbow_bolt_monster1 = { "sk_plr_xbow_bolt_monster1", "0" };
cvar_t sk_plr_xbow_bolt_monster2 = { "sk_plr_xbow_bolt_monster2", "0" };
cvar_t sk_plr_xbow_bolt_monster3 = { "sk_plr_xbow_bolt_monster3", "0" };

cvar_t sk_plr_rpg1 = { "sk_plr_rpg1", "0" };
cvar_t sk_plr_rpg2 = { "sk_plr_rpg2", "0" };
cvar_t sk_plr_rpg3 = { "sk_plr_rpg3", "0" };

cvar_t sk_plr_gauss1 = { "sk_plr_gauss1", "0" };
cvar_t sk_plr_gauss2 = { "sk_plr_gauss2", "0" };
cvar_t sk_plr_gauss3 = { "sk_plr_gauss3", "0" };

cvar_t sk_plr_egon_narrow1 = { "sk_plr_egon_narrow1", "0" };
cvar_t sk_plr_egon_narrow2 = { "sk_plr_egon_narrow2", "0" };
cvar_t sk_plr_egon_narrow3 = { "sk_plr_egon_narrow3", "0" };

cvar_t sk_plr_egon_wide1 = { "sk_plr_egon_wide1", "0" };
cvar_t sk_plr_egon_wide2 = { "sk_plr_egon_wide2", "0" };
cvar_t sk_plr_egon_wide3 = { "sk_plr_egon_wide3", "0" };

cvar_t sk_plr_hand_grenade1 = { "sk_plr_hand_grenade1", "0" };
cvar_t sk_plr_hand_grenade2 = { "sk_plr_hand_grenade2", "0" };
cvar_t sk_plr_hand_grenade3 = { "sk_plr_hand_grenade3", "0" };

cvar_t sk_plr_satchel1 = { "sk_plr_satchel1", "0" };
cvar_t sk_plr_satchel2 = { "sk_plr_satchel2", "0" };
cvar_t sk_plr_satchel3 = { "sk_plr_satchel3", "0" };

cvar_t sk_plr_tripmine1 = { "sk_plr_tripmine1", "0" };
cvar_t sk_plr_tripmine2 = { "sk_plr_tripmine2", "0" };
cvar_t sk_plr_tripmine3 = { "sk_plr_tripmine3", "0" };

cvar_t sk_12mm_bullet1 = { "sk_12mm_bullet1", "0" };
cvar_t sk_12mm_bullet2 = { "sk_12mm_bullet2", "0" };
cvar_t sk_12mm_bullet3 = { "sk_12mm_bullet3", "0" };

cvar_t sk_9mmAR_bullet1 = { "sk_9mmAR_bullet1", "0" };
cvar_t sk_9mmAR_bullet2 = { "sk_9mmAR_bullet2", "0" };
cvar_t sk_9mmAR_bullet3 = { "sk_9mmAR_bullet3", "0" };

cvar_t sk_9mm_bullet1 = { "sk_9mm_bullet1", "0" };
cvar_t sk_9mm_bullet2 = { "sk_9mm_bullet2", "0" };
cvar_t sk_9mm_bullet3 = { "sk_9mm_bullet3", "0" };

cvar_t sk_hornet_dmg1 = { "sk_hornet_dmg1", "0" };
cvar_t sk_hornet_dmg2 = { "sk_hornet_dmg2", "0" };
cvar_t sk_hornet_dmg3 = { "sk_hornet_dmg3", "0" };

cvar_t sk_suitcharger1 = { "sk_suitcharger1", "0" };
cvar_t sk_suitcharger2 = { "sk_suitcharger2", "0" };
cvar_t sk_suitcharger3 = { "sk_suitcharger3", "0" };

cvar_t sk_battery1 = { "sk_battery1", "0" };
cvar_t sk_battery2 = { "sk_battery2", "0" };
cvar_t sk_battery3 = { "sk_battery3", "0" };

cvar_t sk_healthcharger1 = { "sk_healthcharger1", "0" };
cvar_t sk_healthcharger2 = { "sk_healthcharger2", "0" };
cvar_t sk_healthcharger3 = { "sk_healthcharger3", "0" };

cvar_t sk_healthkit1 = { "sk_healthkit1", "0" };
cvar_t sk_healthkit2 = { "sk_healthkit2", "0" };
cvar_t sk_healthkit3 = { "sk_healthkit3", "0" };

cvar_t sk_scientist_heal1 = { "sk_scientist_heal1", "0" };
cvar_t sk_scientist_heal2 = { "sk_scientist_heal2", "0" };
cvar_t sk_scientist_heal3 = { "sk_scientist_heal3", "0" };

cvar_t sk_monster_head1 = { "sk_monster_head1", "2" };
cvar_t sk_monster_head2 = { "sk_monster_head2", "2" };
cvar_t sk_monster_head3 = { "sk_monster_head3", "2" };

cvar_t sk_monster_chest1 = { "sk_monster_chest1", "1" };
cvar_t sk_monster_chest2 = { "sk_monster_chest2", "1" };
cvar_t sk_monster_chest3 = { "sk_monster_chest3", "1" };

cvar_t sk_monster_stomach1 = { "sk_monster_stomach1", "1" };
cvar_t sk_monster_stomach2 = { "sk_monster_stomach2", "1" };
cvar_t sk_monster_stomach3 = { "sk_monster_stomach3", "1" };

cvar_t sk_monster_arm1 = { "sk_monster_arm1", "1" };
cvar_t sk_monster_arm2 = { "sk_monster_arm2", "1" };
cvar_t sk_monster_arm3 = { "sk_monster_arm3", "1" };

cvar_t sk_monster_leg1 = { "sk_monster_leg1", "1" };
cvar_t sk_monster_leg2 = { "sk_monster_leg2", "1" };
cvar_t sk_monster_leg3 = { "sk_monster_leg3", "1" };

cvar_t sk_player_head1 = { "sk_player_head1", "2" };
cvar_t sk_player_head2 = { "sk_player_head2", "2" };
cvar_t sk_player_head3 = { "sk_player_head3", "2" };

cvar_t sk_player_chest1 = { "sk_player_chest1", "1" };
cvar_t sk_player_chest2 = { "sk_player_chest2", "1" };
cvar_t sk_player_chest3 = { "sk_player_chest3", "1" };

cvar_t sk_player_stomach1 = { "sk_player_stomach1", "1" };
cvar_t sk_player_stomach2 = { "sk_player_stomach2", "1" };
cvar_t sk_player_stomach3 = { "sk_player_stomach3", "1" };

cvar_t sk_player_arm1 = { "sk_player_arm1", "1" };
cvar_t sk_player_arm2 = { "sk_player_arm2", "1" };
cvar_t sk_player_arm3 = { "sk_player_arm3", "1" };

cvar_t sk_player_leg1 = { "sk_player_leg1", "1" };
cvar_t sk_player_leg2 = { "sk_player_leg2", "1" };
cvar_t sk_player_leg3 = { "sk_player_leg3", "1" };

void GameDLLInit(void)
{
	g_psv_gravity = CVAR_GET_POINTER("sv_gravity");
	g_psv_aim = CVAR_GET_POINTER("sv_aim");
	g_footsteps = CVAR_GET_POINTER("mp_footsteps");
	g_psv_accelerate = CVAR_GET_POINTER("sv_accelerate");
	g_psv_friction = CVAR_GET_POINTER("sv_friction");
	g_psv_stopspeed = CVAR_GET_POINTER("sv_stopspeed");

	CVAR_REGISTER(&displaysoundlist);

	CVAR_REGISTER(&timelimit);
	CVAR_REGISTER(&friendlyfire);
	CVAR_REGISTER(&flashlight);

	CVAR_REGISTER(&decalfrequency);

	CVAR_REGISTER(&allowmonsters);
	CVAR_REGISTER(&roundtime);
	CVAR_REGISTER(&buytime);
	CVAR_REGISTER(&freezetime);
	CVAR_REGISTER(&c4timer);
	CVAR_REGISTER(&ghostfrequency);
	CVAR_REGISTER(&autokick);

	CVAR_REGISTER(&restartround);
	CVAR_REGISTER(&sv_restart);

	CVAR_REGISTER(&limitteams);
	CVAR_REGISTER(&autoteambalance);
	CVAR_REGISTER(&tkpunish);
	CVAR_REGISTER(&hostagepenalty);
	CVAR_REGISTER(&mirrordamage);
	CVAR_REGISTER(&logmessages);
	CVAR_REGISTER(&forcecamera);
	CVAR_REGISTER(&forcechasecam);

	CVAR_REGISTER(&mapvoteratio);
	CVAR_REGISTER(&maxrounds);
	CVAR_REGISTER(&winlimit);
	CVAR_REGISTER(&windifference);
	CVAR_REGISTER(&fadetoblack);
	CVAR_REGISTER(&logdetail);
	CVAR_REGISTER(&startmoney);
	CVAR_REGISTER(&playerid);
	CVAR_REGISTER(&allow_spectators);
	CVAR_REGISTER(&mp_chattime);

	CVAR_REGISTER(&kick_percent);
	CVAR_REGISTER(&fragsleft);
	CVAR_REGISTER(&timeleft);

	CVAR_REGISTER(&humans_join_team);

	CVAR_REGISTER(&sk_plr_9mm_bullet1);
	CVAR_REGISTER(&sk_plr_9mm_bullet2);
	CVAR_REGISTER(&sk_plr_9mm_bullet3);

	CVAR_REGISTER(&sk_plr_357_bullet1);
	CVAR_REGISTER(&sk_plr_357_bullet2);
	CVAR_REGISTER(&sk_plr_357_bullet3);

	CVAR_REGISTER(&sk_plr_9mmAR_bullet1);
	CVAR_REGISTER(&sk_plr_9mmAR_bullet2);
	CVAR_REGISTER(&sk_plr_9mmAR_bullet3);

	CVAR_REGISTER(&sk_plr_9mmAR_grenade1);
	CVAR_REGISTER(&sk_plr_9mmAR_grenade2);
	CVAR_REGISTER(&sk_plr_9mmAR_grenade3);

	CVAR_REGISTER(&sk_plr_buckshot1);
	CVAR_REGISTER(&sk_plr_buckshot2);
	CVAR_REGISTER(&sk_plr_buckshot3);

	CVAR_REGISTER(&sk_plr_rpg1);
	CVAR_REGISTER(&sk_plr_rpg2);
	CVAR_REGISTER(&sk_plr_rpg3);

	CVAR_REGISTER(&sk_12mm_bullet1);
	CVAR_REGISTER(&sk_12mm_bullet2);
	CVAR_REGISTER(&sk_12mm_bullet3);

	CVAR_REGISTER(&sk_9mmAR_bullet1);
	CVAR_REGISTER(&sk_9mmAR_bullet2);
	CVAR_REGISTER(&sk_9mmAR_bullet3);

	CVAR_REGISTER(&sk_9mm_bullet1);
	CVAR_REGISTER(&sk_9mm_bullet2);
	CVAR_REGISTER(&sk_9mm_bullet3);

	CVAR_REGISTER(&sk_suitcharger1);
	CVAR_REGISTER(&sk_suitcharger2);
	CVAR_REGISTER(&sk_suitcharger3);

	CVAR_REGISTER(&sk_battery1);
	CVAR_REGISTER(&sk_battery2);
	CVAR_REGISTER(&sk_battery3);

	CVAR_REGISTER(&sk_healthcharger1);
	CVAR_REGISTER(&sk_healthcharger2);
	CVAR_REGISTER(&sk_healthcharger3);

	CVAR_REGISTER(&sk_healthkit1);
	CVAR_REGISTER(&sk_healthkit2);
	CVAR_REGISTER(&sk_healthkit3);

	CVAR_REGISTER(&sk_scientist_heal1);
	CVAR_REGISTER(&sk_scientist_heal2);
	CVAR_REGISTER(&sk_scientist_heal3);

	SERVER_COMMAND("exec skill.cfg\n");
}
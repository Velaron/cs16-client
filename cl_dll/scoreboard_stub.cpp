#include "hud.h"
hud_player_info_t		g_PlayerInfoList[MAX_PLAYERS+1];	// player info from the engine
extra_player_info_t		g_PlayerExtraInfo[MAX_PLAYERS+1];	// additional player info sent directly to the client dll
team_info_t		g_TeamInfo[MAX_TEAMS+1];
int g_iUser1;
int g_iUser2;
int g_iUser3;
int g_iTeamNumber;
int g_iPlayerClass;

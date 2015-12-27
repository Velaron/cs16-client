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
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
#include "monsters.h"
#include "pm_shared.h"
#include "hostage.h"
#include <time.h>

#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <linux/limits.h>
#include <dlfcn.h>
#endif

#include <archtypes.h>

extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL int g_iSkillLevel;
extern DLL_GLOBAL ULONG g_ulFrameCount;

extern void CopyToBodyQue(entvars_t *pev);
extern int giPrecacheGrunt;
extern int gmsgSayText;

BOOL g_bClientPrintEnable = TRUE;

void LinkUserMessages(void);
void WriteSigonMessages(void);

void set_suicide_frame(entvars_t *pev)
{
	if (!FStrEq(STRING(pev->model), "models/player.mdl"))
		return;

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	pev->deadflag = DEAD_DEAD;
	pev->nextthink = -1;
}

BOOL ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	return g_pGameRules->ClientConnected(pEntity, pszName, pszAddress, szRejectReason);
}

void ClientDisconnect(edict_t *pEntity)
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pEntity);

	if (g_fGameOver)
		return;

	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_disconnected", STRING(pEntity->v.netname));
	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(pEntity));

	if (pSound)
		pSound->Reset();

	pEntity->v.takedamage = DAMAGE_NO;
	pEntity->v.solid = SOLID_NOT;
	pEntity->v.flags = FL_DORMANT;

	if (pPlayer)
		pPlayer->SetThink(NULL);

	UTIL_SetOrigin(&pEntity->v, pEntity->v.origin);
	g_pGameRules->ClientDisconnected(pEntity);

	if (pPlayer->IsBot())
	{
	}
}

BOOL g_skipCareerInitialSpawn;

void respawn(entvars_t *pev, BOOL fCopyCorpse)
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if (g_pGameRules->m_iTotalRoundsPlayed > 0)
			g_pGameRules->m_bSkipSpawn = false;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);

		if (g_pGameRules->IsCareer() && g_pGameRules->m_bSkipSpawn && pPlayer->IsAlive())
			g_skipCareerInitialSpawn = TRUE;

		pPlayer->Spawn();
		g_skipCareerInitialSpawn = FALSE;
	}
	else if (pev->deadflag > DEAD_NO)
		SERVER_COMMAND("reload\n");
}

void ClientKill(edict_t *pEntity)
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pl = (CBasePlayer *)CBasePlayer::Instance(pev);

	if (pl->IsObserver())
		return;

	if (pl->m_iJoiningState != JOINED)
		return;

	if (gpGlobals->time >= pl->m_fNextSuicideTime)
	{
		pl->m_LastHitGroup = 0;
		pl->m_fNextSuicideTime = gpGlobals->time + 1;
		pEntity->v.health = 0;
		pl->Killed(pev, GIB_NEVER);

		if (g_pGameRules->m_pVIP == pl)
			g_pGameRules->m_iConsecutiveVIP = 10;
	}
}

void ShowMenu(CBasePlayer *pPlayer, int bitsValidSlots, int nDisplayTime, int fNeedMore, char *pszText)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pPlayer->pev);
	WRITE_SHORT(bitsValidSlots);
	WRITE_CHAR(nDisplayTime);
	WRITE_BYTE(fNeedMore);
	WRITE_STRING(pszText);
	MESSAGE_END();
}

void ShowVGUIMenu(CBasePlayer *pPlayer, int MenuType, int BitMask, char *szOldMenu)
{
	if (pPlayer->m_bVGUIMenus == true || MenuType > MENU_BUY_ITEM)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->pev);
		WRITE_BYTE(MenuType);
		WRITE_SHORT(BitMask);
		WRITE_CHAR(-1);
		WRITE_BYTE(0);
		WRITE_STRING(" ");
		MESSAGE_END();
	}
	else
		ShowMenu(pPlayer, BitMask, -1, 0, szOldMenu);
}

int CountTeams(void)
{
	int iNumTerrorist = 0, iNumCT = 0;
	CBaseEntity *pPlayer = NULL;

	while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (FNullEnt(pPlayer->edict()))
			break;

		CBasePlayer *player = GetClassPtr((CBasePlayer *)pPlayer->pev);

		if (player->m_iTeam == TEAM_UNASSIGNED)
			continue;

		if (FBitSet(player->pev->flags, FL_DORMANT))
			continue;

		if (player->m_iTeam == TEAM_SPECTATOR)
			continue;

		if (player->m_iTeam == TEAM_CT)
			iNumCT++;
		else if (player->m_iTeam == TEAM_TERRORIST)
			iNumTerrorist++;
	}

	return iNumCT - iNumTerrorist;
}

void ListPlayers(CBasePlayer *current)
{
	char message[120], cNumber[12];
	strcpy(message, "");
	CBaseEntity *pPlayer = NULL;

	while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (FNullEnt(pPlayer->edict()))
			break;

		if (FBitSet(pPlayer->pev->flags, FL_DORMANT))
			continue;

		CBasePlayer *player = GetClassPtr((CBasePlayer *)pPlayer->pev);
		int iUserID = GETPLAYERUSERID(ENT(player->pev));
		//itoa(iUserID, cNumber, 10);
		memset(cNumber, 0, sizeof(cNumber));
		snprintf(cNumber,sizeof(cNumber)-1,"%i",iUserID);		
		strcpy(message, "\n");
		strcat(message, cNumber);
		strcat(message, " : ");
		strcat(message, STRING(player->pev->netname));
		ClientPrint(current->pev, HUD_PRINTCONSOLE, message);
	}

	ClientPrint(current->pev, HUD_PRINTCONSOLE, "\n");
}

int CountTeamPlayers(int iTeam)
{
	CBaseEntity *pPlayer = NULL;
	int i = 0;

	while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (FNullEnt(pPlayer->edict()))
			break;

		if (FBitSet(pPlayer->pev->flags, FL_DORMANT))
			continue;

		if (GetClassPtr((CBasePlayer *)pPlayer->pev)->m_iTeam == iTeam)
			i++;
	}

	return i;
}

void ProcessKickVote(CBasePlayer *pVotingPlayer, CBasePlayer *pKickPlayer)
{
	CBaseEntity *pTempEntity;
	CBasePlayer *pTempPlayer;
	int iValidVotes;
	int iVoteID;
	int iVotesNeeded;
	BOOL fKickPercent;

	if (!pVotingPlayer || !pKickPlayer)
		return;

	int iTeamCount = CountTeamPlayers(pVotingPlayer->m_iTeam);

	if (iTeamCount < 3)
		return;

	iValidVotes = 0;
	pTempEntity = NULL;
	iVoteID = pVotingPlayer->m_iCurrentKickVote;

	while ((pTempEntity = UTIL_FindEntityByClassname(pTempEntity, "player")) != NULL)
	{
		if (FNullEnt(pTempEntity->edict()))
			break;

		pTempPlayer = GetClassPtr((CBasePlayer *)pTempEntity->pev);

		if (!pTempPlayer || pTempPlayer->m_iTeam == TEAM_UNASSIGNED)
			continue;

		if (pTempPlayer->m_iTeam == pVotingPlayer->m_iTeam && pTempPlayer->m_iCurrentKickVote == iVoteID)
			iValidVotes++;
	}

	if (kick_percent.value < 0)
		CVAR_SET_STRING("mp_kickpercent", "0.0");
	else if (kick_percent.value > 1)
		CVAR_SET_STRING("mp_kickpercent", "1.0");

	iVotesNeeded = (int)(iTeamCount * kick_percent.value + 0.5);
	fKickPercent = iValidVotes >= iVotesNeeded;

	if (fKickPercent)
	{
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "#Game_kicked", STRING(pKickPlayer->pev->netname));
		SERVER_COMMAND(UTIL_VarArgs("kick # %d\n", pVotingPlayer->m_iCurrentKickVote));
		pTempEntity = NULL;

		while ((pTempEntity = UTIL_FindEntityByClassname(pTempEntity, "player")) != NULL)
		{
			if (FNullEnt(pTempEntity->edict()))
				break;

			pTempPlayer = GetClassPtr((CBasePlayer *)pTempEntity->pev);

			if (!pTempPlayer || pTempPlayer->m_iTeam == TEAM_UNASSIGNED)
				continue;

			if (pTempPlayer->m_iTeam == pVotingPlayer->m_iTeam && pTempPlayer->m_iCurrentKickVote == iVoteID)
				pTempPlayer->m_iCurrentKickVote = 0;
		}
	}
}

int SelectDefaultTeam(void)
{
	int team = TEAM_UNASSIGNED;

	if (g_pGameRules->m_iNumTerrorist < g_pGameRules->m_iNumCT)
		team = TEAM_TERRORIST;
	else if (g_pGameRules->m_iNumTerrorist > g_pGameRules->m_iNumCT)
		team = TEAM_CT;
	else if (g_pGameRules->m_iNumCTWins > g_pGameRules->m_iNumTerroristWins)
		team = TEAM_TERRORIST;
	else if (g_pGameRules->m_iNumCTWins < g_pGameRules->m_iNumTerroristWins)
		team = TEAM_CT;
	else
		team = RANDOM_LONG(0, 1) ? TEAM_TERRORIST : TEAM_CT;

	if (g_pGameRules->TeamFull(team))
	{
		if (team == TEAM_TERRORIST)
			team = TEAM_CT;
		else
			team = TEAM_TERRORIST;

		if (g_pGameRules->TeamFull(team))
			return TEAM_UNASSIGNED;
	}

	return team;
}

void CheckStartMoney(void)
{
	if ((int)startmoney.value > STARTMONEY_MAX)
		CVAR_SET_FLOAT("mp_startmoney", STARTMONEY_MAX);
	else if ((int)startmoney.value < STARTMONEY_MIN)
		CVAR_SET_FLOAT("mp_startmoney", STARTMONEY_MIN);
}

void ClientPutInServer(edict_t *pEntity)
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);
	pPlayer->SetCustomDecalFrames(-1);
	pPlayer->SetPrefsFromUserinfo(g_engfuncs.pfnGetInfoKeyBuffer(pEntity));

	if (!g_pGameRules->IsMultiplayer())
	{
		pPlayer->Spawn();
		return;
	}

	pPlayer->m_bNotKilled = true;
	pPlayer->m_iIgnoreGlobalChat = IGNOREMSG_NONE;
	pPlayer->m_iTeamKills = 0;
	pPlayer->m_bJustConnected = true;
	pPlayer->Spawn();
	pPlayer->m_bTeamChanged = false;
	pPlayer->m_iNumSpawns = 0;

	CheckStartMoney();

	pPlayer->m_iAccount = (int)startmoney.value;
	pPlayer->m_fGameHUDInitialized = FALSE;
	pPlayer->m_flDisplayHistory &= ~DHF_ROUND_STARTED;
	pPlayer->pev->flags |= FL_SPECTATOR;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->effects = EF_NODRAW;
	pPlayer->pev->effects |= EF_NOINTERP;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->deadflag = DEAD_DEAD;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->m_iJoiningState = READINGLTEXT;
	pPlayer->m_iTeam = TEAM_UNASSIGNED;
	pPlayer->pev->fixangle = TRUE;
	pPlayer->m_iModelName = CLASS_URBAN;
	pPlayer->m_bContextHelp = true;
	pPlayer->m_bHasNightVision = false;
	pPlayer->m_iHostagesKilled = 0;
	pPlayer->m_iMapVote = 0;
	pPlayer->m_iCurrentKickVote = 0;
	pPlayer->m_fDeadTime = 0;
	pPlayer->has_disconnected = false;
	pPlayer->m_iMenu = Menu_OFF;
	pPlayer->ClearAutoBuyData();
	pPlayer->m_rebuyString = NULL;

	g_engfuncs.pfnSetClientMaxspeed(ENT(pPlayer->pev), 1);
	SET_MODEL(ENT(pPlayer->pev), "models/player.mdl");
	pPlayer->SetThink(NULL);

	CBaseEntity *Target = UTIL_FindEntityByClassname(NULL, "trigger_camera");
	pPlayer->m_pIntroCamera = Target;

	if (g_pGameRules && g_pGameRules->m_bMapHasCameras == 2)
		g_pGameRules->m_bMapHasCameras = pPlayer->m_pIntroCamera != NULL;

	if (pPlayer->m_pIntroCamera)
		Target = UTIL_FindEntityByTargetname(NULL, STRING(pPlayer->m_pIntroCamera->pev->target));

	if (pPlayer->m_pIntroCamera && Target)
	{
		Vector CamAngles = UTIL_VecToAngles((Target->pev->origin - pPlayer->m_pIntroCamera->pev->origin).Normalize());
		CamAngles.x = -CamAngles.x;

		UTIL_SetOrigin(pPlayer->pev, pPlayer->m_pIntroCamera->pev->origin);

		pPlayer->pev->angles = CamAngles;
		pPlayer->pev->v_angle = pev->angles;
		pPlayer->m_fIntroCamTime = gpGlobals->time + 6;
		pPlayer->pev->view_ofs = g_vecZero;
	}
	else
	{
		pPlayer->m_iTeam = TEAM_CT;

		if (g_pGameRules)
			g_pGameRules->GetPlayerSpawnSpot(pPlayer);

		pPlayer->m_iTeam = TEAM_UNASSIGNED;
		pPlayer->pev->v_angle = g_vecZero;
		pPlayer->pev->angles = gpGlobals->v_forward;
	}

	pPlayer->m_iJoiningState = SHOWLTEXT;

	static char sName[128];
	strcpy(sName, STRING(pPlayer->pev->netname));

	for (char *pApersand = sName; pApersand && *pApersand; pApersand++)
	{
		if (*pApersand == '%')
			*pApersand = ' ';
	}

	UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_connected", sName[0] ? sName : "<unconnected>");
}

bool Q_IsValidUChar32(wchar_t uVal)
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return (uVal < 0x110000u) && ((uVal - 0x00D800u) > 0x7FFu) && ((uVal & 0xFFFFu) < 0xFFFEu) && ((uVal - 0x00FDD0u) > 0x1Fu);
}

// Decode one character from a UTF-8 encoded string. Treats 6-byte CESU-8 sequences
// as a single character, as if they were a correctly-encoded 4-byte UTF-8 sequence.
int Q_UTF8ToUChar32(const char *pUTF8_, wchar_t &uValueOut, bool &bErrorOut)
{
	const byte *pUTF8 = (const byte *)pUTF8_;

	int nBytes = 1;
	uint32 uValue = pUTF8[0];
	uint32 uMinValue = 0;

	// 0....... single byte
	if (uValue < 0x80)
		goto decodeFinishedNoCheck;

	// Expecting at least a two-byte sequence with 0xC0 <= first <= 0xF7 (110...... and 11110...)
	if ((uValue - 0xC0u) > 0x37u || (pUTF8[1] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0xC0 << 6) + pUTF8[1] - 0x80;
	nBytes = 2;
	uMinValue = 0x80;

	// 110..... two-byte lead byte
	if (!(uValue & (0x20 << 6)))
		goto decodeFinished;

	// Expecting at least a three-byte sequence
	if ((pUTF8[2] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x20 << 12) + pUTF8[2] - 0x80;
	nBytes = 3;
	uMinValue = 0x800;

	// 1110.... three-byte lead byte
	if (!(uValue & (0x10 << 12)))
		goto decodeFinishedMaybeCESU8;

	// Expecting a four-byte sequence, longest permissible in UTF-8
	if ((pUTF8[3] & 0xC0) != 0x80)
		goto decodeError;

	uValue = (uValue << 6) - (0x10 << 18) + pUTF8[3] - 0x80;
	nBytes = 4;
	uMinValue = 0x10000;

	// 11110... four-byte lead byte. fall through to finished.

decodeFinished:
	if (uValue >= uMinValue && Q_IsValidUChar32(uValue))
	{
	decodeFinishedNoCheck:
		uValueOut = uValue;
		bErrorOut = false;
		return nBytes;
	}
decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;

decodeFinishedMaybeCESU8:
	// Do we have a full UTF-16 surrogate pair that's been UTF-8 encoded afterwards?
	// That is, do we have 0xD800-0xDBFF followed by 0xDC00-0xDFFF? If so, decode it all.
	if ((uValue - 0xD800u) < 0x400u && pUTF8[3] == 0xED && (byte)(pUTF8[4] - 0xB0) < 0x10 && (pUTF8[5] & 0xC0) == 0x80)
	{
		uValue = 0x10000 + ((uValue - 0xD800u) << 10) + ((byte)(pUTF8[4] - 0xB0) << 6) + pUTF8[5] - 0x80;
		nBytes = 6;
		uMinValue = 0x10000;
	}
	goto decodeFinished;
}

bool Q_UnicodeValidate(const char *pUTF8)
{
	bool bError = false;
	wchar_t uVal = 0;
	int nCharSize;

	while (*pUTF8)
	{
		nCharSize = Q_UTF8ToUChar32(pUTF8, uVal, bError);

		if (bError)
			return false;

		if (nCharSize == 6)
			return false;

		pUTF8 += nCharSize;
	}

	return true;
}

#undef CMD_ARGC
#undef CMD_ARGV

int CMD_ARGC(void)
{
	if (UseBotArgs == false)
		return g_engfuncs.pfnCmd_Argc();

	int i = 0;

	while (BotArgs[i])
		i++;

	return i;
}

const char *CMD_ARGV(int i)
{
	if (UseBotArgs == false)
		return g_engfuncs.pfnCmd_Argv(i);

	if (i < 4)
		return BotArgs[i];

	return NULL;
}

CBaseEntity *EntityFromUserID(int userID)
{
	CBaseEntity *pTempEntity = NULL;

	while ((pTempEntity = UTIL_FindEntityByClassname(pTempEntity, "player")) != NULL)
	{
		if (FNullEnt(pTempEntity->edict()))
			break;

		CBasePlayer *pTempPlayer = GetClassPtr((CBasePlayer *)pTempEntity->pev);

		if (pTempPlayer->m_iTeam == TEAM_UNASSIGNED)
			continue;

		if (userID != GETPLAYERUSERID(pTempEntity->edict()))
			continue;

		return pTempPlayer;
	}

	return NULL;
}

void Host_Say(edict_t *pEntity, int teamonly)
{
	CBasePlayer *client;
	int j;
	char *p;
	char text[128];
	char szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = CMD_ARGV(0);
	bool bSenderDead = false;
	char *pszFormat, *pszConsoleFormat;

	entvars_t *pev = &pEntity->v;
	CBasePlayer *player = GetClassPtr((CBasePlayer *)pev);

	if (player->m_flLastTalk != 0 && gpGlobals->time - player->m_flLastTalk < 0.66)
		return;

	player->m_flLastTalk = gpGlobals->time;

	if (player->pev->deadflag != DEAD_NO)
		bSenderDead = true;

	if (CMD_ARGC() == 0)
		return;

	if (!stricmp(pcmd, cpSay) || !strcmp(pcmd, cpSayTeam))
	{
		if (CMD_ARGC() < 2)
			return;

		p = (char *)CMD_ARGS();
	}
	else
	{
		if (CMD_ARGC() >= 2)
			sprintf(szTemp, "%s %s", pcmd, CMD_ARGS());
		else
			sprintf(szTemp, "%s", CMD_ARGS());

		p = szTemp;
	}

	if (p&&*p == '"')
	{
		p++;
		p[strlen(p) - 1] = '\0';
	}

	if (!p || !*p)
		return;

	char *pc = p;

	if (!Q_UnicodeValidate(pc))
		return;

	if (teamonly == TRUE)
	{
		if (player->m_iTeam == TEAM_CT && !bSenderDead)
		{
			pszFormat = "#Cstrike_Chat_CT";
			pszConsoleFormat = "(Counter-Terrorist) %s : %s";
		}
		else if (player->m_iTeam == TEAM_TERRORIST && !bSenderDead)
		{
			pszFormat = "#Cstrike_Chat_T";
			pszConsoleFormat = "(Terrorist) %s : %s";
		}
		else if (player->m_iTeam == TEAM_CT && bSenderDead)
		{
			pszFormat = "#Cstrike_Chat_CT_Dead";
			pszConsoleFormat = "*DEAD*(Counter-Terrorist) %s : %s";
		}
		else if (player->m_iTeam == TEAM_TERRORIST || bSenderDead)
		{
			pszFormat = "#Cstrike_Chat_T_Dead";
			pszConsoleFormat = "*DEAD*(Terrorist) %s : %s";
		}
		else
		{
			pszFormat = "#Cstrike_Chat_Spec";
			pszConsoleFormat = "(Spectator) %s : %s";
		}
	}
	else
	{
		if (bSenderDead)
		{
			if (player->m_iTeam == TEAM_SPECTATOR)
			{
				pszFormat = "#Cstrike_Chat_AllSpec";
				pszConsoleFormat = "*SPEC* %s : %s";
			}
			else
			{
				pszFormat = "#Cstrike_Chat_AllDead";
				pszConsoleFormat = "*DEAD* %s : %s";
			}
		}
		else
		{
			pszFormat = "#Cstrike_Chat_All";
			pszConsoleFormat = "%s : %s";
		}
	}

	text[0] = 0;
	j = sizeof(text) - 3 - strlen(pszFormat);

	if ((int)strlen(p) > j)
		p[j] = '\0';

	for (char *pApersand = p; pApersand && *pApersand; pApersand++)
	{
		if (pApersand[0] == '%' && pApersand[1] == 'l')
		{
			if (pApersand[1] != ' ' && pApersand[1] != '\0')
				pApersand[0] = ' ';
		}
	}

	strcat(text, p);
	strcat(text, "\n");
	client = NULL;

	while ((client = (CBasePlayer *)UTIL_FindEntityByClassname(client, "player")) != NULL)
	{
		if (FNullEnt(client->edict()))
			break;

		if (!client->pev)
			continue;

		if (client->edict() == pEntity)
			continue;

		if (!client->IsNetClient())
			continue;

		if (gpGlobals->deathmatch && g_pGameRules->m_VoiceGameMgr.PlayerHasBlockedPlayer(client, player))
			continue;

		if (teamonly && client->m_iTeam != player->m_iTeam)
			continue;

		if ((client->pev->deadflag != DEAD_NO && !bSenderDead) || (client->pev->deadflag == DEAD_NO && bSenderDead))
		{
			if (!FBitSet(player->pev->flags, FL_PROXY))
				continue;
		}

		if (client->m_iIgnoreGlobalChat == IGNOREMSG_ENEMY)
		{
			if (client->m_iTeam == player->m_iTeam)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, client->pev);
				WRITE_BYTE(ENTINDEX(pEntity));
				WRITE_STRING(pszFormat);
				WRITE_STRING("");
				WRITE_STRING(text);
				MESSAGE_END();
			}
		}
		else if (client->m_iIgnoreGlobalChat == IGNOREMSG_NONE)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, client->pev);
			WRITE_BYTE(ENTINDEX(pEntity));
			WRITE_STRING(pszFormat);
			WRITE_STRING("");
			WRITE_STRING(text);
			MESSAGE_END();
		}
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, VARS(pEntity));
	WRITE_BYTE(ENTINDEX(pEntity));
	WRITE_STRING(pszFormat);
	WRITE_STRING("");
	WRITE_STRING(text);
	MESSAGE_END();

	if (pszConsoleFormat)
		g_engfuncs.pfnServerPrint(UTIL_VarArgs(pszConsoleFormat, STRING(player->pev->netname), text));

	if (CVAR_GET_FLOAT("mp_logmessages") != 0)
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" %s \"%s\"%s\n", STRING(player->pev->netname), GETPLAYERUSERID(player->edict()), GETPLAYERAUTHID(player->edict()), GetTeam(player->m_iTeam), (teamonly != TRUE) ? "say" : "say_team", pc, (player->m_iTeam != TEAM_SPECTATOR && bSenderDead) ? " (dead)" : "");
}

extern WeaponInfoStruct *GetWeaponInfo(int id);
extern void BlinkAccount(CBasePlayer *pPlayer, int time);
extern bool CanBuyWeaponByMaptype(int iTeam, int iWpnID, bool bIsVipMaps);
extern bool BuyAmmo(CBasePlayer *pPlayer, int nSlot, bool bBlinkMoney);
extern void BlinkAccount(CBasePlayer *pPlayer, int numBlinks);
extern const char *BuyAliasToWeaponID(const char *alias, int &wpnId);

void DropPrimary(CBasePlayer *pPlayer)
{
	if (pPlayer->HasShield())
	{
		pPlayer->DropShield(true);
		return;
	}

	while (pPlayer->m_rgpPlayerItems[WPNSLOT_PRIMARY])
		pPlayer->DropPlayerItem(STRING(pPlayer->m_rgpPlayerItems[WPNSLOT_PRIMARY]->pev->classname));
}

void DropPistol(CBasePlayer *pPlayer)
{
	if (pPlayer->HasShield())
		pPlayer->DrawnShiled();

	while (pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY])
		pPlayer->DropPlayerItem(STRING(pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY]->pev->classname));
}

bool CanBuyThis(CBasePlayer *pPlayer, int weaponId)
{
	if ((pPlayer->HasShield() && weaponId == WEAPON_ELITE) || (pPlayer->HasShield() && weaponId == WEAPON_SHIELDGUN))
		return false;

	if (pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY] != NULL && pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY]->m_iId == WEAPON_ELITE && weaponId == WEAPON_SHIELDGUN)
		return false;

	if (pPlayer->m_rgpPlayerItems[WPNSLOT_PRIMARY] && pPlayer->m_rgpPlayerItems[WPNSLOT_PRIMARY]->m_iId == weaponId)
	{
		if (g_bClientPrintEnable)
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cstrike_Already_Own_Weapon");

		return false;
	}

	if (pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY] && pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY]->m_iId == weaponId)
	{
		if (g_bClientPrintEnable)
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cstrike_Already_Own_Weapon");

		return false;
	}

	if (!CanBuyWeaponByMaptype(pPlayer->m_iTeam, weaponId, g_pGameRules->m_iMapHasVIPSafetyZone == 1))
	{
		if (g_bClientPrintEnable)
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Buy_This");

		return false;
	}

	return true;
}

bool CanBuyWeaponByMaptype(int iTeam, int iWpnID, bool bIsVipMaps)
{
	if (bIsVipMaps)
	{
		if (iTeam == TEAM_CT)
		{
			switch (iWpnID)
			{
				case WEAPON_P228:
				case WEAPON_XM1014:
				case WEAPON_AUG:
				case WEAPON_FIVESEVEN:
				case WEAPON_UMP45:
				case WEAPON_SG550:
				case WEAPON_FAMAS:
				case WEAPON_USP:
				case WEAPON_GLOCK18:
				case WEAPON_MP5N:
				case WEAPON_M249:
				case WEAPON_M3:
				case WEAPON_M4A1:
				case WEAPON_TMP:
				case WEAPON_DEAGLE:
				case WEAPON_P90:
				case WEAPON_SHIELDGUN: return true;

				default: return false;
			}
		}
		else if (iTeam == TEAM_TERRORIST)
		{
			switch (iWpnID)
			{
				case WEAPON_P228:
				case WEAPON_MAC10:
				case WEAPON_ELITE:
				case WEAPON_UMP45:
				case WEAPON_GALIL:
				case WEAPON_USP:
				case WEAPON_GLOCK18:
				case WEAPON_AWP:
				case WEAPON_DEAGLE:
				case WEAPON_AK47: return true;

				default: return false;
			}
		}

		return false;
	}

	if (iTeam == TEAM_CT)
	{
		switch (iWpnID)
		{
			case WEAPON_P228:
			case WEAPON_SCOUT:
			case WEAPON_XM1014:
			case WEAPON_AUG:
			case WEAPON_FIVESEVEN:
			case WEAPON_UMP45:
			case WEAPON_SG550:
			case WEAPON_FAMAS:
			case WEAPON_USP:
			case WEAPON_GLOCK18:
			case WEAPON_AWP:
			case WEAPON_MP5N:
			case WEAPON_M249:
			case WEAPON_M3:
			case WEAPON_M4A1:
			case WEAPON_TMP:
			case WEAPON_DEAGLE:
			case WEAPON_P90:
			case WEAPON_SHIELDGUN: return true;

			default: return false;
		}
	}
	else if (iTeam == TEAM_TERRORIST)
	{
		switch (iWpnID)
		{
			case WEAPON_P228:
			case WEAPON_SCOUT:
			case WEAPON_XM1014:
			case WEAPON_MAC10:
			case WEAPON_ELITE:
			case WEAPON_UMP45:
			case WEAPON_GALIL:
			case WEAPON_USP:
			case WEAPON_GLOCK18:
			case WEAPON_AWP:
			case WEAPON_MP5N:
			case WEAPON_M249:
			case WEAPON_M3:
			case WEAPON_G3SG1:
			case WEAPON_DEAGLE:
			case WEAPON_SG552:
			case WEAPON_AK47:
			case WEAPON_P90: return true;

			default: return false;
		}
	}

	return false;
}

bool IsPrimaryWeapon(int iWpnID)
{
	switch (iWpnID)
	{
		case WEAPON_SCOUT:
		case WEAPON_XM1014:
		case WEAPON_MAC10:
		case WEAPON_AUG:
		case WEAPON_UMP45:
		case WEAPON_SG550:
		case WEAPON_GALIL:
		case WEAPON_FAMAS:
		case WEAPON_AWP:
		case WEAPON_MP5N:
		case WEAPON_M249:
		case WEAPON_M3:
		case WEAPON_M4A1:
		case WEAPON_TMP:
		case WEAPON_G3SG1:
		case WEAPON_SG552:
		case WEAPON_AK47:
		case WEAPON_P90:
		case WEAPON_SHIELDGUN: return true;

		default: return false;
	}
}

bool IsSecondaryWeapon(int iWpnID)
{
	switch (iWpnID)
	{
		case WEAPON_P228:
		case WEAPON_ELITE:
		case WEAPON_FIVESEVEN:
		case WEAPON_USP:
		case WEAPON_GLOCK18:
		case WEAPON_DEAGLE: return true;

		default: return false;
	}
}

void Buy(CBasePlayer *pPlayer, int iSlot)
{
	switch (iSlot)
	{
		case 1:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
				ShowVGUIMenu(pPlayer, MENU_BUY_PISTOL, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_0, "#CT_BuyPistol");
			else
				ShowVGUIMenu(pPlayer, MENU_BUY_PISTOL, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_0, "#T_BuyPistol");

			pPlayer->m_iMenu = Menu_BuyPistol;
			break;
		}

		case 2:
		{
			if (g_pGameRules->m_iMapHasVIPSafetyZone == 1 && pPlayer->m_iTeam == TEAM_TERRORIST)
				ShowVGUIMenu(pPlayer, MENU_BUY_SHOTGUN, MENU_KEY_0, "#AS_BuyShotgun");
			else
				ShowVGUIMenu(pPlayer, MENU_BUY_SHOTGUN, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_0, "#BuyShotgun");

			pPlayer->m_iMenu = Menu_BuyShotgun;
			break;
		}

		case 3:
		{
			if (g_pGameRules->m_iMapHasVIPSafetyZone == 1)
			{
				if (pPlayer->m_iTeam == TEAM_CT)
					ShowVGUIMenu(pPlayer, MENU_BUY_SUBMACHINEGUN, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_0, "#AS_CT_BuySubMachineGun");
				else
					ShowVGUIMenu(pPlayer, MENU_BUY_SUBMACHINEGUN, MENU_KEY_1 | MENU_KEY_3 | MENU_KEY_0, "#AS_T_BuySubMachineGun");
			}
			else
			{
				if (pPlayer->m_iTeam == TEAM_CT)
					ShowVGUIMenu(pPlayer, MENU_BUY_SUBMACHINEGUN, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_0, "#CT_BuySubMachineGun");
				else
					ShowVGUIMenu(pPlayer, MENU_BUY_SUBMACHINEGUN, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_0, "#T_BuySubMachineGun");
			}

			pPlayer->m_iMenu = Menu_BuySubMachineGun;
			break;
		}

		case 4:
		{
			if (g_pGameRules->m_iMapHasVIPSafetyZone == 1)
			{
				if (pPlayer->m_iTeam == TEAM_CT)
					ShowVGUIMenu(pPlayer, MENU_BUY_RIFLE, MENU_KEY_1 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_0, "#AS_CT_BuyRifle");
				else
					ShowVGUIMenu(pPlayer, MENU_BUY_RIFLE, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5 | MENU_KEY_0, "#AS_T_BuyRifle");
			}
			else
			{
				if (pPlayer->m_iTeam == TEAM_CT)
					ShowVGUIMenu(pPlayer, MENU_BUY_RIFLE, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#CT_BuyRifle");
				else
					ShowVGUIMenu(pPlayer, MENU_BUY_RIFLE, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#T_BuyRifle");
			}

			pPlayer->m_iMenu = Menu_BuyRifle;
			break;
		}

		case 5:
		{
			if (g_pGameRules->m_iMapHasVIPSafetyZone == 1 && pPlayer->m_iTeam == TEAM_TERRORIST)
				ShowVGUIMenu(pPlayer, MENU_BUY_MACHINEGUN, MENU_KEY_0, "#AS_T_BuyMachineGun");
			else
				ShowVGUIMenu(pPlayer, MENU_BUY_MACHINEGUN, MENU_KEY_1 | MENU_KEY_0, "#BuyMachineGun");

			pPlayer->m_iMenu = Menu_BuyMachineGun;
			break;
		}

		case 6:
		{
			if (!(pPlayer->m_signals.GetState() & SIGNAL_BUY))
				break;

			if (BuyAmmo(pPlayer, WPNSLOT_PRIMARY, true))
				while (BuyAmmo(pPlayer, WPNSLOT_PRIMARY, false));

			pPlayer->BuildRebuyStruct();
			break;
		}

		case 7:
		{
			if (!(pPlayer->m_signals.GetState() & SIGNAL_BUY))
				break;

			if (BuyAmmo(pPlayer, WPNSLOT_SECONDARY, true))
				while (BuyAmmo(pPlayer, WPNSLOT_SECONDARY, false));

			pPlayer->BuildRebuyStruct();
			break;
		}

		case 8:
		{
			if (!(pPlayer->m_signals.GetState() & SIGNAL_BUY))
				break;

			if (g_pGameRules->m_bMapHasBombTarget == true)
			{
				if (pPlayer->m_iTeam == TEAM_CT)
					ShowVGUIMenu(pPlayer, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_7 | MENU_KEY_8 | MENU_KEY_0, "#DCT_BuyItem");
				else
					ShowVGUIMenu(pPlayer, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#DT_BuyItem");
			}
			else
			{
				if (pPlayer->m_iTeam == TEAM_CT)
					ShowVGUIMenu(pPlayer, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_8 | MENU_KEY_0, "#CT_BuyItem");
				else
					ShowVGUIMenu(pPlayer, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#T_BuyItem");
			}

			pPlayer->m_iMenu = Menu_BuyItem;
			break;
		}
	}
}

void BuyPistol(CBasePlayer *pPlayer, int iSlot)
{
	int iWeapon = 0;
	int iWeaponPrice = 0;
	const char *pszWeapon;

	if (!pPlayer->CanPlayerBuy(true))
		return;

	if (iSlot < 1 || iSlot > 5)
		return;

	switch (iSlot)
	{
		case 1:
		{
			iWeapon = WEAPON_GLOCK18;
			iWeaponPrice = GLOCK18_PRICE;
			pszWeapon = "weapon_glock18";
			break;
		}

		case 2:
		{
			iWeapon = WEAPON_USP;
			iWeaponPrice = USP_PRICE;
			pszWeapon = "weapon_usp";
			break;
		}

		case 3:
		{
			iWeapon = WEAPON_P228;
			iWeaponPrice = P228_PRICE;
			pszWeapon = "weapon_p228";
			break;
		}

		case 4:
		{
			iWeapon = WEAPON_DEAGLE;
			iWeaponPrice = DEAGLE_PRICE;
			pszWeapon = "weapon_deagle";
			break;
		}

		case 5:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_FIVESEVEN;
				iWeaponPrice = FIVESEVEN_PRICE;
				pszWeapon = "weapon_fiveseven";
			}
			else
			{
				iWeapon = WEAPON_ELITE;
				iWeaponPrice = ELITE_PRICE;
				pszWeapon = "weapon_elite";
			}

			break;
		}

		default: pszWeapon = NULL;
	}

	if (!CanBuyThis(pPlayer, iWeapon))
		return;

	if (pPlayer->m_iAccount < iWeaponPrice)
	{
		if (g_bClientPrintEnable)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
			BlinkAccount(pPlayer, 2);
		}

		return;
	}

	DropPistol(pPlayer);

	pPlayer->GiveNamedItem(pszWeapon);
	pPlayer->AddAccount(-iWeaponPrice);
}

void BuyShotgun(CBasePlayer *pPlayer, int iSlot)
{
	int iWeapon = 0;
	int iWeaponPrice = 0;
	const char *pszWeapon;

	if (!pPlayer->CanPlayerBuy(true))
		return;

	if (iSlot < 1 || iSlot > 2)
		return;

	switch (iSlot)
	{
		case 1:
		{
			iWeapon = WEAPON_M3;
			iWeaponPrice = M3_PRICE;
			pszWeapon = "weapon_m3";
			break;
		}

		case 2:
		{
			iWeapon = WEAPON_XM1014;
			iWeaponPrice = XM1014_PRICE;
			pszWeapon = "weapon_xm1014";
			break;
		}

		default: pszWeapon = NULL;
	}

	if (!CanBuyThis(pPlayer, iWeapon))
		return;

	if (pPlayer->m_iAccount < iWeaponPrice)
	{
		if (g_bClientPrintEnable)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
			BlinkAccount(pPlayer, 2);
		}

		return;
	}

	DropPrimary(pPlayer);

	pPlayer->GiveNamedItem(pszWeapon);
	pPlayer->AddAccount(-iWeaponPrice);
}

void BuySubMachineGun(CBasePlayer *pPlayer, int iSlot)
{
	int iWeapon = 0;
	int iWeaponPrice = 0;
	const char *pszWeapon;

	if (!pPlayer->CanPlayerBuy(true))
		return;

	if (iSlot < 1 || iSlot > 4)
		return;

	switch (iSlot)
	{
		case 1:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_TMP;
				iWeaponPrice = TMP_PRICE;
				pszWeapon = "weapon_tmp";
			}
			else
			{
				iWeapon = WEAPON_MAC10;
				iWeaponPrice = MAC10_PRICE;
				pszWeapon = "weapon_mac10";
			}

			break;
		}

		case 2:
		{
			iWeapon = WEAPON_MP5N;
			iWeaponPrice = MP5NAVY_PRICE;
			pszWeapon = "weapon_mp5navy";
			break;
		}

		case 3:
		{
			iWeapon = WEAPON_UMP45;
			iWeaponPrice = UMP45_PRICE;
			pszWeapon = "weapon_mp5navy";
			break;
		}

		case 4:
		{
			iWeapon = WEAPON_P90;
			iWeaponPrice = P90_PRICE;
			pszWeapon = "weapon_p90";
			break;
		}

		default: pszWeapon = NULL;
	}

	if (!CanBuyThis(pPlayer, iWeapon))
		return;

	if (pPlayer->m_iAccount < iWeaponPrice)
	{
		if (g_bClientPrintEnable)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
			BlinkAccount(pPlayer, 2);
		}

		return;
	}

	DropPrimary(pPlayer);

	pPlayer->GiveNamedItem(pszWeapon);
	pPlayer->AddAccount(-iWeaponPrice);
}

void BuyWeaponByWeaponID(CBasePlayer *pPlayer, int weaponID)
{
	if (!pPlayer->CanPlayerBuy(true))
		return;

	if (!CanBuyThis(pPlayer, weaponID))
		return;

	WeaponInfoStruct *info = GetWeaponInfo(weaponID);

	if (!info || !info->entityName)
		return;

	if (pPlayer->m_iAccount < info->cost)
	{
		if (g_bClientPrintEnable)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
			BlinkAccount(pPlayer, 2);
		}

		return;
	}

	if (IsPrimaryWeapon(weaponID))
			DropPrimary(pPlayer);
	else
			DropPistol(pPlayer);

	pPlayer->GiveNamedItem(info->entityName);
	pPlayer->AddAccount(-info->cost);
}

void BuyRifle(CBasePlayer *pPlayer, int iSlot)
{
	int iWeapon = 0;
	int iWeaponPrice = 0;
	const char *pszWeapon;

	if (!pPlayer->CanPlayerBuy(true))
		return;

	if (iSlot < 1 || iSlot > 6)
		return;

	switch (iSlot)
	{
		default:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_FAMAS;
				iWeaponPrice = FAMAS_PRICE;
				pszWeapon = "weapon_famas";
			}
			else
			{
				iWeapon = WEAPON_GALIL;
				iWeaponPrice = GALIL_PRICE;
				pszWeapon = "weapon_galil";
			}

			break;
		}

		case 2:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_AK47;
				iWeaponPrice = AK47_PRICE;
				pszWeapon = "weapon_ak47";
			}
			else
			{
				iWeapon = WEAPON_SCOUT;
				iWeaponPrice = SCOUT_PRICE;
				pszWeapon = "weapon_scout";
			}

			break;
		}

		case 3:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_M4A1;
				iWeaponPrice = M4A1_PRICE;
				pszWeapon = "weapon_m4a1";
			}
			else
			{
				iWeapon = WEAPON_SCOUT;
				iWeaponPrice = SCOUT_PRICE;
				pszWeapon = "weapon_scout";
			}

			break;
		}

		case 4:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_AUG;
				iWeaponPrice = AUG_PRICE;
				pszWeapon = "weapon_aug";
			}
			else
			{
				iWeapon = WEAPON_SG552;
				iWeaponPrice = SG552_PRICE;
				pszWeapon = "weapon_sg552";
			}

			break;
		}

		case 5:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_SG550;
				iWeaponPrice = SG550_PRICE;
				pszWeapon = "weapon_sg550";
			}
			else
			{
				iWeapon = WEAPON_AWP;
				iWeaponPrice = AWP_PRICE;
				pszWeapon = "weapon_awp";
			}

			break;
		}

		case 6:
		{
			if (pPlayer->m_iTeam == TEAM_CT)
			{
				iWeapon = WEAPON_AWP;
				iWeaponPrice = AWP_PRICE;
				pszWeapon = "weapon_awp";
			}
			else
			{
				iWeapon = WEAPON_G3SG1;
				iWeaponPrice = G3SG1_PRICE;
				pszWeapon = "weapon_g3sg1";
			}

			break;
		}
	}

	if (!CanBuyThis(pPlayer, iWeapon))
		return;

	if (pPlayer->m_iAccount < iWeaponPrice)
	{
		if (g_bClientPrintEnable)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
			BlinkAccount(pPlayer, 2);
		}

		return;
	}

	DropPrimary(pPlayer);

	pPlayer->GiveNamedItem(pszWeapon);
	pPlayer->AddAccount(-iWeaponPrice);
}

void BuyMachineGun(CBasePlayer *pPlayer, int iSlot)
{
	int iWeapon = 0;
	int iWeaponPrice = 0;
	const char *pszWeapon;

	if (!pPlayer->CanPlayerBuy(true))
		return;

	if (iSlot != 1)
		return;

	iWeapon = WEAPON_M249;
	iWeaponPrice = M249_PRICE;
	pszWeapon = "weapon_m249";

	if (!CanBuyThis(pPlayer, iWeapon))
		return;

	if (pPlayer->m_iAccount < iWeaponPrice)
	{
		if (g_bClientPrintEnable)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
			BlinkAccount(pPlayer, 2);
		}

		return;
	}

	DropPrimary(pPlayer);

	pPlayer->GiveNamedItem(pszWeapon);
	pPlayer->AddAccount(-iWeaponPrice);
}

extern int gmsgStatusIcon;

void BuyItem(CBasePlayer *pPlayer, int iSlot)
{
	int iItemPrice = 0;
	const char *pszItem = NULL;

	if (!pPlayer->CanPlayerBuy(true))
		return;

	if (pPlayer->m_iTeam == TEAM_CT)
	{
		if (iSlot < 1 || iSlot > 8)
			return;
	}
	else
	{
		if (iSlot < 1 || iSlot > 6)
			return;
	}

	switch (iSlot)
	{
		case 1:
		{
			if (pPlayer->pev->armorvalue >= 100)
			{
				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Kevlar");

				break;
			}

			if (pPlayer->m_iAccount < KEVLAR_PRICE)
			{
				if (g_bClientPrintEnable)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
					BlinkAccount(pPlayer, 2);
				}
			}
			else
			{
				if (pPlayer->m_iKevlar == 2)
				{
					if (g_bClientPrintEnable)
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Helmet_Bought_Kevlar");
				}

				pPlayer->GiveNamedItem("item_kevlar");
				pPlayer->AddAccount(-KEVLAR_PRICE);
			}

			break;
		}

		case 2:
		{
			if (pPlayer->pev->armorvalue >= 100)
			{
				if (pPlayer->m_iKevlar == 2)
				{
					if (g_bClientPrintEnable)
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Kevlar_Helmet");

					break;
				}

				if (pPlayer->m_iAccount < HELMET_PRICE)
				{
					if (g_bClientPrintEnable)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
						BlinkAccount(pPlayer, 2);
					}
				}
				else
				{
					if (g_bClientPrintEnable)
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Kevlar_Bought_Helmet");

					pPlayer->GiveNamedItem("item_assaultsuit");
					pPlayer->AddAccount(-HELMET_PRICE);
				}
			}
			else
			{
				if (pPlayer->m_iKevlar == 2)
				{
					if (pPlayer->m_iAccount < KEVLAR_PRICE)
					{
						if (g_bClientPrintEnable)
						{
							ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
							BlinkAccount(pPlayer, 2);
						}
					}
					else
					{
						if (g_bClientPrintEnable)
							ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Helmet_Bought_Kevlar");

						pPlayer->GiveNamedItem("item_assaultsuit");
						pPlayer->AddAccount(-KEVLAR_PRICE);
					}
				}
				else
				{
					if (pPlayer->m_iAccount < ASSAULTSUIT_PRICE)
					{
						if (g_bClientPrintEnable)
						{
							ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
							BlinkAccount(pPlayer, 2);
						}
					}
					else
					{
						pPlayer->GiveNamedItem("item_assaultsuit");
						pPlayer->AddAccount(-ASSAULTSUIT_PRICE);
					}
				}
			}

			break;
		}

		case 3:
		{
			if (pPlayer->AmmoInventory(pPlayer->GetAmmoIndex("Flashbang")) >= 2)
			{
				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Carry_Anymore");

				break;
			}

			if (pPlayer->m_iAccount < FLASHBANG_PRICE)
			{
				if (g_bClientPrintEnable)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
					BlinkAccount(pPlayer, 2);
				}
			}
			else
			{
				pPlayer->GiveNamedItem("weapon_flashbang");
				pPlayer->AddAccount(-FLASHBANG_PRICE);
			}

			break;
		}

		case 4:
		{
			if (pPlayer->AmmoInventory(pPlayer->GetAmmoIndex("HEGrenade")) >= 1)
			{
				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Carry_Anymore");

				break;
			}

			if (pPlayer->m_iAccount < HEGRENADE_PRICE)
			{
				if (g_bClientPrintEnable)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
					BlinkAccount(pPlayer, 2);
				}
			}
			else
			{
				pPlayer->GiveNamedItem("weapon_hegrenade");
				pPlayer->AddAccount(-HEGRENADE_PRICE);
			}

			break;
		}

		case 5:
		{
			if (pPlayer->AmmoInventory(pPlayer->GetAmmoIndex("SmokeGrenade")) >= 1)
			{
				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Carry_Anymore");

				break;
			}

			if (pPlayer->m_iAccount < SMOKEGRENADE_PRICE)
			{
				if (g_bClientPrintEnable)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
					BlinkAccount(pPlayer, 2);
				}
			}
			else
			{
				pPlayer->GiveNamedItem("weapon_smokegrenade");
				pPlayer->AddAccount(-SMOKEGRENADE_PRICE);
			}

			break;
		}

		case 6:
		{
			if (pPlayer->m_bHasNightVision == true)
			{
				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_One");

				break;
			}

			if (pPlayer->m_iAccount < NVG_PRICE)
			{
				if (g_bClientPrintEnable)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
					BlinkAccount(pPlayer, 2);
				}
			}
			else
			{
				if (!(pPlayer->m_flDisplayHistory & DHF_NIGHTVISION))
				{
					pPlayer->HintMessage("#Hint_use_nightvision");
					pPlayer->m_flDisplayHistory |= DHF_NIGHTVISION;
				}

				EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/equip_nvg.wav", VOL_NORM, ATTN_NORM);

				pPlayer->m_bHasNightVision = true;
				pPlayer->AddAccount(-NVG_PRICE);

				SendItemStatus(pPlayer);
			}

			break;
		}

		case 7:
		{
			if (pPlayer->m_iTeam != TEAM_CT || g_pGameRules->m_bMapHasBombTarget != true)
				break;

			if (pPlayer->m_bHasDefuser == true)
			{
				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_One");

				break;
			}

			if (pPlayer->m_iAccount < DEFUSEKIT_PRICE)
			{
				if (g_bClientPrintEnable)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
					BlinkAccount(pPlayer, 2);
				}
			}
			else
			{
				pPlayer->m_bHasDefuser = true;

				MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev);
				WRITE_BYTE(STATUSICON_SHOW);
				WRITE_STRING("defuser");
				WRITE_BYTE(0);
				WRITE_BYTE(160);
				WRITE_BYTE(0);
				MESSAGE_END();

				pPlayer->pev->body = 1;
				pPlayer->AddAccount(-DEFUSEKIT_PRICE);

				EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/kevlar.wav", VOL_NORM, ATTN_NORM);
				SendItemStatus(pPlayer);
			}

			break;
		}

		case 8:
		{
			if (!CanBuyThis(pPlayer, WEAPON_SHIELDGUN))
				break;

			if (pPlayer->m_iAccount < SHIELDGUN_PRICE)
			{
				if (g_bClientPrintEnable)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
					BlinkAccount(pPlayer, 2);
				}
			}
			else
			{
				DropPrimary(pPlayer);

				pPlayer->GiveShield(true);
				pPlayer->AddAccount(-SHIELDGUN_PRICE);

				EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);
			}

			break;
		}
	}
}

char *sPlayerModelFiles[] =
{
	"models/player.mdl",
	"models/player/leet/leet.mdl",
	"models/player/gign/gign.mdl",
	"models/player/vip/vip.mdl",
	"models/player/gsg9/gsg9.mdl",
	"models/player/guerilla/guerilla.mdl",
	"models/player/arctic/arctic.mdl",
	"models/player/sas/sas.mdl",
	"models/player/terror/terror.mdl",
	"models/player/urban/urban.mdl"
};

#define PLAYERMODEL_COUNT (sizeof(sPlayerModelFiles) / sizeof(sPlayerModelFiles[0]))

void HandleMenu_ChooseAppearance(CBasePlayer *player, int slot)
{
	char *appearance;
	int modelName;
	int numSkins;

	if (player->m_iTeam == TEAM_TERRORIST)
	{
		if (slot > 4)
			slot = RANDOM_LONG(1, 4);

		switch (slot)
		{
			default:
			case 1:
			{
				modelName = MODEL_TERROR;
				appearance = "terror";
				numSkins = 8;
				break;
			}

			case 2:
			{
				modelName = MODEL_LEET;
				appearance = "leet";
				numSkins = 8;
				break;
			}

			case 3:
			{
				modelName = MODEL_ARCTIC;
				appearance = "arctic";
				numSkins = 8;
				break;
			}

			case 4:
			{
				modelName = MODEL_GUERILLA;
				appearance = "guerilla";
				numSkins = 8;
				break;
			}
		}
	}
	else if (player->m_iTeam == TEAM_CT)
	{
		if (slot > 4)
			slot = RANDOM_LONG(1, 4);

		switch (slot)
		{
			default:
			case 1:
			{
				modelName = MODEL_URBAN;
				appearance = "urban";
				numSkins = 9;
				break;
			}

			case 2:
			{
				modelName = MODEL_GSG9;
				appearance = "gsg9";
				numSkins = 9;
				break;
			}

			case 3:
			{
				modelName = MODEL_SAS;
				appearance = "sas";
				numSkins = 9;
				break;
			}

			case 4:
			{
				modelName = MODEL_GIGN;
				appearance = "gign";
				numSkins = 9;
				break;
			}
		}
	}

	player->m_iMenu = Menu_OFF;

	if (player->m_iJoiningState != JOINED)
	{
		if (player->m_iJoiningState == PICKINGTEAM)
		{
			player->m_iJoiningState = GETINTOGAME;

			if (g_pGameRules->IsCareer())
			{
				if (!player->IsBot())
					g_pGameRules->CheckWinConditions();
			}
		}
	}
	else
		g_pGameRules->CheckWinConditions();

	player->pev->body = 0;
	player->m_iModelName = modelName;
	g_engfuncs.pfnSetClientKeyValue(player->entindex(), g_engfuncs.pfnGetInfoKeyBuffer(ENT(player->pev)), "model", appearance);
	player->SetNewPlayerModel(sPlayerModelFiles[numSkins]);

	if (!g_pGameRules->m_iMapHasVIPSafetyZone)
	{
		if ((UTIL_FindEntityByClassname(NULL, "func_vip_safetyzone")) != NULL)
			g_pGameRules->m_iMapHasVIPSafetyZone = 1;
		else
			g_pGameRules->m_iMapHasVIPSafetyZone = 2;
	}

	if (g_pGameRules->m_iMapHasVIPSafetyZone == 1)
	{
		if (!g_pGameRules->m_pVIP && player->m_iTeam == TEAM_CT)
			player->MakeVIP();
	}
}

extern int gmsgMoney;
extern int gmsgScoreInfo;
extern int gmsgTeamInfo;
extern int gmsgSpectator;

BOOL HandleMenu_ChooseTeam(CBasePlayer *pPlayer, int slot)
{
	int team;
	int oldTeam;

	if (pPlayer->m_bIsVIP == true)
	{
		if (pPlayer->pev->deadflag == DEAD_NO)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Switch_From_VIP");
			CLIENT_COMMAND(ENT(pPlayer->pev), "slot10\n");
			return TRUE;
		}

		if (g_pGameRules->IsVIPQueueEmpty() != FALSE)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Switch_From_VIP");
			CLIENT_COMMAND(ENT(pPlayer->pev), "slot10\n");
			return TRUE;
		}
	}

	switch (slot)
	{
		default: return FALSE;

		case 1:
		{
			team = TEAM_TERRORIST;
			break;
		}

		case 2:
		{
			team = TEAM_CT;
			break;
		}

		case 3:
		{
			if (g_pGameRules->m_iMapHasVIPSafetyZone != 1 || pPlayer->m_iTeam != TEAM_CT)
				return FALSE;

			g_pGameRules->AddToVIPQueue(pPlayer);
			CLIENT_COMMAND(ENT(pPlayer->pev), "slot10\n");
			return TRUE;
		}

		case 5:
		{
			team = SelectDefaultTeam();

			if (team == TEAM_UNASSIGNED)
			{
				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#All_Teams_Full");
				return FALSE;
			}

			break;
		}

		case 6:
		{
			if (!allow_spectators.value)
			{
				if (!FBitSet(pPlayer->pev->flags, FL_PROXY))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Be_Spectator");
					CLIENT_COMMAND(ENT(pPlayer->pev), "slot10\n");
					return FALSE;
				}
			}

			if (pPlayer->m_iTeam == TEAM_SPECTATOR)
				return TRUE;

			if (!g_pGameRules->IsFreezePeriod())
			{
				if (pPlayer->pev->deadflag == DEAD_NO)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Be_Spectator");
					CLIENT_COMMAND(ENT(pPlayer->pev), "slot10\n");
					return FALSE;
				}
			}

			if (pPlayer->m_iTeam != TEAM_UNASSIGNED)
			{
				if (pPlayer->pev->deadflag == DEAD_NO)
					ClientKill(pPlayer->edict());
			}

			pPlayer->RemoveAllItems(TRUE);
			pPlayer->m_bHasC4 = false;

			if (pPlayer->m_iTeam != TEAM_SPECTATOR)
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" joined team \"SPECTATOR\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), GetTeam(pPlayer->m_iTeam));

			pPlayer->m_iTeam = TEAM_SPECTATOR;
			pPlayer->m_iJoiningState = JOINED;
			pPlayer->m_iAccount = 0;

			MESSAGE_BEGIN(MSG_ONE, gmsgMoney, NULL, pPlayer->pev);
			WRITE_LONG(pPlayer->m_iAccount);
			WRITE_BYTE(0);
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_BROADCAST, gmsgScoreInfo);
			WRITE_BYTE(ENTINDEX(pPlayer->edict()));
			WRITE_SHORT((int)pPlayer->pev->frags);
			WRITE_SHORT(pPlayer->m_iDeaths);
			WRITE_SHORT(0);
			WRITE_SHORT(0);
			MESSAGE_END();

			pPlayer->m_pIntroCamera = NULL;
			pPlayer->m_bTeamChanged = true;

			MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
			WRITE_BYTE(ENTINDEX(pPlayer->edict()));
			WRITE_STRING(GetTeam(pPlayer->m_iTeam));
			MESSAGE_END();

			if (pPlayer->m_iTeam != TEAM_UNASSIGNED)
				pPlayer->SetScoreboardAttributes();

			edict_t *pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot(pPlayer);
			pPlayer->StartObserver(VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles);

			MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
			WRITE_BYTE(ENTINDEX(pPlayer->edict()));
			WRITE_BYTE(1);
			MESSAGE_END();

			if (fadetoblack.value)
				UTIL_ScreenFade(pPlayer, Vector(0, 0, 0), 0.001, 0, 0, 0);

			return TRUE;
		}
	}

	if (g_pGameRules->TeamFull(team))
	{
		if (team == TEAM_TERRORIST)
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Terrorists_Full");
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#CTs_Full");

		return FALSE;
	}

	if (g_pGameRules->TeamStacked(team, pPlayer->m_iTeam))
	{
		if (team == TEAM_TERRORIST)
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Too_Many_Terrorists");
		else
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Too_Many_CTs");

		return FALSE;
	}

	if (!pPlayer->IsBot() && team != TEAM_SPECTATOR)
	{
		if (!stricmp(humans_join_team.string, "CT"))
		{
			//if (team != TEAM_CT)
			{
				if (team == TEAM_CT)//WTF?!
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Humans_Join_Team_CT");
				else
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Humans_Join_Team_T");

				return FALSE;
			}
		}
		else if (!stricmp(humans_join_team.string, "T"))
		{
			if (team != TEAM_TERRORIST)
			{
				if (team == TEAM_CT)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Humans_Join_Team_CT");
				else
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Humans_Join_Team_T");

				return FALSE;
			}
		}
	}

	if (pPlayer->m_bTeamChanged)
	{
		if (pPlayer->pev->deadflag != DEAD_NO)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Only_1_Team_Change");
			return FALSE;
		}
	}

	if (pPlayer->m_iTeam == TEAM_SPECTATOR && team != TEAM_SPECTATOR)
	{
		pPlayer->m_bNotKilled = true;
		pPlayer->m_iIgnoreGlobalChat = IGNOREMSG_NONE;
		pPlayer->m_iTeamKills = 0;

		CheckStartMoney();

		pPlayer->m_iAccount = (int)startmoney.value;
		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->movetype = MOVETYPE_NOCLIP;
		pPlayer->pev->effects = EF_NODRAW;
		pPlayer->pev->effects |= EF_NOINTERP;
		pPlayer->pev->takedamage = DAMAGE_NO;
		pPlayer->pev->deadflag = DEAD_DEAD;
		pPlayer->pev->velocity = g_vecZero;
		pPlayer->pev->punchangle = g_vecZero;
		pPlayer->m_bHasNightVision = false;
		pPlayer->m_iHostagesKilled = 0;
		pPlayer->m_fDeadTime = 0;
		pPlayer->has_disconnected = false;
		pPlayer->m_iJoiningState = GETINTOGAME;

		SendItemStatus(pPlayer);
		g_engfuncs.pfnSetClientMaxspeed(ENT(pPlayer->pev), 1);
		SET_MODEL(ENT(pPlayer->pev), "models/player.mdl");
	}

	if (!g_pGameRules->IsCareer())
	{
		if (team == TEAM_TERRORIST)
			ShowVGUIMenu(pPlayer, MENU_CLASS_T, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5, "#Terrorist_Select");
		else if (team == TEAM_CT)
			ShowVGUIMenu(pPlayer, MENU_CLASS_CT, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5, "#CT_Select");
	}

	pPlayer->m_iMenu = Menu_ChooseAppearance;

	if (pPlayer->pev->deadflag == DEAD_NO)
		ClientKill(pPlayer->edict());

	pPlayer->m_bTeamChanged = true;
	oldTeam = pPlayer->m_iTeam;
	pPlayer->m_iTeam = team;

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(ENTINDEX(pPlayer->edict()));
	WRITE_STRING(GetTeam(team));
	MESSAGE_END();

	if (team != TEAM_UNASSIGNED)
		pPlayer->SetScoreboardAttributes();

	const char *name = pPlayer->pev->netname ? STRING(pPlayer->pev->netname) : "<unconnected>";

	if (!name[0])
		name = "<unconnected>";

	if (team == TEAM_TERRORIST)
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_join_terrorist", name);
	else
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_join_ct", name);

	UTIL_LogPrintf("\"%s<%i><%s><%s>\" joined team \"%s\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), GetTeam(oldTeam), GetTeam(team));
	return TRUE;
}

void Radio1(CBasePlayer *player, int slot)
{
	if (gpGlobals->time <= player->m_flRadioTime)
		return;

	if (player->m_iRadioMessages <= 0)
		return;

	player->m_iRadioMessages--;
	player->m_flRadioTime = gpGlobals->time + 1.5;

	switch (slot)
	{
		case 1: player->Radio("%!MRAD_COVERME", "#Cover_me"); break;
		case 2: player->Radio("%!MRAD_TAKEPOINT", "#You_take_the_point"); break;
		case 3: player->Radio("%!MRAD_POSITION", "#Hold_this_position"); break;
		case 4: player->Radio("%!MRAD_REGROUP", "#Regroup_team"); break;
		case 5: player->Radio("%!MRAD_FOLLOWME", "#Follow_me"); break;
		case 6: player->Radio("%!MRAD_HITASSIST", "#Taking_fire"); break;
	}
}

void Radio2(CBasePlayer *player, int slot)
{
	if (gpGlobals->time <= player->m_flRadioTime)
		return;

	if (player->m_iRadioMessages <= 0)
		return;

	player->m_iRadioMessages--;
	player->m_flRadioTime = gpGlobals->time + 1.5;

	switch (slot)
	{
		case 1: player->Radio("%!MRAD_GO", "#Go_go_go"); break;
		case 2: player->Radio("%!MRAD_FALLBACK", "#Team_fall_back"); break;
		case 3: player->Radio("%!MRAD_STICKTOG", "#Stick_together_team"); break;
		case 4: player->Radio("%!MRAD_GETINPOS", "#Get_in_position_and_wait"); break;
		case 5: player->Radio("%!MRAD_STORMFRONT", "#Storm_the_front"); break;
		case 6: player->Radio("%!MRAD_REPORTIN", "#Report_in_team"); break;
	}
}

void Radio3(CBasePlayer *player, int slot)
{
	if (gpGlobals->time <= player->m_flRadioTime)
		return;

	if (player->m_iRadioMessages <= 0)
		return;

	player->m_iRadioMessages--;
	player->m_flRadioTime = gpGlobals->time + 1.5;

	switch (slot)
	{
		case 1:
		{
			if (RANDOM_LONG(0, 1))
				player->Radio("%!MRAD_AFFIRM", "#Affirmative");
			else
				player->Radio("%!MRAD_ROGER", "#Roger_that");

			break;
		}

		case 2: player->Radio("%!MRAD_ENEMYSPOT", "#Enemy_spotted"); break;
		case 3: player->Radio("%!MRAD_BACKUP", "#Need_backup"); break;
		case 4: player->Radio("%!MRAD_CLEAR", "#Sector_clear"); break;
		case 5: player->Radio("%!MRAD_INPOS", "#In_position"); break;
		case 6: player->Radio("%!MRAD_REPRTINGIN", "#Reporting_in"); break;
		case 7: player->Radio("%!MRAD_BLOW", "#Get_out_of_there"); break;
		case 8: player->Radio("%!MRAD_NEGATIVE", "#Negative"); break;
		case 9: player->Radio("%!MRAD_ENEMYDOWN", "#Enemy_down"); break;
	}
}

bool BuyGunAmmo(CBasePlayer *player, CBasePlayerItem *weapon, bool bBlinkMoney)
{
	int cost;
	char *classname;

	if (!player->CanPlayerBuy(true))
		return false;

	int nAmmo = weapon->PrimaryAmmoIndex();

	if (nAmmo == -1)
		return false;

	if (player->m_rgAmmo[nAmmo] >= weapon->iMaxAmmo1())
		return false;

	switch (weapon->m_iId)
	{
		case WEAPON_AWP:
		{
			cost = AMMO_338MAG_PRICE;
			classname = "ammo_338magnum";
			break;
		}

		case WEAPON_M249:
		{
			cost = AMMO_556NATO_PRICE;
			classname = "ammo_556natobox";
			break;
		}

		case WEAPON_AUG:
		case WEAPON_SG550:
		case WEAPON_GALIL:
		case WEAPON_FAMAS:
		case WEAPON_M4A1:
		case WEAPON_SG552:
		{
			cost = AMMO_556NATO_PRICE;
			classname = "ammo_556nato";
			break;
		}

		case WEAPON_SCOUT:
		case WEAPON_G3SG1:
		case WEAPON_AK47:
		{
			cost = AMMO_762NATO_PRICE;
			classname = "ammo_762nato";
			break;
		}

		case WEAPON_XM1014:
		case WEAPON_M3:
		{
			cost = AMMO_BUCKSHOT_PRICE;
			classname = "ammo_buckshot";
			break;
		}

		case WEAPON_MAC10:
		case WEAPON_UMP45:
		case WEAPON_USP:
		{
			cost = AMMO_45ACP_PRICE;
			classname = "ammo_45acp";
			break;
		}

		case WEAPON_FIVESEVEN:
		case WEAPON_P90:
		{
			classname = "ammo_57mm";
			cost = AMMO_57MM_PRICE;
			break;
		}

		case WEAPON_ELITE:
		case WEAPON_GLOCK18:
		case WEAPON_MP5N:
		case WEAPON_TMP:
		{
			cost = AMMO_9MM_PRICE;
			classname = "ammo_9mm";
			break;
		}

		case WEAPON_DEAGLE:
		{
			cost = AMMO_50AE_PRICE;
			classname = "ammo_50ae";
			break;
		}

		case WEAPON_P228:
		{
			classname = "ammo_357sig";
			cost = AMMO_357SIG_PRICE;
			break;
		}

		default: ALERT(at_console, "Tried to buy ammo for an unrecognized gun\n"); return false;
	}

	if (player->m_iAccount >= cost)
	{
		player->GiveNamedItem(classname);
		player->AddAccount(-cost);
		return true;
	}

	if (bBlinkMoney)
	{
		if (g_bClientPrintEnable)
		{
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Not_Enough_Money");
			BlinkAccount(player, 2);
		}
	}

	return false;
}

bool BuyAmmo(CBasePlayer *player, int nSlot, bool bBlinkMoney)
{
	if (!player->CanPlayerBuy(true))
		return false;

	if (nSlot > WPNSLOT_SECONDARY)
		return false;

	CBasePlayerItem *pItem = player->m_rgpPlayerItems[nSlot];

	if (player->HasShield() && player->m_rgpPlayerItems[WPNSLOT_SECONDARY])
		pItem = player->m_rgpPlayerItems[WPNSLOT_SECONDARY];

	if (!pItem)
		return false;

	while (BuyGunAmmo(player, pItem, bBlinkMoney))
	{
		pItem = pItem->m_pNext;

		if (!pItem)
			return true;
	}

	return false;
}

BOOL HandleBuyAliasCommands(CBasePlayer *pPlayer, const char *pszCommand)
{
	BOOL bRetVal = FALSE;
	int weaponID = 0;
	const char *pszFailItem = BuyAliasToWeaponID(pszCommand, weaponID);

	if (weaponID)
	{
		if (CanBuyWeaponByMaptype(pPlayer->m_iTeam, weaponID, g_pGameRules->m_iMapHasVIPSafetyZone == 1))
		{
			bRetVal = TRUE;
			BuyWeaponByWeaponID(pPlayer, weaponID);
		}
		else if (pszFailItem)
		{
			bRetVal = TRUE;

			if (g_bClientPrintEnable)
				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Alias_Not_Avail", pszFailItem);
		}
		else
		{
			bRetVal = TRUE;

			if (g_bClientPrintEnable)
				ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Buy_This");
		}
	}
	else
	{
		if (!strcmp(pszCommand, "primammo"))
		{
			bRetVal = TRUE;

			if (BuyAmmo(pPlayer, WPNSLOT_PRIMARY, true))
				while (BuyAmmo(pPlayer, WPNSLOT_PRIMARY, false));
		}
		else if (!strcmp(pszCommand, "secammo"))
		{
			bRetVal = TRUE;

			if (BuyAmmo(pPlayer, WPNSLOT_SECONDARY, true))
				while (BuyAmmo(pPlayer, WPNSLOT_SECONDARY, false));
		}
		else if (!strcmp(pszCommand, "vest"))
		{
			bRetVal = TRUE;
			BuyItem(pPlayer, 1);
		}
		else if (!strcmp(pszCommand, "vesthelm"))
		{
			bRetVal = TRUE;
			BuyItem(pPlayer, 2);
		}
		else if (!strcmp(pszCommand, "flash"))
		{
			bRetVal = TRUE;
			BuyItem(pPlayer, 3);
		}
		else if (!strcmp(pszCommand, "hegren"))
		{
			bRetVal = TRUE;
			BuyItem(pPlayer, 4);
		}
		else if (!strcmp(pszCommand, "sgren"))
		{
			bRetVal = TRUE;
			BuyItem(pPlayer, 5);
		}
		else if (!strcmp(pszCommand, "nvgs"))
		{
			bRetVal = TRUE;
			BuyItem(pPlayer, 6);
		}
		else if (!strcmp(pszCommand, "defuser"))
		{
			bRetVal = TRUE;

			if (pPlayer->m_iTeam != TEAM_CT)
			{
				pszFailItem = "#Bomb_Defusal_Kit";

				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Alias_Not_Avail", pszFailItem);
			}
			else
				BuyItem(pPlayer, 7);
		}
		else if (!strcmp(pszCommand, "shield"))
		{
			bRetVal = TRUE;

			if (pPlayer->m_iTeam != TEAM_CT)
			{
				pszFailItem = "#TactShield_Desc";

				if (g_bClientPrintEnable)
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Alias_Not_Avail", pszFailItem);
			}
			else
				BuyItem(pPlayer, 8);
		}
	}

	pPlayer->BuildRebuyStruct();
	return bRetVal;
}

BOOL HandleRadioAliasCommands(CBasePlayer *pPlayer, const char *pszCommand)
{
	BOOL bRetVal = FALSE;

	if (!strcmp(pszCommand, "coverme"))
	{
		bRetVal = TRUE;
		Radio1(pPlayer, 1);
	}
	else if (!strcmp(pszCommand, "takepoint"))
	{
		bRetVal = TRUE;
		Radio1(pPlayer, 2);
	}
	else if (!strcmp(pszCommand, "holdpos"))
	{
		bRetVal = TRUE;
		Radio1(pPlayer, 3);
	}
	else if (!strcmp(pszCommand, "regroup"))
	{
		bRetVal = TRUE;
		Radio1(pPlayer, 4);
	}
	else if (!strcmp(pszCommand, "followme"))
	{
		bRetVal = TRUE;
		Radio1(pPlayer, 5);
	}
	else if (!strcmp(pszCommand, "takingfire"))
	{
		bRetVal = TRUE;
		Radio1(pPlayer, 6);
	}
	else if (!strcmp(pszCommand, "go"))
	{
		bRetVal = TRUE;
		Radio2(pPlayer, 1);
	}
	else if (!strcmp(pszCommand, "fallback"))
	{
		bRetVal = TRUE;
		Radio2(pPlayer, 2);
	}
	else if (!strcmp(pszCommand, "sticktog"))
	{
		bRetVal = TRUE;
		Radio2(pPlayer, 3);
	}
	else if (!strcmp(pszCommand, "getinpos"))
	{
		bRetVal = TRUE;
		Radio2(pPlayer, 4);
	}
	else if (!strcmp(pszCommand, "stormfront"))
	{
		bRetVal = TRUE;
		Radio2(pPlayer, 5);
	}
	else if (!strcmp(pszCommand, "report"))
	{
		bRetVal = TRUE;
		Radio2(pPlayer, 6);
	}
	else if (!strcmp(pszCommand, "roger"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 1);
	}
	else if (!strcmp(pszCommand, "enemyspot"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 2);
	}
	else if (!strcmp(pszCommand, "needbackup"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 3);
	}
	else if (!strcmp(pszCommand, "sectorclear"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 4);
	}
	else if (!strcmp(pszCommand, "inposition"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 5);
	}
	else if (!strcmp(pszCommand, "reportingin"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 6);
	}
	else if (!strcmp(pszCommand, "getout"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 7);
	}
	else if (!strcmp(pszCommand, "negative"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 8);
	}
	else if (!strcmp(pszCommand, "enemydown"))
	{
		bRetVal = TRUE;
		Radio3(pPlayer, 9);
	}

	return bRetVal;
}

float g_flTimeLimit = 0;
float g_flResetTime = 0;

extern int GetMapCount(void);
extern char g_szMapBriefingText[512];

extern int gmsgNVGToggle;
extern int gmsgBuyClose;
extern int gmsgADStop;


void ClientCommand(edict_t *pEntity)
{
	const char *pcmd = CMD_ARGV(0);
	const char *pstr;
	CHalfLifeMultiplay *mp = g_pGameRules;

	if (!pEntity->pvPrivateData)
		return;

	entvars_t *pev = &pEntity->v;
	CBasePlayer *player = GetClassPtr((CBasePlayer *)pev);

	if (FStrEq(pcmd, "say"))
	{
		if (gpGlobals->time >= player->m_flLastCommandTime[0])
		{
			player->m_flLastCommandTime[0] = gpGlobals->time + 0.3;
			Host_Say(pEntity, FALSE);
		}

		return;
	}
	else if (FStrEq(pcmd, "say_team"))
	{
		if (gpGlobals->time >= player->m_flLastCommandTime[1])
		{
			player->m_flLastCommandTime[1] = gpGlobals->time + 0.3;
			Host_Say(pEntity, TRUE);
		}

		return;
	}
	else if (FStrEq(pcmd, "fullupdate"))
	{
		if (gpGlobals->time >= player->m_flLastCommandTime[2])
		{
			player->m_flLastCommandTime[2] = gpGlobals->time + 0.6;
			player->ForceClientDllUpdate();
		}

		return;
	}
	else if (FStrEq(pcmd, "vote"))
	{
		if (gpGlobals->time >= player->m_flLastCommandTime[3])
		{
			BOOL iFailed = FALSE;
			player->m_flLastCommandTime[3] = gpGlobals->time + 0.3;

			if (gpGlobals->time < player->m_flNextVoteTime)
			{
				ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Wait_3_Seconds");
				return;
			}

			player->m_flNextVoteTime = gpGlobals->time + 3;

			if (player->m_iTeam != TEAM_UNASSIGNED)
			{
				int iNumArgs = CMD_ARGC();
				const char *pszArg1 = CMD_ARGV(1);
				int iVoteLength = strlen(pszArg1);

				if (iNumArgs != 2 || (iVoteLength <= 0 || iVoteLength > 7))
					iFailed = TRUE;

				int iVoteID = atoi(pszArg1);

				if (iVoteID <= 0)
					iFailed = TRUE;

				if (iFailed)
				{
					ListPlayers(player);
					ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Game_vote_usage");
					return;
				}

				CBaseEntity *pEntity = NULL;
				int iVote = 0;

				while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
				{
					if (FNullEnt(pEntity->edict()))
						break;

					if (pEntity->pev->flags & FL_DORMANT)
						continue;

					if (GetClassPtr((CBasePlayer *)pEntity->pev)->m_iTeam == player->m_iTeam)
						iVote++;
				}

				if (iVote < 3)
				{
					ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Cannot_Vote_With_Less_Than_Three");
					return;
				}

				CBaseEntity *pKickEntity = EntityFromUserID(iVoteID);

				if (pKickEntity)
				{
					CBasePlayer *pKickPlayer = GetClassPtr((CBasePlayer *)pKickEntity->pev);

					if (pKickPlayer->m_iTeam != player->m_iTeam)
					{
						ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Game_vote_players_on_your_team");
						return;
					}

					if (pKickPlayer == player)
					{
						ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Game_vote_not_yourself");
						return;
					}

					ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Game_vote_cast", UTIL_dtos1(iVoteID));
					player->m_iCurrentKickVote = iVoteID;
					ProcessKickVote(player, pKickPlayer);
					return;
				}

				ListPlayers(player);
				ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Game_vote_player_not_found", UTIL_dtos1(iVoteID));
			}
		}

		return;
	}
	else if (FStrEq(pcmd, "listmaps"))
	{
		if (gpGlobals->time >= player->m_flLastCommandTime[4])
		{
			player->m_flLastCommandTime[4] = gpGlobals->time + 0.3;
			g_pGameRules->DisplayMaps(player, 0);
		}

		return;
	}
	else if (FStrEq(pcmd, "votemap"))
	{
		if (gpGlobals->time >= player->m_flLastCommandTime[5])
		{
			BOOL iFailed = FALSE;
			player->m_flLastCommandTime[5] = gpGlobals->time + 0.3;

			if (gpGlobals->time < player->m_flNextVoteTime)
			{
				ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Wait_3_Seconds");
				return;
			}

			player->m_flNextVoteTime = gpGlobals->time + 3;

			if (player->m_iTeam != TEAM_UNASSIGNED)
			{
				if (gpGlobals->time < 180)
				{
					ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Cannot_Vote_Map");
					return;
				}

				int iNumArgs = CMD_ARGC();
				const char *pszArg1 = CMD_ARGV(1);
				int iVoteLength = strlen(pszArg1);

				if (iNumArgs != 2 || iVoteLength > 5)
					iFailed = TRUE;

				int iVoteID = atoi(pszArg1);

				if (iVoteID < 1 || iVoteID > MAX_MAPS)
					iFailed = TRUE;

				if (iVoteID > GetMapCount())
					iFailed = TRUE;

				if (iFailed)
				{
					g_pGameRules->DisplayMaps(player, 0);
					ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Game_votemap_usage");
					return;
				}

				CBaseEntity *pEntity = NULL;
				int iVote = 0;

				while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
				{
					if (FNullEnt(pEntity->edict()))
						break;

					if (FBitSet(pEntity->pev->flags, FL_DORMANT))
						continue;

					if (GetClassPtr((CBasePlayer *)pEntity->pev)->m_iTeam == player->m_iTeam)
						iVote++;
				}

				if (iVote < 2)
				{
					ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Cannot_Vote_Need_More_People");
					return;
				}

				if (player->m_iCurrentKickVote)
				{
					g_pGameRules->m_iMapVotes[player->m_iMapVote]--;

					if (g_pGameRules->m_iMapVotes[player->m_iMapVote] < 0)
						g_pGameRules->m_iMapVotes[player->m_iMapVote] = 0;
				}

				ClientPrint(player->pev, HUD_PRINTCONSOLE, "#Game_voted_for_map", UTIL_dtos1(iVoteID));
				player->m_iMapVote = iVoteID;
				g_pGameRules->ProcessMapVote(player, iVoteID);
			}
		}

		return;
	}
	else if (FStrEq(pcmd, "timeleft"))
	{
		if (gpGlobals->time > player->m_iTimeCheckAllowed)
		{
			if (!timelimit.value)
			{
				ClientPrint(player->pev, HUD_PRINTTALK, "#Game_no_timelimit");
				return;
			}

			int iTimeRemaining = (int)(g_flTimeLimit - gpGlobals->time);

			if (iTimeRemaining < 0)
				iTimeRemaining = 0;

			int iMinutes = iTimeRemaining % 60;
			int iSeconds = iTimeRemaining / 60;
			char secs[3];
			char *temp = UTIL_dtos2(iMinutes);

			if (iMinutes >= 10)
			{
				secs[0] = temp[0];
				secs[1] = temp[1];
				secs[2] = '\0';
			}
			else
			{
				secs[0] = '0';
				secs[1] = temp[0];
				secs[2] = '\0';
			}

			ClientPrint(player->pev, HUD_PRINTTALK, "#Game_timelimit", UTIL_dtos1(iSeconds), secs);
		}

		return;
	}
	else if (FStrEq(pcmd, "listplayers"))
	{
		if (gpGlobals->time >= player->m_flLastCommandTime[6])
		{
			player->m_flLastCommandTime[6] = gpGlobals->time + 0.3;
			ListPlayers(player);
		}

		return;
	}
	else if (FStrEq(pcmd, "client_buy_open"))
	{
		if (player->m_iMenu == Menu_OFF)
			player->m_iMenu = Menu_ClientBuy;

		if (player->m_signals.GetState() & SIGNAL_BUY)
		{
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, player->pev);
			MESSAGE_END();
		}

		return;
	}
	else if (FStrEq(pcmd, "client_buy_close"))
	{
		if (player->m_iMenu == Menu_ClientBuy)
			player->m_iMenu = Menu_OFF;

		return;
	}
	else if (FStrEq(pcmd, "menuselect"))
	{
		int slot = atoi(CMD_ARGV(1));

		if (player->m_iJoiningState == JOINED || (player->m_iMenu != TEAM_SPECTATOR && player->m_iMenu != TEAM_TERRORIST))
		{
			if (slot == 10)
				player->m_iMenu = Menu_OFF;
		}

		switch (player->m_iMenu)
		{
			case Menu_ChooseTeam:
			{
				if (!player->m_bVGUIMenus)
				{
					if (!HandleMenu_ChooseTeam(player, slot))
					{
						if (player->m_iJoiningState == JOINED)
							ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5 | MENU_KEY_0, "#IG_Team_Select");
						else
							ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5, "#Team_Select");

						player->m_iMenu = Menu_ChooseTeam;
					}
				}

				break;
			}

			case Menu_IGChooseTeam:
			{
				if (!player->m_bVGUIMenus)
					HandleMenu_ChooseTeam(player, slot);

				break;
			}

			case Menu_ChooseAppearance:
			{
				if (!player->m_bVGUIMenus)
					HandleMenu_ChooseAppearance(player, slot);

				break;
			}

			case Menu_Buy:
			{
				if (!player->m_bVGUIMenus)
					Buy(player, slot);

				break;
			}

			case Menu_BuyPistol:
			{
				if (!player->m_bVGUIMenus)
					BuyPistol(player, slot);

				break;
			}

			case Menu_BuyShotgun:
			{
				if (!player->m_bVGUIMenus)
					BuyShotgun(player, slot);

				break;
			}

			case Menu_BuySubMachineGun:
			{
				if (!player->m_bVGUIMenus)
					BuySubMachineGun(player, slot);

				break;
			}

			case Menu_BuyRifle:
			{
				if (!player->m_bVGUIMenus)
					BuyRifle(player, slot);

				break;
			}

			case Menu_BuyMachineGun:
			{
				if (!player->m_bVGUIMenus)
					BuyMachineGun(player, slot);

				break;
			}

			case Menu_BuyItem:
			{
				BuyItem(player, slot);
				break;
			}

			case Menu_Radio1:
			{
				Radio1(player, slot);
				break;
			}

			case Menu_Radio2:
			{
				Radio2(player, slot);
				break;
			}

			case Menu_Radio3:
			{
				Radio3(player, slot);
				break;
			}

			case 0: break;
			default: ALERT(at_console, "ClientCommand(): Invalid menu selected\n"); break;
		}

		return;
	}
	else if (FStrEq(pcmd, "chooseteam"))
	{
		if (player->m_iMenu == Menu_ChooseAppearance)
			return;

		if (player->m_bTeamChanged)
		{
			if (player->pev->deadflag != DEAD_NO)
			{
				ClientPrint(player->pev, HUD_PRINTCENTER, "#Only_1_Team_Change");
				return;
			}
		}

		if (!g_pGameRules->IsCareer())
		{
			if (g_pGameRules->m_iMapHasVIPSafetyZone == 1 && player->m_iJoiningState == JOINED && player->m_iTeam == TEAM_CT)
			{
				if (g_pGameRules->IsFreezePeriod() || player->pev->deadflag != DEAD_NO)
					ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#IG_VIP_Team_Select_Spect");
				else
					ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5 | MENU_KEY_0, "#IG_VIP_Team_Select");
			}
			else
			{
				if (g_pGameRules->IsFreezePeriod() || player->pev->deadflag != DEAD_NO)
					ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#IG_Team_Select_Spect");
				else
					ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_5 | MENU_KEY_0, "#IG_Team_Select");
			}

			player->m_iMenu = Menu_ChooseTeam;
		}

		return;
	}
	else if (FStrEq(pcmd, "showbriefing"))
	{
		if (player->m_iMenu != Menu_OFF)
			return;

		if (!g_szMapBriefingText[0])
			return;

		if (player->m_iTeam == TEAM_UNASSIGNED)
			return;

		if (player->m_afPhysicsFlags & PFLAG_OBSERVER)
			return;

		player->MenuPrint(g_szMapBriefingText);
		player->m_bMissionBriefing = true;
		return;
	}
	else if (FStrEq(pcmd, "ignoremsg"))
	{
		if (player->m_iIgnoreGlobalChat == IGNOREMSG_NONE)
		{
			player->m_iIgnoreGlobalChat = IGNOREMSG_ENEMY;
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Ignore_Broadcast_Messages");
		}
		else if (player->m_iIgnoreGlobalChat == IGNOREMSG_ENEMY)
		{
			player->m_iIgnoreGlobalChat = IGNOREMSG_TEAM;
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Ignore_Broadcast_Team_Messages");
		}
		else if (player->m_iIgnoreGlobalChat == IGNOREMSG_TEAM)
		{
			player->m_iIgnoreGlobalChat = IGNOREMSG_NONE;
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Accept_All_Messages");
		}

		return;
	}
	else if (FStrEq(pcmd, "ignorerad"))
	{
		if (!player->m_bIgnoreRadio)
		{
			player->m_bIgnoreRadio = true;
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Ignore_Radio");
		}
		else
		{
			player->m_bIgnoreRadio = false;
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Accept_Radio");
		}

		return;
	}
	else if (FStrEq(pcmd, "become_vip"))
	{
		if (player->m_iJoiningState != JOINED || player->m_iTeam != TEAM_CT)
		{
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Command_Not_Available");
			return;
		}

		g_pGameRules->AddToVIPQueue(player);
		return;
	}
	else if (FStrEq(pcmd, "spectate") && FBitSet(player->pev->flags, FL_PROXY))
	{
		HandleMenu_ChooseTeam(player, 6);
		return;
	}
	else if (FStrEq(pcmd, "specmode"))
	{
		int i = atoi(CMD_ARGV(1));

		if (player->IsObserver() && player->CanSwitchObserverModes())
			player->Observer_SetMode(i);
		else
			player->m_iObserverLastMode = i;

		if (i == OBS_CHASE_FREE)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgADStop, NULL, player->pev);
			MESSAGE_END();
		}

		return;
	}
	else if (FStrEq(pcmd, "spec_set_ad"))
	{
		if (atof(CMD_ARGV(1)) < 0)
			player->SetObserverAutoDirector(false);
		else
			player->SetObserverAutoDirector(true);

		return;
	}
	else if (FStrEq(pcmd, "follownext"))
	{
		int i = atoi(CMD_ARGV(1));

		if (player->IsObserver() && player->CanSwitchObserverModes())
			player->Observer_FindNextPlayer(i != 0);

		return;
	}
	else if (FStrEq(pcmd, "follow"))
	{
		if (player->IsObserver() && player->CanSwitchObserverModes())
			player->Observer_FindNextPlayer(false, (char *)CMD_ARGV(1));

		return;
	}

	if (g_pGameRules->ClientCommand_DeadOrAlive(GetClassPtr((CBasePlayer *)pev), pcmd))
		return;

	if (FStrEq(pcmd, "jointeam"))
	{
		if (player->m_iMenu == Menu_ChooseAppearance)
		{
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Command_Not_Available");
			return;
		}

		int i = atoi(CMD_ARGV(1));

		if (HandleMenu_ChooseTeam(player, i))
		{
			if (i == 3 || i == 6 || player->m_bIsVIP)
				player->m_iMenu = Menu_OFF;
			else
				player->m_iMenu = Menu_ChooseAppearance;
		}
		else
		{
			if (player->m_iJoiningState == JOINED)
				ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5 | MENU_KEY_0, "#IG_Team_Select");
			else
				ShowVGUIMenu(player, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5, "#Team_Select");

			player->m_iMenu = Menu_ChooseTeam;
		}

		return;
	}
	else if (FStrEq(pcmd, "joinclass"))
	{
		int i = atoi(CMD_ARGV(1));

		if (player->m_iMenu != Menu_ChooseAppearance)
		{
			ClientPrint(player->pev, HUD_PRINTCENTER, "#Command_Not_Available");
			return;
		}

		HandleMenu_ChooseAppearance(player, i);
		return;
	}

	if (player->pev->deadflag != DEAD_NO)
		return;

	if (FStrEq(pcmd, "nightvision"))
	{
		if (!player->m_bHasNightVision)
			return;

		if (player->m_bNightVisionOn)
		{
			EMIT_SOUND(ENT(player->pev), CHAN_ITEM, "items/nvg_off.wav", RANDOM_FLOAT(0.92, 1), ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, player->pev);
			WRITE_BYTE(0);
			MESSAGE_END();

			player->m_bNightVisionOn = false;

			for (int i = 1; i < gpGlobals->maxClients; i++)
			{
				CBasePlayer *pOther = (CBasePlayer *)UTIL_PlayerByIndex(i);

				if (pOther && pOther->IsObservingPlayer(player))
				{
					EMIT_SOUND(ENT(pOther->pev), CHAN_ITEM, "items/nvg_off.wav", RANDOM_FLOAT(0.92, 1), ATTN_NORM);

					MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pOther->pev);
					WRITE_BYTE(0);
					MESSAGE_END();

					pOther->m_bNightVisionOn = false;
				}
			}
		}
		else
		{
			EMIT_SOUND(ENT(player->pev), CHAN_ITEM, "items/nvg_on.wav", RANDOM_FLOAT(0.92, 1), ATTN_NORM);

			MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, player->pev);
			WRITE_BYTE(1);
			MESSAGE_END();

			player->m_bNightVisionOn = true;

			for (int i = 1; i < gpGlobals->maxClients; i++)
			{
				CBasePlayer *pOther = (CBasePlayer *)UTIL_PlayerByIndex(i);

				if (pOther && pOther->IsObservingPlayer(player))
				{
					EMIT_SOUND(ENT(pOther->pev), CHAN_ITEM, "items/nvg_on.wav", RANDOM_FLOAT(0.92, 1), ATTN_NORM);

					MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pOther->pev);
					WRITE_BYTE(1);
					MESSAGE_END();

					pOther->m_bNightVisionOn = true;
				}
			}
		}

		return;
	}
	else if (FStrEq(pcmd, "radio1"))
	{
		ShowMenu(player, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, -1, 0, "#RadioA");
		player->m_iMenu = Menu_Radio1;
		return;
	}
	else if (FStrEq(pcmd, "radio2"))
	{
		ShowMenu(player, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, -1, 0, "#RadioB");
		player->m_iMenu = Menu_Radio2;
		return;
	}
	else if (FStrEq(pcmd, "radio3"))
	{
		ShowMenu(player, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_7 | MENU_KEY_8 | MENU_KEY_9 | MENU_KEY_0, -1, 0, "#RadioC");
		player->m_iMenu = Menu_Radio3;
		return;
	}
	else if (FStrEq(pcmd, "drop"))
	{
		if (player->HasShield())
		{
			if (player->m_pActiveItem && player->m_pActiveItem->m_iId == WEAPON_C4)
				player->DropPlayerItem("weapon_c4");
			else
				player->DropShield(true);
		}
		else
			player->DropPlayerItem(CMD_ARGV(1));

		return;
	}
	else if (FStrEq(pcmd, "fov"))
	{
		return;
	}
#ifdef _DEBUG
	else if (FStrEq(pcmd, "give"))
	{
		GetClassPtr((CBasePlayer *)pev)->GiveNamedItem(CMD_ARGV(1));
		return;
	}
#endif
	else if (FStrEq(pcmd, "use"))
	{
		GetClassPtr((CBasePlayer *)pev)->SelectItem(CMD_ARGV(1));
		return;
	}
	else if (((pstr = strstr(pcmd, "weapon_")) != NULL) && (pstr == pcmd))
	{
		GetClassPtr((CBasePlayer *)pev)->SelectItem(pcmd);
		return;
	}
	else if (FStrEq(pcmd, "lastinv"))
	{
		GetClassPtr((CBasePlayer *)pev)->SelectLastItem();
		return;
	}
	else if (FStrEq(pcmd, "buyammo1"))
	{
		if (!(player->m_signals.GetState() & SIGNAL_BUY))
			return;

		BuyAmmo(player, WPNSLOT_PRIMARY, true);
		player->BuildRebuyStruct();
		return;
	}

	if (FStrEq(pcmd, "buyammo2"))
	{
		if (!(player->m_signals.GetState() & SIGNAL_BUY))
			return;

		BuyAmmo(player, WPNSLOT_SECONDARY, true);
		player->BuildRebuyStruct();
		return;
	}
	else if (FStrEq(pcmd, "buyequip"))
	{
		if (!(player->m_signals.GetState() & SIGNAL_BUY))
			return;

		if (g_pGameRules->m_bMapHasBombTarget == true)
		{
			if (player->m_iTeam == TEAM_CT)
				ShowVGUIMenu(player, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_7 | MENU_KEY_8 | MENU_KEY_0, "#DCT_BuyItem");
			else
				ShowVGUIMenu(player, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#DT_BuyItem");
		}
		else
		{
			if (player->m_iTeam == TEAM_CT)
				ShowVGUIMenu(player, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_8 | MENU_KEY_0, "#CT_BuyItem");
			else
				ShowVGUIMenu(player, MENU_BUY_ITEM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_0, "#T_BuyItem");
		}

		player->m_iMenu = Menu_BuyItem;
		return;
	}
	else if (FStrEq(pcmd, "buy"))
	{
		if (!(player->m_signals.GetState() & SIGNAL_BUY))
			return;

		ShowVGUIMenu(player, MENU_BUY, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_3 | MENU_KEY_4 | MENU_KEY_5 | MENU_KEY_6 | MENU_KEY_7 | MENU_KEY_8 | MENU_KEY_0, "#Buy");
		player->m_iMenu = Menu_Buy;
		return;
	}
	else if (FStrEq(pcmd, "cl_setautobuy"))
	{
		player->ClearAutoBuyData();

		for (int i = 1; i < CMD_ARGC(); i++)
			player->AddAutoBuyData(CMD_ARGV(i));

		BOOL oldval = g_bClientPrintEnable;
		g_bClientPrintEnable = false;
		player->AutoBuy();
		g_bClientPrintEnable = oldval;
		return;
	}
	else if (FStrEq(pcmd, "cl_setrebuy"))
	{
		if (CMD_ARGC() == 2)
		{
			player->InitRebuyData(CMD_ARGV(1));

			BOOL oldval = g_bClientPrintEnable;
			g_bClientPrintEnable = false;
			player->Rebuy();
			g_bClientPrintEnable = oldval;
		}

		return;
	}
	else if (FStrEq(pcmd, "cl_autobuy"))
	{
		if (!(player->m_signals.GetState() & SIGNAL_BUY))
			return;

		BOOL oldval = g_bClientPrintEnable;
		g_bClientPrintEnable = false;
		player->AutoBuy();
		g_bClientPrintEnable = oldval;
		return;
	}
	else if (FStrEq(pcmd, "cl_rebuy"))
	{
		if (!(player->m_signals.GetState() & SIGNAL_BUY))
			return;

		BOOL oldval = g_bClientPrintEnable;
		g_bClientPrintEnable = false;
		player->Rebuy();
		g_bClientPrintEnable = oldval;
		return;
	}
	else if (FStrEq(pcmd, "smartradio"))
	{
		player->SmartRadio();
		return;
	}
	if (HandleBuyAliasCommands(player, pcmd))
		return;

	if (HandleRadioAliasCommands(player, pcmd))
		return;

	if (g_pGameRules->ClientCommand(GetClassPtr((CBasePlayer *)pev), pcmd))
		return;

	char command[128];
	strncpy(command, pcmd, 127);
	command[127] = '\0';
	ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "#Game_unknown_command", command);
}

extern int gmsgBlinkAcct;

void BlinkAccount(CBasePlayer *pPlayer, int numBlinks)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgBlinkAcct, NULL, pPlayer->pev);
	WRITE_BYTE(numBlinks);
	MESSAGE_END();
}

void ClientUserInfoChanged(edict_t *pEntity, char *infobuffer)
{
	if (!pEntity->pvPrivateData)
		return;

	char *szBufferName = g_engfuncs.pfnInfoKeyValue(infobuffer, "name");
	int iClientIndex = ENTINDEX(pEntity);

	if (pEntity->v.netname && STRING(pEntity->v.netname)[0] != '\0' && !FStrEq(STRING(pEntity->v.netname), szBufferName))
	{
		char sName[32];
		snprintf(sName, sizeof(sName), "%s", szBufferName);

		for (char *pApersand = sName; pApersand != NULL && *pApersand != '\0'; pApersand++)
		{
			if (*pApersand == '%' || *pApersand == '&')
				*pApersand = ' ';
		}

		if (sName[0] == '#')
			sName[0] = '*';

		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pEntity);

		if (pPlayer->pev->deadflag != DEAD_NO)
		{
			pPlayer->m_bHasChangedName = true;
			snprintf(pPlayer->m_szNewName, sizeof(pPlayer->m_szNewName), "%s", sName);
			ClientPrint(pPlayer->pev, HUD_PRINTTALK, "#Name_change_at_respawn");
			g_engfuncs.pfnSetClientKeyValue(iClientIndex, infobuffer, "name", (char *)STRING(pEntity->v.netname));
		}
		else
		{
			g_engfuncs.pfnSetClientKeyValue(iClientIndex, infobuffer, "name", sName);

			MESSAGE_BEGIN(MSG_BROADCAST, gmsgSayText);
			WRITE_BYTE(iClientIndex);
			WRITE_STRING("#Cstrike_Name_Change");
			WRITE_STRING(STRING(pEntity->v.netname));
			WRITE_STRING(sName);
			MESSAGE_END();

			UTIL_LogPrintf("\"%s<%i><%s><%s>\" changed name to \"%s\"\n", STRING(pEntity->v.netname), GETPLAYERUSERID(pEntity), GETPLAYERAUTHID(pEntity), GetTeam(pPlayer->m_iTeam), sName);
		}
	}

	g_pGameRules->ClientUserInfoChanged(GetClassPtr((CBasePlayer *)&pEntity->v), infobuffer);
}

static int g_serveractive = 0;

void ServerDeactivate(void)
{
	if (g_serveractive != 1)
		return;

	g_serveractive = 0;
	CLocalNav::Reset();
}

void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	g_serveractive = 1;

	EmptyEntityHashTable();

	for (int i = 0; i < edictCount; i++)
	{
		if (pEdictList[i].free)
			continue;

		if (i < clientMax || !pEdictList[i].pvPrivateData)
			continue;

		CBaseEntity *pClass = CBaseEntity::Instance(&pEdictList[i]);

		if (pClass && !(pClass->pev->flags & FL_DORMANT))
		{
			AddEntityHashValue(&pEdictList[i].v, STRING(pEdictList[i].v.classname), CLASSNAME);
			pClass->Activate();
		}
		else
			ALERT(at_console, "Can't instance %s\n", STRING(pEdictList[i].v.classname));
	}

	LinkUserMessages();
	WriteSigonMessages();

	if (g_pGameRules)
		g_pGameRules->CheckMapConditions();
}

void PlayerPreThink(edict_t *pEntity)
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PreThink();
}

void PlayerPostThink(edict_t *pEntity)
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PostThink();
}

void ParmsNewLevel(void)
{

}

void ParmsChangeLevel(void)
{
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if (pSaveData)
		pSaveData->connectionCount = BuildChangeList(pSaveData->levelList, MAX_LEVEL_CONNECTIONS);
}

void StartFrame(void)
{
	if (g_pGameRules)
		g_pGameRules->Think();

	if (g_fGameOver)
		return;

	CLocalNav::Think();
	static cvar_t *skill = NULL;

	if (!skill)
		skill = CVAR_GET_POINTER("skill");

	gpGlobals->teamplay = 1;

	if (skill)
		g_iSkillLevel = skill->value;
	else
		g_iSkillLevel = 0;

	g_ulFrameCount++;
}

unsigned short m_usResetDecals;
int g_iShadowSprite;

void ClientPrecache(void)
{
	int i;

	PRECACHE_SOUND("weapons/dryfire_pistol.wav");
	PRECACHE_SOUND("weapons/dryfire_rifle.wav");
	PRECACHE_SOUND("player/pl_shot1.wav");
	PRECACHE_SOUND("player/pl_die1.wav");
	PRECACHE_SOUND("player/headshot1.wav");
	PRECACHE_SOUND("player/headshot2.wav");
	PRECACHE_SOUND("player/headshot3.wav");
	PRECACHE_SOUND("player/bhit_flesh-1.wav");
	PRECACHE_SOUND("player/bhit_flesh-2.wav");
	PRECACHE_SOUND("player/bhit_flesh-3.wav");
	PRECACHE_SOUND("player/bhit_kevlar-1.wav");
	PRECACHE_SOUND("player/bhit_helmet-1.wav");
	PRECACHE_SOUND("player/die1.wav");
	PRECACHE_SOUND("player/die2.wav");
	PRECACHE_SOUND("player/die3.wav");
	PRECACHE_SOUND("player/death6.wav");
	PRECACHE_SOUND("radio/locknload.wav");
	PRECACHE_SOUND("radio/letsgo.wav");
	PRECACHE_SOUND("radio/moveout.wav");
	PRECACHE_SOUND("radio/com_go.wav");
	PRECACHE_SOUND("radio/rescued.wav");
	PRECACHE_SOUND("radio/rounddraw.wav");
	PRECACHE_SOUND("items/kevlar.wav");
	PRECACHE_SOUND("items/ammopickup2.wav");
	PRECACHE_SOUND("items/nvg_on.wav");
	PRECACHE_SOUND("items/nvg_off.wav");
	PRECACHE_SOUND("items/equip_nvg.wav");
	PRECACHE_SOUND("weapons/c4_beep1.wav");
	PRECACHE_SOUND("weapons/c4_beep2.wav");
	PRECACHE_SOUND("weapons/c4_beep3.wav");
	PRECACHE_SOUND("weapons/c4_beep4.wav");
	PRECACHE_SOUND("weapons/c4_beep5.wav");
	PRECACHE_SOUND("weapons/c4_explode1.wav");
	PRECACHE_SOUND("weapons/c4_plant.wav");
	PRECACHE_SOUND("weapons/c4_disarm.wav");
	PRECACHE_SOUND("weapons/c4_disarmed.wav");
	PRECACHE_SOUND("weapons/explode3.wav");
	PRECACHE_SOUND("weapons/explode4.wav");
	PRECACHE_SOUND("weapons/explode5.wav");
	PRECACHE_SOUND("player/sprayer.wav");
	PRECACHE_SOUND("player/pl_fallpain2.wav");
	PRECACHE_SOUND("player/pl_fallpain3.wav");
	PRECACHE_SOUND("player/pl_snow1.wav");
	PRECACHE_SOUND("player/pl_snow2.wav");
	PRECACHE_SOUND("player/pl_snow3.wav");
	PRECACHE_SOUND("player/pl_snow4.wav");
	PRECACHE_SOUND("player/pl_snow5.wav");
	PRECACHE_SOUND("player/pl_snow6.wav");
	PRECACHE_SOUND("player/pl_step1.wav");
	PRECACHE_SOUND("player/pl_step2.wav");
	PRECACHE_SOUND("player/pl_step3.wav");
	PRECACHE_SOUND("player/pl_step4.wav");
	PRECACHE_SOUND("common/npc_step1.wav");
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");
	PRECACHE_SOUND("player/pl_metal1.wav");
	PRECACHE_SOUND("player/pl_metal2.wav");
	PRECACHE_SOUND("player/pl_metal3.wav");
	PRECACHE_SOUND("player/pl_metal4.wav");
	PRECACHE_SOUND("player/pl_dirt1.wav");
	PRECACHE_SOUND("player/pl_dirt2.wav");
	PRECACHE_SOUND("player/pl_dirt3.wav");
	PRECACHE_SOUND("player/pl_dirt4.wav");
	PRECACHE_SOUND("player/pl_duct1.wav");
	PRECACHE_SOUND("player/pl_duct2.wav");
	PRECACHE_SOUND("player/pl_duct3.wav");
	PRECACHE_SOUND("player/pl_duct4.wav");
	PRECACHE_SOUND("player/pl_grate1.wav");
	PRECACHE_SOUND("player/pl_grate2.wav");
	PRECACHE_SOUND("player/pl_grate3.wav");
	PRECACHE_SOUND("player/pl_grate4.wav");
	PRECACHE_SOUND("player/pl_slosh1.wav");
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");
	PRECACHE_SOUND("player/pl_tile1.wav");
	PRECACHE_SOUND("player/pl_tile2.wav");
	PRECACHE_SOUND("player/pl_tile3.wav");
	PRECACHE_SOUND("player/pl_tile4.wav");
	PRECACHE_SOUND("player/pl_tile5.wav");
	PRECACHE_SOUND("player/pl_swim1.wav");
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");
	PRECACHE_SOUND("player/pl_ladder1.wav");
	PRECACHE_SOUND("player/pl_ladder2.wav");
	PRECACHE_SOUND("player/pl_ladder3.wav");
	PRECACHE_SOUND("player/pl_ladder4.wav");
	PRECACHE_SOUND("player/pl_wade1.wav");
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");
	PRECACHE_SOUND("debris/wood1.wav");
	PRECACHE_SOUND("debris/wood2.wav");
	PRECACHE_SOUND("debris/wood3.wav");
	PRECACHE_SOUND("plats/train_use1.wav");
	PRECACHE_SOUND("plats/vehicle_ignition.wav");
	PRECACHE_SOUND("buttons/spark5.wav");
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/glass1.wav");
	PRECACHE_SOUND("debris/glass2.wav");
	PRECACHE_SOUND("debris/glass3.wav");
	PRECACHE_SOUND("items/flashlight1.wav");
	PRECACHE_SOUND("items/flashlight1.wav");
	PRECACHE_SOUND("common/bodysplat.wav");
	PRECACHE_SOUND("player/pl_pain2.wav");
	PRECACHE_SOUND("player/pl_pain4.wav");
	PRECACHE_SOUND("player/pl_pain5.wav");
	PRECACHE_SOUND("player/pl_pain6.wav");
	PRECACHE_SOUND("player/pl_pain7.wav");

	for (i = 0; i < PLAYERMODEL_COUNT; i++)
		PRECACHE_MODEL(sPlayerModelFiles[i]);

	PRECACHE_MODEL("models/p_ak47.mdl");
	PRECACHE_MODEL("models/p_aug.mdl");
	PRECACHE_MODEL("models/p_awp.mdl");
	PRECACHE_MODEL("models/p_c4.mdl");
	PRECACHE_MODEL("models/w_c4.mdl");
	PRECACHE_MODEL("models/p_deagle.mdl");
	PRECACHE_MODEL("models/shield/p_shield_deagle.mdl");
	PRECACHE_MODEL("models/p_flashbang.mdl");
	PRECACHE_MODEL("models/shield/p_shield_flashbang.mdl");
	PRECACHE_MODEL("models/p_hegrenade.mdl");
	PRECACHE_MODEL("models/shield/p_shield_hegrenade.mdl");
	PRECACHE_MODEL("models/p_glock18.mdl");
	PRECACHE_MODEL("models/shield/p_shield_glock18.mdl");
	PRECACHE_MODEL("models/p_p228.mdl");
	PRECACHE_MODEL("models/shield/p_shield_p228.mdl");
	PRECACHE_MODEL("models/p_smokegrenade.mdl");
	PRECACHE_MODEL("models/shield/p_shield_smokegrenade.mdl");
	PRECACHE_MODEL("models/p_usp.mdl");
	PRECACHE_MODEL("models/shield/p_shield_usp.mdl");
	PRECACHE_MODEL("models/p_fiveseven.mdl");
	PRECACHE_MODEL("models/shield/p_shield_fiveseven.mdl");
	PRECACHE_MODEL("models/p_knife.mdl");
	PRECACHE_MODEL("models/shield/p_shield_knife.mdl");
	PRECACHE_MODEL("models/w_flashbang.mdl");
	PRECACHE_MODEL("models/w_hegrenade.mdl");
	PRECACHE_MODEL("models/p_sg550.mdl");
	PRECACHE_MODEL("models/p_g3sg1.mdl");
	PRECACHE_MODEL("models/p_m249.mdl");
	PRECACHE_MODEL("models/p_m3.mdl");
	PRECACHE_MODEL("models/p_m4a1.mdl");
	PRECACHE_MODEL("models/p_mac10.mdl");
	PRECACHE_MODEL("models/p_mp5.mdl");
	PRECACHE_MODEL("models/p_ump45.mdl");
	PRECACHE_MODEL("models/p_p90.mdl");
	PRECACHE_MODEL("models/p_scout.mdl");
	PRECACHE_MODEL("models/p_sg552.mdl");
	PRECACHE_MODEL("models/w_smokegrenade.mdl");
	PRECACHE_MODEL("models/p_tmp.mdl");
	PRECACHE_MODEL("models/p_elite.mdl");
	PRECACHE_MODEL("models/p_xm1014.mdl");
	PRECACHE_MODEL("models/p_galil.mdl");
	PRECACHE_MODEL("models/p_famas.mdl");
	PRECACHE_MODEL("models/p_shield.mdl");
	PRECACHE_MODEL("models/w_shield.mdl");

	for (i = 0; i < 4; i++)
		ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-38, -24, -41), (float *)&Vector(38, 24, 41), sPlayerModelFiles[i]);

	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/black_smoke1.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/black_smoke2.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/black_smoke3.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/black_smoke4.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/fast_wallpuff1.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/smokepuff.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/gas_puff_01.spr");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/scope_arc.tga");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/scope_arc_nw.tga");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/scope_arc_ne.tga");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, (float *)&g_vecZero, (float *)&g_vecZero, "sprites/scope_arc_sw.tga");

	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-12, -6, -22), (float *)&Vector(12, 6, 22), "models/p_deagle.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-12, -6, -22), (float *)&Vector(12, 6, 22), "models/p_p228.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-12, -6, -22), (float *)&Vector(12, 6, 22), "models/p_elite.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-12, -6, -22), (float *)&Vector(12, 6, 22), "models/p_usp.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-12, -6, -22), (float *)&Vector(12, 6, 22), "models/p_fiveseven.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-12, -6, -22), (float *)&Vector(12, 6, 22), "models/p_glock18.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-25, -19, -21), (float *)&Vector(25, 23, 21), "models/p_xm1014.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-25, -19, -21), (float *)&Vector(25, 23, 21), "models/p_m3.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-23, -8, -20), (float *)&Vector(23, 8, 20), "models/p_mac10.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-23, -8, -20), (float *)&Vector(23, 8, 20), "models/p_mp5.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-23, -8, -20), (float *)&Vector(23, 8, 20), "models/p_ump45.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-23, -8, -20), (float *)&Vector(23, 8, 20), "models/p_tmp.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-23, -8, -20), (float *)&Vector(23, 8, 20), "models/p_p90.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_ak47.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_aug.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_awp.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_g3sg1.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_sg550.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_m4a1.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_scout.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_sg552.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_famas.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-31, -8, -21), (float *)&Vector(31, 12, 31), "models/p_galil.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-24, -10, -20), (float *)&Vector(25, 10, 20), "models/p_m249.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-7, -7, -15), (float *)&Vector(7, 7, 15), "models/p_c4.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-4, -8, -3), (float *)&Vector(3, 7, 3), "models/w_c4.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-7, -2, -18), (float *)&Vector(7, 2, 18), "models/p_flashbang.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-7, -2, -18), (float *)&Vector(7, 2, 18), "models/p_hegrenade.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-7, -2, -18), (float *)&Vector(7, 2, 18), "models/p_smokegrenade.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-5, -5, -5), (float *)&Vector(5, 5, 14), "models/w_flashbang.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-5, -5, -5), (float *)&Vector(5, 5, 14), "models/w_hegrenade.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-5, -5, -5), (float *)&Vector(5, 5, 14), "models/w_smokegrenade.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-7, -11, -18), (float *)&Vector(7, 6, 18), "models/p_knife.mdl");

	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_deagle.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_fiveseven.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_flashbang.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_glock18.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_hegrenade.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_knife.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_p228.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_smokegrenade.mdl");
	ENGINE_FORCE_UNMODIFIED(force_model_specifybounds, (float *)&Vector(-16, -8, -54), (float *)&Vector(16, 6, 24), "models/shield/p_shield_usp.mdl");

	PRECACHE_SOUND("common/wpn_hudoff.wav");
	PRECACHE_SOUND("common/wpn_hudon.wav");
	PRECACHE_SOUND("common/wpn_moveselect.wav");
	PRECACHE_SOUND("common/wpn_select.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");
	PRECACHE_SOUND("player/geiger6.wav");
	PRECACHE_SOUND("player/geiger5.wav");
	PRECACHE_SOUND("player/geiger4.wav");
	PRECACHE_SOUND("player/geiger3.wav");
	PRECACHE_SOUND("player/geiger2.wav");
	PRECACHE_SOUND("player/geiger1.wav");

	if (giPrecacheGrunt)
		UTIL_PrecacheOther("enemy_terrorist");

	g_iShadowSprite = PRECACHE_MODEL("sprites/shadow_circle.spr");

	PRECACHE_MODEL("sprites/wall_puff1.spr");
	PRECACHE_MODEL("sprites/wall_puff2.spr");
	PRECACHE_MODEL("sprites/wall_puff3.spr");
	PRECACHE_MODEL("sprites/wall_puff4.spr");
	PRECACHE_MODEL("sprites/black_smoke1.spr");
	PRECACHE_MODEL("sprites/black_smoke2.spr");
	PRECACHE_MODEL("sprites/black_smoke3.spr");
	PRECACHE_MODEL("sprites/black_smoke4.spr");
	PRECACHE_MODEL("sprites/fast_wallpuff1.spr");
	PRECACHE_MODEL("sprites/pistol_smoke1.spr");
	PRECACHE_MODEL("sprites/pistol_smoke2.spr");
	PRECACHE_MODEL("sprites/rifle_smoke1.spr");
	PRECACHE_MODEL("sprites/rifle_smoke2.spr");
	PRECACHE_MODEL("sprites/rifle_smoke3.spr");

	PRECACHE_GENERIC("sprites/scope_arc.tga");
	PRECACHE_GENERIC("sprites/scope_arc_nw.tga");
	PRECACHE_GENERIC("sprites/scope_arc_ne.tga");
	PRECACHE_GENERIC("sprites/scope_arc_sw.tga");

	m_usResetDecals = g_engfuncs.pfnPrecacheEvent(1, "events/decal_reset.sc");
}

const char *GetGameDescription(void)
{
	return "Counter-Strike";
}

void Sys_Error (const char *error_string)
{
}

void PlayerCustomization(edict_t *pEntity, customization_t *pCust)
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (!pPlayer)
	{
		ALERT(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		ALERT(at_console, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
		case t_decal: pPlayer->SetCustomDecalFrames(pCust->nUserData2); break;
		case t_sound:
		case t_skin:
		case t_model: break;
		default: ALERT(at_console, "PlayerCustomization:  Unknown customization type!\n"); break;
	}
}

void SpectatorConnect(edict_t *pEntity)
{
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorConnect();
}

void SpectatorDisconnect(edict_t *pEntity)
{
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorDisconnect();
}

void SpectatorThink(edict_t *pEntity)
{
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorThink();
}

void SetupVisibility(edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas)
{
	edict_t *pView = pClient;

	if (pViewEntity)
		pView = pViewEntity;

	if (pClient->v.flags & FL_PROXY)
	{
		*pvs = NULL;
		*pas = NULL;
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pClient);

	if (pPlayer->pev->iuser2)
	{
		if (pPlayer->m_hObserverTarget)
		{
			if (pPlayer->m_afPhysicsFlags & PFLAG_USING)
			{
				pView = pPlayer->m_hObserverTarget->edict();
				UTIL_SetOrigin(pPlayer->pev, pPlayer->m_hObserverTarget->pev->origin);
			}
		}
	}

	Vector org = pView->v.origin + pView->v.view_ofs;

	if (pView->v.flags & FL_DUCKING)
		org = org + (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);

	*pvs = ENGINE_SET_PVS((float *)&org);
	*pas = ENGINE_SET_PAS((float *)&org);
}

typedef struct
{
	float m_fTimeEnteredPVS;
}
ENTITYPVSSTATUS;

typedef struct
{
	ENTITYPVSSTATUS m_Status[1380];
	int headnode;
	int num_leafs;
	short leafnums[MAX_ENT_LEAFS];
}
PLAYERPVSSTATUS;

PLAYERPVSSTATUS g_PVSStatus[32];

void MarkEntityInPVS(int clientnum, int entitynum, float time, bool inpvs = false)
{
	if (!inpvs)
		g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS = time;
	else
		g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS = 0;
}

void ResetPlayerPVS(edict_t *client, int clientnum)
{
	memset(&g_PVSStatus[clientnum], 0, sizeof(g_PVSStatus[clientnum]));
	g_PVSStatus[clientnum].headnode = client->headnode;
	g_PVSStatus[clientnum].num_leafs = client->num_leafs;
	memcpy(&(g_PVSStatus[clientnum].leafnums), client->leafnums, sizeof(g_PVSStatus[clientnum].leafnums));
}

bool CheckEntityRecentlyInPVS(int clientnum, int entitynum, float time)
{
	if (g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS && time > g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS + 1.0)
		return true;

	return false;
}

bool CheckPlayerPVSLeafChanged(edict_t *client, int clientnum)
{
	if (g_PVSStatus[clientnum].headnode != client->headnode || g_PVSStatus[clientnum].num_leafs != client->num_leafs)
		return true;

	if (g_PVSStatus[clientnum].num_leafs > 0)
	{
		for (int i = 0; i < g_PVSStatus[clientnum].num_leafs; i++)
		{
			if (client->leafnums[i] != g_PVSStatus[clientnum].leafnums[i])
				return true;
		}
	}

	return false;
}

#include "entity_state.h"

int AddToFullPack(struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet)
{
	if ((ent->v.effects == EF_NODRAW) && ent != host)
		return 0;

	if (!ent->v.modelindex || !STRING(ent->v.model))
		return 0;

	if ((ent->v.flags & FL_SPECTATOR) && ent != host)
		return 0;

	int i;
	int hostindex = ENTINDEX(host) - 1;

	if (CheckPlayerPVSLeafChanged(host, hostindex))
		ResetPlayerPVS(host, hostindex);

	if (ent != host)
	{
		if (!CheckEntityRecentlyInPVS(hostindex, e, gpGlobals->time))
		{
			if (!ENGINE_CHECK_VISIBILITY((const struct edict_s *)ent, pSet))
			{
				MarkEntityInPVS(hostindex, e, gpGlobals->time, true);
				return 0;
			}

			MarkEntityInPVS(hostindex, e, gpGlobals->time);
		}
	}

	if (ent->v.flags & FL_SKIPLOCALHOST)
	{
		if ((hostflags & 1) && (ent->v.owner == host))
			return 0;
	}

	if (host->v.groupinfo)
	{
		UTIL_SetGroupTrace(host->v.groupinfo, GROUP_OP_AND);

		if (ent->v.groupinfo)
		{
			if (g_groupop == GROUP_OP_AND)
			{
				if (!(ent->v.groupinfo & host->v.groupinfo))
					return 0;
			}
			else if (g_groupop == GROUP_OP_NAND)
			{
				if (ent->v.groupinfo & host->v.groupinfo)
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	memset(state, 0, sizeof(*state));

	state->number = e;
	state->entityType = ENTITY_NORMAL;

	if (ent->v.flags & FL_CUSTOMENTITY)
		state->entityType = ENTITY_BEAM;

	state->animtime = (int)(1000.0 * ent->v.animtime) / 1000.0;

	memcpy(state->origin, ent->v.origin, 3 * sizeof(float));
	memcpy(state->angles, ent->v.angles, 3 * sizeof(float));
	memcpy(state->mins, ent->v.mins, 3 * sizeof(float));
	memcpy(state->maxs, ent->v.maxs, 3 * sizeof(float));
	memcpy(state->startpos, ent->v.startpos, 3 * sizeof(float));
	memcpy(state->endpos, ent->v.endpos, 3 * sizeof(float));

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;
	state->modelindex = ent->v.modelindex;
	state->frame = ent->v.frame;
	state->skin = ent->v.skin;
	state->effects = ent->v.effects;

	if (!player && ent->v.animtime && !ent->v.velocity[0] && !ent->v.velocity[1] && !ent->v.velocity[2])
		state->eflags |= EFLAG_SLERP;

	state->scale = ent->v.scale;
	state->solid = ent->v.solid;
	state->colormap = ent->v.colormap;
	state->movetype = ent->v.movetype;
	state->sequence = ent->v.sequence;
	state->framerate = ent->v.framerate;
	state->body = ent->v.body;

	for (i = 0; i < 4; i++)
		state->controller[i] = ent->v.controller[i];

	for (i = 0; i < 2; i++)
		state->blending[i] = ent->v.blending[i];

	state->rendermode = ent->v.rendermode;
	state->renderamt = (int)ent->v.renderamt;
	state->renderfx = ent->v.renderfx;
	state->rendercolor.r = (byte)ent->v.rendercolor.x;
	state->rendercolor.g = (byte)ent->v.rendercolor.y;
	state->rendercolor.b = (byte)ent->v.rendercolor.z;
	state->aiment = 0;

	if (ent->v.aiment)
		state->aiment = ENTINDEX(ent->v.aiment);

	state->owner = 0;

	if (ent->v.owner)
	{
		int owner = ENTINDEX(ent->v.owner);

		if (owner >= 1 && owner <= gpGlobals->maxClients)
			state->owner = owner;
	}

	if (!player)
		state->playerclass = ent->v.playerclass;

	if (player)
	{
		memcpy(state->basevelocity, ent->v.basevelocity, 3 * sizeof(float));

		state->weaponmodel = MODEL_INDEX(STRING(ent->v.weaponmodel));
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction = ent->v.friction;
		state->gravity = ent->v.gravity;
		state->usehull = (ent->v.flags & FL_DUCKING) ? 1 : 0;
		state->health = (int)ent->v.health;
	}

	state->iuser4 = ent->v.iuser4;
	return 1;
}

void CreateBaseline(int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs)
{
	baseline->origin = entity->v.origin;
	baseline->angles = entity->v.angles;
	baseline->frame = entity->v.frame;
	baseline->skin = (short)entity->v.skin;
	baseline->rendermode = (byte)entity->v.rendermode;
	baseline->renderamt = (byte)entity->v.renderamt;
	baseline->rendercolor.r = (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g = (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b = (byte)entity->v.rendercolor.z;
	baseline->renderfx = (byte)entity->v.renderfx;

	if (player)
	{
		baseline->mins = player_mins;
		baseline->maxs = player_maxs;
		baseline->colormap = eindex;
		baseline->modelindex = playermodelindex;
		baseline->friction = 1;
		baseline->movetype = MOVETYPE_WALK;
		baseline->scale = entity->v.scale;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1;
		baseline->gravity = 1;
	}
	else
	{
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;
		baseline->colormap = 0;
		baseline->modelindex = entity->v.modelindex;
		baseline->movetype = entity->v.movetype;
		baseline->scale = entity->v.scale;
		baseline->solid = entity->v.solid;
		baseline->framerate = entity->v.framerate;
		baseline->gravity = entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int field;
}
entity_field_alias_t;

#define FIELD_ORIGIN0 0
#define FIELD_ORIGIN1 1
#define FIELD_ORIGIN2 2
#define FIELD_ANGLES0 3
#define FIELD_ANGLES1 4
#define FIELD_ANGLES2 5

static entity_field_alias_t entity_field_alias[] =
{
	{ "origin[0]", 0 },
	{ "origin[1]", 0 },
	{ "origin[2]", 0 },
	{ "angles[0]", 0 },
	{ "angles[1]", 0 },
	{ "angles[2]", 0 },
};

void Entity_FieldInit(struct delta_s *pFields)
{
	entity_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN0].name);
	entity_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN1].name);
	entity_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ORIGIN2].name);
	entity_field_alias[FIELD_ANGLES0].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES0].name);
	entity_field_alias[FIELD_ANGLES1].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES1].name);
	entity_field_alias[FIELD_ANGLES2].field = DELTA_FINDFIELD(pFields, entity_field_alias[FIELD_ANGLES2].name);
}

void Entity_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	static int initialized = 0;

	if (!initialized)
	{
		Entity_FieldInit(pFields);
		initialized = 1;
	}

	entity_state_t *f = (entity_state_t *)from;
	entity_state_t *t = (entity_state_t *)to;
	int localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();

	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if (t->impacttime != 0 && t->starttime != 0)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES2].field);
	}

	if (t->movetype == MOVETYPE_FOLLOW && t->aiment != 0)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

static entity_field_alias_t player_field_alias[] =
{
	{ "origin[0]", 0 },
	{ "origin[1]", 0 },
	{ "origin[2]", 0 },
};

void Player_FieldInit(struct delta_s *pFields)
{
	player_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN0].name);
	player_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN1].name);
	player_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN2].name);
}

void Player_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	static int initialized = 0;

	if (!initialized)
	{
		Player_FieldInit(pFields);
		initialized = 1;
	}

	entity_state_t *f = (entity_state_t *)from;
	entity_state_t *t = (entity_state_t *)to;
	int localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();

	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if (t->movetype == MOVETYPE_FOLLOW && t->aiment != 0)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

#define CUSTOMFIELD_ORIGIN0 0
#define CUSTOMFIELD_ORIGIN1 1
#define CUSTOMFIELD_ORIGIN2 2
#define CUSTOMFIELD_ANGLES0 3
#define CUSTOMFIELD_ANGLES1 4
#define CUSTOMFIELD_ANGLES2 5
#define CUSTOMFIELD_SKIN 6
#define CUSTOMFIELD_SEQUENCE 7
#define CUSTOMFIELD_ANIMTIME 8

entity_field_alias_t custom_entity_field_alias[] =
{
	{ "origin[0]", 0 },
	{ "origin[1]", 0 },
	{ "origin[2]", 0 },
	{ "angles[0]", 0 },
	{ "angles[1]", 0 },
	{ "angles[2]", 0 },
	{ "skin", 0 },
	{ "sequence", 0 },
	{ "animtime", 0 },
};

void Custom_Entity_FieldInit(struct delta_s *pFields)
{
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].name);
	custom_entity_field_alias[CUSTOMFIELD_SKIN].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].name);
	custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field= DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].name);
	custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field= DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].name);
}

void Custom_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	static int initialized = 0;

	if (!initialized)
	{
		Custom_Entity_FieldInit(pFields);
		initialized = 1;
	}

	entity_state_t *f = (entity_state_t *)from;
	entity_state_t *t = (entity_state_t *)to;
	int beamType = t->rendermode & 0xF;

	if (beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field);
	}

	if (beamType != BEAM_POINTS)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field);
	}

	if (beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field);
	}

	if ((int)f->animtime == (int)t->animtime)
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field);
}

void RegisterEncoders(void)
{
	DELTA_ADDENCODER("Entity_Encode", Entity_Encode);
	DELTA_ADDENCODER("Custom_Encode", Custom_Encode);
	DELTA_ADDENCODER("Player_Encode", Player_Encode);
}

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
//GCC почему-то не видит этот дефайн в extdll.h

int GetWeaponData(struct edict_s *player, struct weapon_data_s *info)
{
#ifdef CLIENT_WEAPONS
	entvars_t *pev = &player->v;
	CBasePlayer *pl = (CBasePlayer *)CBasePlayer::Instance(pev);
	memset(info, 0, 32 * sizeof(weapon_data_t));

	if (!pl)
		return 1;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (pl->m_rgpPlayerItems[i])
		{
			CBasePlayerItem *pPlayerItem = pl->m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				
				CBasePlayerWeapon *gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();
				
				if (gun && gun->UseDecrement())
				{
					ItemInfo II;
					memset(&II, 0, sizeof(II));
					gun->GetItemInfo(&II);

					if (II.iId >= 0 && II.iId < 32)
					{
						weapon_data_t *item = &info[II.iId];
						item->m_iId = II.iId;
						item->m_iClip = gun->m_iClip;
						item->m_flTimeWeaponIdle = max(gun->m_flTimeWeaponIdle, -0.001);
						item->m_flNextPrimaryAttack = max(gun->m_flNextPrimaryAttack, -0.001);
						item->m_flNextSecondaryAttack = max(gun->m_flNextSecondaryAttack, -0.001);
						item->m_flNextReload = max(gun->m_flNextReload, -0.001);
						item->m_fInReload = gun->m_fInReload;
						item->m_fInSpecialReload = gun->m_fInSpecialReload;
						item->m_fInZoom = gun->m_iShotsFired;
						item->m_fAimedDamage = gun->m_flLastFire;
						item->m_iWeaponState = gun->m_iWeaponState;
						item->fuser2 = gun->m_flStartThrow;
						item->fuser3 = gun->m_flReleaseThrow;
						item->iuser1 = gun->m_iSwing;
					}
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}
#endif

	return 1;
}

void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd)
{
	entvars_t *pev = (entvars_t *)&ent->v;
	CBasePlayer *pl = (CBasePlayer *)CBasePlayer::Instance(pev);
	entvars_t *pevOrg = NULL;

	if (pl->pev->iuser1 == OBS_IN_EYE && pl->m_hObserverTarget != NULL)
	{
		pevOrg = pev;
		pev = pl->m_hObserverTarget->pev;
		pl = (CBasePlayer *)CBasePlayer::Instance(pev);
	}

	cd->flags = pev->flags;
	cd->health = pev->health;
	cd->viewmodel = MODEL_INDEX(STRING(pev->viewmodel));
	cd->waterlevel = pev->waterlevel;
	cd->watertype = pev->watertype;
	cd->weapons = pev->weapons;
	cd->origin = pev->origin;
	cd->velocity = pev->velocity;
	cd->view_ofs = pev->view_ofs;
	cd->punchangle = pev->punchangle;
	cd->bInDuck = pev->bInDuck;
	cd->flTimeStepSound = pev->flTimeStepSound;
	cd->flDuckTime = pev->flDuckTime;
	cd->flSwimTime = pev->flSwimTime;
	cd->waterjumptime = (const int)pev->teleport_time;

	strcpy(cd->physinfo, ENGINE_GETPHYSINFO(ent));

	cd->maxspeed = pev->maxspeed;
	cd->fov = pev->fov;
	cd->weaponanim = pev->weaponanim;
	cd->pushmsec = pev->pushmsec;

	if (pevOrg)
	{
		cd->iuser1 = pevOrg->iuser1;
		cd->iuser2 = pevOrg->iuser2;
		cd->iuser3 = pevOrg->iuser3;
	}
	else
	{
		cd->iuser1 = pev->iuser1;
		cd->iuser2 = pev->iuser2;
		cd->iuser3 = pev->iuser3;
	}

	cd->fuser1 = pev->fuser1;
	cd->fuser2 = pev->fuser2;
	cd->fuser3 = pev->fuser3;

#ifdef CLIENT_WEAPONS
	if (sendweapons && pl)
	{
		cd->ammo_shells = pl->ammo_buckshot;
		cd->ammo_nails = pl->ammo_9mm;
		cd->ammo_cells = pl->ammo_556nato;
		cd->ammo_rockets = pl->ammo_556natobox;
		cd->vuser2.x = pl->ammo_762nato;
		cd->vuser2.y = pl->ammo_45acp;
		cd->vuser2.z = pl->ammo_50ae;
		cd->vuser3.x = pl->ammo_338mag;
		cd->vuser3.y = pl->ammo_57mm;
		cd->vuser3.z = pl->ammo_357sig;
		cd->m_flNextAttack = pl->m_flNextAttack;

		int iUser3 = 0;

		if (pl->m_bCanShoot == true && !pl->m_bIsDefusing)
			iUser3 |= IUSER3_CANSHOOT;

		if (g_pGameRules->IsFreezePeriod())
			iUser3 |= IUSER3_FREEZETIMEOVER;
		else
			iUser3 &= ~IUSER3_FREEZETIMEOVER;

		if (pl->HasShield())
			iUser3 |= IUSER3_HOLDINGSHIELD;
		else
			iUser3 &= ~IUSER3_HOLDINGSHIELD;

		if (!pl->IsObserver() && !pevOrg)
			cd->iuser3 = iUser3;

		if (pl->m_signals.GetState() & SIGNAL_BOMB)
			iUser3 |= IUSER3_INBOMBZONE;
		else
			iUser3 &= ~IUSER3_INBOMBZONE;

		if (pl->m_pActiveItem)
		{
			CBasePlayerWeapon *gun = (CBasePlayerWeapon *)pl->m_pActiveItem->GetWeaponPtr();

			if (gun && gun->UseDecrement())
			{
				ItemInfo II;
				memset(&II, 0, sizeof(II));
				gun->GetItemInfo(&II);

				cd->m_iId = II.iId;
				cd->vuser4.x = gun->m_iPrimaryAmmoType;
				cd->vuser4.y = pl->m_rgAmmo[gun->m_iSecondaryAmmoType];
			}
		}
	}
#endif
}

void CmdStart(const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed)
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = (CBasePlayer *)CBasePlayer::Instance(pev);

	if (!pl)
		return;

	if (pl->pev->groupinfo != 0)
		UTIL_SetGroupTrace(pl->pev->groupinfo, GROUP_OP_AND);

	pl->random_seed = random_seed;
}

void CmdEnd(const edict_t *player)
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = (CBasePlayer *)CBasePlayer::Instance(pev);

	if (!pl)
		return;

	if (pl->pev->groupinfo != 0)
		UTIL_UnsetGroupTrace();

	if (pev->flags & FL_DUCKING)
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
}

int ConnectionlessPacket(const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size)
{
	*response_buffer_size = 0;
	return 0;
}

int GetHullBounds(int hullnumber, float *mins, float *maxs)
{
	int iret = 0;

	switch (hullnumber)
	{
		case 0:
		{
			mins = VEC_HULL_MIN;
			maxs = VEC_HULL_MAX;
			iret = 1;
			break;
		}

		case 1:
		{
			mins = VEC_DUCK_HULL_MIN;
			maxs = VEC_DUCK_HULL_MAX;
			iret = 1;
			break;
		}

		case 2:
		{
			mins = Vector(0, 0, 0);
			maxs = Vector(0, 0, 0);
			iret = 1;
			break;
		}
	}

	return iret;
}

void CreateInstancedBaselines(void)
{
	entity_state_t state;
	memset(&state, 0, sizeof(state)); //-V597
}

int InconsistentFile(const edict_t *player, const char *filename, char *disconnect_message)
{
	if (CVAR_GET_FLOAT("mp_consistency") != 1)
		return 0;

	sprintf(disconnect_message, "Server is enforcing file consistency for %s\n", filename);
	return 1;
}

int AllowLagCompensation(void)
{
	return 1;
}

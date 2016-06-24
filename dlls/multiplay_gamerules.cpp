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
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "skill.h"
#include "game.h"
#include "items.h"
#include "hltv.h"
#include "client.h"
#include "hostage.h"
#include "mapinfo.h"
#include "shake.h"
#include "trains.h"
#include "vehicle.h"
#include <ctype.h>
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#include <stdint.h>

extern DLL_GLOBAL CHalfLifeMultiplay *g_pGameRules;
extern DLL_GLOBAL BOOL g_fGameOver;
extern float g_flTimeLimit;
extern float g_flResetTime;

extern void ClearBodyQue(void);
extern int CountTeamPlayers(int team);

extern int gmsgDeathMsg;
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;
extern int gmsgSendAudio;
extern int gmsgBombPickup;
extern int gmsgTeamScore;
extern int gmsgForceCam;
extern int gmsgRadar;
extern int gmsgLocation;

extern int gmsgSayText;
extern int gmsgGameMode;
extern int gmsgAllowSpec;
extern int gmsgShowTimer;
extern int gmsgHLTV;

extern unsigned short m_usResetDecals;
extern float g_flWeaponCheat;

extern int gmsgViewMode;
extern int gmsgCheckModels;
extern int gmsgTeamInfo;

#define ITEM_RESPAWN_TIME 30
#define WEAPON_RESPAWN_TIME 20
#define AMMO_RESPAWN_TIME 20

class CCStrikeGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
		if (pListener->m_iTeam != pTalker->m_iTeam)
			return false;

		BOOL pListenerAlive = pListener->IsAlive();
		BOOL pTalkerAlive = pTalker->IsAlive();

		if (pListener->IsObserver())
			return true;

		if (pListenerAlive)
		{
			if (!pTalkerAlive)
				return false;
		}
		else
		{
			if (pTalkerAlive)
				return true;
		}

		return pListenerAlive == pTalkerAlive;
	}
};

CCStrikeGameMgrHelper g_GameMgrHelper;

void Broadcast(const char *sentence)
{
	char text[256];

	if (!sentence)
		return;

	strcpy(text, "%!MRAD_");
	strcat(text, UTIL_VarArgs("%s", sentence));

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgSendAudio);
	WRITE_BYTE(0);
	WRITE_STRING(text);
	WRITE_SHORT(100);
	MESSAGE_END();
}

char *GetTeam(int teamNo)
{
	switch (teamNo)
	{
		case TEAM_TERRORIST: return "TERRORIST";
		case TEAM_CT: return "CT";
		case TEAM_SPECTATOR: return "SPECTATOR";
	}

	return "";
}

void EndRoundMessage(const char *sentence, int event)
{
	char *team = NULL;
	bool teamTriggered = true;

	UTIL_ClientPrintAll(HUD_PRINTCENTER, sentence);

	switch (event)
	{
		case Target_Bombed:
		case VIP_Assassinated:
		case Terrorists_Escaped:
		case Terrorists_Win:
		case Hostages_Not_Rescued:
		case VIP_Not_Escaped: team = GetTeam(TEAM_TERRORIST); break;
		case VIP_Escaped:
		case CTs_PreventEscape:
		case Escaping_Terrorists_Neutralized:
		case Bomb_Defused:
		case CTs_Win:
		case All_Hostages_Rescued:
		case Target_Saved:
		case Terrorists_Not_Escaped: team = GetTeam(TEAM_CT); break;
		default: teamTriggered = false; break;
	}

	if (g_pGameRules)
	{
		if (teamTriggered == true)
			UTIL_LogPrintf("Team \"%s\" triggered \"%s\" (CT \"%i\") (T \"%i\")\n", team, &sentence[1], g_pGameRules->m_iNumCTWins, g_pGameRules->m_iNumTerroristWins);
		else
			UTIL_LogPrintf("World triggered \"%s\" (CT \"%i\") (T \"%i\")\n", &sentence[1], g_pGameRules->m_iNumCTWins, g_pGameRules->m_iNumTerroristWins);
	}

	UTIL_LogPrintf("World triggered \"Round_End\"\n");
	if (g_pGameRules->ShouldRestart())
	{
		SERVER_COMMAND("quit\n");
		SERVER_EXECUTE2();
		/*I know, that this is a strange way to make full restart, but I can't find right way */
	}
}

void ReadMultiplayCvars(CHalfLifeMultiplay *mp);

cvar_t *sv_clienttrace;
CHalfLifeMultiplay *g_pMPGameRules;

CHalfLifeMultiplay::CHalfLifeMultiplay(void)
{
	int i;

	m_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);
	RefreshSkillData();

	m_flIntermissionStartTime = 0;
	m_flIntermissionEndTime = 0;
	m_fTeamCount = 0;
	m_iAccountTerrorist = m_iAccountCT = 0;
	m_iHostagesRescued = 0;
	m_iRoundWinStatus = 0;
	m_iNumTerroristWins = m_iNumCTWins = 0;
	m_pVIP = NULL;
	m_iNumTerrorist = m_iNumCT = 0;
	m_iNumSpawnableTerrorist = m_iNumSpawnableCT = 0;
	m_bMapHasCameras = 2;
	g_fGameOver = FALSE;
	m_iLoserBonus = 1400;
	m_iNumConsecutiveCTLoses = 0;
	m_iNumConsecutiveTerroristLoses = 0;
	m_iC4Guy = 0;
	m_bBombDefused = false;
	m_bTargetBombed = false;
	m_bFreezePeriod = TRUE;
	m_bLevelInitialized = false;
	m_tmNextPeriodicThink = 0;
	m_bFirstConnected = false;
	m_bCompleteReset = false;
	m_flRequiredEscapeRatio = 0.5;
	m_iNumEscapers = 0;
	m_bTCantBuy = m_bCTCantBuy = false;
	m_flBombRadius = 500;
	m_iTotalGunCount = 0;
	m_iTotalGrenadeCount = 0;
	m_iTotalArmourCount = 0;
	m_iConsecutiveVIP = 0;
	m_iUnBalancedRounds = 0;
	m_iNumEscapeRounds = 0;
	m_bRoundTerminating = false;
	g_iHostageNumber = 0;
	m_bBombDropped = FALSE;
	m_iMaxRounds = (int)CVAR_GET_FLOAT("mp_maxrounds");

	if (m_iMaxRounds < 0)
	{
		m_iMaxRounds = 0;
		CVAR_SET_FLOAT("mp_maxrounds", 0);
	}

	m_iTotalRoundsPlayed = 0;
	m_iMaxRoundsWon = (int)CVAR_GET_FLOAT("mp_winlimit");

	if (m_iMaxRoundsWon < 0)
	{
		m_iMaxRoundsWon = 0;
		CVAR_SET_FLOAT("mp_winlimit", 0);
	}

	for (i = 0; i < MAX_MAPS; i++)
		m_iMapVotes[i] = 0;

	m_iLastPick = 1;
	m_bMapHasEscapeZone = false;
	m_iMapHasVIPSafetyZone = 0;
	m_bMapHasBombZone = false;
	m_bMapHasRescueZone = false;
	m_iStoredSpectValue = allow_spectators.value;

	for (i = 0; i < MAX_VIPQUEUES; i++)
		VIPQueue[i] = NULL;

	CVAR_SET_FLOAT("cl_himodels", 0);
	ReadMultiplayCvars(this);

	m_iIntroRoundTime += 2;
	m_iRoundTimeSecs = m_iIntroRoundTime;
	m_bInCareerGame = false;
	m_fMaxIdlePeriod = m_iRoundTime * 2;

	if (IS_DEDICATED_SERVER())
	{
		CVAR_SET_FLOAT("pausable", 0);
	}
	else
	{
		if (IsCareer())
		{
		}
		else
		{
			CVAR_SET_FLOAT("pausable", 0);

			const char *lservercfgfile = CVAR_GET_STRING("lservercfgfile");

			if (lservercfgfile && *lservercfgfile)
			{
				char szCommand[256];
				ALERT(at_console, "Executing listen server config file\n");
				sprintf(szCommand, "exec %s\n", lservercfgfile);
				SERVER_COMMAND(szCommand);
			}
		}
	}

	static bool installedCommands = false;

	if (installedCommands == false)
	{
		g_engfuncs.pfnAddServerCommand("perf_test", loopPerformance);
		installedCommands = true;
	}

	m_fIntroRoundCount = m_fRoundCount = 0;
	m_bSkipSpawn = m_bInCareerGame;
	m_fCareerRoundMenuTime = 0;
	m_fCareerMatchMenuTime = 0;
	m_iCareerMatchWins = 0;
	m_iRoundWinDifference = CVAR_GET_FLOAT("mp_windifference");

	if (m_iRoundWinDifference < 1)
	{
		m_iRoundWinDifference = 1;
		CVAR_SET_FLOAT("mp_windifference", 1);
	}

	sv_clienttrace = CVAR_GET_POINTER("sv_clienttrace");
	g_pMPGameRules = this;
	m_bShouldRestart = FALSE;
}

void ReadMultiplayCvars(CHalfLifeMultiplay *mp)
{
	mp->m_iRoundTime = (int)(CVAR_GET_FLOAT("mp_roundtime") * 60);
	mp->m_iC4Timer = (int)CVAR_GET_FLOAT("mp_c4timer");
	mp->m_iIntroRoundTime = (int)CVAR_GET_FLOAT("mp_freezetime");
	mp->m_iLimitTeams = (int)CVAR_GET_FLOAT("mp_limitteams");

	if (mp->m_iRoundTime > 540)
	{
		CVAR_SET_FLOAT("mp_roundtime", 9);
		mp->m_iRoundTime = 540;
	}
	else if (mp->m_iRoundTime < 60)
	{
		CVAR_SET_FLOAT("mp_roundtime", 1);
		mp->m_iRoundTime = 60;
	}

	if (mp->m_iIntroRoundTime > 60)
	{
		CVAR_SET_FLOAT("mp_freezetime", 60);
		mp->m_iIntroRoundTime = 60;
	}
	else if (mp->m_iIntroRoundTime < 0)
	{
		CVAR_SET_FLOAT("mp_freezetime", 0);
		mp->m_iIntroRoundTime = 0;
	}

	if (mp->m_iC4Timer > 90)
	{
		CVAR_SET_FLOAT("mp_c4timer", 90);
		mp->m_iC4Timer = 90;
	}
	else if (mp->m_iC4Timer < 10)
	{
		CVAR_SET_FLOAT("mp_c4timer", 10);
		mp->m_iC4Timer = 10;
	}

	if (mp->m_iLimitTeams > 20)
	{
		CVAR_SET_FLOAT("mp_limitteams", 20);
		mp->m_iLimitTeams = 20;
	}
	else if (mp->m_iLimitTeams <= 0)
	{
		CVAR_SET_FLOAT("mp_limitteams", 0);
		mp->m_iLimitTeams = 20;
	}
}

void CHalfLifeMultiplay::RefreshSkillData(void)
{
	CGameRules::RefreshSkillData();

	gSkillData.agruntHealth = 12;
	gSkillData.apacheHealth = 12;
	gSkillData.bullsquidDmgWhip = 30;
	gSkillData.agruntDmgPunch = 40;
	gSkillData.barneyHealth = 100;
	gSkillData.bigmommaHealthFactor = 20;
	gSkillData.bigmommaDmgSlash = 20;
	gSkillData.bigmommaDmgBlast = 120;
}

void CHalfLifeMultiplay::RemoveGuns(void)
{
	CBaseEntity *toremove = NULL;

	while ((toremove = UTIL_FindEntityByClassname(toremove, "weaponbox")) != NULL)
		((CWeaponBox *)toremove)->Kill();

	toremove = NULL;

	while ((toremove = UTIL_FindEntityByClassname(toremove, "weapon_shield")) != NULL)
	{
		toremove->SetThink(&CBaseEntity::SUB_Remove);
		toremove->SetTouch(NULL);
		toremove->pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CHalfLifeMultiplay::CleanUpMap(void)
{
	CBaseEntity *torestart;
	CBaseEntity *toremove;
	int icount;

	torestart = UTIL_FindEntityByClassname(NULL, "cycler_sprite");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "cycler_sprite");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "light");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "light");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "func_breakable");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "func_breakable");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "func_door");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "func_door");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "func_water");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "func_water");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "func_door_rotating");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "func_door_rotating");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "func_tracktrain");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "func_tracktrain");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "func_vehicle");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "func_vehicle");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "func_train");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "func_train");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "armoury_entity");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "armoury_entity");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "ambient_generic");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "ambient_generic");
	}

	torestart = UTIL_FindEntityByClassname(NULL, "env_sprite");

	while (torestart)
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname(torestart, "env_sprite");
	}

	icount = 0;
	toremove = UTIL_FindEntityByClassname(NULL, "grenade");

	while (toremove && icount < 20)
	{
		UTIL_Remove(toremove);
		toremove = UTIL_FindEntityByClassname(toremove, "grenade");
		icount++;
	}

	toremove = UTIL_FindEntityByClassname(NULL, "item_thighpack");

	while (toremove)
	{
		UTIL_Remove(toremove);
		toremove = UTIL_FindEntityByClassname(toremove, "item_thighpack");
	}

	RemoveGuns();
	PLAYBACK_EVENT(FEV_GLOBAL | FEV_RELIABLE, 0, m_usResetDecals);
}

void CHalfLifeMultiplay::GiveC4(void)
{
	if (++m_iC4Guy > m_iNumTerrorist)
		m_iC4Guy = 1;

	int nums = 0;
	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		if (!pEntity->IsPlayer())
			continue;

		if (pEntity->pev->flags == FL_DORMANT)
			continue;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pPlayer->pev->deadflag != DEAD_NO || pPlayer->m_iTeam != TEAM_TERRORIST)
			continue;

		if (++nums == m_iC4Guy)
		{
			pPlayer->m_bHasC4 = true;
			pPlayer->GiveNamedItem("weapon_c4");
			pPlayer->SetBombIcon(FALSE);
			pPlayer->pev->body = 1;
			pPlayer->m_flDisplayHistory |= DHF_BOMB_RETRIEVED;
			pPlayer->HintMessage("#Hint_you_have_the_bomb", FALSE, TRUE);
			UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Spawned_With_The_Bomb\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));
		}
	}

	CBasePlayer *pPlayer = NULL;

	while ((pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (FNullEnt(pPlayer->edict()))
			break;

		if (pPlayer->m_iTeam != TEAM_CT && pPlayer->IsBombGuy())
			return;
	}

	m_iC4Guy = 0;
	pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		if (!pEntity->IsPlayer())
			continue;

		if (pEntity->pev->flags == FL_DORMANT)
			continue;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pPlayer->pev->deadflag != DEAD_NO || pPlayer->m_iTeam != TEAM_TERRORIST)
			continue;

		pPlayer->m_bHasC4 = true;
		pPlayer->GiveNamedItem("weapon_c4");
		pPlayer->SetBombIcon(FALSE);
		pPlayer->pev->body = 1;
		pPlayer->m_flDisplayHistory |= DHF_BOMB_RETRIEVED;
		pPlayer->HintMessage("#Hint_you_have_the_bomb", FALSE, TRUE);
		UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Spawned_With_The_Bomb\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));
		g_pGameRules->m_bBombDropped = false;
		break;
	}
}

void CHalfLifeMultiplay::TerminateRound(float tmDelay, int iWinStatus)
{
	g_pGameRules->m_iRoundWinStatus = iWinStatus;
	g_pGameRules->m_bRoundTerminating = true;
	g_pGameRules->m_fTeamCount = gpGlobals->time + tmDelay;
}

void CHalfLifeMultiplay::QueueCareerRoundEndMenu(float tmDelay, int iWinStatus)
{

}

void CHalfLifeMultiplay::CheckWinConditions(void)
{
	if (m_bFirstConnected && m_iRoundWinStatus)
		return;

	int NumDeadCT, NumDeadTerrorist, NumAliveTerrorist, NumAliveCT;
	BOOL bNeededPlayers = FALSE;
	InitializePlayerCounts(NumAliveTerrorist, NumAliveCT, NumDeadTerrorist, NumDeadCT);

	if (NeededPlayersCheck(bNeededPlayers))
		return;

	if (VIPRoundEndCheck(bNeededPlayers))
		return;

	if (PrisonRoundEndCheck(NumAliveTerrorist, NumAliveCT, NumDeadTerrorist, NumDeadCT, bNeededPlayers))
		return;

	if (BombRoundEndCheck(bNeededPlayers))
		return;

	if (TeamExterminationCheck(NumAliveTerrorist, NumAliveCT, NumDeadTerrorist, NumDeadCT, bNeededPlayers))
		return;

	if (HostageRescueRoundEndCheck(bNeededPlayers))
		return;
}

void CHalfLifeMultiplay::InitializePlayerCounts(int &NumAliveTerrorist, int &NumAliveCT, int &NumDeadTerrorist, int &NumDeadCT)
{
	NumAliveTerrorist = NumAliveCT = NumDeadCT = NumDeadTerrorist = 0;
	m_iNumTerrorist = m_iNumCT = m_iNumSpawnableTerrorist = m_iNumSpawnableCT = 0;
	m_iHaveEscaped = 0;

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pEntity->pev->flags == FL_DORMANT)
			continue;

		switch (pPlayer->m_iTeam)
		{
			case TEAM_CT:
			{
				m_iNumCT++;

				if (pPlayer->m_iMenu != Menu_ChooseAppearance)
					m_iNumSpawnableCT++;

				if (pPlayer->pev->deadflag != DEAD_NO)
					NumDeadCT++;
				else
					NumAliveCT++;

				break;
			}

			case TEAM_TERRORIST:
			{
				m_iNumTerrorist++;

				if (pPlayer->m_iMenu != Menu_ChooseAppearance)
					m_iNumSpawnableTerrorist++;

				if (pPlayer->pev->deadflag != DEAD_NO)
					NumDeadTerrorist++;
				else
					NumAliveTerrorist++;

				if (pPlayer->m_bEscaped == true)
					m_iHaveEscaped++;

				break;
			}
		}
	}
}

BOOL CHalfLifeMultiplay::NeededPlayersCheck(BOOL &bNeededPlayers)
{
	if (!m_iNumSpawnableTerrorist || !m_iNumSpawnableCT)
	{
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "#Game_scoring");
		bNeededPlayers = TRUE;
		m_bFirstConnected = false;
	}

	if (!m_bFirstConnected && m_iNumSpawnableTerrorist != 0 && m_iNumSpawnableCT != 0)
	{
		if (IsCareer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(gpGlobals->maxClients);

			if (!pPlayer || !pPlayer->IsBot())
				return TRUE;
		}

		UTIL_LogPrintf("World triggered \"Game_Commencing\"\n");
		m_bFreezePeriod = FALSE;
		m_bCompleteReset = true;

		EndRoundMessage("#Game_Commencing", Round_Draw);

		if (IsCareer() == FALSE)
			TerminateRound(3, WINSTATUS_DRAW);
		else
			TerminateRound(0, WINSTATUS_DRAW);

		m_bFirstConnected = true;
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::VIPRoundEndCheck(BOOL bNeededPlayers)
{
	if (m_iMapHasVIPSafetyZone != 1)
		return FALSE;

	if (!m_pVIP)
		return FALSE;

	if (m_pVIP->m_bEscaped == true)
	{
		Broadcast("ctwin");
		m_iAccountCT += 3500;

		if (!bNeededPlayers)
		{
			m_iNumCTWins++;
			UpdateTeamScores();
		}

		MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
		WRITE_BYTE(9);
		WRITE_BYTE(DRC_CMD_EVENT);
		WRITE_SHORT(ENTINDEX(ENT(m_pVIP->pev)));
		WRITE_SHORT(0);
		WRITE_LONG(15 | DRC_FLAG_FINAL);
		MESSAGE_END();

		EndRoundMessage("#VIP_Escaped", VIP_Escaped);
		TerminateRound(5, WINSTATUS_CT);
		return TRUE;
	}
	else if (m_pVIP->pev->deadflag != DEAD_NO)
	{
		Broadcast("terwin");
		m_iAccountTerrorist += 3250;

		if (!bNeededPlayers)
		{
			m_iNumTerroristWins++;
			UpdateTeamScores();
		}

		EndRoundMessage("#VIP_Assassinated", VIP_Assassinated);
		TerminateRound(5, WINSTATUS_TERRORIST);
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::PrisonRoundEndCheck(int NumAliveTerrorist, int NumAliveCT, int NumDeadTerrorist, int NumDeadCT, BOOL bNeededPlayers)
{
	if (m_bMapHasEscapeZone != true)
		return FALSE;

	float flEscapeRatio = (float)m_iHaveEscaped / (float)m_iNumEscapers;

	if (flEscapeRatio >= m_flRequiredEscapeRatio)
	{
		Broadcast("terwin");
		m_iAccountTerrorist += 3150;

		if (!bNeededPlayers)
		{
			m_iNumTerroristWins++;
			UpdateTeamScores();
		}

		EndRoundMessage("#Terrorists_Escaped", Terrorists_Escaped);
		TerminateRound(5, WINSTATUS_TERRORIST);

		if (IsCareer())
			QueueCareerRoundEndMenu(5, WINSTATUS_TERRORIST);

		return TRUE;
	}

	if (!NumAliveTerrorist && flEscapeRatio < m_flRequiredEscapeRatio)
	{
		Broadcast("ctwin");
		m_iAccountCT += (1 - flEscapeRatio) * 3500;

		if (!bNeededPlayers)
		{
			m_iNumCTWins++;
			UpdateTeamScores();
		}

		EndRoundMessage("#CTs_PreventEscape", CTs_PreventEscape);
		TerminateRound(5, WINSTATUS_CT);

		if (IsCareer())
			QueueCareerRoundEndMenu(5, WINSTATUS_CT);

		return TRUE;
	}
	else if (!NumAliveTerrorist && NumDeadTerrorist && m_iNumSpawnableCT > 0)
	{
		Broadcast("ctwin");
		m_iAccountCT += (1 - flEscapeRatio) * 3250;

		if (!bNeededPlayers)
		{
			m_iNumCTWins++;
			UpdateTeamScores();
		}

		EndRoundMessage("#Escaping_Terrorists_Neutralized", Escaping_Terrorists_Neutralized);
		TerminateRound(5, WINSTATUS_CT);
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::BombRoundEndCheck(BOOL bNeededPlayers)
{
	if (m_bTargetBombed == true && m_bMapHasBombTarget == true)
	{
		Broadcast("terwin");
		m_iAccountTerrorist += 3500;

		if (!bNeededPlayers)
		{
			m_iNumTerroristWins++;
			UpdateTeamScores();
		}

		EndRoundMessage("#Target_Bombed", Target_Bombed);
		TerminateRound(5, WINSTATUS_TERRORIST);
		return TRUE;
	}

	if (m_bBombDefused == true && m_bMapHasBombTarget == true)
	{
		Broadcast("ctwin");
		m_iAccountCT += 3250;
		m_iAccountTerrorist += 800;

		if (!bNeededPlayers)
		{
			m_iNumCTWins++;
			UpdateTeamScores();
		}

		EndRoundMessage("#Bomb_Defused", Bomb_Defused);
		TerminateRound(5, WINSTATUS_CT);
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::TeamExterminationCheck(int NumAliveTerrorist, int NumAliveCT, int NumDeadTerrorist, int NumDeadCT, BOOL bNeededPlayers)
{
	if (m_iNumCT > 0 && m_iNumSpawnableCT > 0 && m_iNumTerrorist > 0 && m_iNumSpawnableTerrorist > 0)
	{
		if (!NumAliveTerrorist && NumDeadTerrorist)
		{
			bool nowin = false;
			CGrenade *pGrenade = NULL;

			while ((pGrenade = (CGrenade *)UTIL_FindEntityByClassname(pGrenade, "grenade")))
			{
				if (pGrenade->m_bIsC4 == true && !pGrenade->m_bJustBlew)
					nowin = true;
			}

			if (!nowin)
			{
				Broadcast("ctwin");
				m_iAccountCT += m_bMapHasBombTarget ? 3250 : 3000;

				if (!bNeededPlayers)
				{
					m_iNumCTWins++;
					UpdateTeamScores();
				}

				EndRoundMessage("#CTs_Win", CTs_Win);
				TerminateRound(5, WINSTATUS_CT);
				return TRUE;
			}
		}
		else if (!NumAliveCT && NumDeadCT)
		{
			Broadcast("terwin");
			m_iAccountTerrorist += m_bMapHasBombTarget ? 3250 : 3000;

			if (!bNeededPlayers)
			{
				m_iNumTerroristWins++;
				UpdateTeamScores();
			}

			EndRoundMessage("#Terrorists_Win", Terrorists_Win);
			TerminateRound(5, WINSTATUS_TERRORIST);
			return TRUE;
		}
	}
	else if (!NumAliveCT && !NumAliveTerrorist)
	{
		EndRoundMessage("#Round_Draw", Round_Draw);
		Broadcast("rounddraw");
		TerminateRound(5, WINSTATUS_DRAW);
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::HostageRescueRoundEndCheck(BOOL bNeededPlayers)
{
	int iHostages = 0;
	bool bHostageAlive = false;
	CBaseEntity *hostage = NULL;

	while ((hostage = UTIL_FindEntityByClassname(hostage, "hostage_entity")) != NULL)
	{
		iHostages++;

		if (hostage->pev->takedamage == DAMAGE_YES)
			bHostageAlive = true;
	}

	if (bHostageAlive == false && iHostages > 0 && m_iHostagesRescued > (iHostages * 0.5))
	{
		Broadcast("ctwin");
		m_iAccountCT += 2500;

		if (!bNeededPlayers)
		{
			m_iNumCTWins++;
			UpdateTeamScores();
		}

		EndRoundMessage("#All_Hostages_Rescued", All_Hostages_Rescued);
		TerminateRound(5, WINSTATUS_CT);

		if (IsCareer())
		{
		}

		return TRUE;
	}

	return FALSE;
}

void CHalfLifeMultiplay::BalanceTeams(void)
{
	int iTeamToSwap = TEAM_UNASSIGNED;
	int iNumToSwap;

	if (m_iMapHasVIPSafetyZone == 1)
	{
		int iDesiredNumCT = ((m_iNumCT + m_iNumTerrorist) % 2 != 0) ? (int)((m_iNumCT + m_iNumTerrorist) * 0.55) + 1 : (int)((m_iNumCT + m_iNumTerrorist) / 2);
		int iDesiredNumTerrorist = (m_iNumCT + m_iNumTerrorist) - iDesiredNumCT;

		if (m_iNumCT < iDesiredNumCT)
		{
			iTeamToSwap = TEAM_TERRORIST;
			iNumToSwap = iDesiredNumCT - m_iNumCT;
		}
		else if (m_iNumTerrorist < iDesiredNumTerrorist)
		{
			iTeamToSwap = TEAM_CT;
			iNumToSwap = iDesiredNumTerrorist - m_iNumTerrorist;
		}
		else
			return;
	}
	else
	{
		if (m_iNumCT > m_iNumTerrorist)
		{
			iTeamToSwap = TEAM_CT;
			iNumToSwap = (m_iNumCT - m_iNumTerrorist) / 2;
		}
		else if (m_iNumTerrorist > m_iNumCT)
		{
			iTeamToSwap = TEAM_TERRORIST;
			iNumToSwap = (m_iNumTerrorist - m_iNumCT) / 2;
		}
		else
			return;
	}

	if (iNumToSwap > 4)
		iNumToSwap = 4;

	int iHighestUserID = -1;
	CBasePlayer *toSwap = NULL;
	CBaseEntity *pEntity = NULL;

	for (int i = 1; i <= iNumToSwap; i++)
	{
		iHighestUserID = 0;
		toSwap = NULL;

		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
		{
			if (FNullEnt(pEntity->edict()))
				break;

			if (pEntity->pev->flags == FL_DORMANT)
				continue;

			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

			if (pPlayer->m_iTeam != iTeamToSwap || GETPLAYERUSERID(ENT(pPlayer->pev)) <= iHighestUserID || m_pVIP == pPlayer)
				continue;

			iHighestUserID = GETPLAYERUSERID(ENT(pPlayer->pev));
			toSwap = pPlayer;
		}

		if (toSwap != NULL)
			toSwap->SwitchTeam();
	}
}

void CHalfLifeMultiplay::CheckMapConditions(void)
{
	if ((UTIL_FindEntityByClassname(NULL, "func_bomb_target")) != NULL)
	{
		m_bMapHasBombTarget = true;
		m_bMapHasBombZone = true;
	}
	else if ((UTIL_FindEntityByClassname(NULL, "info_bomb_target")) != NULL)
	{
		m_bMapHasBombTarget = true;
		m_bMapHasBombZone = false;
	}
	else
	{
		m_bMapHasBombTarget = false;
		m_bMapHasBombZone = false;
	}

	if ((UTIL_FindEntityByClassname(NULL, "func_hostage_rescue")) != NULL)
		m_bMapHasRescueZone = true;
	else
		m_bMapHasRescueZone = false;

	if ((UTIL_FindEntityByClassname(NULL, "func_buyzone")) != NULL)
		m_bMapHasBuyZone = true;
	else
		m_bMapHasBuyZone = false;

	if ((UTIL_FindEntityByClassname(NULL, "func_escapezone")) != NULL)
		m_bMapHasEscapeZone = true;
	else
		m_bMapHasEscapeZone = false;

	if ((UTIL_FindEntityByClassname(NULL, "func_vip_safetyzone")) != NULL)
		m_iMapHasVIPSafetyZone = 1;
	else
		m_iMapHasVIPSafetyZone = 2;
}

void CHalfLifeMultiplay::UpdateTeamScores(void)
{
	MESSAGE_BEGIN(MSG_ALL, gmsgTeamScore);
	WRITE_STRING(GetTeam(TEAM_CT));
	WRITE_SHORT(m_iNumCTWins);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamScore);
	WRITE_STRING(GetTeam(TEAM_TERRORIST));
	WRITE_SHORT(m_iNumTerroristWins);
	MESSAGE_END();
}

void CHalfLifeMultiplay::SwapAllPlayers(void)
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		if (pEntity->pev->flags == FL_DORMANT)
			continue;

		GetClassPtr((CBasePlayer *)pEntity->pev)->SwitchTeam();
	}

	int iTemp = m_iNumTerroristWins;
	m_iNumTerroristWins = m_iNumCTWins;
	m_iNumCTWins = iTemp;
	UpdateTeamScores();
}

void CHalfLifeMultiplay::RestartRound(void)
{
	m_iTotalRoundsPlayed++;
	ClearBodyQue();

	CVAR_SET_FLOAT("sv_accelerate", 5);
	CVAR_SET_FLOAT("sv_friction", 4);
	CVAR_SET_FLOAT("sv_stopspeed", 75);

	m_iNumCT = CountTeamPlayers(TEAM_CT);
	m_iNumTerrorist = CountTeamPlayers(TEAM_TERRORIST);

	if (m_bMapHasBombTarget)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgBombPickup);
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_ALL, gmsgShowTimer);
		MESSAGE_END();
	}

	MESSAGE_BEGIN(MSG_SPEC, gmsgHLTV);
	WRITE_BYTE(0);
	WRITE_BYTE(100 | 128);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_SPEC, gmsgHLTV);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	MESSAGE_END();

	if (CVAR_GET_FLOAT("mp_autoteambalance") != 0 && m_iUnBalancedRounds >= 1)
		BalanceTeams();

	if (m_iNumCT - m_iNumTerrorist >= 2 || m_iNumTerrorist - m_iNumCT >= 2)
		m_iUnBalancedRounds++;
	else
		m_iUnBalancedRounds = 0;

	if (CVAR_GET_FLOAT("mp_autoteambalance") != 0 && m_iUnBalancedRounds == 1)
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "#Auto_Team_Balance_Next_Round");

	if (m_bCompleteReset)
	{
		if (timelimit.value < 0)
			CVAR_SET_FLOAT("mp_timelimit", 0);

		g_flResetTime = gpGlobals->time;

		if (timelimit.value)
			g_flTimeLimit = gpGlobals->time + timelimit.value * 60;

		m_iTotalRoundsPlayed = 0;
		m_iMaxRounds = (int)CVAR_GET_FLOAT("mp_maxrounds");

		if (m_iMaxRounds < 0)
		{
			m_iMaxRounds = 0;
			CVAR_SET_FLOAT("mp_maxrounds", 0);
		}

		m_iMaxRoundsWon = (int)CVAR_GET_FLOAT("mp_winlimit");

		if (m_iMaxRoundsWon < 0)
		{
			m_iMaxRoundsWon = 0;
			CVAR_SET_FLOAT("mp_winlimit", 0);
		}

		m_iNumTerroristWins = 0;
		m_iNumCTWins = 0;
		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses = 0;
		UpdateTeamScores();

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);

			if (pPlayer && !FNullEnt(pPlayer->pev))
				pPlayer->Restart();
		}
	}

	m_bFreezePeriod = TRUE;
	m_bRoundTerminating = false;
	ReadMultiplayCvars(this);
	m_iRoundTimeSecs = m_iIntroRoundTime;
	m_fMaxIdlePeriod = m_iRoundTime * 2;

	CMapInfo *params = (CMapInfo *)UTIL_FindEntityByClassname(NULL, "info_map_parameters");

	if (params)
	{
		switch (params->m_iBuyingStatus)
		{
			case BUYING_EVERYONE:
			{
				m_bCTCantBuy = false;
				m_bTCantBuy = false;
				ALERT(at_console, "EVERYONE CAN BUY!\n");
				break;
			}

			case BUYING_ONLY_CT:
			{
				m_bCTCantBuy = false;
				m_bTCantBuy = true;
				ALERT(at_console, "Only CT's can buy!!\n");
				break;
			}

			case BUYING_ONLY_T:
			{
				m_bCTCantBuy = true;
				m_bTCantBuy = false;
				ALERT(at_console, "Only T's can buy!!\n");
				break;
			}

			case BUYING_NO_ONE:
			{
				m_bCTCantBuy = true;
				m_bTCantBuy = true;
				ALERT(at_console, "No one can buy!!\n");
				break;
			}

			default:
			{
				m_bCTCantBuy = false;
				m_bTCantBuy = false;
				break;
			}
		}

		m_flBombRadius = params->m_flBombRadius;
	}

	CheckMapConditions();

	if (m_bMapHasEscapeZone)
	{
		m_iHaveEscaped = 0;
		m_iNumEscapers = 0;

		if (m_iNumEscapeRounds >= 3)
		{
			SwapAllPlayers();
			m_iNumEscapeRounds = 0;
		}

		m_iNumEscapeRounds++;
	}

	if (m_iMapHasVIPSafetyZone == 1)
	{
		PickNextVIP();
		m_iConsecutiveVIP++;
	}

	int acct_tmp = 0;
	CHostage *pHostage = NULL;

	while ((pHostage = (CHostage *)UTIL_FindEntityByClassname(pHostage, "hostage_entity")) != NULL)
	{
		if (acct_tmp >= 2000)
			break;

		if (pHostage->pev->solid != SOLID_NOT)
		{
			acct_tmp += 150;

			if (pHostage->pev->deadflag == DEAD_DEAD)
				pHostage->pev->deadflag = DEAD_RESPAWNABLE;
		}

		pHostage->RePosition();
	}

	if (m_iRoundWinStatus == WINSTATUS_TERRORIST)
	{
		if (m_iNumConsecutiveTerroristLoses > 1)
			m_iLoserBonus = 1500;

		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses++;
	}
	else if (m_iRoundWinStatus == WINSTATUS_CT)
	{
		if (m_iNumConsecutiveCTLoses > 1)
			m_iLoserBonus = 1500;

		m_iNumConsecutiveCTLoses = 0;
		m_iNumConsecutiveTerroristLoses++;
	}

	if (m_iNumConsecutiveTerroristLoses > 1 && m_iLoserBonus < 3000)
		m_iLoserBonus += 500;

	if (m_iNumConsecutiveCTLoses > 1 && m_iLoserBonus < 3000)
		m_iLoserBonus += 500;

	if (m_iRoundWinStatus == WINSTATUS_TERRORIST)
	{
		m_iAccountTerrorist += acct_tmp;
		m_iAccountCT += m_iLoserBonus;
	}
	else if (m_iRoundWinStatus == WINSTATUS_CT)
	{
		m_iAccountCT += acct_tmp;

		if (!m_bMapHasEscapeZone)
			m_iAccountTerrorist += m_iLoserBonus;
	}

	m_iAccountCT += m_iHostagesRescued * 750;
	m_fIntroRoundCount = m_fRoundCount = gpGlobals->time;

	if (m_bCompleteReset)
	{
		m_iAccountTerrorist = m_iAccountCT = 0;
		m_iNumTerroristWins = 0;
		m_iNumCTWins = 0;
		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses = 0;
		m_iLoserBonus = 1400;
	}

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		if (pEntity->pev->flags == FL_DORMANT)
			continue;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		pPlayer->m_iNumSpawns = 0;
		pPlayer->m_bTeamChanged = false;

		if (!pPlayer->IsPlayer())
			pPlayer->SyncRoundTimer();

		if (pPlayer->m_iTeam == TEAM_CT)
		{
			if (pPlayer->m_bReceivesNoMoneyNextRound == false)
				pPlayer->AddAccount(m_iAccountCT);
		}
		else if (pPlayer->m_iTeam == TEAM_TERRORIST)
		{
			m_iNumEscapers++;

			if (pPlayer->m_bReceivesNoMoneyNextRound == false)
				pPlayer->AddAccount(m_iAccountTerrorist);

			if (m_bMapHasEscapeZone == true)
				pPlayer->m_bNotKilled = false;
		}

		if (pPlayer->m_iTeam != TEAM_UNASSIGNED && pPlayer->m_iTeam != TEAM_SPECTATOR)
		{
			if (pPlayer->m_bHasC4 == true)
				pPlayer->DropPlayerItem("weapon_c4");

			pPlayer->RoundRespawn();
		}
	}

	CleanUpMap();

	if (m_bMapHasBombTarget == true)
		GiveC4();

	m_flIntermissionEndTime = 0;
	m_flIntermissionStartTime = 0;
	m_fTeamCount = 0;
	m_iAccountTerrorist = m_iAccountCT = 0;
	m_iHostagesRescued = 0;
	m_iHostagesTouched = 0;
	m_iRoundWinStatus = 0;
	m_bBombDefused = false;
	m_bTargetBombed = false;
	m_bLevelInitialized = false;
	m_bCompleteReset = false;
}

BOOL CHalfLifeMultiplay::IsThereABomber(void)
{
	CBasePlayer *pPlayer = NULL;

	while ((pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (pPlayer->m_iTeam != TEAM_CT && pPlayer->IsBombGuy())
			return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::IsThereABomb(void)
{
	CGrenade *pWeaponC4 = NULL;
	bool bFoundBomb = false;

	while ((pWeaponC4 = (CGrenade *)UTIL_FindEntityByClassname(pWeaponC4, "grenade")) != NULL)
	{
		if (pWeaponC4->m_bIsC4)
		{
			bFoundBomb = true;
			break;
		}
	}

	if (bFoundBomb || (UTIL_FindEntityByClassname(NULL, "weapon_c4")) != NULL)
		return TRUE;

	return FALSE;
}

BOOL CHalfLifeMultiplay::TeamFull(int team_id)
{
	switch (team_id)
	{
		case TEAM_TERRORIST: return m_iNumTerrorist >= m_iSpawnPointCount_Terrorist;
		case TEAM_CT: return m_iNumCT >= m_iSpawnPointCount_CT;
	}

	return false;
}

BOOL CHalfLifeMultiplay::TeamStacked(int newTeam_id, int curTeam_id)
{
	if (newTeam_id == curTeam_id)
		return FALSE;

	if (m_iLimitTeams == 0)
		return FALSE;

	switch (newTeam_id)
	{
		case TEAM_TERRORIST:
		{
			if (curTeam_id != TEAM_UNASSIGNED && curTeam_id != TEAM_SPECTATOR)
			{
				if ((m_iNumTerrorist + 1) > (m_iNumCT + m_iLimitTeams - 1))
					return TRUE;
				else
					return FALSE;
			}
			else
			{
				if ((m_iNumTerrorist + 1) > (m_iNumCT + m_iLimitTeams))
					return TRUE;
				else
					return FALSE;
			}
		}

		case TEAM_CT:
		{
			if (curTeam_id != TEAM_UNASSIGNED && curTeam_id != TEAM_SPECTATOR)
			{
				if ((m_iNumCT + 1) > (m_iNumTerrorist + m_iLimitTeams - 1))
					return TRUE;
				else
					return FALSE;
			}
			else
			{
				if ((m_iNumCT + 1) > (m_iNumTerrorist + m_iLimitTeams))
					return TRUE;
				else
					return FALSE;
			}
		}
	}

	return FALSE;
}

void CHalfLifeMultiplay::StackVIPQueue(void)
{
	for (int i = MAX_VIPQUEUES - 2; i; i--)
	{
		if (VIPQueue[i - 1])
		{
			if (!VIPQueue[i])
			{
				VIPQueue[i] = VIPQueue[i + 1];
				VIPQueue[i + 1] = NULL;
			}
		}
		else
		{
			VIPQueue[i - 1] = VIPQueue[i];
			VIPQueue[i] = VIPQueue[i + 1];
			VIPQueue[i + 1] = NULL;
		}
	}
}

void CHalfLifeMultiplay::CheckVIPQueue(void)
{
	for (int i = 0; i < MAX_VIPQUEUES; i++)
	{
		if (VIPQueue[i] && VIPQueue[i]->m_iTeam != TEAM_CT)
			VIPQueue[i] = NULL;
	}

	StackVIPQueue();
}

BOOL CHalfLifeMultiplay::IsVIPQueueEmpty(void)
{
	CheckVIPQueue();
	return !VIPQueue[0] && !VIPQueue[1] && !VIPQueue[2] && !VIPQueue[3] && !VIPQueue[4];
}

BOOL CHalfLifeMultiplay::AddToVIPQueue(CBasePlayer *pPlayer)
{
	CheckVIPQueue();

	if (pPlayer->m_iTeam != TEAM_CT)
		return FALSE;

	int i;

	for (i = 0; i < MAX_VIPQUEUES; i++)
	{
		if (VIPQueue[i] == pPlayer)
		{
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Game_in_position", UTIL_dtos1(i + 1));
			return FALSE;
		}
	}

	for (i = 0; i < MAX_VIPQUEUES; i++)
	{
		if (!VIPQueue[i])
		{
			VIPQueue[i] = pPlayer;
			StackVIPQueue();
			ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Game_added_position", UTIL_dtos1(i + 1));
			return TRUE;
		}
	}

	ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#All_VIP_Slots_Full");
	return FALSE;
}

void CHalfLifeMultiplay::ResetCurrentVIP(void)
{
	char *model;

	switch (RANDOM_LONG(0, 4))
	{
		default:
		{
			model = "urban";
			m_pVIP->m_iModelName = CLASS_URBAN;
			break;
		}

		case 1:
		{
			model = "gsg9";
			m_pVIP->m_iModelName = CLASS_GSG9;
			break;
		}

		case 2:
		{
			model = "sas";
			m_pVIP->m_iModelName = CLASS_SAS;
			break;
		}

		case 3:
		{
			model = "gign";
			m_pVIP->m_iModelName = CLASS_GIGN;
			break;
		}
	}

	g_engfuncs.pfnSetClientKeyValue(ENTINDEX(m_pVIP->edict()), g_engfuncs.pfnGetInfoKeyBuffer(m_pVIP->edict()), "model", model);

	m_pVIP->m_bIsVIP = false;
	m_pVIP->m_bNotKilled = false;
}

void CHalfLifeMultiplay::PickNextVIP()
{
	if (!IsVIPQueueEmpty())
	{
		if (m_pVIP != NULL)
		{
			ResetCurrentVIP();
		}

		for (int i = 0; i < MAX_VIPQUEUES; i++)
		{
			m_pVIP = VIPQueue[i];
			m_pVIP->MakeVIP();

			VIPQueue[i] = NULL;

			StackVIPQueue();
			m_iConsecutiveVIP = 0;
		}
	}
	else if (m_iConsecutiveVIP > 2)
	{
		if (++m_iLastPick > m_iNumCT)
		{
			m_iLastPick = 1;
		}

		int count = 1;

		CBasePlayer *pLastPlayer = NULL;
		CBaseEntity *pEntity = NULL;

		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
		{
			if (FNullEnt(pEntity->edict()))
				break;

			if (pEntity->IsDormant())
				continue;

			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer*)pEntity->pev);

			if (pPlayer->m_iTeam == TEAM_CT)
			{
				if (count == m_iLastPick)
				{
					if (pPlayer == m_pVIP && pLastPlayer)
					{
						pPlayer = pLastPlayer;
					}

					if (m_pVIP != NULL)
					{
						ResetCurrentVIP();
					}

					pPlayer->MakeVIP();
					m_iConsecutiveVIP = 0;

					break;
				}

				count++;
			}

			if (pPlayer->m_iTeam != TEAM_SPECTATOR)
			{
				pLastPlayer = pPlayer;
			}
		}
	}
	else if (!m_pVIP)
	{
		CBaseEntity *pEntity = NULL;

		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
		{
			if (FNullEnt(pEntity->edict()))
				break;

			if (pEntity->IsDormant())
				continue;

			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer*)pEntity->pev);

			if (pPlayer->m_iTeam = TEAM_CT)
			{
				pPlayer->MakeVIP();
				m_iConsecutiveVIP = 0;
			}

			break;
		}
	}
}

#define MAX_INTERMISSION_TIME 120

void CHalfLifeMultiplay::Think(void)
{
	m_VoiceGameMgr.Update(gpGlobals->frametime);

	if (sv_clienttrace->value != 1)
		CVAR_SET_FLOAT("sv_clienttrace", 1);

	if (!m_fRoundCount)
		m_fIntroRoundCount = m_fRoundCount = gpGlobals->time;

	if (m_flForceCameraValue != forcecamera.value || m_flForceChaseCamValue != forcechasecam.value || m_flFadeToBlackValue != fadetoblack.value)
	{
		MESSAGE_BEGIN(MSG_ALL, gmsgForceCam);
		WRITE_BYTE(forcecamera.value != 0);
		WRITE_BYTE(forcechasecam.value != 0);
		WRITE_BYTE(fadetoblack.value != 0);
		MESSAGE_END();

		m_flForceCameraValue = forcecamera.value;
		m_flForceChaseCamValue = forcechasecam.value;
		m_flFadeToBlackValue = fadetoblack.value;
	}

	if (CheckGameOver())
		return;

	if (CheckTimeLimit())
		return;

	if (!IsCareer())
	{
		if (CheckMaxRounds())
			return;

		if (CheckWinLimit())
			return;
	}

	CheckAllowSpecator();

	if (IsFreezePeriod())
		CheckFreezePeriodExpired();
	else
		CheckRoundTimeExpired();

	if (m_fTeamCount && m_fTeamCount <= gpGlobals->time)
	{
		if (!IsCareer() || m_fCareerRoundMenuTime == 0)
			RestartRound();
	}

	CheckLevelInitialized();

	if (gpGlobals->time > m_tmNextPeriodicThink)
	{
		m_tmNextPeriodicThink = gpGlobals->time + 1;

		CheckGameCvar();
		CheckRestartRound();
	}
}

BOOL CHalfLifeMultiplay::HasRoundTimeExpired(void)
{
	if (TimeRemaining() > 0 || m_iRoundWinStatus)
		return FALSE;

	return IsBombPlanted() == FALSE;
}

BOOL CHalfLifeMultiplay::IsBombPlanted(void)
{
	if (m_bMapHasBombTarget)
	{
		CGrenade *pGrenade = NULL;

		if ((pGrenade = (CGrenade *)UTIL_FindEntityByClassname(pGrenade, "grenade")) != NULL)
		{
			if (pGrenade->m_bIsC4)
				return TRUE;
		}
	}

	return FALSE;
}

void CHalfLifeMultiplay::MarkLivingPlayersOnTeamAsNotReceivingMoneyNextRound(int team)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *entity = UTIL_PlayerByIndex(i);


		if (!entity||FNullEnt(entity->pev))
			continue;

		CBasePlayer *pPlayer = (CBasePlayer *)entity;

		if (pPlayer->m_iTeam == team)
		{
			if (pPlayer->pev->health > 0)
			{
				if (pPlayer->pev->deadflag == DEAD_NO)
					pPlayer->m_bReceivesNoMoneyNextRound = true;
			}
		}
	}
}

void CHalfLifeMultiplay::CareerRestart(void)
{
	g_fGameOver = FALSE;

	if (m_fTeamCount == 0)
		m_fTeamCount = gpGlobals->time + 1;

	m_bCompleteReset = true;
	m_fCareerRoundMenuTime = 0;
	m_fCareerMatchMenuTime = 0;
	m_bSkipSpawn = false;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *entity = UTIL_PlayerByIndex(i);

		if (!entity || FNullEnt(entity->pev))
			continue;

		CBasePlayer *pPlayer = (CBasePlayer *)entity;

		if (!pPlayer->IsBot())
			pPlayer->ForceClientDllUpdate();
	}
}

void CHalfLifeMultiplay::CheckFreezePeriodExpired(void)
{
	if (TimeRemaining() > 0)
		return;

	UTIL_LogPrintf("World triggered \"Round_Start\"\n");
	m_bFreezePeriod = FALSE;

	char CT_sentence[40], T_sentence[40];

	switch (RANDOM_LONG(0, 3))
	{
		case 0:
		{
			strncpy(CT_sentence, "%!MRAD_MOVEOUT", sizeof(CT_sentence));
			strncpy(T_sentence, "%!MRAD_MOVEOUT", sizeof(T_sentence));
			break;
		}

		case 1:
		{
			strncpy(CT_sentence, "%!MRAD_LETSGO", sizeof(CT_sentence));
			strncpy(T_sentence, "%!MRAD_LETSGO", sizeof(T_sentence));
			break;
		}

		case 2:
		{
			strncpy(CT_sentence, "%!MRAD_LOCKNLOAD", sizeof(CT_sentence));
			strncpy(T_sentence, "%!MRAD_LOCKNLOAD", sizeof(T_sentence));
			break;
		}

		case 3:
		{
			strncpy(CT_sentence, "%!MRAD_GO", sizeof(CT_sentence));
			strncpy(T_sentence, "%!MRAD_GO", sizeof(T_sentence));
			break;
		}
	}

	if (m_bMapHasEscapeZone == true)
	{
		strncpy(CT_sentence, "%!MRAD_ELIM", sizeof(CT_sentence));
		strncpy(T_sentence, "%!MRAD_GETOUT", sizeof(T_sentence));
	}
	else if (m_iMapHasVIPSafetyZone == 1)
	{
		strncpy(CT_sentence, "%!MRAD_VIP", sizeof(CT_sentence));
		strncpy(T_sentence, "%!MRAD_LOCKNLOAD", sizeof(T_sentence));
	}

	bool bCTPlayed = false, bTPlayed = false;
	m_iRoundTimeSecs = m_iRoundTime;
	m_fRoundCount = gpGlobals->time;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (!pPlayer || pPlayer->pev->flags == FL_DORMANT)
			continue;

		if (pPlayer->m_iJoiningState == JOINED)
		{
			if (pPlayer->m_iTeam == TEAM_CT && !bCTPlayed)
			{
				pPlayer->Radio(CT_sentence, NULL);
				bCTPlayed = true;
			}
			else if (pPlayer->m_iTeam == TEAM_TERRORIST && !bCTPlayed)
			{
				pPlayer->Radio(T_sentence, NULL);
				bTPlayed = true;
			}

			if (pPlayer->m_iTeam != TEAM_SPECTATOR)
			{
				pPlayer->ResetMaxSpeed();
				pPlayer->m_bCanShoot = true;
			}
		}

		pPlayer->SyncRoundTimer();
	}
}

void CHalfLifeMultiplay::CheckRoundTimeExpired(void)
{
	if (!HasRoundTimeExpired())
		return;

	CGrenade *pGrenade = (CGrenade *)UTIL_FindEntityByClassname(NULL, "grenade");

	if (m_bMapHasBombTarget == true)
	{
		Broadcast("ctwin");
		m_iAccountCT += 3250;
		m_iNumCTWins++;

		EndRoundMessage("#Target_Saved", Target_Saved);
		TerminateRound(5, WINSTATUS_CT);

		if (IsCareer())
			QueueCareerRoundEndMenu(5, WINSTATUS_CT);

		UpdateTeamScores();
		MarkLivingPlayersOnTeamAsNotReceivingMoneyNextRound(TEAM_TERRORIST);
	}
	else if ((UTIL_FindEntityByClassname(NULL, "hostage_entity")) != NULL)
	{
		Broadcast("terwin");
		m_iAccountTerrorist += 3250;
		m_iNumTerroristWins++;

		EndRoundMessage("#Hostages_Not_Rescued", Hostages_Not_Rescued);
		TerminateRound(5, WINSTATUS_TERRORIST);

		if (IsCareer())
			QueueCareerRoundEndMenu(5, WINSTATUS_TERRORIST);

		UpdateTeamScores();
		MarkLivingPlayersOnTeamAsNotReceivingMoneyNextRound(TEAM_CT);
	}
	else if (m_bMapHasEscapeZone == true)
	{
		Broadcast("ctwin");
		m_iNumCTWins++;

		EndRoundMessage("#Terrorists_Not_Escaped", Terrorists_Not_Escaped);
		TerminateRound(5, WINSTATUS_CT);

		if (IsCareer())
			QueueCareerRoundEndMenu(5, WINSTATUS_CT);

		UpdateTeamScores();
	}
	else if (m_iMapHasVIPSafetyZone == 1)
	{
		Broadcast("terwin");
		m_iAccountTerrorist += 3250;
		m_iNumTerroristWins++;

		EndRoundMessage("#VIP_Not_Escaped", VIP_Not_Escaped);
		TerminateRound(5, WINSTATUS_TERRORIST);

		if (IsCareer())
			QueueCareerRoundEndMenu(5, WINSTATUS_TERRORIST);

		UpdateTeamScores();
	}

	m_fRoundCount = gpGlobals->time + 60;
}

void CHalfLifeMultiplay::CheckLevelInitialized(void)
{
	if (m_bLevelInitialized)
		return;

	m_iSpawnPointCount_CT = 0;
	m_iSpawnPointCount_Terrorist = 0;

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_player_deathmatch")) != NULL)
		m_iSpawnPointCount_Terrorist++;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_player_start")) != NULL)
		m_iSpawnPointCount_CT++;

	m_bLevelInitialized = true;
}

void CHalfLifeMultiplay::CheckRestartRound(void)
{
	int iRestartDelay = restartround.value;

	if (!iRestartDelay)
		iRestartDelay = sv_restart.value;

	if (iRestartDelay > 0)
	{
		if (iRestartDelay > 60)
			iRestartDelay = 60;

		UTIL_LogPrintf("World triggered \"Restart_Round_(%i_%s)\"\n", iRestartDelay, iRestartDelay == 1 ? "second" : "seconds");

		if (g_pGameRules)
		{
			UTIL_LogPrintf("Team \"CT\" scored \"%i\" with \"%i\" players\n", g_pGameRules->m_iNumCTWins, g_pGameRules->m_iNumCT);
			UTIL_LogPrintf("Team \"TERRORIST\" scored \"%i\" with \"%i\" players\n", g_pGameRules->m_iNumTerroristWins, g_pGameRules->m_iNumTerrorist);
		}

		UTIL_ClientPrintAll(HUD_PRINTCENTER, "#Game_will_restart_in", UTIL_dtos1(iRestartDelay), iRestartDelay == 1 ? "SECOND" : "SECONDS");
		UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "#Game_will_restart_in_console", UTIL_dtos1(iRestartDelay), iRestartDelay == 1 ? "SECOND" : "SECONDS");

		m_bCompleteReset = true;
		m_fTeamCount = gpGlobals->time + iRestartDelay;

		CVAR_SET_FLOAT("sv_restartround", 0);
		CVAR_SET_FLOAT("sv_restart", 0);

		CareerRestart();
	}
}

BOOL CHalfLifeMultiplay::CheckGameOver(void)
{
	if (!g_fGameOver)
		return FALSE;

	int time = (int)CVAR_GET_FLOAT("mp_chattime");

	if (time < 1)
		CVAR_SET_STRING("mp_chattime", "1");
	else if (time > MAX_INTERMISSION_TIME)
		CVAR_SET_STRING("mp_chattime", UTIL_dtos1(MAX_INTERMISSION_TIME));

	m_flIntermissionEndTime = m_flIntermissionStartTime + mp_chattime.value;

	if (m_flIntermissionEndTime < gpGlobals->time && !IsCareer() && (!UTIL_HumansInGame(false) || m_iEndIntermissionButtonHit || gpGlobals->time > m_flIntermissionStartTime + MAX_INTERMISSION_TIME))
		ChangeLevel();

	return TRUE;
}

BOOL CHalfLifeMultiplay::CheckTimeLimit(void)
{
	if (timelimit.value < 0)
	{
		CVAR_SET_FLOAT("mp_timelimit", 0);
		return FALSE;
	}

	if (timelimit.value)
	{
		g_flTimeLimit = g_flResetTime + timelimit.value * 60;

		if (g_flTimeLimit <= gpGlobals->time)
		{
			ALERT(at_console, "Changing maps because time limit has been met\n");
			GoToIntermission();
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::CheckMaxRounds(void)
{
	if (!m_iMaxRounds)
		return FALSE;

	if (m_iTotalRoundsPlayed < m_iMaxRounds)
		return FALSE;

	ALERT(at_console, "Changing maps due to maximum rounds have been met\n");
	GoToIntermission();
	return TRUE;
}

BOOL CHalfLifeMultiplay::CheckWinLimit(void)
{
	if (!m_iMaxRoundsWon)
		return FALSE;

	if (m_iNumCTWins < m_iMaxRoundsWon && m_iNumTerroristWins < m_iMaxRoundsWon)
		return FALSE;

	ALERT(at_console, "Changing maps...one team has won the specified number of rounds\n");
	GoToIntermission();
	return TRUE;
}

void CHalfLifeMultiplay::CheckAllowSpecator(void)
{
	if (m_iStoredSpectValue != allow_spectators.value)
	{
		m_iStoredSpectValue = allow_spectators.value;

		MESSAGE_BEGIN(MSG_ALL, gmsgAllowSpec);
		WRITE_BYTE(allow_spectators.value);
		MESSAGE_END();
	}
}

void CHalfLifeMultiplay::CheckGameCvar(void)
{
	if (g_psv_accelerate->value != 5)
		CVAR_SET_FLOAT("sv_accelerate", 5);

	if (g_psv_friction->value != 4)
		CVAR_SET_FLOAT("sv_friction", 4);

	if (g_psv_stopspeed->value != 75)
		CVAR_SET_FLOAT("sv_stopspeed", 75);

	m_iMaxRounds = maxrounds.value;

	if (m_iMaxRounds < 0)
	{
		m_iMaxRounds = 0;
		CVAR_SET_FLOAT("mp_maxrounds", 0);
	}

	m_iMaxRoundsWon = winlimit.value;

	if (m_iMaxRoundsWon < 0)
	{
		m_iMaxRoundsWon = 0;
		CVAR_SET_FLOAT("mp_winlimit", 0);
	}
}

BOOL CHalfLifeMultiplay::IsMultiplayer(void)
{
	return TRUE;
}

BOOL CHalfLifeMultiplay::IsDeathmatch(void)
{
	return TRUE;
}

BOOL CHalfLifeMultiplay::IsCoOp(void)
{
	return (BOOL)gpGlobals->coop;
}

BOOL CHalfLifeMultiplay::FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
	if (!pWeapon->CanDeploy())
		return FALSE;

	if (!pPlayer->m_pActiveItem)
		return TRUE;

	if (!pPlayer->m_iAutoWepSwitch)
		return FALSE;

	if (!pPlayer->m_pActiveItem->CanHolster())
		return FALSE;

	if (pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight())
		return TRUE;

	return FALSE;
}

BOOL CHalfLifeMultiplay::GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon)
{
	if (!pCurrentWeapon->CanHolster())
		return FALSE;

	int iBestWeight = -1;
	CBasePlayerItem *pBest = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		CBasePlayerItem *pCheck = pPlayer->m_rgpPlayerItems[i];

		while (pCheck)
		{
			if (pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon)
			{
				if (pCheck->CanDeploy())
				{
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	if (!pBest)
		return FALSE;

	pPlayer->SwitchWeapon(pBest);
	return TRUE;
}

BOOL CHalfLifeMultiplay::ClientCommand(CBasePlayer *pPlayer, const char *pcmd)
{
	return FALSE;
}

BOOL CHalfLifeMultiplay::ClientCommand_DeadOrAlive(CBasePlayer *pPlayer, const char *pcmd)
{
	return m_VoiceGameMgr.ClientCommand(pPlayer, pcmd) != false;
}

BOOL CHalfLifeMultiplay::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	m_VoiceGameMgr.ClientConnected(pEntity);
	return TRUE;
}

void CHalfLifeMultiplay::UpdateGameMode(CBasePlayer *pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(1);
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD(CBasePlayer *pl)
{
	int i;

	UTIL_LogPrintf("\"%s<%i><%s><>\" entered the game\n", STRING(pl->pev->netname), GETPLAYERUSERID(pl->edict()), GETPLAYERAUTHID(pl->edict()));
	UpdateGameMode(pl);

	if (g_flWeaponCheat == 0)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgViewMode, NULL, pl->edict());
		MESSAGE_END();
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl->edict());
	WRITE_BYTE(ENTINDEX(pl->edict()));
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(pl->m_iTeam);
	MESSAGE_END();

	if (IsCareer())
	{
	}
	else
		SendMOTDToClient(pl->edict());

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (plr)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pl->edict());
			WRITE_BYTE(i);
			WRITE_SHORT((int)plr->pev->frags);
			WRITE_SHORT(plr->m_iDeaths);
			WRITE_SHORT(0);
			WRITE_SHORT(plr->m_iTeam);
			MESSAGE_END();
		}
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgTeamScore, NULL, pl->edict());
	WRITE_STRING(GetTeam(TEAM_TERRORIST));
	WRITE_SHORT(m_iNumTerroristWins);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgTeamScore, NULL, pl->edict());
	WRITE_STRING(GetTeam(TEAM_CT));
	WRITE_SHORT(m_iNumCTWins);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgAllowSpec, NULL, pl->edict());
	WRITE_BYTE(allow_spectators.value);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgForceCam, NULL, pl->edict());
	WRITE_BYTE(forcecamera.value != 0);
	WRITE_BYTE(forcechasecam.value != 0);
	WRITE_BYTE(fadetoblack.value != 0);
	MESSAGE_END();

	if (g_fGameOver)
	{
		MESSAGE_BEGIN(MSG_ONE, SVC_INTERMISSION, NULL, pl->edict());
		MESSAGE_END();
	}

	for (i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (plr)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamInfo, NULL, pl->edict());
			WRITE_BYTE(ENTINDEX(plr->edict()));
			WRITE_STRING(GetTeam(plr->m_iTeam));
			MESSAGE_END();

			plr->SetScoreboardAttributes(pl);

			if (i != ENTINDEX(pl->edict()))
			{
				if (plr->pev->flags == FL_DORMANT)
					continue;

				if (plr->pev->deadflag == DEAD_NO)
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgRadar, NULL, pl->pev);
					WRITE_BYTE(ENTINDEX(plr->edict()));
					WRITE_COORD(plr->pev->origin.x);
					WRITE_COORD(plr->pev->origin.y);
					WRITE_COORD(plr->pev->origin.z);
					MESSAGE_END();
				}
			}
		}
	}

	if (g_pGameRules->m_bBombDropped)
	{
		CBaseEntity *pC4 = UTIL_FindEntityByClassname(NULL, "weapon_c4");

		if (pC4)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBombDrop, NULL, pl->pev);
			WRITE_COORD(pC4->pev->origin.x);
			WRITE_COORD(pC4->pev->origin.y);
			WRITE_COORD(pC4->pev->origin.z);
			WRITE_BYTE(0);
			MESSAGE_END();
		}
	}
}

void CHalfLifeMultiplay::ClientDisconnected(edict_t *pClient)
{
	if (pClient)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pClient);

		if (pPlayer)
		{
			pPlayer->has_disconnected = true;
			pPlayer->pev->deadflag = DEAD_DEAD;
			pPlayer->SetScoreboardAttributes();

			if (pPlayer->m_bHasC4 == true)
				pPlayer->DropPlayerItem("weapon_c4");

			if (pPlayer->m_bHasDefuser == true)
				pPlayer->DropPlayerItem("item_thighpack");

			if (pPlayer->m_bIsVIP == true)
				m_pVIP = NULL;

			pPlayer->m_iCurrentKickVote = 0;

			if (pPlayer->m_iCurrentKickVote)
			{
				m_iMapVotes[pPlayer->m_iMapVote]--;

				if (m_iMapVotes[pPlayer->m_iMapVote] < 0)
					m_iMapVotes[pPlayer->m_iMapVote] = 0;
			}

			MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
			WRITE_BYTE(ENTINDEX(pClient));
			WRITE_SHORT(0);
			WRITE_SHORT(0);
			WRITE_SHORT(0);
			WRITE_SHORT(0);
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
			WRITE_BYTE(ENTINDEX(pClient));
			WRITE_STRING(GetTeam(TEAM_UNASSIGNED));
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ALL, gmsgLocation);
			WRITE_BYTE(ENTINDEX(pClient));
			WRITE_STRING("");
			MESSAGE_END();

			FireTargets("game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0);
			UTIL_LogPrintf("\"%s<%i><%s><%s>\" disconnected\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), GetTeam(pPlayer->m_iTeam));
			pPlayer->RemoveAllItems(TRUE);

			if (pPlayer->m_pObserver)
				pPlayer->m_pObserver->SUB_Remove();

			CBasePlayer *client = NULL;

			while ((client = (CBasePlayer *)UTIL_FindEntityByClassname(client, "player")) != NULL)
			{
				if (FNullEnt(client->edict()))
					break;

				if (!client->pev || client == pPlayer)
					continue;

				if (client->m_hObserverTarget == pPlayer)
				{
					int iMode = client->pev->iuser1;
					client->pev->iuser1 = 0;
					client->Observer_SetMode(iMode);
				}
			}
		}
	}

	CheckWinConditions();
}

float CHalfLifeMultiplay::FlPlayerFallDamage(CBasePlayer *pPlayer)
{
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED * 1.25;
}

BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker)
{
	if (!pAttacker)
		return TRUE;

	if (PlayerRelationship(pPlayer, pAttacker) != GR_TEAMMATE)
		return TRUE;

	if (CVAR_GET_FLOAT("mp_friendlyfire") != 0)
		return TRUE;

	if (pAttacker == pPlayer)
		return TRUE;

	return FALSE;
}

void ShowVGUIMenu(CBasePlayer *pPlayer, int MenuType, int BitMask, char *szOldMenu);
BOOL HandleMenu_ChooseTeam(CBasePlayer *pPlayer, int slot);
void HandleMenu_ChooseAppearance(CBasePlayer *player, int slot);

void CHalfLifeMultiplay::PlayerThink(CBasePlayer *pPlayer)
{
	if (g_fGameOver)
	{
		if (!IsCareer() && (pPlayer->m_afButtonPressed & (IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP)))
			m_iEndIntermissionButtonHit = TRUE;

		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}

	if (!pPlayer->m_bCanShoot)
	{
		if (!IsFreezePeriod())
			pPlayer->m_bCanShoot = true;
	}

	if (pPlayer->m_pActiveItem && pPlayer->m_pActiveItem->IsWeapon())
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)pPlayer->m_pActiveItem->GetWeaponPtr();

		if (pWeapon->m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
			pPlayer->m_bCanShoot = false;
	}

	if (pPlayer->m_iMenu != Menu_ChooseTeam && pPlayer->m_iJoiningState == SHOWTEAMSELECT)
	{
		int slot = -1;

		if (!stricmp(humans_join_team.string, "T"))
		{
			slot = 1;
		}
		else if (!stricmp(humans_join_team.string, "CT"))
		{
			slot = 2;
		}
		else
		{
			if (!allow_spectators.value)
				ShowVGUIMenu(pPlayer, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5, "#Team_Select");
			else
				ShowVGUIMenu(pPlayer, MENU_TEAM, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_5 | MENU_KEY_6, "#Team_Select_Spect");
		}

		pPlayer->m_iMenu = Menu_ChooseTeam;
		pPlayer->m_iJoiningState = PICKINGTEAM;

		if (slot > 0)
		{
			if (!pPlayer->IsBot())
			{
				HandleMenu_ChooseTeam(pPlayer, slot);

				if (slot != 6)
				{
					if (IsCareer())
						HandleMenu_ChooseAppearance(pPlayer, 6);
				}
			}
		}
	}
}

void CHalfLifeMultiplay::PlayerSpawn(CBasePlayer *pPlayer)
{
	if (pPlayer->m_bJustConnected)
		return;

	pPlayer->pev->weapons |= (1 << WEAPON_SUIT);

	BOOL addDefault = TRUE;
	CBaseEntity *pWeaponEntity = NULL;

	while ((pWeaponEntity = UTIL_FindEntityByClassname(pWeaponEntity, "game_player_equip")) != NULL)
	{
		pWeaponEntity->Touch(pPlayer);
		addDefault = FALSE;
	}

	if (pPlayer->m_bNotKilled == true)
		addDefault = false;

	if (addDefault || pPlayer->m_bIsVIP)
		pPlayer->GiveDefaultItems();

	pPlayer->SetPlayerModel(false);
}

BOOL CHalfLifeMultiplay::FPlayerCanRespawn(CBasePlayer *pPlayer)
{
	if (pPlayer->m_iNumSpawns > 0)
		return FALSE;

	m_iNumCT = CountTeamPlayers(TEAM_CT);
	m_iNumTerrorist = CountTeamPlayers(TEAM_TERRORIST);

	if (m_iNumTerrorist > 0 && m_iNumCT > 0 && gpGlobals->time > m_fRoundCount + 20)
	{
		if (fadetoblack.value != 0)
			UTIL_ScreenFade(pPlayer, Vector(0, 0, 0), 3, 3, 255, FFADE_OUT | FFADE_STAYOUT);

		return FALSE;
	}

	if (pPlayer->m_iMenu == Menu_ChooseAppearance)
		return FALSE;

	return TRUE;
}

float CHalfLifeMultiplay::FlPlayerSpawnTime(CBasePlayer *pPlayer)
{
	return gpGlobals->time;
}

BOOL CHalfLifeMultiplay::AllowAutoTargetCrosshair(void)
{
	return FALSE;
}

int CHalfLifeMultiplay::IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled)
{
	return 1;
}

void CHalfLifeMultiplay::PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor)
{
	DeathNotice(pVictim, pKiller, pInflictor);

	pVictim->m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
	pVictim->m_iDeaths++;
	pVictim->m_bNotKilled = false;
	pVictim->m_bEscaped = false;
	pVictim->m_iTrain = TRAIN_NEW | TRAIN_OFF;
	SET_VIEW(ENT(pVictim->pev), ENT(pVictim->pev));

	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance(pKiller);

	if (ktmp && ktmp->Classify() == CLASS_PLAYER)
	{
		peKiller = (CBasePlayer *)ktmp;
	}
	else if (ktmp && ktmp->Classify() == CLASS_VEHICLE)
	{
		CBasePlayer *pDriver = (CBasePlayer *)((CFuncVehicle *)ktmp)->m_pDriver;

		if (pDriver)
		{
			pKiller = ktmp->pev;
			peKiller = (CBasePlayer *)pDriver;
		}
	}

	FireTargets("game_playerdie", pVictim, pVictim, USE_TOGGLE, 0);

	if (pVictim->pev == pKiller)
	{
		pKiller->frags -= 1;
	}
	else if (peKiller && peKiller->IsPlayer())
	{
		CBasePlayer *killer = GetClassPtr((CBasePlayer *)pKiller);

		if (killer->m_iTeam == pVictim->m_iTeam)
		{
			pKiller->frags -= IPointsForKill(peKiller, pVictim);
			killer->AddAccount(-3300);
			killer->m_iTeamKills++;
			killer->m_bJustKilledTeammate = true;

			ClientPrint(killer->pev, HUD_PRINTCENTER, "#Killed_Teammate");
			ClientPrint(killer->pev, HUD_PRINTCONSOLE, "#Game_teammate_kills", UTIL_dtos1(killer->m_iTeamKills));

			if (killer->m_iTeamKills == 3)
			{
				if (CVAR_GET_FLOAT("mp_autokick") != 0)
				{
					ClientPrint(killer->pev, HUD_PRINTCONSOLE, "#Banned_For_Killing_Teamates");

					int iUserID = GETPLAYERUSERID(ENT(killer->pev));

					if (iUserID != -1)
						SERVER_COMMAND(UTIL_VarArgs("kick # %d\n", iUserID));
				}
			}

			if (!(killer->m_flDisplayHistory & DHF_FRIEND_KILLED))
			{
				killer->m_flDisplayHistory |= DHF_FRIEND_KILLED;
				killer->HintMessage("#Hint_careful_around_teammates");
			}
		}
		else
		{
			pKiller->frags += IPointsForKill(peKiller, pVictim);

			if (pVictim->m_bIsVIP == true)
			{
				killer->HintMessage("#Hint_reward_for_killing_vip", TRUE);
				killer->AddAccount(2500);

				MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
				WRITE_BYTE(9);
				WRITE_BYTE(DRC_CMD_EVENT);
				WRITE_SHORT(ENTINDEX(ENT(pVictim->pev)));
				WRITE_SHORT(ENTINDEX(ENT(pInflictor)));
				WRITE_LONG(15 | DRC_FLAG_DRAMATIC | DRC_FLAG_FINAL);
				MESSAGE_END();

				UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Assassinated_The_VIP\"\n", STRING(killer->pev->netname), GETPLAYERUSERID(killer->edict()), GETPLAYERAUTHID(killer->edict()));
			}
			else
				killer->AddAccount(300);

			if (!(killer->m_flDisplayHistory & DHF_ENEMY_KILLED))
			{
				killer->m_flDisplayHistory |= DHF_ENEMY_KILLED;
				killer->HintMessage("#Hint_win_round_by_killing_enemy");
			}
		}

		FireTargets("game_playerkill", peKiller, peKiller, USE_TOGGLE, 0);
	}
	else
		pKiller->frags -= 1;

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(pVictim->edict()));
	WRITE_SHORT((int)pVictim->pev->frags);
	WRITE_SHORT(pVictim->m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(pVictim->m_iTeam);
	MESSAGE_END();

	CBaseEntity *ep = CBaseEntity::Instance(pKiller);

	if (ep && ep->Classify() == CLASS_PLAYER)
	{
		CBasePlayer *PK = (CBasePlayer *)ep;

		MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
		WRITE_BYTE(ENTINDEX(PK->edict()));
		WRITE_SHORT((int)PK->pev->frags);
		WRITE_SHORT(PK->m_iDeaths);
		WRITE_SHORT(0);
		WRITE_SHORT(PK->m_iTeam);
		MESSAGE_END();

		PK->m_flNextDecalTime = gpGlobals->time;
	}
}

void CHalfLifeMultiplay::DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor)
{
	CBaseEntity::Instance(pKiller);

	const char *killer_weapon_name = "world";
	int killer_index = 0;
	char *tau = "tau_cannon";
	char *gluon = "gluon gun";

	if (pKiller->flags & FL_CLIENT)
	{
		killer_index = ENTINDEX(ENT(pKiller));

		if (pevInflictor)
		{
			if (pevInflictor == pKiller)
			{
				CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pKiller);

				if (pPlayer->m_pActiveItem)
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
			}
			else
				killer_weapon_name = STRING(pevInflictor->classname);
		}
	}
	else
		killer_weapon_name = STRING(pevInflictor->classname);

	if (!strncmp(killer_weapon_name, "weapon_", 7))
		killer_weapon_name += 7;
	else if (!strncmp(killer_weapon_name, "monster_", 8))
		killer_weapon_name += 8;
	else if (!strncmp(killer_weapon_name, "func_", 5))
		killer_weapon_name += 5;

	int iGotHeadshot = FALSE;

	if (pVictim->m_bHeadshotKilled == true)
		iGotHeadshot = TRUE;

	MESSAGE_BEGIN(MSG_ALL, gmsgDeathMsg);
	WRITE_BYTE(killer_index);
	WRITE_BYTE(ENTINDEX(pVictim->edict()));
	WRITE_BYTE(iGotHeadshot);
	WRITE_STRING(killer_weapon_name);
	MESSAGE_END();

	if (!strcmp(killer_weapon_name, "egon"))
		killer_weapon_name = gluon;
	else if (!strcmp(killer_weapon_name, "gauss"))
		killer_weapon_name = tau;

	if (pVictim->pev == pKiller)
	{
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n", STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()), GetTeam(pVictim->m_iTeam), killer_weapon_name);
	}
	else if (pKiller->flags & FL_CLIENT)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pKiller);
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n", STRING(pKiller->netname), GETPLAYERUSERID(ENT(pKiller)), GETPLAYERAUTHID(ENT(pKiller)), GetTeam(pPlayer->m_iTeam), STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()), GetTeam(pVictim->m_iTeam), killer_weapon_name);
	}
	else
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n", STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()), GetTeam(pVictim->m_iTeam), killer_weapon_name);

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);
	WRITE_BYTE(DRC_CMD_EVENT);
	WRITE_SHORT(ENTINDEX(pVictim->edict()));

	if (pevInflictor)
		WRITE_SHORT(ENTINDEX(ENT(pevInflictor)));
	else
		WRITE_SHORT(ENTINDEX(ENT(pKiller)));

	if (iGotHeadshot)
		WRITE_LONG(15 | DRC_FLAG_DRAMATIC | DRC_FLAG_SLOWMOTION);
	else
		WRITE_LONG(7 | DRC_FLAG_DRAMATIC);

	MESSAGE_END();
}

void CHalfLifeMultiplay::PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{

}

float CHalfLifeMultiplay::FlWeaponRespawnTime(CBasePlayerItem *pWeapon)
{
	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

#define ENTITY_INTOLERANCE 100

float CHalfLifeMultiplay::FlWeaponTryRespawn(CBasePlayerItem *pWeapon)
{
	if (pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD))
	{
		if (NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE))
			return 0;

		return FlWeaponRespawnTime(pWeapon);
	}

	return 0;
}

Vector CHalfLifeMultiplay::VecWeaponRespawnSpot(CBasePlayerItem *pWeapon)
{
	return pWeapon->pev->origin;
}

int CHalfLifeMultiplay::WeaponShouldRespawn(CBasePlayerItem *pWeapon)
{
	if (pWeapon->pev->spawnflags & SF_NORESPAWN)
		return GR_WEAPON_RESPAWN_NO;

	return GR_WEAPON_RESPAWN_YES;
}

BOOL CHalfLifeMultiplay::CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem)
{
	return CGameRules::CanHavePlayerItem(pPlayer, pItem);
}

BOOL CHalfLifeMultiplay::CanHaveItem(CBasePlayer *pPlayer, CItem *pItem)
{
	return TRUE;
}

void CHalfLifeMultiplay::PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem)
{

}

int CHalfLifeMultiplay::ItemShouldRespawn(CItem *pItem)
{
	if (pItem->pev->spawnflags & SF_NORESPAWN)
		return GR_ITEM_RESPAWN_NO;

	return GR_ITEM_RESPAWN_YES;
}

float CHalfLifeMultiplay::FlItemRespawnTime(CItem *pItem)
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

Vector CHalfLifeMultiplay::VecItemRespawnSpot(CItem *pItem)
{
	return pItem->pev->origin;
}

void CHalfLifeMultiplay::PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount)
{

}

BOOL CHalfLifeMultiplay::IsAllowedToSpawn(CBaseEntity *pEntity)
{
	return TRUE;
}

int CHalfLifeMultiplay::AmmoShouldRespawn(CBasePlayerAmmo *pAmmo)
{
	if (pAmmo->pev->spawnflags & SF_NORESPAWN)
		return GR_AMMO_RESPAWN_NO;

	return GR_AMMO_RESPAWN_YES;
}

float CHalfLifeMultiplay::FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo)
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

Vector CHalfLifeMultiplay::VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo)
{
	return pAmmo->pev->origin;
}

float CHalfLifeMultiplay::FlHealthChargerRechargeTime(void)
{
	return 60;
}

float CHalfLifeMultiplay::FlHEVChargerRechargeTime(void)
{
	return 30;
}

int CHalfLifeMultiplay::DeadPlayerWeapons(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

int CHalfLifeMultiplay::DeadPlayerAmmo(CBasePlayer *pPlayer)
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot(pPlayer);

	if (IsMultiplayer() && pentSpawnSpot->v.target)
		FireTargets(STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0);

	return pentSpawnSpot;
}

int CHalfLifeMultiplay::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	if (!pPlayer || !pTarget)
		return GR_NOTTEAMMATE;

	if (!pTarget->IsPlayer())
		return GR_NOTTEAMMATE;

	if (GetClassPtr((CBasePlayer *)pPlayer->pev)->m_iTeam != GetClassPtr((CBasePlayer *)pTarget->pev)->m_iTeam)
		return GR_NOTTEAMMATE;

	return GR_TEAMMATE;
}

BOOL CHalfLifeMultiplay::FAllowFlashlight(void)
{
	static cvar_t *mp_flashlight = NULL;

	if (!mp_flashlight)
		mp_flashlight = CVAR_GET_POINTER("mp_flashlight");

	if (mp_flashlight)
		return mp_flashlight->value != 0;

	return FALSE;
}

BOOL CHalfLifeMultiplay::FAllowMonsters(void)
{
	return CVAR_GET_FLOAT("mp_allowmonsters") != 0;
}

void CHalfLifeMultiplay::GoToIntermission(void)
{
	if (g_fGameOver)
		return;

	if (g_pGameRules)
	{
		UTIL_LogPrintf("Team \"CT\" scored \"%i\" with \"%i\" players\n", g_pGameRules->m_iNumCTWins, g_pGameRules->m_iNumCT);
		UTIL_LogPrintf("Team \"TERRORIST\" scored \"%i\" with \"%i\" players\n", g_pGameRules->m_iNumTerroristWins, g_pGameRules->m_iNumTerrorist);
	}

	if (IsCareer())
	{
	}

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	if (IsCareer())
		SERVER_COMMAND("setpause\n");

	int time = (int)CVAR_GET_FLOAT("mp_chattime");

	if (time < 1)
		CVAR_SET_STRING("mp_chattime", "1");
	else if (time > MAX_INTERMISSION_TIME)
		CVAR_SET_STRING("mp_chattime", UTIL_dtos1(MAX_INTERMISSION_TIME));

	m_flIntermissionEndTime = gpGlobals->time + (int)mp_chattime.value;
	m_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;
	m_iSpawnPointCount_Terrorist = 0;
	m_iSpawnPointCount_CT = 0;
	m_bLevelInitialized = false;
}

void CHalfLifeMultiplay::ServerDeactivate(void)
{
	if (IsCareer())
	{
	}
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;
	char mapname[32];
	int minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
}
mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
}
mapcycle_t;

void DestroyMapCycle(mapcycle_t *cycle)
{
	mapcycle_item_t *n, *start;
	mapcycle_item_t *p = cycle->items;

	if (p)
	{
		start = p;
		p = p->next;

		while (p != start)
		{
			n = p->next;
			delete p;
			p = n;
		}

		delete cycle->items;
	}

	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char mp_com_token[1500];

char *MP_COM_GetToken(void)
{
	return mp_com_token;
}

char *MP_COM_Parse(char *data)
{
	int len = 0, c;
	mp_com_token[0] = '\0';

	if (!data)
		return NULL;

skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;

		data++;
	}

	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;

		goto skipwhite;
	}

	if (c == '\"')
	{
		data++;

		while (1)
		{
			c = *data++;

			if (c == '\"' || !c)
			{
				mp_com_token[len] = '\0';
				return data;
			}

			mp_com_token[len++] = c;
		}
	}

	if (c == '{' || c == '}'|| c == ')'|| c == '(' || c == '\'' || c == ',')
	{
		mp_com_token[len++] = c;
		mp_com_token[len] = '\0';
		return data + 1;
	}

	do
	{
		mp_com_token[len] = c;
		data++;
		len++;
		c = *data;

		if (c == '{' || c == '}'|| c == ')'|| c == '(' || c == '\'' || c == ',')
			break;
	}
	while (c > 32);

	mp_com_token[len] = '\0';
	return data;
}

int MP_COM_TokenWaiting(char *buffer)
{
	char *p = buffer;

	while (*p && *p != '\n')
	{
		if (!isspace(*p) || isalnum(*p))
			return 1;

		p++;
	}

	return 0;
}

int ReloadMapCycleFile(char *filename, mapcycle_t *cycle)
{
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char *)LOAD_FILE_FOR_ME(filename, &length);
	mapcycle_item_s *item, *newlist = NULL, *next;

	if (pFileList && length)
	{
		while (1)
		{
			int hasbuffer = 0;
			char szBuffer[MAX_RULE_BUFFER];
			memset(szBuffer, 0, MAX_RULE_BUFFER);
			pFileList = MP_COM_Parse(pFileList);

			if (mp_com_token[0] == '\0')
				break;

			char szMap[32];
			strcpy(szMap, mp_com_token);

			if (MP_COM_TokenWaiting(pFileList))
			{
				pFileList = MP_COM_Parse(pFileList);

				if (mp_com_token[0] != '\0')
				{
					hasbuffer = 1;
					strcpy(szBuffer, mp_com_token);
				}
			}

			if (IS_MAP_VALID(szMap))
			{
				item = new mapcycle_item_s;
				strcpy(item->mapname, szMap);

				item->minplayers = 0;
				item->maxplayers = 0;

				memset(item->rulebuffer, 0, MAX_RULE_BUFFER);

				if (hasbuffer)
				{
					char *s = g_engfuncs.pfnInfoKeyValue(szBuffer, "minplayers");

					if (s && s[0])
					{
						item->minplayers = atoi(s);
						item->minplayers = max(item->minplayers, 0);
						item->minplayers = min(item->minplayers, gpGlobals->maxClients);
					}

					s = g_engfuncs.pfnInfoKeyValue(szBuffer, "maxplayers");

					if (s && s[0])
					{
						item->maxplayers = atoi(s);
						item->maxplayers = max(item->maxplayers, 0);
						item->maxplayers = min(item->maxplayers, gpGlobals->maxClients);
					}

					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "minplayers");
					g_engfuncs.pfnInfo_RemoveKey(szBuffer, "maxplayers");
					strcpy(item->rulebuffer, szBuffer);
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
				ALERT(at_console, "Skipping %s from mapcycle, not a valid map\n", szMap);
		}

		FREE_FILE(aFileList);
	}

	item = cycle->items;

	while (item)
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}

	cycle->items = newlist;
	item = cycle->items;

	if (!item)
		return 0;

	while (item->next)
		item = item->next;

	item->next = cycle->items;
	cycle->next_item = item->next;
	return 1;
}

int CountPlayers(void)
{
	int num = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(i);

		if (pEnt)
			num = num + 1;
	}

	return num;
}

void ExtractCommandString(char *s, char *szCommand)
{
	char pkey[512];
	char value[512];

	if (*s == '\\')
		s++;

	while (1)
	{
		char *o = pkey;

		while (*s != '\\')
		{
			if (!*s)
				return;

			*o++ = *s++;
		}

		*o = '\0';
		s++;
		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;

			*o++ = *s++;
		}

		*o = '\0';
		strcat(szCommand, pkey);

		if (value[0] != '\0')
		{
			strcat(szCommand, " ");
			strcat(szCommand, value);
		}

		strcat(szCommand, "\n");

		if (!*s)
			return;

		s++;
	}
}

int GetMapCount(void)
{
	static mapcycle_t mapcycle;
	char *mapcfile = (char *)CVAR_GET_STRING("mapcyclefile");

	DestroyMapCycle(&mapcycle);
	ReloadMapCycleFile(mapcfile, &mapcycle);

	int num = 0;

	for (mapcycle_item_s *item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next)
		num = num + 1;

	return num;
}

void CHalfLifeMultiplay::DisplayMaps(CBasePlayer *pPlayer, int mapId)
{
	static mapcycle_t mapcycle;
	char *mapcfile = (char *)CVAR_GET_STRING("mapcyclefile");
	int index = 0, id = 0;
	char *pszNextMaps = NULL;

	DestroyMapCycle(&mapcycle);
	ReloadMapCycleFile(mapcfile, &mapcycle);

	for (mapcycle_item_s *item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next, index++)
	{
		id++;

		if (pPlayer)
		{
			if (m_iMapVotes[index] != 1)
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "#Votes", UTIL_dtos1(index), item->mapname, UTIL_dtos2(m_iMapVotes[index]));
			else
				ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "#Vote", UTIL_dtos1(index), item->mapname, UTIL_dtos2(1));
		}

		if (id == mapId)
			pszNextMaps = item->mapname;
	}

	if (!pszNextMaps || !mapId)
		return;

	if (strcmp(pszNextMaps, STRING(gpGlobals->mapname)))
	{
		CHANGE_LEVEL(pszNextMaps, NULL);
		return;
	}

	if (timelimit.value)
	{
		timelimit.value += 30;
		UTIL_ClientPrintAll(HUD_PRINTCENTER, "#Map_Vote_Extend");
	}

	ResetAllMapVotes();
}

void CHalfLifeMultiplay::ResetAllMapVotes(void)
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pPlayer->m_iTeam != TEAM_UNASSIGNED)
			pPlayer->m_iCurrentKickVote = 0;
	}

	for (int i = 0; i < MAX_MAPS; i++)
		m_iMapVotes[i] = 0;
}

void CHalfLifeMultiplay::ProcessMapVote(CBasePlayer *pPlayer, int mapId)
{
	CBaseEntity *pEntity = NULL;
	int playerCount = 0, count = 0;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (pPlayer->m_iTeam != TEAM_UNASSIGNED)
		{
			playerCount++;

			if (pPlayer->m_iCurrentKickVote = mapId)
				count++;
		}
	}

	m_iMapVotes[mapId] = count;
	float radio = mapvoteratio.value;

	if (mapvoteratio.value > 1)
	{
		radio = 1;
		CVAR_SET_STRING("mp_mapvoteratio", "1.0");
	}
	else if (mapvoteratio.value < 0.35)
	{
		radio = 0.35;
		CVAR_SET_STRING("mp_mapvoteratio", "0.35");
	}

	int needCount;

	if (playerCount > 2)
		needCount = (int)(playerCount * radio + 0.5);
	else
		needCount = 2;

	if (count < needCount)
	{
		DisplayMaps(pPlayer, 0);
		ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, "#Game_required_votes", UTIL_dtos1(needCount));
	}
	else
		DisplayMaps(NULL, mapId);
}

void CHalfLifeMultiplay::ChangeLevel(void)
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy(szFirstMapInList, "hldm1");

	int curplayers;
	BOOL do_cycle = TRUE;
	char *mapcfile = (char *)CVAR_GET_STRING("mapcyclefile");

	szCommands[0] = '\0';
	szRules[0] = '\0';
	curplayers = CountPlayers();

	if (stricmp(mapcfile, szPreviousMapCycleFile))
	{
		strcpy(szPreviousMapCycleFile, mapcfile);
		DestroyMapCycle(&mapcycle);

		if (!ReloadMapCycleFile(mapcfile, &mapcycle) || !mapcycle.items)
		{
			ALERT(at_console, "Unable to load map cycle file %s\n", mapcfile);
			do_cycle = FALSE;
		}
	}

	if (do_cycle && mapcycle.items)
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		strcpy(szNextMap, STRING(gpGlobals->mapname));
		strcpy(szFirstMapInList, STRING(gpGlobals->mapname));

		for (item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next)
		{
			keeplooking = FALSE;

			if (item->minplayers)
			{
				if (curplayers >= item->minplayers)
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
					keeplooking = TRUE;
			}

			if (item->maxplayers)
			{
				if (curplayers <= item->maxplayers)
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
					keeplooking = TRUE;
			}

			if (keeplooking)
				continue;

			found = TRUE;
			break;
		}

		if (!found)
			item = mapcycle.next_item;

		mapcycle.next_item = item->next;
		strcpy(szNextMap, item->mapname);
		ExtractCommandString(item->rulebuffer, szCommands);
		strcpy(szRules, item->rulebuffer);
	}

	if (!IS_MAP_VALID(szNextMap))
		strcpy(szNextMap, szFirstMapInList);

	g_fGameOver = TRUE;
	ALERT(at_console, "CHANGE LEVEL: %s\n", szNextMap);

	if (minplayers || maxplayers)
		ALERT(at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers);

	if (strlen(szRules))
		ALERT(at_console, "RULES:  %s\n", szRules);

	CHANGE_LEVEL(szNextMap, NULL);

	if (strlen(szCommands))
		SERVER_COMMAND(szCommands);
}



void CHalfLifeMultiplay::SendMOTDToClient(edict_t *client)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgServerName, NULL, client);
	WRITE_STRING(CVAR_GET_STRING("hostname"));
	MESSAGE_END();
	
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char *)LOAD_FILE_FOR_ME((char *)CVAR_GET_STRING("motdfile"), &length);


	
	while (pFileList && *pFileList && char_count < MAX_MOTD_LENGTH)
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if (strlen(pFileList) < MAX_MOTD_CHUNK)
		{
			strcpy(chunk, pFileList);
		}
		else
		{
			strncpy(chunk, pFileList, MAX_MOTD_CHUNK);
			chunk[MAX_MOTD_CHUNK] = '\0';
		}

		char_count += strlen(chunk);

		if (char_count < MAX_MOTD_LENGTH)
			pFileList = aFileList + char_count;
		else
			*pFileList = '\0';

		MESSAGE_BEGIN(MSG_ONE, gmsgMOTD, NULL, client);
		WRITE_BYTE(*pFileList ? FALSE : TRUE);
		WRITE_STRING(chunk);
		MESSAGE_END();
	}

	FREE_FILE(aFileList);
}

void CHalfLifeMultiplay::ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer)
{
	pPlayer->SetPlayerModel(pPlayer->m_bHasC4);
	pPlayer->SetPrefsFromUserinfo(infobuffer);
}


void CHalfLifeMultiplay::SetRestartServerAtRoundEnd()
{
	m_bShouldRestart = TRUE;
}

BOOL CHalfLifeMultiplay::ShouldRestart()
{
	return m_bShouldRestart;
}

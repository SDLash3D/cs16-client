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
#include "client.h"
#include "player.h"
#include "trains.h"
#include "vehicle.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "training_gamerules.h"
#include "game.h"
#include "hltv.h"
#include "pm_shared.h"
#include "studio.h"
#include "hostage.h"

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL BOOL g_fDrawLines;
extern DLL_GLOBAL int g_iSkillLevel, gDisplayTitle;

int gEvilImpulse101;
char g_szMapBriefingText[512];
BOOL gInitHUD = TRUE;

extern void CopyToBodyQue(entvars_t *pev);
extern void respawn(entvars_t *pev, BOOL fCopyCorpse);
extern Vector VecBModelOrigin(entvars_t *pevBModel);
extern edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer);
extern CGraph WorldGraph;

#define FLASH_DRAIN_TIME 1.2
#define FLASH_CHARGE_TIME 0.2

cvar_t *sv_aim;

TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
{
	DEFINE_FIELD(CBasePlayer, m_flFlashLightTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_iFlashBattery, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonLast, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonPressed, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_afButtonReleased, FIELD_INTEGER),
	DEFINE_ARRAY(CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS),
	DEFINE_FIELD(CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_flTimeStepSound, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flSwimTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flDuckTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flWallJumpTime, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_flSuitUpdate, FIELD_TIME),
	DEFINE_ARRAY(CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST),
	DEFINE_FIELD(CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER),
	DEFINE_ARRAY(CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT),
	DEFINE_ARRAY(CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT),
	DEFINE_FIELD(CBasePlayer, m_lastDamageAmount, FIELD_INTEGER),
	DEFINE_ARRAY(CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES),
	DEFINE_FIELD(CBasePlayer, m_pActiveItem, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayer, m_pLastItem, FIELD_CLASSPTR),
	DEFINE_ARRAY(CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
	DEFINE_FIELD(CBasePlayer, m_idrowndmg, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_idrownrestored, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_tSneaking, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_iTrain, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_flFallVelocity, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayer, m_iTargetVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponVolume, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iWeaponFlash, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_fLongJump, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_fInitHUD, FIELD_BOOLEAN),
	DEFINE_FIELD(CBasePlayer, m_tbdPrev, FIELD_TIME),
	DEFINE_FIELD(CBasePlayer, m_pTank, FIELD_EHANDLE),
	DEFINE_FIELD(CBasePlayer, m_iHideHUD, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iFOV, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_flDisplayHistory, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayer, m_iJoiningState, FIELD_INTEGER),
};

int giPrecacheGrunt = 0;

int gmsgCurWeapon = 0;
int gmsgGeigerRange = 0;
int gmsgFlashlight = 0;
int gmsgFlashBattery = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgHudText = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgWeaponList = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgViewMode = 0;
int gmsgShowGameTitle = 0;
int gmsgDeathMsg = 0;
int gmsgScoreAttrib = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgAmmoX = 0;
int gmsgSendAudio = 0;
int gmsgRoundTime = 0;
int gmsgMoney = 0;
int gmsgArmorType = 0;
int gmsgBlinkAcct = 0;
int gmsgStatusValue = 0;
int gmsgStatusText = 0;
int gmsgStatusIcon = 0;
int gmsgBarTime = 0;
int gmsgReloadSound = 0;
int gmsgCrosshair = 0;
int gmsgNVGToggle = 0;
int gmsgRadar = 0;
int gmsgSpectator = 0;
int gmsgVGUIMenu = 0;
int gmsgTutorText = 0;
int gmsgTutorLine = 0;
int gmsgTutorState = 0;
int gmsgTutorClose = 0;
int gmsgAllowSpec = 0;
int gmsgBombDrop = 0;
int gmsgBombPickup = 0;
int gmsgSendCorpse = 0;
int gmsgHostagePos = 0;
int gmsgHostageK = 0;
int gmsgHLTV = 0;
int gmsgSpecHealth = 0;
int gmsgForceCam = 0;
int gmsgADStop = 0;
int gmsgReceiveW = 0;
int gmsgCZCareer = 0;
int gmsgCZCareerHUD = 0;
int gmsgShadowIdx = 0;
int gmsgTaskTime = 0;
int gmsgScenarioIcon = 0;
int gmsgBotVoice = 0;
int gmsgBuyClose = 0;
int gmsgSpecHealth2 = 0;
int gmsgBarTime2 = 0;
int gmsgItemStatus = 0;
int gmsgLocation = 0;
int gmsgBotProgress = 0;
int gmsgBrass = 0;
int gmsgFog = 0;
int gmsgShowTimer = 0;
int gmsgHudTextArgs = 0;

void LinkUserMessages(void)
{
	if (gmsgCurWeapon)
		return;

	gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3);
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	gmsgFlashlight = REG_USER_MSG("Flashlight", 2);
	gmsgFlashBattery = REG_USER_MSG("FlashBat", 1);
	gmsgHealth = REG_USER_MSG("Health", 1);
	gmsgDamage = REG_USER_MSG("Damage", 12);
	gmsgBattery = REG_USER_MSG("Battery", 2);
	gmsgTrain = REG_USER_MSG("Train", 1);
	gmsgHudText = REG_USER_MSG("HudTextPro", -1);
	REG_USER_MSG("HudText", -1);
	gmsgSayText = REG_USER_MSG("SayText", -1);
	gmsgTextMsg = REG_USER_MSG("TextMsg", -1);
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 0);
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0);
	gmsgViewMode = REG_USER_MSG("ViewMode", 0);
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG("DeathMsg", -1);
	gmsgScoreAttrib = REG_USER_MSG("ScoreAttrib", 2);
	gmsgScoreInfo = REG_USER_MSG("ScoreInfo", 9);
	gmsgTeamInfo = REG_USER_MSG("TeamInfo", -1);
	gmsgTeamScore = REG_USER_MSG("TeamScore", -1);
	gmsgGameMode = REG_USER_MSG("GameMode", 1);
	gmsgMOTD = REG_USER_MSG("MOTD", -1);
	gmsgServerName = REG_USER_MSG("ServerName", -1);
	gmsgAmmoPickup = REG_USER_MSG("AmmoPickup", 2);
	gmsgWeapPickup = REG_USER_MSG("WeapPickup", 1);
	gmsgItemPickup = REG_USER_MSG("ItemPickup", -1);
	gmsgHideWeapon = REG_USER_MSG("HideWeapon", 1);
	gmsgSetFOV = REG_USER_MSG("SetFOV", 1);
	gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgAmmoX = REG_USER_MSG("AmmoX", 2);
	gmsgSendAudio = REG_USER_MSG("SendAudio", -1);
	gmsgRoundTime = REG_USER_MSG("RoundTime", 2);
	gmsgMoney = REG_USER_MSG("Money", 5);
	gmsgArmorType = REG_USER_MSG("ArmorType", 1);
	gmsgBlinkAcct = REG_USER_MSG("BlinkAcct", 1);
	gmsgStatusValue = REG_USER_MSG("StatusValue", -1);
	gmsgStatusText = REG_USER_MSG("StatusText", -1);
	gmsgStatusIcon = REG_USER_MSG("StatusIcon", -1);
	gmsgBarTime = REG_USER_MSG("BarTime", 2);
	gmsgReloadSound = REG_USER_MSG("ReloadSound", 2);
	gmsgCrosshair = REG_USER_MSG("Crosshair", 1);
	gmsgNVGToggle = REG_USER_MSG("NVGToggle", 1);
	gmsgRadar = REG_USER_MSG("Radar", 7);
	gmsgSpectator = REG_USER_MSG("Spectator", 2);
	gmsgVGUIMenu = REG_USER_MSG("VGUIMenu", -1);
	gmsgTutorText = REG_USER_MSG("TutorText", -1);
	gmsgTutorLine = REG_USER_MSG("TutorLine", -1);
	gmsgTutorState = REG_USER_MSG("TutorState", -1);
	gmsgTutorClose = REG_USER_MSG("TutorClose", -1);
	gmsgAllowSpec = REG_USER_MSG("AllowSpec", 1);
	gmsgBombDrop = REG_USER_MSG("BombDrop", 7);
	gmsgBombPickup = REG_USER_MSG("BombPickup", 0);
	gmsgSendCorpse = REG_USER_MSG("ClCorpse", -1);
	gmsgHostagePos = REG_USER_MSG("HostagePos", 8);
	gmsgHostageK = REG_USER_MSG("HostageK", 1);
	gmsgHLTV = REG_USER_MSG("HLTV", 2);
	gmsgSpecHealth = REG_USER_MSG("SpecHealth", 1);
	gmsgForceCam = REG_USER_MSG("ForceCam", 3);
	gmsgADStop = REG_USER_MSG("ADStop", 0);
	gmsgReceiveW = REG_USER_MSG("ReceiveW", 1);
	gmsgCZCareer = REG_USER_MSG("CZCareer", -1);
	gmsgCZCareerHUD = REG_USER_MSG("CZCareerHUD", -1);
	gmsgShadowIdx = REG_USER_MSG("ShadowIdx", 4);
	gmsgTaskTime = REG_USER_MSG("TaskTime", 4);
	gmsgScenarioIcon = REG_USER_MSG("Scenario", -1);
	gmsgBotVoice = REG_USER_MSG("BotVoice", 2);
	gmsgBuyClose = REG_USER_MSG("BuyClose", 0);
	gmsgSpecHealth2 = REG_USER_MSG("SpecHealth2", 2);
	gmsgBarTime2 = REG_USER_MSG("BarTime2", 4);
	gmsgItemStatus = REG_USER_MSG("ItemStatus", 1);
	gmsgLocation = REG_USER_MSG("Location", -1);
	gmsgBotProgress = REG_USER_MSG("BotProgress", -1);
	gmsgBrass = REG_USER_MSG("Brass", -1);
	gmsgFog = REG_USER_MSG("Fog", 7);
	gmsgShowTimer = REG_USER_MSG("ShowTimer", 0);
	gmsgHudTextArgs = REG_USER_MSG("HudTextArgs", -1);
}

void WriteSigonMessages(void)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		ItemInfo &II = CBasePlayerItem::ItemInfoArray[i];

		if (!II.iId)
			continue;

		const char *pszName;

		if (!II.pszName)
			pszName = "Empty";
		else
			pszName = II.pszName;

		MESSAGE_BEGIN(MSG_INIT, gmsgWeaponList);
		WRITE_STRING(pszName);
		WRITE_BYTE(CBasePlayer::GetAmmoIndex(II.pszAmmo1));
		WRITE_BYTE(II.iMaxAmmo1);
		WRITE_BYTE(CBasePlayer::GetAmmoIndex(II.pszAmmo2));
		WRITE_BYTE(II.iMaxAmmo2);
		WRITE_BYTE(II.iSlot);
		WRITE_BYTE(II.iPosition);
		WRITE_BYTE(II.iId);
		WRITE_BYTE(II.iFlags);
		MESSAGE_END();
	}
}

LINK_ENTITY_TO_CLASS(player, CBasePlayer);

void SendItemStatus(CBasePlayer *pPlayer)
{
	int itemStatus = 0;

	if (pPlayer->m_bHasNightVision)
		itemStatus |= ITEMSTATE_HASNIGHTVISION;

	if (pPlayer->m_bHasDefuser)
		itemStatus |= ITEMSTATE_HASDEFUSER;

	MESSAGE_BEGIN(MSG_ONE, gmsgItemStatus, NULL, pPlayer->pev);
	WRITE_BYTE(itemStatus);
	MESSAGE_END();
}

void CBasePlayer::SetPlayerModel(BOOL HasC4)
{
	char *infobuffer;
	char *model;

	if (m_iTeam == TEAM_CT)
	{
		switch (m_iModelName)
		{
			default:
			case CLASS_URBAN: model = "urban"; break;
			case CLASS_GSG9: model = "gsg9"; break;
			case CLASS_SAS: model = "sas"; break;
			case CLASS_GIGN: model = "gign"; break;
			case CLASS_VIP: model = "vip"; break;
		}
	}
	else if (m_iTeam == TEAM_TERRORIST)
	{
		switch (m_iModelName)
		{
			default:
			case CLASS_TERROR: model = "terror"; break;
			case CLASS_LEET: model = "leet"; break;
			case CLASS_ARCTIC: model = "arctic"; break;
			case CLASS_GUERILLA: model = "guerilla"; break;
		}
	}
	else
		model = "urban";

	infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());

	if (strcmp(g_engfuncs.pfnInfoKeyValue(infobuffer, "model"), model))
		g_engfuncs.pfnSetClientKeyValue(entindex(), infobuffer, "model", model);
}

void CBasePlayer::SmartRadio(void)
{

}

char *BufPrintf(char *buf, int &len, const char *fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);

	if (len > 0)
	{
		_vsnprintf(buf, len, fmt, argptr);
		va_end(argptr);

		len -= strlen(buf);
		return &buf[strlen(buf)];
	}

	return NULL;
}

char *NumAsString(int val)
{
	static char string[4][16];
	static int curstring;
	int len = 16;

	curstring = (curstring + 1) % 4;
	BufPrintf(string[curstring], len, "%d", val);
	return string[curstring];
}

void CBasePlayer::Radio(const char *msg_id, const char *msg_verbose, int pitch, bool showIcon)
{
	if (!IsPlayer())
		return;

	if (pev->deadflag != DEAD_NO && !IsBot())
		return;

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
	{
		if (FNullEnt(pEntity->edict()))
			break;

		CBasePlayer *pTarget = GetClassPtr((CBasePlayer *)pEntity->pev);

		if (!pTarget->IsPlayer())
		{
			if (pTarget->pev->iuser1 == OBS_CHASE_LOCKED || pTarget->pev->iuser1 == OBS_CHASE_FREE || pTarget->pev->iuser1 == OBS_IN_EYE)
			{
				if (!FNullEnt(m_hObserverTarget))
					pTarget = (CBasePlayer *)((CBaseEntity *)m_hObserverTarget);
				else
					pTarget = NULL;
			}
		}
		else
		{
			if (pTarget->pev->flags == FL_DORMANT)
				pTarget = NULL;
		}

		if (!pTarget)
			continue;

		if (pTarget->m_iTeam != m_iTeam)
			continue;

		if (pTarget->m_bIgnoreRadio)
			continue;

		MESSAGE_BEGIN(MSG_ONE, gmsgSendAudio, NULL, pTarget->pev);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_STRING(msg_id);
		WRITE_SHORT(pitch);
		MESSAGE_END();

		if (msg_verbose)
			ClientPrint(pTarget->pev, HUD_PRINTRADIO, NumAsString(ENTINDEX(edict())), "#Game_radio", STRING(pev->netname), msg_verbose);

		if (showIcon)
		{
			MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pTarget->pev);
			WRITE_BYTE(TE_PLAYERATTACHMENT);
			WRITE_BYTE(ENTINDEX(edict()));
			WRITE_COORD(35);
			WRITE_SHORT(g_sModelIndexRadio);
			WRITE_SHORT(15);
			MESSAGE_END();
		}
	}
}

void CBasePlayer::Pain(int nHitGroup, bool bHitKevlar)
{
	int iRand = RANDOM_LONG(0, 2);

	if (nHitGroup == HITGROUP_HEAD)
	{
		if (m_iKevlar == 2)
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_helmet-1.wav", VOL_NORM, ATTN_NORM);
			return;
		}

		switch (iRand)
		{
			case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot1.wav", VOL_NORM, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot2.wav", VOL_NORM, ATTN_NORM); break;
			default: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/headshot3.wav", VOL_NORM, ATTN_NORM); break;
		}
	}
	else if (nHitGroup == HITGROUP_LEFTLEG || nHitGroup == HITGROUP_RIGHTLEG)
	{
		if (bHitKevlar)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_kevlar-1.wav", VOL_NORM, ATTN_NORM);

		switch (iRand)
		{
			case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-1.wav", VOL_NORM, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-2.wav", VOL_NORM, ATTN_NORM); break;
			default: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-3.wav", VOL_NORM, ATTN_NORM); break;
		}
	}
	else
	{
		switch (iRand)
		{
			case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-1.wav", VOL_NORM, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-2.wav", VOL_NORM, ATTN_NORM); break;
			default: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/bhit_flesh-3.wav", VOL_NORM, ATTN_NORM); break;
		}
	}
}

Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

int TrainSpeed(int iSpeed, int iMax)
{
	int iRet = 0;
	float fMax = iMax;
	float fSpeed = iSpeed;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer::DeathSound(void)
{
	switch (RANDOM_LONG(1, 4))
	{
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die1.wav", VOL_NORM, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die2.wav", VOL_NORM, ATTN_NORM); break;
		case 3: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/die3.wav", VOL_NORM, ATTN_NORM); break;
		case 4: EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/death6.wav", VOL_NORM, ATTN_NORM); break;
	}
}

int CBasePlayer::TakeHealth(float flHealth, int bitsDamageType)
{
	return CBaseMonster::TakeHealth(flHealth, bitsDamageType);
}

Vector CBasePlayer::GetGunPosition(void)
{
	return pev->origin + pev->view_ofs;
}

bool CBasePlayer::IsHittingShield(const Vector &vecDirection, TraceResult *ptr)
{
	if (HasShield() == false)
		 return false;

	if (ptr->iHitgroup == HITGROUP_SHIELD)
		 return true;

	if (m_bShieldDrawn == false)
		return false;

	UTIL_MakeVectors(pev->angles);
	return false;
}

void CBasePlayer::TraceAttack(entvars_t *pevAttacker, float flDamage, const Vector &vecDir, TraceResult *ptr, int bitsDamageType)
{
	bool bShouldBleed = true;
	bool bShouldSpark = false;
	bool bHitShield = IsHittingShield(vecDir, ptr);
	CBasePlayer *pAttacker = (CBasePlayer *)CBasePlayer::Instance(pevAttacker);

	if (CVAR_GET_FLOAT("mp_friendlyfire") == 0 && m_iTeam == pAttacker->m_iTeam)
		bShouldBleed = false;

	if (!pev->takedamage)
		return;

	m_LastHitGroup = ptr->iHitgroup;

	if (bHitShield)
	{
		flDamage = 0;
		bShouldBleed = false;

		if (RANDOM_LONG(0, 1))
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
		else
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

		UTIL_Sparks(ptr->vecEndPos);

		pev->punchangle.x = flDamage * RANDOM_FLOAT(-0.15, 0.15);
		pev->punchangle.z = flDamage * RANDOM_FLOAT(-0.15, 0.15);

		if (pev->punchangle.x < 4)
			pev->punchangle.x = -4;

		if (pev->punchangle.z < -5)
			pev->punchangle.z = -5;
		else if (pev->punchangle.z > 5)
			pev->punchangle.z = 5;
	}
	else
	{
		switch (ptr->iHitgroup)
		{
			case HITGROUP_GENERIC: break;
			case HITGROUP_HEAD:
			{
				if (m_iKevlar == 2)
				{
					bShouldBleed = false;
					bShouldSpark = true;
				}

				flDamage *= 4;

				if (bShouldBleed == true)
				{
					pev->punchangle.x = flDamage * -0.5;

					if (pev->punchangle.x < -12)
						pev->punchangle.x = -12;

					pev->punchangle.z = flDamage * RANDOM_FLOAT(-1, 1);

					if (pev->punchangle.z < -9)
						pev->punchangle.z = -9;
					else if (pev->punchangle.z > 9)
						pev->punchangle.z = 9;
				}

				break;
			}

			case HITGROUP_CHEST:
			{
				if (m_iKevlar != 0)
					bShouldBleed = false;

				flDamage *= 1;

				if (bShouldBleed == true)
				{
					pev->punchangle.x = flDamage * -0.1;

					if (pev->punchangle.x < -4)
						pev->punchangle.x = -4;
				}

				break;
			}

			case HITGROUP_STOMACH:
			{
				if (m_iKevlar != 0)
					bShouldBleed = false;

				flDamage *= 1.25;

				if (bShouldBleed == true)
				{
					pev->punchangle.x = flDamage * -0.1;

					if (pev->punchangle.x < -4)
						pev->punchangle.x = -4;
				}

				break;
			}

			case HITGROUP_LEFTARM:
			case HITGROUP_RIGHTARM:
			{
				if (m_iKevlar != 0)
					bShouldBleed = false;

				flDamage *= 1;
				break;
			}

			case HITGROUP_LEFTLEG:
			case HITGROUP_RIGHTLEG:
			{
				flDamage *= 0.75;
				break;
			}
		}
	}

	if (bShouldBleed == true)
	{
		BloodSplat(ptr->vecEndPos, vecDir, ptr->iHitgroup, flDamage * 5);
		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);
		TraceBleed(flDamage, vecDir, ptr, bitsDamageType);
	}
	else if (ptr->iHitgroup == HITGROUP_HEAD && bShouldSpark == true)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
		WRITE_BYTE(TE_STREAK_SPLASH);
		WRITE_COORD(ptr->vecEndPos.x);
		WRITE_COORD(ptr->vecEndPos.y);
		WRITE_COORD(ptr->vecEndPos.z);
		WRITE_COORD(ptr->vecPlaneNormal.x);
		WRITE_COORD(ptr->vecPlaneNormal.y);
		WRITE_COORD(ptr->vecPlaneNormal.z);
		WRITE_BYTE(5);
		WRITE_SHORT(22);
		WRITE_SHORT(25);
		WRITE_SHORT(65);
		MESSAGE_END();
	}

	AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
}

const char *GetWeaponName(entvars_t *pevInflictor, entvars_t *pKiller)
{
	const char *killer_weapon_name = "world";

	if (pKiller->flags & FL_CLIENT)
	{
		int killer_index = ENTINDEX(ENT(pKiller));

		if (pevInflictor)
		{
			if (pevInflictor == pKiller)
			{
				CBasePlayer *pAttacker = (CBasePlayer *)CBaseEntity::Instance(pKiller);

				if (pAttacker)
					if (pAttacker->m_pActiveItem)
						killer_weapon_name = pAttacker->m_pActiveItem->pszName();
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
	else if (strncmp(killer_weapon_name, "func_", 5))
		killer_weapon_name += 5;

	return killer_weapon_name;
}

extern char *GetTeam(int team);

void LogAttack(CBasePlayer *pAttacker, CBasePlayer *pVictim, BOOL teamAttack, int healthHit, int armorHit, int newHealth, int newArmor, const char *killer_weapon_name)
{
	int detail = logdetail.value;

	if (!detail)
		return;

	if (!pAttacker || !pVictim)
		return;

	if ((teamAttack && (detail & LOG_TEAMMATEATTACK)) || (!teamAttack && (detail & LOG_ENEMYATTACK)))
		UTIL_LogPrintf("\"%s<%i><%s><%s>\" attacked \"%s<%i><%s><%s>\" with \"%s\" (damage \"%d\") (damage_armor \"%d\") (health \"%d\") (armor \"%d\")\n", STRING(pAttacker->pev->netname), GETPLAYERUSERID(pAttacker->edict()), GETPLAYERAUTHID(pAttacker->edict()), GetTeam(pAttacker->m_iTeam), STRING(pVictim->pev->netname), GETPLAYERUSERID(pVictim->edict()), GETPLAYERAUTHID(pVictim->edict()), GetTeam(pVictim->m_iTeam), killer_weapon_name, healthHit, armorHit, newHealth, newArmor);
}

#define ARMOR_RATIO 0.5
#define ARMOR_BONUS 0.5

int CBasePlayer::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	BOOL teamAttack = FALSE;
	int armorHit = 0;
	CBasePlayer *pAttack = NULL;
	float flShieldRatio = 0;
	int fTookDamage;

	if (bitsDamageType & (DMG_EXPLOSION | DMG_BLAST | DMG_FALL))
		m_LastHitGroup = HITGROUP_GENERIC;
	else if (m_LastHitGroup == HITGROUP_SHIELD && (bitsDamageType & DMG_BULLET))
		return 0;

	float flBonus = ARMOR_BONUS;
	float flRatio = ARMOR_RATIO;

	if (HasShield())
		flShieldRatio = 0.2;

	if (m_bIsVIP == true)
		flRatio *= 0.5;

	if (bitsDamageType & (DMG_EXPLOSION | DMG_BLAST))
	{
		if (!IsAlive())
			return 0;

		if (bitsDamageType & DMG_EXPLOSION)
		{
			CBaseEntity *pInflictor = GetClassPtr((CBaseEntity *)pevInflictor);

			if (!strcmp(STRING(pInflictor->pev->classname), "grenade"))
			{
				CGrenade *pGrenade = GetClassPtr((CGrenade *)pevInflictor);

				if (CVAR_GET_FLOAT("mp_friendlyfire") != 0)
				{
					if (pGrenade->m_iTeam == m_iTeam)
						teamAttack = TRUE;

					pAttack = (CBasePlayer *)GET_PRIVATE(ENT(pevAttacker));
				}
				else if (pGrenade->m_iTeam == m_iTeam && &ENT(pev)->v != pevAttacker)
					return 0;
			}
		}

		if (!FNullEnt(ENT(pevInflictor)))
			m_vBlastVector = pev->origin - pevInflictor->origin;

		if (pev->armorvalue && IsArmored(m_LastHitGroup))
		{
			float flNew = flRatio * flDamage;
			float flArmor = (flDamage - flNew) * flBonus;

			if (flArmor <= pev->armorvalue)
			{
				armorHit = pev->armorvalue;

				if (flArmor < 0)
					flArmor = 1;

				pev->armorvalue -= flArmor;
				armorHit -= pev->armorvalue;
			}
			else
			{
				flNew = flDamage - pev->armorvalue;
				armorHit = flArmor;
				pev->armorvalue = 0;
			}

			flDamage = flNew;

			if (pev->armorvalue <= 0)
				m_iKevlar = 0;

			Pain(m_LastHitGroup, true);
		}
		else
			Pain(m_LastHitGroup, false);

		m_lastDamageAmount = flDamage;

		if (pev->health > flDamage)
		{
			SetAnimation(PLAYER_FLINCH);
			Pain(m_LastHitGroup, false);
		}
		else
		{
			if (bitsDamageType & DMG_BLAST)
				m_bKilledByBomb = true;
			else if (bitsDamageType & DMG_EXPLOSION)
				m_bKilledByGrenade = true;
		}

		LogAttack(pAttack, this, teamAttack, flDamage, armorHit, pev->health - flDamage, pev->armorvalue, GetWeaponName(pevInflictor, pevAttacker));
		fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

		if (fTookDamage > 0)
		{
			if (g_pGameRules->IsCareer())
			{
			}
		}

		{
			for (int i = 0; i < CDMG_TIMEBASED; i++)
			{
				if (bitsDamageType & (DMG_PARALYZE << i))
					m_rgbTimeBasedDamage[i] = 0;
			}
		}

		MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
		WRITE_BYTE(9);
		WRITE_BYTE(DRC_CMD_EVENT);
		WRITE_SHORT(ENTINDEX(edict()));
		WRITE_SHORT(ENTINDEX(ENT(pevInflictor)));
		WRITE_LONG(5);
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_SPEC, gmsgHLTV);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_BYTE((int)(pev->health ? pev->health : 0) | 128);
		MESSAGE_END();

		for (int i = 1; i < gpGlobals->maxClients; i++)
		{
			CBasePlayer *temp = (CBasePlayer *)UTIL_PlayerByIndex(i);

			if (!temp)
				continue;

			if (temp->m_hObserverTarget == this)
			{
				MESSAGE_BEGIN(MSG_ONE, gmsgSpecHealth2, NULL, temp->pev);
				WRITE_BYTE(ENTINDEX(edict()));
				WRITE_BYTE(pev->health ? pev->health : 0);
				MESSAGE_END();
			}
		}

		return fTookDamage;
	}

	CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);

	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		if (strcmp(STRING(pevInflictor->classname), "grenade"))
			return 0;
	}

	if ((bitsDamageType & DMG_BLAST) && g_pGameRules->IsMultiplayer())
		flBonus *= 2;

	if (!IsAlive())
		return 0;

	pAttacker = GetClassPtr((CBaseEntity *)pevAttacker);

	if (pAttacker->IsPlayer())
	{
		pAttack = GetClassPtr((CBasePlayer *)pevAttacker);

		if (pAttack != this && pAttack->m_iTeam == m_iTeam)
		{
			if (!(pAttack->m_flDisplayHistory & DHF_FRIEND_INJURED))
			{
				pAttack->m_flDisplayHistory |= DHF_FRIEND_INJURED;
				pAttack->HintMessage("#Hint_try_not_to_injure_teamAttacks");
			}

			teamAttack = TRUE;

			if (gpGlobals->time > m_flLastAttackedTeammate + 0.6)
			{
				CBaseEntity *pEntity = NULL;

				while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
				{
					if (FNullEnt(pEntity->edict()))
						break;

					CBasePlayer *temp = GetClassPtr((CBasePlayer *)pEntity->pev);

					if (temp->m_iTeam == m_iTeam)
						ClientPrint(temp->pev, HUD_PRINTTALK, "#Game_teamAttack_attack", STRING(pev->netname));
				}

				m_flLastAttackedTeammate = gpGlobals->time;
			}
		}

		if (pAttack->m_iTeam == m_iTeam)
			flDamage *= 0.35;

		if (pAttack->m_pActiveItem)
		{
			flRatio += flShieldRatio;

			switch (pAttack->m_pActiveItem->m_iId)
			{
				case WEAPON_AUG:
				case WEAPON_M4A1: flRatio *= 1.4; break;
				case WEAPON_AWP: flRatio *= 1.95; break;
				case WEAPON_G3SG1: flRatio *= 1.65; break;
				case WEAPON_SG550: flRatio *= 1.45; break;
				case WEAPON_M249: flRatio *= 1.5; break;
				case WEAPON_ELITE: flRatio *= 1.05; break;
				case WEAPON_DEAGLE: flRatio *= 1.5; break;
				case WEAPON_GLOCK18: flRatio *= 1.05; break;
				case WEAPON_FIVESEVEN:
				case WEAPON_P90: flRatio *= 1.5; break;
				case WEAPON_MAC10: flRatio *= 0.95; break;
				case WEAPON_P228: flRatio *= 1.25; break;
				case WEAPON_SCOUT:
				case WEAPON_KNIFE: flRatio *= 1.7; break;
				case WEAPON_FAMAS:
				case WEAPON_SG552: flRatio *= 1.4; break;
				case WEAPON_GALIL:
				case WEAPON_AK47: flRatio *= 1.55; break;
			}

			if (ShouldDoLargeFlinch(m_LastHitGroup, pAttack->m_pActiveItem->m_iId))
			{
				if (pev->velocity.Length() < 300)
				{
					m_flVelocityModifier = 0.65;
					pev->velocity = pev->velocity + (pev->origin - pAttack->pev->origin).Normalize() * 170;
				}

				SetAnimation(PLAYER_LARGE_FLINCH);
			}
			else
			{
				m_flVelocityModifier = 0.5;

				if (m_LastHitGroup == HITGROUP_HEAD)
					m_bHighDamage = (flDamage > 60);
				else
					m_bHighDamage = (flDamage > 20);

				SetAnimation(PLAYER_FLINCH);
			}
		}
	}

	m_lastDamageAmount = flDamage;

	if (pev->armorvalue && !(bitsDamageType & (DMG_DROWN | DMG_FALL)) && IsArmored(m_LastHitGroup))
	{
		float flNew = flRatio * flDamage;
		float flArmor = (flDamage - flNew) * flBonus;

		if (flArmor <= pev->armorvalue)
		{
			armorHit = pev->armorvalue;

			if (flArmor < 0)
				flArmor = 1;

			pev->armorvalue -= flArmor;
			armorHit -= pev->armorvalue;
		}
		else
		{
			armorHit = flArmor;
			flArmor = pev->armorvalue * (1 / flBonus);
			pev->armorvalue = 0;
			flNew = flDamage - flArmor;
		}

		flDamage = flNew;

		if (pev->armorvalue <= 0)
			m_iKevlar = 0;

		Pain(m_LastHitGroup, true);
	}
	else
		Pain(m_LastHitGroup, false);

	LogAttack(pAttack, this, teamAttack, flDamage, armorHit, pev->health - flDamage, pev->armorvalue, GetWeaponName(pevInflictor, pevAttacker));
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	if (fTookDamage > 0)
	{
		if (g_pGameRules->IsCareer())
		{
		}
	}

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
		{
			if (bitsDamageType & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
		}
	}

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);
	WRITE_BYTE(DRC_CMD_EVENT);
	WRITE_SHORT(ENTINDEX(edict()));
	WRITE_SHORT(ENTINDEX(ENT(pevInflictor)));
	WRITE_LONG(5);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_SPEC, gmsgHLTV);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_BYTE((int)(pev->health ? pev->health : 0) | 128);
	MESSAGE_END();

	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer *temp = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (!temp)
			continue;

		if (temp->m_hObserverTarget == this)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgSpecHealth, NULL, temp->pev);
			WRITE_BYTE(pev->health ? pev->health : 0);
			MESSAGE_END();
		}
	}

	m_bitsHUDDamage = -1;
	m_bitsDamageType |= bitsDamageType;
	return fTookDamage;
}

const char *GetCSModelName(int item_id);
void packPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem, bool dropAmmo);

void CBasePlayer::PackDeadPlayerItems(void)
{
	int iWeaponRules = g_pGameRules->DeadPlayerWeapons(this);
	int iAmmoRules = g_pGameRules->DeadPlayerAmmo(this);

	if (iWeaponRules != GR_PLR_DROP_GUN_NO)
	{
		bool bHasShield = false;

		if (m_bOwnsShield)
		{
			if (m_pActiveItem)
			{
				bHasShield = true;

				if (m_pActiveItem->CanHolster())
					DropShield(true);
			}
			else
				DropShield(true);
		}

		int iBestWeight = 0;
		CBasePlayerItem *pItem = NULL;

		for (int i = 0; i < MAX_ITEM_TYPES; i++)
		{
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

			while (pPlayerItem)
			{
				ItemInfo II;

				if (pPlayerItem->iItemSlot() < WPNSLOT_KNIFE && !bHasShield)
				{
					if (pPlayerItem->GetItemInfo(&II))
					{
						if (II.iWeight > iBestWeight)
						{
							iBestWeight = II.iWeight;
							pItem = pPlayerItem;
						}
					}
				}
				else
				{
					if (pPlayerItem->iItemSlot() == WPNSLOT_GRENADE)
					{
					}
				}

				pPlayerItem = pPlayerItem->m_pNext;
			}
		}

		packPlayerItem(this, pItem, iAmmoRules != GR_PLR_DROP_AMMO_NO);
	}

	RemoveAllItems(TRUE);
}

void packPlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem, bool packAmmo)
{
	if (pItem)
	{
		const char *modelName = GetCSModelName(pItem->m_iId);

		if (modelName)
		{
			CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create("weaponbox", pPlayer->pev->origin, pPlayer->pev->angles, ENT(pPlayer->pev));
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->pev->velocity = pPlayer->pev->velocity * 0.75;
			pWeaponBox->SetThink(&CWeaponBox::Kill);
			pWeaponBox->pev->nextthink = gpGlobals->time + 300;
			pWeaponBox->PackWeapon(pItem);

			if (packAmmo)
				pWeaponBox->PackAmmo(MAKE_STRING(CBasePlayerItem::ItemInfoArray[pItem->m_iId].pszAmmo1), pPlayer->m_rgAmmo[pItem->PrimaryAmmoIndex()]);

			SET_MODEL(ENT(pWeaponBox->pev), modelName);
		}
	}
}

const char *GetCSModelName(int item_id)
{
	const char *modelName = NULL;

	switch (item_id)
	{
		case WEAPON_AK47: modelName = "models/w_ak47.mdl"; break;
		case WEAPON_GALIL: modelName = "models/w_galil.mdl"; break;
		case WEAPON_FAMAS: modelName = "models/w_famas.mdl"; break;
		case WEAPON_AWP: modelName = "models/w_awp.mdl"; break;
		case WEAPON_DEAGLE: modelName = "models/w_deagle.mdl"; break;
		case WEAPON_G3SG1: modelName = "models/w_g3sg1.mdl"; break;
		case WEAPON_SG550: modelName = "models/w_sg550.mdl"; break;
		case WEAPON_GLOCK18: modelName = "models/w_glock18.mdl"; break;
		case WEAPON_M249: modelName = "models/w_m249.mdl"; break;
		case WEAPON_M3: modelName = "models/w_m3.mdl"; break;
		case WEAPON_M4A1: modelName = "models/w_m4a1.mdl"; break;
		case WEAPON_MP5N: modelName = "models/w_mp5.mdl"; break;
		case WEAPON_SG552: modelName = "models/w_sg552.mdl"; break;
		case WEAPON_AUG: modelName = "models/w_aug.mdl"; break;
		case WEAPON_TMP: modelName = "models/w_tmp.mdl"; break;
		case WEAPON_USP: modelName = "models/w_usp.mdl"; break;
		case WEAPON_ELITE: modelName = "models/w_elite.mdl"; break;
		case WEAPON_FIVESEVEN: modelName = "models/w_fiveseven.mdl"; break;
		case WEAPON_P90: modelName = "models/w_p90.mdl"; break;
		case WEAPON_UMP45: modelName = "models/w_ump45.mdl"; break;
		case WEAPON_MAC10: modelName = "models/w_mac10.mdl"; break;
		case WEAPON_P228: modelName = "models/w_p228.mdl"; break;
		case WEAPON_SCOUT: modelName = "models/w_scout.mdl"; break;
		case WEAPON_KNIFE: modelName = "models/w_knife.mdl"; break;
		case WEAPON_FLASHBANG: modelName = "models/w_flashbang.mdl"; break;
		case WEAPON_HEGRENADE: modelName = "models/w_hegrenade.mdl"; break;
		case WEAPON_SMOKEGRENADE: modelName = "models/w_smokegrenade.mdl"; break;
		case WEAPON_XM1014: modelName = "models/w_xm1014.mdl"; break;
		case WEAPON_C4: modelName = "models/w_backpack.mdl"; break;
		case WEAPON_SHIELDGUN: modelName = "models/w_shield.mdl"; break;
		default: ALERT(at_console, "CBasePlayer::PackDeadPlayerItems(): Unhandled item- not creating weaponbox\n");
	}

	return modelName;
}

void CBasePlayer::GiveDefaultItems(void)
{
	RemoveAllItems(FALSE);
	m_bHasPrimary = false;

	if (m_iTeam == TEAM_CT)
	{
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_usp");

		if (m_bIsVIP)
			GiveAmmo(12, "45acp", _45ACP_MAX_CARRY);
		else
			GiveAmmo(24, "45acp", _45ACP_MAX_CARRY);
	}
	else if (m_iTeam == TEAM_TERRORIST)
	{
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_glock18");
		GiveAmmo(40, "9mm", _9MM_MAX_CARRY);
	}
}

void CBasePlayer::RemoveAllItems(BOOL removeSuit)
{
   BOOL bKillProgBar = false;
   int i;

   if (m_bHasDefuser)
   {
      m_bHasDefuser = false;
      pev->body = 0;

      MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, ENT(pev));
      WRITE_BYTE(STATUSICON_HIDE);
      WRITE_STRING("defuser");
      MESSAGE_END();

      SendItemStatus(this);
      bKillProgBar = true;
   }

   if (m_bHasC4)
   {
      m_bHasC4 = false;
      pev->body = 0;

      MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, ENT(pev));
      WRITE_BYTE(STATUSICON_HIDE);
      WRITE_STRING("c4");
      MESSAGE_END();

      bKillProgBar = true;
   }

   RemoveShield();

   if (bKillProgBar)
      SetProgressBarTime(0);

   if (m_pActiveItem)
   {
      ResetAutoaim();

      m_pActiveItem->Holster();
      m_pActiveItem = NULL;
   }
   m_pLastItem = NULL;

   for (i = 0; i < MAX_ITEM_TYPES; i++)
   {
      m_pActiveItem = m_rgpPlayerItems[i];

      while (m_pActiveItem)
      {
         CBasePlayerItem *pPendingItem = m_pActiveItem->m_pNext;

         m_pActiveItem->Drop();
         m_pActiveItem = pPendingItem;
      }

      m_rgpPlayerItems[i] = NULL;
   }

   m_pActiveItem = NULL;
   m_bHasPrimary = NULL;

   pev->viewmodel = 0;
   pev->weaponmodel = 0;

   if (removeSuit)
      pev->weapons = 0;
   else
      pev->weapons &= ~WEAPON_ALLWEAPONS;

   for (i = 0; i < MAX_AMMO_SLOTS; i++)
      m_rgAmmo[i] = 0;

   UpdateClientData();

   MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, ENT(pev));
   WRITE_BYTE(0);
   WRITE_BYTE(0);
   WRITE_BYTE(0);
   MESSAGE_END();
}

void CBasePlayer::SetBombIcon(BOOL bFlash)
{
	if (m_bHasC4)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);

		if (bFlash)
			WRITE_BYTE(STATUSICON_FLASH);
		else
			WRITE_BYTE(STATUSICON_SHOW);

		WRITE_STRING("c4");
		WRITE_BYTE(0);
		WRITE_BYTE(160);
		WRITE_BYTE(0);
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("c4");
		MESSAGE_END();
	}

	SetScoreboardAttributes();
}

void CBasePlayer::SendFOV(int fov)
{
	m_iFOV = m_iClientFOV = pev->fov = fov;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
	WRITE_BYTE(fov);
	MESSAGE_END();
}

void CBasePlayer::SetProgressBarTime(int time)
{
	if (time)
	{
		m_progressStart = gpGlobals->time;
		m_progressEnd = time + gpGlobals->time;
	}
	else
	{
		m_progressStart = 0;
		m_progressEnd = 0;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgBarTime, NULL, pev);
	WRITE_SHORT(time);
	MESSAGE_END();

	CBaseEntity *pPlayer = NULL;
	int myIndex = entindex();

	while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (FNullEnt(pPlayer->edict()))
			break;

		CBasePlayer *player = GetClassPtr((CBasePlayer *)pPlayer->pev);

		if (player->pev->iuser1 == OBS_IN_EYE && player->pev->iuser2 == myIndex)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBarTime, NULL, player->pev);
			WRITE_SHORT(time);
			MESSAGE_END();
		}
	}
}

void CBasePlayer::SetProgressBarTime2(int time, float timeElapsed)
{
	if (time)
	{
		m_progressStart = gpGlobals->time - timeElapsed;
		m_progressEnd = time + gpGlobals->time - timeElapsed;
	}
	else
	{
		time = 0;
		m_progressStart = 0;
		m_progressEnd = 0;
	}

	int iTimeElapsed = (time * 100.0 / (m_progressEnd - m_progressStart));

	MESSAGE_BEGIN(MSG_ONE, gmsgBarTime2, NULL, pev);
	WRITE_SHORT(time);
	WRITE_SHORT(iTimeElapsed);
	MESSAGE_END();

	CBaseEntity *pPlayer = NULL;
	int myIndex = entindex();

	while ((pPlayer = UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (FNullEnt(pPlayer->edict()))
			break;

		CBasePlayer *player = GetClassPtr((CBasePlayer *)pPlayer->pev);

		if (player->pev->iuser1 == OBS_IN_EYE && player->pev->iuser2 == myIndex)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBarTime2, NULL, player->pev);
			WRITE_SHORT(time);
			WRITE_SHORT(iTimeElapsed);
			MESSAGE_END();
		}
	}
}

entvars_t *g_pevLastInflictor;

void CBasePlayer::Killed(entvars_t *pevAttacker, int iGib)
{
	m_canSwitchObserverModes = false;

	if (m_LastHitGroup == HITGROUP_HEAD)
		m_bHeadshotKilled = true;

	if (g_pGameRules->IsCareer())
	{
	}

	if (!m_bKilledByBomb)
		g_pGameRules->PlayerKilled(this, pevAttacker, g_pevLastInflictor);

	MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	m_bNightVisionOn = false;

	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer *pOther = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (pOther && pOther->IsObservingPlayer(this))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pOther->pev);
			WRITE_BYTE(0);
			MESSAGE_END();

			pOther->m_bNightVisionOn = false;
		}
	}

	if (m_pTank)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (pSound)
		pSound->Reset();

	SetAnimation(PLAYER_DIE);

	if (m_pActiveItem && m_pActiveItem->m_pPlayer)
	{
		switch (m_pActiveItem->m_iId)
		{
			case WEAPON_HEGRENADE:
			{
				if ((pev->button & IN_ATTACK) && m_rgAmmo[((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType])
					CGrenade::ShootTimed2(pev, GetGunPosition(), pev->angles, 1.5, m_iTeam, ((CHEGrenade *)m_pActiveItem)->m_usCreateExplosion);

				break;
			}

			case WEAPON_FLASHBANG:
			{
				if (pev->button & IN_ATTACK && m_rgAmmo[((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType])
					CGrenade::ShootTimed(pev, GetGunPosition(), pev->angles, 1.5);

				break;
			}

			case WEAPON_SMOKEGRENADE:
			{
				if ((pev->button & IN_ATTACK) && m_rgAmmo[((CBasePlayerWeapon *)m_pActiveItem)->m_iPrimaryAmmoType])
					CGrenade::ShootSmokeGrenade(pev, GetGunPosition(), pev->angles, 1.5, ((CSmokeGrenade *)m_pActiveItem)->m_usCreateSmoke);

				break;
			}
		}
	}

	pev->modelindex = m_modelIndexPlayer;
	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_TOSS;
	pev->takedamage = DAMAGE_NO;

	pev->gamestate = 1;
	m_bShieldDrawn = false;

	pev->flags &= ~FL_ONGROUND;

	if (!fadetoblack.value)
	{
		pev->iuser1 = OBS_CHASE_FREE;
		pev->iuser2 = ENTINDEX(ENT(pev));
		pev->iuser3 = ENTINDEX(ENT(pevAttacker));
		m_hObserverTarget = UTIL_PlayerByIndex(pev->iuser3);
	}
	else
		UTIL_ScreenFade(this, Vector(0, 0, 0), 3, 3, 255, FFADE_OUT | FFADE_STAYOUT);

	SetScoreboardAttributes();

	if (m_iThrowDirection)
	{
		switch (m_iThrowDirection)
		{
			case THROW_FORWARD:
			{
				UTIL_MakeVectors(pev->angles);
				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(100, 200);
				pev->velocity.z = RANDOM_FLOAT(50, 100);
				break;
			}

			case THROW_BACKWARD:
			{
				UTIL_MakeVectors(pev->angles);
				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(-100, -200);
				pev->velocity.z = RANDOM_FLOAT(50, 100);
				break;
			}

			case THROW_HITVEL:
			{
				if (FClassnameIs(pevAttacker, "player"))
				{
					UTIL_MakeVectors(pevAttacker->angles);
					pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(200, 300);
					pev->velocity.z = RANDOM_FLOAT(200, 300);
				}

				break;
			}

			case THROW_BOMB:
			{
				pev->velocity = m_vBlastVector * (1 / m_vBlastVector.Length()) * (2300 - m_vBlastVector.Length()) * 0.25;
				pev->velocity.z = (2300 - m_vBlastVector.Length()) / 2.75;
				break;
			}

			case THROW_GRENADE:
			{
				pev->velocity = m_vBlastVector * (1 / m_vBlastVector.Length()) * (500 - m_vBlastVector.Length());
				pev->velocity.z = (350 - m_vBlastVector.Length()) * 1.5;
				break;
			}

			case THROW_HITVEL_MINUS_AIRVEL:
			{
				if (FClassnameIs(pevAttacker, "player"))
				{
					UTIL_MakeVectors(pevAttacker->angles);
					pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(200, 300);
				}

				break;
			}
		}

		m_iThrowDirection = THROW_NONE;
	}

	SetSuitUpdate(NULL, FALSE, 0);

	m_iClientHealth = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
	WRITE_BYTE(m_iClientHealth);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0xFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	SendFOV(0);

	g_pGameRules->CheckWinConditions();
	m_bNotKilled = false;

	if (m_bHasC4 == true)
	{
		DropPlayerItem("weapon_c4");
		SetProgressBarTime(0);
	}
	else if (m_bHasDefuser == true)
	{
		m_bHasDefuser = false;
		pev->body = 0;
		GiveNamedItem("item_thighpack");

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("defuser");
		MESSAGE_END();

		SendItemStatus(this);
		SetProgressBarTime(0);
	}

	m_bIsDefusing = false;

	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
	WRITE_BYTE(STATUSICON_HIDE);
	WRITE_STRING("buyzone");
	MESSAGE_END();

	if (m_iMenu >= Menu_Buy)
	{
		if (m_iMenu <= Menu_BuyItem)
		{
			CLIENT_COMMAND(ENT(pev), "slot10\n");
		}
		else if (m_iMenu == Menu_ClientBuy)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
			MESSAGE_END();
		}
	}

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;

	if (m_bPunishedForTK)
	{
		m_bPunishedForTK = false;
		HintMessage("#Hint_cannot_play_because_tk");
	}

	if ((pev->health < -9000 && iGib != GIB_NEVER) || iGib == GIB_ALWAYS)
	{
		pev->solid = SOLID_NOT;
		GibMonster();
		pev->effects |= EF_NODRAW;
		g_pGameRules->CheckWinConditions();
		return;
	}

	DeathSound();
	pev->angles.x = 0;
	pev->angles.z = 0;

	if (!(m_flDisplayHistory & DHF_SPEC_DUCK))
	{
		HintMessage("#Spec_Duck", TRUE, TRUE);
		m_flDisplayHistory |= DHF_SPEC_DUCK;
	}
}

BOOL CBasePlayer::IsBombGuy(void)
{
	if (!g_pGameRules->IsMultiplayer())
		return FALSE;

	return m_bHasC4;
}

void CBasePlayer::SetAnimation(PLAYER_ANIM playerAnim)
{
	int animDesired;
	float speed;
	char szAnim[64];
	int hopSeq, leapSeq;

	if (!pev->modelindex)
		return;

	if ((playerAnim == PLAYER_FLINCH || playerAnim == PLAYER_LARGE_FLINCH) && HasShield() == true)
		return;

	if (!(playerAnim == PLAYER_FLINCH || playerAnim == PLAYER_LARGE_FLINCH || gpGlobals->time > m_flFlinchTime || pev->health <= 0))
		return;

	speed = pev->velocity.Length2D();

	if (FBitSet(pev->flags, FL_FROZEN))
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	hopSeq = LookupActivity(ACT_HOP);
	leapSeq = LookupActivity(ACT_LEAP);

	switch (playerAnim)
	{
		case PLAYER_JUMP:
		{
			if (m_Activity == ACT_SWIM || m_Activity == ACT_DIESIMPLE || m_Activity == ACT_HOVER)
				m_IdealActivity = m_Activity;
			else
				m_IdealActivity = ACT_HOP;

			break;
		}

		case PLAYER_SUPERJUMP:
		{
			if (m_Activity == ACT_SWIM || m_Activity == ACT_DIESIMPLE || m_Activity == ACT_HOVER)
				m_IdealActivity = m_Activity;
			else
				m_IdealActivity = ACT_LEAP;

			break;
		}

		case PLAYER_DIE:
		{
			m_IdealActivity = ACT_DIESIMPLE;
			DeathSound();
			break;
		}

		case PLAYER_ATTACK1:
		{
			if (m_Activity == ACT_SWIM || m_Activity == ACT_DIESIMPLE || m_Activity == ACT_HOVER)
				m_IdealActivity = m_Activity;
			else
				m_IdealActivity = ACT_RANGE_ATTACK1;

			break;
		}

		case PLAYER_ATTACK2:
		{
			if (m_Activity == ACT_SWIM || m_Activity == ACT_DIESIMPLE || m_Activity == ACT_HOVER)
				m_IdealActivity = m_Activity;
			else
				m_IdealActivity = ACT_RANGE_ATTACK2;

			break;
		}

		case PLAYER_RELOAD:
		{
			if (m_Activity == ACT_SWIM || m_Activity == ACT_DIESIMPLE || m_Activity == ACT_HOVER)
				m_IdealActivity = m_Activity;
			else
				m_IdealActivity = ACT_RELOAD;

			break;
		}

		case PLAYER_IDLE:
		case PLAYER_WALK:
		{
			if (FBitSet(pev->flags, FL_ONGROUND) || (m_Activity != ACT_HOP && m_Activity != ACT_LEAP))
			{
				if (pev->waterlevel <= 1)
					m_IdealActivity = ACT_WALK;
				else if (!speed)
					m_IdealActivity = ACT_HOVER;
				else
					m_IdealActivity = ACT_SWIM;
			}
			else
				m_IdealActivity = m_Activity;

			break;
		}

		case PLAYER_HOLDBOMB: m_IdealActivity = ACT_HOLDBOMB; break;
		case PLAYER_FLINCH: m_IdealActivity = ACT_FLINCH; break;
		case PLAYER_LARGE_FLINCH: m_IdealActivity = ACT_LARGE_FLINCH; break;
	}

	switch (m_IdealActivity)
	{
		case ACT_HOP:
		case ACT_LEAP:
		{
			if (m_Activity == m_IdealActivity)
				return;

			if (m_Activity == ACT_RANGE_ATTACK1)
				strcpy(szAnim, "ref_shoot_");
			else if (m_Activity == ACT_RANGE_ATTACK2)
				strcpy(szAnim, "ref_shoot2_");
			else if (m_Activity == ACT_RELOAD)
				strcpy(szAnim, "ref_reload_");
			else
				strcpy(szAnim, "ref_aim_");

			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);

			if (animDesired == -1)
				animDesired = 0;

			if (pev->sequence != animDesired || !m_fSequenceLoops)
				pev->frame = 0;

			if (!m_fSequenceLoops)
				pev->effects |= EF_NOINTERP;

			if (m_IdealActivity == ACT_LEAP)
			{
				pev->gaitsequence = LookupActivity(ACT_LEAP);
				m_Activity = m_IdealActivity;
			}
			else
			{
				pev->gaitsequence = LookupActivity(ACT_HOP);
				m_Activity = m_IdealActivity;
			}

			break;
		}

		case ACT_RANGE_ATTACK1:
		{
			m_flLastFired = gpGlobals->time;

			if (FBitSet(pev->flags, FL_DUCKING))
				strcpy(szAnim, "crouch_shoot_");
			else
				strcpy(szAnim, "ref_shoot_");

			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);

			if (animDesired == -1)
				animDesired = 0;

			pev->sequence = animDesired;
			pev->frame = 0;
			ResetSequenceInfo();
			m_Activity = m_IdealActivity;
			break;
		}

		case ACT_RANGE_ATTACK2:
		{
			m_flLastFired = gpGlobals->time;

			if (FBitSet(pev->flags, FL_DUCKING))
				strcpy(szAnim, "crouch_shoot2_");
			else
				strcpy(szAnim, "ref_shoot2_");

			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);

			if (animDesired == -1)
				animDesired = 0;

			pev->sequence = animDesired;
			pev->frame = 0;
			ResetSequenceInfo();
			m_Activity = m_IdealActivity;
			break;
		}

		case ACT_RELOAD:
		{
			if (FBitSet(pev->flags, FL_DUCKING))
				strcpy(szAnim, "crouch_reload_");
			else
				strcpy(szAnim, "ref_reload_");

			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);

			if (animDesired == -1)
				animDesired = 0;

			if (pev->sequence != animDesired || !m_fSequenceLoops)
				pev->frame = 0;

			if (!m_fSequenceLoops)
				pev->effects |= EF_NOINTERP;

			m_Activity = m_IdealActivity;
			break;
		}

		case ACT_HOLDBOMB:
		{
			if (FBitSet(pev->flags, FL_DUCKING))
				strcpy(szAnim, "crouch_aim_");
			else
				strcpy(szAnim, "ref_aim_");

			strcat(szAnim, m_szAnimExtention);
			animDesired = LookupSequence(szAnim);

			if (animDesired == -1)
				animDesired = 0;

			m_Activity = m_IdealActivity;
			break;
		}

		case ACT_WALK:
		{
			if ((m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished) && (m_Activity != ACT_RANGE_ATTACK2 || m_fSequenceFinished) && (m_Activity != ACT_FLINCH || m_fSequenceFinished) && (m_Activity != ACT_LARGE_FLINCH || m_fSequenceFinished) && (m_Activity != ACT_RELOAD || m_fSequenceFinished))
			{
				if (speed <= 135 || gpGlobals->time <= m_flLastFired + 4)
				{
					if (FBitSet(pev->flags, FL_DUCKING))
						strcpy(szAnim, "crouch_aim_");
					else
						strcpy(szAnim, "ref_aim_");

					strcat(szAnim, m_szAnimExtention);
					animDesired = LookupSequence(szAnim);

					if (animDesired == -1)
						animDesired = 0;

					m_Activity = ACT_WALK;
				}
				else
				{
					strcpy(szAnim, "run_");
					strcat(szAnim, m_szAnimExtention);
					animDesired = LookupSequence(szAnim);

					if (animDesired == -1)
					{
						if (FBitSet(pev->flags, FL_DUCKING))
							strcpy(szAnim, "crouch_aim_");
						else
							strcpy(szAnim, "ref_aim_");

						strcat(szAnim, m_szAnimExtention);
						animDesired = LookupSequence(szAnim);

						if (animDesired == -1)
							animDesired = 0;

						m_Activity = ACT_RUN;
						pev->gaitsequence = LookupActivity(ACT_RUN);
					}
					else
					{
						m_Activity = ACT_RUN;
						pev->gaitsequence = animDesired;
					}
				}
			}
			else
				animDesired = pev->sequence;

			if (speed <= 135)
				pev->gaitsequence = LookupActivity(ACT_WALK);
			else
				pev->gaitsequence = LookupActivity(ACT_RUN);

			break;
		}

		case ACT_FLINCH:
		case ACT_LARGE_FLINCH:
		{
			m_Activity = m_IdealActivity;

			switch (m_LastHitGroup)
			{
				case HITGROUP_GENERIC:
				{
					if (RANDOM_LONG(0, 1))
						animDesired = LookupSequence("gut_flinch");
					else
						animDesired = LookupSequence("head_flinch");

					break;
				}

				case HITGROUP_HEAD:
				case HITGROUP_CHEST:
				{
					animDesired = LookupSequence("head_flinch");
					break;
				}

				default: animDesired = LookupSequence("gut_flinch");
			}

			if (animDesired == -1)
				animDesired = 0;

			break;
		}

		case ACT_DIESIMPLE:
		{
			if (m_Activity != m_IdealActivity)
			{
				m_Activity = m_IdealActivity;
				m_flDeathThrowTime = 0;
				m_iThrowDirection = THROW_NONE;

				switch (m_LastHitGroup)
				{
					case HITGROUP_GENERIC:
					{
						switch (RANDOM_LONG(0, 8))
						{
							case 0: animDesired = LookupActivity(ACT_DIE_HEADSHOT); m_iThrowDirection = THROW_BACKWARD; break;
							case 1: animDesired = LookupActivity(ACT_DIE_GUTSHOT); break;
							case 2: animDesired = LookupActivity(ACT_DIE_BACKSHOT); m_iThrowDirection = THROW_HITVEL; break;
							case 3: animDesired = LookupActivity(ACT_DIESIMPLE); break;
							case 4: animDesired = LookupActivity(ACT_DIEBACKWARD); m_iThrowDirection = THROW_HITVEL; break;
							case 5: animDesired = LookupActivity(ACT_DIEFORWARD); m_iThrowDirection = THROW_FORWARD; break;
							case 6: animDesired = LookupActivity(ACT_DIE_CHESTSHOT); break;
							case 7: animDesired = LookupActivity(ACT_DIE_GUTSHOT); break;
							case 8: animDesired = LookupActivity(ACT_DIE_HEADSHOT); break;
							default: animDesired = LookupActivity(ACT_DIESIMPLE); break;
						}

						break;
					}

					case HITGROUP_HEAD:
					{
						int random = RANDOM_LONG(0, 8);
						m_bHeadshotKilled = true;

						if (m_bHighDamage == true)
							random++;

						switch (random)
						{
							case 0: m_iThrowDirection = THROW_NONE; break;
							case 1: m_iThrowDirection = THROW_BACKWARD; break;
							case 2: m_iThrowDirection = THROW_BACKWARD; break;
							case 3: m_iThrowDirection = THROW_FORWARD; break;
							case 4: m_iThrowDirection = THROW_FORWARD; break;
							case 5: m_iThrowDirection = THROW_HITVEL; break;
							case 6: m_iThrowDirection = THROW_HITVEL; break;
							case 7: m_iThrowDirection = THROW_NONE; break;
							case 8: m_iThrowDirection = THROW_NONE; break;
							default: m_iThrowDirection = THROW_NONE; break;
						}

						animDesired = LookupActivity(ACT_DIE_HEADSHOT);
						break;
					}

					case HITGROUP_CHEST: animDesired = LookupActivity(ACT_DIE_CHESTSHOT); break;
					case HITGROUP_STOMACH: animDesired = LookupActivity(ACT_DIE_GUTSHOT); break;
					case HITGROUP_LEFTARM: animDesired = LookupSequence("left"); break;
					case HITGROUP_RIGHTARM:
					{
						m_iThrowDirection = RANDOM_LONG(0, 1) ? THROW_HITVEL : THROW_HITVEL_MINUS_AIRVEL;
						animDesired = LookupSequence("right");
						break;
					}

					case HITGROUP_LEFTLEG:
					case HITGROUP_RIGHTLEG: animDesired = LookupActivity(ACT_DIESIMPLE); break;
				}

				if (FBitSet(pev->flags, FL_DUCKING))
				{
					animDesired = LookupSequence("crouch_die");
					m_iThrowDirection = THROW_BACKWARD;
				}
				else
				{
					if (m_bKilledByBomb == true || m_bKilledByGrenade == true)
					{
						UTIL_MakeVectors(pev->angles);

						if (DotProduct(gpGlobals->v_forward, m_vBlastVector) > 0)
							animDesired = LookupSequence("left");
						else if (RANDOM_LONG(0, 1))
							animDesired = LookupSequence("crouch_die");
						else
							animDesired = LookupActivity(ACT_DIE_HEADSHOT);

						if (m_bKilledByBomb == true)
							m_iThrowDirection = THROW_BOMB;
						else if (m_bKilledByGrenade == true)
							m_iThrowDirection = THROW_GRENADE;
					}
				}

				if (animDesired == -1)
					animDesired = 0;

				if (pev->sequence != animDesired)
				{
					pev->gaitsequence = 0;
					pev->sequence = animDesired;
					pev->frame = 0;
					ResetSequenceInfo();
				}
			}

			return;
		}

		default:
		{
			if (m_Activity == m_IdealActivity)
				return;

			m_Activity = m_IdealActivity;
			animDesired = LookupActivity(m_IdealActivity);

			if (pev->gaitsequence != animDesired)
			{
				pev->gaitsequence = 0;
				pev->sequence = animDesired;
				pev->frame = 0;
				ResetSequenceInfo();
			}

			return;
		}
	}

	if (pev->gaitsequence != hopSeq && pev->gaitsequence != leapSeq)
	{
		if (FBitSet(pev->flags, FL_DUCKING))
		{
			if (speed)
				pev->gaitsequence = LookupActivity(ACT_CROUCH);
			else
				pev->gaitsequence = LookupActivity(ACT_CROUCHIDLE);
		}
		else if (speed > 135)
		{
			if (gpGlobals->time > m_flLastFired + 4)
			{
				if (m_Activity != ACT_FLINCH && m_Activity != ACT_LARGE_FLINCH)
				{
					strcpy(szAnim, "run_");
					strcat(szAnim, m_szAnimExtention);
					animDesired = LookupSequence(szAnim);

					if (animDesired == -1)
					{
						if (FBitSet(pev->flags, FL_DUCKING))
							strcpy(szAnim, "crouch_aim_");
						else
							strcpy(szAnim, "ref_aim_");

						strcat(szAnim, m_szAnimExtention);
						animDesired = LookupSequence(szAnim);
					}
					else
						pev->gaitsequence = animDesired;

					m_Activity = ACT_RUN;
				}
			}

			pev->gaitsequence = LookupActivity(ACT_RUN);
		}
		else if (speed > 0)
			pev->gaitsequence = LookupActivity(ACT_WALK);
		else
			pev->gaitsequence = LookupActivity(ACT_IDLE);
	}

	if (pev->sequence == animDesired)
		return;

	pev->sequence = animDesired;
	pev->frame = 0;
	ResetSequenceInfo();
}

void CBasePlayer::TabulateAmmo(void)
{
	ammo_buckshot = AmmoInventory(GetAmmoIndex("buckshot"));
	ammo_9mm = AmmoInventory(GetAmmoIndex("9mm"));
	ammo_556nato = AmmoInventory(GetAmmoIndex("556Nato"));
	ammo_556natobox = AmmoInventory(GetAmmoIndex("556NatoBox"));
	ammo_762nato = AmmoInventory(GetAmmoIndex("762Nato"));
	ammo_45acp = AmmoInventory(GetAmmoIndex("45acp"));
	ammo_50ae = AmmoInventory(GetAmmoIndex("50AE"));
	ammo_338mag = AmmoInventory(GetAmmoIndex("338Magnum"));
	ammo_57mm = AmmoInventory(GetAmmoIndex("57mm"));
	ammo_357sig = AmmoInventory(GetAmmoIndex("357SIG"));
}

#define AIRTIME 12

void CBasePlayer::WaterMove(void)
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP || pev->movetype == MOVETYPE_NONE)
		return;

	if (pev->health < 0)
		return;

	if (pev->waterlevel != 3)
	{
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", VOL_NORM, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", VOL_NORM, ATTN_NORM);

		pev->air_finished = gpGlobals->time + 12;
		pev->dmg = 2;

		if (m_idrowndmg > m_idrownrestored)
		{
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
			m_bitsDamageType &= ~DMG_DROWN;
			m_bitsDamageType |= DMG_DROWNRECOVER;
		}
	}
	else
	{
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		m_bitsDamageType &= ~DMG_DROWNRECOVER;

		if (gpGlobals->time > pev->air_finished)
		{
			if (gpGlobals->time > pev->pain_finished)
			{
				pev->dmg += 1;

				if (pev->dmg > 5)
					pev->dmg = 5;

				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;
				m_idrowndmg += pev->dmg;
			}
		}
		else
			m_bitsDamageType &= ~DMG_DROWN;
	}

	if (!pev->waterlevel)
	{
		if (pev->flags & FL_INWATER)
			pev->flags &= ~FL_INWATER;

		return;
	}

	air = (int)(pev->air_finished - gpGlobals->time);

	if (!RANDOM_LONG(0, 31) && RANDOM_LONG(0, AIRTIME - 1) >= air)
	{
		switch (RANDOM_LONG(0, 3))
		{
			case 0: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM); break;
			case 2: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM); break;
			case 3: EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM); break;
		}
	}

	if (pev->watertype == CONTENT_LAVA)
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->waterlevel * 10, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME)
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->waterlevel * 4, DMG_ACID);
	}

	if (!(pev->flags & FL_INWATER))
	{
		pev->flags |= FL_INWATER;
		pev->dmgtime = 0;
	}
}

BOOL CBasePlayer::IsOnLadder(void)
{
	return pev->movetype == MOVETYPE_FLY;
}

LINK_ENTITY_TO_CLASS(weapon_shield, CWShield);

void CWShield::Spawn(void)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	SET_MODEL(ENT(pev), "models/w_shield.mdl");
}

void CWShield::Touch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (pPlayer->pev->deadflag == DEAD_NO)
	{
		if (m_hEntToIgnoreTouchesFrom != 0 && pPlayer == m_hEntToIgnoreTouchesFrom)
		{
			if (m_flTimeToIgnoreTouches > gpGlobals->time)
				return;

			m_hEntToIgnoreTouchesFrom = NULL;
		}
	}

	if (pPlayer->m_bHasPrimary)
		return;

	if (pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY] != NULL && pPlayer->m_rgpPlayerItems[WPNSLOT_SECONDARY]->m_iId == WEAPON_ELITE)
		return;

	if (pPlayer->m_pActiveItem)
	{
		if (!pPlayer->m_pActiveItem->CanHolster())
			return;
	}

	if (pPlayer->m_bIsVIP != true)
	{
		pPlayer->GiveShield(true);
		EMIT_SOUND(edict(), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);
		UTIL_Remove(this);

		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CWShield::SetCantBePickedUpByUser(CBaseEntity *pEntity, float time)
{
	m_hEntToIgnoreTouchesFrom = pEntity;
	m_flTimeToIgnoreTouches = gpGlobals->time + time;
}

void CBasePlayer::GiveShield(bool bRetire)
{
	m_bOwnsShield = true;
	m_bHasPrimary = true;

	if (m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		if (bRetire == true)
		{
			if (m_rgAmmo[pWeapon->m_iPrimaryAmmoType] > 0)
				pWeapon->Holster();

			if (!pWeapon->Deploy())
				pWeapon->RetireWeapon();
		}
	}

	pev->gamestate = 0;
}

void CBasePlayer::RemoveShield(void)
{
	if (HasShield())
	{
		m_bOwnsShield = false;
		m_bHasPrimary = false;
		m_bShieldDrawn = false;
		pev->gamestate = 1;

		UpdateShieldCrosshair(true);
	}
}

void CBasePlayer::DropShield(bool bDeploy)
{
	if (HasShield() == false)
		return;

	if (!m_pActiveItem||(m_pActiveItem && !m_pActiveItem->CanHolster()))
		return;

	if (m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		if (pWeapon->m_iId == WEAPON_HEGRENADE || pWeapon->m_iId == WEAPON_FLASHBANG || pWeapon->m_iId == WEAPON_SMOKEGRENADE)
		{
			if (m_rgAmmo[pWeapon->m_iPrimaryAmmoType] <= 0)
				g_pGameRules->GetNextBestWeapon(this, pWeapon);
		}
	}

	if (m_pActiveItem&&IsThrowingGrenade())
		m_pActiveItem->Holster();

	if (IsReloading())
		StopReload();

	DrawnShiled();
	RemoveShield();

	if (m_pActiveItem)
	{
		if (bDeploy == true)
			m_pActiveItem->Deploy();
	}

	UTIL_MakeVectors(pev->angles);

	CWShield *pShield = (CWShield *)CBaseEntity::Create("weapon_shield", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
	pShield->pev->angles.x = 0;
	pShield->pev->angles.z = 0;
	pShield->pev->velocity = gpGlobals->v_forward * 400;
	pShield->SetThink(&CBaseEntity::SUB_Remove);
	pShield->pev->nextthink = gpGlobals->time + 300;
	pShield->SetCantBePickedUpByUser(this, 2.0);
}

bool CBasePlayer::IsReloading(void)
{
	if (m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		if (pWeapon->m_fInReload)
			return true;
	}

	return false;
}

bool CBasePlayer::IsThrowingGrenade(void)
{
	if (m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

		if (pWeapon->m_flStartThrow != 0)
			return true;
	}

	return false;
}

bool CBasePlayer::IsProtectedByShield(void)
{
	if (HasShield())
	{
		if (m_bShieldDrawn != false)
			return true;
	}

	return false;
}

void CBasePlayer::StopReload(void)
{
	CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

	pWeapon->m_fInReload = FALSE;
	m_flNextAttack = UTIL_WeaponTimeBase();
}

void CBasePlayer::DrawnShiled(void)
{
	if (IsProtectedByShield() && m_pActiveItem)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;
		pWeapon->SecondaryAttack();
	}

	m_bShieldDrawn = false;
}

bool CBasePlayer::HasShield(void)
{
	return m_bOwnsShield;
}

void CBasePlayer::AddAccount(int amount, bool bTrackChange)
{
	m_iAccount += amount;

	if (m_iAccount < 0)
		m_iAccount = 0;
	else if (m_iAccount > STARTMONEY_MAX)
		m_iAccount = STARTMONEY_MAX;

	MESSAGE_BEGIN(MSG_ONE, gmsgMoney, NULL, pev);
	WRITE_LONG(m_iAccount);
	WRITE_BYTE(bTrackChange);
	MESSAGE_END();
}

void CBasePlayer::ResetMenu(void)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
	WRITE_SHORT(0);
	WRITE_CHAR(0);
	WRITE_BYTE(0);
	WRITE_STRING("");
	MESSAGE_END();
}

void CBasePlayer::SyncRoundTimer(void)
{
	float tmRemaining;

	if (g_pGameRules->IsMultiplayer())
		tmRemaining = g_pGameRules->TimeRemaining();
	else
		tmRemaining = 0;

	if (tmRemaining < 0)
		tmRemaining = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgRoundTime, NULL, pev);
	WRITE_SHORT(tmRemaining);
	MESSAGE_END();

	if (g_pGameRules->IsCareer())
	{
	}
}

void CBasePlayer::RemoveLevelText(void)
{
	ResetMenu();
}

#define MENUBUF_LEN 51

void CBasePlayer::MenuPrint(const char *pszText)
{
	char szBuffer[MENUBUF_LEN + 1];

	while (strlen(pszText) >= MENUBUF_LEN - 1)
	{
		strncpy(szBuffer, pszText, MENUBUF_LEN - 1);
		szBuffer[MENUBUF_LEN - 1] = '\0';
		pszText += MENUBUF_LEN - 1;

		MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
		WRITE_SHORT(0xFFFF);
		WRITE_CHAR(-1);
		WRITE_BYTE(1);
		WRITE_STRING(szBuffer);
		MESSAGE_END();
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
	WRITE_SHORT(0xFFFF);
	WRITE_CHAR(-1);
	WRITE_BYTE(0);
	WRITE_STRING(pszText);
	MESSAGE_END();
}

void CBasePlayer::MakeVIP(void)
{
	pev->body = 0;
	m_iModelName = MODEL_VIP;

	g_engfuncs.pfnSetClientKeyValue(entindex(), g_engfuncs.pfnGetInfoKeyBuffer(edict()), "model", "vip");
	UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Became_VIP\"\n", STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()));

	m_iTeam = TEAM_CT;
	m_bIsVIP = true;
	m_bNotKilled = false;
	g_pGameRules->m_pVIP = this;
	g_pGameRules->m_iConsecutiveVIP = 1;
}

extern void CheckStartMoney(void);

void CBasePlayer::JoiningThink(void)
{
	switch (m_iJoiningState)
	{
		case JOINED: return;
		case SHOWLTEXT:
		{
			RemoveLevelText();
			m_iJoiningState = SHOWTEAMSELECT;

			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
			WRITE_BYTE(STATUSICON_HIDE);
			WRITE_STRING("defuser");
			MESSAGE_END();

			m_bHasDefuser = false;
			m_bMissionBriefing = false;
			m_fLastMovement = gpGlobals->time;

			SendItemStatus(this);
			break;
		}

		case READINGLTEXT:
		{
			if (m_afButtonPressed & (IN_ATTACK | IN_ATTACK2 | IN_JUMP))
			{
				m_afButtonPressed &= ~(IN_ATTACK | IN_ATTACK2 | IN_JUMP);
				RemoveLevelText();
				m_iJoiningState = SHOWTEAMSELECT;
			}

			break;
		}

		case GETINTOGAME:
		{
			m_bNotKilled = false;
			m_iIgnoreGlobalChat = IGNOREMSG_NONE;
			m_iTeamKills = 0;
			m_iFOV = 90;
			memset(&m_rebuyStruct, 0, sizeof(m_rebuyStruct));
			m_bIsInRebuy = false;
			m_bJustConnected = false;
			ResetMaxSpeed();
			m_iJoiningState = JOINED;

			if (g_pGameRules->m_bMapHasEscapeZone == true && m_iTeam == TEAM_CT)
			{
				m_iAccount = 0;
				CheckStartMoney();
				AddAccount(startmoney.value);
			}

			if (g_pGameRules->FPlayerCanRespawn(this))
			{
				Spawn();
				g_pGameRules->CheckWinConditions();

				if (!g_pGameRules->m_fTeamCount && g_pGameRules->m_bMapHasBombTarget && !g_pGameRules->IsThereABomber() && !g_pGameRules->IsThereABomb())
					g_pGameRules->GiveC4();

				if (m_iTeam == TEAM_TERRORIST)
					g_pGameRules->m_iNumEscapers++;
			}
			else
			{
				pev->deadflag = DEAD_RESPAWNABLE;

				if (pev->classname)
					RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

				pev->classname = MAKE_STRING("player");
				AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

				pev->flags &= FL_PROXY;
				pev->flags |= (FL_SPECTATOR | FL_CLIENT);

				edict_t *pSpot = (edict_t *)g_pGameRules->GetPlayerSpawnSpot(this);
				entvars_t *pevSpot = VARS(pSpot);
				StartObserver(pevSpot->origin, pevSpot->angles);
				g_pGameRules->CheckWinConditions();

				MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
				WRITE_BYTE(ENTINDEX(edict()));
				WRITE_STRING(GetTeam(m_iTeam));
				MESSAGE_END();

				MESSAGE_BEGIN(MSG_ALL, gmsgLocation);
				WRITE_BYTE(ENTINDEX(edict()));
				WRITE_STRING("");
				MESSAGE_END();

				MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
				WRITE_BYTE(ENTINDEX(edict()));
				WRITE_SHORT(pev->frags);
				WRITE_SHORT(m_iDeaths);
				WRITE_SHORT(0);
				WRITE_SHORT(m_iTeam);
				MESSAGE_END();

				if (!(m_flDisplayHistory & DHF_SPEC_DUCK))
				{
					HintMessage("#Spec_Duck", TRUE, TRUE);
					m_flDisplayHistory |= DHF_SPEC_DUCK;
				}
			}

			return;
		}
	}

	if (m_pIntroCamera && m_fIntroCamTime <= gpGlobals->time)
		MoveToNextIntroCamera();
}

void CBasePlayer::Disappear(void)
{
	if (m_pTank)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (pSound)
		pSound->Reset();

	m_fSequenceFinished = TRUE;
	pev->modelindex = m_modelIndexPlayer;
	pev->view_ofs = Vector(0, 0, -8);
	pev->deadflag = DEAD_DYING;
	pev->solid = SOLID_NOT;
	pev->flags &= FL_ONGROUND;

	SetSuitUpdate(NULL, FALSE, 0);

	m_iClientHealth = 0;
	MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
	WRITE_BYTE(m_iClientHealth);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0xFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	SendFOV(0);

	g_pGameRules->CheckWinConditions();
	m_bNotKilled = false;

	if (m_bHasC4 == true)
	{
		DropPlayerItem("weapon_c4");
		SetProgressBarTime(0);
	}

	if (m_bHasDefuser == true)
	{
		m_bHasDefuser = false;
		pev->body = 0;
		GiveNamedItem("item_thighpack");

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("defuser");
		MESSAGE_END();

		SendItemStatus(this);
		SetProgressBarTime(0);
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
	WRITE_BYTE(STATUSICON_HIDE);
	WRITE_STRING("buyzone");
	MESSAGE_END();

	if (m_iMenu >= Menu_Buy)
	{
		if (m_iMenu <= Menu_BuyItem)
		{
			CLIENT_COMMAND(ENT(pev), "slot10\n");
		}
		else if (m_iMenu == Menu_ClientBuy)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
			MESSAGE_END();
		}
	}

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->angles.x = 0;
	pev->angles.z = 0;
}

void CBasePlayer::PlayerDeathThink(void)
{
	if (m_iJoiningState != JOINED)
		return;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		float flForward = pev->velocity.Length() - 20;

		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	if (HasWeapons())
		PackDeadPlayerItems();

	if (pev->modelindex && !m_fSequenceFinished && pev->deadflag == DEAD_DYING)
	{
		StudioFrameAdvance();
		return;
	}

	if (pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND))
		pev->movetype = MOVETYPE_NONE;

	if (pev->deadflag == DEAD_DYING)
	{
		m_fDeadTime = gpGlobals->time;
		pev->deadflag = DEAD_DEAD;
	}

	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0;

	BOOL fAnyButtonDown = (pev->button & ~IN_SCORE);

	if (g_pGameRules->IsMultiplayer())
	{
		if (gpGlobals->time > m_fDeadTime + 3)
		{
			if (!(m_afPhysicsFlags & PFLAG_OBSERVER))
			{
				SpawnClientSideCorpse();

				if (pev->view_ofs != g_vecZero)
					StartObserver(pev->origin, pev->angles);
			}
		}
	}

	if (pev->deadflag == DEAD_DEAD && m_iTeam != TEAM_UNASSIGNED && m_iTeam != TEAM_SPECTATOR)
	{
		if (fAnyButtonDown)
			return;

		if (g_pGameRules->FPlayerCanRespawn(this))
		{
			pev->deadflag = DEAD_RESPAWNABLE;

			if (g_pGameRules->IsMultiplayer())
				g_pGameRules->CheckWinConditions();
		}

		pev->nextthink = gpGlobals->time + 0.1;
	}
	else if (pev->deadflag == DEAD_RESPAWNABLE)
	{
		respawn(pev, FALSE);
		pev->button = 0;
		pev->nextthink = -1;
	}
}

void CBasePlayer::RoundRespawn(void)
{
	m_canSwitchObserverModes = true;

	if (m_bJustKilledTeammate == true && CVAR_GET_FLOAT("mp_tkpunish") != 0)
	{
		m_bJustKilledTeammate = false;
		CLIENT_COMMAND(ENT(pev), "kill\n");
		m_bPunishedForTK = true;
	}

	if (m_iMenu != Menu_ChooseAppearance)
	{
		respawn(pev, FALSE);
		pev->button = 0;
		pev->nextthink = -1;
	}

	if (m_pActiveItem)
	{
		if (m_pActiveItem->iItemSlot() == WPNSLOT_GRENADE)
			SwitchWeapon(m_pActiveItem);
	}

	m_lastLocation[0] = 0;
}

void CBasePlayer::StartObserver(const Vector &vecPosition, const Vector &vecViewAngle)
{
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_KILLPLAYERATTACHMENTS);
	WRITE_BYTE(ENTINDEX(edict()));
	MESSAGE_END();

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	if (m_pTank)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	SetSuitUpdate(NULL, FALSE, 0);

	MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(0xFF);
	WRITE_BYTE(0xFF);
	MESSAGE_END();

	SendFOV(0);

	m_iHideHUD = (HIDEHUD_WEAPONS | HIDEHUD_HEALTH);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;

	UTIL_SetOrigin(pev, vecPosition);

	m_afPhysicsFlags &= ~PFLAG_DUCKING;
	pev->flags &= ~FL_DUCKING;
	pev->health = 1;
	m_iObserverC4State = 0;
	m_bObserverHasDefuser = false;
	m_iObserverWeapon = 0;
	m_flNextObserverInput = 0;
	pev->iuser1 = OBS_NONE;

	static int iFirstTime = 1;

	if (iFirstTime && g_pGameRules && g_pGameRules->IsCareer() && !IsBot())
	{
		Observer_SetMode(OBS_CHASE_LOCKED);
		CLIENT_COMMAND(edict(), "spec_autodirector_internal 1\n");
		iFirstTime = 0;
	}
	else
		Observer_SetMode(m_iObserverLastMode);

	ResetMaxSpeed();

	MESSAGE_BEGIN(MSG_ALL, gmsgSpectator);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_BYTE(1);
	MESSAGE_END();
}

#define PLAYER_SEARCH_RADIUS 64.0

void CBasePlayer::PlayerUse(void)
{
	if (!((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE))
		return;

	if (m_afButtonPressed & IN_USE)
	{
		if (m_pTank)
		{
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
			return;
		}

		if (m_afPhysicsFlags & PFLAG_ONTRAIN)
		{
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			CBaseEntity *pTrain = Instance(pev->groundentity);

			if (pTrain && pTrain->Classify() == CLASS_VEHICLE)
				((CFuncVehicle *)pTrain)->m_pDriver = NULL;

			return;
		}

		CBaseEntity *pTrain = Instance(pev->groundentity);

		if (pTrain && !(pev->button & IN_JUMP) && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(pev))
		{
			m_afPhysicsFlags |= PFLAG_ONTRAIN;
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);

			if (pTrain->Classify() == CLASS_VEHICLE)
			{
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/vehicle_ignition.wav", 0.8, ATTN_NORM);
				((CFuncVehicle *)pTrain)->m_pDriver = this;
			}
			else
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);

			return;
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	float flMaxDot = VIEW_FIELD_NARROW;

	UTIL_MakeVectors(pev->v_angle);

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != NULL)
	{
		if (pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
		{
			Vector vecLOS = VecBModelOrigin(pObject->pev) - (pev->origin + pev->view_ofs);
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);
			float flDot = DotProduct(vecLOS, gpGlobals->v_forward);

			if (flDot > flMaxDot)
			{
				flMaxDot = flDot;
				pClosest = pObject;
			}
		}
	}

	pObject = pClosest;

	if (pObject)
	{
		int caps = pObject->ObjectCaps();

		if (m_afButtonPressed & IN_USE)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if (((pev->button & IN_USE) && (caps & FCAP_CONTINUOUS_USE)) || ((m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE))))
		{
			if (caps & FCAP_CONTINUOUS_USE)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
			return;
		}

		if ((m_afButtonReleased & IN_USE) && (pObject->ObjectCaps() & FCAP_ONOFF_USE))
			pObject->Use(this, this, USE_SET, 0);
	}
	else if (m_afButtonPressed & IN_USE)
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
}

void CBasePlayer::HostageUsed(void)
{
	if (m_flDisplayHistory & DHF_HOSTAGE_USED)
		return;

	if (m_iTeam == TEAM_TERRORIST)
		HintMessage("#Hint_use_hostage_to_stop_him");
	else if (m_iTeam == TEAM_CT)
		HintMessage("#Hint_lead_hostage_to_rescue_point");

	m_flDisplayHistory |= DHF_HOSTAGE_USED;
}

void CBasePlayer::Jump(void)
{
	if (pev->flags & FL_WATERJUMP)
		return;

	if (pev->waterlevel >= 2)
		return;

	if (!(m_afButtonPressed & IN_JUMP))
		return;

	if (!(pev->flags & FL_ONGROUND) || !pev->groundentity)
		return;

	//UTIL_MakeVectors(pev->angles);
	SetAnimation(PLAYER_JUMP);

	if (pev->flags & FL_DUCKING || m_afPhysicsFlags & PFLAG_DUCKING)
	{
		if (m_fLongJump && (pev->button & IN_DUCK) && (gpGlobals->time - m_flDuckTime < 1) && pev->velocity.Length() > 50)
			SetAnimation(PLAYER_SUPERJUMP);
	}

	entvars_t *pevGround = VARS(pev->groundentity);

	if (pevGround)
	{
		if (pevGround->flags & FL_CONVEYOR)
			pev->velocity = pev->velocity + pev->basevelocity;

		if (FClassnameIs(pevGround, "func_tracktrain") || FClassnameIs(pevGround, "func_train") || FClassnameIs(pevGround, "func_vehicle"))
			pev->velocity = pevGround->velocity + pev->velocity;
	}
}

void FixPlayerCrouchStuck(edict_t *pPlayer)
{
	for (int i = 0; i < 18; i++)
	{
		TraceResult trace;
		UTIL_TraceHull(pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace);

		if (!trace.fStartSolid)
			break;

		pPlayer->v.origin.z++;
	}
}

void CBasePlayer::Duck(void)
{
	if (pev->button & IN_DUCK)
		SetAnimation(PLAYER_WALK);
}

int CBasePlayer::Classify(void)
{
	return CLASS_PLAYER;
}

void CBasePlayer::AddPoints(int score, BOOL bAllowNegativeScore)
{
	if (score < 0 && !bAllowNegativeScore)
	{
		if (pev->frags < 0)
			return;

		if (-score > pev->frags)
			score = -pev->frags;
	}

	pev->frags += score;

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_SHORT(pev->frags);
	WRITE_SHORT(m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(m_iTeam);
	MESSAGE_END();
}

void CBasePlayer::AddPointsToTeam(int score, BOOL bAllowNegativeScore)
{
	int index = entindex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pOther = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (pOther && i != index)
		{
			if (g_pGameRules->PlayerRelationship(this, pOther) == GR_TEAMMATE)
				pOther->AddPoints(score, bAllowNegativeScore);
		}
	}
}

bool CBasePlayer::CanPlayerBuy(bool display)
{
	if (!g_pGameRules->IsMultiplayer())
		return CHalfLifeTraining::PlayerCanBuy(this);

	if (pev->deadflag != DEAD_NO)
		return false;

	if (!(m_signals.GetState() & SIGNAL_BUY))
		return false;

	int buyTime = (int)CVAR_GET_FLOAT("mp_buytime") * 60;

	if (buyTime < 15)
	{
		buyTime = 15;
		CVAR_SET_FLOAT("mp_buytime", 1 / (60 / 15));
	}

	if (gpGlobals->time - g_pGameRules->m_fRoundCount > buyTime)
	{
		if (display == true)
			ClientPrint(pev, HUD_PRINTCENTER, "#Cant_buy", UTIL_dtos1(buyTime));

		return false;
	}

	if (m_bIsVIP)
	{
		if (display == true)
			ClientPrint(pev, HUD_PRINTCENTER, "#VIP_cant_buy");

		return false;
	}

	if (g_pGameRules->m_bCTCantBuy == true && m_iTeam == TEAM_CT)
	{
		if (display == true)
			ClientPrint(pev, HUD_PRINTCENTER, "#CT_cant_buy");

		return false;
	}

	if (g_pGameRules->m_bTCantBuy == true && m_iTeam == TEAM_TERRORIST)
	{
		if (display == true)
			ClientPrint(pev, HUD_PRINTCENTER, "#Terrorist_cant_buy");

		return false;
	}

	return true;
}

void CBasePlayer::InitStatusBar(void)
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString0[0] = '\0';
}

void CBasePlayer::UpdateStatusBar(void)
{
	int newSBarState[SBAR_END];
	memset(newSBarState, 0, sizeof(newSBarState));

	char sbuf0[SBAR_STRING_SIZE];
	strcpy(sbuf0, m_SbarString0);

	TraceResult tr;
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	Vector vecSrc = EyePosition();
	float range = (pev->flags & FL_SPECTATOR) ? MAX_SPECTATOR_ID_RANGE : MAX_ID_RANGE;
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * range);
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1)
	{
		if (!FNullEnt(tr.pHit))
		{
			CBaseEntity *pEntity = Instance(tr.pHit);

			if (gpGlobals->time >= m_blindUntilTime && pEntity->Classify() == CLASS_PLAYER)
			{
				CBasePlayer *pTarget = (CBasePlayer *)pEntity;
				newSBarState[SBAR_ID_TARGETNAME] = ENTINDEX(pTarget->edict());
				newSBarState[SBAR_ID_TARGETTYPE] = (pTarget->m_iTeam == m_iTeam) ? SBAR_TARGETTYPE_TEAMMATE : SBAR_TARGETTYPE_ENEMY;

				if (m_iTeam == pTarget->m_iTeam || IsObserver())
				{
					if (playerid.value != PLAYERID_OFF || IsObserver())
						strcpy(sbuf0, "1 %c1: %p2\n2  %h: %i3%%");
					else
						strcpy(sbuf0, " ");

					newSBarState[SBAR_ID_TARGETHEALTH] = (pTarget->pev->health / pTarget->pev->max_health) * 100;

					if (!(m_flDisplayHistory & DHF_FRIEND_SEEN) && !FBitSet(pev->flags, FL_SPECTATOR))
					{
						m_flDisplayHistory |= DHF_FRIEND_SEEN;
						HintMessage("#Hint_spotted_a_friend");
					}
				}
				else if (!IsObserver())
				{
					if (playerid.value != PLAYERID_TEAMONLY && playerid.value != PLAYERID_OFF)
						strcpy(sbuf0, "1 %c1: %p2");
					else
						strcpy(sbuf0, " ");

					if (!(m_flDisplayHistory & DHF_ENEMY_SEEN))
					{
						m_flDisplayHistory |= DHF_ENEMY_SEEN;
						HintMessage("#Hint_spotted_an_enemy");
					}
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 2;
			}
			else if (pEntity->Classify() == CLASS_HUMAN_PASSIVE)
			{
				if (playerid.value != PLAYERID_OFF || IsObserver())
					strcpy(sbuf0, "1 %c1  %h: %i3%%");
				else
					strcpy(sbuf0, " ");

				newSBarState[SBAR_ID_TARGETTYPE] = SBAR_TARGETTYPE_HOSTAGE;
				newSBarState[SBAR_ID_TARGETHEALTH] = (pEntity->pev->health / pEntity->pev->max_health) * 100;

				if (!(m_flDisplayHistory & DHF_HOSTAGE_SEEN_FAR) && tr.flFraction > 0.1)
				{
					m_flDisplayHistory |= DHF_HOSTAGE_SEEN_FAR;

					if (m_iTeam == TEAM_TERRORIST)
						HintMessage("#Hint_prevent_hostage_rescue", TRUE);
					else if (m_iTeam == TEAM_CT)
						HintMessage("#Hint_rescue_the_hostages", TRUE);
				}
				else if (m_iTeam == TEAM_CT && !(m_flDisplayHistory & DHF_HOSTAGE_SEEN_NEAR) && tr.flFraction <= 0.1)
				{
					m_flDisplayHistory |= (DHF_HOSTAGE_SEEN_NEAR | DHF_HOSTAGE_SEEN_FAR);
					HintMessage("#Hint_press_use_so_hostage_will_follow");
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 2;
			}
		}
	}
	else if (m_flStatusBarDisappearDelay > gpGlobals->time)
	{
		newSBarState[SBAR_ID_TARGETTYPE] = m_izSBarState[SBAR_ID_TARGETTYPE];
		newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
		newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
	}

	BOOL bForceResend = FALSE;

	if (strcmp(sbuf0, m_SbarString0))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, NULL, pev);
		WRITE_BYTE(0);
		WRITE_STRING(sbuf0);
		MESSAGE_END();

		strcpy(m_SbarString0, sbuf0);
		bForceResend = TRUE;
	}

	for (int i = 1; i < SBAR_END; i++)
	{
		if (newSBarState[i] != m_izSBarState[i] || bForceResend)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusValue, NULL, pev);
			WRITE_BYTE(i);
			WRITE_SHORT(newSBarState[i]);
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

void CBasePlayer::PreThink(void)
{
	int buttonsChanged = pev->button ^ m_afButtonLast;

	if (pev->button != m_afButtonLast)
		m_fLastMovement = gpGlobals->time;

	m_afButtonPressed = buttonsChanged & pev->button;
	m_afButtonReleased = buttonsChanged & ~pev->button;
	m_hintMessageQueue.Update(this);
	g_pGameRules->PlayerThink(this);

	if (g_fGameOver)
		return;

	if (m_iJoiningState != JOINED)
		JoiningThink();

	if (m_bMissionBriefing == true)
	{
		if (m_afButtonPressed & (IN_ATTACK | IN_ATTACK2))
		{
			m_afButtonPressed &= ~(IN_ATTACK | IN_ATTACK2);
			RemoveLevelText();
			m_bMissionBriefing = false;
		}
	}

	UTIL_MakeVectors(pev->v_angle);
	ItemPreFrame();
	WaterMove();

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (m_flVelocityModifier < 1)
		{
			m_flVelocityModifier += 0.01;
			pev->velocity = pev->velocity * m_flVelocityModifier;
		}

		if (m_flVelocityModifier > 1)
			m_flVelocityModifier = 1;
	}

	if (m_flIdleCheckTime <= gpGlobals->time || m_flIdleCheckTime == 0)
	{
		m_flIdleCheckTime = gpGlobals->time + 5;

		if (g_pGameRules&&gpGlobals->time - m_fLastMovement > g_pGameRules->m_fMaxIdlePeriod)
		{
			if (CVAR_GET_FLOAT("mp_autokick") != 0)
			{
				UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Game_idle_kick\" (auto)\n", STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()), GetTeam(m_iTeam));
				UTIL_ClientPrintAll(HUD_PRINTCONSOLE, "#Game_idle_kick", STRING(pev->netname));
				SERVER_COMMAND(UTIL_VarArgs("kick %s\n", STRING(pev->netname)));
				m_fLastMovement = gpGlobals->time;
			}
		}
	}

	if (g_pGameRules && g_pGameRules->FAllowFlashlight())
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	UpdateClientData();
	CheckTimeBasedDamage();
	CheckSuitUpdate();

	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	if (IsObserver() && (m_afPhysicsFlags & PFLAG_OBSERVER))
	{
		Observer_HandleButtons();
		Observer_CheckTarget();
		Observer_CheckProperties();
		return;
	}

	if (pev->deadflag >= DEAD_DYING && pev->deadflag != DEAD_RESPAWNABLE)
	{
		PlayerDeathThink();
		return;
	}

	CBaseEntity *pGroundEntity = Instance(pev->groundentity);

	if (pGroundEntity && pGroundEntity->Classify() == CLASS_VEHICLE)
		pev->iuser4 = 1;
	else
		pev->iuser4 = 0;

	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
	{
		CBaseEntity *pTrain = Instance(pev->groundentity);
		float vel;

		if (!pTrain)
		{
			TraceResult trainTrace;
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, ENT(pev), &trainTrace);

			if (trainTrace.flFraction != 1 || trainTrace.pHit)
				pTrain = Instance(trainTrace.pHit);

			if (!pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev))
			{
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				((CFuncVehicle *)pTrain)->m_pDriver = NULL;
				return;
			}
		}
		else if (!FBitSet(pev->flags, FL_ONGROUND) || (pTrain->pev->spawnflags & SF_TRACKTRAIN_NOCONTROL))
		{
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			((CFuncVehicle *)pTrain)->m_pDriver = NULL;
			return;
		}

		vel = 0;
		pev->velocity = g_vecZero;

		if (pTrain->Classify() == CLASS_VEHICLE)
		{
			if (pev->button & IN_FORWARD)
			{
				vel = 1;
				pTrain->Use(this, this, USE_SET, vel);
			}

			if (pev->button & IN_BACK)
			{
				vel = -1;
				pTrain->Use(this, this, USE_SET, vel);
			}

			if (pev->button & IN_MOVELEFT)
			{
				vel = 20;
				pTrain->Use(this, this, USE_SET, vel);
			}

			if (pev->button & IN_MOVERIGHT)
			{
				vel = 30;
				pTrain->Use(this, this, USE_SET, vel);
			}
		}
		else
		{
			if (m_afButtonPressed & IN_FORWARD)
			{
				vel = 1;
				pTrain->Use(this, this, USE_SET, vel);
			}
			else if (m_afButtonPressed & IN_BACK)
			{
				vel = -1;
				pTrain->Use(this, this, USE_SET, vel);
			}
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if (m_iTrain & TRAIN_ACTIVE)
		m_iTrain = TRAIN_NEW;

	if (pev->button & IN_JUMP)
		Jump();

	if ((pev->button & IN_DUCK) || FBitSet(pev->flags, FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING))
		Duck();

	if (!FBitSet(pev->flags, FL_ONGROUND))
		m_flFallVelocity = -pev->velocity.z;

	m_hEnemy = NULL;

	if (m_afPhysicsFlags & PFLAG_ONBARNACLE)
		pev->velocity = g_vecZero;

	if (!(m_flDisplayHistory & DHF_ROUND_STARTED) && CanPlayerBuy(false) == true)
	{
		HintMessage("#Hint_press_buy_to_purchase");
		m_flDisplayHistory |= DHF_ROUND_STARTED;
	}

	UpdateLocation(false);
}

void CBasePlayer::CheckTimeBasedDamage(void)
{
	BYTE bDuration = 0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
		return;

	if (fabs(gpGlobals->time - m_tbdPrev) < 2)
		return;

	m_tbdPrev = gpGlobals->time;

	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
				case itbd_Paralyze: bDuration = PARALYZE_DURATION; break;
				case itbd_NerveGas: bDuration = NERVEGAS_DURATION; break;

				case itbd_Poison:
				{
					TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
					bDuration = POISON_DURATION;
					break;
				}

				case itbd_DrownRecover:
				{
					if (m_idrowndmg > m_idrownrestored)
					{
						int idif = min(m_idrowndmg - m_idrownrestored, 10);
						TakeHealth(idif, DMG_GENERIC);
						m_idrownrestored += idif;
					}

					bDuration = 4;
					break;
				}

				case itbd_Radiation: bDuration = RADIATION_DURATION; break;
				case itbd_Acid: bDuration = ACID_DURATION; break;
				case itbd_SlowBurn: bDuration = SLOWBURN_DURATION; break;
				case itbd_SlowFreeze: bDuration = SLOWFREEZE_DURATION; break;
				default: bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				if ((i == itbd_NerveGas && m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION) || (i == itbd_Poison && m_rgbTimeBasedDamage[i] < POISON_DURATION))
				{
					if (m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}

				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

#define GEIGERDELAY 0.25

void CBasePlayer::UpdateGeigerCounter(void)
{
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;
	byte range = (byte)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;
		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, NULL, pev);
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

#define SUITUPDATETIME 3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate(void)
{
	int isentence = 0;
	int isearch = m_iSuitPlayNext;

	if (!(pev->weapons & (1 << WEAPON_SUIT)))
		return;

	UpdateGeigerCounter();

	if (g_pGameRules->IsMultiplayer())
		return;

	if (gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0)
	{
		for (int i = 0; i < CSUITPLAYLIST; i++)
		{
			if (isentence = m_rgSuitPlayList[isearch])
				break;

			if (++isearch == CSUITPLAYLIST)
				isearch = 0;
		}

		if (isentence)
		{
			m_rgSuitPlayList[isearch] = 0;

			if (isentence > 0)
			{
				char sentence[CBSENTENCENAME_MAX + 1];
				strcpy(sentence, "!");
				strcat(sentence, gszallsentencenames[isentence]);
				EMIT_SOUND_SUIT(ENT(pev), sentence);
			}
			else
				EMIT_GROUPID_SUIT(ENT(pev), -isentence);

			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME;
		}
		else
			m_flSuitUpdate = 0;
	}
}

void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime)
{

}

void CBasePlayer::SetNewPlayerModel(const char *model)
{
	SET_MODEL(edict(), model);
	m_modelIndexPlayer = pev->modelindex;
}

void CBasePlayer::CheckPowerups(entvars_t *pev)
{
	if (pev->health > 0)
		pev->modelindex = m_modelIndexPlayer;
}

void CBasePlayer::UpdatePlayerSound(void)
{
	int iBodyVolume;
	CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));

	if (!pSound)
	{
		ALERT(at_console, "Client lost reserved sound!\n");
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		iBodyVolume = pev->velocity.Length();

		if (iBodyVolume> 512)
			iBodyVolume = 512;
	}
	else
		iBodyVolume = 0;

	if (pev->button & IN_JUMP)
		iBodyVolume += 100;

	if (m_iWeaponVolume > iBodyVolume)
	{
		m_iTargetVolume = m_iWeaponVolume;
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
		m_iTargetVolume = iBodyVolume;

	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	int iVolume = pSound->m_iVolume;

	if (m_iTargetVolume > iVolume)
	{
		iVolume = m_iTargetVolume;
	}
	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
			iVolume = 0;
	}

	if (m_fNoPlayerSound)
		iVolume = 0;

	if (gpGlobals->time > m_flStopExtraSoundTime)
		m_iExtraSoundTypes = 0;

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iVolume = iVolume;
		pSound->m_iType |= (bits_SOUND_PLAYER | m_iExtraSoundTypes);
	}

	m_iWeaponFlash -= 256 * gpGlobals->frametime;

	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	UTIL_MakeVectors(pev->angles);
	gpGlobals->v_forward.z = 0;
}

void CBasePlayer::PostThink(void)
{
	if (g_fGameOver)
		goto pt_end;

	if (!IsAlive())
		goto pt_end;

	if (m_pTank)
	{
		if (!m_pTank->OnControls(pev) || pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
		else
			m_pTank->Use(this, this, USE_SET, 2);
	}

	ItemPostFrame();

	if ((pev->flags & FL_ONGROUND) && pev->health > 0 && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		if (pev->watertype != CONTENT_WATER)
		{
			if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
			{
				float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

				if (flFallDamage > pev->health)
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", VOL_NORM, ATTN_NORM);

				if (flFallDamage > 0)
				{
					TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL);
					pev->punchangle.x = 0;
				}
			}
		}

		if (IsAlive())
			SetAnimation(PLAYER_WALK);
	}

	if (pev->flags & FL_ONGROUND)
	{
		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
			CSoundEnt::InsertSound(bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2);

		m_flFallVelocity = 0;
	}

	if (IsAlive())
	{
		if (pev->velocity.x || pev->velocity.y)
		{
			if (/*(pev->velocity.x || pev->velocity.y) &&*/ FBitSet(pev->flags, FL_ONGROUND))//(pev->velocity.x || pev->velocity.y) проверены строкой выше...
				SetAnimation(PLAYER_WALK);
			else if (pev->waterlevel > 1)
				SetAnimation(PLAYER_WALK);
		}
		else if (pev->gaitsequence != ACT_FLY)
			SetAnimation(PLAYER_IDLE);
	}

	StudioFrameAdvance();
	CheckPowerups(pev);
	UpdatePlayerSound();

pt_end:
#ifdef CLIENT_WEAPONS
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (!m_rgpPlayerItems[i])
			continue;

		CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[i];

		while (pPlayerItem)
		{
			CBasePlayerWeapon *gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();

			if (gun && gun->UseDecrement())
			{
				gun->m_flNextPrimaryAttack = max(gun->m_flNextPrimaryAttack - gpGlobals->frametime, -1.0);
				gun->m_flNextSecondaryAttack = max(gun->m_flNextSecondaryAttack - gpGlobals->frametime, -0.001);

				if (gun->m_flTimeWeaponIdle != 1000)
					gun->m_flTimeWeaponIdle = max(gun->m_flTimeWeaponIdle - gpGlobals->frametime, -0.001);
			}

			pPlayerItem = pPlayerItem->m_pNext;
		}
	}

	m_flNextAttack -= gpGlobals->frametime;

	if (m_flNextAttack < -0.001)
		m_flNextAttack = -0.001;
#endif
	m_afButtonLast = pev->button;
	m_iGaitsequence = pev->gaitsequence;
	StudioProcessGait();
}

BOOL IsSpawnPointValid(CBaseEntity *pPlayer, CBaseEntity *pSpot)
{
	CBaseEntity *ent = NULL;

	if (!pSpot->IsTriggered(pPlayer))
		return FALSE;

	while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 64)) != NULL)
	{
		if (ent->IsPlayer() && ent != pPlayer)
			return FALSE;
	}

	return TRUE;
}

DLL_GLOBAL CBaseEntity *g_pLastSpawn;
DLL_GLOBAL CBaseEntity *g_pLastCTSpawn, *g_pLastTerroristSpawn;

inline int FNullEnt(CBaseEntity *ent) { return (!ent) || FNullEnt(ent->edict()); }

bool SelectSpawnSpot(CBaseEntity *pPlayer, const char *pEntClassName, CBaseEntity *&pSpot)
{
	edict_t *player = pPlayer->edict();
	pSpot = UTIL_FindEntityByClassname(pSpot, pEntClassName);

	if (FNullEnt(pSpot))
		pSpot = UTIL_FindEntityByClassname(pSpot, pEntClassName);

	CBaseEntity *pFirstSpot = pSpot;

	do
	{
		if (pSpot)
		{
			if (IsSpawnPointValid(pPlayer, pSpot))
			{
				if (pSpot->pev->origin == Vector(0, 0, 0))
				{
					pSpot = UTIL_FindEntityByClassname(pSpot, pEntClassName);
					continue;
				}

				return true;
			}
		}

		pSpot = UTIL_FindEntityByClassname(pSpot, pEntClassName);
	}
	while (pSpot != pFirstSpot);

	if (!FNullEnt(pSpot))
	{
		CBaseEntity *ent = UTIL_FindEntityInSphere(NULL, pSpot->pev->origin, 64);

		while (ent)
		{
			if (ent->IsPlayer() && ent->edict() != player)
				ent->TakeDamage(VARS(INDEXENT(0)), VARS(INDEXENT(0)), 200, DMG_GENERIC);

			ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 64);
		}

		return true;
	}

	return false;
}

edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer)
{
	CBaseEntity *pSpot;
	edict_t *player = pPlayer->edict();

	if (g_pGameRules->IsCoOp())
	{
		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_coop");

		if (!FNullEnt(pSpot))
			goto ReturnSpot;

		pSpot = UTIL_FindEntityByClassname(g_pLastSpawn, "info_player_start");

		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else if (g_pGameRules->IsDeathmatch() && ((CBasePlayer *)pPlayer)->m_bIsVIP == true)
	{
		pSpot = UTIL_FindEntityByClassname(NULL, "info_vip_start");

		if (!FNullEnt(pSpot))
			goto ReturnSpot;

		goto CTSpawn;
	}
	else if (g_pGameRules->IsDeathmatch() && ((CBasePlayer *)pPlayer)->m_iTeam == TEAM_CT)
	{
CTSpawn:
		pSpot = g_pLastCTSpawn;

		if (SelectSpawnSpot(pPlayer, "info_player_start", pSpot))
			goto ReturnSpot;
	}
	else if (g_pGameRules->IsDeathmatch() && ((CBasePlayer *)pPlayer)->m_iTeam == TEAM_TERRORIST)
	{
		pSpot = g_pLastTerroristSpawn;

		if (SelectSpawnSpot(pPlayer, "info_player_deathmatch", pSpot))
			goto ReturnSpot;
	}

	if (FStringNull(gpGlobals->startspot) || !strlen(STRING(gpGlobals->startspot)))
	{
		pSpot = UTIL_FindEntityByClassname(NULL, "info_player_deathmatch");

		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname(NULL, STRING(gpGlobals->startspot));

		if (!FNullEnt(pSpot))
			goto ReturnSpot;
	}

ReturnSpot:
	if (FNullEnt(pSpot))
	{
		ALERT(at_error, "PutClientInServer: no info_player_start on level");
		return INDEXENT(0);
	}

	if (((CBasePlayer *)pPlayer)->m_iTeam == TEAM_TERRORIST)
		g_pLastTerroristSpawn = pSpot;
	else
		g_pLastCTSpawn = pSpot;

	return pSpot->edict();
}

void CBasePlayer::ResetStamina(void)
{
	pev->fuser1 = 0;
	pev->fuser2 = 0;
	pev->fuser3 = 0;
}

extern BOOL g_skipCareerInitialSpawn;

void CBasePlayer::Spawn(void)
{
	int i;

	m_iGaitsequence = 0;
	m_flGaitframe = 0;
	m_flGaityaw = 0;
	m_prevgaitorigin = Vector(0, 0, 0);
	m_flGaitMovement = 0;
	m_progressStart = 0;
	m_progressEnd = 0;

	if (pev->classname)
		RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->classname = MAKE_STRING("player");
	AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->health = 100;

	if (!m_bNotKilled)
	{
		pev->armorvalue = 0;
		m_iKevlar = 0;
	}

	pev->maxspeed = 1000;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = pev->health;
	pev->flags &= FL_PROXY;
	pev->flags |= FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2;
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;

	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	m_fLongJump = FALSE;
	m_iClientFOV = -1;
	m_pentCurBombTarget = NULL;

	if (m_bOwnsShield)
		pev->gamestate = 0;
	else
		pev->gamestate = 1;

	ResetStamina();
	pev->friction = 1;
	pev->gravity = 1;

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");
	m_hintMessageQueue.Reset();

	m_flVelocityModifier = 1;
	m_iLastZoom = 90;
	m_flLastTalk = 0;
	m_flIdleCheckTime = 0;
	m_flRadioTime = 0;
	m_iRadioMessages = 60;
	m_bHasC4 = false;
	m_bKilledByBomb = false;
	m_bKilledByGrenade = false;
	m_flDisplayHistory &= ~DHM_ROUND_CLEAR;
	m_tmHandleSignals = 0;
	m_fCamSwitch = FALSE;
	m_iChaseTarget = 1;
	m_bEscaped = false;
	m_tmNextRadarUpdate = gpGlobals->time;
	m_vLastOrigin = Vector(0, 0, 0);
	m_iCurrentKickVote = 0;
	m_flNextVoteTime = 0;
	m_bJustKilledTeammate = false;

	SET_VIEW(ENT(pev), ENT(pev));

	m_hObserverTarget = NULL;
	pev->iuser1 = pev->iuser2 = pev->iuser3 = 0;
	m_flLastFired = -15;
	m_bHeadshotKilled = false;
	m_bReceivesNoMoneyNextRound = false;
	m_bShieldDrawn = false;

	m_blindUntilTime = 0;
	m_blindStartTime = 0;
	m_blindHoldTime = 0;
	m_blindFadeTime = 0;
	m_blindAlpha = 0;

	m_canSwitchObserverModes = true;
	m_lastLocation[0] = 0;
	m_bitsDamageType &= ~(DMG_DROWN | DMG_DROWNRECOVER);
	m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
	m_idrowndmg = 0;
	m_idrownrestored = 0;

	if (m_iObserverC4State)
	{
		m_iObserverC4State = 0;
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("c4");
		MESSAGE_END();
	}

	if (m_bObserverHasDefuser)
	{
		m_bObserverHasDefuser = false;
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("defuser");
		MESSAGE_END();
	}

	if (g_pGameRules->IsFreezePeriod())
		m_bCanShoot = false;
	else
		m_bCanShoot = true;

	m_iNumSpawns++;
	InitStatusBar();

	for (i = 0; i < 20; i++)
		m_vRecentPath[i] = Vector(0, 0, 0);

	if (m_pActiveItem && !pev->viewmodel)
	{
		switch (m_pActiveItem->m_iId)
		{
			case WEAPON_AWP: pev->viewmodel = MAKE_STRING("models/v_awp.mdl"); break;
			case WEAPON_G3SG1: pev->viewmodel = MAKE_STRING("models/v_g3sg1.mdl"); break;
			case WEAPON_SCOUT: pev->viewmodel = MAKE_STRING("models/v_scout.mdl"); break;
			case WEAPON_SG550: pev->viewmodel = MAKE_STRING("models/v_sg550.mdl"); break;
		}
	}

	m_iFOV = 90;
	m_flNextDecalTime = 0;
	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5;
	m_bloodColor = BLOOD_COLOR_RED;
	m_flgeigerDelay = gpGlobals->time + 2;
	m_flNextAttack = UTIL_WeaponTimeBase();

	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1;

	if (m_bHasDefuser == true)
		pev->body = 1;
	else
		pev->body = 0;

	if (m_bMissionBriefing == true)
	{
		RemoveLevelText();
		m_bMissionBriefing = false;
	}

	m_flFallVelocity = 0;

	if (!g_skipCareerInitialSpawn)
		g_pGameRules->GetPlayerSpawnSpot(this);

	if (!pev->modelindex)
	{
		SET_MODEL(ENT(pev), "models/player.mdl");
		m_modelIndexPlayer = pev->modelindex;
	}

	pev->sequence = LookupActivity(ACT_IDLE);

	if (pev->flags & FL_DUCKING)
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector(0, 32, 0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
		ALERT(at_console, "Couldn't alloc player sound slot!\n");

	m_iHideHUD &= ~(HIDEHUD_WEAPONS | HIDEHUD_HEALTH | HIDEHUD_TIMER | HIDEHUD_MONEY);
	m_fNoPlayerSound = FALSE;
	m_pLastItem = NULL;
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;
	m_fInitHUD = TRUE;

	if (!m_bNotKilled)
	{
		m_iClientHideHUD = -1;

		for (i = 0; i < MAX_AMMO_SLOTS; i++)
			m_rgAmmo[i] = 0;

		m_bHasPrimary = false;
		m_bHasNightVision = false;
		SendItemStatus(this);
	}
	else
	{
		for (i = 0; i < MAX_AMMO_SLOTS; i++)
			m_rgAmmoLast[i] = -1;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	m_bNightVisionOn = false;

	for (i = 1; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer *pOther = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (pOther && pOther->IsObservingPlayer(this))
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgNVGToggle, NULL, pOther->pev);
			WRITE_BYTE(0);
			MESSAGE_END();

			pOther->m_bNightVisionOn = false;
		}
	}

	m_lastx = m_lasty = 0;
	g_pGameRules->PlayerSpawn(this);
	m_bNotKilled = true;
	m_bIsDefusing = false;

	ResetMaxSpeed();
	UTIL_SetOrigin(pev, pev->origin);

	if (m_bIsVIP == true)
	{
		m_iKevlar = 2;
		pev->armorvalue = 200;
		HintMessage("#Hint_you_are_the_vip");
	}

	SetScoreboardAttributes();

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_STRING(GetTeam(m_iTeam));
	MESSAGE_END();

	UpdateLocation(true);

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_SHORT(pev->frags);
	WRITE_SHORT(m_iDeaths);
	WRITE_SHORT(0);
	WRITE_SHORT(m_iTeam);
	MESSAGE_END();

	if (m_bHasChangedName)
	{
		char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(ENT(pev));
		char *curname = g_engfuncs.pfnInfoKeyValue(infobuffer, "name");

		if (strcmp(m_szNewName, curname))
			g_engfuncs.pfnSetClientKeyValue(entindex(), infobuffer, "name", m_szNewName);

		m_bHasChangedName = false;
		m_szNewName[0] = '\0';
	}

	UTIL_ScreenFade(this, Vector(0, 0, 0), 0.001, 0, 0, 0);
	SyncRoundTimer();

	m_allowAutoFollowTime = false;

	for (i = 0; i < 8; i++)
		m_flLastCommandTime[i] = -1;

	sv_aim = CVAR_GET_POINTER("sv_aim");
}

void CBasePlayer::Precache(void)
{
	if (WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet)
	{
		if (!WorldGraph.FSetGraphPointers())
			ALERT(at_console, "**Graph pointers were not set!\n");
		else
			ALERT(at_console, "**Graph Pointers Set!\n");
	}

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;
	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;
	m_iClientBattery = -1;
	m_iTrain = TRAIN_NEW;

	LinkUserMessages();
	m_iUpdateTime = 5;

	if (gInitHUD)
		m_fInitHUD = TRUE;
}

int CBasePlayer::Save(CSave &save)
{
	if (!CBaseMonster::Save(save))
		return 0;

	return save.WriteFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));
}

void CBasePlayer::SetScoreAttrib(CBasePlayer *dest)
{
   int state = 0;
   if (pev->deadflag != DEAD_NO)
      state |= SCORE_STATUS_DEAD;

   if (m_bHasC4)
      state |= SCORE_STATUS_BOMB;

   if (m_bIsVIP)
      state |= SCORE_STATUS_VIP;

   if (gmsgScoreAttrib)
   {
      MESSAGE_BEGIN(MSG_ONE, gmsgScoreAttrib, NULL, ENT(dest->pev));
      WRITE_BYTE(entindex());
      WRITE_BYTE(state);
      MESSAGE_END();
   }
}

void CBasePlayer::SetScoreboardAttributes(CBasePlayer *pPlayer)
{
   if (pPlayer != NULL)
   {
      SetScoreAttrib(pPlayer);
      return;
   }

   for (int i = 1; i <= gpGlobals->maxClients; i++)
   {
      CBasePlayer *player = (CBasePlayer *)UTIL_PlayerByIndex(i);

      if (player && !FNullEnt(player->edict()))
         SetScoreboardAttributes(player);
   }
}

void CBasePlayer::RenewItems(void)
{

}

int CBasePlayer::Restore(CRestore &restore)
{
	if (!CBaseMonster::Restore(restore))
		return 0;

	int status = restore.ReadFields("PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData));
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if (!pSaveData->fUseLandmark)
	{
		ALERT(at_console, "No Landmark:%s\n", pSaveData->szLandmarkName);

		edict_t *pentSpawnSpot = EntSelectSpawnPoint(this);
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0, 0, 1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}

	pev->angles.z = 0;
	pev->angles = pev->v_angle;
	pev->fixangle = TRUE;
	m_bloodColor = BLOOD_COLOR_RED;
	m_modelIndexPlayer = pev->modelindex;

	if (pev->flags & FL_DUCKING)
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

	m_flDisplayHistory &= ~DHM_CONNECT_CLEAR;
	SetScoreboardAttributes();
	return status;
}

void CBasePlayer::Restart(void)
{
	pev->frags = 0;
	m_iDeaths = 0;
	m_iAccount = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgMoney, NULL, pev);
	WRITE_LONG(m_iAccount);
	WRITE_BYTE(0);
	MESSAGE_END();

	m_bNotKilled = false;

	RemoveShield();
	CheckStartMoney();
	AddAccount(startmoney.value);

	MESSAGE_BEGIN(MSG_ALL, gmsgScoreInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(0);
	WRITE_SHORT(m_iTeam);
	MESSAGE_END();
}

void CBasePlayer::SelectNextItem(int iItem)
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[iItem];

	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		pItem = m_pActiveItem->m_pNext;

		if (!pItem)
			return;

		CBasePlayerItem *pLast = pItem;

		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[iItem] = pItem;
	}

	ResetAutoaim();

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	if (HasShield() != false)
	{
		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;
		pWeapon->m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
		m_bShieldDrawn = false;
	}

	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
      UpdateShieldCrosshair(true);
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();

		ResetMaxSpeed();
	}
}

void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem *pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];

			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
					break;

				pItem = pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
		return;

	ResetAutoaim();

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_bShieldDrawn = false;
      UpdateShieldCrosshair(true);
		m_pActiveItem->Deploy();
		m_pActiveItem->UpdateItemInfo();

		ResetMaxSpeed();
	}
}

void CBasePlayer::SelectLastItem(void)
{
   if (m_pActiveItem && !m_pActiveItem->CanHolster())
      return;

   if (!m_pLastItem || m_pLastItem == m_pActiveItem)
   {
      for (int i = 1; i < MAX_ITEMS; i++)
      {
         CBasePlayerItem *pItem = m_rgpPlayerItems[i];
         if (pItem && pItem != m_pActiveItem)
         {
            m_pLastItem = pItem;
            break;
         }
      }
   }

   if (!m_pLastItem || m_pLastItem == m_pActiveItem)
      return;

   ResetAutoaim();

   if (m_pActiveItem)
      m_pActiveItem->Holster();

   if (HasShield())
   {
      CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

      if (m_pActiveItem)
         pWeapon->m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;

      m_bShieldDrawn = false;
   }

   CBasePlayerItem *pTemp = m_pActiveItem;

   m_pActiveItem = m_pLastItem;
   m_pLastItem = pTemp;

   m_pActiveItem->Deploy();
   m_pActiveItem->UpdateItemInfo();

   UpdateShieldCrosshair(true);

   ResetMaxSpeed();
}

BOOL CBasePlayer::HasWeapons(void)
{
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
			return TRUE;
	}

	return FALSE;
}

void CBasePlayer::SelectPrevItem(int iItem)
{

}

const char *CBasePlayer::TeamID(void)
{
	if (!pev)
		return "";

	return m_szTeamName;
}

class CSprayCan : public CBaseEntity
{
public:
	void Spawn(entvars_t *pevOwner);
	void Think(void);
	int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn(entvars_t *pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", VOL_NORM, ATTN_NORM);
}

void CSprayCan::Think(void)
{
	TraceResult tr;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pev->owner);
	int nFrames;

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	int playernum = ENTINDEX(pev->owner);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, TRUE);

		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn(entvars_t *pevOwner);
	void Spray(void);
};

void CBloodSplat::Spawn(entvars_t *pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink(&CBloodSplat::Spray);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray(void)
{
	if (g_Language != LANGUAGE_GERMAN)
	{
		TraceResult tr;
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);
		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayer::GiveNamedItem(const char *pszName)
{
	edict_t *pent = CREATE_NAMED_ENTITY(MAKE_STRING(pszName));

	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in GiveNamedItem!");
		return;
	}

	pent->v.origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	DispatchSpawn(pent);
	DispatchTouch(pent, ENT(pev));
}

CBaseEntity *FindEntityForward(CBaseEntity *pEntity)
{
	UTIL_MakeVectors(pEntity->pev->v_angle);

	TraceResult tr;
	Vector vecSrc = pEntity->pev->origin + pEntity->pev->view_ofs;
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 8192;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, pEntity->edict(), &tr);

	if (tr.flFraction != 1 && !FNullEnt(tr.pHit))
		return CBaseEntity::Instance(tr.pHit);

	return NULL;
}

BOOL CBasePlayer::FlashlightIsOn(void)
{
	return pev->effects & EF_DIMLIGHT;
}

void CBasePlayer::FlashlightTurnOn(void)
{
	if (!g_pGameRules->FAllowFlashlight())
		return;

	if (pev->weapons & (1 << WEAPON_SUIT))
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, VOL_NORM, ATTN_NORM);

		pev->effects |= EF_DIMLIGHT;
		MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, NULL, pev);
		WRITE_BYTE(1);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = gpGlobals->time + FLASH_DRAIN_TIME;
	}
}

void CBasePlayer::FlashlightTurnOff(void)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, VOL_NORM, ATTN_NORM);

	pev->effects &= ~EF_DIMLIGHT;
	MESSAGE_BEGIN(MSG_ONE, gmsgFlashlight, NULL, pev);
	WRITE_BYTE(0);
	WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = gpGlobals->time + FLASH_CHARGE_TIME;
}

void CBasePlayer::ForceClientDllUpdate(void)
{
	m_fInitHUD = TRUE;
	m_iClientHealth = -1;
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;
	m_fWeapon = FALSE;

	UpdateClientData();
	HandleSignals();
}

void CBasePlayer::HandleSignals(void)
{
	if (g_pGameRules->IsMultiplayer())
	{
		if (!g_pGameRules->m_bMapHasBuyZone)
		{
			if (m_iTeam == TEAM_TERRORIST || m_iTeam == TEAM_CT)
			{
				CBaseEntity *pEntity = NULL;
				char *classname = (m_iTeam == TEAM_TERRORIST) ? "info_player_deathmatch" : "info_player_start";

				while ((pEntity = UTIL_FindEntityByClassname(pEntity, classname)) != NULL)
				{
					if ((pEntity->pev->origin - pev->origin).Length() < 200)
						m_signals.Signal(SIGNAL_BUY);
				}
			}
		}

		if (!g_pGameRules->m_bMapHasBombZone)
		{
			CBaseEntity *pEntity = NULL;

			while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_bomb_target")) != NULL)
			{
				if ((pEntity->pev->origin - pev->origin).Length() <= 256)
					m_signals.Signal(SIGNAL_BOMB);
			}
		}

		if (!g_pGameRules->m_bMapHasRescueZone)
		{
			CBaseEntity *pEntity = NULL;

			while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_hostage_rescue")) != NULL)
			{
				if ((pEntity->pev->origin - pev->origin).Length() <= 256)
					m_signals.Signal(SIGNAL_RESCUE);
			}
		}
	}

	int signalSave = m_signals.GetSignal();
	int signalChanged = m_signals.GetState() ^ m_signals.GetSignal();
	m_signals.Update();

	if (signalChanged & SIGNAL_BUY)
	{
		if (signalSave & SIGNAL_BUY)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
			WRITE_BYTE(STATUSICON_SHOW);
			WRITE_STRING("buyzone");
			WRITE_BYTE(0);
			WRITE_BYTE(160);
			WRITE_BYTE(0);
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
			WRITE_BYTE(STATUSICON_HIDE);
			WRITE_STRING("buyzone");
			MESSAGE_END();

			if (m_iMenu >= Menu_Buy)
			{
				if (m_iMenu <= Menu_BuyItem)
				{
					CLIENT_COMMAND(ENT(pev), "slot10\n");
				}
				else if (m_iMenu == Menu_ClientBuy)
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
					MESSAGE_END();
				}
			}
		}
	}

	if (signalChanged & SIGNAL_BOMB)
	{
		if (signalSave & SIGNAL_BOMB)
		{
			if (m_bHasC4 && !(m_flDisplayHistory & DHF_IN_TARGET_ZONE))
			{
				m_flDisplayHistory |= DHF_IN_TARGET_ZONE;
				HintMessage("#Hint_you_are_in_targetzone");
			}

			SetBombIcon(TRUE);
		}
		else
			SetBombIcon(FALSE);
	}

	if (signalChanged & SIGNAL_RESCUE)
	{
		if (signalSave & SIGNAL_RESCUE)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
			WRITE_BYTE(STATUSICON_SHOW);
			WRITE_STRING("rescue");
			WRITE_BYTE(0);
			WRITE_BYTE(160);
			WRITE_BYTE(0);
			MESSAGE_END();

			if (m_iTeam == TEAM_CT && !(m_flDisplayHistory & DHF_IN_RESCUE_ZONE))
			{
				m_flDisplayHistory |= DHF_IN_RESCUE_ZONE;
				HintMessage("#Hint_hostage_rescue_zone");
			}
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
			WRITE_BYTE(STATUSICON_HIDE);
			WRITE_STRING("rescue");
			MESSAGE_END();

			if (m_iMenu >= Menu_Buy)
			{
				if (m_iMenu <= Menu_BuyItem)
				{
					CLIENT_COMMAND(ENT(pev), "slot10\n");
				}
				else if (m_iMenu == Menu_ClientBuy)
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
					MESSAGE_END();
				}
			}
		}
	}

	if (signalChanged & SIGNAL_ESCAPE)
	{
		if (signalSave & SIGNAL_ESCAPE)
			EnterEscapeZone();
		else
			LeaveEscapeZone();
	}

	if (signalChanged & SIGNAL_VIPSAFETY)
	{
		if (signalSave & SIGNAL_VIPSAFETY)
			EnterVIPSafetyZone();
		else
			LeaveVIPSafetyZone();
	}
}

void CBasePlayer::EnterEscapeZone(void)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
	WRITE_BYTE(STATUSICON_SHOW);
	WRITE_STRING("escape");
	WRITE_BYTE(0);
	WRITE_BYTE(160);
	WRITE_BYTE(0);
	MESSAGE_END();

	if (m_iTeam == TEAM_CT && !(m_flDisplayHistory & DHF_IN_ESCAPE_ZONE))
	{
		m_flDisplayHistory |= DHF_IN_ESCAPE_ZONE;
		HintMessage("#Hint_terrorist_escape_zone", TRUE);
	}
}

void CBasePlayer::LeaveEscapeZone(void)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
	WRITE_BYTE(STATUSICON_HIDE);
	WRITE_STRING("escape");
	MESSAGE_END();

	if (m_iMenu >= Menu_Buy)
	{
		if (m_iMenu <= Menu_BuyItem)
		{
			CLIENT_COMMAND(ENT(pev), "slot10\n");
		}
		else if (m_iMenu == Menu_ClientBuy)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
			MESSAGE_END();
		}
	}
}

void CBasePlayer::EnterVIPSafetyZone(void)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
	WRITE_BYTE(STATUSICON_SHOW);
	WRITE_STRING("vipsafety");
	WRITE_BYTE(0);
	WRITE_BYTE(160);
	WRITE_BYTE(0);
	MESSAGE_END();

	if (m_iTeam == TEAM_CT && !(m_flDisplayHistory & DHF_IN_VIPSAFETY_ZONE))
	{
		m_flDisplayHistory |= DHF_IN_VIPSAFETY_ZONE;
		HintMessage("#Hint_ct_vip_zone");
	}
	else if (m_iTeam == TEAM_TERRORIST && !(m_flDisplayHistory & DHF_IN_VIPSAFETY_ZONE))
	{
		m_flDisplayHistory |= DHF_IN_VIPSAFETY_ZONE;
		HintMessage("#Hint_terrorist_vip_zone");
	}
}

void CBasePlayer::LeaveVIPSafetyZone(void)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
	WRITE_BYTE(STATUSICON_HIDE);
	WRITE_STRING("vipsafety");
	MESSAGE_END();

	if (m_iMenu >= Menu_Buy)
	{
		if (m_iMenu <= Menu_BuyItem)
		{
			CLIENT_COMMAND(ENT(pev), "slot10\n");
		}
		else if (m_iMenu == Menu_ClientBuy)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgBuyClose, NULL, pev);
			MESSAGE_END();
		}
	}
}

extern float g_flWeaponCheat;

void CBasePlayer::ImpulseCommands(void)
{
	PlayerUse();

	TraceResult tr;
	int iImpulse = pev->impulse;

	switch (iImpulse)
	{
		case 99:
		{
			int iOn;

			if (!gmsgLogo)
			{
				iOn = 1;
				gmsgLogo = REG_USER_MSG("Logo", 1);
			}
			else
				iOn = 0;

			ASSERT(gmsgLogo > 0);

			MESSAGE_BEGIN(MSG_ONE, gmsgLogo, NULL, pev);
			WRITE_BYTE(iOn);
			MESSAGE_END();

			if (!iOn)
				gmsgLogo = 0;

			break;
		}

		case 100:
		{
			if (FlashlightIsOn())
				FlashlightTurnOff();
			else
				FlashlightTurnOn();

			break;
		}

		case 201:
		{
			if (gpGlobals->time < m_flNextDecalTime)
				break;

			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, edict(), &tr);

			if (tr.flFraction != 1)
			{
				m_flNextDecalTime = gpGlobals->time + CVAR_GET_FLOAT("decalfrequency");
				CSprayCan *pCan = GetClassPtr((CSprayCan *)NULL);
				pCan->Spawn(pev);
			}

			break;
		}

		default: CheatImpulseCommands(iImpulse);
	}

	pev->impulse = 0;
}

void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
	TraceResult tr;

	if (!g_flWeaponCheat)
		return;

	switch (iImpulse)
	{
		case 204:
		{
			UTIL_BloodDrips(pev->origin, Vector(0, 0, 1), BLOOD_COLOR_RED, 2000);
			break;
		}

		case 76:
		{
			if (giPrecacheGrunt)
			{
				UTIL_MakeVectors(Vector(0, pev->v_angle.y, 0));
				Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
			}
			else
			{
				giPrecacheGrunt = 1;
				ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
			}

			break;
		}

		case 101:
		{
			gEvilImpulse101 = TRUE;
			AddAccount(STARTMONEY_MAX);
			ALERT(at_console, "Crediting %s with %i\n", STRING(pev->netname),STARTMONEY_MAX);
			break;
		}

		case 102: CGib::SpawnRandomGibs(pev, 1, 1); break;

		case 103:
		{
			CBaseEntity *pEntity = FindEntityForward(this);

			if (pEntity)
			{
				CBaseMonster *pMonster = pEntity->MyMonsterPointer();

				if (pMonster)
					pMonster->ReportAIState();
			}

			break;
		}

		case 104: gGlobalState.DumpGlobals(); break;

		case 105:
		{
			if (m_fNoPlayerSound)
			{
				ALERT(at_console, "Player is audible\n");
				m_fNoPlayerSound = FALSE;
			}
			else
			{
				ALERT(at_console, "Player is silent\n");
				m_fNoPlayerSound = TRUE;
			}

			break;
		}

		case 106:
		{
			CBaseEntity *pEntity = FindEntityForward(this);

			if (pEntity)
			{
				ALERT(at_console, "Classname: %s", STRING(pEntity->pev->classname));

				if (!FStringNull(pEntity->pev->targetname))
					ALERT(at_console, " - Targetname: %s\n", STRING(pEntity->pev->targetname));
				else
					ALERT(at_console, " - TargetName: No Targetname\n");

				ALERT(at_console, "Model: %s\n", STRING(pEntity->pev->model));

				if (pEntity->pev->globalname)
					ALERT(at_console, "Globalname: %s\n", STRING(pEntity->pev->globalname));
			}

			break;
		}

		case 107:
		{
			edict_t *pWorld = g_engfuncs.pfnPEntityOfEntIndex(0);
			Vector start = pev->origin + pev->view_ofs;
			Vector end = start + gpGlobals->v_forward * 1024;
			UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);

			if (tr.pHit)
				pWorld = tr.pHit;

			const char *pszTextureName = TRACE_TEXTURE(pWorld, start, end);

			if (pszTextureName)
				ALERT(at_console, "Texture: %s\n", pszTextureName);

			break;
		}

		case 195: Create("node_viewer_fly", pev->origin, pev->angles); break;
		case 196: Create("node_viewer_large", pev->origin, pev->angles); break;
		case 197: Create("node_viewer_human", pev->origin, pev->angles); break;

		case 199:
		{
			ALERT(at_console, "%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
			WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
			break;
		}

		case 202:
		{
			UTIL_MakeVectors(pev->v_angle);
			UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, edict(), &tr);

			if (tr.flFraction != 1)
			{
				CBloodSplat *pBlood = GetClassPtr((CBloodSplat *)NULL);
				pBlood->Spawn(pev);
			}

			break;
		}

		case 203:
		{
			CBaseEntity *pEntity = FindEntityForward(this);

			if (pEntity && pEntity->pev->takedamage != DAMAGE_NO)
				pEntity->SetThink(&CBaseEntity::SUB_Remove);

			break;
		}
	}
}

int CBasePlayer::AddPlayerItem(CBasePlayerItem *pItem)
{
	CBasePlayerItem *pInsert = m_rgpPlayerItems[pItem->iItemSlot()];

	while (pInsert)
	{
		if (FClassnameIs(pInsert->pev, STRING(pItem->pev->classname)))
		{
			if (pItem->AddDuplicate(pInsert))
			{
				g_pGameRules->PlayerGotWeapon(this, pItem);
				pItem->CheckRespawn();
				pItem->UpdateItemInfo();

				if (m_pActiveItem)
					m_pActiveItem->UpdateItemInfo();

				pItem->Kill();
			}
			else if (gEvilImpulse101)
				pItem->Kill();

			return FALSE;
		}

		pInsert = pInsert->m_pNext;
	}

	if (pItem->AddToPlayer(this))
	{
		g_pGameRules->PlayerGotWeapon(this, pItem);

		if (pItem->iItemSlot() == WPNSLOT_PRIMARY)
			m_bHasPrimary = true;

		pItem->CheckRespawn();
		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot()];
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem;

		if (HasShield() != false)
			pev->gamestate = 0;

		if (g_pGameRules->FShouldSwitchWeapon(this, pItem))
		{
			if (m_bShieldDrawn == false)
				SwitchWeapon(pItem);
		}

		return TRUE;
	}
	else if (gEvilImpulse101)
		pItem->Kill();

	return FALSE;
}

int CBasePlayer::RemovePlayerItem(CBasePlayerItem *pItem)
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim();
		pItem->pev->nextthink = 0;
		pItem->SetThink(NULL);
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	else if (m_pLastItem == pItem)
		m_pLastItem = NULL;

	CBasePlayerItem *pPrev = m_rgpPlayerItems[pItem->iItemSlot()];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot()] = pItem->m_pNext;
		return TRUE;
	}

	while (pPrev && pPrev->m_pNext != pItem)
		pPrev = pPrev->m_pNext;

	if (pPrev)
	{
		pPrev->m_pNext = pItem->m_pNext;
		return TRUE;
	}

	return FALSE;
}

int CBasePlayer::GiveAmmo(int iCount, char *szName, int iMax)
{
	if (pev->flags & FL_SPECTATOR)
		return -1;

	if (!szName)
		return -1;

	if (!g_pGameRules->CanHaveAmmo(this, szName, iMax))
		return -1;

	int i = GetAmmoIndex(szName);

	if (i < 0 || i >= MAX_AMMO_SLOTS)
		return -1;

	int iAdd = min(iCount, iMax - m_rgAmmo[i]);

	if (iAdd < 1)
		return i;

	m_rgAmmo[i] += iAdd;

	if (gmsgAmmoPickup)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoPickup, NULL, pev);
		WRITE_BYTE(GetAmmoIndex(szName));
		WRITE_BYTE(iAdd);
		MESSAGE_END();
	}

	TabulateAmmo();
	return i;
}

void CBasePlayer::ItemPreFrame(void)
{
#ifdef CLIENT_WEAPONS
	if (m_flNextAttack > 0)
		return;
#else
	if (gpGlobals->time < m_flNextAttack)
		return;
#endif

	if (m_pActiveItem)
		m_pActiveItem->ItemPreFrame();
}

void CBasePlayer::ItemPostFrame(void)
{
	if (m_pTank != 0)
		return;

	if (m_pActiveItem)
	{
		if (HasShield() && IsReloading())
		{
			if (pev->button & IN_ATTACK2)
				m_flNextAttack = UTIL_WeaponTimeBase();
		}
	}

#ifdef CLIENT_WEAPONS
	if (m_flNextAttack > 0)
		return;
#else
	if (gpGlobals->time < m_flNextAttack)
		return;
#endif

	ImpulseCommands();

	if (m_pActiveItem)
		m_pActiveItem->ItemPostFrame();
}

int CBasePlayer::AmmoInventory(int iAmmoIndex)
{
	if (iAmmoIndex == -1)
		return -1;

	return m_rgAmmo[iAmmoIndex];
}

int CBasePlayer::GetAmmoIndex(const char *psz)
{
	if (!psz)
		return -1;

	for (int i = 1; i < MAX_AMMO_SLOTS; i++)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if (!stricmp(psz, CBasePlayerItem::AmmoInfoArray[i].pszName))
			return i;
	}

	return -1;
}

void CBasePlayer::SendAmmoUpdate(void)
{
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (m_rgAmmo[i] == m_rgAmmoLast[i])
			continue;

		m_rgAmmoLast[i] = m_rgAmmo[i];

		ASSERT(m_rgAmmo[i] >= 0);
		ASSERT(m_rgAmmo[i] < 255);

		MESSAGE_BEGIN(MSG_ONE, gmsgAmmoX, NULL, pev);
		WRITE_BYTE(i);
		WRITE_BYTE(max(min(m_rgAmmo[i], 254), 0));
		MESSAGE_END();
	}
}

void CBasePlayer::SendHostagePos(void)
{
	CHostage *pHostage = NULL;

	while ((pHostage = (CHostage *)UTIL_FindEntityByClassname(pHostage, "hostage_entity")) != NULL)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHostagePos, NULL, pev);
		WRITE_BYTE(1);
		WRITE_BYTE(pHostage->m_iHostageIndex);
		WRITE_COORD(pHostage->pev->origin.x);
		WRITE_COORD(pHostage->pev->origin.y);
		WRITE_COORD(pHostage->pev->origin.z);
		MESSAGE_END();
	}

	SendHostageIcons();
}

void CBasePlayer::SendHostageIcons(void)
{

}

void CBasePlayer::SendWeatherInfo(void)
{
	if (UTIL_FindEntityByClassname(NULL, "env_rain") || UTIL_FindEntityByClassname(NULL, "func_rain"))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgReceiveW, NULL, pev);
		WRITE_BYTE(1);
		MESSAGE_END();
	}
	else if (UTIL_FindEntityByClassname(NULL, "env_snow") || UTIL_FindEntityByClassname(NULL, "func_snow"))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgReceiveW, NULL, pev);
		WRITE_BYTE(2);
		MESSAGE_END();
	}
}

void CBasePlayer::UpdateClientData(void)
{
	if (m_fInitHUD)
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;
		m_signals.Update();

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, NULL, pev);
		MESSAGE_END();

		if (!m_fGameHUDInitialized)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, NULL, pev);
			MESSAGE_END();

			CClientFog *pFog = (CClientFog *)UTIL_FindEntityByClassname(NULL, "env_fog");

			if (pFog)
			{
				int r = pFog->pev->rendercolor[0];
				int g = pFog->pev->rendercolor[1];
				int b = pFog->pev->rendercolor[2];
				int density_y = pFog->pev->rendercolor[3];

				if (r > 255)
					r = 255;
				else if (r < 0)
					r = 0;

				if (g > 255)
					g = 255;
				else if (g < 0)
					g = 0;

				if (b > 255)
					b = 255;
				else if (b < 0)
					b = 0;

				MESSAGE_BEGIN(MSG_ONE, gmsgFog, NULL, pev);
				WRITE_BYTE(r);
				WRITE_BYTE(g);
				WRITE_BYTE(b);
				WRITE_BYTE((int)pFog->m_fDensity << 24);
				WRITE_BYTE((int)pFog->m_fDensity << 16);
				WRITE_BYTE((int)pFog->m_fDensity << 8);
				WRITE_BYTE((int)pFog->m_fDensity);
				MESSAGE_END();
			}

			g_pGameRules->InitHUD(this);
			m_fGameHUDInitialized = TRUE;

			if (g_pGameRules->IsMultiplayer())
				FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);

			m_iObserverLastMode = OBS_CHASE_FREE;
			m_iObserverC4State = 0;
			m_bObserverHasDefuser = false;
			SetObserverAutoDirector(false);
		}

		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);

		MESSAGE_BEGIN(MSG_ONE, gmsgMoney, NULL, pev);
		WRITE_LONG(m_iAccount);
		WRITE_BYTE(0);
		MESSAGE_END();

		if (m_bHasDefuser == true)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
			WRITE_BYTE(STATUSICON_SHOW);
			WRITE_STRING("defuser");
			WRITE_BYTE(0);
			WRITE_BYTE(160);
			WRITE_BYTE(0);
			MESSAGE_END();
		}

		SetBombIcon(FALSE);
		SyncRoundTimer();
		SendHostagePos();
		SendWeatherInfo();

		if (g_pGameRules->IsMultiplayer())
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgTeamScore, NULL, pev);
			WRITE_STRING(GetTeam(TEAM_CT));
			WRITE_SHORT(g_pGameRules->m_iNumCTWins);
			MESSAGE_END();

			MESSAGE_BEGIN(MSG_ONE, gmsgTeamScore, NULL, pev);
			WRITE_STRING(GetTeam(TEAM_TERRORIST));
			WRITE_SHORT(g_pGameRules->m_iNumTerroristWins);
			MESSAGE_END();
		}
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pev);
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if (m_iFOV != m_iClientFOV)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
		WRITE_BYTE(m_iFOV);
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_SPEC, gmsgHLTV);
		WRITE_BYTE(ENTINDEX(edict()));
		WRITE_BYTE(m_iFOV);
		MESSAGE_END();
	}

	if (gDisplayTitle)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgShowGameTitle, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		gDisplayTitle = FALSE;
	}

	if (pev->health != m_iClientHealth)
	{
		int iHelath = max(pev->health, 0);
		MESSAGE_BEGIN(MSG_ONE, gmsgHealth, NULL, pev);
		WRITE_BYTE(iHelath);
		MESSAGE_END();

		m_iClientHealth = (int)pev->health;
	}

	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = (int)pev->armorvalue;

		ASSERT(gmsgBattery > 0);

		MESSAGE_BEGIN(MSG_ONE, gmsgBattery, NULL, pev);
		WRITE_SHORT((int)pev->armorvalue);
		MESSAGE_END();
	}

	if (pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		Vector damageOrigin = pev->origin;
		edict_t *other = pev->dmg_inflictor;

		if (other)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(other);

			if (pEntity)
				damageOrigin = pEntity->Center();
		}

		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN(MSG_ONE, gmsgDamage, NULL, pev);
		WRITE_BYTE((int)pev->dmg_save);
		WRITE_BYTE((int)pev->dmg_take);
		WRITE_LONG(visibleDamageBits);
		WRITE_COORD(damageOrigin.x);
		WRITE_COORD(damageOrigin.y);
		WRITE_COORD(damageOrigin.z);
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	if (m_flFlashLightTime && m_flFlashLightTime <= gpGlobals->time)
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				m_iFlashBattery--;

				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgFlashBattery, NULL, pev);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();
	}

	if (m_iTrain & TRAIN_NEW)
	{
		ASSERT(gmsgTrain > 0);

		MESSAGE_BEGIN(MSG_ONE, gmsgTrain, NULL, pev);
		WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	SendAmmoUpdate();

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
			m_rgpPlayerItems[i]->UpdateClientData(this);
	}

	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	if (m_flNextSBarUpdateTime < gpGlobals->time)
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}

	if (!(m_flDisplayHistory & DHF_AMMO_EXHAUSTED))
	{
		if (m_pActiveItem && m_pActiveItem->IsWeapon())
		{
			CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_pActiveItem;

			if (!(pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE))
			{
				if (pWeapon->m_iPrimaryAmmoType != -1 && m_rgAmmo[pWeapon->m_iPrimaryAmmoType] == 0 && pWeapon->m_iClip == 0)
				{
					m_flDisplayHistory |= DHF_AMMO_EXHAUSTED;
					HintMessage("#Hint_out_of_ammo");
				}
			}
		}
	}

	if (gpGlobals->time > m_tmHandleSignals)
	{
		m_tmHandleSignals = gpGlobals->time + 0.5;
		HandleSignals();
	}

	if (pev->deadflag == DEAD_NO)
	{
		if (gpGlobals->time > m_tmNextRadarUpdate)
		{
			m_tmNextRadarUpdate = gpGlobals->time + 1;

			if ((pev->origin - m_vLastOrigin).Length() >= 64)
			{
				for (int k = 1; k <= gpGlobals->maxClients; k++)
				{
					CBaseEntity *pEntity = UTIL_PlayerByIndex(k);

					if (!pEntity || k == ENTINDEX(edict()))
						continue;

					CBasePlayer *pOther = GetClassPtr((CBasePlayer *)pEntity->pev);

					if (pOther->pev->flags == FL_DORMANT)
						continue;

					if (pOther->pev->deadflag != DEAD_NO)
						continue;

					if (pOther->m_iTeam == m_iTeam)
					{
						MESSAGE_BEGIN(MSG_ONE, gmsgRadar, NULL, pOther->pev);
						WRITE_BYTE(ENTINDEX(edict()));
						WRITE_COORD(pev->origin.x);
						WRITE_COORD(pev->origin.y);
						WRITE_COORD(pev->origin.z);
						MESSAGE_END();
					}
				}
			}

			m_vLastOrigin = pev->origin;
		}
	}
}

BOOL CBasePlayer::FBecomeProne(void)
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

void CBasePlayer::BarnacleVictimBitten(entvars_t *pevBarnacle)
{
	TakeDamage(pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB);
}

void CBasePlayer::BarnacleVictimReleased(void)
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}

int CBasePlayer::Illumination(void)
{
	int iIllum = CBaseEntity::Illumination() + m_iWeaponFlash;

	if (iIllum > 255)
		iIllum = 255;

	return iIllum;
}

void CBasePlayer::EnableControl(BOOL fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}

void CBasePlayer::ResetMaxSpeed(void)
{
	float speed = 240;

	if (IsObserver())
		speed = 900;
	else if (g_pGameRules->IsMultiplayer() && g_pGameRules->IsFreezePeriod())
		speed = 1;
	else if (m_bIsVIP == true)
		speed = 227;
	else if (m_pActiveItem)
		speed = m_pActiveItem->GetMaxSpeed();

	g_engfuncs.pfnSetClientMaxspeed(ENT(pev), speed);
}

bool CBasePlayer::HintMessage(const char *pMessage, BOOL bDisplayIfDead, BOOL bOverrideClientSettings)
{
	if (!bDisplayIfDead && !IsAlive())
		return false;

	if (bOverrideClientSettings || m_bShowHints)
		return m_hintMessageQueue.AddMessage(pMessage, 6.0, true, NULL);

	return true;
}

#define DOT_1DEGREE 0.9998476951564
#define DOT_2DEGREE 0.9993908270191
#define DOT_3DEGREE 0.9986295347546
#define DOT_4DEGREE 0.9975640502598
#define DOT_5DEGREE 0.9961946980917
#define DOT_6DEGREE 0.9945218953683
#define DOT_7DEGREE 0.9925461516413
#define DOT_8DEGREE 0.9902680687416
#define DOT_9DEGREE 0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

Vector CBasePlayer::GetAutoaimVector(float flDelta)
{
	if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors(pev->v_angle + pev->punchangle);
		return gpGlobals->v_forward;
	}

	Vector vecSrc = GetGunPosition();
	m_vecAutoAim = Vector(0, 0, 0);
	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, 8192, flDelta);

	if (!g_pGameRules->AllowAutoTargetCrosshair())
		m_fOnTarget = FALSE;
	else if (m_fOldTargeting != m_fOnTarget)
		m_pActiveItem->UpdateItemInfo();

	if (angles.x > 180)
		angles.x -= 360;

	if (angles.x < 180)
		angles.x += 360;

	if (angles.y > 180)
		angles.y -= 360;

	if (angles.y < 180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;

	if (angles.x < -25)
		angles.x = -25;

	if (angles.y > 12)
		angles.y = 12;

	if (angles.y < -12)
		angles.y = -12;

	if (g_iSkillLevel == SKILL_EASY)
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	else
		m_vecAutoAim = angles * 0.9;

	if (sv_aim && sv_aim->value > 0)
	{
		if (m_vecAutoAim.x != m_lastx || m_vecAutoAim.y != m_lasty)
		{
			SET_CROSSHAIRANGLE(ENT(pev), -m_vecAutoAim.x, m_vecAutoAim.y);

			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}

	UTIL_MakeVectors(pev->v_angle + pev->punchangle + m_vecAutoAim);
	return gpGlobals->v_forward;
}

Vector CBasePlayer::AutoaimDeflection(Vector &vecSrc, float flDist, float flDelta)
{
	m_fOnTarget = FALSE;
	return g_vecZero;
}

void CBasePlayer::ResetAutoaim(void)
{
	if (m_vecAutoAim.x || m_vecAutoAim.y)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		SET_CROSSHAIRANGLE(ENT(pev), 0, 0);
	}

	m_fOnTarget = FALSE;
}

void CBasePlayer::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 && nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

int CBasePlayer::GetCustomDecalFrames(void)
{
	return m_nCustomSprayFrames;
}

void CBasePlayer::Blind(float flUntilTime, float flHoldTime, float flFadeTime, int iAlpha)
{
	m_blindUntilTime = flUntilTime + gpGlobals->time;
	m_blindStartTime = gpGlobals->time;
	m_blindHoldTime = flHoldTime;
	m_blindFadeTime = flFadeTime;
	m_blindAlpha = iAlpha;
}

void CBasePlayer::DropPlayerItem(const char *pszItemName)
{
	if (!strlen(pszItemName))
		pszItemName = NULL;

	if (m_bIsVIP)
	{
		ClientPrint(pev, HUD_PRINTCENTER, "#Weapon_Cannot_Be_Dropped");
		return;
	}

	if (!pszItemName && HasShield() == true)
		DropShield(true);

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		CBasePlayerItem *pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			if (pszItemName)
			{
				if (!strcmp(pszItemName, STRING(pWeapon->pev->classname)))
					break;
			}
			else if (pWeapon == m_pActiveItem)
				break;

			pWeapon = pWeapon->m_pNext;
		}

		if (pWeapon)
		{
			if (!pWeapon->CanDrop())
			{
				ClientPrint(pev, HUD_PRINTCENTER, "#Weapon_Cannot_Be_Dropped");
				continue;
			}

			g_pGameRules->GetNextBestWeapon(this, pWeapon);
			UTIL_MakeVectors(pev->angles);
			pev->weapons &= ~(1 << pWeapon->m_iId);

			if (pWeapon->iItemSlot() == WPNSLOT_PRIMARY)
				m_bHasPrimary = false;

			if (!strcmp(STRING(pWeapon->pev->classname), "weapon_c4"))
			{
				m_bHasC4 = false;
				pev->body = 0;
				SetBombIcon(FALSE);
				pWeapon->m_pPlayer->SetProgressBarTime(0);

				if (!g_pGameRules->m_fTeamCount)
				{
					UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Dropped_The_Bomb\"\n", STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()));
					g_pGameRules->m_bBombDropped = TRUE;

					CBaseEntity *pEntity = NULL;

					while ((pEntity = UTIL_FindEntityByClassname(pEntity, "player")) != NULL)
					{
						if (FNullEnt(pEntity->edict()))
							break;

						if (!pEntity->IsPlayer())
							continue;

						if (pEntity->pev->flags != FL_DORMANT)
						{
							CBasePlayer *pOther = GetClassPtr((CBasePlayer *)pEntity->pev);

							if (pOther->pev->deadflag == DEAD_NO && pOther->m_iTeam == TEAM_TERRORIST)
							{
								ClientPrint(pOther->pev, HUD_PRINTCENTER, "#Game_bomb_drop", STRING(pev->netname));

								MESSAGE_BEGIN(MSG_ONE, gmsgBombDrop, NULL, pOther->pev);
								WRITE_COORD(pev->origin.x);
								WRITE_COORD(pev->origin.y);
								WRITE_COORD(pev->origin.z);
								WRITE_BYTE(0);
								MESSAGE_END();
							}
						}
					}
				}
			}

			CWeaponBox *pWeaponBox = (CWeaponBox *)Create("weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict());
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->SetThink(&CWeaponBox::Kill);
			pWeaponBox->pev->nextthink = gpGlobals->time + 300;
			pWeaponBox->PackWeapon(pWeapon);
			pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;

			if (pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE)
			{
				int iAmmoIndex = GetAmmoIndex(pWeapon->pszAmmo1());

				if (iAmmoIndex != -1)
				{
					pWeaponBox->PackAmmo(MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[iAmmoIndex] > 0);
					m_rgAmmo[iAmmoIndex] = 0;
				}
			}

			const char *modelname = GetCSModelName(pWeapon->m_iId);

			if (modelname)
				SET_MODEL(ENT(pWeaponBox->pev), modelname);

			return;
		}
	}
}

void CBasePlayer::ThrowPrimary(void)
{
	ThrowWeapon("weapon_m249");
	ThrowWeapon("weapon_g3sg1");
	ThrowWeapon("weapon_sg550");
	ThrowWeapon("weapon_awp");
	ThrowWeapon("weapon_mp5navy");
	ThrowWeapon("weapon_tmp");
	ThrowWeapon("weapon_p90");
	ThrowWeapon("weapon_ump45");
	ThrowWeapon("weapon_m4a1");
	ThrowWeapon("weapon_m3");
	ThrowWeapon("weapon_sg552");
	ThrowWeapon("weapon_scout");
	ThrowWeapon("weapon_galil");
	ThrowWeapon("weapon_famas");

	DropShield(true);
}

void CBasePlayer::ThrowWeapon(char *pszWeaponName)
{
	for (int i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		CBasePlayerItem *pWeapon = m_rgpPlayerItems[i];

		if (!pWeapon)
			break;

		if (!strcmp(pszWeaponName, STRING(pWeapon->pev->classname)))
		{
			DropPlayerItem(pszWeaponName);
			break;
		}

		pWeapon = pWeapon->m_pNext;
	}
}

BOOL CBasePlayer::HasPlayerItem(CBasePlayerItem *pCheckItem)
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs(pItem->pev, STRING(pCheckItem->pev->classname)))
			return TRUE;

		pItem = pItem->m_pNext;
	}

	return FALSE;
}

BOOL CBasePlayer::HasNamedPlayerItem(const char *pszItemName)
{
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		CBasePlayerItem *pItem = m_rgpPlayerItems[i];

		while (pItem)
		{
			if (!strcmp(pszItemName, STRING(pItem->pev->classname)))
				return TRUE;

			pItem = pItem->m_pNext;
		}
	}

	return FALSE;
}

void CBasePlayer::SwitchTeam(void)
{
	char *model;
	int oldTeam = m_iTeam;
	const char *name;

	if (m_iTeam == TEAM_CT)
	{
		m_iTeam = TEAM_TERRORIST;

		switch (m_iModelName)
		{
			default:
			case CLASS_GSG9: m_iModelName = CLASS_TERROR; model = "terror"; break;
			case CLASS_URBAN: m_iModelName = CLASS_LEET; model = "leet"; break;
			case CLASS_SAS: m_iModelName = CLASS_ARCTIC; model = "arctic"; break;
			case CLASS_GIGN: m_iModelName = CLASS_GUERILLA; model = "guerilla"; break;
		}

		char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());
		g_engfuncs.pfnSetClientKeyValue(entindex(), infobuffer, "model", model);
	}
	else if (m_iTeam == TEAM_TERRORIST)
	{
		m_iTeam = TEAM_CT;

		switch (m_iModelName)
		{
			default:
			case CLASS_LEET: m_iModelName = CLASS_URBAN; model = "urban"; break;
			case CLASS_TERROR: m_iModelName = CLASS_GSG9; model = "gsg9"; break;
			case CLASS_ARCTIC: m_iModelName = CLASS_SAS; model = "sas"; break;
			case CLASS_GUERILLA: m_iModelName = CLASS_GIGN; model = "gign"; break;
		}

		char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer(edict());
		g_engfuncs.pfnSetClientKeyValue(entindex(), infobuffer, "model", model);
	}

	MESSAGE_BEGIN(MSG_ALL, gmsgTeamInfo);
	WRITE_BYTE(ENTINDEX(edict()));
	WRITE_STRING(GetTeam(m_iTeam));
	MESSAGE_END();

	UpdateLocation(true);

	if (m_iTeam != TEAM_UNASSIGNED)
		SetScoreboardAttributes();

	if (pev->netname)
	{
		name = STRING(pev->netname);

		if (!*name)
			name = "<unconnected>";
	}
	else
		name = "<unconnected>";

	if (m_iTeam == TEAM_TERRORIST)
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_join_terrorist_auto", name);
	else
		UTIL_ClientPrintAll(HUD_PRINTNOTIFY, "#Game_join_ct_auto", name);

	if (m_bHasDefuser)
	{
		m_bHasDefuser = false;
		pev->body = 0;

		MESSAGE_BEGIN(MSG_ONE, gmsgStatusIcon, NULL, pev);
		WRITE_BYTE(STATUSICON_HIDE);
		WRITE_STRING("defuser");
		MESSAGE_END();

		SetProgressBarTime(0);

		for (int i = 0; i < MAX_ITEM_TYPES; i++)
		{
			m_pActiveItem = m_rgpPlayerItems[i];

			if (!m_pActiveItem)
				continue;

			if (FClassnameIs(m_pActiveItem->pev, "item_thighpack"))
			{
				m_pActiveItem->Drop();
				m_rgpPlayerItems[i] = NULL;
			}
		}
	}

	UTIL_LogPrintf("\"%s<%i><%s><%s>\" joined team \"%s\" (auto)\n", STRING(pev->netname), GETPLAYERUSERID(edict()), GETPLAYERAUTHID(edict()), GetTeam(oldTeam), GetTeam(m_iTeam));

	if (IsBot())
	{
	}
}

void CBasePlayer::UpdateShieldCrosshair(bool bShieldDrawn)
{
	if (bShieldDrawn)
      m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	else
		m_iHideHUD |= HIDEHUD_CROSSHAIR;
}

BOOL CBasePlayer::SwitchWeapon(CBasePlayerItem *pWeapon)
{
	if (!pWeapon->CanDeploy())
		return FALSE;

	ResetAutoaim();

	if (m_pActiveItem)
		m_pActiveItem->Holster();

	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = pWeapon;
	m_pLastItem = pTemp;
	pWeapon->Deploy();

	if (pWeapon->m_pPlayer)
		pWeapon->m_pPlayer->ResetMaxSpeed();

	if (HasShield() == true)
      UpdateShieldCrosshair(true);

	return TRUE;
}

void CBasePlayer::MoveToNextIntroCamera(void)
{
	m_pIntroCamera = UTIL_FindEntityByClassname(m_pIntroCamera, "trigger_camera");

	if (!m_pIntroCamera)
		m_pIntroCamera = UTIL_FindEntityByClassname(NULL, "trigger_camera");

	CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(m_pIntroCamera->pev->target));

	if (pTarget)
	{
		Vector vecDir = (pTarget->pev->origin - m_pIntroCamera->pev->origin).Normalize();
		Vector vecGoal = UTIL_VecToAngles(vecDir);
		vecGoal.x = -vecGoal.x;
		UTIL_SetOrigin(pev, m_pIntroCamera->pev->origin);

		pev->angles = vecGoal;
		pev->v_angle = pev->angles;
		pev->velocity = g_vecZero;
		pev->punchangle = g_vecZero;
		pev->fixangle = TRUE;
		pev->view_ofs = g_vecZero;
		m_fIntroCamTime = gpGlobals->time + 6;
	}
	else
		m_pIntroCamera = NULL;
}

class CDeadHEV : public CBaseMonster
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);
	int Classify(void) { return CLASS_HUMAN_MILITARY; }

public:
	int m_iPose;
	static char *m_szPoses[4];
};

char *CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue(KeyValueData *pkvd)
{
	if (!strcmp(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
};

LINK_ENTITY_TO_CLASS(monster_hevsuit_dead, CDeadHEV);

void CDeadHEV::Spawn(void)
{
	PRECACHE_MODEL("models/player.mdl");
	SET_MODEL(ENT(pev), "models/player.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead hevsuit with bad pose\n");
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	pev->health = 8;
	MonsterInitDead();
}

class CStripWeapons : public CPointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = NULL;

	if (pActivator && pActivator->IsPlayer())
		pPlayer = (CBasePlayer *)pActivator;
	else if (!g_pGameRules->IsDeathmatch())
		pPlayer = (CBasePlayer *)Instance(INDEXENT(1));

	if (pPlayer)
		pPlayer->RemoveAllItems(FALSE);
}

class CRevertSaved : public CPointEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int Save(CSave &save);
	int Restore(CRestore &restore);

public:
	void EXPORT MessageThink(void);
	void EXPORT LoadThink(void);

public:
	inline float Duration(void) { return pev->dmg_take; }
	inline float HoldTime(void) { return pev->dmg_save; }
	inline float MessageTime(void) { return m_messageTime; }
	inline float LoadTime(void) { return m_loadTime; }
	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

public:
	static TYPEDESCRIPTION m_SaveData[];

public:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
{
	DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT),
	DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

void CRevertSaved::KeyValue(KeyValueData *pkvd)
{
	if (!strcmp(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CRevertSaved::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}

void CRevertSaved::MessageThink(void)
{
	UTIL_ShowMessageAll(STRING(pev->message));
	float nextThink = LoadTime() - MessageTime();

	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}

void CRevertSaved::LoadThink(void)
{
	if (!gpGlobals->deathmatch)
		SERVER_COMMAND("reload\n");
}

class CInfoIntermission : public CPointEntity
{
public:
	void Spawn(void);
	void Think(void);
};

void CInfoIntermission::Spawn(void)
{
	UTIL_SetOrigin(pev, pev->origin);

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;
	pev->nextthink = gpGlobals->time + 2;
}

void CInfoIntermission::Think(void)
{
	edict_t *pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);

void CBasePlayer::StudioEstimateGait(void)
{
	float dt;
	vec3_t est_velocity;

	dt = gpGlobals->frametime;

	if (dt < 0)
		dt = 0;
	else if (dt > 1)
		dt = 1;

	if (dt == 0) 
	{
		m_flGaitMovement = 0;
		return;
	}

	est_velocity[0] = pev->origin[0] - m_prevgaitorigin[0];
	est_velocity[1] = pev->origin[1] - m_prevgaitorigin[1];
	est_velocity[2] = pev->origin[2] - m_prevgaitorigin[2];
	m_prevgaitorigin[0] = pev->origin[0];
	m_prevgaitorigin[1] = pev->origin[1];
	m_prevgaitorigin[2] = pev->origin[2];
	m_flGaitMovement = sqrt(est_velocity[0] * est_velocity[0] + est_velocity[1] * est_velocity[1] + est_velocity[2] * est_velocity[2]); 

	if (dt <= 0 || m_flGaitMovement / dt < 5)
	{
		m_flGaitMovement = 0;
		est_velocity[0] = est_velocity[1] = 0;
	}

	if (est_velocity[0] == 0 && est_velocity[1] == 0)
	{
		float flYawDiff = pev->angles[1] - m_flGaityaw;
		float flYaw = flYawDiff;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;

		if (flYawDiff > 180)
			flYawDiff -= 360;

		if (flYawDiff < -180)
			flYawDiff += 360;

		if (flYaw < -180)
			flYaw += 360;
		else if (flYaw > 180)
			flYaw -= 360;

		if (flYaw > -5 && flYaw < 5)
			m_flYawModifier = 0.05;

		if (flYaw < -90 || flYaw > 90)
			m_flYawModifier = 3.5;

		if (dt < 0.25)
			flYawDiff *= dt * m_flYawModifier;
		else
			flYawDiff *= dt;

		if (fabs(flYawDiff) < 0.1)
			flYawDiff = 0;

		m_flGaityaw += flYawDiff;
		m_flGaityaw -= (int)(m_flGaityaw / 360) * 360;
		m_flGaitMovement = 0;
	}
	else
	{
		m_flGaityaw = (atan2(est_velocity[1], est_velocity[0]) * 180 / M_PI);

		if (m_flGaityaw > 180)
			m_flGaityaw = 180;

		if (m_flGaityaw < -180)
			m_flGaityaw = -180;
	}
}

void CBasePlayer::CalculatePitchBlend(void)
{
	int iBlend;
	float temp;

	temp = (int)(pev->angles[0] * 3);

	if (temp <= -45)
		iBlend = 255;
	else if (temp < 45)
		iBlend = ((45.0 - temp) / (45.0 + 45)) * 255;
	else
		iBlend = 0;

	pev->blending[1] = iBlend;
	m_flPitch = iBlend;
}

void CBasePlayer::CalculateYawBlend(void)
{
	float dt;
	float flYaw;
	float maxyaw;
	float blend_yaw;

	dt = gpGlobals->frametime;

	if (dt < 0)
		dt = 0;
	else if (dt > 1)
		dt = 1;

	StudioEstimateGait();

	maxyaw = 255.0;
	flYaw = pev->angles[1] - m_flGaityaw;

	if (flYaw < -180)
		flYaw += 360;
	else if (flYaw > 180)
		flYaw -= 360;

	if (m_flGaitMovement != 0)
	{
		if (flYaw > 120)
		{
			m_flGaityaw -= 180;
			m_flGaitMovement = -m_flGaitMovement;
			flYaw -= 180;
		}
		else if (flYaw < -120)
		{
			m_flGaityaw += 180;
			m_flGaitMovement = -m_flGaitMovement;
			flYaw += 180;
		}
	}

	flYaw = (flYaw / 90) * 128.0 + 127.0;

	if (flYaw > 255)
		flYaw = 255;
	else if (flYaw < 0)
		flYaw = 0;

	blend_yaw = maxyaw - flYaw;
	pev->blending[0] = (int)(blend_yaw);
	m_flYaw = blend_yaw;
}

void CBasePlayer::StudioProcessGait(void)
{
	mstudioseqdesc_t *pseqdesc;
	float dt = gpGlobals->frametime;

	if (dt < 0)
		dt = 0;
	else if (dt > 1.0)
		dt = 1;

	CalculateYawBlend();
	CalculatePitchBlend();

	void *model = GET_MODEL_PTR(ENT(pev));

	if (!model)
		return;

	studiohdr_t *pstudiohdr = (studiohdr_t *)model;

	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + pev->gaitsequence;

	if (pseqdesc->linearmovement[0] > 0)
		m_flGaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	else
		m_flGaitframe += pseqdesc->fps * dt * pev->framerate;

	m_flGaitframe -= (int)(m_flGaitframe / pseqdesc->numframes) * pseqdesc->numframes;

	if (m_flGaitframe < 0)
		m_flGaitframe += pseqdesc->numframes;

}

float GetPlayerPitch(const edict_t *pEdict)
{
	entvars_t *pev = VARS((edict_t *)pEdict);
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);

	if (!pPlayer)
		return 0;

	return pPlayer->m_flPitch;
}

float GetPlayerYaw(const edict_t *pEdict)
{
	entvars_t *pev = VARS((edict_t *)pEdict);
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);

	if (!pPlayer)
		return 0;

	return pPlayer->m_flYaw;
}

int GetPlayerGaitsequence(const edict_t *pEdict)
{
	entvars_t *pev = VARS((edict_t *)pEdict);
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pev);

	if (!pPlayer)
		return 0;

	return pPlayer->m_iGaitsequence;
}

BOOL CBasePlayer::IsArmored(int nHitGroup)
{
	if (m_iKevlar == 0)
		return FALSE;

	switch (nHitGroup)
	{
		case HITGROUP_HEAD:
		{
			if (m_iKevlar == 2)
				return TRUE;

			return FALSE;
		}

		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM: return TRUE;
	}

	return FALSE;
}

BOOL CBasePlayer::ShouldDoLargeFlinch(int nHitGroup, int nGunType)
{
	if (pev->flags & FL_DUCKING)
		return FALSE;

	if (nHitGroup != HITGROUP_LEFTLEG && nHitGroup != HITGROUP_RIGHTLEG)
	{
		switch (nGunType)
		{
			case WEAPON_SCOUT:
			case WEAPON_AUG:
			case WEAPON_SG550:
			case WEAPON_GALIL:
			case WEAPON_FAMAS:
			case WEAPON_AWP:
			case WEAPON_M3:
			case WEAPON_M4A1:
			case WEAPON_G3SG1:
			case WEAPON_DEAGLE:
			case WEAPON_SG552:
			case WEAPON_AK47: return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SpawnClientSideCorpse(void)
{
   char *infobuffer = GET_INFO_BUFFER(edict());
   char *pModel = GET_KEY_VALUE(infobuffer, "model");

   MESSAGE_BEGIN(MSG_ALL, gmsgSendCorpse);
   WRITE_STRING(pModel);
   WRITE_LONG(pev->origin.x * 128);
   WRITE_LONG(pev->origin.y * 128);
   WRITE_LONG(pev->origin.z * 128);
   WRITE_COORD(pev->angles.x);
   WRITE_COORD(pev->angles.y);
   WRITE_COORD(pev->angles.z);
   WRITE_LONG((pev->animtime - gpGlobals->time) * 100);
   WRITE_BYTE(pev->sequence);
   WRITE_BYTE(pev->body);
   WRITE_BYTE(m_iTeam);
   WRITE_BYTE(entindex());
   MESSAGE_END();

   m_canSwitchObserverModes = true;
   /*
   if (TheTutor)
                TheTutor->OnEvent(EVENT_CLIENT_CORPSE_SPAWNED, this, NULL);
   */
}

void CBasePlayer::SetPrefsFromUserinfo(char *infobuffer)
{
	const char *pszKeyVal;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue(infobuffer, "_cl_autowepswitch");

	if (strcmp(pszKeyVal, ""))
		m_iAutoWepSwitch = atoi(pszKeyVal);
	else
		m_iAutoWepSwitch = 1;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue(infobuffer, "_vgui_menus");

	if (strcmp(pszKeyVal, ""))
		m_bVGUIMenus = atoi(pszKeyVal) != 0;
	else
		m_bVGUIMenus = true;

	pszKeyVal = g_engfuncs.pfnInfoKeyValue(infobuffer, "_ah");

	if (strcmp(pszKeyVal, ""))
		m_bShowHints = atoi(pszKeyVal) != 0;
	else
		m_bShowHints = true;
}

const char *BotArgs[4];
bool UseBotArgs = false;

void CBasePlayer::ClientCommand(const char *arg0, const char *arg1, const char *arg2, const char *arg3)
{
	UseBotArgs = true;
	BotArgs[0] = arg0;
	BotArgs[1] = arg1;
	BotArgs[2] = arg2;
	BotArgs[3] = arg3;

	::ClientCommand(edict());
	UseBotArgs = false;
}

const char *GetBuyStringForWeaponClass(int wpnclass)
{
	switch (wpnclass)
	{
		case WEAPONCLASS_PISTOL: return "deagle elites fn57 usp glock p228 shield";
		case WEAPONCLASS_GRENADE: return "hegren";
		case WEAPONCLASS_SUBMACHINEGUN: return "p90 ump45 mp5 tmp mac10";
		case WEAPONCLASS_SHOTGUN: return "xm1014 m3";
		case WEAPONCLASS_MACHINEGUN: return "m249";
		case WEAPONCLASS_RIFLE: return "sg552 aug ak47 m4a1 galil famas";
		case WEAPONCLASS_SNIPERRIFLE: return "awp sg550 g3sg1 scout";
	}

	return NULL;
}

void CBasePlayer::ClearAutoBuyData(void)
{
	m_autoBuyString[0] = 0;
}

void CBasePlayer::AddAutoBuyData(const char *string)
{
	int len = strlen(m_autoBuyString);

	if (len < MAX_AUTOBUY_LENGTH - 1)
	{
		if (len > 0)
			m_autoBuyString[len] = ' ';

		strncat(m_autoBuyString, string, MAX_AUTOBUY_LENGTH - len-1);//strncat вставляет терминальный 0.
	}
}

void CBasePlayer::InitRebuyData(const char *string)
{
	if (!string)
		return;

	int len = strlen(string);

	if (len <= MAX_REBUY_LENGTH)
	{
		if (m_rebuyString)
		{
			delete m_rebuyString;
			m_rebuyString = NULL;
		}

		m_rebuyString = new char[strlen(string) + 1];
		strcpy(m_rebuyString, string);
		m_rebuyString[strlen(string)] = 0;
	}
}

void CBasePlayer::AutoBuy(void)
{
	bool boughtPrimary = false, boughtSecondary = false;

	if (m_autoBuyString[0])
		ParseAutoBuyString(m_autoBuyString, boughtPrimary, boughtSecondary);
}

WeaponAliasInfo weaponAliasInfo[] =
{
	{ "p228", WEAPON_P228 },
	{ "???", WEAPON_GLOCK },
	{ "scout", WEAPON_SCOUT },
	{ "hegren", WEAPON_HEGRENADE },
	{ "xm1014", WEAPON_XM1014 },
	{ "c4", WEAPON_C4 },
	{ "mac10", WEAPON_MAC10 },
	{ "aug", WEAPON_AUG },
	{ "sgren", WEAPON_SMOKEGRENADE },
	{ "elites", WEAPON_ELITE },
	{ "fn57", WEAPON_FIVESEVEN },
	{ "ump45", WEAPON_UMP45 },
	{ "sg550", WEAPON_SG550 },
	{ "galil", WEAPON_GALIL },
	{ "famas", WEAPON_FAMAS },
	{ "usp", WEAPON_USP },
	{ "glock", WEAPON_GLOCK18 },
	{ "awp", WEAPON_AWP },
	{ "mp5", WEAPON_MP5N },
	{ "m249", WEAPON_M249 },
	{ "m3", WEAPON_M3 },
	{ "m4a1", WEAPON_M4A1 },
	{ "tmp", WEAPON_TMP },
	{ "g3sg1", WEAPON_G3SG1 },
	{ "flash", WEAPON_FLASHBANG },
	{ "deagle", WEAPON_DEAGLE },
	{ "sg552", WEAPON_SG552 },
	{ "ak47", WEAPON_AK47 },
	{ "knife", WEAPON_KNIFE },
	{ "p90", WEAPON_P90 },
	{ "shield", WEAPON_SHIELDGUN },
	{ "none", WEAPON_NONE },
	{ "grenade", WEAPON_HEGRENADE },
	{ "hegrenade", WEAPON_HEGRENADE },
	{ "glock18", WEAPON_GLOCK18 },
	{ "elite", WEAPON_ELITE },
	{ "fiveseven", WEAPON_FIVESEVEN },
	{ "mp5navy", WEAPON_MP5N },
	{ NULL, WEAPON_NONE }
};

WeaponBuyAliasInfo weaponBuyAliasInfo[] =
{
	{ "galil", WEAPON_GALIL, "#Galil" },
	{ "defender", WEAPON_GALIL, "#Galil" },
	{ "ak47", WEAPON_AK47, "#AK47" },
	{ "cv47", WEAPON_AK47, "#AK47" },
	{ "scout", WEAPON_SCOUT, NULL },
	{ "sg552", WEAPON_SG552, "#SG552" },
	{ "krieg552", WEAPON_SG552, "#SG552" },
	{ "awp", WEAPON_AWP, NULL },
	{ "magnum", WEAPON_AWP, NULL },
	{ "g3sg1", WEAPON_G3SG1, "#G3SG1" },
	{ "d3au1", WEAPON_G3SG1, "#G3SG1" },
	{ "famas", WEAPON_FAMAS, "#Famas" },
	{ "clarion", WEAPON_FAMAS, "#Famas" },
	{ "m4a1", WEAPON_M4A1, "#M4A1" },
	{ "aug", WEAPON_AUG, "#Aug" },
	{ "bullpup", WEAPON_AUG, "#Aug" },
	{ "sg550", WEAPON_SG550, "#SG550" },
	{ "krieg550", WEAPON_SG550, "#SG550" },
	{ "glock", WEAPON_GLOCK18, NULL },
	{ "9x19mm", WEAPON_GLOCK18, NULL },
	{ "usp", WEAPON_USP, NULL },
	{ "km45", WEAPON_USP, NULL },
	{ "p228", WEAPON_P228, NULL },
	{ "228compact", WEAPON_P228, NULL },
	{ "deagle", WEAPON_DEAGLE, NULL },
	{ "nighthawk", WEAPON_DEAGLE, NULL },
	{ "elites", WEAPON_ELITE, "#Beretta96G" },
	{ "fn57", WEAPON_FIVESEVEN, "#FiveSeven" },
	{ "fiveseven", WEAPON_FIVESEVEN, "#FiveSeven" },
	{ "m3", WEAPON_M3, NULL },
	{ "12gauge", WEAPON_M3, NULL },
	{ "xm1014", WEAPON_XM1014, NULL },
	{ "autoshotgun", WEAPON_XM1014, NULL },
	{ "mac10", WEAPON_MAC10, "#Mac10" },
	{ "tmp", WEAPON_TMP, "#tmp" },
	{ "mp", WEAPON_TMP, "#tmp" },
	{ "mp5", WEAPON_MP5N, NULL },
	{ "smg", WEAPON_MP5N, NULL },
	{ "ump45", WEAPON_UMP45, NULL },
	{ "p90", WEAPON_P90, NULL },
	{ "c90", WEAPON_P90, NULL },
	{ "m249", WEAPON_M249, NULL },
	{ NULL, 0 }
};

WeaponClassAliasInfo weaponClassAliasInfo[] =
{
	{ "p228", WEAPONCLASS_PISTOL },
	{ "???", WEAPONCLASS_PISTOL },
	{ "scout", WEAPONCLASS_SNIPERRIFLE },
	{ "hegren", WEAPONCLASS_GRENADE },
	{ "xm1014", WEAPONCLASS_SHOTGUN },
	{ "c4", WEAPONCLASS_GRENADE },
	{ "mac10", WEAPONCLASS_SUBMACHINEGUN },
	{ "aug", WEAPONCLASS_RIFLE },
	{ "sgren", WEAPONCLASS_GRENADE },
	{ "elites", WEAPONCLASS_PISTOL },
	{ "fn57", WEAPONCLASS_PISTOL },
	{ "ump45", WEAPONCLASS_SUBMACHINEGUN },
	{ "sg550", WEAPONCLASS_SNIPERRIFLE },
	{ "galil", WEAPONCLASS_RIFLE },
	{ "famas", WEAPONCLASS_RIFLE },
	{ "usp", WEAPONCLASS_PISTOL },
	{ "glock", WEAPONCLASS_PISTOL },
	{ "awp", WEAPONCLASS_SNIPERRIFLE },
	{ "mp5", WEAPONCLASS_SUBMACHINEGUN },
	{ "m249", WEAPONCLASS_MACHINEGUN },
	{ "m3", WEAPONCLASS_SHOTGUN },
	{ "m4a1", WEAPONCLASS_RIFLE },
	{ "tmp", WEAPONCLASS_SUBMACHINEGUN },
	{ "g3sg1", WEAPONCLASS_SNIPERRIFLE },
	{ "flash", WEAPONCLASS_GRENADE },
	{ "deagle", WEAPONCLASS_PISTOL },
	{ "sg552", WEAPONCLASS_RIFLE },
	{ "ak47", WEAPONCLASS_RIFLE },
	{ "knife", WEAPONCLASS_KNIFE },
	{ "p90", WEAPONCLASS_SUBMACHINEGUN },
	{ "shield", WEAPONCLASS_PISTOL },
	{ "grenade", WEAPONCLASS_GRENADE },
	{ "hegrenade", WEAPONCLASS_GRENADE },
	{ "glock18", WEAPONCLASS_PISTOL },
	{ "elite", WEAPONCLASS_PISTOL },
	{ "fiveseven", WEAPONCLASS_PISTOL },
	{ "mp5navy", WEAPONCLASS_SUBMACHINEGUN },
	{ "grenade", WEAPONCLASS_GRENADE },
	{ "pistol", WEAPONCLASS_PISTOL },
	{ "SMG", WEAPONCLASS_SUBMACHINEGUN },
	{ "machinegun", WEAPONCLASS_MACHINEGUN },
	{ "shotgun", WEAPONCLASS_SHOTGUN },
	{ "rifle", WEAPONCLASS_RIFLE },
	{ "rifle", WEAPONCLASS_RIFLE },
	{ "sniper", WEAPONCLASS_SNIPERRIFLE },
	{ "none", WEAPONCLASS_NONE },
	{ NULL, 0 }
};

int AliasToWeaponID(const char *alias)
{
	if (alias)
	{
		int i = 0;

		while (weaponAliasInfo[i].alias)
		{
			if (!stricmp(weaponAliasInfo[i].alias, alias))
				break;

			i++;
		}

		return weaponAliasInfo[i].id;
	}

	return WEAPON_NONE;
}

const char *BuyAliasToWeaponID(const char *alias, int &wpnId)
{
	if (alias)
	{
		int i = 0;

		while (weaponBuyAliasInfo[i].alias)
		{
			if (!stricmp(weaponBuyAliasInfo[i].alias, alias))
				break;

			i++;
		}

		wpnId = weaponBuyAliasInfo[i].id;
		return weaponBuyAliasInfo[i].failName;
	}

	wpnId = WEAPON_NONE;
	return NULL;
}

const char *WeaponIDToAlias(int wpnId)
{
	int i = 0;

	while (1)
	{
		if (weaponAliasInfo[i].id == wpnId)
			break;

		i++;
	}

	return weaponAliasInfo[i].alias;
}

int AliasToWeaponClass(const char *alias)
{
	int i = 0;

	while (1)
	{
		if (!stricmp(weaponClassAliasInfo[i].alias, alias))
			break;

		i++;
	}

	return weaponClassAliasInfo[i].id;
}

int WeaponIDToWeaponClass(int wpnId)
{
	int i = 0;

	while (1)
	{
		if (weaponAliasInfo[i].id == wpnId)
			break;

		i++;
	}

	return AliasToWeaponClass(weaponAliasInfo[i].alias);
}

bool IsPrimaryWeaponClass(int classId)
{
	switch (classId)
	{
		case WEAPONCLASS_PISTOL: return false;
		case WEAPONCLASS_GRENADE: return false;
		case WEAPONCLASS_SUBMACHINEGUN: return true;
		case WEAPONCLASS_SHOTGUN: return true;
		case WEAPONCLASS_MACHINEGUN: return true;
		case WEAPONCLASS_RIFLE: return true;
		case WEAPONCLASS_SNIPERRIFLE: return true;
	}

	return false;
}

bool IsPrimaryWeaponId(int wpnId)
{
	const char *alias = WeaponIDToAlias(wpnId);

	if (alias)
	{
		int classId = AliasToWeaponClass(alias);
		return IsPrimaryWeaponClass(classId);
	}

	return false;
}

bool IsSecondaryWeaponClass(int classId)
{
	switch (classId)
	{
		case WEAPONCLASS_PISTOL: return true;
		case WEAPONCLASS_GRENADE: return false;
		case WEAPONCLASS_SUBMACHINEGUN: return false;
		case WEAPONCLASS_SHOTGUN: return false;
		case WEAPONCLASS_MACHINEGUN: return false;
		case WEAPONCLASS_RIFLE: return false;
		case WEAPONCLASS_SNIPERRIFLE: return false;
	}

	return false;
}

bool IsSecondaryWeaponId(int wpnId)
{
	const char *alias = WeaponIDToAlias(wpnId);

	if (alias)
	{
		int classId = AliasToWeaponClass(alias);
		return IsSecondaryWeaponClass(classId);
	}

	return false;
}

AutoBuyInfoStruct g_autoBuyInfo[] =
{
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE), "galil", "weapon_galil" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE), "ak47", "weapon_ak47" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE), "scout", "weapon_scout" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE), "sg552", "weapon_sg552" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE), "awp", "weapon_awp" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE), "g3sg1", "weapon_g3sg1" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE), "famas", "weapon_famas" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE), "m4a1", "weapon_m4a1" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_RIFLE), "aug", "weapon_aug" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SNIPERRIFLE), "sg550", "weapon_sg550" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL), "glock", "weapon_glock18" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL), "usp", "weapon_usp" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL), "p228", "weapon_p228" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL), "deagle", "weapon_deagle" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL), "elites", "weapon_elite" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_PISTOL), "fn57", "weapon_fiveseven" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SHOTGUN), "m3", "weapon_m3" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SHOTGUN), "xm1014", "weapon_xm1014" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG), "mac10", "weapon_mac10" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG), "tmp", "weapon_tmp" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG), "mp5", "weapon_mp5navy" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG), "ump45", "weapon_ump45" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SMG), "p90", "weapon_p90" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_MACHINEGUN), "m249", "weapon_m249" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_AMMO), "primammo", "primammo" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_SECONDARY | AUTOBUYCLASS_AMMO), "secammo", "secammo" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_ARMOR), "vest", "item_kevlar" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_ARMOR), "vesthelm", "item_assaultsuit" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_GRENADE), "flash", "weapon_flashbang" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_GRENADE), "hegren", "weapon_hegrenade" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_GRENADE), "sgren", "weapon_smokegrenade" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_NIGHTVISION), "nvgs", "nvgs" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_DEFUSER), "defuser", "defuser" },
	{ (AutoBuyClassType)(AUTOBUYCLASS_PRIMARY | AUTOBUYCLASS_SHIELD), "shield", "shield" },
	{ (AutoBuyClassType)0, NULL, NULL }
};

WeaponInfoStruct weaponInfo[] =
{
	{ WEAPON_P228, 600, 50, 13, 13, 52, AMMO_357SIG, "weapon_p228" },
	{ WEAPON_GLOCK, 400, 20, 30, 20, 120, AMMO_9MM, "weapon_glock18" },
	{ WEAPON_GLOCK18, 400, 20, 30, 20, 120, AMMO_9MM, "weapon_glock18" },
	{ WEAPON_SCOUT, 2750, 80, 30, 10, 90, AMMO_762NATO, "weapon_scout" },
	{ WEAPON_XM1014, 3000, 65, 8, 7, 32, AMMO_BUCKSHOT, "weapon_xm1014" },
	{ WEAPON_MAC10, 1400, 25, 12, 30, 100, AMMO_45ACP, "weapon_mac10" },
	{ WEAPON_AUG, 3500, 60, 30, 30, 90, AMMO_556NATO, "weapon_aug" },
	{ WEAPON_ELITE, 800, 20, 30, 30, 120, AMMO_9MM, "weapon_elite" },
	{ WEAPON_FIVESEVEN, 750, 50, 50, 20, 100, AMMO_57MM, "weapon_fiveseven" },
	{ WEAPON_UMP45, 1700, 25, 12, 25, 100, AMMO_45ACP, "weapon_ump45" },
	{ WEAPON_SG550, 4200, 60, 30, 30, 90, AMMO_556NATO, "weapon_sg550" },
	{ WEAPON_GALIL, 2000, 60, 30, 35, 90, AMMO_556NATO, "weapon_galil" },
	{ WEAPON_FAMAS, 2250, 60, 30, 25, 90, AMMO_556NATO, "weapon_famas" },
	{ WEAPON_USP, 500, 25, 12, 12, 100, AMMO_45ACP, "weapon_usp" },
	{ WEAPON_AWP, 4750, 125, 10, 10, 30, AMMO_338MAGNUM, "weapon_awp" },
	{ WEAPON_MP5N, 1500, 20, 30, 30, 120, AMMO_9MM, "weapon_mp5navy" },
	{ WEAPON_M249, 5750, 60, 30, 100, 200, AMMO_556NATOBOX, "weapon_m249" },
	{ WEAPON_M3, 1700, 65, 8, 8, 32, AMMO_BUCKSHOT, "weapon_m3" },
	{ WEAPON_M4A1, 3100, 60, 30, 30, 90, AMMO_556NATO, "weapon_m4a1" },
	{ WEAPON_TMP, 1250, 20, 30, 30, 120, AMMO_9MM, "weapon_tmp" },
	{ WEAPON_G3SG1, 5000, 80, 30, 20, 90, AMMO_762NATO, "weapon_g3sg1" },
	{ WEAPON_DEAGLE, 650, 40, 7, 7, 35, AMMO_50AE, "weapon_deagle" },
	{ WEAPON_SG552, 3500, 60, 30, 30, 90, AMMO_556NATO, "weapon_sg552" },
	{ WEAPON_AK47, 2500, 80, 30, 30, 90, AMMO_762NATO, "weapon_ak47" },
	{ WEAPON_P90, 2350, 50, 50, 50, 100, AMMO_57MM, "weapon_p90" },
	{ WEAPON_SHIELDGUN, 2200, 0, 0, 0, 0, -1, 0 },
	{ 0, 0, 0, 0, 0, 0, -1, 0 }
};

WeaponInfoStruct *GetWeaponInfo(int id)
{
	int i = 0;

	while (1)
	{
		if (weaponInfo[i].id == id)
			break;

		i++;
	}

	return &weaponInfo[i];
}

bool CBasePlayer::ShouldExecuteAutoBuyCommand(const AutoBuyInfoStruct *commandInfo, bool boughtPrimary, bool boughtSecondary)
{
	if (!commandInfo)
		return false;

	if ((boughtPrimary) && ((commandInfo->m_class & AUTOBUYCLASS_PRIMARY) != 0) && ((commandInfo->m_class & AUTOBUYCLASS_AMMO) == 0))
		return false;

	if ((boughtSecondary) && ((commandInfo->m_class & AUTOBUYCLASS_SECONDARY) != 0) && ((commandInfo->m_class & AUTOBUYCLASS_AMMO) == 0))
		return false;

	return true;
}

AutoBuyInfoStruct *CBasePlayer::GetAutoBuyCommandInfo(const char *command)
{
	int i = 0;
	AutoBuyInfoStruct *ret = NULL;
	AutoBuyInfoStruct *temp = &(g_autoBuyInfo[i]);

	while (ret == NULL)
	{
		temp = &(g_autoBuyInfo[i]);

      if (temp && !temp->m_class)
         break;

		++i;

		if (stricmp(temp->m_command, command) == 0)
			ret = temp;
	}

	return ret;
}

void CBasePlayer::PrioritizeAutoBuyString(char *autobuyString, const char *priorityString)
{
	char newString[MAX_AUTOBUY_LENGTH];
	int newStringPos = 0;
	char priorityToken[60];

	if ((priorityString == NULL) || (autobuyString == NULL))
		return;

	const char *priorityChar = priorityString;

	while (*priorityChar != NULL)
	{
		int i = 0;

		while ((*priorityChar != 0) && (*priorityChar != ' '))
		{
			priorityToken[i] = *priorityChar;
			++i;
			++priorityChar;
		}

		priorityToken[i] = 0;

		while (*priorityChar == ' ')
			++priorityChar;

		if (priorityToken[0] == '\0')
			continue;

		char *autoBuyPosition = strstr(autobuyString, priorityToken);

		if (autoBuyPosition != NULL)
		{
			while ((*autoBuyPosition != NULL) && (*autoBuyPosition != ' '))
			{
				newString[newStringPos] = *autoBuyPosition;
				*autoBuyPosition = ' ';
				++newStringPos;
				++autoBuyPosition;
			}

			newString[newStringPos++] = ' ';
		}
	}

	char *autobuyPosition = autobuyString;

	while (*autobuyPosition != NULL)
	{
		while (*autobuyPosition == ' ')
			++autobuyPosition;

		while ((*autobuyPosition != NULL) && (*autobuyPosition != ' '))
		{
			newString[newStringPos] = *autobuyPosition;
			++newStringPos;
			++autobuyPosition;
		}

		newString[newStringPos++] = ' ';
	}

	newString[newStringPos] = 0;
	strcpy(autobuyString, newString);
}

void CBasePlayer::ParseAutoBuyString(const char *string, bool &boughtPrimary, bool &boughtSecondary)
{
	char command[32];
	const char *c = string;

	if (!string || !string[0])
		return;

	while (*c != NULL)
	{
		int i = 0;

		while ((*c != NULL) && (*c != ' ') && (i < 32))
		{
			command[i] = *(c);
			++c;
			++i;
		}

		if (*c == ' ')
			++c;

		command[i] = 0;
		i = 0;

		while (command[i] != 0)
		{
			if (command[i] == ' ')
			{
				command[i] = 0;
				break;
			}

			++i;
		}

		if (command[0]=='\0')
			continue;

		AutoBuyInfoStruct *commandInfo = GetAutoBuyCommandInfo(command);

		if (ShouldExecuteAutoBuyCommand(commandInfo, boughtPrimary, boughtSecondary))
		{
			ClientCommand(commandInfo->m_command);
			PostAutoBuyCommandProcessing(commandInfo, boughtPrimary, boughtSecondary);
		}
	}
}

void CBasePlayer::PostAutoBuyCommandProcessing(const AutoBuyInfoStruct *commandInfo, bool &boughtPrimary, bool &boughtSecondary)
{
	if (!commandInfo)
		return;

	CBasePlayerWeapon *pPrimary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_PRIMARY];
	CBasePlayerWeapon *pSecondary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_SECONDARY];

	if ((pPrimary != NULL) && !stricmp(STRING(pPrimary->pev->classname), commandInfo->m_classname))
		boughtPrimary = true;
	else if ((commandInfo->m_class & AUTOBUYCLASS_SHIELD) && HasShield() != false)
		boughtPrimary = true;
	else if ((pSecondary != NULL) && !stricmp(STRING(pSecondary->pev->classname), commandInfo->m_classname))
		boughtSecondary = true;
}

void CBasePlayer::BuildRebuyStruct(void)
{
	if (m_bIsInRebuy)
		return;

	CBasePlayerWeapon *pPrimary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_PRIMARY];
	CBasePlayerWeapon *pSecondary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_SECONDARY];

	if (pPrimary != NULL)
	{
		m_rebuyStruct.m_primaryWeapon = pPrimary->m_iId;
		m_rebuyStruct.m_primaryAmmo = m_rgAmmo[pPrimary->m_iPrimaryAmmoType];
	}
	else
	{
		m_rebuyStruct.m_primaryAmmo = 0;

		if (HasShield())
			m_rebuyStruct.m_primaryWeapon = WEAPON_SHIELDGUN;
		else
			m_rebuyStruct.m_primaryWeapon = 0;
	}

	if (pSecondary != NULL)
	{
		m_rebuyStruct.m_secondaryWeapon = pSecondary->m_iId;
		m_rebuyStruct.m_secondaryAmmo = m_rgAmmo[pSecondary->m_iPrimaryAmmoType];
	}
	else
	{
		m_rebuyStruct.m_secondaryWeapon = 0;
		m_rebuyStruct.m_secondaryAmmo = 0;
	}

	int heGrenade = GetAmmoIndex("HEGrenade");

	if (heGrenade != -1)
		m_rebuyStruct.m_heGrenade = m_rgAmmo[heGrenade];
	else
		m_rebuyStruct.m_heGrenade = 0;

	int flashbang = GetAmmoIndex("Flashbang");

	if (flashbang != -1)
		m_rebuyStruct.m_flashbang = m_rgAmmo[flashbang];
	else
		m_rebuyStruct.m_flashbang = 0;

	int smokeGrenade = GetAmmoIndex("SmokeGrenade");

	if (smokeGrenade != -1)
		m_rebuyStruct.m_smokeGrenade = m_rgAmmo[smokeGrenade];
	else
		m_rebuyStruct.m_smokeGrenade = 0;

	m_rebuyStruct.m_defuser = m_bHasDefuser;
	m_rebuyStruct.m_nightVision = m_bHasNightVision;
	m_rebuyStruct.m_armor = m_iKevlar;
}

char *MP_COM_GetToken(void);
char *MP_COM_Parse(char *data);
int MP_COM_TokenWaiting(char *buffer);

void CBasePlayer::Rebuy(void)
{
	char *string = m_rebuyString;
	char *token;

	m_bIsInRebuy = true;

	while (1)
	{
		string = MP_COM_Parse(string);
		token = MP_COM_GetToken();

		if (!string)
			break;

		if (!stricmp(token, "primaryWeapon"))
			RebuyPrimaryWeapon();
		else if (!stricmp(token, "primaryAmmo"))
			RebuyPrimaryAmmo();
		else if (!stricmp(token, "secondaryWeapon"))
			RebuySecondaryWeapon();
		else if (!stricmp(token, "secondaryAmmo"))
			RebuySecondaryAmmo();
		else if (!stricmp(token, "hegrenade"))
			RebuyHEGrenade();
		else if (!stricmp(token, "flashbang"))
			RebuyFlashbang();
		else if (!stricmp(token, "smokegrenade"))
			RebuySmokeGrenade();
		else if (!stricmp(token, "defuser"))
			RebuyDefuser();
		else if (!stricmp(token, "nightvision"))
			RebuyNightVision();
		else if (!stricmp(token, "armor"))
			RebuyArmor();
	}

	m_bIsInRebuy = false;
}

void CBasePlayer::RebuyPrimaryWeapon(void)
{
	CBasePlayerWeapon *pPrimary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_PRIMARY];

	if (pPrimary)
		return;

	if (m_rebuyStruct.m_primaryWeapon)
	{
		const char *alias = WeaponIDToAlias(m_rebuyStruct.m_primaryWeapon);

		if (alias)
			ClientCommand(alias);
	}
}

void CBasePlayer::RebuyPrimaryAmmo(void)
{
	CBasePlayerWeapon *pPrimary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_PRIMARY];

	if (!pPrimary)
		return;

	if (m_rebuyStruct.m_primaryAmmo > m_rgAmmo[pPrimary->m_iPrimaryAmmoType])
		ClientCommand("primammo");
}

void CBasePlayer::RebuySecondaryWeapon(void)
{
	CBasePlayerWeapon *pSecondary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_SECONDARY];

	if (pSecondary)
		return;

	if (m_rebuyStruct.m_secondaryWeapon)
	{
		const char *alias = WeaponIDToAlias(m_rebuyStruct.m_secondaryWeapon);

		if (alias)
			ClientCommand(alias);
	}
}

void CBasePlayer::RebuySecondaryAmmo(void)
{
	CBasePlayerWeapon *pSecondary = (CBasePlayerWeapon *)m_rgpPlayerItems[WPNSLOT_SECONDARY];

	if (!pSecondary)
		return;

	if (m_rebuyStruct.m_secondaryAmmo > m_rgAmmo[pSecondary->m_iPrimaryAmmoType])
		ClientCommand("secammo");
}

void CBasePlayer::RebuyHEGrenade(void)
{
	int heGrenade = GetAmmoIndex("HEGrenade");
	if(heGrenade<0)
	{
#ifdef _DEBUG
		UTIL_LogPrintf("%s: %s returned -1\n",__FUNCTION__,GetAmmoIndex);
#endif
		return;
	}
	int numToBuy = max(0, m_rebuyStruct.m_heGrenade - m_rgAmmo[heGrenade]);

	for (int i = 0; i < numToBuy; i++)
		ClientCommand("hegren");
}

void CBasePlayer::RebuyFlashbang(void)
{
	int flashbang = GetAmmoIndex("Flashbang");
	if(flashbang<0)
	{
#ifdef _DEBUG
		UTIL_LogPrintf("%s: %s returned -1\n",__FUNCTION__,GetAmmoIndex);
#endif
		return;
	}
	int numToBuy = max(0, m_rebuyStruct.m_flashbang - m_rgAmmo[flashbang]);

	for (int i = 0; i < numToBuy; i++)
		ClientCommand("flash");
}

void CBasePlayer::RebuySmokeGrenade(void)
{
	int smokeGrenade = GetAmmoIndex("SmokeGrenade");
	if(smokeGrenade<0)
	{
#ifdef _DEBUG
		UTIL_LogPrintf("%s: %s returned -1\n",__FUNCTION__,GetAmmoIndex);
#endif
		return;
	}
	int numToBuy = max(0, m_rebuyStruct.m_smokeGrenade - m_rgAmmo[smokeGrenade]);

	for (int i = 0; i < numToBuy; i++)
		ClientCommand("sgren");
}

void CBasePlayer::RebuyDefuser(void)
{
	if (m_rebuyStruct.m_defuser)
	{
		if (m_bHasDefuser == false)
			ClientCommand("defuser");
	}
}

void CBasePlayer::RebuyNightVision(void)
{
	if (m_rebuyStruct.m_nightVision)
	{
		if (m_bHasNightVision == false)
			ClientCommand("nvgs");
	}
}

void CBasePlayer::RebuyArmor(void)
{
	if (m_rebuyStruct.m_armor)
	{
		if (m_rebuyStruct.m_armor > m_iKevlar)
		{
			if (m_rebuyStruct.m_armor == 1)
				ClientCommand("vest");
			else
				ClientCommand("vesthelm");
		}
	}
}

bool CBasePlayer::IsObservingPlayer(CBasePlayer *pTarget)
{
	if (!pTarget)
		return false;

	if (pev->flags == FL_DORMANT)
		return false;

	if (FNullEnt(pTarget))
		return false;

	if (pev->iuser1 == OBS_IN_EYE && pev->iuser2 == ENTINDEX(pTarget->edict()))
		return true;

	return false;
}

void CBasePlayer::SetObserverAutoDirector(bool bState)
{
	m_bObserverAutoDirector = bState;
}

BOOL CBasePlayer::CanSwitchObserverModes(void)
{
	return m_canSwitchObserverModes;
}

void CBasePlayer::UpdateLocation(bool bForceUpdate)
{
	if (!bForceUpdate && m_flLastUpdateTime >= gpGlobals->time + 2)
		return;

	char *location = "";

	if (!location[0])
		return;

	if (m_lastLocation[0] && !strcmp(location, &m_lastLocation[1]))
		return;

	m_flLastUpdateTime = gpGlobals->time;
	snprintf(m_lastLocation, sizeof(m_lastLocation), "#%s", location);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer *pOther = (CBasePlayer *)UTIL_PlayerByIndex(i);

		if (pOther->m_iTeam == m_iTeam || pOther->m_iTeam == TEAM_SPECTATOR)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgLocation, NULL, pOther->pev);
			WRITE_BYTE(ENTINDEX(edict()));
			WRITE_STRING(m_lastLocation);
			MESSAGE_END();
		}
		else if (bForceUpdate)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgLocation, NULL, pOther->pev);
			WRITE_BYTE(ENTINDEX(edict()));
			WRITE_STRING("");
			MESSAGE_END();
		}
	}
}

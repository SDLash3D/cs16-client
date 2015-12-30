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
#include "hltv.h"
#include "gamerules.h"

//#define C4MADNESS
enum c4_e
{
	C4_IDLE1,
	C4_DRAW,
	C4_DROP,
	C4_ARM
};

LINK_ENTITY_TO_CLASS(weapon_c4, CC4);

void CC4::Spawn(void)
{
	SET_MODEL(ENT(pev), "models/w_backpack.mdl");

	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;

	m_iId = WEAPON_C4;
	m_iDefaultAmmo = C4_DEFAULT_GIVE;
	m_bStartedArming = false;
	m_fArmedTime = 0;

	if (!FStringNull(pev->targetname))
	{
		pev->effects |= EF_NODRAW;
		DROP_TO_FLOOR(ENT(pev));
		return;
	}

	FallInit();
	SetThink(&CBasePlayerItem::FallThink);
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CC4::Precache(void)
{
	PRECACHE_MODEL("models/v_c4.mdl");
	PRECACHE_MODEL("models/w_backpack.mdl");

	PRECACHE_SOUND("weapons/c4_click.wav");
}

int CC4::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "C4";
	p->iMaxAmmo1 = C4_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_C4;
	p->iWeight = C4_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CC4::Deploy(void)
{
	pev->body = 0;
	m_bStartedArming = false;
	m_fArmedTime = 0;

	if (m_pPlayer->HasShield() != false)
	{
		m_bHasShield = true;
		m_pPlayer->pev->gamestate = 1;
	}

	return DefaultDeploy("models/v_c4.mdl", "models/p_c4.mdl", C4_DRAW, "c4", UseDecrement() != FALSE);
}

void CC4::Holster(int skiplocal)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_bStartedArming = false;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_C4);
		DestroyItem();
	}

	if (m_bHasShield == true)
	{
		m_pPlayer->pev->gamestate = 0;
		m_bHasShield = false;
	}
}

void CC4::PrimaryAttack(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	BOOL onBombZone = m_pPlayer->m_signals.GetState() & SIGNAL_BOMB;
	BOOL onGround = m_pPlayer->pev->flags & FL_ONGROUND;

	if (!m_bStartedArming)
	{
		if (!onBombZone)
		{
			ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Plant_At_Bomb_Spot");
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
			return;
		}

		if (!onGround)
		{
			ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Plant_Must_Be_On_Ground");
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
			return;
		}
		g_engfuncs.pfnSetClientMaxspeed(ENT(m_pPlayer->pev), 1);

		m_bStartedArming = true;
		m_bBombPlacedAnimation = false;
		m_fArmedTime = gpGlobals->time + 3;
		SendWeaponAnim(C4_ARM, UseDecrement() != FALSE);	
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		m_pPlayer->SetProgressBarTime(3);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
	}
	else
	{
		if (!onGround || !onBombZone)
		{
			if (onBombZone)
				ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Plant_Must_Be_On_Ground");
			else
				ClientPrint(m_pPlayer->pev, HUD_PRINTCENTER, "#C4_Arming_Cancelled");

			m_bStartedArming = false;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
			m_pPlayer->ResetMaxSpeed();
			m_pPlayer->SetProgressBarTime(0);
			m_pPlayer->SetAnimation(PLAYER_HOLDBOMB);

			if (m_bBombPlacedAnimation == true)
				SendWeaponAnim(C4_DRAW, UseDecrement() != FALSE);
			else
				SendWeaponAnim(C4_IDLE1, UseDecrement() != FALSE);

			return;
		}

		if (gpGlobals->time > m_fArmedTime)
		{
			if (m_bStartedArming == true)
			{
				m_bStartedArming = false;
				m_fArmedTime = 0;
				Broadcast("BOMBPL");
				m_pPlayer->m_bHasC4 = false;

#ifndef CLIENT_WEAPONS
		if (pev->speed != 0 && g_pGameRules)
			g_pGameRules->m_iC4Timer = (int)pev->speed;
#endif
				CGrenade *pGrenade = CGrenade::ShootSatchelCharge(m_pPlayer->pev, m_pPlayer->pev->origin, Vector(0, 0, 0));

				MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
				WRITE_BYTE(9);
				WRITE_BYTE(DRC_CMD_EVENT);
				WRITE_SHORT(ENTINDEX(m_pPlayer->edict()));
				WRITE_SHORT(0);
				WRITE_LONG(11 | DRC_FLAG_FACEPLAYER);
				MESSAGE_END();

#ifndef CLIENT_WEAPONS
				MESSAGE_BEGIN(MSG_ALL, gmsgBombDrop);
				WRITE_COORD(pGrenade->pev->origin.x);
				WRITE_COORD(pGrenade->pev->origin.y);
				WRITE_COORD(pGrenade->pev->origin.z);
				WRITE_BYTE(1);
				MESSAGE_END();
#endif
				UTIL_ClientPrintAll(HUD_PRINTCENTER, "#Bomb_Planted");

				UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Planted_The_Bomb\"\n", STRING(m_pPlayer->pev->netname), GETPLAYERUSERID(m_pPlayer->edict()), GETPLAYERAUTHID(m_pPlayer->edict()));
#ifndef CLIENT_WEAPONS
				g_pGameRules->m_bBombDropped = false;
#endif		
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/c4_plant.wav", VOL_NORM, ATTN_NORM);

				m_pPlayer->pev->body = 0;
				m_pPlayer->ResetMaxSpeed();
				m_pPlayer->SetBombIcon(FALSE);
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

				if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
				{
					RetireWeapon();
					return;
				}
			}
		}
		else
		{
			if (gpGlobals->time >= m_fArmedTime - 0.75)
			{
				if (m_bBombPlacedAnimation == false)
				{
					m_bBombPlacedAnimation = true;
					SendWeaponAnim(C4_DROP, UseDecrement() != FALSE);
					m_pPlayer->SetAnimation(PLAYER_HOLDBOMB);
				}
			}
		}
	}

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
}

void CC4::WeaponIdle(void)
{
	if (m_bStartedArming == true)
	{
		m_bStartedArming = false;
		m_pPlayer->ResetMaxSpeed();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
		m_pPlayer->SetProgressBarTime(0);

		if (m_bBombPlacedAnimation == true)
			SendWeaponAnim(C4_DRAW, UseDecrement() != FALSE);
		else
			SendWeaponAnim(C4_IDLE1, UseDecrement() != FALSE);
	}

	if (m_flTimeWeaponIdle <= UTIL_WeaponTimeBase())
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim(C4_DRAW, UseDecrement() != FALSE);
		SendWeaponAnim(C4_IDLE1, UseDecrement() != FALSE);
	}
}

void CC4::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "detonatedelay"))
	{
		pev->speed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "detonatetarget"))
	{
		pev->noise1 = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "defusetarget"))
	{
		pev->target = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CC4::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_pPlayer)
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(1);

	if (pPlayer)
	{
		edict_t *target = pPlayer->m_pentCurBombTarget;
		pPlayer->m_pentCurBombTarget = NULL;

#ifndef CLIENT_WEAPONS
		if (pev->speed != 0 && g_pGameRules)
			g_pGameRules->m_iC4Timer = (int)pev->speed;
#endif

		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/c4_plant.wav", VOL_NORM, ATTN_NORM);
		CGrenade::ShootSatchelCharge(m_pPlayer->pev, m_pPlayer->pev->origin, Vector(0, 0, 0));
		CGrenade *pGrenade = NULL;

		while ((pGrenade = (CGrenade *)UTIL_FindEntityByClassname(pGrenade, "grenade")) != NULL)
		{
			if (pGrenade->m_bIsC4 && pGrenade->m_flNextFreq == gpGlobals->time)
			{
				pGrenade->pev->target = pev->target;
				pGrenade->pev->noise1 = pev->noise1;
				break;
			}
		}

		pPlayer->m_pentCurBombTarget = target;
		SUB_Remove();
	}
}

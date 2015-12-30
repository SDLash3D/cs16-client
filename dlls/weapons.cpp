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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"
#include "hltv.h"

#ifdef LINUX
#include <ctype.h>
#endif
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

DLL_GLOBAL short g_sModelIndexLaser;
DLL_GLOBAL const char *g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL short g_sModelIndexLaserDot;
DLL_GLOBAL short g_sModelIndexFireball;
DLL_GLOBAL short g_sModelIndexFireball2;
DLL_GLOBAL short g_sModelIndexFireball3;
DLL_GLOBAL short g_sModelIndexFireball4;
DLL_GLOBAL short g_sModelIndexSmoke;
DLL_GLOBAL short g_sModelIndexSmokePuff;
DLL_GLOBAL short g_sModelIndexWExplosion;
DLL_GLOBAL short g_sModelIndexBubbles;
DLL_GLOBAL short g_sModelIndexBloodDrop;
DLL_GLOBAL short g_sModelIndexBloodSpray;
DLL_GLOBAL short g_sModelIndexRadio;
DLL_GLOBAL short g_sModelIndexCTGhost;
DLL_GLOBAL short g_sModelIndexTGhost;
DLL_GLOBAL short g_sModelIndexC4Glow;

ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];
AmmoInfo CBasePlayerItem::AmmoInfoArray[MAX_AMMO_SLOTS];

extern int gEvilImpulse101;
extern int gmsgCurWeapon;

MULTIDAMAGE gMultiDamage;

int MaxAmmoCarry(int iszName)
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (CBasePlayerItem::ItemInfoArray[i].pszAmmo1 && !strcmp(STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo1))
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo1;

		if (CBasePlayerItem::ItemInfoArray[i].pszAmmo2 && !strcmp(STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo2))
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo2;
	}

	ALERT(at_console, "MaxAmmoCarry() doesn't recognize '%s'!\n", STRING(iszName));
	return -1;
}

void ClearMultiDamage(void)
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount = 0;
	gMultiDamage.type = 0;
}

void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker)
{
	if (!gMultiDamage.pEntity)
		return;

	gMultiDamage.pEntity->TakeDamage(pevInflictor, pevAttacker, gMultiDamage.amount, gMultiDamage.type);
}

void AddMultiDamage(entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType)
{
	if (!pEntity)
		return;

	gMultiDamage.type |= bitsDamageType;

	if (pEntity != gMultiDamage.pEntity)
	{
		ApplyMultiDamage(pevInflictor,pevInflictor);
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.amount = 0;
	}

	gMultiDamage.amount += flDamage;
}

void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage)
{
	UTIL_BloodDrips(vecSpot, g_vecAttackDir, bloodColor, flDamage);
}

void DecalGunshot(TraceResult *pTrace, int iBulletType, bool ClientOnly, entvars_t *pShooter, bool bHitMetal)
{

}

extern int gmsgBrass;

void EjectBrass(const Vector &vecOrigin, const Vector &vecLeft, const Vector &vecVelocity, float rotation, int model, int soundtype, int entityIndex)
{
	MESSAGE_BEGIN(MSG_PVS, gmsgBrass, vecOrigin);
	WRITE_BYTE(TE_MODEL);
	WRITE_COORD(vecOrigin.x);
	WRITE_COORD(vecOrigin.y);
	WRITE_COORD(vecOrigin.z);
	WRITE_COORD(vecLeft.x);
	WRITE_COORD(vecLeft.y);
	WRITE_COORD(vecLeft.z);
	WRITE_COORD(vecVelocity.x);
	WRITE_COORD(vecVelocity.y);
	WRITE_COORD(vecVelocity.z);
	WRITE_ANGLE(rotation);
	WRITE_SHORT(model);
	WRITE_BYTE(soundtype);
	WRITE_BYTE(entityIndex);
	MESSAGE_END();
}

void EjectBrass2(const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype, entvars_t *pev)
{
	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pev);
	WRITE_BYTE(TE_MODEL);
	WRITE_COORD(vecOrigin.x);
	WRITE_COORD(vecOrigin.y);
	WRITE_COORD(vecOrigin.z);
	WRITE_COORD(vecVelocity.x);
	WRITE_COORD(vecVelocity.y);
	WRITE_COORD(vecVelocity.z);
	WRITE_ANGLE(rotation);
	WRITE_SHORT(model);
	WRITE_BYTE(0);
	WRITE_BYTE(5);
	MESSAGE_END();
}

int giAmmoIndex = 0;

void AddAmmoNameToAmmoRegistry(const char *szAmmoName)
{
	for (int i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (!CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if (!stricmp(CBasePlayerItem::AmmoInfoArray[i].pszName, szAmmoName))
			return;
	}

	giAmmoIndex++;
	ASSERT(giAmmoIndex < MAX_AMMO_SLOTS);

	if (giAmmoIndex >= MAX_AMMO_SLOTS)
		giAmmoIndex = 0;

	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].pszName = szAmmoName;
	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;
}

void UTIL_PrecacheOtherWeapon(const char *szClassname)
{
	edict_t *pent = CREATE_NAMED_ENTITY(MAKE_STRING(szClassname));

	if (FNullEnt(pent))
	{
		ALERT(at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n");
		return;
	}

	CBaseEntity *pEntity = CBaseEntity::Instance(VARS(pent));

	if (pEntity)
	{
		ItemInfo II;
		pEntity->Precache();
		memset(&II, 0, sizeof II);

		if (((CBasePlayerItem *)pEntity)->GetItemInfo(&II))
		{
			CBasePlayerItem::ItemInfoArray[II.iId] = II;

			if (II.pszAmmo1 && *II.pszAmmo1)
				AddAmmoNameToAmmoRegistry(II.pszAmmo1);

			if (II.pszAmmo2 && *II.pszAmmo2)
				AddAmmoNameToAmmoRegistry(II.pszAmmo2);

			memset(&II, 0, sizeof II);
		}
	}

	REMOVE_ENTITY(pent);
}

void W_Precache(void)
{
	memset(CBasePlayerItem::ItemInfoArray, 0, sizeof(CBasePlayerItem::ItemInfoArray));
	memset(CBasePlayerItem::AmmoInfoArray, 0, sizeof(CBasePlayerItem::AmmoInfoArray));
	giAmmoIndex = 0;

	UTIL_PrecacheOther("item_suit");
	UTIL_PrecacheOther("item_battery");
	UTIL_PrecacheOther("item_antidote");
	UTIL_PrecacheOther("item_security");
	UTIL_PrecacheOther("item_longjump");
	UTIL_PrecacheOther("item_kevlar");
	UTIL_PrecacheOther("item_assaultsuit");
	UTIL_PrecacheOther("item_thighpack");

	UTIL_PrecacheOtherWeapon("weapon_awp");
	UTIL_PrecacheOther("ammo_338magnum");
	UTIL_PrecacheOtherWeapon("weapon_g3sg1");
	UTIL_PrecacheOtherWeapon("weapon_ak47");
	UTIL_PrecacheOtherWeapon("weapon_scout");
	UTIL_PrecacheOther("ammo_762nato");
	UTIL_PrecacheOtherWeapon("weapon_m249");
	UTIL_PrecacheOther("ammo_556natobox");
	UTIL_PrecacheOtherWeapon("weapon_m4a1");
	UTIL_PrecacheOtherWeapon("weapon_sg552");
	UTIL_PrecacheOtherWeapon("weapon_aug");
	UTIL_PrecacheOtherWeapon("weapon_sg550");
	UTIL_PrecacheOther("ammo_556nato");
	UTIL_PrecacheOtherWeapon("weapon_m3");
	UTIL_PrecacheOtherWeapon("weapon_xm1014");
	UTIL_PrecacheOther("ammo_buckshot");
	UTIL_PrecacheOtherWeapon("weapon_usp");
	UTIL_PrecacheOtherWeapon("weapon_mac10");
	UTIL_PrecacheOtherWeapon("weapon_ump45");
	UTIL_PrecacheOther("ammo_45acp");
	UTIL_PrecacheOtherWeapon("weapon_fiveseven");
	UTIL_PrecacheOtherWeapon("weapon_p90");
	UTIL_PrecacheOther("ammo_57mm");
	UTIL_PrecacheOtherWeapon("weapon_deagle");
	UTIL_PrecacheOther("ammo_50ae");
	UTIL_PrecacheOtherWeapon("weapon_p228");
	UTIL_PrecacheOther("ammo_357sig");
	UTIL_PrecacheOtherWeapon("weapon_knife");
	UTIL_PrecacheOtherWeapon("weapon_glock18");
	UTIL_PrecacheOtherWeapon("weapon_mp5navy");
	UTIL_PrecacheOtherWeapon("weapon_tmp");
	UTIL_PrecacheOtherWeapon("weapon_elite");
	UTIL_PrecacheOther("ammo_9mm");
	UTIL_PrecacheOtherWeapon("weapon_flashbang");
	UTIL_PrecacheOtherWeapon("weapon_hegrenade");
	UTIL_PrecacheOtherWeapon("weapon_smokegrenade");
	UTIL_PrecacheOtherWeapon("weapon_c4");
	UTIL_PrecacheOtherWeapon("weapon_galil");
	UTIL_PrecacheOtherWeapon("weapon_famas");

	if (g_pGameRules->IsDeathmatch())
		UTIL_PrecacheOther("weaponbox");

	g_sModelIndexFireball = PRECACHE_MODEL("sprites/zerogxplode.spr");
	g_sModelIndexWExplosion = PRECACHE_MODEL("sprites/WXplo1.spr");
	g_sModelIndexSmoke = PRECACHE_MODEL("sprites/steam1.spr");
	g_sModelIndexBubbles = PRECACHE_MODEL("sprites/bubble.spr");
	g_sModelIndexBloodSpray = PRECACHE_MODEL("sprites/bloodspray.spr");
	g_sModelIndexBloodDrop = PRECACHE_MODEL("sprites/blood.spr");
	g_sModelIndexSmokePuff = PRECACHE_MODEL("sprites/smokepuff.spr");
	g_sModelIndexFireball2 = PRECACHE_MODEL("sprites/eexplo.spr");
	g_sModelIndexFireball3 = PRECACHE_MODEL("sprites/fexplo.spr");
	g_sModelIndexFireball4 = PRECACHE_MODEL("sprites/fexplo1.spr");
	g_sModelIndexRadio = PRECACHE_MODEL("sprites/radio.spr");
	g_sModelIndexCTGhost = PRECACHE_MODEL("sprites/b-tele1.spr");
	g_sModelIndexTGhost = PRECACHE_MODEL("sprites/c-tele1.spr");
	g_sModelIndexC4Glow = PRECACHE_MODEL("sprites/ledglow.spr");
	g_sModelIndexLaser = PRECACHE_MODEL((char *)g_pModelNameLaser);
	g_sModelIndexLaserDot = PRECACHE_MODEL("sprites/laserdot.spr");

	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_MODEL("sprites/explode1.spr");
	PRECACHE_SOUND("weapons/debris1.wav");
	PRECACHE_SOUND("weapons/debris2.wav");
	PRECACHE_SOUND("weapons/debris3.wav");
	PRECACHE_SOUND("weapons/grenade_hit1.wav");
	PRECACHE_SOUND("weapons/grenade_hit2.wav");
	PRECACHE_SOUND("weapons/grenade_hit3.wav");
	PRECACHE_SOUND("weapons/bullet_hit1.wav");
	PRECACHE_SOUND("weapons/bullet_hit2.wav");
	PRECACHE_SOUND("items/weapondrop1.wav");
	PRECACHE_SOUND("weapons/generic_reload.wav");
}

TYPEDESCRIPTION CBasePlayerItem::m_SaveData[] =
{
	DEFINE_FIELD(CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayerItem, m_pNext, FIELD_CLASSPTR),
	DEFINE_FIELD(CBasePlayerItem, m_iId, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CBasePlayerItem, CBaseAnimating);

TYPEDESCRIPTION CBasePlayerWeapon::m_SaveData[] =
{
#ifdef CLIENT_WEAPONS
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_FLOAT),
	DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_FLOAT),
#else
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME),
	DEFINE_FIELD(CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME),
	DEFINE_FIELD(CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME),
#endif
	DEFINE_FIELD(CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iClip, FIELD_INTEGER),
	DEFINE_FIELD(CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CBasePlayerWeapon, CBasePlayerItem);

void CBasePlayerItem::SetObjectCollisionBox(void)
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16);
}

void CBasePlayerItem::FallInit(void)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	SetTouch(&CBasePlayerItem::DefaultTouch);
	SetThink(&CBasePlayerItem::FallThink);

	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayerItem::FallThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		if (!FNullEnt(pev->owner))
		{
			int pitch = 95 + RANDOM_LONG(0, 29);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", VOL_NORM, ATTN_NORM, 0, pitch);
		}

		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize();
	}
}

void CBasePlayerItem::Materialize(void)
{
	if (pev->effects & EF_NODRAW)
	{
		if (g_pGameRules->IsMultiplayer())
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", VOL_NORM, ATTN_NORM, 0, 150);

		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CBasePlayerItem::DefaultTouch);

	if (g_pGameRules->IsMultiplayer())
	{
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 1;
	}
	else
		SetThink(NULL);
}

void CBasePlayerItem::AttemptToMaterialize(void)
{
	float time = g_pGameRules->FlWeaponTryRespawn(this);

	if (!time)
	{
		Materialize();
		return;
	}

	pev->nextthink = gpGlobals->time + time;
}

void CBasePlayerItem::CheckRespawn(void)
{
	switch (g_pGameRules->WeaponShouldRespawn(this))
	{
		case GR_WEAPON_RESPAWN_YES: return;
		case GR_WEAPON_RESPAWN_NO: return;
	}
}

CBaseEntity *CBasePlayerItem::Respawn(void)
{
	CBaseEntity *pNewWeapon = CBaseEntity::Create((char *)STRING(pev->classname), g_pGameRules->VecWeaponRespawnSpot(this), pev->angles, pev->owner);

	if (pNewWeapon)
	{
		pNewWeapon->pev->effects |= EF_NODRAW;
		pNewWeapon->SetTouch(NULL);
		pNewWeapon->SetThink(&CBasePlayerItem::AttemptToMaterialize);

		DROP_TO_FLOOR(ENT(pev));
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime(this);
	}
	else
		ALERT(at_console, "Respawn failed to create %s!\n", STRING(pev->classname));

	return pNewWeapon;
}

void CBasePlayerItem::DefaultTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (pPlayer->m_bIsVIP == true && m_iId != WEAPON_USP && m_iId != WEAPON_GLOCK18 && m_iId != WEAPON_P228 && m_iId != WEAPON_DEAGLE && m_iId != WEAPON_KNIFE)
		return;

	if (!g_pGameRules->CanHavePlayerItem(pPlayer, this))
	{
		if (gEvilImpulse101)
			UTIL_Remove(this);

		return;
	}

	if (pOther->AddPlayerItem(this))
	{
		AttachToPlayer(pPlayer);
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);
	}

	SUB_UseTargets(pOther, USE_TOGGLE, 0);
}

BOOL CanAttack(float attack_time, float curtime, BOOL isPredicted)
{
#ifdef CLIENT_WEAPONS
	if (!isPredicted)
#else
	if (1)
#endif
		return (attack_time <= curtime) ? TRUE : FALSE;
	else
		return (attack_time <= 0) ? TRUE : FALSE;
}

void CBasePlayerWeapon::SetPlayerShieldAnim(void)
{
	if (m_pPlayer->HasShield() == true)
	{
		if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
			strcpy(m_pPlayer->m_szAnimExtention, "shield");
		else
			strcpy(m_pPlayer->m_szAnimExtention, "shieldgun");
	}
}

void CBasePlayerWeapon::ResetPlayerShieldAnim(void)
{
	if (m_pPlayer->HasShield() == true)
	{
		if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
			strcpy(m_pPlayer->m_szAnimExtention, "shieldgun");
	}
}

void CBasePlayerWeapon::EjectBrassLate(void)
{
	int soundType;
	Vector vecUp, vecRight, vecShellVelocity;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	vecUp = RANDOM_FLOAT (100, 150) * gpGlobals->v_up;
	vecRight = RANDOM_FLOAT (50, 70) * gpGlobals->v_right;

	vecShellVelocity = (m_pPlayer->pev->velocity + vecRight + vecUp) + gpGlobals->v_forward * 25;
	soundType = (m_iId == WEAPON_XM1014 || m_iId == WEAPON_M3) ? 2 : 1;

	EjectBrass (pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -9 + gpGlobals->v_forward * 16, gpGlobals->v_right * -9, vecShellVelocity, pev->angles.y, m_iShellId, soundType, m_pPlayer->entindex ());
}

bool CBasePlayerWeapon::ShieldSecondaryFire(int up_anim, int down_anim)
{
	if (m_pPlayer->HasShield() == false)
		return false;

	if (m_iWeaponState & WPNSTATE_SHIELD_DRAWN)
	{
		m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim(down_anim, UseDecrement() != FALSE);
		strcpy(m_pPlayer->m_szAnimExtention, "shieldgun");
		m_fMaxSpeed = 250;
		m_pPlayer->m_bShieldDrawn = false;
	}
	else
	{
		m_iWeaponState |= WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim(up_anim, UseDecrement() != FALSE);
		strcpy(m_pPlayer->m_szAnimExtention, "shielded");
		m_fMaxSpeed = 180;
		m_pPlayer->m_bShieldDrawn = true;
	}

	m_pPlayer->UpdateShieldCrosshair((m_iWeaponState & WPNSTATE_SHIELD_DRAWN) == 0 );
	m_pPlayer->ResetMaxSpeed();

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.4;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
	return true;
}

void CBasePlayerWeapon::KickBack(float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change)
{
	float flFront, flSide;

	if (m_iShotsFired == 1)
	{
		flFront = up_base;
		flSide = lateral_base;
	}
	else
	{
		flFront = m_iShotsFired * up_modifier + up_base;
		flSide = m_iShotsFired * lateral_modifier + lateral_base;
	}

	m_pPlayer->pev->punchangle.x -= flFront;

	if (m_pPlayer->pev->punchangle.x < -up_max)
		m_pPlayer->pev->punchangle.x = -up_max;

	if (m_iDirection == 1)
	{
		m_pPlayer->pev->punchangle.y += flSide;

		if (m_pPlayer->pev->punchangle.y > lateral_max)
			m_pPlayer->pev->punchangle.y = lateral_max;
	}
	else
	{
		m_pPlayer->pev->punchangle.y -= flSide;

		if (m_pPlayer->pev->punchangle.y < -lateral_max)
			m_pPlayer->pev->punchangle.y = -lateral_max;
	}

	if (!RANDOM_LONG(0, direction_change))
		m_iDirection = !m_iDirection;
}

void CBasePlayerWeapon::FireRemaining(int &shotsFired, float &shootTime, BOOL isGlock18)
{
	m_iClip--;

	if (m_iClip < 0)
	{
		m_iClip = 0;
		shotsFired = 3;
		shootTime = 0;
		return;
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir;

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	if (isGlock18)
	{
		vecDir = m_pPlayer->FireBullets3(m_pPlayer->GetGunPosition(), gpGlobals->v_forward, 0.05, 8192, 1, BULLET_PLAYER_9MM, 18, 0.9, m_pPlayer->pev, TRUE, m_pPlayer->random_seed);
		PLAYBACK_EVENT_FULL(flags, ENT(m_pPlayer->pev), m_usFireGlock18, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000), (int)(m_pPlayer->pev->punchangle.y * 10000), m_iClip != 0, FALSE);
		m_pPlayer->ammo_9mm--;
	}
	else
	{
		vecDir = m_pPlayer->FireBullets3(m_pPlayer->GetGunPosition(), gpGlobals->v_forward, m_fBurstSpread, 8192, 2, BULLET_PLAYER_556MM, 30, 0.96, m_pPlayer->pev, TRUE, m_pPlayer->random_seed);
		PLAYBACK_EVENT_FULL(flags, ENT(m_pPlayer->pev), m_usFireFamas, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000000), (int)(m_pPlayer->pev->punchangle.y * 10000000), m_iClip != 0, FALSE);
		m_pPlayer->ammo_556nato--;
	}

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
	shotsFired++;

	if (shotsFired == 3)
		shootTime = 0;
	else
		shootTime = gpGlobals->time + 0.1;
}

bool CBasePlayerWeapon::HasSecondaryAttack(void)
{
	if (m_pPlayer->HasShield() == false)
	{
		if (m_iId == WEAPON_AK47 || m_iId == WEAPON_XM1014 || m_iId == WEAPON_MAC10 || m_iId == WEAPON_ELITE || m_iId == WEAPON_FIVESEVEN || m_iId == WEAPON_MP5N || m_iId == WEAPON_M249 || m_iId == WEAPON_M3 || m_iId == WEAPON_TMP || m_iId == WEAPON_DEAGLE || m_iId == WEAPON_P228 || m_iId == WEAPON_P90 || m_iId == WEAPON_C4 || m_iId == WEAPON_GALIL)
			return false;
	}

	return true;
}

void CBasePlayerWeapon::ItemPostFrame(void)
{
	int button = m_pPlayer->pev->button;

	if (!HasSecondaryAttack())
		button &= ~IN_ATTACK2;

	if (m_flGlock18Shoot != 0)
		FireRemaining(m_iGlock18ShotsFired, m_flGlock18Shoot, TRUE);
	else if (gpGlobals->time > m_flFamasShoot && m_flFamasShoot != 0)
		FireRemaining(m_iFamasShotsFired, m_flFamasShoot, FALSE);

	if (m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if (m_pPlayer->m_bResumeZoom == true)
		{
			m_pPlayer->pev->fov = m_pPlayer->m_iFOV = m_pPlayer->m_iLastZoom;

			if (m_pPlayer->m_iFOV == m_pPlayer->m_iLastZoom)
				m_pPlayer->m_bResumeZoom = false;
		}
	}

	if (m_pPlayer->m_flEjectBrass != 0)
	{
		if (gpGlobals->time >= m_pPlayer->m_flEjectBrass)
		{
			m_pPlayer->m_flEjectBrass = 0;
			EjectBrassLate();
		}
	}

	if (m_pPlayer->HasShield() != false)
	{
		if (m_fInReload && m_pPlayer->pev->button & IN_ATTACK2)
		{
			SecondaryAttack();
			m_pPlayer->pev->button &= ~IN_ATTACK2;
			m_fInReload = FALSE;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase();
		}
	}

	if ((m_fInReload) && m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase())
	{
		int j = min(iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;
		m_pPlayer->TabulateAmmo();
		m_fInReload = FALSE;
	}

	if ((button & IN_ATTACK2) && m_flNextSecondaryAttack <= UTIL_WeaponTimeBase())
	{
		if (pszAmmo2() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()])
			m_fFireOnEmpty = TRUE;

		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && m_flNextPrimaryAttack <= UTIL_WeaponTimeBase())
	{
		if ((!m_iClip && pszAmmo1()) || (iMaxClip() == WEAPON_NOCLIP && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]))
			m_fFireOnEmpty = TRUE;

		m_pPlayer->TabulateAmmo();

		if ((m_pPlayer->m_bCanShoot == true && g_pGameRules->IsMultiplayer() && !g_pGameRules->IsFreezePeriod() && !m_pPlayer->m_bIsDefusing) || !g_pGameRules->IsMultiplayer())
			PrimaryAttack();
	}
	else if (m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload)
	{
		if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		{
			if (m_flFamasShoot == 0 && m_flGlock18Shoot == 0)
			{
				if (!(m_iWeaponState & WPNSTATE_SHIELD_DRAWN))
					Reload();
			}
		}
	}
	else if (!(button & (IN_ATTACK | IN_ATTACK2)))
	{
		if (m_bDelayFire == true)
		{
			m_bDelayFire = false;

			if (m_iShotsFired > 15)
				m_iShotsFired = 15;

			m_flDecreaseShotsFired = gpGlobals->time + 0.4;
		}

		m_fFireOnEmpty = FALSE;

		if (m_iId != WEAPON_USP && m_iId != WEAPON_GLOCK18 && m_iId != WEAPON_P228 && m_iId != WEAPON_DEAGLE && m_iId != WEAPON_ELITE && m_iId != WEAPON_FIVESEVEN)
		{
			if (m_iShotsFired > 0)
			{
				if (gpGlobals->time > m_flDecreaseShotsFired)
				{
					m_iShotsFired--;
					m_flDecreaseShotsFired = gpGlobals->time + 0.0225;
				}
			}
		}
		else
			m_iShotsFired = 0;

		if (!IsUseable() && m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		{
		}
		else
		{
			if (!m_iClip && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
			{
				if (m_flFamasShoot == 0 && m_flGlock18Shoot == 0)
				{
					Reload();
					return;
				}
			}
		}

		WeaponIdle();
		return;
	}

	if (ShouldWeaponIdle())
		WeaponIdle();
}

void CBasePlayerItem::DestroyItem(void)
{
	if (m_pPlayer)
		m_pPlayer->RemovePlayerItem(this);

	Kill();
}

int CBasePlayerItem::AddToPlayer(CBasePlayer *pPlayer)
{
	m_pPlayer = pPlayer;
	return TRUE;
}

void CBasePlayerItem::Drop(void)
{
	SetTouch(NULL);
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayerItem::Kill(void)
{
	SetTouch(NULL);
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayerItem::Holster(int skiplocal)
{
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerItem::AttachToPlayer(CBasePlayer *pPlayer)
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW;
	pev->modelindex = 0;
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();
	pev->nextthink = gpGlobals->time + 0.1;

	SetTouch(NULL);
}

int CBasePlayerWeapon::AddDuplicate(CBasePlayerItem *pOriginal)
{
	if (m_iDefaultAmmo)
		return ExtractAmmo((CBasePlayerWeapon *)pOriginal);
	else
		return ExtractClipAmmo((CBasePlayerWeapon *)pOriginal);
}

int CBasePlayerWeapon::AddToPlayer(CBasePlayer *pPlayer)
{
	int bResult = CBasePlayerItem::AddToPlayer(pPlayer);
	pPlayer->pev->weapons |= (1 << m_iId);

	if (!m_iPrimaryAmmoType)
	{
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex(pszAmmo1());
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex(pszAmmo2());
	}

	if (bResult)
		bResult = AddWeapon();

	if (bResult)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
	}

	return bResult;
}

int CBasePlayerWeapon::UpdateClientData(CBasePlayer *pPlayer)
{
	BOOL bSend = FALSE;
	int state = 0;

	if (pPlayer->m_pActiveItem == this)
	{
		if (pPlayer->m_fOnTarget)
			state = WEAPON_IS_ONTARGET;
		else
			state = 1;
	}

	if (!pPlayer->m_fWeapon)
		bSend = TRUE;

	if (this == pPlayer->m_pActiveItem || this == pPlayer->m_pClientActiveItem)
	{
		if (pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem)
			bSend = TRUE;
	}

	if (m_iClip != m_iClientClip || state != m_iClientWeaponState || pPlayer->m_iFOV != pPlayer->m_iClientFOV)
		bSend = TRUE;

	if (bSend)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev);
		WRITE_BYTE(state);
		WRITE_BYTE(m_iId);
		WRITE_BYTE(m_iClip);
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = TRUE;
	}

	if (m_pNext)
		m_pNext->UpdateClientData(pPlayer);

	return 1;
}

void CBasePlayerWeapon::SendWeaponAnim(int iAnim, int skiplocal)
{
	m_pPlayer->pev->weaponanim = iAnim;

#ifdef CLIENT_WEAPONS
	if (skiplocal && ENGINE_CANSKIP(ENT(m_pPlayer->pev)))
		return;
#endif

	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev);
	WRITE_BYTE(iAnim);
	WRITE_BYTE(pev->body);
	MESSAGE_END();
}

BOOL CBasePlayerWeapon::AddPrimaryAmmo(int iCount, char *szName, int iMaxClip, int iMaxCarry)
{
	int iIdAmmo;

	if (iMaxClip < 1)
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMaxCarry);
	}
	else if (!m_iClip)
	{
		int i = min(m_iClip + iCount, iMaxClip) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo(iCount - i, szName, iMaxCarry);
	}
	else
		iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMaxCarry);

	if (iIdAmmo > 0)
	{
		m_iPrimaryAmmoType = iIdAmmo;

		if (m_pPlayer->HasPlayerItem(this))
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}

BOOL CBasePlayerWeapon::AddSecondaryAmmo(int iCount, char *szName, int iMax)
{
	int iIdAmmo = m_pPlayer->GiveAmmo(iCount, szName, iMax);

	if (iIdAmmo > 0)
	{
		m_iSecondaryAmmoType = iIdAmmo;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", VOL_NORM, ATTN_NORM);
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}

BOOL CBasePlayerWeapon::IsUseable(void)
{
	if (m_iClip <= 0)
	{
		if (m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0 && iMaxAmmo1() != -1)
			return FALSE;
	}

	return TRUE;
}

BOOL CBasePlayerWeapon::CanDeploy(void)
{
	return TRUE;
}

BOOL CBasePlayerWeapon::DefaultDeploy(char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int skiplocal)
{
	if (!CanDeploy())
		return FALSE;

	m_pPlayer->TabulateAmmo();
	m_pPlayer->pev->viewmodel = MAKE_STRING(szViewModel);
	m_pPlayer->pev->weaponmodel = MAKE_STRING(szWeaponModel);
	model_name = m_pPlayer->pev->weaponmodel;
	strcpy(m_pPlayer->m_szAnimExtention, szAnimExt);
	SendWeaponAnim(iAnim, skiplocal);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.75;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	m_flDecreaseShotsFired = gpGlobals->time;
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 90;
	m_pPlayer->m_bResumeZoom = false;
	m_pPlayer->m_iLastZoom = 90;
	return TRUE;
}

void CBasePlayerWeapon::ReloadSound(void)
{
	CBasePlayer *pPlayer = NULL;

	while ((pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname(pPlayer, "player")) != NULL)
	{
		if (pPlayer->pev->flags == FL_DORMANT)
			break;

		if (pPlayer == m_pPlayer)
			continue;

		float flDistance = m_pPlayer->pev->origin.Length() - pPlayer->pev->origin.Length();

		if (flDistance <= 512)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgReloadSound, NULL, ENT(m_pPlayer->pev));
			WRITE_BYTE((1.0 - (flDistance / 512.0)) * 255.0);

			const char *classname = STRING(pev->classname);

			if (!strcmp(classname, "weapon_m3") || !strcmp(classname, "weapon_xm1014"))
				WRITE_BYTE(0);
			else
				WRITE_BYTE(1);

			MESSAGE_END();
		}
	}
}

BOOL CBasePlayerWeapon::DefaultReload(int iClipSize, int iAnim, float fDelay, int body)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

	if (!j)
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	ReloadSound();
	SendWeaponAnim(iAnim, UseDecrement() ? 1 : 0);

	m_fInReload = TRUE;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + fDelay + 0.5;
	return TRUE;
}

BOOL CBasePlayerWeapon::PlayEmptySound(void)
{
	if (m_iId == WEAPON_USP || m_iId == WEAPON_GLOCK18 || m_iId == WEAPON_P228 || m_iId == WEAPON_DEAGLE || m_iId == WEAPON_ELITE || m_iId == WEAPON_FIVESEVEN)
	{
		if (m_iPlayEmptySound)
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/dryfire_pistol.wav", 0.8, ATTN_NORM);
			return FALSE;
		}
	}
	else
	{
		if (m_iPlayEmptySound)
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/dryfire_rifle.wav", 0.8, ATTN_NORM);
			return FALSE;
		}
	}

	return FALSE;
}

void CBasePlayerWeapon::ResetEmptySound(void)
{
	m_iPlayEmptySound = 1;
}

int CBasePlayerWeapon::PrimaryAmmoIndex(void)
{
	return m_iPrimaryAmmoType;
}

int CBasePlayerWeapon::SecondaryAmmoIndex(void)
{
	return -1;
}

void CBasePlayerWeapon::Holster(int skiplocal)
{
	m_fInReload = FALSE;
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerAmmo::Spawn(void)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CBasePlayerAmmo::DefaultTouch);

	if (g_pGameRules->IsMultiplayer())
	{
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 2;
	}
}

CBaseEntity *CBasePlayerAmmo::Respawn(void)
{
	pev->effects |= EF_NODRAW;
	SetTouch(NULL);

	UTIL_SetOrigin(pev, g_pGameRules->VecAmmoRespawnSpot(this));

	SetThink(&CBasePlayerAmmo::Materialize);
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime(this);
	return this;
}

void CBasePlayerAmmo::Materialize(void)
{
	if (pev->effects & EF_NODRAW)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", VOL_NORM, ATTN_NORM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch(&CBasePlayerAmmo::DefaultTouch);
}

void CBasePlayerAmmo::DefaultTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	if (AddAmmo(pOther))
	{
		if (g_pGameRules->AmmoShouldRespawn(this) != GR_AMMO_RESPAWN_YES)
		{
			SetTouch(NULL);
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		else
			Respawn();
	}
	else if (gEvilImpulse101)
	{
		SetTouch(NULL);
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

int CBasePlayerWeapon::ExtractAmmo(CBasePlayerWeapon *pWeapon)
{
	int iReturn = 0;

	if (pszAmmo1())
	{
		iReturn = pWeapon->AddPrimaryAmmo(m_iDefaultAmmo, (char *)pszAmmo1(), iMaxClip(), iMaxAmmo1());
		m_iDefaultAmmo = 0;
	}

	if (pszAmmo2())
		iReturn = AddSecondaryAmmo(0, (char *)pszAmmo2(), iMaxAmmo2());

	return iReturn;
}

BOOL CBasePlayerWeapon::ExtractClipAmmo(CBasePlayerWeapon *pWeapon)
{
	int iAmmo;

	if (m_iClip == WEAPON_NOCLIP)
		iAmmo = 0;
	else
		iAmmo = m_iClip;

	return pWeapon->m_pPlayer->GiveAmmo(iAmmo, (char *)pszAmmo1(), iMaxAmmo1());
}

void CBasePlayerWeapon::RetireWeapon(void)
{
	m_pPlayer->pev->viewmodel = NULL;
	m_pPlayer->pev->weaponmodel = NULL;
	g_pGameRules->GetNextBestWeapon(m_pPlayer, this);
}

LINK_ENTITY_TO_CLASS(weaponbox, CWeaponBox);

TYPEDESCRIPTION CWeaponBox::m_SaveData[] =
{
	DEFINE_ARRAY(CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS),
	DEFINE_ARRAY(CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS),
	DEFINE_ARRAY(CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES),
	DEFINE_FIELD(CWeaponBox, m_cAmmoTypes, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CWeaponBox, CBaseEntity);

void CWeaponBox::Precache(void)
{
	PRECACHE_MODEL("models/w_weaponbox.mdl");
}

void CWeaponBox::KeyValue(KeyValueData *pkvd)
{
	if (m_cAmmoTypes < MAX_AMMO_SLOTS)
	{
		PackAmmo(ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue));
		m_cAmmoTypes++;
		pkvd->fHandled = TRUE;
	}
	else
		ALERT(at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_SLOTS);
}

void CWeaponBox::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	SET_MODEL(ENT(pev), "models/w_weaponbox.mdl");
}

void CWeaponBox::Kill(void)
{
	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		CBasePlayerItem *pWeapon = m_rgpPlayerItems[i];

		while (pWeapon)
		{
			pWeapon->SetThink(&CBaseEntity::SUB_Remove);
			pWeapon->pev->nextthink = gpGlobals->time + 0.1;
			pWeapon = pWeapon->m_pNext;
		}
	}

	UTIL_Remove(this);
}

extern int gmsgBombPickup;

void CWeaponBox::Touch(CBaseEntity *pOther)
{
	if (!(pev->flags & FL_ONGROUND))
		return;

	if (!pOther->IsPlayer())
		return;

	if (!pOther->IsAlive())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (pPlayer->m_bIsVIP == true || pPlayer->m_bShieldDrawn == true)
		return;

	bool bCanAmmo = true;
	bool bAddItem = false;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			CBasePlayerItem *pItem = m_rgpPlayerItems[i];

			while (pItem)
			{
				if (FClassnameIs(pItem->pev, "weapon_c4"))
				{
					if (pPlayer->m_iTeam != TEAM_TERRORIST || pPlayer->pev->deadflag != DEAD_NO)
						return;

					if (pPlayer->m_bShowHints && !(pPlayer->m_flDisplayHistory & DHF_BOMB_RETRIEVED))
					{
						pPlayer->m_flDisplayHistory |= DHF_BOMB_RETRIEVED;
						pPlayer->HintMessage("#Hint_you_have_the_bomb");
					}
					else
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Got_bomb");

					UTIL_LogPrintf("\"%s<%i><%s><TERRORIST>\" triggered \"Got_The_Bomb\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));

					MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
					WRITE_BYTE(9);
					WRITE_BYTE(DRC_CMD_EVENT);
					WRITE_SHORT(ENTINDEX(pPlayer->edict()));
					WRITE_SHORT(ENTINDEX(edict()));
					WRITE_LONG(5);
					MESSAGE_END();

					pPlayer->m_bHasC4 = true;
					pPlayer->SetBombIcon(FALSE);
					pPlayer->pev->body = 1;

					CBaseEntity *pEntity = NULL;

					while (pEntity = UTIL_FindEntityByClassname(pEntity, "player"))
					{
						if (FNullEnt(pEntity->edict()))
							break;

						if (!pEntity->IsPlayer())
							continue;

						if (pEntity->pev->flags == FL_DORMANT)
							continue;

						CBasePlayer *pOther = GetClassPtr((CBasePlayer *)pEntity->pev);

						if (pOther->pev->deadflag == DEAD_NO && pOther->m_iTeam == TEAM_TERRORIST)
						{
							if (pOther != pPlayer)
								ClientPrint(pOther->pev, HUD_PRINTCENTER, "#Game_bomb_pickup", &gpGlobals->pStringBase[pPlayer->pev->netname]);

							MESSAGE_BEGIN(MSG_ONE, gmsgBombPickup, NULL, pOther->pev);
							MESSAGE_END();
						}
					}
				}

				if (i >= WPNSLOT_PRIMARY && i <= WPNSLOT_SECONDARY && pPlayer->m_rgpPlayerItems[i] != NULL)
				{
				}
				else if (i == WPNSLOT_GRENADE)
				{
					if (m_rgpPlayerItems[i]&&m_rgpPlayerItems[i]->IsWeapon())
					{
						CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon *)m_rgpPlayerItems[i];
						const char *pszGrenadeName = NULL;
						int iMaxGrenades = 1;

						switch (pWeapon->m_iId)
						{
							case WEAPON_HEGRENADE: pszGrenadeName = "weapon_hegrenade"; break;
							case WEAPON_SMOKEGRENADE: pszGrenadeName = "weapon_smokegrenade"; break;
							case WEAPON_FLASHBANG: pszGrenadeName = "weapon_flashbang"; iMaxGrenades = 2; break;
						}

						if (m_rgAmmo[pWeapon->m_iPrimaryAmmoType] < iMaxGrenades && pszGrenadeName)
						{
							pPlayer->GiveNamedItem(pszGrenadeName);
							pItem = m_rgpPlayerItems[i] = m_rgpPlayerItems[i]->m_pNext;
							continue;
						}
					}
				}
				else if (pPlayer->HasShield() != false && i == WPNSLOT_PRIMARY)
				{
				}
				else
				{
					bAddItem = true;

					if (pPlayer->AddPlayerItem(pItem))
						pItem->AttachToPlayer(pPlayer);

					pItem = m_rgpPlayerItems[i] = m_rgpPlayerItems[i]->m_pNext;
					continue;
				}

				bCanAmmo = false;
				pItem = m_rgpPlayerItems[i]->m_pNext;
			}
		}
	}

	if (bCanAmmo)
	{
		for (int i = 0; i < MAX_AMMO_SLOTS; i++)
		{
			if (!FStringNull(m_rgiszAmmo[i]))
			{
				pPlayer->GiveAmmo(m_rgAmmo[i], (char *)STRING(m_rgiszAmmo[i]), MaxAmmoCarry(m_rgiszAmmo[i]));
				m_rgiszAmmo[i] = NULL;
				m_rgAmmo[i] = 0;
			}
		}
	}

	if (bAddItem)
		EMIT_SOUND(ENT(pOther->pev), CHAN_ITEM, "items/gunpickup2.wav", VOL_NORM, ATTN_NORM);

	if (bCanAmmo)
	{
		SetTouch(NULL);
		UTIL_Remove(this);
	}
}

BOOL CWeaponBox::PackWeapon(CBasePlayerItem *pWeapon)
{
	if (HasWeapon(pWeapon))
		return FALSE;

	if (pWeapon->m_pPlayer)
	{
		if (pWeapon->m_pPlayer->m_pActiveItem == pWeapon)
			pWeapon->Holster();

		if (!pWeapon->m_pPlayer->RemovePlayerItem(pWeapon))
			return FALSE;
	}

	int iWeaponSlot = pWeapon->iItemSlot();

	if (m_rgpPlayerItems[iWeaponSlot])
	{
		pWeapon->m_pNext = m_rgpPlayerItems[iWeaponSlot];
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
	}
	else
	{
		m_rgpPlayerItems[iWeaponSlot] = pWeapon;
		pWeapon->m_pNext = NULL;
	}

	pWeapon->pev->spawnflags |= SF_NORESPAWN;
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = NULL;
	pWeapon->pev->owner = ENT(pev);
	pWeapon->SetThink(NULL);
	pWeapon->SetTouch(NULL);
	pWeapon->m_pPlayer = NULL;
	return TRUE;
}

BOOL CWeaponBox::PackAmmo(int iszName, int iCount)
{
	if (FStringNull(iszName))
	{
		ALERT(at_console, "NULL String in PackAmmo!\n");
		return FALSE;
	}

	int iMaxCarry = MaxAmmoCarry(iszName);

	if (iMaxCarry != -1 && iCount > 0)
	{
		GiveAmmo(iCount, (char *)STRING(iszName), iMaxCarry);
		return TRUE;
	}

	return FALSE;
}

int CWeaponBox::GiveAmmo(int iCount, char *szName, int iMax, int *pIndex)
{
	int i = 1;

	for (; i < MAX_AMMO_SLOTS && !FStringNull(m_rgiszAmmo[i]); i++)
	{
		if (!stricmp(szName, STRING(m_rgiszAmmo[i])))
		{
			if (pIndex)
				*pIndex = i;

			int iAdd = min(iCount, iMax - m_rgAmmo[i]);

			if (iCount == 0 || iAdd > 0)
			{
				m_rgAmmo[i] += iAdd;
				return i;
			}

			return -1;
		}
	}

	if (i < MAX_AMMO_SLOTS)
	{
		if (pIndex)
			*pIndex = i;

		m_rgiszAmmo[i] = MAKE_STRING(szName);
		m_rgAmmo[i] = iCount;
		return i;
	}

	ALERT(at_console, "out of named ammo slots\n");
	return i;
}

BOOL CWeaponBox::HasWeapon(CBasePlayerItem *pCheckItem)
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

BOOL CWeaponBox::IsEmpty(void)
{
	int i;

	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
			return FALSE;
	}

	for (i = 0; i < MAX_AMMO_SLOTS; i++)
	{
		if (m_rgiszAmmo[i])
			return FALSE;
	}

	return TRUE;
}

void CWeaponBox::SetObjectCollisionBox(void)
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16);
}

void CArmoury::ArmouryTouch(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (pPlayer->m_bIsVIP == true)
		return;

	if (m_iCount > 0 && m_iItem <= ARMOURY_M249)
	{
		if (pPlayer->m_bHasPrimary == true)
			return;

		m_iCount--;

		switch (m_iItem)
		{
			case ARMOURY_MP5NAVY:
			{
				pPlayer->GiveNamedItem("weapon_mp5navy");
				pPlayer->GiveAmmo(60, "9mm", _9MM_MAX_CARRY);
				break;
			}

			case ARMOURY_TMP:
			{
				pPlayer->GiveNamedItem("weapon_tmp");
				pPlayer->GiveAmmo(60, "9mm", _9MM_MAX_CARRY);
				break;
			}

			case ARMOURY_P90:
			{
				pPlayer->GiveNamedItem("weapon_p90");
				pPlayer->GiveAmmo(50, "57mm", _57MM_MAX_CARRY);
				break;
			}

			case ARMOURY_MAC10:
			{
				pPlayer->GiveNamedItem("weapon_mac10");
				pPlayer->GiveAmmo(60, "45acp", _45ACP_MAX_CARRY);
				break;
			}

			case ARMOURY_AK47:
			{
				pPlayer->GiveNamedItem("weapon_ak47");
				pPlayer->GiveAmmo(60, "762Nato", _762NATO_MAX_CARRY);
				break;
			}

			case ARMOURY_SG552:
			{
				pPlayer->GiveNamedItem("weapon_sg552");
				pPlayer->GiveAmmo(60, "556Nato", _556NATO_MAX_CARRY);
				break;
			}

			case ARMOURY_M4A1:
			{
				pPlayer->GiveNamedItem("weapon_m4a1");
				pPlayer->GiveAmmo(60, "556Nato", _556NATO_MAX_CARRY);
				break;
			}

			case ARMOURY_AUG:
			{
				pPlayer->GiveNamedItem("weapon_aug");
				pPlayer->GiveAmmo(60, "556Nato", _556NATO_MAX_CARRY);
				break;
			}

			case ARMOURY_SCOUT:
			{
				pPlayer->GiveNamedItem("weapon_scout");
				pPlayer->GiveAmmo(30, "762Nato", _762NATO_MAX_CARRY);
				break;
			}

			case ARMOURY_G3SG1:
			{
				pPlayer->GiveNamedItem("weapon_g3sg1");
				pPlayer->GiveAmmo(30, "762Nato", _762NATO_MAX_CARRY);
				break;
			}

			case ARMOURY_AWP:
			{
				pPlayer->GiveNamedItem("weapon_awp");
				pPlayer->GiveAmmo(20, "338Magnum", _338MAGNUM_MAX_CARRY);
				break;
			}

			case ARMOURY_M3:
			{
				pPlayer->GiveNamedItem("weapon_m3");
				pPlayer->GiveAmmo(24, "buckshot", BUCKSHOT_MAX_CARRY);
				break;
			}

			case ARMOURY_XM1014:
			{
				pPlayer->GiveNamedItem("weapon_xm1014");
				pPlayer->GiveAmmo(24, "buckshot", BUCKSHOT_MAX_CARRY);
				break;
			}

			case ARMOURY_M249:
			{
				pPlayer->GiveNamedItem("weapon_m249");
				pPlayer->GiveAmmo(60, "556NatoBox", _556NATOBOX_MAX_CARRY);
				break;
			}
		}
	}
	else if (m_iCount > 0 && m_iItem >= ARMOURY_FLASHBANG)
	{
		switch (m_iItem)
		{
			case ARMOURY_FLASHBANG:
			{
				if (pPlayer->AmmoInventory(pPlayer->GetAmmoIndex("Flashbang")) >= 2)
					return;

				pPlayer->GiveNamedItem("weapon_flashbang");
				m_iCount--;
				break;
			}

			case ARMOURY_HEGRENADE:
			{
				if (pPlayer->AmmoInventory(pPlayer->GetAmmoIndex("HEGrenade")) >= 1)
					return;

				pPlayer->GiveNamedItem("weapon_hegrenade");
				m_iCount--;
				break;
			}

			case ARMOURY_KEVLAR:
			{
				if (pPlayer->m_iKevlar == 1)
					return;

				pPlayer->GiveNamedItem("item_kevlar");
				m_iCount--;
			}

			case ARMOURY_ASSAULT:
			{
				if (pPlayer->m_iKevlar == 2)
					return;

				pPlayer->GiveNamedItem("item_assaultsuit");
				m_iCount--;
			}

			case ARMOURY_SMOKEGRENADE:
			{
				if (pPlayer->AmmoInventory(pPlayer->GetAmmoIndex("SmokeGrenade")) >= 1)
					return;

				pPlayer->GiveNamedItem("weapon_smokegrenade");
				m_iCount--;
				break;
			}
		}
	}

	if (!m_iCount)
		pev->effects |= EF_NODRAW;
}

void CArmoury::KeyValue(KeyValueData *pkvd)
{
	if (!strcmp(pkvd->szKeyName, "item"))
	{
		m_iItem = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (!strcmp(pkvd->szKeyName, "count"))
	{
		m_iCount = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(armoury_entity, CArmoury);

void CArmoury::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CArmoury::ArmouryTouch);

	switch (m_iItem)
	{
		case ARMOURY_MP5NAVY: SET_MODEL(ENT(pev), "models/w_mp5.mdl"); break;
		case ARMOURY_TMP: SET_MODEL(ENT(pev), "models/w_tmp.mdl"); break;
		case ARMOURY_P90: SET_MODEL(ENT(pev), "models/w_p90.mdl"); break;
		case ARMOURY_MAC10: SET_MODEL(ENT(pev), "models/w_mac10.mdl"); break;
		case ARMOURY_AK47: SET_MODEL(ENT(pev), "models/w_ak47.mdl"); break;
		case ARMOURY_SG552: SET_MODEL(ENT(pev), "models/w_sg552.mdl"); break;
		case ARMOURY_M4A1: SET_MODEL(ENT(pev), "models/w_m4a1.mdl"); break;
		case ARMOURY_AUG: SET_MODEL(ENT(pev), "models/w_aug.mdl"); break;
		case ARMOURY_SCOUT: SET_MODEL(ENT(pev), "models/w_scout.mdl"); break;
		case ARMOURY_G3SG1: SET_MODEL(ENT(pev), "models/w_g3sg1.mdl"); break;
		case ARMOURY_AWP: SET_MODEL(ENT(pev), "models/w_awp.mdl"); break;
		case ARMOURY_M3: SET_MODEL(ENT(pev), "models/w_m3.mdl"); break;
		case ARMOURY_XM1014: SET_MODEL(ENT(pev), "models/w_xm1014.mdl"); break;
		case ARMOURY_M249: SET_MODEL(ENT(pev), "models/w_m249.mdl"); break;
		case ARMOURY_FLASHBANG: SET_MODEL(ENT(pev), "models/w_flashbang.mdl"); break;
		case ARMOURY_HEGRENADE: SET_MODEL(ENT(pev), "models/w_hegrenade.mdl"); break;
		case ARMOURY_KEVLAR: SET_MODEL(ENT(pev), "models/w_kevlar.mdl"); break;
		case ARMOURY_ASSAULT: SET_MODEL(ENT(pev), "models/w_assault.mdl"); break;
		case ARMOURY_SMOKEGRENADE: SET_MODEL(ENT(pev), "models/w_smokegrenade.mdl"); break;
		default: PRECACHE_MODEL("models/w_kevlar.mdl"); break;
	}

	if (m_iCount <= 0)
		m_iCount = 1;

	m_bAlreadyCounted = false;
	m_iInitialCount = m_iCount;
}

void CArmoury::Restart(void)
{
	if (m_iItem == ARMOURY_FLASHBANG || m_iItem == ARMOURY_HEGRENADE)
	{
		if (!m_bAlreadyCounted)
		{
			m_bAlreadyCounted = true;
			g_pGameRules->m_iTotalGrenadeCount += m_iInitialCount;
			m_iCount = m_iInitialCount;
			pev->effects &= ~EF_NODRAW;
			return;
		}

		m_iCount = (float)(m_iInitialCount / g_pGameRules->m_iTotalGrenadeCount) * g_pGameRules->m_iNumTerrorist * 1.75;
	}
	else if (m_iItem == ARMOURY_KEVLAR || m_iItem == ARMOURY_ASSAULT)
	{
		if (!m_bAlreadyCounted)
		{
			m_bAlreadyCounted = true;
			g_pGameRules->m_iTotalArmourCount += m_iInitialCount;
			m_iCount = m_iInitialCount;
			pev->effects &= ~EF_NODRAW;
			return;
		}

		m_iCount = (float)(m_iInitialCount / g_pGameRules->m_iTotalArmourCount) * g_pGameRules->m_iNumTerrorist;
	}
	else
	{
		if (!m_bAlreadyCounted)
		{
			m_bAlreadyCounted = true;
			g_pGameRules->m_iTotalGunCount += m_iInitialCount;
			m_iCount = m_iInitialCount;
			pev->effects &= ~EF_NODRAW;
			return;
		}

		m_iCount = (float)(m_iInitialCount / g_pGameRules->m_iTotalGunCount) * g_pGameRules->m_iNumTerrorist * 0.85;
	}

	if (m_iCount < 1)
		m_iCount = 1;

	pev->effects &= ~EF_NODRAW;
}

void CArmoury::Precache(void)
{
	switch (m_iItem)
	{
		case ARMOURY_MP5NAVY: PRECACHE_MODEL("models/w_mp5.mdl"); break;
		case ARMOURY_TMP: PRECACHE_MODEL("models/w_tmp.mdl"); break;
		case ARMOURY_P90: PRECACHE_MODEL("models/w_p90.mdl"); break;
		case ARMOURY_MAC10: PRECACHE_MODEL("models/w_mac10.mdl"); break;
		case ARMOURY_AK47: PRECACHE_MODEL("models/w_ak47.mdl"); break;
		case ARMOURY_SG552: PRECACHE_MODEL("models/w_sg552.mdl"); break;
		case ARMOURY_M4A1: PRECACHE_MODEL("models/w_m4a1.mdl"); break;
		case ARMOURY_AUG: PRECACHE_MODEL("models/w_aug.mdl"); break;
		case ARMOURY_SCOUT: PRECACHE_MODEL("models/w_scout.mdl"); break;
		case ARMOURY_G3SG1: PRECACHE_MODEL("models/w_g3sg1.mdl"); break;
		case ARMOURY_AWP: PRECACHE_MODEL("models/w_awp.mdl"); break;
		case ARMOURY_M3: PRECACHE_MODEL("models/w_m3.mdl"); break;
		case ARMOURY_XM1014: PRECACHE_MODEL("models/w_xm1014.mdl"); break;
		case ARMOURY_M249: PRECACHE_MODEL("models/w_m249.mdl"); break;
		case ARMOURY_FLASHBANG: PRECACHE_MODEL("models/w_flashbang.mdl"); break;
		case ARMOURY_HEGRENADE: PRECACHE_MODEL("models/w_hegrenade.mdl"); break;
		case ARMOURY_KEVLAR: PRECACHE_MODEL("models/w_kevlar.mdl"); break;
		case ARMOURY_ASSAULT: PRECACHE_MODEL("models/w_assault.mdl"); break;
		case ARMOURY_SMOKEGRENADE: PRECACHE_MODEL("models/w_smokegrenade.mdl"); break;
		default: PRECACHE_MODEL("models/w_kevlar.mdl"); break;
	}
}

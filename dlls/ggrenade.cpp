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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "player.h"
#include "gamerules.h"
#include "hltv.h"

extern int gmsgBombPickup;

LINK_ENTITY_TO_CLASS(grenade, CGrenade);

#define SF_DETONATE 0x0001

void CGrenade::Explode(Vector vecSrc, Vector vecAim)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), ignore_monsters, ENT(pev), &tr);
	Explode(&tr, DMG_BLAST);
}

void CGrenade::Explode(TraceResult *pTrace, int bitsDamageType)
{
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	if (pTrace->flFraction != 1)
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);

	int iContents = UTIL_PointContents(pev->origin);
	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3);
	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL;
	RadiusFlash(pev->origin, pev, pevOwner, 4);

	if (RANDOM_FLOAT(0, 1) < 0.5)
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	else
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);

	float flRndSound = RANDOM_FLOAT(0, 1);

	switch (RANDOM_LONG(0, 1))
	{
		case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/flashbang-1.wav", 0.55, ATTN_NORM); break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/flashbang-2.wav", 0.55, ATTN_NORM); break;
	}

	pev->effects |= EF_NODRAW;
	SetThink(&CGrenade::Smoke);
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0, 3);

		for (int i = 0; i < sparkCount; i++)
			Create("spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL);
	}
}

void CGrenade::Explode2(TraceResult *pTrace, int bitsDamageType)
{
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	UTIL_ScreenShake(pTrace->vecEndPos, 25, 150, 1, 3000);

	g_pGameRules->m_bTargetBombed = true;

	if (g_pGameRules->IsCareer())
	{
	}

	m_bJustBlew = true;
	g_pGameRules->CheckWinConditions();

	if (pTrace->flFraction != 1)
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);

	int iContents = UTIL_PointContents(pev->origin);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z - 10);
	WRITE_SHORT(g_sModelIndexFireball3);
	WRITE_BYTE((pev->dmg - 275) * 0.6);
	WRITE_BYTE(150);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-512, 512));
	WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-512, 512));
	WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-10, 10));
	WRITE_SHORT(g_sModelIndexFireball2);
	WRITE_BYTE((pev->dmg - 275) * 0.6);
	WRITE_BYTE(150);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-512, 512));
	WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-512, 512));
	WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-10, 10));
	WRITE_SHORT(g_sModelIndexFireball3);
	WRITE_BYTE((pev->dmg - 275) * 0.6);
	WRITE_BYTE(150);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-512, 512));
	WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-512, 512));
	WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-10, 10));
	WRITE_SHORT(g_sModelIndexFireball);
	WRITE_BYTE((pev->dmg - 275) * 0.6);
	WRITE_BYTE(17);
	MESSAGE_END();

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/c4_explode1.wav", VOL_NORM, 0.25);
	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3);
	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL;
	RadiusDamage(pev, pevOwner, g_pGameRules->m_flBombRadius, CLASS_NONE, bitsDamageType);

	if (g_pGameRules->IsCareer())
	{
	}

	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);
	WRITE_BYTE(DRC_CMD_EVENT);
	WRITE_SHORT(ENTINDEX(edict()));
	WRITE_SHORT(0);
	WRITE_LONG(15 | DRC_FLAG_FINAL);
	MESSAGE_END();

	if (RANDOM_FLOAT(0, 1) < 0.5)
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	else
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);

	float flRndSound = RANDOM_FLOAT(0, 1);

	switch (RANDOM_LONG(0, 2))
	{
		case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM); break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM); break;
	}

	pev->effects |= EF_NODRAW;
	SetThink(&CGrenade::Smoke2);
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.85;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0, 3);

		for (int i = 0; i < sparkCount; i++)
			Create("spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL);
	}
}

void CGrenade::Explode3(TraceResult *pTrace, int bitsDamageType)
{
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	if (pTrace->flFraction != 1)
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 20);
	WRITE_SHORT(g_sModelIndexFireball3);
	WRITE_BYTE(25);
	WRITE_BYTE(30);
	WRITE_BYTE(0);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-64, 64));
	WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-64, 64));
	WRITE_COORD(pev->origin.z + RANDOM_FLOAT(30, 35));
	WRITE_SHORT(g_sModelIndexFireball2);
	WRITE_BYTE(30);
	WRITE_BYTE(30);
	WRITE_BYTE(0);
	MESSAGE_END();

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3);
	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL;
	RadiusDamage(pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType);

	if (RANDOM_FLOAT(0, 1) < 0.5)
		UTIL_DecalTrace(pTrace, DECAL_SCORCH1);
	else
		UTIL_DecalTrace(pTrace, DECAL_SCORCH2);

	float flRndSound = RANDOM_FLOAT(0, 1);

	switch (RANDOM_LONG(0, 2))
	{
		case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM); break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM); break;
	}

	pev->effects |= EF_NODRAW;
	SetThink(&CGrenade::Smoke3_C);
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.55;
	int sparkCount = RANDOM_LONG(0, 3);

	for (int i = 0; i < sparkCount; i++)
		Create("spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL);
}

void CGrenade::Smoke3_C(void)
{
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z - 5);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(35 + RANDOM_FLOAT(0, 10));
		WRITE_BYTE(5);
		MESSAGE_END();
	}
	else
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);

	UTIL_Remove(this);
}

void CGrenade::Smoke3_B(void)
{
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-128, 128));
		WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-128, 128));
		WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-10, 10));
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(15 + RANDOM_FLOAT(0, 10));
		WRITE_BYTE(10);
		MESSAGE_END();
	}
	else
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);

	pev->nextthink = gpGlobals->time + 0.15;
	SetThink(&CGrenade::Smoke3_A);
}

void CGrenade::Smoke3_A(void)
{
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-128, 128));
		WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-128, 128));
		WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-10, 10));
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(15 + RANDOM_FLOAT(0, 10));
		WRITE_BYTE(12);
		MESSAGE_END();
	}
	else
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);

	UTIL_Remove(this);
}

void CGrenade::Smoke2(void)
{
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(150);
		WRITE_BYTE(8);
		MESSAGE_END();
	}
	else
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);

	UTIL_Remove(this);
}

void CGrenade::Smoke(void)
{
	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(25);
		WRITE_BYTE(6);
		MESSAGE_END();
	}
	else
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);

	UTIL_Remove(this);
}

void CGrenade::SG_Smoke(void)
{
	Vector vecDir;
	float flSmokeInterval[2];
	int iMaxSmokePuffs;

	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		UTIL_MakeVectors(pev->angles);
		vecDir = gpGlobals->v_forward * RANDOM_FLOAT(3, 8);
		iMaxSmokePuffs = (int)(RANDOM_FLOAT(1.5, 3.5) * 100);
		flSmokeInterval[0] = vecDir.x * cos((float)m_angle * (180 / M_PI)) - vecDir.y * sin((float)m_angle * (180 / M_PI));
		flSmokeInterval[1] = vecDir.x * sin((float)m_angle * (180 / M_PI)) + vecDir.y * cos((float)m_angle * (180 / M_PI));
		m_angle = (m_angle + 30) % 360;
		PLAYBACK_EVENT_FULL(0, NULL, m_usEvent, 0, pev->origin, m_vSmokeDetonate, flSmokeInterval[0], flSmokeInterval[1], iMaxSmokePuffs, 4, m_bLightSmoke, 6);
	}
	else
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);

	if (m_SGSmoke <= 20)
	{
		pev->nextthink = gpGlobals->time + 1;
		SetThink(&CGrenade::SG_Smoke);
		m_SGSmoke++;
	}
	else
	{
		pev->effects |= EF_NODRAW;
		UTIL_Remove(this);
	}
}

void CGrenade::Killed(entvars_t *pevAttacker, int iGib)
{
	Detonate();
}

void CGrenade::DetonateUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CGrenade::Detonate);
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate(void)
{
	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.3);
	SetThink(&CGrenade::Detonate);
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate(void)
{
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);
	Explode(&tr, DMG_BLAST);
}

void CGrenade::SG_Detonate(void)
{
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sg_explode.wav", VOL_NORM, ATTN_NORM);
	edict_t *pentFind = NULL;

	while ((pentFind = FIND_ENTITY_BY_STRING(pentFind, "classname", "grenade")) != NULL)
	{
		if (FNullEnt(pentFind))
			break;

		CBaseEntity *pEnt = CBaseEntity::Instance(pentFind);

		if (pEnt)
		{
			float fDistance = (pEnt->pev->origin - pev->origin).Length();

			if (fDistance != 0 && fDistance <= 250 && gpGlobals->time < pEnt->pev->dmgtime)
				m_bLightSmoke = true;
		}
	}

	m_bDetonated = true;
	PLAYBACK_EVENT_FULL(0, NULL, m_usEvent, 0, pev->origin, (float *)&g_vecZero, 0, 0, 0, 1, m_bLightSmoke, FALSE);
	m_vSmokeDetonate = pev->origin;
	pev->velocity = Vector(RANDOM_FLOAT(-175.0, 175.0), RANDOM_FLOAT(-175.0, 175.0), RANDOM_FLOAT(250.0, 350.0));
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CGrenade::SG_Smoke);
}

void CGrenade::Detonate2(void)
{
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);
	Explode2(&tr, DMG_BLAST);
}

void CGrenade::Detonate3(void)
{
	TraceResult tr;
	Vector vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);
	Explode3(&tr, DMG_BLAST);
}

void CGrenade::ExplodeTouch(CBaseEntity *pOther)
{
	pev->enemy = pOther->edict();

	TraceResult tr;
	Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr);
	Explode(&tr, DMG_BLAST);
}

void CGrenade::DangerSoundThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2);
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != 0)
		pev->velocity = pev->velocity * 0.5;
}

void CGrenade::BounceTouch(CBaseEntity *pOther)
{
	if (pOther->edict() == pev->owner)
		return;

	if (FClassnameIs(pOther->pev, "func_breakable") && pOther->pev->rendermode)
		pev->velocity = pev->velocity * -2;

	Vector testVelocity = pev->velocity;
	testVelocity.z *= 0.7;

	if (!m_fRegisteredSound && testVelocity.Length() <= 60)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, pev->dmg * 2.5, 0.3);
		m_fRegisteredSound = TRUE;
	}

	if (pev->flags & FL_ONGROUND)
	{
		pev->velocity = pev->velocity * 0.8;
		pev->sequence = RANDOM_LONG(1, 1);
	}
	else
	{
		if (m_iBounceCount < 5)
			BounceSound();

		if (m_iBounceCount >= 10)
		{
			pev->groundentity = INDEXENT(0);
			pev->flags |= FL_ONGROUND;
			pev->velocity = g_vecZero;
		}

		m_iBounceCount++;
	}

	pev->framerate = pev->velocity.Length() / 200;

	if (pev->framerate > 1)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;
}

void CGrenade::SlideTouch(CBaseEntity *pOther)
{
	if (pOther->edict() == pev->owner)
		return;

	if (pev->flags & FL_ONGROUND)
		pev->velocity = pev->velocity * 0.95;
	else
		BounceSound();
}

void CGrenade::BounceSound(void)
{
	if (pev->dmg > 50)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/he_bounce-1.wav", 0.25, ATTN_NORM);
		return;
	}

	switch (RANDOM_LONG(0, 2))
	{
		case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM); break;
		case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM); break;
		case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM); break;
	}
}

void CGrenade::TumbleThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);

	if (pev->dmgtime <= gpGlobals->time)
	{
		if (pev->dmg <= 40)
			SetThink(&CGrenade::Detonate);
		else
			SetThink(&CGrenade::Detonate3);
	}

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}

void CGrenade::SG_TumbleThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->flags & FL_ONGROUND)
		pev->velocity = pev->velocity * 0.95;

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);

	if (pev->dmgtime <= gpGlobals->time)
	{
		if (pev->flags & FL_ONGROUND)
			SetThink(&CGrenade::SG_Detonate);
	}

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}

void CGrenade::Spawn(void)
{
	m_iBounceCount = 0;

	pev->movetype = MOVETYPE_BOUNCE;

	if (pev->classname)
		RemoveEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	pev->classname = MAKE_STRING("grenade");
	AddEntityHashValue(pev, STRING(pev->classname), CLASSNAME);

	m_bIsC4 = false;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 30;
	m_fRegisteredSound = FALSE;
}

CGrenade *CGrenade::ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CGrenade *pGrenade = GetClassPtr((CGrenade *)NULL);
	pGrenade->Spawn();

	pGrenade->pev->gravity = 0.5;
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetThink(&CGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;
	pGrenade->pev->avelocity.x = RANDOM_FLOAT(-100, -500);
	pGrenade->SetTouch(&CGrenade::ExplodeTouch);

	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	pGrenade->m_bJustBlew = true;
	return pGrenade;
}

CGrenade *CGrenade::ShootTimed2(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam, unsigned short usEvent)
{
	CGrenade *pGrenade = GetClassPtr((CGrenade *)NULL);
	pGrenade->Spawn();

	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->m_usEvent = usEvent;
	pGrenade->SetTouch(&CGrenade::BounceTouch);
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1;
	pGrenade->m_bJustBlew = true;
	pGrenade->pev->gravity = 0.55;
	pGrenade->pev->friction = 0.7;
	pGrenade->m_iTeam = iTeam;

	SET_MODEL(ENT(pGrenade->pev), "models/w_hegrenade.mdl");
	pGrenade->pev->dmg = 100;
	return pGrenade;
}

CGrenade *CGrenade::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CGrenade *pGrenade = GetClassPtr((CGrenade *)NULL);
	pGrenade->Spawn();

	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->SetTouch(&CGrenade::BounceTouch);
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;

	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1;
	pGrenade->m_bJustBlew = true;
	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	SET_MODEL(ENT(pGrenade->pev), "models/w_flashbang.mdl");
	pGrenade->pev->dmg = 35;
	return pGrenade;
}

void CGrenade::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_bIsC4)
		return;

	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pActivator->pev);

	if (pPlayer->m_iTeam != TEAM_CT)
		return;

	if (m_bStartDefuse)
	{
		m_fNextDefuse = gpGlobals->time + 0.5;
		return;
	}

	g_engfuncs.pfnSetClientMaxspeed(pPlayer->edict(), 1);

	if (g_pGameRules->IsCareer())
	{
	}

	if (pPlayer->m_bHasDefuser == true)
	{
		UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Begin_Bomb_Defuse_With_Kit\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));
		ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Defusing_Bomb_With_Defuse_Kit");
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "weapons/c4_disarm.wav", VOL_NORM, ATTN_NORM);

		pPlayer->m_bIsDefusing = true;
		m_pBombDefuser = pCaller;
		m_bStartDefuse = true;
		m_flDefuseCountDown = gpGlobals->time + 5;
		m_fNextDefuse = gpGlobals->time + 0.5;
		pPlayer->SetProgressBarTime(5);
	}
	else
	{
		UTIL_LogPrintf("\"%s<%i><%s><CT>\" triggered \"Begin_Bomb_Defuse_Without_Kit\"\n", STRING(pPlayer->pev->netname), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()));
		ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "#Defusing_Bomb_Without_Defuse_Kit");
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "weapons/c4_disarm.wav", VOL_NORM, ATTN_NORM);

		pPlayer->m_bIsDefusing = true;
		m_pBombDefuser = pCaller;
		m_bStartDefuse = true;
		m_flDefuseCountDown = gpGlobals->time + 10;
		m_fNextDefuse = gpGlobals->time + 0.5;
		pPlayer->SetProgressBarTime(10);
	}
}

CGrenade *CGrenade::ShootSatchelCharge(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CGrenade *pGrenade = GetClassPtr((CGrenade *)NULL);
	pGrenade->pev->movetype = MOVETYPE_TOSS;

	if (pGrenade->pev->classname)
		RemoveEntityHashValue(pGrenade->pev, STRING(pGrenade->pev->classname), CLASSNAME);

	pGrenade->pev->classname = MAKE_STRING("grenade");
	AddEntityHashValue(pGrenade->pev, STRING(pGrenade->pev->classname), CLASSNAME);

	pGrenade->pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pGrenade->pev), "models/w_c4.mdl");
	UTIL_SetSize(pGrenade->pev, Vector(-3, -6, 0), Vector(3, 6, 8));
	pGrenade->pev->dmg = 100;
	UTIL_SetOrigin(pGrenade->pev, vecStart);

	pGrenade->pev->velocity = g_vecZero;
	pGrenade->pev->angles = vecVelocity;
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->SetThink(&CGrenade::C4Think);
	pGrenade->SetTouch(&CGrenade::C4Touch);
	pGrenade->pev->spawnflags = SF_DETONATE;
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	pGrenade->m_flC4Blow = gpGlobals->time + g_pGameRules->m_iC4Timer;
	pGrenade->m_flNextFreqInterval = g_pGameRules->m_iC4Timer / 4.0;
	pGrenade->m_flNextFreq = gpGlobals->time;
	pGrenade->m_iCurWave = 0;
	pGrenade->m_fAttenu = 0;
	pGrenade->m_sBeepName = 0;
	pGrenade->m_flNextBeep = gpGlobals->time + 0.5;
	pGrenade->m_bIsC4 = true;
	pGrenade->m_fNextDefuse = 0;
	pGrenade->m_bStartDefuse = false;
	pGrenade->m_flNextBlink = gpGlobals->time + 2;
	pGrenade->pev->friction = 0.9;
	pGrenade->m_bJustBlew = false;

	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pevOwner);

	if (pPlayer)
		pGrenade->m_pentCurBombTarget = pPlayer->m_pentCurBombTarget;
	else
		pGrenade->m_pentCurBombTarget = NULL;

	return pGrenade;
}

CGrenade *CGrenade::ShootSmokeGrenade(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, unsigned short usEvent)
{
	CGrenade *pGrenade = GetClassPtr((CGrenade *)NULL);
	pGrenade->Spawn();

	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->m_usEvent = usEvent;
	pGrenade->m_bLightSmoke = false;
	pGrenade->m_bDetonated = false;
	pGrenade->SetTouch(&CGrenade::BounceTouch);
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CGrenade::SG_TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;

	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1;
	pGrenade->m_bJustBlew = true;
	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;
	pGrenade->m_SGSmoke = 0;

	SET_MODEL(ENT(pGrenade->pev), "models/w_smokegrenade.mdl");
	pGrenade->pev->dmg = 35;
	return pGrenade;
}

void CGrenade::C4Think (void)
{
   if (!IsInWorld ())
   {
      UTIL_Remove (this);
      return;
   }

   pev->nextthink = gpGlobals->time + 0.12;

   if (gpGlobals->time >= m_flNextFreq)
   {
      m_flNextFreq = gpGlobals->time + m_flNextFreqInterval;
      m_flNextFreqInterval *= 0.9;

      switch (m_iCurWave)
      {
      case 0:
         m_sBeepName = "weapons/c4_beep1.wav";
         m_fAttenu = 1.5;
         break;

      case 1:
         m_sBeepName = "weapons/c4_beep2.wav";
         m_fAttenu = 1.0;
         break;

      case 2:
         m_sBeepName = "weapons/c4_beep3.wav";
         m_fAttenu = 0.8;
         break;

      case 3:
         m_sBeepName = "weapons/c4_beep4.wav";
         m_fAttenu = 0.5;
         break;

      case 4:
         m_sBeepName = "weapons/c4_beep5.wav";
         m_fAttenu = 0.2;

         break;
      }
      ++m_iCurWave;
   }

   if (gpGlobals->time >= m_flNextBeep)
   {
      m_flNextBeep = gpGlobals->time + 1.4;
      EMIT_SOUND (ENT (pev), CHAN_VOICE, m_sBeepName, VOL_NORM, m_fAttenu);

   }

   if (gpGlobals->time >= m_flNextBlink)
   {
      m_flNextBlink = gpGlobals->time + 2;

      MESSAGE_BEGIN (MSG_PVS, SVC_TEMPENTITY, pev->origin);
      WRITE_BYTE (TE_GLOWSPRITE);
      WRITE_COORD (pev->origin.x);
      WRITE_COORD (pev->origin.y);
      WRITE_COORD (pev->origin.z + 5);
      WRITE_SHORT (g_sModelIndexC4Glow);
      WRITE_BYTE (1);
      WRITE_BYTE (3);
      WRITE_BYTE (255);
      MESSAGE_END ();
   }

   if (gpGlobals->time >= m_flC4Blow)
   {
      MESSAGE_BEGIN (MSG_ALL, gmsgScenarioIcon);
      WRITE_BYTE (0);
      MESSAGE_END ();

      if (m_pentCurBombTarget)
      {
         CBaseEntity *pBombTarget = CBaseEntity::Instance (m_pentCurBombTarget);

         if (pBombTarget)
         {
            pBombTarget->Use (CBaseEntity::Instance (pev->owner), this, USE_TOGGLE, 0);
         }
      }

      CBasePlayer *pBombOwner = (CBasePlayer *)CBaseEntity::Instance (pev->owner);
      if (pBombOwner)
      {
         pBombOwner->pev->frags += 3;
      }

      MESSAGE_BEGIN (MSG_ALL, gmsgBombPickup);
      MESSAGE_END ();

      g_pGameRules->m_bBombDropped = false;

      if (pev->waterlevel != 0)
         UTIL_Remove (this);
      else
         SetThink (&CGrenade::Detonate2);
   }

   if (m_bStartDefuse)
   {
      CBasePlayer *pPlayer = (CBasePlayer *)((CBaseEntity *)m_pBombDefuser);

      if (pPlayer != NULL && gpGlobals->time < m_flDefuseCountDown)
      {
         int iOnGround = ((m_pBombDefuser->pev->flags & FL_ONGROUND) == FL_ONGROUND);

         if (gpGlobals->time > m_fNextDefuse || !iOnGround)
         {
            if (!iOnGround)
            {
               ClientPrint (m_pBombDefuser->pev, HUD_PRINTCENTER, "#C4_Defuse_Must_Be_On_Ground");
            }

            pPlayer->ResetMaxSpeed ();
            pPlayer->m_bIsDefusing = false;
            pPlayer->SetProgressBarTime (0);

            m_pBombDefuser = NULL;
            m_bStartDefuse = false;
            m_flDefuseCountDown = 0;
         }
      }
      else
      {
         if (pPlayer != NULL && m_pBombDefuser->pev->deadflag == DEAD_NO)
         {
            Broadcast ("BOMBDEF");

            MESSAGE_BEGIN (MSG_SPEC, SVC_DIRECTOR);
            WRITE_BYTE (9);
            WRITE_BYTE (DRC_CMD_EVENT);
            WRITE_SHORT (ENTINDEX (m_pBombDefuser->edict ()));
            WRITE_SHORT (0);
            WRITE_LONG (15 | DRC_FLAG_FINAL | DRC_FLAG_FACEPLAYER | DRC_FLAG_DRAMATIC);
            MESSAGE_END ();

            UTIL_LogPrintf ("\"%s<%i><%s><CT>\" triggered \"Defused_The_Bomb\"\n",
               STRING (m_pBombDefuser->pev->netname),
               GETPLAYERUSERID (m_pBombDefuser->edict ()),
               GETPLAYERAUTHID (m_pBombDefuser->edict ()));

            UTIL_EmitAmbientSound (ENT (pev), pev->origin, "weapons/c4_beep5.wav", 0, ATTN_NONE, SND_STOP, 0);
            EMIT_SOUND (ENT (m_pBombDefuser->pev), CHAN_WEAPON, "weapons/c4_disarmed.wav", VOL_NORM, ATTN_NORM);
            UTIL_Remove (this);

            m_bJustBlew = true;

            pPlayer->ResetMaxSpeed ();
            pPlayer->m_bIsDefusing = false;

            MESSAGE_BEGIN (MSG_ALL, gmsgScenarioIcon);
            WRITE_BYTE (0);
            MESSAGE_END ();

            g_pGameRules->m_bBombDefused = true;
            g_pGameRules->CheckWinConditions ();

            m_pBombDefuser->pev->frags += 3;

            MESSAGE_BEGIN (MSG_ALL, gmsgBombPickup);
            MESSAGE_END ();

            g_pGameRules->m_bBombDropped = FALSE;
            m_pBombDefuser = NULL;
            m_bStartDefuse = false;
         }
         else
         {
            if (pPlayer != NULL)
            {
               pPlayer->ResetMaxSpeed ();
               pPlayer->m_bIsDefusing = false;
            }

            m_bStartDefuse = false;
            m_pBombDefuser = NULL;
         }
      }
   }
}


void CGrenade::UseSatchelCharges(entvars_t *pevOwner, SATCHELCODE code)
{
	if (!pevOwner)
		return;

	CBaseEntity *pOwner = CBaseEntity::Instance(pevOwner);
	edict_t *pentOwner = pOwner->edict();
	edict_t *pentFind = FIND_ENTITY_BY_CLASSNAME(NULL, "grenade");

	while (!FNullEnt(pentFind))
	{
		CBaseEntity *pEnt = Instance(pentFind);

		if (pEnt)
		{
			if (FBitSet(pEnt->pev->spawnflags, SF_DETONATE) && pEnt->pev->owner == pentOwner)
			{
				if (code == SATCHEL_DETONATE)
					pEnt->Use(pOwner, pOwner, USE_ON, 0);
				else
					pEnt->pev->owner = NULL;
			}
		}

		pentFind = FIND_ENTITY_BY_CLASSNAME(pentFind, "grenade");
	}
}

TYPEDESCRIPTION CGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CGrenade, m_fAttenu, FIELD_FLOAT),
	DEFINE_FIELD(CGrenade, m_flNextFreq, FIELD_TIME),
	DEFINE_FIELD(CGrenade, m_flC4Blow, FIELD_TIME),
	DEFINE_FIELD(CGrenade, m_flNextFreqInterval, FIELD_TIME),
	DEFINE_FIELD(CGrenade, m_flNextBeep, FIELD_TIME),
	DEFINE_FIELD(CGrenade, m_flDefuseCountDown, FIELD_TIME),
	DEFINE_FIELD(CGrenade, m_flNextBlink, FIELD_TIME),
	DEFINE_FIELD(CGrenade, m_pentCurBombTarget, FIELD_EDICT),
	DEFINE_FIELD(CGrenade, m_sBeepName, FIELD_POINTER),
	DEFINE_FIELD(CGrenade, m_bIsC4, FIELD_BOOLEAN),
	DEFINE_FIELD(CGrenade, m_bStartDefuse, FIELD_BOOLEAN),
	DEFINE_FIELD(CGrenade, m_SGSmoke, FIELD_INTEGER),
	DEFINE_FIELD(CGrenade, m_bJustBlew, FIELD_BOOLEAN),
	DEFINE_FIELD(CGrenade, m_bLightSmoke, FIELD_BOOLEAN),
	DEFINE_FIELD(CGrenade, m_usEvent, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CGrenade, CBaseMonster);
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
#include "nodes.h"
#include "doors.h"

extern CGraph WorldGraph;
extern BOOL FEntIsVisible(entvars_t *pev, entvars_t *pevTarget);
extern DLL_GLOBAL int g_iSkillLevel;

void CPointEntity::Spawn(void)
{
	pev->solid = SOLID_NOT;
}

class CNullEntity : public CBaseEntity
{
public:
	void Spawn(void);
};

void CNullEntity::Spawn(void)
{
	REMOVE_ENTITY(ENT(pev));
}

LINK_ENTITY_TO_CLASS(info_null, CNullEntity);

class CBaseDMStart : public CPointEntity
{
public:
	void KeyValue(KeyValueData *pkvd);
	BOOL IsTriggered(CBaseEntity *pEntity);
};

LINK_ENTITY_TO_CLASS(info_vip_start, CBaseDMStart);
LINK_ENTITY_TO_CLASS(info_player_deathmatch, CBaseDMStart);
LINK_ENTITY_TO_CLASS(info_player_start, CPointEntity);
LINK_ENTITY_TO_CLASS(info_landmark, CPointEntity);
LINK_ENTITY_TO_CLASS(info_hostage_rescue, CPointEntity);
LINK_ENTITY_TO_CLASS(info_bomb_target, CPointEntity);

void CBaseDMStart::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "master"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

BOOL CBaseDMStart::IsTriggered(CBaseEntity *pEntity)
{
	return UTIL_IsMasterTriggered(pev->netname, pEntity);
}

void CBaseEntity::UpdateOnRemove(void)
{
	if (FBitSet(pev->flags, FL_GRAPHED))
	{
		for (int i = 0; i < WorldGraph.m_cLinks; i++)
		{
			if (WorldGraph.m_pLinkPool[i].m_pLinkEnt == pev)
				WorldGraph.m_pLinkPool[i].m_pLinkEnt = NULL;
		}
	}

	if (pev->globalname)
		gGlobalState.EntitySetState(pev->globalname, GLOBAL_DEAD);
}

void CBaseEntity::SUB_Remove(void)
{
	UpdateOnRemove();

	if (pev->health > 0)
	{
		pev->health = 0;
		ALERT(at_aiconsole, "SUB_Remove called on entity with health > 0\n");
	}
	REMOVE_ENTITY(ENT(pev));
}

void CBaseEntity::SUB_DoNothing(void)
{

}

TYPEDESCRIPTION CBaseDelay::m_SaveData[] =
{
	DEFINE_FIELD(CBaseDelay, m_flDelay, FIELD_FLOAT),
	DEFINE_FIELD(CBaseDelay, m_iszKillTarget, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CBaseDelay, CBaseEntity);

void CBaseDelay::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "delay"))
	{
		m_flDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "killtarget"))
	{
		m_iszKillTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CBaseEntity::SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value)
{
	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), pActivator, this, useType, value);
}

void FireTargets(const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	edict_t *pentTarget = NULL;

	if (!targetName)
		return;

	ALERT(at_aiconsole, "Firing: (%s)\n", targetName);

	while (1)
	{
		pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, targetName);

		if (FNullEnt(pentTarget))
			break;

		CBaseEntity *pTarget = CBaseEntity::Instance(pentTarget);

		if (pTarget && !(pTarget->pev->flags & FL_KILLME))
		{
			ALERT(at_aiconsole, "Found: %s, firing (%s)\n", STRING(pTarget->pev->classname), targetName);
			pTarget->Use(pActivator, pCaller, useType, value);
		}
	}
}

LINK_ENTITY_TO_CLASS(DelayedUse, CBaseDelay);

void CBaseDelay::SUB_UseTargets(CBaseEntity *pActivator, USE_TYPE useType, float value)
{
	if (FStringNull(pev->target) && !m_iszKillTarget)
		return;

	if (m_flDelay != 0)
	{
		CBaseDelay *pTemp = GetClassPtr((CBaseDelay *)NULL);

		if (pTemp->pev->classname)
			RemoveEntityHashValue(pTemp->pev, STRING(pTemp->pev->classname), CLASSNAME);

		pTemp->pev->classname = MAKE_STRING("DelayedUse");
		AddEntityHashValue(pTemp->pev, STRING(pTemp->pev->classname), CLASSNAME);

		pTemp->pev->nextthink = gpGlobals->time + m_flDelay;
		pTemp->SetThink(&CBaseDelay::DelayThink);
		pTemp->pev->button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0;
		pTemp->pev->target = pev->target;

		if (pActivator && pActivator->IsPlayer())
			pTemp->pev->owner = pActivator->edict();
		else
			pTemp->pev->owner = NULL;

		return;
	}

	if (m_iszKillTarget)
	{
		ALERT(at_aiconsole, "KillTarget: %s\n", STRING(m_iszKillTarget));
		edict_t *pentKillTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_iszKillTarget));

		while (!FNullEnt(pentKillTarget))
		{
			UTIL_Remove(CBaseEntity::Instance(pentKillTarget));
			ALERT(at_aiconsole, "killing %s\n", STRING(pentKillTarget->v.classname));
			pentKillTarget = FIND_ENTITY_BY_TARGETNAME(pentKillTarget, STRING(m_iszKillTarget));
		}
	}

	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), pActivator, this, useType, value);
}

void SetMovedir(entvars_t *pev)
{
	if (pev->angles == Vector(0, -1, 0))
	{
		pev->movedir = Vector(0, 0, 1);
	}
	else if (pev->angles == Vector(0, -2, 0))
	{
		pev->movedir = Vector(0, 0, -1);
	}
	else
	{
		UTIL_MakeVectors(pev->angles);
		pev->movedir = gpGlobals->v_forward;
	}

	pev->angles = g_vecZero;
}

void CBaseDelay::DelayThink(void)
{
	CBaseEntity *pActivator = NULL;

	if (pev->owner)
		pActivator = CBaseEntity::Instance(pev->owner);

	SUB_UseTargets(pActivator, (USE_TYPE)pev->button, 0);
	REMOVE_ENTITY(ENT(pev));
}

TYPEDESCRIPTION CBaseToggle::m_SaveData[] =
{
	DEFINE_FIELD(CBaseToggle, m_toggle_state, FIELD_INTEGER),
	DEFINE_FIELD(CBaseToggle, m_flActivateFinished, FIELD_TIME),
	DEFINE_FIELD(CBaseToggle, m_flMoveDistance, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flWait, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flLip, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flTWidth, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_flTLength, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_vecPosition1, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecPosition2, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecAngle1, FIELD_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecAngle2, FIELD_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_cTriggersLeft, FIELD_INTEGER),
	DEFINE_FIELD(CBaseToggle, m_flHeight, FIELD_FLOAT),
	DEFINE_FIELD(CBaseToggle, m_hActivator, FIELD_EHANDLE),
	DEFINE_FIELD(CBaseToggle, m_pfnCallWhenMoveDone, FIELD_FUNCTION),
	DEFINE_FIELD(CBaseToggle, m_vecFinalDest, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_vecFinalAngle, FIELD_VECTOR),
	DEFINE_FIELD(CBaseToggle, m_sMaster, FIELD_STRING),
	DEFINE_FIELD(CBaseToggle, m_bitsDamageInflict, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CBaseToggle, CBaseAnimating);

void CBaseToggle::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "lip"))
	{
		m_flLip = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_sMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "distance"))
	{
		m_flMoveDistance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CBaseToggle::LinearMove(Vector vecDest, float flSpeed)
{
	m_vecFinalDest = vecDest;

	if (vecDest == pev->origin)
	{
		LinearMoveDone();
		return;
	}

	Vector vecDestDelta = vecDest - pev->origin;
	float flTravelTime = vecDestDelta.Length() / flSpeed;
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink(&CBaseToggle::LinearMoveDone);
	pev->velocity = vecDestDelta / flTravelTime;
}

void CBaseToggle::LinearMoveDone(void)
{
	UTIL_SetOrigin(pev, m_vecFinalDest);
	pev->velocity = g_vecZero;
	pev->nextthink = -1;

	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}

BOOL CBaseToggle::IsLockedByMaster(void)
{
	if (m_sMaster && !UTIL_IsMasterTriggered(m_sMaster, m_hActivator))
		return TRUE;
	else
		return FALSE;
}

void CBaseToggle::AngularMove(Vector vecDestAngle, float flSpeed)
{
	m_vecFinalAngle = vecDestAngle;

	if (vecDestAngle == pev->angles)
	{
		AngularMoveDone();
		return;
	}

	Vector vecDestDelta = vecDestAngle - pev->angles;
	float flTravelTime = vecDestDelta.Length() / flSpeed;
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink(&CBaseToggle::AngularMoveDone);
	pev->avelocity = vecDestDelta / flTravelTime;
}

void CBaseToggle::AngularMoveDone(void)
{
	pev->angles = m_vecFinalAngle;
	pev->avelocity = g_vecZero;
	pev->nextthink = -1;

	if (m_pfnCallWhenMoveDone)
		(this->*m_pfnCallWhenMoveDone)();
}

float CBaseToggle::AxisValue(int flags, const Vector &angles)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angles.z;

	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angles.x;

	return angles.y;
}

void CBaseToggle::AxisDir(entvars_t *pev)
{
	if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_Z))
		pev->movedir = Vector(0, 0, 1);
	else if (FBitSet(pev->spawnflags, SF_DOOR_ROTATE_X))
		pev->movedir = Vector(1, 0, 0);
	else
		pev->movedir = Vector(0, 1, 0);
}

float CBaseToggle::AxisDelta(int flags, const Vector &angle1, const Vector &angle2)
{
	if (FBitSet(flags, SF_DOOR_ROTATE_Z))
		return angle1.z - angle2.z;

	if (FBitSet(flags, SF_DOOR_ROTATE_X))
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}

BOOL FEntIsVisible(entvars_t *pev, entvars_t *pevTarget)
{
	Vector vecSpot1 = pev->origin + pev->view_ofs;
	Vector vecSpot2 = pevTarget->origin + pevTarget->view_ofs;

	TraceResult tr;
	UTIL_TraceLine(vecSpot1, vecSpot2, ignore_monsters, ENT(pev), &tr);

	if (tr.fInOpen && tr.fInWater)
		return FALSE;

	if (tr.flFraction == 1)
		return TRUE;

	return FALSE;
}
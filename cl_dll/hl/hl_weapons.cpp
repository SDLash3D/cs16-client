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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

#define PLAYER_H
#include "weapons.h"
#undef PLAYER_H

#include "nodes.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "../hud_iface.h"
#include "../com_weapons.h"
#include "../demo.h"

#include "cl_entity.h"

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

extern "C" char PM_FindTextureType( char *name );

extern globalvars_t *gpGlobals;
extern int g_iUser1;

// Pool of client side entities/entvars_t
static entvars_t	ev[ 32 ];
static int			num_ents = 0;

// The entity we'll use to represent the local client
static CBasePlayer	player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t	Globals;

static CBasePlayerWeapon *g_pWpns[ 32 ];

float g_flApplyVel = 0.0;
int   g_irunninggausspred = 0;

vec3_t previousorigin;

// HLDM Weapon placeholder entities.
/*CGlock g_Glock;
CCrowbar g_Crowbar;
CPython g_Python;
CMP5 g_Mp5;
CCrossbow g_Crossbow;
CShotgun g_Shotgun;
CRpg g_Rpg;
CGauss g_Gauss;
CEgon g_Egon;
CHgun g_HGun;
CHandGrenade g_HandGren;
CSatchel g_Satchel;
CTripmine g_Tripmine;
CSqueak g_Snark;*/

CAK47 g_AK47;
CAUG g_AUG;
CAWP g_AWP;
CC4 g_C4;
CDEAGLE g_DEAGLE;
CELITE g_ELITE;
CFamas g_Famas;
CFiveSeven g_FiveSeven;
CFlashbang g_Flashbang;
CG3SG1 g_G3SG1;
CGalil g_Galil;
CGLOCK18 g_GLOCK18;
CHEGrenade g_HEGrenade;
CKnife g_Knife;
CM249 g_M249;
CM3 g_M3;
CM4A1 g_M4A1;
CMAC10 g_MAC10;
CMP5N g_MP5N;
CP228 g_P228;
CP90 g_P90;
CSCOUT g_SCOUT;
CSG550 g_SG550;
CSG552 g_SG552;
CSmokeGrenade g_SmokeGrenade;
CTMP g_TMP;
CUMP45 g_UMP45;
CUSP g_USP;
CXM1014 g_XM1014;

int g_iWeaponFlags;
bool g_bInBombZone;
int g_iFreezeTimeOver;
int g_bHoldingShield;
vec3_t g_vPlayerVelocity;
float g_flPlayerSpeed;
int g_iPlayerFlags;

/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage( ALERT_TYPE atype, char *szFmt, ... )
{
	va_list		argptr;
	static char	string[1024];

	va_start (argptr, szFmt);
	vsprintf (string, szFmt,argptr);
	va_end (argptr);

	gEngfuncs.Con_Printf( "cl:  " );
	gEngfuncs.Con_Printf( string );
}

//Returns if it's multiplayer.
//Mostly used by the client side weapons.
bool bIsMultiplayer ( void )
{
	return gEngfuncs.GetMaxClients() == 1 ? 0 : 1;
}
//Just loads a v_ model.
void LoadVModel ( char *szViewModel, CBasePlayer *m_pPlayer )
{
	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );
}

/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
edict_t *EHANDLE::Get(void)
{
	if (!m_pent)
		return NULL;

	if (m_pent->serialnumber != m_serialnumber)
		return NULL;

	return m_pent;
}

edict_t *EHANDLE::Set(edict_t *pent)
{
	m_pent = pent;

	if (pent)
		m_serialnumber = m_pent->serialnumber;

	return pent;
}

EHANDLE::operator CBaseEntity *(void)
{
	return (CBaseEntity *)GET_PRIVATE(Get());
}

CBaseEntity *EHANDLE::operator = (CBaseEntity *pEntity)
{
	if (pEntity)
	{
		m_pent = ENT(pEntity->pev);

		if (m_pent)
			m_serialnumber = m_pent->serialnumber;
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}

	return pEntity;
}

EHANDLE::operator int(void)
{
	return Get() != NULL;
}

CBaseEntity *EHANDLE::operator ->(void)
{
	return (CBaseEntity *)GET_PRIVATE(Get());
}


void HUD_PrepEntity( CBaseEntity *pEntity, CBasePlayer *pWeaponOwner )
{
	memset( &ev[ num_ents ], 0, sizeof( entvars_t ) );
	pEntity->pev = &ev[ num_ents++ ];

	pEntity->Precache();
	pEntity->Spawn();

	if ( pWeaponOwner )
	{
		ItemInfo info;

		((CBasePlayerWeapon *)pEntity)->m_pPlayer = pWeaponOwner;

		((CBasePlayerWeapon *)pEntity)->GetItemInfo( &info );

		g_pWpns[ info.iId ] = (CBasePlayerWeapon *)pEntity;
	}
}

/*
=====================
CBaseEntity :: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon :: DefaultReload
=====================
*/
BOOL CBasePlayerWeapon :: DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

	if (j == 0)
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, UseDecrement() );

	m_fInReload = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: CanDeploy
=====================
*/
BOOL CBasePlayerWeapon :: CanDeploy( void )
{
	BOOL bHasAmmo = 0;

	if ( !pszAmmo1() )
	{
		// this weapon doesn't use ammo, can always deploy.
		return TRUE;
	}

	if ( pszAmmo1() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0);
	}
	if ( pszAmmo2() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0);
	}
	if (m_iClip > 0)
	{
		bHasAmmo |= 1;
	}
	if (!bHasAmmo)
	{
		return FALSE;
	}

	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: DefaultDeploy

=====================
*/
BOOL CBasePlayerWeapon :: DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int skiplocal )
{
	if ( !CanDeploy() )
		return FALSE;

	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );

	SendWeaponAnim( iAnim, skiplocal );

	g_irunninggausspred = false;
	m_pPlayer->m_flNextAttack = 0.5;
	m_flTimeWeaponIdle = 1.0;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: PlayEmptySound

=====================
*/
BOOL CBasePlayerWeapon :: PlayEmptySound( void )
{
	if (m_iPlayEmptySound)
	{
		HUD_PlaySound( "weapons/357_cock1.wav", 0.8 );
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

/*
=====================
CBasePlayerWeapon :: ResetEmptySound

=====================
*/
void CBasePlayerWeapon :: ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
void CBasePlayerWeapon::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE; // cancel any reload in progress.
	g_irunninggausspred = false;
	m_pPlayer->pev->viewmodel = 0;
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int skiplocal )
{
	m_pPlayer->pev->weaponanim = iAnim;

	HUD_SendWeaponAnim( iAnim, m_pPlayer->pev->body, 0 );
}

Vector CBaseEntity::FireBullets3 (Vector vecSrc, Vector vecDirShooting, float flSpread, float flDistance, int iPenetration, int iBulletType, int iDamage, float flRangeModifier, entvars_t *pevAttacker, bool bPistol, int shared_rand)
{
	int iOriginalPenetration = iPenetration;
	int iPenetrationPower;
	float flPenetrationDistance;
	int iCurrentDamage = iDamage;
	float flCurrentDistance;
	TraceResult tr, tr2;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	CBaseEntity *pEntity;
	bool bHitMetal = false;
	int iSparksAmount;

	switch (iBulletType)
	{
		case BULLET_PLAYER_9MM:
		{
			iPenetrationPower = 21;
			flPenetrationDistance = 800;
			iSparksAmount = 15;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		case BULLET_PLAYER_45ACP:
		{
			iPenetrationPower = 15;
			flPenetrationDistance = 500;
			iSparksAmount = 20;
			iCurrentDamage += (-2 + RANDOM_LONG(0, 4));
			break;
		}

		case BULLET_PLAYER_50AE:
		{
			iPenetrationPower = 30;
			flPenetrationDistance = 1000;
			iSparksAmount = 20;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		case BULLET_PLAYER_762MM:
		{
			iPenetrationPower = 39;
			flPenetrationDistance = 5000;
			iSparksAmount = 30;
			iCurrentDamage += (-2 + RANDOM_LONG(0, 4));
			break;
		}

		case BULLET_PLAYER_556MM:
		{
			iPenetrationPower = 35;
			flPenetrationDistance = 4000;
			iSparksAmount = 30;
			iCurrentDamage += (-3 + RANDOM_LONG(0, 6));
			break;
		}

		case BULLET_PLAYER_338MAG:
		{
			iPenetrationPower = 45;
			flPenetrationDistance = 8000;
			iSparksAmount = 30;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 8));
			break;
		}

		case BULLET_PLAYER_57MM:
		{
			iPenetrationPower = 30;
			flPenetrationDistance = 2000;
			iSparksAmount = 20;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		case BULLET_PLAYER_357SIG:
		{
			iPenetrationPower = 25;
			flPenetrationDistance = 800;
			iSparksAmount = 20;
			iCurrentDamage += (-4 + RANDOM_LONG(0, 10));
			break;
		}

		default:
		{
			iPenetrationPower = 0;
			flPenetrationDistance = 0;
			break;
		}
	}

	if (!pevAttacker)
		pevAttacker = pev;

	float x, y, z;

	if (IsPlayer())
	{
		x = UTIL_SharedRandomFloat(shared_rand, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + 1, -0.5, 0.5);
		y = UTIL_SharedRandomFloat(shared_rand + 2, -0.5, 0.5) + UTIL_SharedRandomFloat(shared_rand + 3, -0.5, 0.5);
	}
	else
	{
		do
		{
			x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
			z = x * x + y * y;
		}
		while (z > 1);
	}

	Vector vecDir = vecDirShooting + x * flSpread * vecRight + y * flSpread * vecUp;
	Vector vecEnd = vecSrc + vecDir * flDistance;
	Vector vecOldSrc;
	Vector vecNewSrc;
	float flDamageModifier = 0.5;

	while (iPenetration != 0)
	{
		ClearMultiDamage();
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

		char cTextureType = UTIL_TextureHit(&tr, vecSrc, vecEnd);
		bool bSparks = false;

		switch (cTextureType)
		{
			case CHAR_TEX_METAL:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.15;
				flDamageModifier = 0.2;
				break;
			}

			case CHAR_TEX_CONCRETE:
			{
				iPenetrationPower *= 0.25;
				flDamageModifier = 0.25;
				break;
			}

			case CHAR_TEX_GRATE:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.5;
				flDamageModifier = 0.4;
				break;
			}

			case CHAR_TEX_VENT:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.5;
				flDamageModifier = 0.45;
				break;
			}

			case CHAR_TEX_TILE:
			{
				iPenetrationPower *= 0.65;
				flDamageModifier = 0.3;
				break;
			}

			case CHAR_TEX_COMPUTER:
			{
				bSparks = true;
				bHitMetal = true;
				iPenetrationPower *= 0.4;
				flDamageModifier = 0.45;
				break;
			}

			case CHAR_TEX_WOOD:
			{
				iPenetrationPower *= 1;
				flDamageModifier = 0.6;
				break;
			}

			default:
			{
				bSparks = false;
				break;
			}
		}

		if (tr.flFraction != 1.0)
		{
			pEntity = CBaseEntity::Instance(tr.pHit);
			iPenetration--;
			flCurrentDistance = tr.flFraction * flDistance;
			iCurrentDamage *= pow(flRangeModifier, flCurrentDistance / 500);

			if (flCurrentDistance > flPenetrationDistance)
				iPenetration = 0;

			if (tr.iHitgroup == HITGROUP_SHIELD)
			{
				iPenetration = 0;

				if (tr.flFraction != 1.0)
				{
					if (RANDOM_LONG(0, 1))
						EMIT_SOUND(pEntity->edict(), CHAN_VOICE, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
					else
						EMIT_SOUND(pEntity->edict(), CHAN_VOICE, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

					pEntity->pev->punchangle.x = iCurrentDamage * RANDOM_FLOAT(-0.15, 0.15);
					pEntity->pev->punchangle.z = iCurrentDamage * RANDOM_FLOAT(-0.15, 0.15);

					if (pEntity->pev->punchangle.x < 4)
						pEntity->pev->punchangle.x = 4;

					if (pEntity->pev->punchangle.z < -5)
						pEntity->pev->punchangle.z = -5;
					else if (pEntity->pev->punchangle.z > 5)
						pEntity->pev->punchangle.z = 5;
				}

				break;
			}

			if ((VARS(tr.pHit)->solid == SOLID_BSP) && (iPenetration != 0))
			{
				if (bPistol)
					DecalGunshot(&tr, iBulletType, false, pev, bHitMetal);
				else if (RANDOM_LONG(0, 3))
					DecalGunshot(&tr, iBulletType, true, pev, bHitMetal);

				vecSrc = tr.vecEndPos + (vecDir * iPenetrationPower);
				flDistance = (flDistance - flCurrentDistance) * 0.5;
				vecEnd = vecSrc + (vecDir * flDistance);

				pEntity->TraceAttack(pevAttacker, iCurrentDamage, vecDir, &tr, DMG_BULLET | DMG_NEVERGIB);

				iCurrentDamage *= flDamageModifier;
			}
			else
			{
				if (bPistol)
					DecalGunshot(&tr, iBulletType, false, pev, bHitMetal);
				else if (RANDOM_LONG(0, 3))
					DecalGunshot(&tr, iBulletType, true, pev, bHitMetal);

				vecSrc = tr.vecEndPos + (vecDir * 42);
				flDistance = (flDistance - flCurrentDistance) * 0.75;
				vecEnd = vecSrc + (vecDir * flDistance);

				pEntity->TraceAttack(pevAttacker, iCurrentDamage, vecDir, &tr, DMG_BULLET | DMG_NEVERGIB);

				iCurrentDamage *= 0.75;
			}
		}
		else
			iPenetration = 0;

		ApplyMultiDamage(pev, pevAttacker);
	}

	return Vector(x * flSpread, y * flSpread, 0);
}
/*
=====================
CBasePlayerWeapon::ItemPostFrame

Handles weapon firing, reloading, etc.
=====================
*/
void CBasePlayerWeapon::ItemPostFrame( void )
{
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= 0.0))
	{
#if 0 // FIXME, need ammo on client to make this work right
		// complete the reload.
		int j = min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;
#else
		m_iClip += 10;
#endif
		m_fInReload = FALSE;
	}

	assert(m_pPlayer);

	if ((m_pPlayer->pev->button & IN_ATTACK2) && (m_flNextSecondaryAttack <= 0.0))
	{
		if ( pszAmmo2() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = TRUE;
		}

		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && (m_flNextPrimaryAttack <= 0.0))
	{
		if ( (m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		PrimaryAttack();
	}
	else if ( m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload )
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ( !(m_pPlayer->pev->button & (IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down

		m_fFireOnEmpty = FALSE;

		// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < 0.0 )
		{
			Reload();
			return;
		}

		WeaponIdle( );
		return;
	}

	// catch all
	if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}

/*
=====================
CBasePlayer::SelectItem

  Switch weapons
=====================
*/
void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem *pItem = NULL;

	if (!pItem)
		return;


	if (pItem == m_pActiveItem)
		return;

	if (m_pActiveItem)
		m_pActiveItem->Holster( );

	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
	}
}

/*
=====================
CBasePlayer::SelectLastItem

=====================
*/
void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem)
	{
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	if (m_pActiveItem)
		m_pActiveItem->Holster( );

	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	// Holster weapon immediately, to allow it to cleanup
	if ( m_pActiveItem )
		 m_pActiveItem->Holster( );

	g_irunninggausspred = false;
}

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn( void )
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy( );

	g_irunninggausspred = false;
}

Vector CBasePlayer::GetGunPosition()
{
	Vector origin = pev->origin;
	Vector view_ofs;

	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight(view_ofs);

	return origin + view_ofs;
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
	memset( ptr, 0, sizeof( *ptr ) );
	ptr->flFraction = 1.0;
}

/*
=====================
UTIL_ParticleBox

For debugging, draw a box around a player made out of particles
=====================
*/
void UTIL_ParticleBox( CBasePlayer *player, float *mins, float *maxs, float life, unsigned char r, unsigned char g, unsigned char b )
{
	int i;
	vec3_t mmin, mmax;

	for ( i = 0; i < 3; i++ )
	{
		mmin[ i ] = player->pev->origin[ i ] + mins[ i ];
		mmax[ i ] = player->pev->origin[ i ] + maxs[ i ];
	}

	gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mmin, (float *)&mmax, 5.0, 0, 255, 0 );
}

/*
=====================
UTIL_ParticleBoxes

For debugging, draw boxes for other collidable players
=====================
*/
void UTIL_ParticleBoxes( void )
{
	int idx;
	physent_t *pe;
	cl_entity_t *player;
	vec3_t mins, maxs;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( player->index - 1 );

	for ( idx = 1; idx < 100; idx++ )
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent( idx );
		if ( !pe )
			break;

		if ( pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients() )
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;

			gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mins, (float *)&maxs, 0, 0, 255, 2.0 );
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=====================
UTIL_ParticleLine

For debugging, draw a line made out of particles
=====================
*/
void UTIL_ParticleLine( CBasePlayer *player, float *start, float *end, float life, unsigned char r, unsigned char g, unsigned char b )
{
	gEngfuncs.pEfxAPI->R_ParticleLine( start, end, r, g, b, life );
}

char UTIL_TextureHit(TraceResult *ptr, Vector vecSrc, Vector vecEnd)
{
		char chTextureType;
		float rgfl1[3], rgfl2[3];
		const char *pTextureName;
		char szbuffer[64];
		CBaseEntity *pEntity;

		if( ptr->pHit == NULL )
			return CHAR_TEX_FLESH;

		pEntity = CBaseEntity::Instance(ptr->pHit);

		if (pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
				return CHAR_TEX_FLESH;

		vecSrc.CopyToArray(rgfl1);
		vecEnd.CopyToArray(rgfl2);

		if (pEntity)
				pTextureName = TRACE_TEXTURE(ENT(pEntity->pev), rgfl1, rgfl2);
		else
				pTextureName = TRACE_TEXTURE(ENT(0), rgfl1, rgfl2);

		if (pTextureName)
		{
				if (*pTextureName == '-' || *pTextureName == '+')
						pTextureName += 2;

				if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
						pTextureName++;

				strcpy(szbuffer, pTextureName);
				szbuffer[CBTEXTURENAMEMAX - 1] = 0;
				chTextureType = PM_FindTextureType(szbuffer);
		}
		else
				chTextureType = 0;

		return chTextureType;
}

CBaseEntity *UTIL_PlayerByIndex(int playerIndex)
{
	CBaseEntity *pPlayer = NULL;

	if (playerIndex > 0 && playerIndex <= gpGlobals->maxClients)
	{
		edict_t *pPlayerEdict = INDEXENT(playerIndex);

		if (pPlayerEdict && !pPlayerEdict->free)
			pPlayer = CBaseEntity::Instance(pPlayerEdict);
	}

	return pPlayer;
}

void UTIL_MakeVectors( const Vector &vec )
{
	gEngfuncs.pfnAngleVectors( vec, gpGlobals->v_forward, gpGlobals->v_right, gpGlobals->v_up );
}

/*
=====================
CBasePlayerWeapon::PrintState

For debugging, print out state variables to log file
=====================
*/
/*void CBasePlayerWeapon::PrintState( void )
{
	COM_Log( "c:\\hl.log", "%.4f ", gpGlobals->time );
	COM_Log( "c:\\hl.log", "%.4f ", m_pPlayer->m_flNextAttack );
	COM_Log( "c:\\hl.log", "%.4f ", m_flNextPrimaryAttack );
	COM_Log( "c:\\hl.log", "%.4f ", m_flTimeWeaponIdle - gpGlobals->time);
	COM_Log( "c:\\hl.log", "%i ", m_iClip );
}*/

int RandomLong( int a, int b )
{
	return gEngfuncs.pfnRandomLong( a, b );
}

/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons( void )
{
	static int initialized = 0;
	if ( initialized )
		return;

	initialized = 1;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// Fill in current time ( probably not needed )
	gpGlobals->time = gEngfuncs.GetClientTime();

	// Fake functions
	g_engfuncs.pfnPrecacheModel		= stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound		= stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent		= stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction	= stub_NameForFunction;
	g_engfuncs.pfnSetModel			= stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent		= HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage		= AlertMessage;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent		= gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat		= gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong		= RandomLong;

	// Allocate a slot for the local player
	HUD_PrepEntity( &player		, NULL );

	// Allocate slot(s) for each weapon that we are going to be predicting
	/*HUD_PrepEntity( &g_Glock	, &player );
	HUD_PrepEntity( &g_Crowbar	, &player );
	HUD_PrepEntity( &g_Python	, &player );
	HUD_PrepEntity( &g_Mp5	, &player );
	HUD_PrepEntity( &g_Crossbow	, &player );
	HUD_PrepEntity( &g_Shotgun	, &player );
	HUD_PrepEntity( &g_Rpg	, &player );
	HUD_PrepEntity( &g_Gauss	, &player );
	HUD_PrepEntity( &g_Egon	, &player );
	HUD_PrepEntity( &g_HGun	, &player );
	HUD_PrepEntity( &g_HandGren	, &player );
	HUD_PrepEntity( &g_Satchel	, &player );
	HUD_PrepEntity( &g_Tripmine	, &player );
	HUD_PrepEntity( &g_Snark	, &player );*/
	HUD_PrepEntity( &g_P228, &player);
	//HUD_PrepEntity( &g_GLOCK18, &player);
	HUD_PrepEntity( &g_SCOUT, &player);
	HUD_PrepEntity( &g_HEGrenade, &player);
	HUD_PrepEntity( &g_XM1014, &player);
	HUD_PrepEntity( &g_C4, &player);
	HUD_PrepEntity( &g_MAC10, &player);
	HUD_PrepEntity( &g_AUG, &player);
	HUD_PrepEntity( &g_SmokeGrenade, &player);
	HUD_PrepEntity( &g_ELITE, &player);
	HUD_PrepEntity( &g_FiveSeven, &player);
	HUD_PrepEntity( &g_UMP45, &player);
	HUD_PrepEntity( &g_SG550, &player);
	HUD_PrepEntity( &g_Galil, &player);
	HUD_PrepEntity( &g_Famas, &player);
	HUD_PrepEntity( &g_USP, &player);
	HUD_PrepEntity( &g_GLOCK18, &player);
	HUD_PrepEntity( &g_AWP, &player);
	HUD_PrepEntity( &g_MP5N, &player);
	HUD_PrepEntity( &g_M249, &player);
	HUD_PrepEntity( &g_M4A1, &player);
	HUD_PrepEntity( &g_M3, &player );
	HUD_PrepEntity( &g_TMP, &player);
	HUD_PrepEntity( &g_G3SG1, &player);
	HUD_PrepEntity( &g_Flashbang, &player);
	HUD_PrepEntity( &g_DEAGLE, &player);
	HUD_PrepEntity( &g_SG552, &player);
	HUD_PrepEntity( &g_AK47, &player);
	HUD_PrepEntity( &g_Knife, &player);
	HUD_PrepEntity( &g_P90, &player );
}

/*
=====================
HUD_GetLastOrg

Retruns the last position that we stored for egon beam endpoint.
=====================
*/
void HUD_GetLastOrg( float *org )
{
	int i;

	// Return last origin
	for ( i = 0; i < 3; i++ )
	{
		org[i] = previousorigin[i];
	}
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg( void )
{
	int i;

	// Offset final origin by view_offset
	for ( i = 0; i < 3; i++ )
	{
		previousorigin[i] = g_finalstate->playerstate.origin[i] + g_finalstate->client.view_ofs[ i ];
	}
}

int GetWeaponAccuracyFlags( int weaponid )
{
	int result = 0;

	if( weaponid <= WEAPON_P90 )
	{
		switch( weaponid )
		{
		case WEAPON_AUG:
		case WEAPON_GALIL:
		case WEAPON_M249:
		case WEAPON_SG552:
		case WEAPON_AK47:
		case WEAPON_P90:
			result = ACCURACY_AIR | ACCURACY_SPEED;
			break;
		case WEAPON_P228:
		case WEAPON_FIVESEVEN:
		case WEAPON_DEAGLE:
			result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK;
			break;
		case WEAPON_GLOCK18:
			if( g_iWeaponFlags & WPNSTATE_GLOCK18_BURST_MODE)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK | ACCURACY_MULTIPLY_BY_14_2;
			}
			break;
		case WEAPON_MAC10:
		case WEAPON_UMP45:
		case WEAPON_MP5N:
		case WEAPON_TMP:
			result = ACCURACY_AIR;
			break;
		case WEAPON_M4A1:
			if(g_iWeaponFlags & WPNSTATE_USP_SILENCED)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_MULTIPLY_BY_14;
			}
			break;
		case WEAPON_FAMAS:
			if(g_iWeaponFlags & WPNSTATE_FAMAS_BURST_MODE)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | (1<<4);
			}
			break;
		case WEAPON_USP:
			if(g_iWeaponFlags & WPNSTATE_USP_SILENCED)
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK;
			}
			else
			{
				result = ACCURACY_AIR | ACCURACY_SPEED | ACCURACY_DUCK | ACCURACY_MULTIPLY_BY_14;
			}
			break;
		}
	}

	return result;
}


/*
=====================
HUD_WeaponsPostThink

Run Weapon firing code on client
=====================
*/
void HUD_WeaponsPostThink( local_state_s *from, local_state_s *to, usercmd_t *cmd, double time, unsigned int random_seed )
{
	int i;
	int buttonsChanged;
	CBasePlayerWeapon *pWeapon = NULL;
	CBasePlayerWeapon *pCurrent;
	weapon_data_t nulldata, *pfrom, *pto;
	static int lasthealth;
	int flags;

	memset( &nulldata, 0, sizeof( nulldata ) );

	HUD_InitClientWeapons();

	// Get current clock
	gpGlobals->time = time;

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	switch ( from->client.m_iId )
	{
		case WEAPON_P228:
			pWeapon = &g_P228;
			break;

		case WEAPON_SCOUT:
			pWeapon = &g_SCOUT;
			break;

		case WEAPON_HEGRENADE:
			pWeapon = &g_HEGrenade;
			break;

		case WEAPON_XM1014:
			pWeapon = &g_XM1014;
			break;

		case WEAPON_C4:
			pWeapon = &g_C4;
			break;

		case WEAPON_MAC10:
			pWeapon = &g_MAC10;
			break;

		case WEAPON_AUG:
			pWeapon = &g_AUG;
			break;

		case WEAPON_SMOKEGRENADE:
			pWeapon = &g_SmokeGrenade;
			break;

		case WEAPON_ELITE:
			pWeapon = &g_ELITE;
			break;

		case WEAPON_FIVESEVEN:
			pWeapon = &g_FiveSeven;
			break;

		case WEAPON_UMP45:
			pWeapon = &g_UMP45;
			break;

		case WEAPON_SG550:
			pWeapon = &g_SG550;
			break;

		case WEAPON_GALIL:
			pWeapon = &g_Galil;
			break;

		case WEAPON_FAMAS:
			pWeapon = &g_Famas;
			break;

		case WEAPON_USP:
			pWeapon = &g_USP;
			break;

		case WEAPON_GLOCK18:
			pWeapon = &g_GLOCK18;
			break;

		case WEAPON_AWP:
			pWeapon = &g_AWP;
			break;

		case WEAPON_MP5N:
			pWeapon = &g_MP5N;
			break;

		case WEAPON_M249:
			pWeapon = &g_M249;
			break;

		case WEAPON_M3:
			pWeapon = &g_M3;
			break;

		case WEAPON_M4A1:
			pWeapon = &g_M4A1;
			break;

		case WEAPON_TMP:
			pWeapon = &g_TMP;
			break;

		case WEAPON_G3SG1:
			pWeapon = &g_G3SG1;
			break;

		case WEAPON_FLASHBANG:
			pWeapon = &g_Flashbang;
			break;

		case WEAPON_DEAGLE:
			pWeapon = &g_DEAGLE;
			break;

		case WEAPON_SG552:
			pWeapon = &g_SG552;
			break;

		case WEAPON_AK47:
			pWeapon = &g_AK47;
			break;

		case WEAPON_KNIFE:
			pWeapon = &g_Knife;
			break;

		case WEAPON_P90:
			pWeapon = &g_P90;
			break;
	}

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if ( g_runfuncs )
	{
		if ( to->client.health <= 0 && lasthealth > 0 )
		{
			player.Killed( NULL, 0 );

		}
		else if ( to->client.health > 0 && lasthealth <= 0 )
		{
			player.Spawn();
		}

		lasthealth = to->client.health;
	}

	// We are not predicting the current weapon, just bow out here.
	if ( !pWeapon )
		return;

	for ( i = 0; i < 32; i++ )
	{
		pCurrent = g_pWpns[ i ];
		if ( !pCurrent )
		{
			continue;
		}

		pfrom = &from->weapondata[ i ];

		pCurrent->m_fInReload			= pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload	= pfrom->m_fInSpecialReload;
		pCurrent->m_iClip				= pfrom->m_iClip;
		pCurrent->m_flNextPrimaryAttack	= pfrom->m_flNextPrimaryAttack;
		pCurrent->m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
		pCurrent->m_flTimeWeaponIdle	= pfrom->m_flTimeWeaponIdle;
		pCurrent->pev->fuser1			= pfrom->fuser1;
		pCurrent->m_flStartThrow		= pfrom->fuser2;
		pCurrent->m_flReleaseThrow		= pfrom->fuser3;
		pCurrent->m_iSwing				= pfrom->iuser1;
		pCurrent->m_iWeaponState		= pfrom->m_iWeaponState;
		pCurrent->m_flLastFire			= pfrom->m_fAimedDamage;
		pCurrent->m_iShotsFired			= pfrom->m_fInZoom;
		pCurrent->m_iSecondaryAmmoType		= (int)from->client.vuser3[ 2 ];
		pCurrent->m_iPrimaryAmmoType		= (int)from->client.vuser4[ 0 ];
		player.m_rgAmmo[ pCurrent->m_iPrimaryAmmoType ]	= (int)from->client.vuser4[ 1 ];
		player.m_rgAmmo[ pCurrent->m_iSecondaryAmmoType ]	= (int)from->client.vuser4[ 2 ];
	}

	if( g_pWpns[ from->client.m_iId - 1])
		g_iWeaponFlags = g_pWpns[ from->client.m_iId - 1 ]->m_iWeaponState;

	// For random weapon events, use this seed to seed random # generator
	player.random_seed = random_seed;

	// Get old buttons from previous state.
	player.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	buttonsChanged = (player.m_afButtonLast ^ cmd->buttons);	// These buttons have changed this frame

	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	player.m_afButtonPressed =  buttonsChanged & cmd->buttons;
	// The ones not down are "released"
	player.m_afButtonReleased = buttonsChanged & (~cmd->buttons);

	// Set player variables that weapons code might check/alter
	player.pev->button = cmd->buttons;

	player.pev->velocity = from->client.velocity;
	g_iPlayerFlags = player.pev->flags = from->client.flags;

	player.pev->deadflag = from->client.deadflag;
	player.pev->waterlevel = from->client.waterlevel;
	player.pev->maxspeed    = from->client.maxspeed;
	player.pev->punchangle = from->client.punchangle;
	player.pev->fov = from->client.fov;
	player.pev->weaponanim = from->client.weaponanim;
	player.pev->viewmodel = from->client.viewmodel;
	player.m_flNextAttack = from->client.m_flNextAttack;

	g_vPlayerVelocity	= from->client.velocity;
	g_flPlayerSpeed		= from->client.velocity.Length();

	//Stores all our ammo info, so the client side weapons can use them.
	player.ammo_9mm			= (int)from->client.ammo_nails;
	player.ammo_556nato		= (int)from->client.ammo_cells;
	player.ammo_buckshot	= (int)from->client.ammo_shells;
	player.ammo_556natobox	= (int)from->client.ammo_rockets;
	player.ammo_762nato		= (int)from->client.vuser2.x;
	player.ammo_45acp		= (int)from->client.vuser2.y;
	player.ammo_50ae		= (int)from->client.vuser2.z;
	player.ammo_338mag		= (int)from->client.vuser3.x;
	player.ammo_57mm		= (int)from->client.vuser3.y;
	player.ammo_357sig		= (int)from->client.vuser3.z;

	cl_entity_t *pplayer = gEngfuncs.GetLocalPlayer();
	if( pplayer )
	{
		player.pev->origin = from->client.origin;
		player.pev->angles	= pplayer->angles;
		player.pev->v_angle = v_angles;
	}

	flags = from->client.iuser3;
	player.m_bCanShoot	= flags & PLAYER_CAN_SHOOT;
	g_iFreezeTimeOver	= !(flags & PLAYER_FREEZE_TIME_OVER);
	g_bInBombZone		= flags & PLAYER_IN_BOMB_ZONE;
	g_bHoldingShield	= flags & PLAYER_HOLDING_SHIELD;

	// Point to current weapon object
	if ( from->client.m_iId )
	{
		player.m_pActiveItem = g_pWpns[ from->client.m_iId ];
	}

	// Don't go firing anything if we have died.
	// Or if we don't have a weapon model deployed
	if ( ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) &&
		 !CL_IsDead() && player.pev->viewmodel && !g_iUser1 )
	{
		if ( player.m_flNextAttack <= 0 )
		{
			pWeapon->ItemPostFrame();
		}
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId					= from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if ( cmd->weaponselect && ( player.pev->deadflag != ( DEAD_DISCARDBODY + 1 ) ) )
	{
		// Switched to a different weapon?
		if ( from->weapondata[ cmd->weaponselect ].m_iId == cmd->weaponselect )
		{
			CBasePlayerWeapon *pNew = g_pWpns[ cmd->weaponselect ];
			if ( pNew && ( pNew != pWeapon ) )
			{
				// Put away old weapon
				if (player.m_pActiveItem)
					player.m_pActiveItem->Holster( );

				player.m_pLastItem = player.m_pActiveItem;
				player.m_pActiveItem = pNew;

				// Deploy new weapon
				if (player.m_pActiveItem)
				{
					player.m_pActiveItem->Deploy( );
				}

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel				= player.pev->viewmodel;
	to->client.fov						= player.pev->fov;
	to->client.weaponanim				= player.pev->weaponanim;
	to->client.m_flNextAttack			= player.m_flNextAttack;
	to->client.maxspeed					= player.pev->maxspeed;
	to->client.punchangle				= player.pev->punchangle;


	to->client.ammo_nails = player.ammo_9mm;
	to->client.ammo_cells = player.ammo_556nato;
	to->client.ammo_shells = player.ammo_buckshot;
	to->client.ammo_rockets = player.ammo_556natobox;
	to->client.vuser2.x = player.ammo_762nato;
	to->client.vuser2.y = player.ammo_45acp;
	to->client.vuser2.z = player.ammo_50ae;
	to->client.vuser3.x = player.ammo_338mag;
	to->client.vuser3.y = player.ammo_57mm;
	to->client.vuser3.z = player.ammo_357sig;
	to->client.iuser3 = flags;




	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if ( g_runfuncs && ( HUD_GetWeaponAnim() != to->client.weaponanim ) )
	{
		int body = 2;

		//Pop the model to body 0.
		/*if ( pWeapon == &g_Tripmine )
			 body = 0;*/

		//Show laser sight/scope combo
		/*if ( pWeapon == &g_Python && bIsMultiplayer() )
			 body = 1;*/

		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim( to->client.weaponanim, body, 1 );
	}

	for ( i = 0; i < 32; i++ )
	{
		pCurrent = g_pWpns[ i ];

		pto = &to->weapondata[ i ];

		if ( !pCurrent )
		{
			memset( pto, 0, sizeof( weapon_data_t ) );
			continue;
		}

		pto->m_fInReload				= pCurrent->m_fInReload;
		pto->m_fInSpecialReload			= pCurrent->m_fInSpecialReload;
		pto->m_iClip					= pCurrent->m_iClip;
		pto->m_flNextPrimaryAttack		= pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack	= pCurrent->m_flNextSecondaryAttack;
		pto->m_flTimeWeaponIdle			= pCurrent->m_flTimeWeaponIdle;
		pto->fuser1						= pCurrent->pev->fuser1;
		pto->fuser2						= pCurrent->m_flStartThrow;
		pto->fuser3						= pCurrent->m_flReleaseThrow;
		pto->iuser1						= pCurrent->m_iSwing;
		pto->m_iWeaponState				= pCurrent->m_iWeaponState;
		pto->m_fAimedDamage				= pCurrent->m_flLastFire;

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload				-= cmd->msec / 1000.0;
		pto->m_fNextAimBonus			-= cmd->msec / 1000.0;
		pto->m_flNextPrimaryAttack		-= cmd->msec / 1000.0;
		pto->m_flNextSecondaryAttack	-= cmd->msec / 1000.0;
		pto->m_flTimeWeaponIdle			-= cmd->msec / 1000.0;
		pto->fuser1						-= cmd->msec / 1000.0;

		to->client.vuser3[2]				= pCurrent->m_iSecondaryAmmoType;
		to->client.vuser4[0]				= pCurrent->m_iPrimaryAmmoType;
		to->client.vuser4[1]				= player.m_rgAmmo[ pCurrent->m_iPrimaryAmmoType ];
		to->client.vuser4[2]				= player.m_rgAmmo[ pCurrent->m_iSecondaryAmmoType ];

		if ( pto->m_fNextAimBonus < -1.0 )
		{
			pto->m_fNextAimBonus = -1.0;
		}

		if ( pto->m_flNextPrimaryAttack < -1.0 )
		{
			pto->m_flNextPrimaryAttack = -1.0;
		}

		if ( pto->m_flNextSecondaryAttack < -0.001 )
		{
			pto->m_flNextSecondaryAttack = -0.001;
		}

		if ( pto->m_flTimeWeaponIdle < -0.001 )
		{
			pto->m_flTimeWeaponIdle = -0.001;
		}

		if ( pto->m_flNextReload < -0.001 )
		{
			pto->m_flNextReload = -0.001;
		}

		if ( pto->fuser1 < -0.001 )
		{
			pto->fuser1 = -0.001;
		}
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= cmd->msec / 1000.0;
	if ( to->client.m_flNextAttack < -0.001 )
	{
		to->client.m_flNextAttack = -0.001;
	}

	to->client.fuser2 -= cmd->msec / 1000.0;
	if ( to->client.fuser2 < -0.001 )
	{
		to->client.fuser2 = -0.001;
	}

	to->client.fuser3 -= cmd->msec / 1000.0;
	if ( to->client.fuser3 < -0.001 )
	{
		to->client.fuser3 = -0.001;
	}

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void _DLLEXPORT HUD_PostRunCmd( local_state_t *from, local_state_t *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
	g_runfuncs = runfuncs;
	//g_curstate = from;

#if defined( CLIENT_WEAPONS )
	if ( cl_lw && cl_lw->value )
	{
		HUD_WeaponsPostThink( from, to, cmd, time, random_seed );
	}
	else
#endif
	{
		to->client.fov = g_lastFOV;
		g_iWeaponFlags = from->weapondata[ from->client.m_iId - 1].m_iWeaponState;
		g_iPlayerFlags = from->client.flags;
		g_iFreezeTimeOver	= !(from->client.iuser3 & PLAYER_FREEZE_TIME_OVER);
		g_bInBombZone		= from->client.iuser3 & PLAYER_IN_BOMB_ZONE;
		g_bHoldingShield	= from->client.iuser3 & PLAYER_HOLDING_SHIELD;
	}

	if ( g_irunninggausspred == 1 )
	{
		Vector forward;
		gEngfuncs.pfnAngleVectors( v_angles, forward, NULL, NULL );
		to->client.velocity = to->client.velocity - forward * g_flApplyVel * 5;
		g_irunninggausspred = false;
	}

	// All games can use FOV state
	g_lastFOV = to->client.fov;
}

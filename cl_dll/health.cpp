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
//
// Health.cpp
//
// implementation of CHudHealth class
//

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "eventscripts.h"

DECLARE_COMMAND( m_Health, ShowRadar );
DECLARE_COMMAND( m_Health, HideRadar );

DECLARE_MESSAGE(m_Health, Health )
DECLARE_MESSAGE(m_Health, Damage )
DECLARE_MESSAGE(m_Health, Radar )
DECLARE_MESSAGE(m_Health, ScoreAttrib )
DECLARE_MESSAGE(m_Health, ClCorpse )

#define PAIN_NAME "sprites/%d_pain.spr"
#define DAMAGE_NAME "sprites/%d_dmg.spr"

cvar_t *cl_radartype;

int giDmgHeight, giDmgWidth;

float g_LocationColor[3];

int giDmgFlags[NUM_DMG_TYPES] = 
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE|DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN|DMG_SLOWBURN,
	DMG_NERVEGAS, 
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_CALTROP,
	DMG_TRANQ,
	DMG_CONCUSS,
	DMG_HALLUC
};

int CHudHealth::Init(void)
{
	HOOK_MESSAGE(Health);
	HOOK_MESSAGE(Damage);
	HOOK_MESSAGE(Radar);
	HOOK_MESSAGE(ScoreAttrib);
	HOOK_MESSAGE(ClCorpse);
	HOOK_COMMAND( "drawradar", ShowRadar );
	HOOK_COMMAND( "hideradar", HideRadar );

	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_bitsDamage = 0;
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;

	memset(m_dmg, 0, sizeof(DAMAGE_IMAGE) * NUM_DMG_TYPES);

	cl_radartype = CVAR_CREATE( "cl_radartype", "0", FCVAR_ARCHIVE );
	CVAR_CREATE("cl_corpsestay", "600", FCVAR_ARCHIVE);
	gHUD.AddHudElem(this);
	return 1;
}

void CHudHealth::Reset( void )
{
	// make sure the pain compass is cleared when the player respawns
	m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;


	// force all the flashing damage icons to expire
	m_bitsDamage = 0;
	for ( int i = 0; i < NUM_DMG_TYPES; i++ )
	{
		m_dmg[i].fExpire = 0;
	}
}

int CHudHealth::VidInit(void)
{
	m_hSprite = 0;

	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" ) + 1;
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );

	giDmgHeight = gHUD.GetSpriteRect(m_HUD_dmg_bio).right - gHUD.GetSpriteRect(m_HUD_dmg_bio).left;
	giDmgWidth = gHUD.GetSpriteRect(m_HUD_dmg_bio).bottom - gHUD.GetSpriteRect(m_HUD_dmg_bio).top;

	m_hRadar = gHUD.GetSprite( gHUD.GetSpriteIndex( "radar" ));
	m_hRadaropaque = gHUD.GetSprite( gHUD.GetSpriteIndex( "radaropaque" ));
	m_hrad = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "radar" ));
	m_hradopaque = gHUD.GetSpriteRect( gHUD.GetSpriteIndex( "radaropaque" ));
	m_bDrawRadar = true;

	return 1;
}

int CHudHealth:: MsgFunc_Health(const char *pszName,  int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if (x != m_iHealth)
	{
		m_fFade = FADE_TIME;
		m_iHealth = x;
	}

	return 1;
}


int CHudHealth:: MsgFunc_Damage(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int armor = READ_BYTE();	// armor
	int damageTaken = READ_BYTE();	// health
	long bitsDamage = READ_LONG(); // damage bits

	vec3_t vecFrom;

	for ( int i = 0 ; i < 3 ; i++)
		vecFrom[i] = READ_COORD();

	UpdateTiles(gHUD.m_flTime, bitsDamage);

	// Actually took damage?
	if ( damageTaken > 0 || armor > 0 )
		CalcDamageDirection(vecFrom);

	return 1;
}

int CHudHealth:: MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int index = READ_BYTE();
	g_PlayerExtraInfo[index].origin.x = READ_COORD();
	g_PlayerExtraInfo[index].origin.y = READ_COORD();
	g_PlayerExtraInfo[index].origin.z = READ_COORD();
	return 1;
}

int CHudHealth:: MsgFunc_ScoreAttrib(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int index = READ_BYTE();
	unsigned char flags = READ_BYTE();
	g_PlayerExtraInfo[index].dead = (flags & 1<<0) != 0;
	g_PlayerExtraInfo[index].has_c4 = (flags & 1<<1) != 0;
	g_PlayerExtraInfo[index].vip = (flags & 1<<2) != 0;
	return 1;
}
// Returns back a color from the
// Green <-> Yellow <-> Red ramp
void CHudHealth::GetPainColor( int &r, int &g, int &b )
{
	int iHealth = m_iHealth;

	if (iHealth > 25)
		iHealth -= 25;
	else if ( iHealth < 0 )
		iHealth = 0;
#if 0
	g = iHealth * 255 / 100;
	r = 255 - g;
	b = 0;
#else
	if (m_iHealth > 25)
	{
		UnpackRGB(r,g,b, RGB_YELLOWISH);
	}
	else
	{
		r = 250;
		g = 0;
		b = 0;
	}
#endif 
}

int CHudHealth::Draw(float flTime)
{
	int r, g, b;
	int a = 0, x, y;
	int HealthWidth;

	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly() )
		return 1;

	if ( !gHUD.m_fPlayerDead && m_bDrawRadar )
		DrawRadar( flTime );

	if ( !m_hSprite )
		m_hSprite = LoadSprite(PAIN_NAME);
	
	// Has health changed? Flash the health #
	if (m_fFade)
	{
		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;

	}
	else
		a = MIN_ALPHA;

	// If health is getting low, make it bright red
	if (m_iHealth <= 15)
		a = 255;
		
	GetPainColor( r, g, b );
	ScaleColors(r, g, b, a );

	// Only draw health if we have the suit.
	if (gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)))
	{
		HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
		int CrossWidth = gHUD.GetSpriteRect(m_HUD_cross).right - gHUD.GetSpriteRect(m_HUD_cross).left;

		y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		x = CrossWidth /2;

		SPR_Set(gHUD.GetSprite(m_HUD_cross), r, g, b);
		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_cross));

		x = CrossWidth + HealthWidth / 2;

		x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iHealth, r, g, b);

		x += HealthWidth/2;

		int iHeight = gHUD.m_iFontHeight;
		int iWidth = HealthWidth/10;
		FillRGBA(x, y, iWidth, iHeight, 255, 160, 0, a);
	}

	DrawDamage(flTime);
	return DrawPain(flTime);
}

void CHudHealth::CalcDamageDirection(vec3_t vecFrom)
{
	vec3_t	forward, right, up;
	float	side, front;
	vec3_t vecOrigin, vecAngles;

	if (!vecFrom[0] && !vecFrom[1] && !vecFrom[2])
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 0;
		return;
	}


	memcpy(vecOrigin, gHUD.m_vecOrigin, sizeof(vec3_t));
	memcpy(vecAngles, gHUD.m_vecAngles, sizeof(vec3_t));


	VectorSubtract (vecFrom, vecOrigin, vecFrom);

	float flDistToTarget = vecFrom.Length();

	vecFrom = vecFrom.Normalize();
	AngleVectors (vecAngles, forward, right, up);

	front = DotProduct (vecFrom, right);
	side = DotProduct (vecFrom, forward);

	if (flDistToTarget <= 50)
	{
		m_fAttackFront = m_fAttackRear = m_fAttackRight = m_fAttackLeft = 1;
	}
	else 
	{
		if (side > 0)
		{
			if (side > 0.3)
				m_fAttackFront = max(m_fAttackFront, side);
		}
		else
		{
			float f = fabs(side);
			if (f > 0.3)
				m_fAttackRear = max(m_fAttackRear, f);
		}

		if (front > 0)
		{
			if (front > 0.3)
				m_fAttackRight = max(m_fAttackRight, front);
		}
		else
		{
			float f = fabs(front);
			if (f > 0.3)
				m_fAttackLeft = max(m_fAttackLeft, f);
		}
	}
}

int CHudHealth::DrawPain(float flTime)
{
	if (!(m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight))
		return 1;

	int r, g, b;
	int x, y, a, shade;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;
	
	// SPR_Draw top
	if (m_fAttackFront > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackFront, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 - SPR_Width(m_hSprite, 0)/2;
		y = ScreenHeight/2 - SPR_Height(m_hSprite,0) * 3;
		SPR_DrawAdditive(0, x, y, NULL);
		m_fAttackFront = max( 0, m_fAttackFront - fFade );
	} else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackRight, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 + SPR_Width(m_hSprite, 1) * 2;
		y = ScreenHeight/2 - SPR_Height(m_hSprite,1)/2;
		SPR_DrawAdditive(1, x, y, NULL);
		m_fAttackRight = max( 0, m_fAttackRight - fFade );
	} else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackRear, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 - SPR_Width(m_hSprite, 2)/2;
		y = ScreenHeight/2 + SPR_Height(m_hSprite,2) * 2;
		SPR_DrawAdditive(2, x, y, NULL);
		m_fAttackRear = max( 0, m_fAttackRear - fFade );
	} else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		GetPainColor(r,g,b);
		shade = a * max( m_fAttackLeft, 0.5 );
		ScaleColors(r, g, b, shade);
		SPR_Set(m_hSprite, r, g, b );

		x = ScreenWidth/2 - SPR_Width(m_hSprite, 3) * 3;
		y = ScreenHeight/2 - SPR_Height(m_hSprite,3)/2;
		SPR_DrawAdditive(3, x, y, NULL);

		m_fAttackLeft = max( 0, m_fAttackLeft - fFade );
	} else
		m_fAttackLeft = 0;

	return 1;
}

int CHudHealth::DrawDamage(float flTime)
{
	int r, g, b, a;
	DAMAGE_IMAGE *pdmg;

	if (!m_bitsDamage)
		return 1;

	UnpackRGB(r,g,b, RGB_YELLOWISH);
	
	a = (int)( fabs(sin(flTime*2)) * 256.0);

	ScaleColors(r, g, b, a);
	int i;
	// Draw all the items
	for (i = 0; i < NUM_DMG_TYPES; i++)
	{
		if (m_bitsDamage & giDmgFlags[i])
		{
			pdmg = &m_dmg[i];
			SPR_Set(gHUD.GetSprite(m_HUD_dmg_bio + i), r, g, b );
			SPR_DrawAdditive(0, pdmg->x, pdmg->y, &gHUD.GetSpriteRect(m_HUD_dmg_bio + i));
		}
	}


	// check for bits that should be expired
	for ( i = 0; i < NUM_DMG_TYPES; i++ )
	{
		DAMAGE_IMAGE *pdmg = &m_dmg[i];

		if ( m_bitsDamage & giDmgFlags[i] )
		{
			pdmg->fExpire = min( flTime + DMG_IMAGE_LIFE, pdmg->fExpire );

			if ( pdmg->fExpire <= flTime		// when the time has expired
				&& a < 40 )						// and the flash is at the low point of the cycle
			{
				pdmg->fExpire = 0;

				int y = pdmg->y;
				pdmg->x = pdmg->y = 0;

				// move everyone above down
				for (int j = 0; j < NUM_DMG_TYPES; j++)
				{
					pdmg = &m_dmg[j];
					if ((pdmg->y) && (pdmg->y < y))
						pdmg->y += giDmgHeight;

				}

				m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
			}
		}
	}

	return 1;
}
 

void CHudHealth::UpdateTiles(float flTime, long bitsDamage)
{	
	DAMAGE_IMAGE *pdmg;

	// Which types are new?
	long bitsOn = ~m_bitsDamage & bitsDamage;
	
	for (int i = 0; i < NUM_DMG_TYPES; i++)
	{
		pdmg = &m_dmg[i];

		// Is this one already on?
		if (m_bitsDamage & giDmgFlags[i])
		{
			pdmg->fExpire = flTime + DMG_IMAGE_LIFE; // extend the duration
			if (!pdmg->fBaseline)
				pdmg->fBaseline = flTime;
		}

		// Are we just turning it on?
		if (bitsOn & giDmgFlags[i])
		{
			// put this one at the bottom
			pdmg->x = giDmgWidth/8;
			pdmg->y = ScreenHeight - giDmgHeight * 2;
			pdmg->fExpire=flTime + DMG_IMAGE_LIFE;
			
			// move everyone else up
			for (int j = 0; j < NUM_DMG_TYPES; j++)
			{
				if (j == i)
					continue;

				pdmg = &m_dmg[j];
				if (pdmg->y)
					pdmg->y -= giDmgHeight;

			}
			pdmg = &m_dmg[i];
		}	
	}	

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bitsDamage;
}

void CHudHealth :: DrawPlayerLocation( void )
{
	DrawConsoleString( 30, 30, g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].location );
}

void CHudHealth :: DrawRadarDot(int x, int y, int size, int r, int g, int b, int a)
{
	FillRGBA(62.5f + x - size/2.0f, 62.5f + y - size/2.0f, size, size, r, g, b, a);
}

Vector2D CHudHealth :: WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles  )
{
	Vector2D diff = vObjectOrigin.Make2D() - vPlayerOrigin.Make2D();

	// Supply epsilon values to avoid divide-by-zero
	if(diff.x == 0)
		diff.x = 0.00001f;
	if(diff.y == 0)
		diff.y = 0.00001f;

	int iMaxRadius = (m_hrad.right - m_hrad.left) / 2.0f;

	float flOffset = atan(diff.y / diff.x) * 180.0f / M_PI;

	if ((diff.x < 0) && (diff.y >= 0))
		flOffset += 180;
	else if ((diff.x < 0) && (diff.y < 0))
		flOffset += 180;
	else if ((diff.x >= 0) && (diff.y < 0))
		flOffset += 360;

	// this magic 32.0f just scales position on radar
	float iRadius = -diff.Length() / 32.0f;
	if( -iRadius > iMaxRadius)
		iRadius = -iMaxRadius;

	flOffset = (vAngles.y - flOffset) * M_PI / 180.0f;

	// transform origin difference to radar source
	Vector2D new_diff;
	new_diff.x = -iRadius * sin(flOffset);
	new_diff.y =  iRadius * cos(flOffset);

	return new_diff;
}

void CHudHealth :: DrawRadar( float flTime )
{
	int iTeamNumber = g_PlayerExtraInfo[ gHUD.m_Scoreboard.m_iPlayerNum ].teamnumber;

	if( g_PlayerExtraInfo[ gHUD.m_Scoreboard.m_iPlayerNum ].dead )
		return;

	if( cl_radartype->value )
	{
		SPR_Set(m_hRadaropaque, 200, 200, 200);
		SPR_DrawHoles(0, 0, 0, &m_hradopaque);
	}
	else
	{
		SPR_Set( m_hRadar, 25, 75, 25 );
		SPR_DrawAdditive( 0, 0, 0, &m_hrad );
	}
	for(int i = 0; i < 33; i++)
	{
		if( i == gHUD.m_Scoreboard.m_iPlayerNum || g_PlayerExtraInfo[i].dead)
			continue;

		if( g_PlayerExtraInfo[i].teamnumber != iTeamNumber )
			continue;

		Vector2D pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[i].origin, gHUD.m_vecAngles);
		if( g_PlayerExtraInfo[i].has_c4 )
			DrawRadarDot( pos.x, pos.y, 2, 255, 0, 0, 255 );
		else
			DrawRadarDot( pos.x, pos.y, 2, 0, 255, 0, 255 );
	}

	if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == 1)
	// draw bomb for T
	{
		if( g_PlayerExtraInfo[33].radarflashon )
		{
			Vector2D pos = WorldToRadar(gHUD.m_vecOrigin, g_PlayerExtraInfo[33].origin, gHUD.m_vecAngles);

			// TODO: make it flash
			DrawRadarDot( pos.x, pos.y, 4, 255, 0, 0, 255 );
		}
	}
	else
	// draw hostages for CT
	{
		for( int i = 0; i < MAX_HOSTAGES; i++ )
		{
			if( g_HostageInfo[i].dead )
				continue;
			if( !g_HostageInfo[i].radarflashon )
				continue;

			Vector2D pos = WorldToRadar( gHUD.m_vecOrigin, g_HostageInfo[i].origin, gHUD.m_vecAngles );

			// TODO: make it flash
			DrawRadarDot( pos.x, pos.y, 4, 250, 0, 0, 255 );
		}
	}
}

int CHudHealth :: MsgFunc_ClCorpse(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	char szModel[64];

	char *pModel = READ_STRING();
	Vector origin;
	origin.x = READ_LONG() / 128.0f;
	origin.y = READ_LONG() / 128.0f;
	origin.z = READ_LONG() / 128.0f;
	Vector angles;
	angles.x = READ_COORD();
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	float delay = READ_LONG() / 100.0f;
	int sequence = READ_BYTE();
	int classID = READ_BYTE();
	int teamID = READ_BYTE();
	int playerID = READ_BYTE();

	if( !cl_minmodels->value )
	{
		if( !strstr(pModel, "models/") )
		{
			snprintf(szModel, sizeof(szModel), "models/player/%s/%s.mdl", pModel, pModel );
		}
	}
	else if( teamID == 1 ) // terrorists
	{
		int modelidx = cl_min_t->value;
		if( BIsValidTModelIndex(modelidx) )
			strncpy(szModel, sPlayerModelFiles[modelidx], sizeof(szModel));
		else strncpy(szModel, sPlayerModelFiles[1], sizeof(szModel) ); // set leet.mdl
	}
	else if( teamID == 2 ) // ct
	{
		int modelidx = cl_min_ct->value;

		if( g_PlayerExtraInfo[playerID].vip )
			strncpy( szModel, sPlayerModelFiles[3], sizeof(szModel) ); // vip.mdl
		else if( BIsValidCTModelIndex( modelidx ) )
			strncpy( szModel, sPlayerModelFiles[ modelidx ], sizeof(szModel));
		else strncpy( szModel, sPlayerModelFiles[2], sizeof(szModel) ); // gign.mdl
	}
	else strncpy( szModel, sPlayerModelFiles[0], sizeof(szModel) ); // player.mdl

	CreateCorpse( &origin, &angles, szModel, delay, sequence, classID );

   return 1;
}

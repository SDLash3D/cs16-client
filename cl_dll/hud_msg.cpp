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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "rain.h"

#include <cstring>

#define MAX_CLIENTS 32

extern BEAM *pBeam;
extern BEAM *pBeam2;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

int CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
	return 1;
}

int CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;

	return 1;
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud::MsgFunc_ReceiveW(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iWeatherType = READ_BYTE();

	if( iWeatherType == 0 )
	{
		ResetRain();
		return 1;
	}

	Rain.distFromPlayer = 500;
	Rain.dripsPerSecond = 500;
	Rain.windX = Rain.windY = 30;
	Rain.randX = Rain.randY = 0;
	Rain.weatherMode = iWeatherType - 1;
	Rain.globalHeight = 100;

	return 1;
}

int CHud::MsgFunc_BombDrop(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	g_PlayerExtraInfo[33].origin.x = READ_COORD();
	g_PlayerExtraInfo[33].origin.y = READ_COORD();
	g_PlayerExtraInfo[33].origin.z = READ_COORD();
	g_PlayerExtraInfo[33].playerclass = 1;
	int Flag = READ_BYTE();
	if( Flag == 1)
	{
		g_PlayerExtraInfo[33].radarflashon = 1;
		g_PlayerExtraInfo[33].radarflashes = 99999;
		g_PlayerExtraInfo[33].radarflash = gHUD.m_flTime;
		gHUD.m_Timer.m_iFlags = 0;
	}
	g_PlayerExtraInfo[33].dead = 0;
	strcpy(g_PlayerExtraInfo[33].teamname, "TERRORIST");

	if ( g_PlayerExtraInfo[33].playerclass == 1 )
		m_Timer.m_iFlags = 0;

   return 1;
}

int CHud::MsgFunc_BombPickup(const char *pszName, int iSize, void *pbuf)
{
	g_PlayerExtraInfo[33].radarflashon = 0;
	g_PlayerExtraInfo[33].radarflash = 0.0f;
	g_PlayerExtraInfo[33].radarflashes = 0;
	g_PlayerExtraInfo[33].dead = 1;

	return 1;
}

int CHud::MsgFunc_HostagePos(const char *pszName, int iSize, void *pbuf)
{

	BEGIN_READ(pbuf, iSize);
	int Flag = READ_BYTE();
	int idx = READ_BYTE();
	if ( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].origin.x = READ_COORD();
		g_HostageInfo[idx].origin.y = READ_COORD();
		g_HostageInfo[idx].origin.z = READ_COORD();
		if ( Flag == 1 )
		{
			g_HostageInfo[idx].radarflash = gHUD.m_flTime;
			g_HostageInfo[idx].radarflashon = 1;
			g_HostageInfo[idx].radarflashes = 99999;
		}
		strcpy(g_HostageInfo[idx].teamname, "CT");
		g_HostageInfo[idx].dead = 0;
	}

	return 1;
}

int CHud::MsgFunc_HostageK(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int idx = READ_BYTE();
	if ( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].dead = 1;
		g_HostageInfo[idx].radarflash = gHUD.m_flTime;
		g_HostageInfo[idx].radarflashes = 15;
	}

	return 1;
}

int CHud::MsgFunc_ShadowIdx(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int idx = READ_BYTE();
	return 1;
}

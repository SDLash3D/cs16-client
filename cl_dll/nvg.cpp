/*
nvg.cpp - Night Vision Googles
Copyright (C) 2015 a1batross
*/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "dlight.h"

DECLARE_MESSAGE(m_NVG, NVGToggle)
DECLARE_COMMAND(m_NVG, NVGAdjustDown)
DECLARE_COMMAND(m_NVG, NVGAdjustUp)

int CHudNVG::Init()
{
	HOOK_MESSAGE(NVGToggle)
	HOOK_COMMAND("+nvgadjust", NVGAdjustUp);
	HOOK_COMMAND("-nvgadjust", NVGAdjustDown);

	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	m_iEnable = 0;
	m_iAlpha = 110; // 220 is max, 30 is min

   return 1;
}

int CHudNVG::Draw(float flTime)
{
	if ( !m_iEnable || gEngfuncs.IsSpectateOnly() )
	{
		return 1;
	}
	gEngfuncs.pfnFillRGBABlend(0, 0, ScreenWidth, ScreenHeight, 50, 225, 50, m_iAlpha);

	// draw a dynamic light on player's origin
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight ( 0 );
	VectorCopy ( gHUD.m_vecOrigin, dl->origin );
	dl->radius = gEngfuncs.pfnRandomFloat( 750, 800 );
	dl->die = flTime + 0.1;
	dl->color.r = 50;
	dl->color.g = 255;
	dl->color.b = 50;
	return 1;
}

int CHudNVG::MsgFunc_NVGToggle(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_iEnable = READ_BYTE();
	return 1;
}

void CHudNVG::UserCmd_NVGAdjustDown()
{
	m_iAlpha += 20;
	if( m_iAlpha > 220 ) m_iAlpha = 220;
}

void CHudNVG::UserCmd_NVGAdjustUp()
{
	m_iAlpha -= 20;
	if( m_iAlpha < 30 ) m_iAlpha = 30;
}

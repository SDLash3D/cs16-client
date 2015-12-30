/*
hud_overlays.cpp - HUD Overlays
Copyright (C) 2015 a1batross
*/

#include "hud.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "cl_util.h"

void Quad(float x1, float y1, float x2, float y2)
{
	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3f(x1, y1, 0);

	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3f(x1, y2, 0);

	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3f(x2, y2, 0);

	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3f(x2, y1, 0);
}


int CHudSniperScope::Init()
{
	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;

   return 1;
}

int CHudSniperScope::VidInit()
{
	m_iScopeArc[0] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_nw.tga", NULL, 0, TF_NEAREST);
	m_iScopeArc[1] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_ne.tga", NULL, 0, TF_NEAREST);
	m_iScopeArc[2] = gRenderAPI.GL_LoadTexture("sprites/scope_arc.tga", NULL, 0, TF_NEAREST);
	m_iScopeArc[3] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_sw.tga", NULL, 0, TF_NEAREST);
	blackTex = gRenderAPI.GL_FindTexture("*black");
	left = (ScreenWidth - ScreenHeight)/2;
	right = left + ScreenHeight;
	centerx = ScreenWidth/2;
	centery = ScreenHeight/2;

   return 1;
}

int CHudSniperScope::Draw(float flTime)
{
	if(gHUD.m_iFOV > 80)
		return 1;
	gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
	gEngfuncs.pTriAPI->Brightness(1.0);
	gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	gRenderAPI.GL_Bind(0, m_iScopeArc[0]);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	Quad(left, 0, centerx, centery);
	gEngfuncs.pTriAPI->End();

	gRenderAPI.GL_Bind(0, m_iScopeArc[1]);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	Quad(centerx, 0, right, centery);
	gEngfuncs.pTriAPI->End();

	gRenderAPI.GL_Bind(0, m_iScopeArc[2]);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	Quad(centerx, centery, right, ScreenHeight);
	gEngfuncs.pTriAPI->End();

	gRenderAPI.GL_Bind(0, m_iScopeArc[3]);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	Quad(left, centery, centerx, ScreenHeight);
	gEngfuncs.pTriAPI->End();

	gRenderAPI.GL_Bind(0, blackTex);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	Quad(0, 0, left, ScreenHeight);
	gEngfuncs.pTriAPI->End();

	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	Quad(right, 0, ScreenWidth, ScreenHeight);
	gEngfuncs.pTriAPI->End();

   return 1;
}

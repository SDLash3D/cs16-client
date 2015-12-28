//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OVERVIEW_H
#define OVERVIEW_H
#pragma once


//-----------------------------------------------------------------------------
// Purpose: Handles the drawing of the top-down map and all the things on it
//-----------------------------------------------------------------------------
class CHudOverview : public CHudBase
{
public:
	int Init();
	int VidInit();

	int Draw(float flTime);
	void InitHUDData( void );

private:
	SptiteHandle_t m_hsprPlayer;
	SptiteHandle_t m_hsprViewcone;
};


#endif // OVERVIEW_H

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
// Ammo.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pm_shared.h"

#include <string.h>
#include <stdio.h>

#include "ammohistory.h"
#include "eventscripts.h"
#include "com_weapons.h"

#include <math.h>
//#include "vgui_TeamFortressViewport.h"

WEAPON *gpActiveSel;	// NULL means off, 1 means just the menu bar, otherwise
						// this points to the active weapon menu item
WEAPON *gpLastSel;		// Last weapon menu selection 

client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);

WeaponsResource gWR;

int g_weaponselect = 0;
int g_iShotsFired;

static inline void SineCosine (float rad, float *sin, float *cos)
{
#if defined (_WIN32) && defined (_MSC_VER)
   __asm
   {
      fld dword ptr[rad]
      fsincos
      mov ebx, [cos]
      fstp dword ptr[ebx]
      mov ebx, [sin]
      fstp dword ptr[ebx]
   }
#elif defined (__linux__) || defined (GCC) || defined (__APPLE__)
   register double _cos, _sin;
   __asm __volatile__ ("fsincos" : "=t" (_cos), "=u" (_sin) : "0" (rad));

   *cos = _cos;
   *sin = _sin;
#else
   *sin = sinf (rad);
   *cos = cosf (rad);
#endif
}

void WeaponsResource :: LoadAllWeaponSprites( void )
{
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if ( rgWeapons[i].iId )
			LoadWeaponSprites( &rgWeapons[i] );
	}
}

int WeaponsResource :: CountAmmo( int iId ) 
{ 
	if ( iId < 0 )
		return 0;

	return riAmmo[iId];
}

int WeaponsResource :: HasAmmo( WEAPON *p )
{
	if ( !p )
		return FALSE;

	// weapons with no max ammo can always be selected
	if ( p->iMax1 == -1 )
		return TRUE;

	return (p->iAmmoType == -1) || p->iClip > 0 || CountAmmo(p->iAmmoType) 
		|| CountAmmo(p->iAmmo2Type) || ( p->iFlags & WEAPON_FLAGS_SELECTONEMPTY );
}


void WeaponsResource :: LoadWeaponSprites( WEAPON *pWeapon )
{
	int i, iRes;

	if (ScreenWidth < 640)
		iRes = 320;
	else
		iRes = 640;

	char sz[128];

	if ( !pWeapon )
		return;

	memset( &pWeapon->rcActive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcInactive, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo, 0, sizeof(wrect_t) );
	memset( &pWeapon->rcAmmo2, 0, sizeof(wrect_t) );
	pWeapon->hInactive = 0;
	pWeapon->hActive = 0;
	pWeapon->hAmmo = 0;
	pWeapon->hAmmo2 = 0;

	sprintf(sz, "sprites/%s.txt", pWeapon->szName);
	client_sprite_t *pList = SPR_GetList(sz, &i);

	if (!pList)
		return;

	client_sprite_t *p;
	
	p = GetSpriteList( pList, "crosshair", iRes, i );
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hCrosshair = SPR_Load(sz);
		pWeapon->rcCrosshair = p->rc;
	}
	else
		pWeapon->hCrosshair = 0;

	p = GetSpriteList(pList, "autoaim", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAutoaim = SPR_Load(sz);
		pWeapon->rcAutoaim = p->rc;
	}
	else
		pWeapon->hAutoaim = 0;

	p = GetSpriteList( pList, "zoom", iRes, i );
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedCrosshair = SPR_Load(sz);
		pWeapon->rcZoomedCrosshair = p->rc;
	}
	else
	{
		pWeapon->hZoomedCrosshair = pWeapon->hCrosshair; //default to non-zoomed crosshair
		pWeapon->rcZoomedCrosshair = pWeapon->rcCrosshair;
	}

	p = GetSpriteList(pList, "zoom_autoaim", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hZoomedAutoaim = SPR_Load(sz);
		pWeapon->rcZoomedAutoaim = p->rc;
	}
	else
	{
		pWeapon->hZoomedAutoaim = pWeapon->hZoomedCrosshair;  //default to zoomed crosshair
		pWeapon->rcZoomedAutoaim = pWeapon->rcZoomedCrosshair;
	}

	p = GetSpriteList(pList, "weapon", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hInactive = SPR_Load(sz);
		pWeapon->rcInactive = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hInactive = 0;

	p = GetSpriteList(pList, "weapon_s", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hActive = SPR_Load(sz);
		pWeapon->rcActive = p->rc;
	}
	else
		pWeapon->hActive = 0;

	p = GetSpriteList(pList, "ammo", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo = SPR_Load(sz);
		pWeapon->rcAmmo = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo = 0;

	p = GetSpriteList(pList, "ammo2", iRes, i);
	if (p)
	{
		sprintf(sz, "sprites/%s.spr", p->szSprite);
		pWeapon->hAmmo2 = SPR_Load(sz);
		pWeapon->rcAmmo2 = p->rc;

		gHR.iHistoryGap = max( gHR.iHistoryGap, pWeapon->rcActive.bottom - pWeapon->rcActive.top );
	}
	else
		pWeapon->hAmmo2 = 0;

}

// Returns the first weapon for a given slot.
WEAPON *WeaponsResource :: GetFirstPos( int iSlot )
{
	WEAPON *pret = NULL;

	for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
	{
		if ( rgSlots[iSlot][i] /*&& HasAmmo( rgSlots[iSlot][i] )*/ )
		{
			pret = rgSlots[iSlot][i];
			break;
		}
	}

	return pret;
}


WEAPON* WeaponsResource :: GetNextActivePos( int iSlot, int iSlotPos )
{
	if ( iSlotPos >= MAX_WEAPON_POSITIONS || iSlot >= MAX_WEAPON_SLOTS )
		return NULL;

	WEAPON *p = gWR.rgSlots[ iSlot ][ iSlotPos+1 ];
	
	if ( !p || !gWR.HasAmmo(p) )
		return GetNextActivePos( iSlot, iSlotPos + 1 );

	return p;
}


int giBucketHeight, giBucketWidth, giABHeight, giABWidth; // Ammo Bar width and height

SptiteHandle_t ghsprBuckets;					// Sprite for top row of weapons menu

DECLARE_MESSAGE(m_Ammo, CurWeapon );	// Current weapon and clip
DECLARE_MESSAGE(m_Ammo, WeaponList);	// new weapon type
DECLARE_MESSAGE(m_Ammo, AmmoX);			// update known ammo type's count
DECLARE_MESSAGE(m_Ammo, AmmoPickup);	// flashes an ammo pickup record
DECLARE_MESSAGE(m_Ammo, WeapPickup);    // flashes a weapon pickup record
DECLARE_MESSAGE(m_Ammo, HideWeapon);	// hides the weapon, ammo, and crosshair displays temporarily
DECLARE_MESSAGE(m_Ammo, ItemPickup);
DECLARE_MESSAGE(m_Ammo, Crosshair);
DECLARE_MESSAGE(m_Ammo, Brass);

DECLARE_COMMAND(m_Ammo, Slot1);
DECLARE_COMMAND(m_Ammo, Slot2);
DECLARE_COMMAND(m_Ammo, Slot3);
DECLARE_COMMAND(m_Ammo, Slot4);
DECLARE_COMMAND(m_Ammo, Slot5);
DECLARE_COMMAND(m_Ammo, Slot6);
DECLARE_COMMAND(m_Ammo, Slot7);
DECLARE_COMMAND(m_Ammo, Slot8);
DECLARE_COMMAND(m_Ammo, Slot9);
DECLARE_COMMAND(m_Ammo, Slot10);
DECLARE_COMMAND(m_Ammo, Close);
DECLARE_COMMAND(m_Ammo, NextWeapon);
DECLARE_COMMAND(m_Ammo, PrevWeapon);
DECLARE_COMMAND(m_Ammo, Adjust_Crosshair);
DECLARE_COMMAND(m_Ammo, Rebuy);
DECLARE_COMMAND(m_Ammo, Autobuy);

// width of ammo fonts
#define AMMO_SMALL_WIDTH 10
#define AMMO_LARGE_WIDTH 20

#define HISTORY_DRAW_TIME	"5"

int CHudAmmo::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(CurWeapon);
	HOOK_MESSAGE(WeaponList);
	HOOK_MESSAGE(AmmoPickup);
	HOOK_MESSAGE(WeapPickup);
	HOOK_MESSAGE(ItemPickup);
	HOOK_MESSAGE(HideWeapon);
	HOOK_MESSAGE(AmmoX);
	HOOK_MESSAGE(Crosshair);
	HOOK_MESSAGE(Brass);

	HOOK_COMMAND("slot1", Slot1);
	HOOK_COMMAND("slot2", Slot2);
	HOOK_COMMAND("slot3", Slot3);
	HOOK_COMMAND("slot4", Slot4);
	HOOK_COMMAND("slot5", Slot5);
	HOOK_COMMAND("slot6", Slot6);
	HOOK_COMMAND("slot7", Slot7);
	HOOK_COMMAND("slot8", Slot8);
	HOOK_COMMAND("slot9", Slot9);
	HOOK_COMMAND("slot10", Slot10);
	HOOK_COMMAND("cancelselect", Close);
	HOOK_COMMAND("invnext", NextWeapon);
	HOOK_COMMAND("invprev", PrevWeapon);
	HOOK_COMMAND("adjust_crosshair", Adjust_Crosshair);
	HOOK_COMMAND("rebuy", Rebuy);
	HOOK_COMMAND("autobuy", Autobuy);

	Reset();

	CVAR_CREATE( "hud_drawhistory_time", HISTORY_DRAW_TIME, 0 );
	CVAR_CREATE( "hud_fastswitch", "0", FCVAR_ARCHIVE );		// controls whether or not weapons can be selected in one keypress
	CVAR_CREATE( "cl_observercrosshair", "1", 0 );
	m_pClCrosshairColor = CVAR_CREATE( "cl_crosshair_color", "50 250 50", FCVAR_ARCHIVE );
	m_pClCrosshairTranslucent = CVAR_CREATE( "cl_crosshair_translucent", "1", FCVAR_ARCHIVE );
	m_pClCrosshairSize = CVAR_CREATE( "cl_crosshair_size", "auto", FCVAR_ARCHIVE );
	m_pClDynamicCrosshair = CVAR_CREATE("cl_dynamiccrosshair", "1", FCVAR_ARCHIVE);

	m_iFlags |= HUD_ACTIVE; //!!!
	m_R = 50;
	m_G = 250;
	m_B = 50;
	m_iAlpha = 200;

	m_cvarB = m_cvarR = m_cvarG = -1;
	m_iCurrentCrosshair = 0;
	m_bAdditive = 1;
	m_iCrosshairScaleBase = 1024;
	m_bDrawCrosshair = true;

	gWR.Init();
	gHR.Init();

	return 1;
};

void CHudAmmo::Reset(void)
{
	m_fFade = 0;
	m_iFlags |= HUD_ACTIVE; //!!!

	gpActiveSel = NULL;
	gHUD.m_iHideHUDDisplay = 0;

	gWR.Reset();
	gHR.Reset();

	//	VidInit();

}

int CHudAmmo::VidInit(void)
{
	// Load sprites for buckets (top row of weapon menu)
	m_HUD_bucket0 = gHUD.GetSpriteIndex( "bucket1" );
	m_HUD_selection = gHUD.GetSpriteIndex( "selection" );

	ghsprBuckets = gHUD.GetSprite(m_HUD_bucket0);
	giBucketWidth = gHUD.GetSpriteRect(m_HUD_bucket0).right - gHUD.GetSpriteRect(m_HUD_bucket0).left;
	giBucketHeight = gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top;

	gHR.iHistoryGap = max( gHR.iHistoryGap, gHUD.GetSpriteRect(m_HUD_bucket0).bottom - gHUD.GetSpriteRect(m_HUD_bucket0).top);

	// If we've already loaded weapons, let's get new sprites
	gWR.LoadAllWeaponSprites();

	if (ScreenWidth >= 640)
	{
		giABWidth = 20;
		giABHeight = 4;
	}
	else
	{
		giABWidth = 10;
		giABHeight = 2;
	}

	return 1;
}

//
// Think:
//  Used for selection of weapon menu item.
//
void CHudAmmo::Think(void)
{
	if ( gHUD.m_fPlayerDead )
		return;

	if ( gHUD.m_iWeaponBits != gWR.iOldWeaponBits )
	{
		gWR.iOldWeaponBits = gHUD.m_iWeaponBits;

		for (int i = 0; i < MAX_WEAPONS-1; i++ )
		{
			WEAPON *p = gWR.GetWeapon(i);

			if ( p )
			{
				if ( gHUD.m_iWeaponBits & ( 1 << p->iId ) )
				{
					gWR.PickupWeapon( p );
				}
				else
				{
					gWR.DropWeapon( p );
				}
			}
		}
	}

	if (!gpActiveSel)
		return;

	// has the player selected one?
	if (gHUD.m_iKeyBits & IN_ATTACK)
	{
		if (gpActiveSel != (WEAPON *)1)
		{
			ServerCmd(gpActiveSel->szName);
			g_weaponselect = gpActiveSel->iId;
		}

		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		gHUD.m_iKeyBits &= ~IN_ATTACK;

		PlaySound("common/wpn_select.wav", 1);
	}

}

//
// Helper function to return a Ammo pointer from id
//

SptiteHandle_t* WeaponsResource :: GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		if ( rgWeapons[i].iAmmoType == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo;
			return &rgWeapons[i].hAmmo;
		}
		else if ( rgWeapons[i].iAmmo2Type == iAmmoId )
		{
			rect = rgWeapons[i].rcAmmo2;
			return &rgWeapons[i].hAmmo2;
		}
	}

	return NULL;
}


// Menu Selection Code

void WeaponsResource :: SelectSlot( int iSlot, int fAdvance, int iDirection )
{
	if ( gHUD.m_Menu.m_fMenuDisplayed && (fAdvance == FALSE) && (iDirection == 1) )	
	{ // menu is overriding slot use commands
		gHUD.m_Menu.SelectMenuItem( iSlot + 1 );  // slots are one off the key numbers
		return;
	}

	if ( iSlot > MAX_WEAPON_SLOTS )
		return;

	if ( gHUD.m_fPlayerDead || gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
		return;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return;

	if ( ! ( gHUD.m_iWeaponBits & ~(1<<(WEAPON_SUIT)) ))
		return;

	WEAPON *p = NULL;
	bool fastSwitch = CVAR_GET_FLOAT( "hud_fastswitch" ) != 0;

	if ( (gpActiveSel == NULL) || (gpActiveSel == (WEAPON *)1) || (iSlot != gpActiveSel->iSlot) )
	{
		PlaySound( "common/wpn_hudon.wav", 1 );
		p = GetFirstPos( iSlot );

		if ( p && fastSwitch ) // check for fast weapon switch mode
		{
			// if fast weapon switch is on, then weapons can be selected in a single keypress
			// but only if there is only one item in the bucket
			WEAPON *p2 = GetNextActivePos( p->iSlot, p->iSlotPos );
			if ( !p2 )
			{	// only one active item in bucket, so change directly to weapon
				ServerCmd( p->szName );
				g_weaponselect = p->iId;
				return;
			}
		}
	}
	else
	{
		PlaySound("common/wpn_moveselect.wav", 1);
		if ( gpActiveSel )
			p = GetNextActivePos( gpActiveSel->iSlot, gpActiveSel->iSlotPos );
		if ( !p )
			p = GetFirstPos( iSlot );
	}

	
	if ( !p )  // no selection found
	{
		// just display the weapon list, unless fastswitch is on just ignore it
		if ( !fastSwitch )
			gpActiveSel = (WEAPON *)1;
		else
			gpActiveSel = NULL;
	}
	else 
		gpActiveSel = p;
}

//------------------------------------------------------------------------
// Message Handlers
//------------------------------------------------------------------------

//
// AmmoX  -- Update the count of a known type of ammo
// 
int CHudAmmo::MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	gWR.SetAmmo( iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();
	int iCount = READ_BYTE();

	// Add ammo to the history
	gHR.AddToHistory( HISTSLOT_AMMO, iIndex, abs(iCount) );

	return 1;
}

int CHudAmmo::MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iIndex = READ_BYTE();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_WEAP, iIndex );

	return 1;
}

int CHudAmmo::MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	const char *szName = READ_STRING();

	// Add the weapon to the history
	gHR.AddToHistory( HISTSLOT_ITEM, szName );

	return 1;
}


int CHudAmmo::MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	gHUD.m_iHideHUDDisplay = READ_BYTE();

	if (gEngfuncs.IsSpectateOnly())
		return 1;

	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		static wrect_t nullrc;
		gpActiveSel = NULL;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
	}
	else
	{
		if ( m_pWeapon )
			SetCrosshair( m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255 );
	}

	return 1;
}

// 
//  CurWeapon: Update hud state with the current weapon and clip count. Ammo
//  counts are updated with AmmoX. Server assures that the Weapon ammo type 
//  numbers match a real ammo type.
//
int CHudAmmo::MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf )
{
	static wrect_t nullrc;
	int fOnTarget = FALSE;

	BEGIN_READ( pbuf, iSize );

	int iState = READ_BYTE();
	int iId = READ_CHAR();
	int iClip = READ_CHAR();

	// detect if we're also on target
	if ( iState > 1 )
	{
		fOnTarget = TRUE;
	}

	if ( iId < 1 )
	{
		SetCrosshair(0, nullrc, 0, 0, 0);
		return 0;
	}

	if ( g_iUser1 != OBS_IN_EYE )
	{
		// Is player dead???
		if ((iId == -1) && (iClip == -1))
		{
			gHUD.m_fPlayerDead = TRUE;
			gpActiveSel = NULL;
			return 1;
		}
		gHUD.m_fPlayerDead = FALSE;
	}

	WEAPON *pWeapon = gWR.GetWeapon( iId );

	if ( !pWeapon )
		return 0;

	if ( iClip < -1 )
		pWeapon->iClip = abs(iClip);
	else
		pWeapon->iClip = iClip;


	if ( iState == 0 )	// we're not the current weapon, so update no more
		return 1;

	m_pWeapon = pWeapon;

	/*if ( gHUD.m_iFOV >= 90 )
	{ // normal crosshairs
		if (fOnTarget && m_pWeapon->hAutoaim)
			SetCrosshair(m_pWeapon->hAutoaim, m_pWeapon->rcAutoaim, 255, 255, 255);
		else
			SetCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 255, 255);
	}
	else*/
	if( gHUD.m_iFOV <= 75 )
	{ // zoomed crosshairs
		if (fOnTarget && m_pWeapon->hZoomedAutoaim)
			SetCrosshair(m_pWeapon->hZoomedAutoaim, m_pWeapon->rcZoomedAutoaim, 255, 255, 255);
		else
			SetCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, 255, 255, 255);

	}
	else
	{
		wrect_t nullrc;
      nullrc.bottom = nullrc.right = nullrc.left = nullrc.top = 0;

		SetCrosshair( 0, nullrc, 0, 0, 0);
	}


	m_fFade = 200.0f; //!!!
	m_iFlags |= HUD_ACTIVE;
	
	return 1;
}

//
// WeaponList -- Tells the hud about a new weapon type.
//
int CHudAmmo::MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	WEAPON Weapon;

	strcpy( Weapon.szName, READ_STRING() );
	Weapon.iAmmoType = (int)READ_CHAR();	
	
	Weapon.iMax1 = READ_BYTE();
	if (Weapon.iMax1 == 255)
		Weapon.iMax1 = -1;

	Weapon.iAmmo2Type = READ_CHAR();
	Weapon.iMax2 = READ_BYTE();
	if (Weapon.iMax2 == 255)
		Weapon.iMax2 = -1;

	Weapon.iSlot = READ_CHAR();
	Weapon.iSlotPos = READ_CHAR();
	Weapon.iId = READ_CHAR();
	Weapon.iFlags = READ_BYTE();
	Weapon.iClip = 0;

	gWR.AddWeapon( &Weapon );

	return 1;

}

int CHudAmmo::MsgFunc_Crosshair(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	if( READ_BYTE() > 0)
	{
		m_bDrawCrosshair = true;
	}
	else
	{
		m_bDrawCrosshair = false;
	}
   return 1;
}

int CHudAmmo::MsgFunc_Brass( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int MessageID = READ_BYTE();

	Vector start, velocity;
	start.x = READ_COORD();
	start.y = READ_COORD();
	start.z = READ_COORD();
	READ_COORD();
	READ_COORD(); // unused data
	READ_COORD();
	velocity.x = READ_COORD();
	velocity.y = READ_COORD();
	velocity.z = READ_COORD();

	float Rotation = M_PI * READ_ANGLE() / 180.0f;
	int ModelIndex = READ_SHORT();
	int BounceSoundType = READ_BYTE();
	int Life = READ_BYTE();
	int PlayerID = READ_BYTE();

	float sin, cos, x, y;
   SineCosine ( Rotation, &sin, &cos );
	x = -9.0 * sin;
	y = 9.0 * cos;

	if( floor(cl_righthand->value) )
	{
		velocity.x += sin * -120.0;
		velocity.y += cos * 120.0;
		x = 9.0 * sin;
		y = -9.0 * cos;
	}

	start.x += x;
	start.y += y;
	EV_EjectBrass( start, velocity, Rotation, ModelIndex, BounceSoundType, Life );
	return 1;
}

//------------------------------------------------------------------------
// Command Handlers
//------------------------------------------------------------------------
// Slot button pressed
void CHudAmmo::SlotInput( int iSlot )
{
	// Let the Viewport use it first, for menus
	//if ( gViewPort && gViewPort->SlotInput( iSlot ) )
		//return;

	gWR.SelectSlot(iSlot, FALSE, 1);
}

void CHudAmmo::UserCmd_Slot1(void)
{
	SlotInput( 0 );
}

void CHudAmmo::UserCmd_Slot2(void)
{
	SlotInput( 1 );
}

void CHudAmmo::UserCmd_Slot3(void)
{
	SlotInput( 2 );
}

void CHudAmmo::UserCmd_Slot4(void)
{
	SlotInput( 3 );
}

void CHudAmmo::UserCmd_Slot5(void)
{
	SlotInput( 4 );
}

void CHudAmmo::UserCmd_Slot6(void)
{
	SlotInput( 5 );
}

void CHudAmmo::UserCmd_Slot7(void)
{
	SlotInput( 6 );
}

void CHudAmmo::UserCmd_Slot8(void)
{
	SlotInput( 7 );
}

void CHudAmmo::UserCmd_Slot9(void)
{
	SlotInput( 8 );
}

void CHudAmmo::UserCmd_Slot10(void)
{
	SlotInput( 9 );
}

void CHudAmmo::UserCmd_Adjust_Crosshair()
{
	int newCrosshair;
	int oldCrosshair = m_iCurrentCrosshair;

	if ( gEngfuncs.Cmd_Argc() <= 1 )
	{
		newCrosshair = (oldCrosshair + 1) % 5;
	}
	else
	{
		const char *arg = gEngfuncs.Cmd_Argv(1);
		newCrosshair = atoi(arg) % 10;
	}

	m_iCurrentCrosshair = newCrosshair;
	if ( newCrosshair <= 9 )
	{
		switch ( newCrosshair )
		{
		case 1:
		case 6:
			m_R = 250;
			m_G = 50;
			m_B = 50;
			break;
		case 2:
		case 7:
			m_R = 50;
			m_G = 50;
			m_B = 250;
			break;
		case 3:
		case 8:
			m_R = 250;
			m_G = 250;
			m_B = 50;
			break;
		case 4:
		case 9:
			m_R = 50;
			m_G = 250;
			m_B = 250;
			break;
		case 5:
		default:
			m_R = 50;
			m_G = 250;
			m_B = 50;
		}
		m_bAdditive = newCrosshair < 5 ? true: false;
	}
	else
	{
		m_R = 50;
		m_G = 250;
		m_B = 50;
		m_bAdditive = 1;
	}

	char s[16];
	sprintf(s, "%d %d %d", m_R, m_G, m_B);
	gEngfuncs.Cvar_Set("cl_crosshair_color", s);
	gEngfuncs.Cvar_Set("cl_crosshair_translucent", (char*)(m_bAdditive ? "1" : "0"));
}


void CHudAmmo::UserCmd_Close(void)
{
	if (gpActiveSel)
	{
		gpLastSel = gpActiveSel;
		gpActiveSel = NULL;
		PlaySound("common/wpn_hudoff.wav", 1);
	}
	else
		ClientCmd("escape");
}


// Selects the next item in the weapon menu
void CHudAmmo::UserCmd_NextWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = 0;
	int slot = 0;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos + 1;
		slot = gpActiveSel->iSlot;
	}

	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot < MAX_WEAPON_SLOTS; slot++ )
		{
			for ( ; pos < MAX_WEAPON_POSITIONS; pos++ )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp /*&& gWR.HasAmmo(wsp)*/ )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = 0;
		}

		slot = 0;  // start looking from the first slot again
	}

	gpActiveSel = NULL;
}

// Selects the previous item in the menu
void CHudAmmo::UserCmd_PrevWeapon(void)
{
	if ( gHUD.m_fPlayerDead || (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL)) )
		return;

	if ( !gpActiveSel || gpActiveSel == (WEAPON*)1 )
		gpActiveSel = m_pWeapon;

	int pos = MAX_WEAPON_POSITIONS-1;
	int slot = MAX_WEAPON_SLOTS-1;
	if ( gpActiveSel )
	{
		pos = gpActiveSel->iSlotPos - 1;
		slot = gpActiveSel->iSlot;
	}
	
	for ( int loop = 0; loop <= 1; loop++ )
	{
		for ( ; slot >= 0; slot-- )
		{
			for ( ; pos >= 0; pos-- )
			{
				WEAPON *wsp = gWR.GetWeaponSlot( slot, pos );

				if ( wsp /*&& gWR.HasAmmo(wsp)*/ )
				{
					gpActiveSel = wsp;
					return;
				}
			}

			pos = MAX_WEAPON_POSITIONS-1;
		}
		
		slot = MAX_WEAPON_SLOTS-1;
	}

	gpActiveSel = NULL;
}

void CHudAmmo::UserCmd_Autobuy()
{
	char *afile = (char*)gEngfuncs.COM_LoadFile("autobuy.txt", 5, NULL);
	char *pfile = afile;
	char token[256];
	char szCmd[1024];

	if( !pfile )
	{
		ConsolePrint( "Can't open autobuy.txt file.\n" );
		return;
	}

	strcpy(szCmd, "cl_setautobuy");

	while(pfile = gEngfuncs.COM_ParseFile( pfile, token ))
	{
		// append space first
		strcat(szCmd, " ");
		strcat(szCmd, token);
	}

	ConsolePrint(szCmd);
	gEngfuncs.pfnClientCmd(szCmd);

	gEngfuncs.COM_FreeFile( afile );
}

void CHudAmmo::UserCmd_Rebuy()
{
	char *afile = (char*)gEngfuncs.COM_LoadFile("rebuy.txt", 5, NULL);
	char *pfile = afile;
	char token[256];
	char szCmd[1024];

	if( !pfile )
	{
		ConsolePrint( "Can't open rebuy.txt file.\n" );
		return;
	}

	strcpy(szCmd, "cl_setrebuy");

	while(pfile = gEngfuncs.COM_ParseFile( pfile, token ))
	{
		// append space first
		strcat(szCmd, " ");
		strcat(szCmd, token);
	}

	ConsolePrint(szCmd);
	gEngfuncs.pfnClientCmd(szCmd);

	gEngfuncs.COM_FreeFile( afile );
}


//-------------------------------------------------------------------------
// Drawing code
//-------------------------------------------------------------------------

int CHudAmmo::Draw(float flTime)
{
	int a, x, y, r, g, b;
	int AmmoWidth;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	if ( (gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL )) )
		return 1;

	// Draw Weapon Menu
	DrawWList(flTime);

	// Draw ammo pickup history
	gHR.DrawAmmoHistory( flTime );

	if (!(m_iFlags & HUD_ACTIVE))
		return 0;

	if (!m_pWeapon)
		return 0;

	WEAPON *pw = m_pWeapon; // shorthand

	// SPR_Draw Ammo
	if ((pw->iAmmoType < 0) && (pw->iAmmo2Type < 0))
		return 0;


	int iFlags = DHN_DRAWZERO; // draw 0 values

	AmmoWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;

	a = (int) max( MIN_ALPHA, m_fFade );

	if (m_fFade > 0)
		m_fFade -= (gHUD.m_flTimeDelta * 20);

	UnpackRGB(r,g,b, RGB_YELLOWISH);

	ScaleColors(r, g, b, a );

	// Does this weapon have a clip?
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight/2;

	// Does weapon have any ammo at all?
	if (m_pWeapon->iAmmoType > 0)
	{
		int iIconWidth = m_pWeapon->rcAmmo.right - m_pWeapon->rcAmmo.left;
		
		if (pw->iClip >= 0)
		{
			// room for the number and the '|' and the current ammo
			
			x = ScreenWidth - (8 * AmmoWidth) - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, pw->iClip, r, g, b);

			wrect_t rc;
			rc.top = 0;
			rc.left = 0;
			rc.right = AmmoWidth;
			rc.bottom = 100;

			int iBarWidth =  AmmoWidth/10;

			x += AmmoWidth/2;

			UnpackRGB(r,g,b, RGB_YELLOWISH);

			// draw the | bar
			FillRGBA(x, y, iBarWidth, gHUD.m_iFontHeight, r, g, b, a);

			x += iBarWidth + AmmoWidth/2;;

			// GL Seems to need this
			ScaleColors(r, g, b, a );
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);		


		}
		else
		{
			// SPR_Draw a bullets only line
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags | DHN_3DIGITS, gWR.CountAmmo(pw->iAmmoType), r, g, b);
		}

		// Draw the ammo Icon
		int iOffset = (m_pWeapon->rcAmmo.bottom - m_pWeapon->rcAmmo.top)/8;
		SPR_Set(m_pWeapon->hAmmo, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo);
	}

	// Does weapon have seconday ammo?
	if (pw->iAmmo2Type > 0) 
	{
		int iIconWidth = m_pWeapon->rcAmmo2.right - m_pWeapon->rcAmmo2.left;

		// Do we have secondary ammo?
		if ((pw->iAmmo2Type != 0) && (gWR.CountAmmo(pw->iAmmo2Type) > 0))
		{
			y -= gHUD.m_iFontHeight + gHUD.m_iFontHeight/4;
			x = ScreenWidth - 4 * AmmoWidth - iIconWidth;
			x = gHUD.DrawHudNumber(x, y, iFlags|DHN_3DIGITS, gWR.CountAmmo(pw->iAmmo2Type), r, g, b);

			// Draw the ammo Icon
			SPR_Set(m_pWeapon->hAmmo2, r, g, b);
			int iOffset = (m_pWeapon->rcAmmo2.bottom - m_pWeapon->rcAmmo2.top)/8;
			SPR_DrawAdditive(0, x, y - iOffset, &m_pWeapon->rcAmmo2);
		}
	}

	if( gHUD.m_iFOV > 75 )
		DrawCrosshair(flTime, m_pWeapon->iId);

	return 1;
}

#define WEST_XPOS (ScreenWidth / 2 - flCrosshairDistance - iLength + 1)
#define EAST_XPOS (flCrosshairDistance + ScreenWidth / 2)
#define EAST_WEST_YPOS (ScreenHeight / 2)

#define NORTH_YPOS (ScreenHeight / 2 - flCrosshairDistance - iLength + 1)
#define SOUTH_YPOS (ScreenHeight / 2 + flCrosshairDistance)
#define NORTH_SOUTH_XPOS (ScreenWidth / 2)

int Distances[30][2] =
{
{ 8, 3 }, // 0
{ 4, 3 }, // 1
{ 5, 3 }, // 2
{ 8, 3 }, // 3
{ 9, 4 }, // 4
{ 6, 3 }, // 5
{ 9, 3 }, // 6
{ 3, 3 }, // 7
{ 8, 3 }, // 8
{ 4, 3 }, // 9
{ 8, 3 }, // 10
{ 6, 3 }, // 11
{ 5, 3 }, // 12
{ 4, 3 }, // 13
{ 4, 3 }, // 14
{ 8, 3 }, // 15
{ 8, 3 }, // 16
{ 8, 3 }, // 17
{ 6, 3 }, // 18
{ 6, 3 }, // 19
{ 8, 6 }, // 20
{ 4, 3 }, // 21
{ 7, 3 }, // 22
{ 6, 4 }, // 23
{ 8, 3 }, // 24
{ 8, 3 }, // 25
{ 5, 3 }, // 26
{ 4, 4 }, // 27
{ 7, 3 }, // 28
{ 7, 3 }, // 29
};

void CHudAmmo::DrawCrosshair( float flTime, int weaponid )
{
	int flags;
	int iDeltaDistance, iDistance;
	int iLength;
	float flCrosshairDistance;

	if ( weaponid > 30 )
	{
		iDistance = 4;
		iDeltaDistance = 3;
	}
	else
	{
		// TODO: Get an info about crosshair for weapon
		iDistance = Distances[weaponid-1][0];
		iDeltaDistance = Distances[weaponid-1][1];
	}

	iLength = 0;
	flags = GetWeaponAccuracyFlags(weaponid);
	if ( flags && m_pClDynamicCrosshair->value && !(gHUD.m_iHideHUDDisplay & 1) )
	{
		if ( g_iPlayerFlags & FL_ONGROUND || !(flags & ACCURACY_AIR) )
		{
			if ( (g_iPlayerFlags & FL_DUCKING) && (flags & ACCURACY_DUCK) )
				iDistance *= 0.5;
			else
			{
				// TODO: Get a weapon speed
				int iWeaponSpeed = 0;

				if ( (g_flPlayerSpeed > iWeaponSpeed) && (flags & ACCURACY_SPEED) )
					iDistance *= 1.5;
			}
		}
		else iDistance *= 2;
		if ( flags & ACCURACY_MULTIPLY_BY_14 )
			iDistance *= 1.4;
		if ( flags & ACCURACY_MULTIPLY_BY_14_2 )
			iDistance *= 1.4;
	}

	if ( m_iAmmoLastCheck >= g_iShotsFired )
	{
		m_flCrosshairDistance -= (m_flCrosshairDistance * 0.013 + 0.1 );
		m_iAlpha += 2;
	}
	else
	{
		m_flCrosshairDistance += iDeltaDistance;
		m_iAlpha -= 40;

		if ( m_flCrosshairDistance > 15.0 )
			m_flCrosshairDistance = 15.0;
		if ( m_iAlpha < 120 )
			m_iAlpha = 120;
	}

	if ( g_iShotsFired > 600 )
		g_iShotsFired = 1;

	CalcCrosshairColor();
	CalcCrosshairDrawMode();
	CalcCrosshairSize();

	m_iAmmoLastCheck = g_iShotsFired;
	if ( iDistance > m_flCrosshairDistance )
		m_flCrosshairDistance = iDistance;
	if ( m_iAlpha > 255 )
		m_iAlpha = 255;
	iLength = (m_flCrosshairDistance - iDistance) * 0.5 + 5;
	flCrosshairDistance = m_flCrosshairDistance;
	if ( ScreenWidth != m_iCrosshairScaleBase )
	{
		flCrosshairDistance = ScreenWidth * flCrosshairDistance / m_iCrosshairScaleBase;
		iLength = ScreenWidth * iLength / m_iCrosshairScaleBase;
	}


	// drawing
	if ( gHUD.m_NVG.m_iEnable )
	{
		gEngfuncs.pfnFillRGBABlend(WEST_XPOS, EAST_WEST_YPOS,	 iLength, 1, 250, 50, 50, m_iAlpha);
		gEngfuncs.pfnFillRGBABlend(EAST_XPOS, EAST_WEST_YPOS,	 iLength, 1, 250, 50, 50, m_iAlpha);
		gEngfuncs.pfnFillRGBABlend(NORTH_SOUTH_XPOS, NORTH_YPOS, 1, iLength, 250, 50, 50, m_iAlpha);
		gEngfuncs.pfnFillRGBABlend(NORTH_SOUTH_XPOS, SOUTH_YPOS, 1, iLength, 250, 50, 50, m_iAlpha);
	}
	else if ( !m_bAdditive )
	{
		gEngfuncs.pfnFillRGBABlend(WEST_XPOS, EAST_WEST_YPOS,	 iLength, 1, m_R, m_G, m_B, m_iAlpha);
		gEngfuncs.pfnFillRGBABlend(EAST_XPOS, EAST_WEST_YPOS,	 iLength, 1, m_R, m_G, m_B, m_iAlpha);
		gEngfuncs.pfnFillRGBABlend(NORTH_SOUTH_XPOS, NORTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha);
		gEngfuncs.pfnFillRGBABlend(NORTH_SOUTH_XPOS, SOUTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha);
	}
	else
	{
		gEngfuncs.pfnFillRGBA(WEST_XPOS, EAST_WEST_YPOS,	iLength, 1, m_R, m_G, m_B, m_iAlpha);
		gEngfuncs.pfnFillRGBA(EAST_XPOS, EAST_WEST_YPOS,	iLength, 1, m_R, m_G, m_B, m_iAlpha);
		gEngfuncs.pfnFillRGBA(NORTH_SOUTH_XPOS,	NORTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha);
		gEngfuncs.pfnFillRGBA(NORTH_SOUTH_XPOS, SOUTH_YPOS, 1, iLength, m_R, m_G, m_B, m_iAlpha);
	}
	return;
}

void CHudAmmo::CalcCrosshairSize()
{
	const char *size = m_pClCrosshairSize->string;

	if( !stricmp(size, "auto") )
	{
		if( ScreenWidth < 640 )
			m_iCrosshairScaleBase = 1024;
		else if( ScreenWidth < 1024 )
			m_iCrosshairScaleBase = 800;
		else m_iCrosshairScaleBase = 640;
		return;
	}

	if( !stricmp ( size, "large" ))
		m_iCrosshairScaleBase = 640;
	else if( !stricmp ( size, "medium" ))
		m_iCrosshairScaleBase = 800;
	else if( !stricmp ( size, "large" ))
		m_iCrosshairScaleBase = 1024;

	return;
}

void CHudAmmo::CalcCrosshairDrawMode()
{
	float drawMode = m_pClCrosshairTranslucent->value;
	if ( drawMode == 0.0f )
	{
		m_bAdditive = 0;
	}
	else if ( drawMode == 1.0f )
	{
		m_bAdditive = 1;
	}
	else
	{
		gEngfuncs.Con_Printf("usage: cl_crosshair_translucent <1|0>\n");
	}
}

void CHudAmmo::CalcCrosshairColor()
{
	const char *colors = m_pClCrosshairColor->string;

	int tempR;
	int tempG;
	int tempB;

	sscanf( colors, "%d %d %d", &tempR, &tempG, &tempB);

	m_cvarR = tempR <= 255? tempR > 0? tempR: 0: 255;
	m_cvarG = tempG <= 255? tempG > 0? tempG: 0: 255;
	m_cvarB = tempB <= 255? tempB > 0? tempB: 0: 255;

	m_R = m_cvarR;
	m_G = m_cvarG;
	m_B = m_cvarB;
}

//
// Draws the ammo bar on the hud
//
int DrawBar(int x, int y, int width, int height, float f)
{
	int r, g, b;

	if (f < 0)
		f = 0;
	if (f > 1)
		f = 1;

	if (f)
	{
		int w = f * width;

		// Always show at least one pixel if we have ammo.
		if (w <= 0)
			w = 1;
		UnpackRGB(r, g, b, RGB_GREENISH);
		FillRGBA(x, y, w, height, r, g, b, 255);
		x += w;
		width -= w;
	}

	UnpackRGB(r, g, b, RGB_YELLOWISH);

	FillRGBA(x, y, width, height, r, g, b, 128);

	return (x + width);
}



void DrawAmmoBar(WEAPON *p, int x, int y, int width, int height)
{
	if ( !p )
		return;
	
	if (p->iAmmoType != -1)
	{
		if (!gWR.CountAmmo(p->iAmmoType))
			return;

		float f = (float)gWR.CountAmmo(p->iAmmoType)/(float)p->iMax1;
		
		x = DrawBar(x, y, width, height, f);


		// Do we have secondary ammo too?

		if (p->iAmmo2Type != -1)
		{
			f = (float)gWR.CountAmmo(p->iAmmo2Type)/(float)p->iMax2;

			x += 5; //!!!

			DrawBar(x, y, width, height, f);
		}
	}
}




//
// Draw Weapon Menu
//
int CHudAmmo::DrawWList(float flTime)
{
	int r,g,b,x,y,a,i;

	if ( !gpActiveSel )
		return 0;

	int iActiveSlot;

	if ( gpActiveSel == (WEAPON *)1 )
		iActiveSlot = -1;	// current slot has no weapons
	else 
		iActiveSlot = gpActiveSel->iSlot;

	x = gHUD.m_Health.m_hrad.right + 10; //!!!
	y = 10; //!!!
	

	// Ensure that there are available choices in the active slot
	if ( iActiveSlot > 0 )
	{
		if ( !gWR.GetFirstPos( iActiveSlot ) )
		{
			gpActiveSel = (WEAPON *)1;
			iActiveSlot = -1;
		}
	}
		
	// Draw top line
	for ( i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		int iWidth;

		UnpackRGB(r,g,b, RGB_YELLOWISH);
	
		if ( iActiveSlot == i )
			a = 255;
		else
			a = 192;

		ScaleColors(r, g, b, 255);
		SPR_Set(gHUD.GetSprite(m_HUD_bucket0 + i), r, g, b );

		// make active slot wide enough to accomodate gun pictures
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos(iActiveSlot);
			if ( p )
				iWidth = p->rcActive.right - p->rcActive.left;
			else
				iWidth = giBucketWidth;
		}
		else
			iWidth = giBucketWidth;

		SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_bucket0 + i));
		
		x += iWidth + 5;
	}


	a = 128; //!!!
	x = gHUD.m_Health.m_hrad.right + 10; //!!!;

	// Draw all of the buckets
	for (i = 0; i < MAX_WEAPON_SLOTS; i++)
	{
		y = giBucketHeight + 10;

		// If this is the active slot, draw the bigger pictures,
		// otherwise just draw boxes
		if ( i == iActiveSlot )
		{
			WEAPON *p = gWR.GetFirstPos( i );
			int iWidth = giBucketWidth;
			if ( p )
				iWidth = p->rcActive.right - p->rcActive.left;

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				p = gWR.GetWeaponSlot( i, iPos );

				if ( !p || !p->iId )
					continue;

			
				// if active, then we must have ammo.
				if ( gWR.HasAmmo(p) )
				{
					UnpackRGB(r,g,b, RGB_YELLOWISH );
					ScaleColors(r, g, b, 192);
				}
				else
				{
					UnpackRGB(r,g,b, RGB_REDISH);
					ScaleColors(r, g, b, 128);
				}


				if ( gpActiveSel == p )
				{
					SPR_Set(p->hActive, r, g, b );
					SPR_DrawAdditive(0, x, y, &p->rcActive);

					SPR_Set(gHUD.GetSprite(m_HUD_selection), r, g, b );
					SPR_DrawAdditive(0, x, y, &gHUD.GetSpriteRect(m_HUD_selection));
				}
				else
				{
					// Draw Weapon if Red if no ammo
					SPR_Set( p->hInactive, r, g, b );
					SPR_DrawAdditive( 0, x, y, &p->rcInactive );
				}

				// Draw Ammo Bar

				DrawAmmoBar(p, x + giABWidth/2, y, giABWidth, giABHeight);
				
				y += p->rcActive.bottom - p->rcActive.top + 5;
			}

			x += iWidth + 5;

		}
		else
		{
			// Draw Row of weapons.

			UnpackRGB(r,g,b, RGB_YELLOWISH);

			for ( int iPos = 0; iPos < MAX_WEAPON_POSITIONS; iPos++ )
			{
				WEAPON *p = gWR.GetWeaponSlot( i, iPos );
				
				if ( !p || !p->iId )
					continue;

				if ( gWR.HasAmmo(p) )
				{
					UnpackRGB(r,g,b, RGB_YELLOWISH);
					a = 128;
				}
				else
				{
					UnpackRGB(r,g,b, RGB_REDISH);
					a = 96;
				}

				FillRGBA( x, y, giBucketWidth, giBucketHeight, r, g, b, a );

				y += giBucketHeight + 5;
			}

			x += giBucketWidth + 5;
		}
	}	

	return 1;

}


/* =================================
	GetSpriteList

Finds and returns the matching 
sprite name 'psz' and resolution 'iRes'
in the given sprite list 'pList'
iCount is the number of items in the pList
================================= */
client_sprite_t *GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount)
{
	if (!pList)
		return NULL;

	int i = iCount;
	client_sprite_t *p = pList;

	while(i--)
	{
		if ((!strcmp(psz, p->szName)) && (p->iRes == iRes))
			return p;
		p++;
	}

	return NULL;
}

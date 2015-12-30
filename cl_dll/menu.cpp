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
// menu.cpp
//
// generic menu handler
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

//#include "vgui_TeamFortressViewport.h"

#define MAX_MENU_STRING	512
/*
enum vguimenutype_t
{
	MENU_TEAM = 2,
	MENU_MAPBRIEFING = 4,
	MENU_CLASS_T = 26,
	MENU_CLASS_CT,
	MENU_BUY,
	MENU_BUY_PISTOL,
	MENU_BUY_SHOTGUN,
	MENU_BUY_RIFLE,
	MENU_BUY_SUBMACHINEGUN,
	MENU_BUY_MACHINEGUN,
	MENU_BUY_ITEM,
};
*/
char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];

int KB_ConvertString( char *in, char **ppout );

DECLARE_MESSAGE( m_Menu, ShowMenu );
DECLARE_MESSAGE( m_Menu, VGUIMenu );
DECLARE_MESSAGE( m_Menu, BuyClose );
DECLARE_MESSAGE( m_Menu, AllowSpec );

DECLARE_COMMAND( m_Menu, OldStyleMenuOpen );
DECLARE_COMMAND( m_Menu, OldStyleMenuClose );
DECLARE_COMMAND( m_Menu, ShowVGUIMenu );

int CHudMenu :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( ShowMenu );
	HOOK_MESSAGE( VGUIMenu );
	HOOK_MESSAGE( BuyClose );
	HOOK_MESSAGE( AllowSpec );
	HOOK_COMMAND( "client_buy_open", OldStyleMenuOpen );
	HOOK_COMMAND( "client_buy_close", OldStyleMenuClose );
	HOOK_COMMAND( "showvguimenu", ShowVGUIMenu );

	_extended_menus = CVAR_CREATE("_extended_menus", "0", FCVAR_ARCHIVE);

	InitHUDData();

	m_bAllowSpec = true; // by default, spectating is allowed

	return 1;
}

void CHudMenu :: InitHUDData( void )
{
	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
	Reset();
}

void CHudMenu :: Reset( void )
{
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = FALSE;
}

int CHudMenu :: VidInit( void )
{
	return 1;
}

int CHudMenu :: Draw( float flTime )
{
	// check for if menu is set to disappear
	if ( m_flShutoffTime > 0 )
	{
		if ( m_flShutoffTime <= gHUD.m_flTime )
		{  // times up, shutoff
			m_fMenuDisplayed = 0;
			m_iFlags &= ~HUD_ACTIVE;
			return 1;
		}
	}

	// don't draw the menu if the scoreboard is being shown
	//if ( gViewPort && gViewPort->IsScoreBoardVisible() )
		//return 1;

	// draw the menu, along the left-hand side of the screen

	// count the number of newlines
	int nlc = 0;
	int i;
	for ( i = 0; i < MAX_MENU_STRING && g_szMenuString[i] != '\0'; i++ )
	{
		if ( g_szMenuString[i] == '\n' )
			nlc++;
	}

	// center it
	int y = (ScreenHeight/2) - ((nlc/2)*12) - 40; // make sure it is above the say text
	int x = 20;

	i = 0;
	while ( i < MAX_MENU_STRING && g_szMenuString[i] != '\0' )
	{
		gHUD.DrawHudString( x, y, 320, g_szMenuString + i, 255, 255, 255 );
		y += 24;

		while ( i < MAX_MENU_STRING && g_szMenuString[i] != '\0' && g_szMenuString[i] != '\n' )
			i++;
		if ( g_szMenuString[i] == '\n' )
			i++;
	}
	
	return 1;
}

// selects an item from the menu
void CHudMenu :: SelectMenuItem( int menu_item )
{
	// if menu_item is in a valid slot,  send a menuselect command to the server
	if ( (menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item-1))) )
	{
		char szbuf[32];
		sprintf( szbuf, "menuselect %d\n", menu_item );
		ClientCmd( szbuf );

		// remove the menu
		m_fMenuDisplayed = 0;
		m_iFlags &= ~HUD_ACTIVE;
	}
}


// Message handler for ShowMenu message
// takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, FALSE if it's the last string
//		string: menu string to display
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudMenu :: MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf )
{
	char *temp = NULL, *menustring;

	BEGIN_READ( pbuf, iSize );

	m_bitsValidSlots = READ_SHORT();
	int DisplayTime = READ_CHAR();
	int NeedMore = READ_BYTE();

	if ( DisplayTime > 0 )
		m_flShutoffTime = DisplayTime + gHUD.m_flTime;
	else
		m_flShutoffTime = -1;

	if ( !m_bitsValidSlots )
	{
		m_fMenuDisplayed = 0; // no valid slots means that the menu should be turned off
		m_iFlags &= ~HUD_ACTIVE;
		ClientCmd("touch_removebutton _menu_*");
		return 1;
	}

	menustring = READ_STRING();

	// menu will be replaced by scripted touch config
	// so execute it and exit
	if( _extended_menus->value != 0.0f )
	{
		if( strcmp(menustring, "#RadioA") )
		{
			ShowVGUIMenu(MENU_RADIOA);
			return 1;
		}
		else if( strcmp(menustring, "#RadioB"))
		{
			ShowVGUIMenu(MENU_RADIOB);
			return 1;
		}
		else if( strcmp(menustring, "#RadioC"))
		{
			ShowVGUIMenu(MENU_RADIOC);
			return 1;
		}
	}

	if ( !m_fWaitingForMore ) // this is the start of a new menu
	{
		strncpy( g_szPrelocalisedMenuString, menustring, MAX_MENU_STRING );
	}
	else
	{  // append to the current menu string
		strncat( g_szPrelocalisedMenuString, menustring, MAX_MENU_STRING - strlen(g_szPrelocalisedMenuString) );
	}
	g_szPrelocalisedMenuString[MAX_MENU_STRING-1] = 0;  // ensure null termination (strncat/strncpy does not)

	if ( !NeedMore )
	{  // we have the whole string, so we can localise it now
		strcpy( g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString( g_szPrelocalisedMenuString ) );

		// Swap in characters
		if ( KB_ConvertString( g_szMenuString, &temp ) )
		{
			strcpy( g_szMenuString, temp );
			free( temp );
		}
	}

	m_fMenuDisplayed = 1;
	m_iFlags |= HUD_ACTIVE;

	m_fWaitingForMore = NeedMore;

	return 1;
}

int CHudMenu::MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ(pbuf, iSize);

	int menuType = READ_BYTE();
	m_bitsValidSlots = READ_SHORT(); // is ignored

	ShowVGUIMenu(menuType);
	return 1;
}

int CHudMenu::MsgFunc_BuyClose(const char *pszName, int iSize, void *pbuf)
{
	UserCmd_OldStyleMenuClose();
	return 1;
}

int CHudMenu::MsgFunc_AllowSpec(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_bAllowSpec = !!READ_BYTE();

	return 1;
}

void CHudMenu::UserCmd_OldStyleMenuOpen()
{
	m_flShutoffTime = -1; // stay open until user will not close it
	strncpy( g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString("Buy"), MAX_MENU_STRING );

}

void CHudMenu::UserCmd_OldStyleMenuClose()
{
	m_fMenuDisplayed = 0; // no valid slots means that the menu should be turned off
	m_iFlags &= ~HUD_ACTIVE;
}

// lol, no real VGUI here
// it's really good only for touchscreen

void CHudMenu::ShowVGUIMenu( int menuType )
{
	char *szCmd;

	switch(menuType)
	{
	case MENU_TEAM:
		szCmd = "exec touch/chooseteam.cfg";
		break;
	case MENU_CLASS_T:
		szCmd = "exec touch/chooseteam_tr.cfg";
		break;
	case MENU_CLASS_CT:
		szCmd = "exec touch/chooseteam_ct.cfg";
		break;
	case MENU_BUY:
		szCmd = "exec touch/buy.cfg";
		break;
	case MENU_BUY_PISTOL:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == 1 )
			szCmd = "exec touch/buy_pistol_t.cfg";
		else szCmd = "exec touch/buy_pistol_ct.cfg";
		break;
	case MENU_BUY_SHOTGUN:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == 1 )
			szCmd = "exec touch/buy_shotgun_t.cfg";
		else szCmd = "exec touch/buy_shotgun_ct.cfg";
		break;
	case MENU_BUY_RIFLE:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == 1 )
			szCmd = "exec touch/buy_rifle_t.cfg";
		else szCmd ="exec touch/buy_rifle_ct.cfg";
		break;
	case MENU_BUY_SUBMACHINEGUN:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == 1 )
			szCmd = "exec touch/buy_submachinegun_t.cfg";
		else szCmd = "exec touch/buy_submachinegun_ct.cfg";
		break;
	case MENU_BUY_MACHINEGUN:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == 1 )
			szCmd = "exec touch/buy_machinegun_t.cfg";
		else szCmd = "exec touch/buy_machinegun_ct.cfg";
		break;
	case MENU_BUY_ITEM:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == 1 )
			szCmd = "exec touch/buy_item_t.cfg";
		else szCmd = "exec touch/buy_item_ct.cfg";
		break;
	case MENU_RADIOA:
		szCmd = "exec touch/radioa.cfg";
		break;
	case MENU_RADIOB:
		szCmd = "exec touch/radiob.cfg";
		break;
	case MENU_RADIOC:
		szCmd = "exec touch/radioc.cfg";
		break;
	case MENU_RADIOSELECTOR:
		szCmd = "exec touch/radioselector.cfg";
		break;
	default:
		szCmd = "touch_removebutton _menu_*"; // back to the default touch page
		m_fMenuDisplayed = 0;
		break;
	}

	m_fMenuDisplayed = 1;
	gEngfuncs.pfnClientCmd(szCmd);
}

void CHudMenu::UserCmd_ShowVGUIMenu()
{
	if( gEngfuncs.Cmd_Argc() != 1 )
	{
		ConsolePrint("usage: showvguimenu <menuType>\n");
	}

	if( gEngfuncs.pfnGetCvarFloat("_vgui_menus") == 0.0f )
		return;

	int menuType = atoi(gEngfuncs.Cmd_Argv(1));
	ShowVGUIMenu(menuType);
}

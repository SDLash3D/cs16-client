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
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//


#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,16,16
#define RGB_GREENISH 0x0000A000 //0,160,0

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4
#define MIN_ALPHA	 100	

#define		HUDELEM_ACTIVE	1

typedef struct {
	int x, y;
} POSITION;

enum 
{ 
	MAX_PLAYERS = 64,
	MAX_TEAMS = 64,
	MAX_TEAM_NAME = 16,
	MAX_HOSTAGES = 24,
};

typedef struct {
	unsigned char r,g,b,a;
} RGBA;

typedef struct cvar_s cvar_t;
extern cvar_t *cl_righthand;
extern cvar_t *cl_weather;
extern cvar_t *cl_minmodels;
extern cvar_t *cl_min_t;
extern cvar_t *cl_min_ct;
extern char *sPlayerModelFiles[];

inline bool BIsValidTModelIndex( int i )
{
	if ( i == 1 || i == 5 || i == 6 || i == 8 || i == 11 )
		return true;
	else
		return false;
}

inline bool BIsValidCTModelIndex( int i )
{
	if ( i == 7 || i == 2 || i == 10 || i == 4 || i == 9)
		return true;
	else
		return false;
}

#define HUD_ACTIVE	1
#define HUD_INTERMISSION 2

#define MAX_PLAYER_NAME_LENGTH		32

#define	MAX_MOTD_LENGTH				1536

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION  m_pos;
	int   m_type;
	int	  m_iFlags; // active, moving, 
	virtual		~CHudBase() {}
	virtual int Init( void ) {return 0;}
	virtual int VidInit( void ) {return 0;}
	virtual int Draw(float flTime) {return 0;}
	virtual void Think(void) {return;}
	virtual void Reset(void) {return;}
	virtual void InitHUDData( void ) {}		// called every time a server is connected to

};

struct HUDLIST {
	CHudBase	*p;
	HUDLIST		*pNext;
};



//
//-----------------------------------------------------
//
//#include "voice_status.h"
#include "hud_spectator.h"


//
//-----------------------------------------------------
//
class CHudAmmo: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Think(void);
	void Reset(void);

	void DrawCrosshair( float flTime, int weaponId );
	void CalcCrosshairSize();
	void CalcCrosshairDrawMode();
	void CalcCrosshairColor();

	int DrawWList(float flTime);
	int MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Crosshair( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Brass( const char *pszName, int iSize, void *pbuf );


	void SlotInput( int iSlot );
	void _cdecl UserCmd_Slot1( void );
	void _cdecl UserCmd_Slot2( void );
	void _cdecl UserCmd_Slot3( void );
	void _cdecl UserCmd_Slot4( void );
	void _cdecl UserCmd_Slot5( void );
	void _cdecl UserCmd_Slot6( void );
	void _cdecl UserCmd_Slot7( void );
	void _cdecl UserCmd_Slot8( void );
	void _cdecl UserCmd_Slot9( void );
	void _cdecl UserCmd_Slot10( void );
	void _cdecl UserCmd_Close( void );
	void _cdecl UserCmd_NextWeapon( void );
	void _cdecl UserCmd_PrevWeapon( void );
	void _cdecl UserCmd_Adjust_Crosshair( void );
<<<<<<< HEAD
   void _cdecl UserCmd_Rebuy (void);
   void _cdecl UserCmd_Autobuy (void);
=======
	void _cdecl UserCmd_Rebuy(void);
	void _cdecl UserCmd_Autobuy(void);
>>>>>>> refs/remotes/SDLash3D/master

private:
	float m_fFade;
	RGBA  m_rgba;
	WEAPON *m_pWeapon;
	int	m_HUD_bucket0;
	int m_HUD_selection;

	int m_iAlpha;
	int m_R, m_G, m_B;
	int m_cvarR, m_cvarG, m_cvarB;
	int m_iCurrentCrosshair;
	int m_iCrosshairScaleBase;
	float m_flCrosshairDistance;
	bool m_bAdditive;
	bool m_bObserverCrosshair ;
	bool m_bDrawCrosshair;
	int m_iAmmoLastCheck;

	cvar_t *m_pClCrosshairColor, *m_pClCrosshairTranslucent, *m_pClCrosshairSize;
	cvar_t *m_pClDynamicCrosshair;

};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SecAmmoIcon( const char *pszName, int iSize, void *pbuf );

private:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};


#include "health.h"


#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Geiger(const char *pszName, int iSize, void *pbuf);
	
private:
	int m_iGeigerRange;

};

//
//-----------------------------------------------------
//
class CHudTrain: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Train(const char *pszName, int iSize, void *pbuf);

private:
	SptiteHandle_t m_hSprite;
	int m_iPos;

};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//

class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );
	void Scroll( int dir );
	void Scroll( float amount );
	float scroll;
	bool m_bShow;

protected:
	static int MOTD_DISPLAY_TIME;
	char m_szMOTD[ MAX_MOTD_LENGTH ];
	
	int m_iLines;
	int m_iMaxLength;
};


class CHudScoreboard: public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, char *team = NULL ); // returns the ypos where it finishes drawing
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScores( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );

	int m_iNumTeams;

	int m_iLastKilledBy;
	int m_fLastKillTime;
	int m_iPlayerNum;
	int m_iShowscoresHeld;

	void GetAllPlayersInfo( void );
};

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	void ParseStatusString( int line_num );

	int MsgFunc_StatusText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_StatusValue( const char *pszName, int iSize, void *pbuf );

protected:
	enum { 
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 2,
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	// the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar

	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float *m_pflNameColors[MAX_STATUSBAR_LINES];
};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//
/*
class CHudScoreboard: public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, char *team = NULL ); // returns the ypos where it finishes drawing
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );

	int m_iNumTeams;

	int m_iLastKilledBy;
	int m_fLastKillTime;
	int m_iPlayerNum;
	int m_iShowscoresHeld;

	void GetAllPlayersInfo( void );
private:
	struct cvar_s *cl_showpacketloss;

};
*/

struct extra_player_info_t 
{
	short frags;
	short deaths;
	short playerclass;
	short teamnumber;
	char teamname[MAX_TEAM_NAME];
	bool has_c4;
	bool vip;
	bool dead;
	Vector origin;
	bool showhealth;
	int health;
	int radarflashon;
	int radarflashes;
	float radarflash;
	char location[32];
};

struct team_info_t 
{
	char name[MAX_TEAM_NAME];
	short frags;
	short deaths;
	short ownteam;
	short players;
	int already_drawn;
	int scores_overriden;
	int teamnumber;
};

struct hostage_info_t
{
	char teamname[MAX_TEAM_NAME];
	vec3_t origin;
	float radarflash;
	int radarflashon;
	int radarflashes;
	int dead;
};

extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
extern team_info_t			g_TeamInfo[MAX_TEAMS+1];
extern hostage_info_t		g_HostageInfo[MAX_HOSTAGES+1];
extern int					g_IsSpectator[MAX_PLAYERS+1];


//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf );

private:
	int m_HUD_d_skull;  // sprite index of skull icon
	int m_HUD_d_headshot;
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_BuyClose( const char *pszName, int iSize, void *pbuf );

	// server sends false when spectating is not allowed, and true when allowed
	// but it's useless, unless we are using Old-Style Menu
	int MsgFunc_AllowSpec( const char *pszName, int iSize, void *pbuf );

	void _cdecl UserCmd_OldStyleMenuClose( void );
	void _cdecl UserCmd_OldStyleMenuOpen( void );
	void _cdecl UserCmd_ShowVGUIMenu( void );
	void ShowVGUIMenu( int menuType ); // cs16client extension

	void SelectMenuItem( int menu_item );

	int m_fMenuDisplayed;
	bool m_bAllowSpec;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;

};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_SayText( const char *pszName, int iSize, void *pbuf );
	void SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
friend class CHudSpectator;

private:

	struct cvar_s *	m_HUD_saytext;
	struct cvar_s *	m_HUD_saytext_time;
};

//
//-----------------------------------------------------
//
class CHudBattery: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_ArmorType( const char *pszName, int iSize, void *pbuf );
	
private:
	SptiteHandle_t m_hSprite1;
	SptiteHandle_t m_hSprite2;
	SptiteHandle_t m_hSprite1Helmet;
	SptiteHandle_t m_hSprite2Helmet;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	int	  m_iBat;	
	float m_fFade;
	int	  m_iHeight;		// width of the battery innards

	enum armortype_t {
		Vest = 0,
		VestHelm
	} m_enArmorType;

};


//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Reset( void );
	int MsgFunc_Flashlight(const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_FlashBat(const char *pszName,  int iSize, void *pbuf );
	
private:
	SptiteHandle_t m_hSprite1;
	SptiteHandle_t m_hSprite2;
	SptiteHandle_t m_hBeam;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	wrect_t *m_prcBeam;
	float m_flBat;	
	int	  m_iBat;	
	int	  m_fOn;
	float m_fFade;
	int	  m_iWidth;		// width of the battery innards
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t	*pMessage;
	float	time;
	int x, y;
	int	totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage: public CHudBase
{
public:
	int Init( void );
	static char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size );
	static char *BufferedLocaliseTextString( const char *msg );
	char *LookupString( const char *msg_name, int *msg_dest = NULL );
	int MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);
};

//
//-----------------------------------------------------
//

class CHudMessage: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_GameTitle(const char *pszName, int iSize, void *pbuf);

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int	XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName, float time );
	void MessageAdd(client_textmessage_t * newMessage );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );

private:
	client_textmessage_t		*m_pMessages[maxHUDMessages];
	float						m_startTime[maxHUDMessages];
	message_parms_t				m_parms;
	float						m_gameTitleTime;
	client_textmessage_t		*m_pGameTitle;

	int m_HUD_title_life;
	int m_HUD_title_half;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH	24

class CHudStatusIcons: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);
	int MsgFunc_StatusIcon(const char *pszName, int iSize, void *pbuf);

	enum { 
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};

	
	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon( char *pszIconName, unsigned char red, unsigned char green, unsigned char blue );
	void DisableIcon( char *pszIconName );

	friend class CHudScoreboard;

private:

	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		SptiteHandle_t spr;
		wrect_t rc;
		unsigned char r, g, b;
		unsigned char secR, secG, secB;
		float flTimeToChange;
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES];
};


//
//-----------------------------------------------------
//
#define MONEY_YPOS ScreenHeight - 3 * gHUD.m_iFontHeight

class CHudMoney : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Money( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_BlinkAcct( const char *pszName, int iSize, void *pbuf );

private:
	int m_iMoneyCount;
	int m_iDelta;
	int m_iBlinkAmt;
	float m_fBlinkTime;
	float m_fFade;
	SptiteHandle_t	m_HUD_dollar;
	SptiteHandle_t m_HUD_minus;
	SptiteHandle_t m_HUD_plus;
};
//
//-----------------------------------------------------
//
class CHudRadio: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	// play a sentence from a radio
	// [byte] unknown (always 1)
	// [string] sentence name
	// [short] unknown. (always 100, it's a volume?)
	int MsgFunc_SendAudio( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ReloadSound( const char *pszName, int iSize, void *pbuf );
private:
	int m_iSenderID;
	char m_sentence[64];
	int m_iPitch;
};

//
//-----------------------------------------------------
//
class CHudTimer: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float fTime);
	// set up the timer.
	// [short]
	int MsgFunc_RoundTime(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_KillTimer(const char *pszName, int iSize, void *pbuf);
	// show the timer
	// [empty]
	int MsgFunc_ShowTimer(const char *pszName, int iSize, void *pbuf);
private:
	int m_HUD_timer;
	int m_iTime;
	float m_fStartTime;
	bool m_bPanicColorChange;
	float m_flPanicTime;
};
//
//-----------------------------------------------------
//
class CHudProgressBar: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );

	// start progress bar
	// [short] Duration
	int MsgFunc_BarTime( const char *pszName, int iSize, void *pbuf );

	// [short] Duration
	// [short] percent
	int MsgFunc_BarTime2( const char *pszName, int iSize, void *pbuf );

	int MsgFunc_BotProgress( const char *pszName, int iSize, void *pbuf );

private:
	int m_iDuration;
	float m_fPercent;
	float m_fStartTime;
	char m_szHeader[256];
};

//
//-----------------------------------------------------
//
// class for drawing sniper scope
class CHudSniperScope: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );

	bool m_bDrawSniperScope;
private:
	float left, right, centerx, centery;
	int m_iScopeArc[4];
	int blackTex;

};

class CHudNVG: public CHudBase
{
public:
	int Init( void );
	int Draw( float flTime );
	int MsgFunc_NVGToggle( const char *pszName, int iSize, void *pbuf);
	void UserCmd_NVGAdjustUp();
	void UserCmd_NVGAdjustDown();

	int m_iEnable;
private:
	int m_iAlpha;

};

//
//-----------------------------------------------------
//



class CHud
{
private:
	HUDLIST						*m_pHudList;
	SptiteHandle_t						m_hsprLogo;
	int							m_iLogo;
	client_sprite_t				*m_pSpriteList;
	int							m_iSpriteCount;
	int							m_iSpriteCountAllRes;
	float						m_flMouseSensitivity;
	int							m_iConcussionEffect; 
	int	m_iForceCamera;
	int m_iForceChaseCam;
	int m_iFadeToBlack;

public:

	SptiteHandle_t						m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	int		m_iKeyBits;
	int		m_iHideHUDDisplay;
	int		m_iFOV;
	int		m_Teamplay;
	int		m_iRes;
	int		RealSize;
	float   m_flScale;
	cvar_t  *m_pCvarStealMouse;
	cvar_t	*m_pCvarDraw;

	int m_iFontHeight;
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b );
	int DrawHudNumber2( int x, int y, bool DrawZero, int iDigits, int iNumber, int r, int g, int b);
	int DrawHudNumber2( int x, int y, int iNumber, int r, int g, int b);
	int DrawHudString(int x, int y, int iMaxX, char *szString, int r, int g, int b, bool drawing = false );
	int DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b, bool drawing = false );
	int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
	int GetNumWidth(int iNumber, int iFlags);
	int DrawHudStringLen( char *szIt );
	void DrawDarkRectangle( int x, int y, int wide, int tall);

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	SptiteHandle_t *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	struct cvar_s *default_fov;
public:
	SptiteHandle_t GetSprite( int index ) 
	{
		return (index < 0) ? 0 : m_rghSprites[index];
	}

	wrect_t& GetSpriteRect( int index )
	{
		return m_rgrcRects[index];
	}

	
	int GetSpriteIndex( const char *SpriteName );	// gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo		m_Ammo;
	CHudHealth		m_Health;
	CHudSpectator		m_Spectator;
	CHudGeiger		m_Geiger;
	CHudBattery		m_Battery;
	CHudTrain		m_Train;
	CHudFlashlight	m_Flash;
	CHudMessage		m_Message;
	CHudStatusBar   m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText		m_SayText;
	CHudMenu		m_Menu;
	CHudAmmoSecondary	m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudScoreboard	m_Scoreboard;
	CHudMOTD	m_MOTD;
	CHudMoney	m_Money;
	CHudTimer	m_Timer;
	CHudRadio	m_Radio;
	CHudProgressBar m_ProgressBar;
	CHudSniperScope m_SniperScope;
	CHudNVG			m_NVG;

	void Init( void );
	void VidInit( void );
	void Think(void);
	int Redraw( float flTime, int intermission );
	int UpdateClientData( client_data_t *cdata, float time );

	CHud() : m_iSpriteCount(0), m_pHudList(NULL) {}  
	~CHud();			// destructor, frees allocated memory

	// user messages
	int _cdecl MsgFunc_Damage(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf);
	int _cdecl MsgFunc_ResetHUD(const char *pszName,  int iSize, void *pbuf);
	int _cdecl MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf);
	int _cdecl MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_ReceiveW( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_BombDrop( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_BombPickup( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_HostagePos(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_HostageK(const char *pszName, int iSize, void *pbuf);
	int _cdecl MsgFunc_ShadowIdx(const char *pszName, int iSize, void *pbuf);


	// Screen information
	SCREENINFO	m_scrinfo;

	int	m_iWeaponBits;
	int	m_fPlayerDead;
	int m_iIntermission;
	int m_iNoConsolePrint;

	// sprite indexes
	int m_HUD_number_0;


	void AddHudElem(CHudBase *p);

	float GetSensitivity();

};

extern CHud gHUD;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;


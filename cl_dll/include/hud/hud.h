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
#pragma once

#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,16,16
#define RGB_GREENISH 0x0000A000 //0,160,0
#define RGB_WHITE 0x00FFFFFF

#include <assert.h>
#include <string.h>

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"

#include "csprite.h"
#include "cvardef.h"
#include "cvar_checker.h"
#include "cl_dll/IGameClientExports.h"


#define MIN_ALPHA	 100	
#define	HUDELEM_ACTIVE	1
#define CHudMsgFunc(x) int MsgFunc_##x(const char *pszName, int iSize, void *buf)
#define CHudUserCmd(x) void UserCmd_##x()

class RGBA
{
public:
	unsigned char r, g, b, a;

	unsigned int Pack()
	{
		return ((a)<<24|(r)<<16|(g)<<8|(b));
	}
};

enum 
{ 
	MAX_PLAYERS = 32,
	MAX_TEAMS = 3,
	MAX_HOSTAGES = 24,
};

#define PLAYERMODEL_PLAYER	0
#define PLAYERMODEL_LEET	1
#define PLAYERMODEL_GIGN	2
#define PLAYERMODEL_VIP		3
extern const char *sPlayerModelFiles[];
extern wrect_t nullrc;

class CClientSprite;

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

struct hostage_info_t
{
	vec3_t origin;
	float radarflashtimedelta;
	float radarflashtime;
	bool dead;
	bool nextflash;
	int radarflashes;
};

struct bomb_info_t
{
	Vector origin;
	int radarflashes;
	float radarflashtime;
	float radarflashtimedelta;
	bool dead;
	bool nextflash;
	bool planted;
};

extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
extern team_info_t			g_TeamInfo[MAX_TEAMS];
extern hostage_info_t		g_HostageInfo[MAX_HOSTAGES];
extern int					g_IsSpectator[MAX_PLAYERS+1];

#define HUD_DRAW         (1 << 0)
#define HUD_THINK        (1 << 1)
#define HUD_ACTIVE       (HUD_DRAW | HUD_THINK)
#define HUD_INTERMISSION (1 << 2)

#define MAX_PLAYER_NAME_LENGTH		32

#define	MAX_MOTD_LENGTH				1536

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	int	  m_iFlags; // active, moving,
	virtual		~CHudBase() {}
	virtual int Init( void ) {return 0;}
	virtual int VidInit( void ) {return 0;}
	virtual int Draw(float flTime) {return 0;}
	virtual void Think(void) {return;}
	virtual void Reset(void) {return;}
	virtual void InitHUDData( void ) {}		// called every time a server is connected to
	virtual void Shutdown( void ) {}
};

struct HUDLIST {
	CHudBase	*p;
	HUDLIST		*pNext;
};



//
//-----------------------------------------------------
//
//#include "voice_status.h"
#include "interpolation.h"

#define INSET_OFF				0
#define	INSET_CHASE_FREE		1
#define	INSET_IN_EYE			2
#define	INSET_MAP_FREE			3
#define	INSET_MAP_CHASE			4

#define MAX_SPEC_HUD_MESSAGES	8



#define OVERVIEW_TILE_SIZE		256		// don't change this
#define OVERVIEW_MAX_LAYERS		1

//-----------------------------------------------------------------------------
// Purpose: Handles the drawing of the spectator stuff (camera & top-down map and all the things on it )
//-----------------------------------------------------------------------------

typedef struct overviewInfo_s {
	char		map[64];	// cl.levelname or empty
	vec3_t		origin;		// center of map
	float		zoom;		// zoom of map images
	int			layers;		// how may layers do we have
	float		layersHeights[OVERVIEW_MAX_LAYERS];
	char		layersImages[OVERVIEW_MAX_LAYERS][255];
	qboolean	rotated;	// are map images rotated (90 degrees) ?

	int			insetWindowX;
	int			insetWindowY;
	int			insetWindowHeight;
	int			insetWindowWidth;
} overviewInfo_t;

typedef struct overviewEntity_s {

	HSPRITE					hSprite;
	struct cl_entity_s *	entity;
	double					killTime;
} overviewEntity_t;

typedef struct cameraWayPoint_s
{
	float	time;
	vec3_t	position;
	vec3_t	angle;
	float	fov;
	int		flags;
} cameraWayPoint_t;

#define	 MAX_OVERVIEW_ENTITIES		128
#define	 MAX_CAM_WAYPOINTS			32

class CHudSpectator : public CHudBase
{
public:
	void Reset();
	int  ToggleInset(bool allowOff);
	void CheckSettings();
	void InitHUDData( void );
	bool AddOverviewEntityToList( HSPRITE sprite, cl_entity_t * ent, double killTime);
	void DeathMessage(int victim);
	bool AddOverviewEntity( int type, struct cl_entity_s *ent, const char *modelname );
	void CheckOverviewEntities();
	void DrawOverview();
	void DrawOverviewEntities();
	void GetMapPosition( float * returnvec );
	void DrawOverviewLayer();
	void LoadMapSprites();
	bool ParseOverviewFile();
	bool IsActivePlayer(cl_entity_t * ent);
	void SetModes(int iMainMode, int iInsetMode);
	void HandleButtonsDown(int ButtonPressed);
	void HandleButtonsUp(int ButtonPressed);
	void FindNextPlayer( bool bReverse );
	void FindPlayer(const char *name);
	void DirectorMessage( int iSize, void *pbuf );
	void SetSpectatorStartPosition();
	int Init();
	int VidInit();

	int Draw(float flTime);

	void	AddWaypoint( float time, vec3_t pos, vec3_t angle, float fov, int flags );
	void	SetCameraView( vec3_t pos, vec3_t angle, float fov);
	float	GetFOV();
	bool	GetDirectorCamera(vec3_t &position, vec3_t &angle);
	void	SetWayInterpolation(cameraWayPoint_t * prev, cameraWayPoint_t * start, cameraWayPoint_t * end, cameraWayPoint_t * next);

	int m_iDrawCycle;
	client_textmessage_t m_HUDMessages[MAX_SPEC_HUD_MESSAGES];
	char				m_HUDMessageText[MAX_SPEC_HUD_MESSAGES][128];
	int					m_lastHudMessage;
	overviewInfo_t		m_OverviewData;
	overviewEntity_t	m_OverviewEntities[MAX_OVERVIEW_ENTITIES];
	int					m_iObserverFlags;
	int					m_iSpectatorNumber;

	float				m_mapZoom;		// zoom the user currently uses
	vec3_t				m_mapOrigin;	// origin where user rotates around
	cvar_t *			m_drawnames;
	cvar_t *			m_drawcone;
	cvar_t *			m_drawstatus;
	cvar_t *			m_autoDirector;
	cvar_t *			m_pip;


	qboolean			m_chatEnabled;

	qboolean			m_IsInterpolating;
	int					m_ChaseEntity;	// if != 0, follow this entity with viewangles
	int					m_WayPoint;		// current waypoint 1
	int					m_NumWayPoints;	// current number of waypoints
	vec3_t				m_cameraOrigin;	// a help camera
	vec3_t				m_cameraAngles;	// and it's angles
	CInterpolation		m_WayInterpolation;
private:
	vec3_t		m_vPlayerPos[MAX_PLAYERS];
	HSPRITE		m_hsprPlayerC4;
	HSPRITE		m_hsprPlayerVIP;
	HSPRITE		m_hsprHostage;
	HSPRITE		m_hsprBackpack;
	HSPRITE		m_hsprBomb;
	HSPRITE		m_hsprPlayerBlue;
	HSPRITE		m_hsprPlayerRed;
	HSPRITE		m_hsprPlayer;
	HSPRITE		m_hsprCamera;
	HSPRITE		m_hsprPlayerDead;
	HSPRITE		m_hsprViewcone;
	HSPRITE		m_hsprUnkownMap;
	HSPRITE		m_hsprBeam;

	wrect_t		m_crosshairRect;

	struct model_s * m_MapSprite;	// each layer image is saved in one sprite, where each tile is a sprite frame
	float		m_flNextObserverInput;
	float		m_zoomDelta;
	float		m_moveDelta;
	float		m_FOV;
	int			m_lastPrimaryObject;
	int			m_lastSecondaryObject;

	cameraWayPoint_t	m_CamPath[MAX_CAM_WAYPOINTS];
};

//
//-----------------------------------------------------
//
class CHudAmmo: public CHudBase
{
	friend class WeaponsResource;
	friend class HistoryResource;

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Think(void);
	void Reset(void);
	void SetCrosshair( HSPRITE hSpr, wrect_t rect, int r, int g, int b );
	void HideCrosshair();

	// replace engine's buggy crosshair
	void DrawSpriteCrosshair();

	// dynamic crosshair
	void DrawCrosshair();
	void CalcCrosshairSize();
	void CalcCrosshairDrawMode();
	void CalcCrosshairColor();

	int DrawWList(float flTime);
	CHudMsgFunc(CurWeapon);
	CHudMsgFunc(WeaponList);
	CHudMsgFunc(AmmoX);
	CHudMsgFunc(AmmoPickup);
	CHudMsgFunc(WeapPickup);
	CHudMsgFunc(ItemPickup);
	CHudMsgFunc(HideWeapon);
	CHudMsgFunc(Crosshair);
	CHudMsgFunc(Brass);


	void SlotInput( int iSlot );
	CHudUserCmd(Slot1);
	CHudUserCmd(Slot2);
	CHudUserCmd(Slot3);
	CHudUserCmd(Slot4);
	CHudUserCmd(Slot5);
	CHudUserCmd(Slot6);
	CHudUserCmd(Slot7);
	CHudUserCmd(Slot8);
	CHudUserCmd(Slot9);
	CHudUserCmd(Slot10);
	CHudUserCmd(Close);
	CHudUserCmd(NextWeapon);
	CHudUserCmd(PrevWeapon);
	CHudUserCmd(Adjust_Crosshair);
	CHudUserCmd(Rebuy);
	CHudUserCmd(Autobuy);

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

	convar_t *m_pClCrosshairColor;
	convar_t *m_pClCrosshairTranslucent;
	convar_t *m_pClCrosshairSize;
	cvar_t *m_pClDynamicCrosshair;
	cvar_t *m_pHud_FastSwitch;
	cvar_t *m_pHud_DrawHistory_Time;

	// replace buggy engine's crosshair
	HSPRITE m_hStaticSpr;
	wrect_t m_rcStaticRc;
	RGBA m_staticRgba;
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

	CHudMsgFunc(SecAmmoVal);
	CHudMsgFunc(SecAmmoIcon);

private:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};


#include "health.h"

//
//-----------------------------------------------------
//
class CHudRadar: public CHudBase
{
public:
	virtual int Init();
	virtual int VidInit();
	virtual int Draw( float flTime );
	virtual void InitHUDData();
	virtual void Reset();
	virtual void Shutdown();

	int MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf);

	void UserCmd_ShowRadar();
	void UserCmd_HideRadar();
	CClientSprite m_hRadar;
	CClientSprite m_hRadarOpaque;

	int MsgFunc_BombDrop(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_BombPickup(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HostagePos(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_HostageK(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_Location(const char *pszName, int iSize, void *pbuf);
private:

	cvar_t *cl_radartype;

	int InitBuiltinTextures();
	void DrawPlayerLocation(int y);
	void DrawRadarDot(int x, int y, int r, int g, int b, int a = 255 );
	void DrawCross(int x, int y, int r, int g, int b, int a = 255 );

	// Call DrawT, DrawFlippedT or DrawRadarDot considering z value
	inline void DrawZAxis( Vector pos, int r, int g, int b, int a = 255 );

	void DrawT( int x, int y, int r, int g, int b, int a = 255 );
	void DrawFlippedT( int x, int y, int r, int g, int b, int a = 255 );
	bool FlashTime( float flTime, hostage_info_t &pplayer );
	bool FlashTime( float flTime, extra_player_info_t &pplayer );
	bool FlashTime( float flTime, bomb_info_t &pplayer );
	Vector WorldToRadar(const Vector vPlayerOrigin, const Vector vObjectOrigin, const Vector vAngles );
	inline void DrawColoredTexture( int x, int y, int size, byte r, byte g, byte b, byte a, uint texHandle );

	bool bUseRenderAPI, bTexturesInitialized;
	uint hCross, hT, hFlippedT;
	int iMaxRadius;
	bomb_info_t m_bombInfo;
};

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
	CHudMsgFunc(Geiger);
	
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
	CHudMsgFunc(Train);

private:
	HSPRITE m_hSprite;
	int m_iPos;

};

//
//-----------------------------------------------------
//
//  MOTD in cs16 must render HTML, so it disabled
//

class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	CHudMsgFunc(MOTD);
	void Scroll( int dir );
	void Scroll( float amount );
	float scroll;
	bool m_bShow;
	cvar_t *cl_hide_motd;

protected:
	static int MOTD_DISPLAY_TIME;
	char m_szMOTD[ MAX_MOTD_LENGTH ];
	
	int m_iLines;
	int m_iMaxLength;
	bool ignoreThisMotd;
};


class CHudScoreboard: public CHudBase
{
	friend class CHudSpectatorGui;
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );

	int DrawScoreboard( float flTime );
	int DrawTeams( float listslot );
	int DrawPlayers( float listslot, int nameoffset = 0, const char *team = NULL ); // returns the ypos where it finishes drawing

	void DeathMsg( int killer, int victim );
	void SetScoreboardDefaults( void );
	void GetAllPlayersInfo( void );

	CHudUserCmd(ShowScores);
	CHudUserCmd(HideScores);
	CHudUserCmd(ShowScoreboard2);
	CHudUserCmd(HideScoreboard2);

	CHudMsgFunc(ScoreInfo);
	CHudMsgFunc(TeamInfo);
	CHudMsgFunc(TeamScore);
	CHudMsgFunc(TeamNames);

	int m_iPlayerNum;
	int m_iNumTeams;
	bool m_bForceDraw; // if called by showscoreboard2
	bool m_bShowscoresHeld;

private:
	int m_iLastKilledBy;
	int m_fLastKillTime;
	RGBA m_colors;
	bool m_bDrawStroke;
	cvar_t *cl_showpacketloss;
	cvar_t *cl_showplayerversion;
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

	CHudMsgFunc(StatusText);
	CHudMsgFunc(StatusValue);

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

	cvar_t *hud_centerid;
};

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
	CHudMsgFunc(DeathMsg);

private:
	int m_HUD_d_skull;  // sprite index of skull icon
	int m_HUD_d_headshot;
	cvar_t *hud_deathnotice_time;
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

	CHudMsgFunc(ShowMenu);
	CHudMsgFunc(BuyClose);
	CHudMsgFunc(VGUIMenu);
	// server sends false when spectating is not allowed, and true when allowed
	CHudMsgFunc(AllowSpec);

	CHudUserCmd(OldStyleMenuClose);
	CHudUserCmd(OldStyleMenuOpen);
	CHudUserCmd(ShowVGUIMenu);

	void ShowVGUIMenu( int menuType ); // cs16client extension

	void SelectMenuItem( int menu_item );

	int m_fMenuDisplayed;
	bool m_bAllowSpec;
	cvar_t *_extended_menus;
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
	CHudMsgFunc(SayText);
	void SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
	void SayTextPrint( char szBuf[3][256] );
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
	CHudMsgFunc(Battery);
	CHudMsgFunc(ArmorType);
	
private:
	enum armortype_t {
		Vest = 0,
		VestHelm
	} m_enArmorType;

	CClientSprite m_hEmpty[VestHelm + 1];
	CClientSprite m_hFull[VestHelm + 1];
	int	  m_iBat;
	float m_fFade;
	int	  m_iHeight;		// width of the battery innards
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
	CHudMsgFunc(Flashlight);
	CHudMsgFunc(FlashBat);
	
private:
	CClientSprite m_hSprite1;
	CClientSprite m_hSprite2;
	CClientSprite m_hBeam;
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
	static char *LookupString( char *msg_name, int *msg_dest = NULL );
	CHudMsgFunc(TextMsg);
};

//
//-----------------------------------------------------
//

class CHudMessage: public CHudBase
{
public:
	friend class CHudTextMessage;
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	CHudMsgFunc(HudText);
	CHudMsgFunc(GameTitle);
	CHudMsgFunc(HudTextArgs);
	CHudMsgFunc(HudTextPro);

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int	XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName, float time );
	void MessageAdd(client_textmessage_t * newMessage );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );

	client_textmessage_t *AllocMessage( const char *text = NULL, client_textmessage_t *copyFrom = NULL );

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
	CHudMsgFunc(StatusIcon);

	enum {
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};

	
	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon( const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue );
	void DisableIcon( const char *pszIconName );

	friend class CHudScoreboard;

private:

	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
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
	CHudMsgFunc(Money);
	CHudMsgFunc(BlinkAcct);

private:
	int m_iMoneyCount;
	int m_iDelta;
	int m_iBlinkAmt;
	float m_fBlinkTime;
	float m_fFade;
	CClientSprite m_hDollar;
	CClientSprite m_hPlus;
	CClientSprite m_hMinus;
};
//
//-----------------------------------------------------
//
class CHudRadio: public CHudBase
{
public:
	int Init( void );
	void Voice(int entindex, bool bTalking );
	// play a sentence from a radio
	// [byte] unknown (always 1)
	// [string] sentence name
	// [short] unknown. (always 100, it's a volume?)
	CHudMsgFunc(SendAudio);
	CHudMsgFunc(ReloadSound);
	CHudMsgFunc(BotVoice);
};

//
//-----------------------------------------------------
//
class CHudTimer: public CHudBase
{
	friend class CHudSpectatorGui;
public:
	int Init( void );
	int VidInit( void );
	int Draw(float fTime);
	// set up the timer.
	// [short]
	CHudMsgFunc(RoundTime);
	// show the timer
	// [empty]
	CHudMsgFunc(ShowTimer);

	int m_right;
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
	void Reset( void );

	// start progress bar
	// [short] Duration
	CHudMsgFunc(BarTime);

	// [short] Duration
	// [short] percent
	CHudMsgFunc(BarTime2);
	CHudMsgFunc(BotProgress);

private:
	int m_iDuration;
	float m_fPercent;
	float m_fStartTime;
	char m_szHeader[256];
	const char *m_szLocalizedHeader;
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
	void Shutdown( void );
private:
	int InitBuiltinTextures();

	float left, right, centerx, centery;
	int m_iScopeArc[4];

	bool m_bUseBuiltin;
	bool m_bBuiltinInitialized;
};

//
//-----------------------------------------------------
//

class CHudNVG: public CHudBase
{
public:
	int Init( void );
	int Draw( float flTime );
	void Reset( void );
	CHudMsgFunc(NVGToggle);

	CHudUserCmd(NVGAdjustUp);
	CHudUserCmd(NVGAdjustDown);
private:
	int m_iAlpha;
	cvar_t *cl_fancy_nvg;
	dlight_t *m_pLight;
};

//
//-----------------------------------------------------
//

class CHudScenario : public CHudBase
{
public:
	int Init();
	int VidInit();
	int Draw(float flTime);
	void Reset();

	CHudMsgFunc(Scenario);
private:
	CClientSprite m_sprite;
	int m_iAlpha;
	int m_iFlashAlpha;
	float m_fFlashRate;
	float m_fNextFlash;
};

//
//-----------------------------------------------------
//

class CHudSpectatorGui: public CHudBase
{
public:
	int Init();
	int VidInit();
	int Draw( float flTime );
	void InitHUDData();
	void Reset();
	void Shutdown();

	CHudMsgFunc( SpecHealth );
	CHudMsgFunc( SpecHealth2 );

	CHudUserCmd( ToggleSpectatorMenu );
	CHudUserCmd( ToggleSpectatorMenuOptions );
	CHudUserCmd( ToggleSpectatorMenuOptionsSettings );
	CHudUserCmd( ToggleSpectatorMenuSpectateOptions );

	void CalcAllNeededData( );

	bool m_bBombPlanted;

private:	
	// szMapName is 64 bytes only. Removing "maps/" and ".bsp" gived me this result
	class Labels
	{
	public:
		short m_iTerrorists;
		short m_iCounterTerrorists;
		char m_szTimer[64];
		char m_szMap[64];
		char m_szNameAndHealth[80];
	} label;
	int m_hTimerTexture;

	enum {
		ROOT_MENU = (1<<0),
		MENU_OPTIONS = (1<<1),
		MENU_OPTIONS_SETTINGS = (1<<2),
		MENU_SPEC_OPTIONS = (1<<3)
	};
	byte m_menuFlags;
};

//
//-----------------------------------------------------
//

class CHud
{
public:
	CHud() : m_pHudList(NULL), m_iSpriteCount(0), m_iRes(640)  {}
	~CHud();			// destructor, frees allocated memory // thanks, Captain Obvious

	void Init( void );
	void VidInit( void );
	void Think( void );
	void Shutdown( void );
	int Redraw( float flTime, int intermission );
	int UpdateClientData( client_data_t *cdata, float time );
	void AddHudElem(CHudBase *p);

	inline float GetSensitivity() { return m_flMouseSensitivity; }
	inline HSPRITE GetSprite( int index )
	{
		assert( index >= -1 && index <= m_iSpriteCount );

		return (index >= 0) ? m_rghSprites[index] : 0;
	}

	inline wrect_t& GetSpriteRect( int index )
	{
		assert( index >= -1 && index <= m_iSpriteCount );

		return (index >= 0) ? m_rgrcRects[index]: nullrc;
	}

	// GetSpriteIndex()
	// searches through the sprite list loaded from hud.txt for a name matching SpriteName
	// returns an index into the gHUD.m_rghSprites[] array
	// returns -1 if sprite not found
	inline int GetSpriteIndex( const char *SpriteName )
	{
		// look through the loaded sprite name list for SpriteName
		for ( int i = 0; i < m_iSpriteCount; i++ )
		{
			if ( strncmp( SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
				return i;
		}

		gEngfuncs.Con_Printf( "GetSpriteIndex: %s sprite not found\n", SpriteName );
		return -1; // invalid sprite
	}

	inline int GetCharWidth ( int ch, float scale = 1.0f )
	{
		return g_pMenu->GetCharacterWidth( QM_SMALLFONT, ch, UI_SMALL_CHAR_HEIGHT * scale ); // TODO
	}

	inline int GetCharHeight( float scale = 1.0f )
	{
		return g_pMenu->GetFontTall( QM_SMALLFONT ) * scale; // TODO
	}

	inline int GetGameType( )
	{
		return m_iGameType;
	}

	inline int GetSpriteRes()
	{
		return m_iRes;
	}


	float   m_flTime;      // the current client time
	float   m_fOldTime;    // the time at which the HUD was last redrawn
	double  m_flTimeDelta; // the difference between flTime and fOldTime
	float   m_flScale;     // hud_scale->value
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	int		m_iKeyBits;
	int		m_iHideHUDDisplay;
	int		m_iFOV;
	int		m_Teamplay;
	cvar_t *m_pCvarDraw;
	cvar_t *cl_predict;
	cvar_t *cl_weapon_wallpuff;
	cvar_t *cl_weapon_sparks;
	cvar_t *cl_lw;
	cvar_t *cl_righthand;
	cvar_t *cl_weather;
	cvar_t *cl_minmodels;
	cvar_t *cl_min_t;
	cvar_t *cl_min_ct;
	cvar_t *cl_gunsmoke;
	cvar_t *hud_textmode;
	cvar_t *hud_colored;
#ifdef __ANDROID__
	cvar_t *cl_android_force_defaults;
#endif

	int m_iFontHeight, m_iFontWidth;
	CHudAmmo        m_Ammo;
	CHudHealth      m_Health;
	CHudSpectator   m_Spectator;
	CHudGeiger      m_Geiger;
	CHudBattery	    m_Battery;
	CHudTrain       m_Train;
	CHudFlashlight  m_Flash;
	CHudMessage     m_Message;
	CHudStatusBar   m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText     m_SayText;
	CHudMenu        m_Menu;
	CHudAmmoSecondary m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudScoreboard  m_Scoreboard;
	CHudMOTD        m_MOTD;
	CHudMoney       m_Money;
	CHudTimer       m_Timer;
	CHudRadio       m_Radio;
	CHudProgressBar m_ProgressBar;
	CHudSniperScope m_SniperScope;
	CHudNVG         m_NVG;
	CHudRadar       m_Radar;
	CHudSpectatorGui m_SpectatorGui;
	CHudScenario	m_Scenario;

	// user messages
	CHudMsgFunc(Damage);
	CHudMsgFunc(GameMode);
	CHudMsgFunc(Logo);
	CHudMsgFunc(ResetHUD);
	CHudMsgFunc(InitHUD);
	CHudMsgFunc(ViewMode);
	CHudMsgFunc(SetFOV);
	CHudMsgFunc(Concuss);
	CHudMsgFunc(ShadowIdx);
	CHudMsgFunc(ServerName);

	// Screen information
	SCREENINFO	m_scrinfo;
	// As Xash3D can fake m_scrinfo for hud scailing
	// we will use a real screen parameters
	SCREENINFO  m_truescrinfo;

	int	m_iWeaponBits;
	int	m_fPlayerDead;
	int m_iIntermission;
	int m_iNoConsolePrint;

	// sprite indexes
	int m_HUD_number_0;

	char m_szServerName[64];

	int m_WhiteTex;

private:
	void SetGameType();

	HUDLIST	*m_pHudList;
	HSPRITE	m_hsprLogo;
	int	m_iLogo;
	client_sprite_t	*m_pSpriteList;
	int	m_iSpriteCount;
	int	m_iSpriteCountAllRes;
	float m_flMouseSensitivity;
	int	m_iConcussionEffect;
	int	m_iForceCamera;
	int m_iForceChaseCam;
	int m_iFadeToBlack;
	int m_iGameType;
	int	m_iRes;

	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	cvar_t *hud_draw;
	cvar_t *default_fov;
	cvar_t *zoom_sens_ratio;

	CCVarChecker m_cvarChecker;
};

extern CHud gHUD;
extern cvar_t *sensitivity;

extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;

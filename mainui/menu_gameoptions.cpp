/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "keydefs.h"
#include "menu_btnsbmp_table.h"

#define ART_BANNER			"gfx/shell/head_advoptions"

#undef ID_BACKGROUND
#undef ID_BANNER

enum
{
	ID_BACKGROUND = 0,
	ID_BANNER,
	ID_DONE,
	ID_CANCEL,
	// cs16-client
	ID_CORSPESTAY,
	ID_DECALS,
	ID_MAXSHELLS,
	ID_MAXPUFFS,
	ID_SG_GREN_TYPE,
	ID_HAND,
	ID_OLDSTYLEMENU,
	ID_EXTENDEDMENU,
	ID_AUTOWEPSWITCH,
	ID_CENTERID,
	ID_AUTOHELP,
	ID_ENDGAME_SCREENSHOT,
	ID_OBSERVERCROSSHAIR,
	ID_TRANSPARENTRADAR,
	ID_MAXPACKET,
	ID_MAXPACKETMESSAGE
};

typedef struct
{
	float	cl_corpsestay;
	float	mp_decals;
	float   maxPacket;
	//float	max_sheels;
	//float	max_smoke_puffs;
	byte	fast_smoke_gas;
	byte	hand;
	byte	oldstylemenu;
	byte	extendedmenus;
	byte	cl_autowepswitch;
	byte	hud_centerid;
	byte	auto_help;
	byte	radar_type;
} uiGameValues_t;

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuPicButton_s	done;
	menuPicButton_s	cancel;

	menuSpinControl_s		cl_corpsestay;
	menuAction_s	cl_corpsestay_message;
	menuSpinControl_s		mp_decals;
	menuAction_s    mp_decals_message;

	menuCheckBox_s	fast_smoke_gas;
	menuCheckBox_s	hand;
	menuCheckBox_s	oldstylemenu;
	menuCheckBox_s  extendedmenus;
	menuCheckBox_s	cl_autowepswitch;
	menuCheckBox_s	hud_centerid;
	menuCheckBox_s	auto_help;
	//menuCheckBox_s	endgame_screenshot;
	//menuCheckBox_s	observer_crosshair;
	menuCheckBox_s	radar_type;

	menuSpinControl_s	maxPacket;
	menuAction_s	maxPacketmessage1;
	menuAction_s	maxPacketmessage2;
} uiGameOptions_t;

static uiGameOptions_t	uiGameOptions;
static uiGameValues_t	uiGameInitial;

/*
=================
UI_GameOptions_UpdateConfig
=================
*/
static void UI_GameOptions_UpdateConfig( void )
{
	static char	corpseStayText[8];
	static char	maxpacketText[8];
	static char decalsText[8];

	sprintf( corpseStayText, "%.f", uiGameOptions.cl_corpsestay.curValue );
	uiGameOptions.cl_corpsestay.generic.name = corpseStayText;

	sprintf( decalsText, "%.f", uiGameOptions.mp_decals.curValue );
	uiGameOptions.mp_decals.generic.name = decalsText;
	if( uiGameOptions.maxPacket.curValue >= 1500 )
	{
		sprintf( maxpacketText, "default" );

		// even do not send it to server
		CVAR_SET_FLOAT( "cl_maxpacket", 40000 );
	}
	else
	{
		sprintf( maxpacketText, "%.f", uiGameOptions.maxPacket.curValue );
		CVAR_SET_FLOAT( "cl_maxpacket", uiGameOptions.maxPacket.curValue );
	}

	uiGameOptions.maxPacket.generic.name = maxpacketText;

	CVAR_SET_FLOAT( "hand",          !uiGameOptions.hand.enabled );
	CVAR_SET_FLOAT( "cl_corpsestay", uiGameOptions.cl_corpsestay.curValue );
	CVAR_SET_FLOAT( "mp_decals",     uiGameOptions.mp_decals.curValue );
	//CVAR_SET_FLOAT( "") maxshells
	//CVAR_SET_FLOAT( "") max_smoke_puffs;
	CVAR_SET_FLOAT( "fastsprites",  uiGameOptions.fast_smoke_gas.enabled );
	CVAR_SET_FLOAT( "_vgui_menus",   !uiGameOptions.oldstylemenu.enabled );
	CVAR_SET_FLOAT( "_extended_menus", uiGameOptions.extendedmenus.enabled );
	CVAR_SET_FLOAT( "_cl_autowepswitch", uiGameOptions.cl_autowepswitch.enabled );
	CVAR_SET_FLOAT( "hud_centerid",  uiGameOptions.hud_centerid.enabled );
	CVAR_SET_FLOAT( "_ah",           uiGameOptions.auto_help.enabled );
	//CVAR_SET_FLOAT( "", uiGameOptions.endgame_screenshot.enabled );
	//CVAR_SET_FLOAT( "", uiGameOptions.observer_crosshair.enabled );
	CVAR_SET_FLOAT( "cl_radartype",  uiGameOptions.radar_type.enabled );

}

/*
=================
UI_GameOptions_DiscardChanges
=================
*/
static void UI_GameOptions_DiscardChanges( void )
{
	CVAR_SET_FLOAT( "hand",          !uiGameInitial.hand );
	CVAR_SET_FLOAT( "cl_corpsestay", uiGameInitial.cl_corpsestay );
	CVAR_SET_FLOAT( "mp_decals",     uiGameInitial.mp_decals );
	//CVAR_SET_FLOAT( "") maxshells
	//CVAR_SET_FLOAT( "") max_smoke_puffs;
	CVAR_SET_FLOAT( "fastsprites",  uiGameInitial.fast_smoke_gas );
	CVAR_SET_FLOAT( "_vgui_menus",   !uiGameInitial.oldstylemenu );
	CVAR_SET_FLOAT( "_extended_menus", uiGameInitial.extendedmenus );
	CVAR_SET_FLOAT( "_cl_autowepswitch", uiGameInitial.cl_autowepswitch );
	CVAR_SET_FLOAT( "hud_centerid",  uiGameInitial.hud_centerid );
	CVAR_SET_FLOAT( "_ah",           uiGameInitial.auto_help );
	//CVAR_SET_FLOAT( "_extended_menus", uiGameOptions.endgame_screenshot.enabled );
	//CVAR_SET_FLOAT( "", uiGameInitial.observer_crosshair );
	CVAR_SET_FLOAT( "cl_radartype",  uiGameInitial.radar_type );
	CVAR_SET_FLOAT( "cl_maxpacket", uiGameInitial.maxPacket );
}

/*
=================
UI_GameOptions_KeyFunc
=================
*/
static const char *UI_GameOptions_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE
#ifdef __ANDROID__
		|| key == 236 // ANDROID_K_MENU
#endif
		)
		UI_GameOptions_DiscardChanges ();
	return UI_DefaultKey( &uiGameOptions.menu, key, down );
}

/*
=================
UI_GameOptions_GetConfig
=================
*/
static void UI_GameOptions_GetConfig( void )
{
	uiGameOptions.cl_corpsestay.curValue = uiGameInitial.cl_corpsestay = CVAR_GET_FLOAT( "cl_corpsestay" );
	uiGameOptions.mp_decals.curValue = uiGameInitial.mp_decals = CVAR_GET_FLOAT( "mp_decals" );
	uiGameOptions.maxPacket.curValue = uiGameInitial.maxPacket = CVAR_GET_FLOAT( "cl_maxpacket" );

	uiGameInitial.hand = uiGameOptions.hand.enabled = !CVAR_GET_FLOAT( "hand" );
	uiGameInitial.oldstylemenu = uiGameOptions.oldstylemenu.enabled = !CVAR_GET_FLOAT( "_vgui_menus" );

	if( CVAR_GET_FLOAT( "fastsprites" ))
		uiGameInitial.fast_smoke_gas = uiGameOptions.fast_smoke_gas.enabled = true;
	if( CVAR_GET_FLOAT( "_extended_menus" ))
		uiGameInitial.extendedmenus = uiGameOptions.extendedmenus.enabled = true;
	if( CVAR_GET_FLOAT( "_cl_autowepswitch" ))
		uiGameInitial.cl_autowepswitch = uiGameOptions.cl_autowepswitch.enabled = true;
	if( CVAR_GET_FLOAT( "hud_centerid" ))
		uiGameInitial.hud_centerid = uiGameOptions.hud_centerid.enabled = true;
	if( CVAR_GET_FLOAT( "_ah" ))
		uiGameInitial.auto_help = uiGameOptions.auto_help.enabled = true;
	if( CVAR_GET_FLOAT( "cl_radartype" ))
		uiGameInitial.radar_type = uiGameOptions.radar_type.enabled = true;

	UI_GameOptions_UpdateConfig ();
}

/*
=================
UI_GameOptions_Callback
=================
*/
static void UI_GameOptions_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( item->id >= ID_HAND && item->id <= ID_TRANSPARENTRADAR )
	{
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
	}

	if( event == QM_CHANGED )
	{
		UI_GameOptions_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		CLIENT_COMMAND( FALSE, "trysaveconfig\n" );
		UI_PopMenu();
		break;
	case ID_CANCEL:
		UI_GameOptions_DiscardChanges();
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_GenItemInit
=================
*/
inline void UI_GenItemInit( menuCommon_s &item, int id, menuType_t type, unsigned int flags,
	int x, int y, const char *name, const char *statusText )
{
	item.id = id;
	item.type = type;
	item.flags = flags;
	item.x = x;
	item.y = y;
	item.name = name;
	item.callback = UI_GameOptions_Callback;
	item.statusText = statusText;
}

/*
=================
UI_GameOptions_Init
=================
*/
static void UI_GameOptions_Init( void )
{
	static unsigned int iTypicalFlags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DROPSHADOW;
	const int gap = 50;
	memset( &uiGameInitial, 0, sizeof( uiGameValues_t ));
	memset( &uiGameOptions, 0, sizeof( uiGameOptions_t ));

	uiGameOptions.menu.vidInitFunc = UI_GameOptions_Init;
	uiGameOptions.menu.keyFunc = UI_GameOptions_KeyFunc;

	uiGameOptions.background.generic.id = ID_BACKGROUND;
	uiGameOptions.background.generic.type = QMTYPE_BITMAP;
	uiGameOptions.background.generic.flags = QMF_INACTIVE;
	uiGameOptions.background.generic.x = 0;
	uiGameOptions.background.generic.y = 0;
	uiGameOptions.background.generic.width = uiStatic.width;
	uiGameOptions.background.generic.height = 768;
	uiGameOptions.background.pic = ART_BACKGROUND;

	uiGameOptions.banner.generic.id = ID_BANNER;
	uiGameOptions.banner.generic.type = QMTYPE_BITMAP;
	uiGameOptions.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiGameOptions.banner.generic.x = UI_BANNER_POSX;
	uiGameOptions.banner.generic.y = UI_BANNER_POSY;
	uiGameOptions.banner.generic.width = UI_BANNER_WIDTH;
	uiGameOptions.banner.generic.height = UI_BANNER_HEIGHT;
	uiGameOptions.banner.pic = ART_BANNER;

	int x = 72, y = 180;

	UI_GenItemInit( uiGameOptions.hand.generic, ID_HAND, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Use left hand", "Draw gun at left side" );
	UI_GenItemInit( uiGameOptions.fast_smoke_gas.generic, ID_SG_GREN_TYPE, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Low quality smoke", "Use low quality smoke, for slow devices" );
	UI_GenItemInit( uiGameOptions.oldstylemenu.generic,	ID_OLDSTYLEMENU, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Old style menus", "Use old-styled numerical buy menu" );
	UI_GenItemInit( uiGameOptions.extendedmenus.generic, ID_EXTENDEDMENU, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Extended touch menu", "Force touch menus for radio" );
	UI_GenItemInit( uiGameOptions.cl_autowepswitch.generic, ID_AUTOWEPSWITCH, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Automatic weapon switch", "Enable automatic weapon switch" );
	UI_GenItemInit( uiGameOptions.hud_centerid.generic, ID_CENTERID, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Center player names", "" );
	UI_GenItemInit( uiGameOptions.auto_help.generic, ID_AUTOHELP, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Auto-help", "Do you need to know how to play Counter-Strike?" );
	UI_GenItemInit( uiGameOptions.radar_type.generic, ID_TRANSPARENTRADAR, QMTYPE_CHECKBOX, iTypicalFlags,
		x, y += gap, "Opaque radar", "Opaque or transparent radar. Useful for slow devices." );


	UI_GenItemInit( uiGameOptions.cl_corpsestay_message.generic, 10000, QMTYPE_ACTION, QMF_SMALLFONT| QMF_INACTIVE|QMF_DROPSHADOW,
		420, y = 240, "Time before dead bodies disappear:", NULL);
	uiGameOptions.cl_corpsestay_message.generic.color = uiColorHelp;

	UI_GenItemInit( uiGameOptions.cl_corpsestay.generic, ID_CORSPESTAY, QMTYPE_SPINCONTROL, QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW,
		450, y += gap, "600", NULL );
	uiGameOptions.cl_corpsestay.generic.height = 26;
	uiGameOptions.cl_corpsestay.generic.width = 168;
	uiGameOptions.cl_corpsestay.minValue = 0;
	uiGameOptions.cl_corpsestay.maxValue = 1000;
	uiGameOptions.cl_corpsestay.range	 = 50;

	UI_GenItemInit( uiGameOptions.mp_decals_message.generic, 10000, QMTYPE_ACTION, QMF_SMALLFONT|QMF_INACTIVE|QMF_DROPSHADOW,
		420, y += gap, "Multiplayer decal limit:", NULL);
	uiGameOptions.mp_decals_message.generic.color = uiColorHelp;

	UI_GenItemInit( uiGameOptions.mp_decals.generic, ID_DECALS, QMTYPE_SPINCONTROL, QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW,
		450, y += gap, "300", NULL );
	uiGameOptions.mp_decals.generic.height = 26;
	uiGameOptions.mp_decals.generic.width = 168;
	uiGameOptions.mp_decals.minValue = 0;
	uiGameOptions.mp_decals.maxValue = 1000;
	uiGameOptions.mp_decals.range	 = 50;

	uiGameOptions.maxPacketmessage1.generic.id = ID_MAXPACKETMESSAGE;
	uiGameOptions.maxPacketmessage1.generic.type = QMTYPE_ACTION;
	uiGameOptions.maxPacketmessage1.generic.flags = QMF_SMALLFONT|QMF_INACTIVE|QMF_DROPSHADOW;
	uiGameOptions.maxPacketmessage1.generic.x = 420;
	uiGameOptions.maxPacketmessage1.generic.y = (y += gap);
	uiGameOptions.maxPacketmessage1.generic.name = "Limit network packet size";
	uiGameOptions.maxPacketmessage1.generic.color = uiColorHelp;

	uiGameOptions.maxPacket.generic.id = ID_MAXPACKET;
	uiGameOptions.maxPacket.generic.type = QMTYPE_SPINCONTROL;
	uiGameOptions.maxPacket.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiGameOptions.maxPacket.generic.x = 450;
	uiGameOptions.maxPacket.generic.y = (y += gap);
	uiGameOptions.maxPacket.generic.width = 168;
	uiGameOptions.maxPacket.generic.height = 26;
	uiGameOptions.maxPacket.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.maxPacket.generic.statusText = "Limit packet size durning connection";
	uiGameOptions.maxPacket.minValue = 200;
	uiGameOptions.maxPacket.maxValue = 1500;
	uiGameOptions.maxPacket.range = 50;

	uiGameOptions.maxPacketmessage2.generic.id = ID_MAXPACKETMESSAGE;
	uiGameOptions.maxPacketmessage2.generic.type = QMTYPE_ACTION;
	uiGameOptions.maxPacketmessage2.generic.flags = QMF_SMALLFONT|QMF_INACTIVE|QMF_DROPSHADOW;
	uiGameOptions.maxPacketmessage2.generic.x = 420;
	uiGameOptions.maxPacketmessage2.generic.y =  (y += gap);
	uiGameOptions.maxPacketmessage2.generic.name = "^3Use 700 or less if connection hangs\nafter \"^6Spooling demo header^3\" message";
	uiGameOptions.maxPacketmessage2.generic.color = uiColorWhite;

	UI_GenItemInit( uiGameOptions.done.generic, ID_DONE, QMTYPE_BM_BUTTON, QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW,
		450, y += gap, "Done", "Save changes and go back to the Customize Menu");
	UI_UtilSetupPicButton( &uiGameOptions.done, PC_DONE );

	UI_GenItemInit( uiGameOptions.cancel.generic, ID_CANCEL, QMTYPE_BM_BUTTON, QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW,
		450, y += gap, "Cancel", "Go back to the Customize Menu");
	UI_UtilSetupPicButton( &uiGameOptions.cancel, PC_CANCEL );

	UI_GameOptions_GetConfig();

	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.background );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.banner );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.hand );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.fast_smoke_gas );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.oldstylemenu );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.extendedmenus );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.cl_autowepswitch );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.hud_centerid );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.auto_help );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.radar_type );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.cl_corpsestay );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.cl_corpsestay_message );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.mp_decals );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.mp_decals_message );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.maxPacket );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.maxPacketmessage1 );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.maxPacketmessage2 );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.done );
	UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.cancel );
}

/*
=================
UI_GameOptions_Precache
=================
*/
void UI_GameOptions_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_GameOptions_Menu
=================
*/
void UI_GameOptions_Menu( void )
{
	UI_GameOptions_Precache();
	UI_GameOptions_Init();

	UI_GameOptions_UpdateConfig();
	UI_PushMenu( &uiGameOptions.menu );
}

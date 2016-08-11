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

#include "mathlib.h"
#include "extdll.h"
#include "const.h"
#include "basemenu.h"
#include "utils.h"
#include "keydefs.h"
#include "ref_params.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "menu_btnsbmp_table.h"

#define ART_BANNER		"gfx/shell/head_customize"

#undef ID_BACKGROUND
#undef ID_BANNER
enum
{
	ID_BACKGROUND = 0,
	ID_BANNER,
	ID_DONE,
	ID_ADVOPTIONS,
	ID_NAME,
	ID_CROSSHAIRVIEW,
	ID_CROSSHAIRSIZE,
	ID_CROSSHAIRTRANSLUCENCY,
	ID_CROSSHAIRCOLOR,
	ID_SPRAYVIEW,
	ID_SPRAYDECAL,
	ID_SPRAYCOLOR
};

#define MAX_SPRAYDECALS	50

struct uiPlayerSetup_t
{
	menuFramework_s	menu;
	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuField_s	name;
	menuPicButton_s	done;
	menuPicButton_s	AdvOptions;

	menuBitmap_s	  crosshairView;
	menuSpinControl_s crosshairSize;
	menuSpinControl_s crosshairColor;
	menuCheckBox_s    crosshairTranslucent;

#if 0
	char	sprays[MAX_SPRAYDECALS][CS_SIZE];
	int		num_sprays;
	char	currentspray[CS_SIZE];

	menuBitmap_s	  sprayView;
	menuSpinControl_s sprayDecals;
	menuSpinControl_s sprayColor;
#endif

	HIMAGE uiWhite;
};

static uiPlayerSetup_t	uiPlayerSetup;

static char g_szCrosshairAvailColors[6][CS_SIZE] =
{
	"Green", "Red", "Blue", "Yellow", "Ltblue", ""
};
static byte g_iCrosshairAvailColors[6][3] =
{
	{ 50,  250, 50  },
	{ 250, 50,  50  },
	{ 50,  50,  250 },
	{ 250, 250, 50  },
	{ 50,  250, 250 },
	{ 0,   0,   0   }
};
static char g_szCrosshairAvailSizes[4][CS_SIZE] =
{
	"auto", "small", "medium", "large"
};

/*
=================
UI_PlayerSetup_FindSprayDecals
=================
*/
#if 0
static void UI_PlayerSetup_FindSprayDecals( void )
{
	char	**filenames;
	int	numFiles;

	uiPlayerSetup.num_sprays = 0;

	// Get file list
	filenames = FS_SEARCH( "logos/*.bmp", &numFiles, FALSE );

	// build the spray list
	for( int i = 0; i < numFiles; i++ )
	{
		if( !FILE_EXISTS( filenames[i] )) continue;

		strcpy( uiPlayerSetup.sprays[uiPlayerSetup.num_sprays], filenames[i] );
		uiPlayerSetup.num_sprays++;
	}
}
#endif
/*
=================
UI_PlayerSetup_GetConfig
=================
*/
static void UI_PlayerSetup_GetConfig( void )
{
	strncpy( uiPlayerSetup.name.buffer, CVAR_GET_STRING( "name" ), sizeof( uiPlayerSetup.name.buffer ));

	char curColor[CS_SIZE];
	int rgb[3];
	strncpy( curColor, CVAR_GET_STRING("cl_crosshair_color"), CS_SIZE);
	sscanf( curColor, "%d %d %d", rgb, rgb + 1, rgb + 2 );

	// check for custom colors
	int i;
	for( i = 0; i < 5; i++)
	{
		if( rgb[0] == g_iCrosshairAvailColors[i][0] &&
			rgb[1] == g_iCrosshairAvailColors[i][1] &&
			rgb[2] == g_iCrosshairAvailColors[i][2] )
		{
			break;
		}
	}

	if( i == 5 )
	{
		strcpy( g_szCrosshairAvailColors[i], curColor );
		g_iCrosshairAvailColors[i][0] = rgb[0];
		g_iCrosshairAvailColors[i][1] = rgb[1];
		g_iCrosshairAvailColors[i][2] = rgb[2];
		uiPlayerSetup.crosshairColor.maxValue = 5;
	}
	else
	{
		g_szCrosshairAvailColors[5][0] = '\0'; // mark that we're not used custom colors
		uiPlayerSetup.crosshairColor.maxValue = 4;
	}

	uiPlayerSetup.crosshairColor.generic.name = g_szCrosshairAvailColors[i];
	uiPlayerSetup.crosshairColor.curValue = i;

	static char crosshairSize[CS_SIZE];
	strncpy( crosshairSize, CVAR_GET_STRING( "cl_crosshair_size" ), CS_SIZE );
    for( i = 0; i < 4; i++ )
	{
		if( !stricmp( crosshairSize, g_szCrosshairAvailSizes[i] ) )
		{
			uiPlayerSetup.crosshairSize.generic.name = g_szCrosshairAvailSizes[i];
			break;
		}
	}
	// fix possible errors
	if( i == 4 )
	{
		uiPlayerSetup.crosshairSize.generic.name = g_szCrosshairAvailSizes[0];
		CVAR_SET_STRING( "cl_crosshair_size", "auto" );
	}
	if( CVAR_GET_FLOAT( "cl_crosshair_translucent") )
		uiPlayerSetup.crosshairTranslucent.enabled = 1;
}

/*
=================
UI_PlayerSetup_SetConfig
=================
*/
static void UI_PlayerSetup_SetConfig( void )
{
    CVAR_SET_STRING( "name", uiPlayerSetup.name.buffer );
	char curColor[CS_SIZE];
	int i = uiPlayerSetup.crosshairColor.curValue;
	snprintf( curColor, CS_SIZE, "%i %i %i",
			  g_iCrosshairAvailColors[i][0],
			  g_iCrosshairAvailColors[i][1],
			  g_iCrosshairAvailColors[i][2]
			);
	CVAR_SET_STRING( "cl_crosshair_color", curColor );
	CVAR_SET_STRING( "cl_crosshair_size", uiPlayerSetup.crosshairSize.generic.name );
	CVAR_SET_FLOAT( "cl_crosshair_translucent", uiPlayerSetup.crosshairTranslucent.enabled );
}

/*
=================
UI_PlayerSetup_UpdateConfig
=================
*/
static void UI_PlayerSetup_UpdateConfig( void )
{
    CVAR_SET_STRING( "name", uiPlayerSetup.name.buffer );
	char curColor[CS_SIZE];
	int i = uiPlayerSetup.crosshairColor.curValue;
	snprintf( curColor, CS_SIZE, "%i %i %i",
			  g_iCrosshairAvailColors[i][0],
			  g_iCrosshairAvailColors[i][1],
			  g_iCrosshairAvailColors[i][2]
			);
	CVAR_SET_STRING( "cl_crosshair_color", curColor );
	uiPlayerSetup.crosshairColor.generic.name = g_szCrosshairAvailColors[i];

	i = uiPlayerSetup.crosshairSize.curValue;
	uiPlayerSetup.crosshairSize.generic.name = g_szCrosshairAvailSizes[i];

	CVAR_SET_STRING( "cl_crosshair_size", uiPlayerSetup.crosshairSize.generic.name );
	CVAR_SET_FLOAT( "cl_crosshair_translucent", uiPlayerSetup.crosshairTranslucent.enabled );
}

/*
=================
UI_PlayerSetup_Callback
=================
*/
static void UI_PlayerSetup_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	/*if( item->id >= ID_DONE || item->id <= ID_SPRAYCOLOR )
	{
		if( event == QM_PRESSED )
			((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_PRESSED;
		else ((menuCheckBox_s *)self)->focusPic = UI_CHECKBOX_FOCUS;
	}*/

	if( event == QM_CHANGED )
	{
		UI_PlayerSetup_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PlayerSetup_SetConfig();
		UI_PopMenu();
		break;
	case ID_ADVOPTIONS:
		UI_PlayerSetup_SetConfig();
		UI_GameOptions_Menu();
		break;
	}
}

/*
=================
UI_Crosshair_Ownerdraw
=================
*/
static void UI_Crosshair_Ownerdraw( void *self )
{
	menuBitmap_s *item = (menuBitmap_s*)self;

	UI_DrawPic(item->generic.x, item->generic.y, item->generic.width, item->generic.height, 0x00FFFFFF, "gfx/vgui/crosshair" );


	int l;
	switch( (int)uiPlayerSetup.crosshairSize.curValue )
	{
	case 1:
		l = 10;
		break;
	case 2:
		l = 20;
		break;
	case 3:
		l = 30;
		break;
	case 0:
		if( ScreenWidth < 640 )
			l = 30;
		else if( ScreenWidth < 1024 )
			l = 20;
		else l = 10;
	}

	l *= ScreenHeight / 768.0f;

	int x = item->generic.x, // xpos
		y = item->generic.y, // ypos
		w = item->generic.width, // width
		h = item->generic.height, // height
		// delta distance
		d = (item->generic.width / 2 - l) * 0.5,
		// alpha
		a = 180,
		// red
		r = g_iCrosshairAvailColors[(int)uiPlayerSetup.crosshairColor.curValue][0],
		// green
		g = g_iCrosshairAvailColors[(int)uiPlayerSetup.crosshairColor.curValue][1],
		// blue
		b = g_iCrosshairAvailColors[(int)uiPlayerSetup.crosshairColor.curValue][2];

	if( uiPlayerSetup.crosshairTranslucent.enabled )
	{
		// verical
		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawTrans(x + w / 2, y + d,         1, l );

		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawTrans(x + w / 2, y + h / 2 + d, 1, l );

		// horizontal
		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawTrans(x + d,         y + h / 2, l, 1 );

		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawTrans(x + w / 2 + d, y + h / 2, l, 1 );
	}
	else
	{
		// verical
		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawAdditive(x + w / 2, y + d,         1, l );

		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawAdditive(x + w / 2, y + h / 2 + d, 1, l );

		// horizontal
		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawAdditive(x + d,         y + h / 2, l, 1 );

		PIC_Set(uiPlayerSetup.uiWhite, r, g, b, a);
		PIC_DrawAdditive(x + w / 2 + d, y + h / 2, l, 1 );
	}

#if 0
	// verical
	FillRGBA(x + w / 2, y + d,         1, l, r, g, b, a);
	FillRGBA(x + w / 2, y + h / 2 + d, 1, l, r, g, b, a);

	// horizontal
	FillRGBA(x + d,         y + h / 2, l, 1, r, g, b, a);
	FillRGBA(x + w / 2 + d, y + h / 2, l, 1, r, g, b, a);
#endif
}

/*
=================
UI_PlayerSetup_Init
=================
*/
static void UI_PlayerSetup_Init( void )
{
	memset( &uiPlayerSetup, 0, sizeof( uiPlayerSetup_t ));

	uiPlayerSetup.menu.vidInitFunc = UI_PlayerSetup_Init;

	uiPlayerSetup.uiWhite = PIC_Load("*white");

	uiPlayerSetup.background.generic.id = ID_BACKGROUND;
	uiPlayerSetup.background.generic.type = QMTYPE_BITMAP;
	uiPlayerSetup.background.generic.flags = QMF_INACTIVE;
	uiPlayerSetup.background.generic.x = 0;
	uiPlayerSetup.background.generic.y = 0;
	uiPlayerSetup.background.generic.width = uiStatic.width;
	uiPlayerSetup.background.generic.height = 768;
	uiPlayerSetup.background.pic = ART_BACKGROUND;

	uiPlayerSetup.banner.generic.id = ID_BANNER;
	uiPlayerSetup.banner.generic.type = QMTYPE_BITMAP;
	uiPlayerSetup.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiPlayerSetup.banner.generic.x = UI_BANNER_POSX;
	uiPlayerSetup.banner.generic.y = UI_BANNER_POSY;
	uiPlayerSetup.banner.generic.width = UI_BANNER_WIDTH;
	uiPlayerSetup.banner.generic.height = UI_BANNER_HEIGHT;
	uiPlayerSetup.banner.pic = ART_BANNER;

	uiPlayerSetup.done.generic.id = ID_DONE;
	uiPlayerSetup.done.generic.type = QMTYPE_BM_BUTTON;
	uiPlayerSetup.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayerSetup.done.generic.x = 72;
	uiPlayerSetup.done.generic.y = 230;
	uiPlayerSetup.done.generic.name = "Done";
	uiPlayerSetup.done.generic.statusText = "Go back to the Multiplayer Menu";
	uiPlayerSetup.done.generic.callback = UI_PlayerSetup_Callback;

	UI_UtilSetupPicButton( &uiPlayerSetup.done, PC_DONE );

	uiPlayerSetup.AdvOptions.generic.id = ID_ADVOPTIONS;
	uiPlayerSetup.AdvOptions.generic.type = QMTYPE_BM_BUTTON;
	uiPlayerSetup.AdvOptions.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayerSetup.AdvOptions.generic.x = 72;
	uiPlayerSetup.AdvOptions.generic.y = 280;
	uiPlayerSetup.AdvOptions.generic.name = "Adv. Options";
	uiPlayerSetup.AdvOptions.generic.statusText = "Configure handness, fov and other advanced options";
	uiPlayerSetup.AdvOptions.generic.callback = UI_PlayerSetup_Callback;

	UI_UtilSetupPicButton( &uiPlayerSetup.AdvOptions, PC_ADV_OPT );

	uiPlayerSetup.name.generic.id = ID_NAME;
	uiPlayerSetup.name.generic.type = QMTYPE_FIELD;
	uiPlayerSetup.name.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayerSetup.name.generic.x = 320;
	uiPlayerSetup.name.generic.y = 260;
	uiPlayerSetup.name.generic.width = 256;
	uiPlayerSetup.name.generic.height = 36;
	uiPlayerSetup.name.generic.callback = UI_PlayerSetup_Callback;
	uiPlayerSetup.name.generic.statusText = "Enter your multiplayer display name";
	uiPlayerSetup.name.maxLength = 32;

	uiPlayerSetup.crosshairView.generic.id = ID_CROSSHAIRVIEW;
	uiPlayerSetup.crosshairView.generic.type = QMTYPE_BITMAP;
	uiPlayerSetup.crosshairView.generic.flags = QMF_INACTIVE;
	uiPlayerSetup.crosshairView.generic.x = 320;
	uiPlayerSetup.crosshairView.generic.y = 310;
	uiPlayerSetup.crosshairView.generic.width = 96;
	uiPlayerSetup.crosshairView.generic.height = 96;
	uiPlayerSetup.crosshairView.generic.callback = UI_PlayerSetup_Callback;
	uiPlayerSetup.crosshairView.generic.ownerdraw = UI_Crosshair_Ownerdraw;
	uiPlayerSetup.crosshairView.generic.statusText = "Choose dynamic crosshair";
	uiPlayerSetup.crosshairView.pic = "gfx/vgui/crosshair";

	uiPlayerSetup.crosshairSize.generic.id = ID_CROSSHAIRSIZE;
	uiPlayerSetup.crosshairSize.generic.type = QMTYPE_SPINCONTROL;
	uiPlayerSetup.crosshairSize.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayerSetup.crosshairSize.generic.x = 480;
	uiPlayerSetup.crosshairSize.generic.y = 315;
	uiPlayerSetup.crosshairSize.generic.height = 26;
	uiPlayerSetup.crosshairSize.generic.width = 256;
	uiPlayerSetup.crosshairSize.generic.callback = UI_PlayerSetup_Callback;
	uiPlayerSetup.crosshairSize.generic.statusText = "Set crosshair size";
	uiPlayerSetup.crosshairSize.minValue = 0;
	uiPlayerSetup.crosshairSize.maxValue = 3;
	uiPlayerSetup.crosshairSize.range	 = 1;

	uiPlayerSetup.crosshairColor.generic.id = ID_CROSSHAIRCOLOR;
	uiPlayerSetup.crosshairColor.generic.type = QMTYPE_SPINCONTROL;
	uiPlayerSetup.crosshairColor.generic.flags = QMF_CENTER_JUSTIFY|QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiPlayerSetup.crosshairColor.generic.x = 480;
	uiPlayerSetup.crosshairColor.generic.y = 375;
	uiPlayerSetup.crosshairColor.generic.height = 26;
	uiPlayerSetup.crosshairColor.generic.width = 256;
	uiPlayerSetup.crosshairColor.generic.callback = UI_PlayerSetup_Callback;
	uiPlayerSetup.crosshairColor.generic.statusText = "Set crosshair color";
	uiPlayerSetup.crosshairColor.minValue = 0;
	uiPlayerSetup.crosshairColor.maxValue = 4;
	uiPlayerSetup.crosshairColor.range	  = 1;

	uiPlayerSetup.crosshairTranslucent.generic.id = ID_CROSSHAIRCOLOR;
	uiPlayerSetup.crosshairTranslucent.generic.type = QMTYPE_CHECKBOX;
	uiPlayerSetup.crosshairTranslucent.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DROPSHADOW;
	uiPlayerSetup.crosshairTranslucent.generic.x = 320;
	uiPlayerSetup.crosshairTranslucent.generic.y = 420;
	uiPlayerSetup.crosshairTranslucent.generic.callback = UI_PlayerSetup_Callback;
	uiPlayerSetup.crosshairTranslucent.generic.name = "Translucent crosshair";
	uiPlayerSetup.crosshairTranslucent.generic.statusText = "Set additive render crosshair";

	UI_PlayerSetup_GetConfig();

	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.background );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.banner );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.done );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.AdvOptions );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.name );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.crosshairView );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.crosshairSize );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.crosshairTranslucent );
	UI_AddItem( &uiPlayerSetup.menu, (void *)&uiPlayerSetup.crosshairColor );

}

/*
=================
UI_PlayerSetup_Precache
=================
*/
void UI_PlayerSetup_Precache( void )
{
	PIC_Load( "gfx/vgui/crosshair" );
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_PlayerSetup_Menu
=================
*/
void UI_PlayerSetup_Menu( void )
{
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		return;

	UI_PlayerSetup_Precache();
	UI_PlayerSetup_Init();

	UI_PlayerSetup_UpdateConfig();
	UI_PushMenu( &uiPlayerSetup.menu );
}

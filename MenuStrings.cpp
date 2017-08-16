/*
MenuStrings.cpp - custom menu strings
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Utils.h"
#include "MenuStrings.h"

const char *MenuStrings[IDS_LAST] =
{
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 10
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 20
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 30
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 40
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 50
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 60
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 70
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 80
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 90
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 100
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 110
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 120
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 130
"",
"Display mode",
"",
"",
"",
"",
"",
"",
"",
"",	// 140
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 150
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 160
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 170
"Reverse mouse",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 180
"",
"",
"",
"Mouse sensitivity",
"",
"",
"",
"Return to game.",
"Start a new game.",
"",	// 190
"Load a previously saved game.",
"Load a saved game, save the current game.",
"Change game settings, configure controls",
"",
"",
"",
"",
"",
"",
"",	// 200
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 210
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 220
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 230
"",
"",
"",
"Starting a Hazard Course will exit\nany current game, OK to exit?",
"",	// filled in UI_LoadCustomStrings
"Are you sure you want to quit?",
"",
"",
"",
"Starting a new game will exit\nany current game, OK to exit?",	// 240
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 250
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 260
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 270
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 280
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 290
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 300
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 310
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 320
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 330
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 340
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 350
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 360
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 370
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 380
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 390
"",
"",
"",
"",
"",
"",
"",
"",
"",
"Find more about Valve's product lineup",	// 400
"",
"http://store.steampowered.com/app/70/",
"",
"",
"",
"",
"",
"",
"",
"",	// 410
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 420
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 430
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 440
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 450
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 460
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 470
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 480
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 490
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 500
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 510
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 520
"",
"",
"",
"",
"",
"",
"",
"",
"",
"Select a custom game",	// 530
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 540
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 550
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 560
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 570
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 580
"",
"",
"",
"",
"",
"",
"",
"",
"",
"",	// 590
"",
"",
"",
"",
"",
"",
"",
"",
"", // 599
};

void UI_InitAliasStrings( void )
{
	char token[1024];

	// some strings needs to be initialized here
	sprintf( token, "Quit %s without\nsaving current game?", gMenu.m_gameinfo.title );
	MenuStrings[IDS_MAIN_QUITPROMPTINGAME] = StringCopy( token );

	sprintf( token, "Learn how to play %s", gMenu.m_gameinfo.title );
	MenuStrings[IDS_MAIN_TRAININGHELP] = StringCopy( token );

	sprintf( token, "Play %s on the 'easy' skill setting", gMenu.m_gameinfo.title );
	MenuStrings[IDS_NEWGAME_EASYHELP] = StringCopy( token );

	sprintf( token, "Play %s on the 'medium' skill setting", gMenu.m_gameinfo.title );
	MenuStrings[IDS_NEWGAME_MEDIUMHELP] = StringCopy( token );

	sprintf( token, "Play %s on the 'difficult' skill setting", gMenu.m_gameinfo.title );
	MenuStrings[IDS_NEWGAME_DIFFICULTHELP] = StringCopy( token );

	sprintf( token, "Quit playing %s", gMenu.m_gameinfo.title );
	MenuStrings[IDS_MAIN_QUITHELP] = StringCopy( token );

	sprintf( token, "Search for %s servers, configure character", gMenu.m_gameinfo.title );
	MenuStrings[IDS_MAIN_MULTIPLAYERHELP] = StringCopy( token );
}

void UI_LoadCustomStrings( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/strings.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int string_num;

	UI_InitAliasStrings ();

	if( !afile )
		return;

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		if( isdigit( token[0] ))
		{
			string_num = atoi( token );

			// check for bad stiringnum
			if( string_num < 0 ) continue;
			if( string_num > ( IDS_LAST - 1 ))
				continue;
		}
		else continue; // invalid declaration ?

		// parse new string 
		pfile = EngFuncs::COM_ParseFile( pfile, token );
		MenuStrings[string_num] = StringCopy( token ); // replace default string with custom
	}

	EngFuncs::COM_FreeFile( afile );
}

/*
vgui_parser.cpp - implementation of VGUI *.res parser
Copyright (C) 2015 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#pragma once
#ifndef VGUI_PARSER_H
#define VGUI_PARSER_H

void Localize_Init( );
void Localize_Free( );

const char* Localize( const char* string );
void StripEndNewlineFromString( char *str );
#endif

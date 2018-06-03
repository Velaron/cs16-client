/*
Primitive.h - menu primitives
Copyright (C) 2017 a1batross

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
#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#define BIT( x ) ( 1U << x )

#ifdef _MSC_VER
#pragma warning(disable:4244) // float->int
#endif

// engine constants
enum
{
	GAME_NORMAL = 0,
	GAME_SINGLEPLAYER_ONLY,
	GAME_MULTIPLAYER_ONLY
};

enum
{
	KEY_CONSOLE = 0,
	KEY_GAME,
	KEY_MENU
};

enum
{
	QMF_GRAYED             = BIT( 1 ), // Grays and disables
	QMF_INACTIVE           = BIT( 2 ), // Disables any input
	QMF_DROPSHADOW         = BIT( 4 ),
	QMF_SILENT             = BIT( 5 ), // Don't play sounds
	QMF_HASMOUSEFOCUS      = BIT( 6 ),
	QMF_MOUSEONLY          = BIT( 7 ), // Only mouse input allowed
	QMF_NOTIFY             = BIT( 9 ), // draw notify at right screen side
	QMF_ACT_ONRELEASE      = BIT( 10 ), // call Key_Event when button is released
	QMF_HASKEYBOARDFOCUS   = BIT( 11 ),
	QMF_DIALOG             = BIT( 12 ), // modal windows. Will grab key, char and mousemove events
	QMF_DISABLESCAILING    = BIT( 13 ), // disables CalcPosition and CalcSizes

	QMF_HIDDENBYPARENT     = BIT( 30 ), // parent set this flag and don't want to draw this control
	QMF_HIDDEN             = BIT( 31 ), // INTERNAL USE ONLY: Use Show/Hide/SetVisibility/ToggleVisibility
};


enum ETextAlignment
{
	QM_CENTER		 = 0,
	QM_TOP           = BIT( 0 ),
	QM_BOTTOM        = BIT( 1 ),
	QM_LEFT          = BIT( 2 ),
	QM_RIGHT         = BIT( 3 ),

	QM_TOPLEFT       = QM_TOP     | QM_LEFT,
	QM_TOPRIGHT      = QM_TOP     | QM_RIGHT,
	QM_BOTTOMLEFT    = QM_BOTTOM  | QM_LEFT,
	QM_BOTTOMRIGHT   = QM_BOTTOM  | QM_RIGHT,
};

enum EFontSizes
{
	QM_DEFAULTFONT = 0, // medium size font
	QM_SMALLFONT,       // small
	QM_BIGFONT,         // big
	QM_BOLDFONT,
#ifdef MAINUI_RENDER_PICBUTTON_TEXT
	QM_LIGHTBLUR,
	QM_HEAVYBLUR
#endif
};

enum EFocusAnimation
{
	QM_NOFOCUSANIMATION = 0,
	QM_HIGHLIGHTIFFOCUS,      // just simple hightlight
	QM_PULSEIFFOCUS           // pulse animation
};

enum ELetterCase
{
	QM_NOLETTERCASE = 0,
	QM_LOWERCASE,
	QM_UPPERCASE
};

typedef int HFont; // handle to a font

struct Point
{
	Point() : x(0), y(0) {}
	Point( int x, int y ) : x(x), y(y) {}

	int x, y;
	Point Scale();
	friend Point operator +( Point &a, Point &b ) { return Point( a.x + b.x, a.y + b.y ); }
	friend Point operator -( Point &a, Point &b ) { return Point( a.x - b.x, a.y - b.y ); }

	Point& operator+=( Point &a )
	{
		x += a.x;
		y += a.y;
		return *this;
	}

	Point& operator-=( Point &a )
	{
		x -= a.x;
		y -= a.y;
		return *this;
	}

	Point operator *( float scale ) { return Point( (int)(x * scale), (int)(y * scale) ); }
	Point operator /( float scale ) { return Point( (int)(x / scale), (int)(y / scale) ); }
};

struct Size
{
	Size() : w(0), h(0) {}
	Size( int w, int h ) : w(w), h(h) {}

	int w, h;
	Size Scale();
};


#endif // PRIMITIVE_H

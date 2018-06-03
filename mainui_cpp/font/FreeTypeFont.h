/*
FreeTypeFont.h -- freetype2 font renderer
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
#if !defined( _WIN32 ) && !defined( FREETYPEFONT_H )
#define FREETYPEFONT_H

#if defined(MAINUI_USE_CUSTOM_FONT_RENDER) && defined(MAINUI_USE_FREETYPE)

#include "BaseFontBackend.h"

extern "C"
{
    #include <fontconfig/fontconfig.h>
    #include <ft2build.h>
    #include FT_FREETYPE_H
}

#include "utl/utlmemory.h"
#include "utl/utlrbtree.h"

struct abc_t
{
	int ch;
	int a, b, c;
};

class CFreeTypeFont : public CBaseFont
{
public:
	CFreeTypeFont();
	~CFreeTypeFont();

	bool Create(const char *name,
		int tall, int weight,
		int blur, float brighten,
		int outlineSize,
		int scanlineOffset, float scanlineScale,
		int flags);
	void GetCharRGBA(int ch, Point pt, Size sz, unsigned char *rgba, Size &drawSize);
	bool IsValid() const;
	void GetCharABCWidths( int ch, int &a, int &b, int &c );
	bool HasChar( int ch ) const;
private:
	CUtlRBTree<abc_t, int> m_ABCCache;

	FT_Face face;
	static FT_Library m_Library;
	char m_szRealFontFile[4096];
	bool FindFontDataFile(const char *name, int tall, int weight, int flags, char *dataFile, int dataFileChars);

	friend class CFontManager;
};

#endif // defined(MAINUI_USE_CUSTOM_FONT_RENDER) && defined(MAINUI_USE_FREETYPE)

#endif // FREETYPEFONT_H

/*
WinAPIFont.h - Win32 Font backend
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
#if !defined(WINAPIFONT_H) && defined(MAINUI_USE_CUSTOM_FONT_RENDER) && defined(_WIN32)
#define WINAPIFONT_H

#define WIN32_LEAN_AND_MEAN
#define UNICODE // use unicode fonts
#include <windows.h>
#undef GetCharABCWidths

#include "BaseFontBackend.h"
#include "FontManager.h"

#include "utl/utlmemory.h"
#include "utl/utlrbtree.h"

struct abc_t
{
	int ch;
	int a, b, c;
};

class CWinAPIFont : public CBaseFont
{
public:
	CWinAPIFont( );
	~CWinAPIFont( );

	bool Create( const char *name,
		int tall, int weight,
		int blur, float brighten,
		int outlineSize,
		int scanlineOffset, float scanlineScale,
		int flags );
	void GetCharRGBA( int ch, Point pt, Size sz, unsigned char *rgba, Size &drawSize );
	bool IsValid( ) const;
	void GetCharABCWidths( int ch, int &a, int &b, int &c );
	bool HasChar( int ch ) const;

	bool m_bFound;

private:
	CUtlRBTree<abc_t, int> m_ABCCache;

	HFONT m_hFont;
	HDC m_hDC;
	HBITMAP m_hDIB;


	int m_rgiBitmapSize[2];

	// pointer to buffer for use when generated bitmap versions of a texture

	unsigned char	*m_pBuf;


	friend class CFontManager;
	friend int CALLBACK FontEnumProc( const LOGFONT *, const TEXTMETRIC *, DWORD, LPARAM lpParam );
};

#endif // WINAPIFONT_H

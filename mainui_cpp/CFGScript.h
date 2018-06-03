/*
cfgscript.c - "Valve script" parsing routines
Copyright (C) 2016 mittorn

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
#ifndef CFGSCRIPT_H
#define CFGSCRIPT_H

#define MAX_STRING 256

#include "StringArrayModel.h"

typedef enum
{
	T_NONE = 0,
	T_BOOL,
	T_NUMBER,
	T_LIST,
	T_STRING,
	T_COUNT
} cvartype_t;

typedef struct scrvarlistentry_s
{
	char *szName;
	float flValue;
	struct scrvarlistentry_s *next;
} scrvarlistentry_t;

typedef struct scrvarlist_s
{
	int iCount;
	scrvarlistentry_t *pEntries;
	scrvarlistentry_t *pLast;
	const char **pArray;
	CStringArrayModel *pModel; // ready model for use in UI
} scrvarlist_t;

typedef struct
{
	float fMin;
	float fMax;
} scrvarnumber_t;

struct scrvardef_t
{
	scrvardef_t() :
		flags(0), name(), value(), desc(),
		type(T_NONE), next(0) {}

	int flags;
	char name[MAX_STRING];
	char value[MAX_STRING];
	char desc[MAX_STRING];
	union
	{
		scrvarnumber_t number;
		scrvarlist_t list;
	};
	cvartype_t type;
	struct scrvardef_t *next;
};

scrvardef_t *CSCR_LoadDefaultCVars( const char *scriptfilename, int *count );
void CSCR_FreeList( scrvardef_t *list );

#endif // CFGSCRIPT_H

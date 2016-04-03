/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

#include "port.h"

#include <string.h>
#include "wrect.h" // need for cl_dll.h
#include "cl_dll.h"
#include "vgui_parser.h"
#include "unicode_strtools.h"

#include "errno.h"

#include <unordered_map>
#include <string>

// for localized titles.txt strings
using namespace std;
typedef unordered_map< string, char* > CDict;

CDict gTitlesTXT;

const char *Localize( const char *szStr )
{
	StripEndNewlineFromString( (char *)szStr );
	auto got = gTitlesTXT.find( string(szStr) );

	// if iterator points to end, then 'key' not found in dictionary
	if( got == gTitlesTXT.end() )
		return szStr;

	return got->second;
}

void Localize_Init( )
{
	const char *gamedir = gEngfuncs.pfnGetGameDirectory( );

	char filename[64];
	snprintf( filename, sizeof( filename ), "%s/resource/%s_english.txt", gamedir, gamedir );

#ifndef OPENBINARY
	FILE *wf = fopen( filename, "r" );
#else
	FILE *wf = fopen( filename, "rb" );
#endif

	if( !wf )
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s. Strings will not be localized!.\n", filename );
		return;
	}

	fseek( wf, 0L, SEEK_END );
	int unicodeLength = ftell( wf );
	fseek( wf, 0L, SEEK_SET );

	uchar16 *unicodeBuf = new uchar16[unicodeLength];
	int totalRead = fread( unicodeBuf, 1, unicodeLength, wf );
	if( totalRead == unicodeLength ) // no problem, so read it.
	{
		int ansiLength = totalRead / 2;
		char *afile = new char[ansiLength]; // save original pointer, so we can free it later
		char *pfile = afile;
		char *token = new char[MAX_LOCALIZEDSTRING_SIZE];

		Q_UTF16ToUTF8( unicodeBuf + 1, afile, ansiLength, STRINGCONVERT_ASSERT_REPLACE );

		while( (pfile = gEngfuncs.COM_ParseFile( pfile, token )) && gTitlesTXT.size() < gTitlesTXT.max_size() )
		{
			if( strlen( token ) > 5 )
			{
				char szLocString[MAX_LOCALIZEDSTRING_SIZE];
				pfile = gEngfuncs.COM_ParseFile( pfile, szLocString );

				if( pfile && gTitlesTXT.size() < gTitlesTXT.max_size() )
				{
					size_t iLen = strlen( szLocString ) + 1;
					char *szLocCopyString = new char[iLen];
					strncpy(szLocCopyString, szLocString, iLen );
					gTitlesTXT[ string(token) ] = szLocCopyString;
				}
			}
		}

		delete[] token;
		delete[] afile;
	}
	else
	{
		gEngfuncs.Con_Printf( "Warning: total read of %s differs from size! Error: %s\n", filename, strerror( errno ));
		gEngfuncs.Con_Printf( "Couldn't open file %s. Strings will not be localized!.\n", filename );
	}

	fclose( wf );
	delete[] unicodeBuf;
}

void Localize_Free( )
{
	for( auto it = gTitlesTXT.begin(); it != gTitlesTXT.end(); ++it )
	{
		delete[] it->second;
	}
	gTitlesTXT.clear();
	return;
}

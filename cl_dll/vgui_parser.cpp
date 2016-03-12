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

#include <string.h>
#include "wrect.h" // need for cl_dll.h
#include "cl_dll.h"
#include "vgui_parser.h"
#include "unicode_strtools.h"

#include <unordered_map>
#include <stdexcept> // std::out_of_range

// for localized titles.txt strings
typedef std::unordered_map< std::string, const char * > CDict;

CDict gTitlesTXT;

const char *Localize( const char *string )
{
	StripEndNewlineFromString( (char *)string );
	std::string	key(string);
	CDict::const_iterator got = gTitlesTXT.find(key);

	// if iterator points to end, then it 'key' not found in dictionary
	if( got != gTitlesTXT.end() || got->first != key )
		return got->second;
	else return string;
}

void Localize_Init( )
{
	char filename[64];
	const char *gamedir = gEngfuncs.pfnGetGameDirectory( );
	snprintf( filename, sizeof( filename ), "%s/resource/%s_english.txt", gamedir, gamedir );
#ifndef OPENBINARY
	FILE *wf = fopen( filename, "r" );
#else
	FILE *wf = fopen( filename, "rb" );
#endif

	if ( !wf )
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s. Strings will not be localized!.\n", filename );
		return;
	}

	fseek( wf, 0L, SEEK_END );
	int unicodeLength = ftell( wf );
	fseek( wf, 0L, SEEK_SET );

	uchar16 *unicodeBuf = new uchar16[unicodeLength];
	fread( unicodeBuf, 1, unicodeLength, wf );
	fclose( wf );
	unicodeBuf++;

	int ansiLength = unicodeLength / 2;

	char *afile = new char[ansiLength]; // save original pointer, so we can free it later
	char *token = new char[MAX_LOCALIZEDSTRING_SIZE];
	char *pfile = afile;
	Q_UTF16ToUTF8( unicodeBuf, afile, ansiLength, STRINGCONVERT_ASSERT_REPLACE );

	while ( true )
	{
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
		if ( !pfile )
		{
			break;
		}

		if ( strlen( token ) > 5 )
		{
			char *localizedString = new char[MAX_LOCALIZEDSTRING_SIZE];
			pfile = gEngfuncs.COM_ParseFile( pfile, localizedString );

			if( pfile && gTitlesTXT.size() < gTitlesTXT.max_size() )
			{
				gTitlesTXT[ std::string(token) ] = localizedString;
			}
			else
			{
				delete[] localizedString;
				break;
			}
		}
	}

	delete[] token;
	delete[] --unicodeBuf;
	delete[] afile;
}

void Localize_Free( )
{
	for (auto& x: gTitlesTXT)
	{
		gEngfuncs.Con_DPrintf("Destroying pair: %s %s\n", x.first, x.second);
		delete []x.second;
	}
	return;
}

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

struct locString
{
	char toLocalize[MAX_TOLOCALIZE_STRING_SIZE];
	char localizedString[MAX_LOCALIZEDSTRING_SIZE];
};

locString gTitlesTXT[MAX_LOCALIZED_TITLES]; // for localized titles.txt strings
//locString gCstrikeMsgs[1024]; // for another
int giLastTitlesTXT = 0;
const char* Localize( const char* string )
{
	StripEndNewlineFromString( (char*)string );
        
	for( int i = 0; i < giLastTitlesTXT; ++i )
	{
		if( !stricmp(gTitlesTXT[i].toLocalize, string))
			return gTitlesTXT[i].localizedString;
	}
	// nothing was found
	return string;
}

void Localize_Init ()
{
	char filename[64];
	const char *gamedir = gEngfuncs.pfnGetGameDirectory();
	snprintf( filename, 64, "%s/resource/%s_english.txt", gamedir, gamedir );
#ifndef OPENBINARY
	FILE *wf = fopen( filename, "r" );
#else
	FILE *wf = fopen( filename, "rb" );
#endif
	
	if (!wf)
	{
		gEngfuncs.Con_Printf ("Couldn't open file %s. Strings will not be localized!.\n", filename);
		return;
	}

	fseek (wf, 0L, SEEK_END);
	int unicodeLength = ftell (wf);
	fseek (wf, 0L, SEEK_SET);

	uchar16 *unicodeBuf = new uchar16[unicodeLength];
	fread (unicodeBuf, 1, unicodeLength, wf);
	fclose (wf);
	unicodeBuf++;

	int ansiLength = unicodeLength / 2;

	char *afile = new char[ansiLength]; // save original pointer, so we can free it later
	char *pfile = afile;
	Q_UTF16ToUTF8 (unicodeBuf, afile, ansiLength, STRINGCONVERT_ASSERT_REPLACE);

	char token[2048];
	while (true)
	{
		if (giLastTitlesTXT > MAX_LOCALIZED_TITLES)
		{
			gEngfuncs.Con_Printf ("Too many localized titles.txt strings\n");
			break;
		}

		pfile = gEngfuncs.COM_ParseFile (pfile, token);
		if(!pfile) break;

		if (strlen (token) > 5)
		{
			strncpy (gTitlesTXT[giLastTitlesTXT].toLocalize, token, MAX_TOLOCALIZE_STRING_SIZE);

			pfile = gEngfuncs.COM_ParseFile (pfile, gTitlesTXT[giLastTitlesTXT].localizedString);
		
			if(!pfile) break;
			giLastTitlesTXT++;
		}
	}

	delete [] --unicodeBuf;
	delete [] afile;
}

void Localize_Free( )
{
	return;
}

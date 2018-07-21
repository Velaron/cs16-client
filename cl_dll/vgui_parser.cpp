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
#include <ctype.h>

#include "interface.h"

// for localized titles.txt strings
typedef struct base_command_hashmap_s
{
	const char					*name;
	const char             		*value;    // key for searching
	struct base_command_hashmap_s *next;
} base_command_hashmap_t;

#define HASH_SIZE 256 // 256 * 4 * 4 == 4096 bytes
static base_command_hashmap_t *hashed_cmds[HASH_SIZE];

/*
=================
Com_HashKey

returns hash key for string
=================
*/
static uint Com_HashKey( const char *string, uint hashSize )
{
	uint	i, hashKey = 0;

	for( i = 0; string[i]; i++ )
	{
		hashKey = (hashKey + i) * 37 + tolower( string[i] );
	}

	return (hashKey % hashSize);
}

/*
============
BaseCmd_FindInBucket

Find base command in bucket
============
*/
base_command_hashmap_t *BaseCmd_FindInBucket( base_command_hashmap_t *bucket, const char *name )
{
	base_command_hashmap_t *i = bucket;
	for( ; i && strcasecmp( name, i->name ); // filter out
		 i = i->next );

	return i;
}

/*
============
BaseCmd_GetBucket

Get bucket which contain basecmd by given name
============
*/
base_command_hashmap_t *BaseCmd_GetBucket( const char *name )
{
	return hashed_cmds[ Com_HashKey( name, HASH_SIZE ) ];
}

const char *Localize( const char *szStr )
{
	char *str = strdup( szStr );
	StripEndNewlineFromString( str );
	
	base_command_hashmap_t *base = BaseCmd_GetBucket( str );
	base_command_hashmap_t *found = BaseCmd_FindInBucket( base, str );
	
	free( str );

	if( found )
		return found->value;
	return "";
}

/*
============
BaseCmd_Insert

Add new typed base command to hashmap
============
*/
void BaseCmd_Insert( const char *name, const char *second )
{
	uint hash = Com_HashKey( name, HASH_SIZE );
	base_command_hashmap_t *elem;

	elem = new base_command_hashmap_t;
	elem->name = strdup(name);
	elem->value = strdup(second);
	elem->next   = hashed_cmds[hash];
	hashed_cmds[hash] = elem;
}


static void Localize_AddToDictionary( const char *gamedir, const char *name, const char *lang )
{
	char filename[64];
	snprintf( filename, sizeof( filename ), "resource/%s_%s.txt", name, lang );

	int unicodeLength;
	uchar16 *unicodeBuf = (uchar16*)gEngfuncs.COM_LoadFile( filename, 5, &unicodeLength );

	if( unicodeBuf ) // no problem, so read it.
	{
		int ansiLength = unicodeLength / 2;
		char *afile = new char[ansiLength]; // save original pointer, so we can free it later
		char *pfile = afile;
		char *token = new char[MAX_LOCALIZEDSTRING_SIZE];
		int i = 0;

		Q_UTF16ToUTF8( unicodeBuf + 1, afile, ansiLength, STRINGCONVERT_ASSERT_REPLACE );

		pfile = gEngfuncs.COM_ParseFile( pfile, token );

		if( stricmp( token, "lang" ))
		{
			gEngfuncs.Con_Printf( "Localize_AddToDict( %s, %s, %s ): invalid header, got %s", gamedir, name, lang, token );
			goto error;
		}

		pfile = gEngfuncs.COM_ParseFile( pfile, token );

		if( strcmp( token, "{" ))
		{
			gEngfuncs.Con_Printf( "Localize_AddToDict( %s, %s, %s ): want {, got %s", gamedir, name, lang, token );
			goto error;
		}

		pfile = gEngfuncs.COM_ParseFile( pfile, token );

		if( stricmp( token, "Language" ))
		{
			gEngfuncs.Con_Printf( "Localize_AddToDict( %s, %s, %s ): want Language, got %s", gamedir, name, lang, token );
			goto error;
		}

		// skip language actual name
		pfile = gEngfuncs.COM_ParseFile( pfile, token );

		pfile = gEngfuncs.COM_ParseFile( pfile, token );

		if( stricmp( token, "Tokens" ))
		{
			gEngfuncs.Con_Printf( "Localize_AddToDict( %s, %s, %s ): want Tokens, got %s", gamedir, name, lang, token );
			goto error;
		}

		pfile = gEngfuncs.COM_ParseFile( pfile, token );

		if( strcmp( token, "{" ))
		{
			gEngfuncs.Con_Printf( "Localize_AddToDict( %s, %s, %s ): want { after Tokens, got %s", gamedir, name, lang, token );
			goto error;
		}

		while( (pfile = gEngfuncs.COM_ParseFile( pfile, token )))
		{
			if( !strcmp( token, "}" ))
				break;

			char szLocString[MAX_LOCALIZEDSTRING_SIZE];
			pfile = gEngfuncs.COM_ParseFile( pfile, szLocString );

			if( !strcmp( szLocString, "}" ))
				break;
			

			if( pfile )
			{				
				BaseCmd_Insert( token, szLocString );
				i++;
			}
		}

		gEngfuncs.Con_Printf( "Localize_AddToDict: loaded %i words from %s\n", i, filename );

error:
		delete[] token;
		delete[] afile;

		gEngfuncs.COM_FreeFile( unicodeBuf );
	}
	else
	{
		gEngfuncs.Con_Printf( "Couldn't open file %s. Strings will not be localized!.\n", filename );
	}
}

void Localize_Init( )
{
	const char *gamedir = gEngfuncs.pfnGetGameDirectory( );
	
	memset( hashed_cmds, 0, sizeof( hashed_cmds ) );

	Localize_AddToDictionary( gamedir, gamedir,  "english" );
	Localize_AddToDictionary( "valve", "valve",  "english" );
	Localize_AddToDictionary( "valve", "gameui", "english" );
}

void Localize_Free( )
{
	
	for( int i = 0; i < HASH_SIZE; i++ )
	{
		base_command_hashmap_t *base = hashed_cmds[i];
		while( base )
		{
			base_command_hashmap_t *next = base->next;
			
			delete [] base->value;
			delete [] base->name;
			delete base;
			
			base = next;
		}
	}
	
	return;
}

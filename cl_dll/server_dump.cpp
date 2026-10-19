/*
 * server_dump.cpp - dump de servidores do CS 1.6.
 *
 * Comando: dump_servidor  (abre o painel; o painel tambem fica no ImGui)
 *
 * Fontes:
 *   master  - hl1master.steampowered.com:27010 (protocolo de master do GoldSrc)
 *   lan     - broadcast em 27015 + 127.0.0.1:27015
 *   manual  - lista de ip:porta digitada no proprio painel
 *
 * Consulta: A2S_INFO ('T' / "details"), A2S_PLAYER ('U') e A2S_RULES ('V').
 * O dump roda em thread separada e o painel mostra progresso e o arquivo salvo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <utility>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>

#include "hud.h"
#include "ui/hub_menu.h"
#include "cl_util.h"
#include "imgui.h"
#include "imgui_host.h"
#include "server_dump.h"

/* ------------------------------------------------------------------ */
/* opcoes                                                             */
/* ------------------------------------------------------------------ */

enum
{
	DUMP_F_ADDRESS   = 1 << 0,
	DUMP_F_NAME      = 1 << 1,
	DUMP_F_MAP       = 1 << 2,
	DUMP_F_PLAYERS   = 1 << 3,
	DUMP_F_PING      = 1 << 4,
	DUMP_F_RULES     = 1 << 5,
	DUMP_F_PLAYERLIST= 1 << 6,
	DUMP_F_VERSION   = 1 << 7
};

enum
{
	DUMP_SRC_MASTER = 0,
	DUMP_SRC_LAN    = 1,
	DUMP_SRC_MANUAL = 2,
	DUMP_SRC_HISTORY = 3
};

enum
{
	DUMP_FMT_TXT = 0,
	DUMP_FMT_CSV = 1,
	DUMP_FMT_JSON = 2
};

struct DumpServer
{
	std::string addr;
	int port;
	std::string name;
	std::string map;
	std::string folder;
	std::string game;
	std::string version;
	int players;
	int maxplayers;
	int bots;
	int ping;
	int secure;
	bool ok;
	std::vector< std::pair<std::string, std::string> > rules;
	std::vector<std::string> playerNames;
	std::vector<int> playerScores;

	DumpServer()
		: port( 0 ), players( 0 ), maxplayers( 0 ), bots( 0 ), ping( -1 ), secure( 0 ), ok( false )
	{
	}
};

struct DumpOptions
{
	std::string path;
	int format;
	int fields;
	int timeoutMs;
	int maxServers;
	int source;
	std::string manual;
};

/* ------------------------------------------------------------------ */
/* estado compartilhado com a UI                                      */
/* ------------------------------------------------------------------ */

static std::mutex g_mutex;
static std::thread g_thread;
static std::atomic<bool> g_shutdown( false );
static std::atomic<bool> g_running( false );
static std::atomic<bool> g_cancel( false );

static std::string g_status = "Parado";
static std::string g_lastFile;
static std::string g_log;
static int g_progressDone = 0;
static int g_progressTotal = 0;
static int g_found = 0;

/* cvars */
static cvar_t *cv_path;
static cvar_t *cv_format;
static cvar_t *cv_fields;
static cvar_t *cv_source;
static cvar_t *cv_timeout;
static cvar_t *cv_max;
static cvar_t *cv_manual;
static cvar_t *cv_auto;
static bool g_autoDone = false;

/* painel */

static bool g_open = false;
static char g_pathBuf[512];
static char g_manualBuf[4096];

/* ------------------------------------------------------------------ */
/* utilidades de texto                                                */
/* ------------------------------------------------------------------ */

static void Log( const char *fmt, ... )
{
	char buf[512];
	va_list ap;

	va_start( ap, fmt );
	vsnprintf( buf, sizeof( buf ), fmt, ap );
	va_end( ap );

	if( !g_shutdown )
		gEngfuncs.Con_Printf( "[dump] %s\n", buf );

	std::lock_guard<std::mutex> lock( g_mutex );
	g_log += buf;
	g_log += "\n";
	if( g_log.size() > 6000 )
		g_log.erase( 0, g_log.size() - 6000 );
}

static void SetStatus( const char *s )
{
	std::lock_guard<std::mutex> lock( g_mutex );
	g_status = s;
}

static std::string Trim( const std::string &in )
{
	size_t a = in.find_first_not_of( " \t\r\n" );
	size_t b = in.find_last_not_of( " \t\r\n" );

	if( a == std::string::npos )
		return std::string();
	return in.substr( a, b - a + 1 );
}

static std::string JsonEscape( const std::string &in )
{
	std::string out;
	size_t i;

	for( i = 0; i < in.size(); i++ )
	{
		char c = in[i];

		switch( c )
		{
		case '"': out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if( (unsigned char)c < 0x20 )
			{
				char tmp[8];

				snprintf( tmp, sizeof( tmp ), "\\u%04x", (unsigned char)c );
				out += tmp;
			}
			else
			{
				out += c;
			}
			break;
		}
	}
	return out;
}

static std::string CsvEscape( const std::string &in )
{
	std::string out = "\"";
	size_t i;

	for( i = 0; i < in.size(); i++ )
	{
		if( in[i] == '"' )
			out += "\"\"";
		else
			out += in[i];
	}
	out += "\"";
	return out;
}

static std::string ExpandHome( const std::string &in )
{
	if( !in.empty() && in[0] == '~' )
	{
		const char *home = getenv( "HOME" );

		if( home )
			return std::string( home ) + in.substr( 1 );
	}
	return in;
}

static bool IsDirectory( const std::string &p )
{
	struct stat st;

	if( stat( p.c_str(), &st ) != 0 )
		return false;
	return S_ISDIR( st.st_mode );
}

static void MakeDirs( const std::string &p )
{
	size_t i;

	for( i = 1; i < p.size(); i++ )
	{
		if( p[i] == '/' )
		{
			std::string part = p.substr( 0, i );

			if( !part.empty() )
				mkdir( part.c_str(), 0755 );
		}
	}
	mkdir( p.c_str(), 0755 );
}

/* ------------------------------------------------------------------ */
/* socket / protocolo A2S                                             */
/* ------------------------------------------------------------------ */

static int UdpSocket( int timeoutMs )
{
	int s = socket( AF_INET, SOCK_DGRAM, 0 );
	struct timeval tv;

	if( s < 0 )
		return -1;

	tv.tv_sec = timeoutMs / 1000;
	tv.tv_usec = ( timeoutMs % 1000 ) * 1000;
	setsockopt( s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof( tv ) );
	setsockopt( s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof( tv ) );
	return s;
}

static int SendTo( int s, const char *ip, int port, const unsigned char *data, int len )
{
	struct sockaddr_in dst;
	struct hostent *he;

	memset( &dst, 0, sizeof( dst ) );
	dst.sin_family = AF_INET;
	dst.sin_port = htons( (uint16_t)port );

	if( inet_pton( AF_INET, ip, &dst.sin_addr ) != 1 )
	{
		he = gethostbyname( ip );
		if( !he || !he->h_addr_list[0] )
			return -1;
		memcpy( &dst.sin_addr, he->h_addr_list[0], sizeof( dst.sin_addr ) );
	}

	return sendto( s, data, len, 0, (struct sockaddr *)&dst, sizeof( dst ) ) == len ? 0 : -1;
}

static int RecvFrom( int s, unsigned char *buf, int buflen, char *fromIp, int fromIpLen, int *fromPort )
{
	struct sockaddr_in src;
	socklen_t slen = sizeof( src );
	int n;

	n = (int)recvfrom( s, buf, buflen, 0, (struct sockaddr *)&src, &slen );
	if( n <= 0 )
		return n;

	if( fromIp && fromIpLen > 0 )
		inet_ntop( AF_INET, &src.sin_addr, fromIp, fromIpLen );
	if( fromPort )
		*fromPort = ntohs( src.sin_port );
	return n;
}

/* leitores com limite, nunca passam do fim do pacote */
static bool RdByte( const unsigned char *buf, int len, int *pos, int *out )
{
	if( *pos + 1 > len )
		return false;
	*out = buf[*pos];
	(*pos)++;
	return true;
}

static bool RdShort( const unsigned char *buf, int len, int *pos, int *out )
{
	if( *pos + 2 > len )
		return false;
	*out = buf[*pos] | ( buf[*pos + 1] << 8 );
	*pos += 2;
	return true;
}

static bool RdLong( const unsigned char *buf, int len, int *pos, long *out )
{
	if( *pos + 4 > len )
		return false;
	*out = (long)( buf[*pos] | ( buf[*pos + 1] << 8 ) | ( buf[*pos + 2] << 16 ) | ( buf[*pos + 3] << 24 ) );
	*pos += 4;
	return true;
}

static bool RdFloat( const unsigned char *buf, int len, int *pos, float *out )
{
	long v;

	if( !RdLong( buf, len, pos, &v ) )
		return false;
	*out = *(float *)&v;
	return true;
}

static bool RdStr( const unsigned char *buf, int len, int *pos, std::string *out )
{
	int start = *pos;

	while( *pos < len && buf[*pos] != 0 )
		(*pos)++;
	if( *pos >= len )
		return false;

	out->assign( (const char *)buf + start, (size_t)( *pos - start ) );
	(*pos)++;
	return true;
}

static bool LooksLikeInfo( const unsigned char *buf, int n )
{
	if( n < 6 )
		return false;
	if( buf[0] != 0xFF || buf[1] != 0xFF || buf[2] != 0xFF || buf[3] != 0xFF )
		return false;
	return buf[4] == 0x49 || buf[4] == 0x6D;
}

/* A2S_INFO: 'T' "Source Engine Query" e, como fallback, "details" */
static bool QueryInfo( const char *ip, int port, DumpServer *sv, int timeoutMs )
{
	unsigned char req[64];
	unsigned char buf[8192];
	int s, n, len, pos, i;
	long appid = 0;

	struct timespec t0, t1;

	sv->addr = ip;
	sv->port = port;
	sv->ok = false;

	s = UdpSocket( timeoutMs );
	if( s < 0 )
		return false;

	len = 0;
	req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF;
	req[len++] = 0x54;
	memcpy( req + len, "Source Engine Query", 19 );
	len += 19;
	req[len++] = 0;

	clock_gettime( CLOCK_MONOTONIC, &t0 );
	if( SendTo( s, ip, port, req, len ) != 0 )
	{
		close( s );
		return false;
	}

	n = RecvFrom( s, buf, sizeof( buf ), NULL, 0, NULL );
	clock_gettime( CLOCK_MONOTONIC, &t1 );
	sv->ping = (int)( ( t1.tv_sec - t0.tv_sec ) * 1000 + ( t1.tv_nsec - t0.tv_nsec ) / 1000000 );

	if( n <= 0 || !LooksLikeInfo( buf, n ) )
	{
		/* fallback antigo do GoldSrc */
		len = 0;
		req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF;
		memcpy( req + len, "details", 7 );
		len += 7;
		req[len++] = 0;

		SendTo( s, ip, port, req, len );
		n = RecvFrom( s, buf, sizeof( buf ), NULL, 0, NULL );
	}

	if( n <= 0 || !LooksLikeInfo( buf, n ) )
	{
		close( s );
		return false;
	}

	pos = 5; /* pula header + tipo */

	if( buf[4] == 0x49 )
	{
		int protocol = 0;
		std::string ver;

		if( !RdByte( buf, n, &pos, &protocol ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->name ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->map ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->folder ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->game ) ) goto fail;
		if( protocol >= 48 ) { int app = 0; if( !RdShort( buf, n, &pos, &app ) ) goto fail; }
		if( !RdByte( buf, n, &pos, &sv->players ) ) goto fail;
		if( !RdByte( buf, n, &pos, &sv->maxplayers ) ) goto fail;
		if( !RdByte( buf, n, &pos, &sv->bots ) ) goto fail;
		pos += 4; /* servertype, env, visibility, vac */
		if( pos <= n )
			RdStr( buf, n, &pos, &ver );
		sv->version = ver;
	}
	else /* 'm' - resposta antiga do GoldSrc */
	{
		std::string addr, mod;

		if( !RdStr( buf, n, &pos, &addr ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->name ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->map ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->folder ) ) goto fail;
		if( !RdStr( buf, n, &pos, &sv->game ) ) goto fail;
		if( !RdByte( buf, n, &pos, &sv->players ) ) goto fail;
		if( !RdByte( buf, n, &pos, &sv->maxplayers ) ) goto fail;
		if( !RdByte( buf, n, &pos, &i ) ) goto fail; /* protocolo */
		sv->version = "";
		if( RdByte( buf, n, &pos, &sv->secure ) )
		{
			/* servertype/env/visibility + mod + secure + bots */
			if( RdByte( buf, n, &pos, &i ) && RdByte( buf, n, &pos, &i ) &&
				RdByte( buf, n, &pos, &i ) && RdByte( buf, n, &pos, &i ) )
			{
				if( i )
				{
					RdStr( buf, n, &pos, &mod ); /* link */
					RdStr( buf, n, &pos, &mod ); /* download link */
					RdByte( buf, n, &pos, &i );
					RdLong( buf, n, &pos, &appid );
					RdLong( buf, n, &pos, &appid );
					RdByte( buf, n, &pos, &i );
					RdByte( buf, n, &pos, &i );
				}
				RdByte( buf, n, &pos, &sv->secure );
				RdByte( buf, n, &pos, &sv->bots );
			}
		}
	}

	sv->ok = true;
	close( s );
	return true;

fail:
	close( s );
	return false;
}

/* A2S_PLAYER / A2S_RULES: precisam do challenge */
static int QueryChallenge( int s, const char *ip, int port, unsigned char type,
	unsigned char *buf, int buflen, int timeoutMs )
{
	unsigned char req[16];
	int n, len = 0, pos, challenge = 0;
	long ch = 0;

	req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF;
	req[len++] = type;
	req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF;

	if( SendTo( s, ip, port, req, len ) != 0 )
		return -1;

	n = RecvFrom( s, buf, buflen, NULL, 0, NULL );
	if( n < 5 )
		return -1;

	if( buf[4] == 0x41 ) /* 'A' challenge */
	{
		pos = 5;
		if( !RdLong( buf, n, &pos, &ch ) )
			return -1;
		challenge = (int)ch;

		len = 0;
		req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF;
		req[len++] = type;
		req[len++] = (unsigned char)( challenge & 0xFF );
		req[len++] = (unsigned char)( ( challenge >> 8 ) & 0xFF );
		req[len++] = (unsigned char)( ( challenge >> 16 ) & 0xFF );
		req[len++] = (unsigned char)( ( challenge >> 24 ) & 0xFF );

		if( SendTo( s, ip, port, req, len ) != 0 )
			return -1;

		n = RecvFrom( s, buf, buflen, NULL, 0, NULL );
		if( n < 5 )
			return -1;
	}

	return n;
}

static void QueryPlayers( const char *ip, int port, DumpServer *sv, int timeoutMs )
{
	unsigned char buf[8192];
	int s = UdpSocket( timeoutMs );
	int n, pos, count, i;

	if( s < 0 )
		return;

	n = QueryChallenge( s, ip, port, 0x55, buf, sizeof( buf ), timeoutMs );
	close( s );

	if( n <= 5 || buf[4] != 0x44 ) /* 'D' */
		return;

	pos = 5;
	if( !RdByte( buf, n, &pos, &count ) )
		return;
	if( count < 0 || count > 64 )
		return;

	for( i = 0; i < count; i++ )
	{
		std::string name;
		long score = 0;
		float t = 0.f;
		int idx = 0;

		if( !RdByte( buf, n, &pos, &idx ) ) /* indice do jogador */
			break;
		if( !RdStr( buf, n, &pos, &name ) )
			break;
		if( !RdLong( buf, n, &pos, &score ) )
			break;
		if( !RdFloat( buf, n, &pos, &t ) )
			break;

		sv->playerNames.push_back( name );
		sv->playerScores.push_back( (int)score );
	}
}

static void QueryRules( const char *ip, int port, DumpServer *sv, int timeoutMs )
{
	unsigned char buf[16384];
	int s = UdpSocket( timeoutMs );
	int n, pos, count, i;

	if( s < 0 )
		return;

	n = QueryChallenge( s, ip, port, 0x56, buf, sizeof( buf ), timeoutMs );
	close( s );

	if( n <= 5 || buf[4] != 0x45 ) /* 'E' */
		return;

	pos = 5;
	if( !RdShort( buf, n, &pos, &count ) )
		return;
	if( count < 0 || count > 512 )
		return;

	for( i = 0; i < count; i++ )
	{
		std::string k, v;

		if( !RdStr( buf, n, &pos, &k ) )
			break;
		if( !RdStr( buf, n, &pos, &v ) )
			break;
		sv->rules.push_back( std::make_pair( k, v ) );
	}
}

/* ------------------------------------------------------------------ */
/* fontes de servidores                                               */
/* ------------------------------------------------------------------ */

struct Candidate
{
	std::string ip;
	int port;
};

static void AddCandidate( std::vector<Candidate> *list, const std::string &ip, int port )
{
	size_t i;

	if( ip.empty() || port <= 0 || port > 65535 )
		return;
	for( i = 0; i < list->size(); i++ )
	{
		if( ( *list )[i].ip == ip && ( *list )[i].port == port )
			return;
	}
	{
		Candidate c;

		c.ip = ip;
		c.port = port;
		list->push_back( c );
	}
}

static void ParseManual( const std::string &text, std::vector<Candidate> *out )
{
	std::string cur;
	size_t i;

	for( i = 0; i <= text.size(); i++ )
	{
		char c = ( i < text.size() ) ? text[i] : '\n';

		if( c == '\n' || c == ',' || c == ';' || c == ' ' )
		{
			std::string item = Trim( cur );

			cur.clear();
			if( item.empty() )
				continue;

			{
				size_t colon = item.find( ':' );
				int port = 27015;

				if( colon != std::string::npos )
				{
					port = atoi( item.c_str() + colon + 1 );
					item = item.substr( 0, colon );
				}
				AddCandidate( out, Trim( item ), port );
			}
		}
		else
		{
			cur += c;
		}
	}
}

static void CollectLan( std::vector<Candidate> *out, int timeoutMs )
{
	unsigned char req[64];
	unsigned char buf[8192];
	int s, n, len = 0;
	int on = 1;
	int waitMs;
	char ip[64];
	int port;

	s = UdpSocket( 250 );
	if( s < 0 )
		return;

	setsockopt( s, SOL_SOCKET, SO_BROADCAST, &on, sizeof( on ) );

	req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF;
	req[len++] = 0x54;
	memcpy( req + len, "Source Engine Query", 19 );
	len += 19;
	req[len++] = 0;

	SendTo( s, "255.255.255.255", 27015, req, len );
	SendTo( s, "127.0.0.1", 27015, req, len );

	waitMs = timeoutMs * 3;
	if( waitMs < 600 )
		waitMs = 600;

	{
		int elapsed = 0;

		while( elapsed < waitMs )
		{
			n = RecvFrom( s, buf, sizeof( buf ), ip, sizeof( ip ), &port );
			if( n <= 0 )
			{
				elapsed += 250;
				continue;
			}
			if( LooksLikeInfo( buf, n ) )
				AddCandidate( out, ip, port );
		}
	}

	close( s );
}

/* Historico: le os "connect ip:porta" do console_history.txt do jogo. */
static void CollectHistory( std::vector<Candidate> *out )
{
	const char *gamedir = gEngfuncs.pfnGetGameDirectory();
	char path[512];
	FILE *f;
	char line[512];

	if( !gamedir )
		return;

	snprintf( path, sizeof( path ), "%s/console_history.txt", gamedir );
	f = fopen( path, "r" );
	if( !f )
	{
		Log( "historico: %s nao encontrado", path );
		return;
	}

	while( fgets( line, sizeof( line ), f ) )
	{
		char *p = line;
		char ip[128];
		int port = 27015;
		int i = 0;

		while( *p == ' ' || *p == '\t' )
			p++;
		if( strncmp( p, "connect", 7 ) != 0 )
			continue;

		p += 7;
		while( *p == ' ' )
			p++;

		while( *p && *p != ':' && *p != ' ' && *p != '\r' && *p != '\n' && i < (int)sizeof( ip ) - 1 )
			ip[i++] = *p++;
		ip[i] = 0;

		if( *p == ':' )
			port = atoi( p + 1 );

		AddCandidate( out, ip, port );
	}

	fclose( f );
	Log( "historico: %d servidores", (int)out->size() );
}

static void CollectMaster( std::vector<Candidate> *out, int timeoutMs )
{
	unsigned char req[128];
	unsigned char buf[16384];
	int s, n, len = 0, pos, i;
	const char *host = "hl1master.steampowered.com";
	int port = 27010;

	s = UdpSocket( timeoutMs * 2 );
	if( s < 0 )
		return;

	req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF; req[len++] = 0xFF;
	req[len++] = 0x31; /* '1' */
	req[len++] = 0xFF; /* regiao: todas */
	memcpy( req + len, "0.0.0.0:0", 9 );
	len += 9;
	req[len++] = 0;
	req[len++] = 0;

	if( SendTo( s, host, port, req, len ) != 0 )
	{
		Log( "master: falha ao enviar para %s:%d", host, port );
		close( s );
		return;
	}

	n = RecvFrom( s, buf, sizeof( buf ), NULL, 0, NULL );
	close( s );

	if( n < 6 || buf[4] != 0x66 )
	{
		Log( "master: sem resposta valida (%d bytes)", n );
		return;
	}

	/* resposta: header(4) + 'f' + 0x0A + entradas de 6 bytes (ip4 + porta2) */
	for( pos = 6; pos + 6 <= n; pos += 6 )
	{
		char ip[32];
		int p = ( buf[pos + 4] << 8 ) | buf[pos + 5];

		snprintf( ip, sizeof( ip ), "%u.%u.%u.%u",
			buf[pos], buf[pos + 1], buf[pos + 2], buf[pos + 3] );
		AddCandidate( out, ip, p );
		(void)i;
	}

	Log( "master: %d servidores na lista", (int)out->size() );
}

/* ------------------------------------------------------------------ */
/* escrita do dump                                                    */
/* ------------------------------------------------------------------ */

static std::string BuildFileName( const DumpOptions &opt )
{
	std::string path = ExpandHome( Trim( opt.path ) );
	char stamp[64];
	time_t t = time( NULL );
	struct tm tmv;
	const char *ext = "txt";

	if( path.empty() )
		path = "dumps";

	if( opt.format == DUMP_FMT_CSV )
		ext = "csv";
	else if( opt.format == DUMP_FMT_JSON )
		ext = "json";

	localtime_r( &t, &tmv );
	strftime( stamp, sizeof( stamp ), "%Y%m%d_%H%M%S", &tmv );

	/* diretorio: gera nome; arquivo: usa o que o usuario digitou.
	 * "arquivo" = o ultimo componente do caminho tem extensao. */
	{
		size_t slash = path.find_last_of( '/' );
		std::string last = ( slash == std::string::npos ) ? path : path.substr( slash + 1 );
		bool isFile = last.find( '.' ) != std::string::npos;

		if( !isFile )
		{
			if( path[path.size() - 1] != '/' )
				path += "/";
			MakeDirs( path );
			path += "servers_";
			path += stamp;
			path += ".";
			path += ext;
		}
	}

	return path;
}

static bool WriteTxt( FILE *f, const std::vector<DumpServer> &list, int fields )
{
	size_t i;

	fprintf( f, "dump de servidores - %s\n", g_lastFile.c_str() );
	fprintf( f, "total: %d\n\n", (int)list.size() );

	for( i = 0; i < list.size(); i++ )
	{
		const DumpServer &s = list[i];

		fprintf( f, "== %s:%d ==\n", s.addr.c_str(), s.port );
		if( fields & DUMP_F_NAME ) fprintf( f, "nome      : %s\n", s.name.c_str() );
		if( fields & DUMP_F_MAP ) fprintf( f, "mapa      : %s\n", s.map.c_str() );
		if( fields & DUMP_F_PLAYERS ) fprintf( f, "jogadores : %d/%d (bots %d)\n", s.players, s.maxplayers, s.bots );
		if( fields & DUMP_F_PING ) fprintf( f, "ping      : %d ms\n", s.ping );
		if( fields & DUMP_F_VERSION ) fprintf( f, "versao    : %s\n", s.version.c_str() );
		if( fields & DUMP_F_RULES )
		{
			size_t r;

			fprintf( f, "regras    :\n" );
			for( r = 0; r < s.rules.size(); r++ )
				fprintf( f, "  %s = %s\n", s.rules[r].first.c_str(), s.rules[r].second.c_str() );
		}
		if( fields & DUMP_F_PLAYERLIST )
		{
			size_t p;

			fprintf( f, "jogadores online:\n" );
			for( p = 0; p < s.playerNames.size(); p++ )
				fprintf( f, "  %-24s %d\n", s.playerNames[p].c_str(), s.playerScores[p] );
		}
		fprintf( f, "\n" );
	}
	return true;
}

static bool WriteCsv( FILE *f, const std::vector<DumpServer> &list, int fields )
{
	size_t i;

	fprintf( f, "endereco,porta" );
	if( fields & DUMP_F_NAME ) fprintf( f, ",nome" );
	if( fields & DUMP_F_MAP ) fprintf( f, ",mapa" );
	if( fields & DUMP_F_PLAYERS ) fprintf( f, ",jogadores,maxplayers,bots" );
	if( fields & DUMP_F_PING ) fprintf( f, ",ping" );
	if( fields & DUMP_F_VERSION ) fprintf( f, ",versao" );
	if( fields & DUMP_F_RULES ) fprintf( f, ",regras" );
	if( fields & DUMP_F_PLAYERLIST ) fprintf( f, ",jogadores_online" );
	fprintf( f, "\n" );

	for( i = 0; i < list.size(); i++ )
	{
		const DumpServer &s = list[i];
		std::string extra;

		fprintf( f, "%s,%d", s.addr.c_str(), s.port );
		if( fields & DUMP_F_NAME ) fprintf( f, ",%s", CsvEscape( s.name ).c_str() );
		if( fields & DUMP_F_MAP ) fprintf( f, ",%s", CsvEscape( s.map ).c_str() );
		if( fields & DUMP_F_PLAYERS ) fprintf( f, ",%d,%d,%d", s.players, s.maxplayers, s.bots );
		if( fields & DUMP_F_PING ) fprintf( f, ",%d", s.ping );
		if( fields & DUMP_F_VERSION ) fprintf( f, ",%s", CsvEscape( s.version ).c_str() );
		if( fields & DUMP_F_RULES )
		{
			size_t r;

			for( r = 0; r < s.rules.size(); r++ )
			{
				if( r ) extra += "; ";
				extra += s.rules[r].first + "=" + s.rules[r].second;
			}
			fprintf( f, ",%s", CsvEscape( extra ).c_str() );
		}
		if( fields & DUMP_F_PLAYERLIST )
		{
			size_t p;

			extra.clear();
			for( p = 0; p < s.playerNames.size(); p++ )
			{
				if( p ) extra += "; ";
				extra += s.playerNames[p];
			}
			fprintf( f, ",%s", CsvEscape( extra ).c_str() );
		}
		fprintf( f, "\n" );
	}
	return true;
}

static bool WriteJson( FILE *f, const std::vector<DumpServer> &list, int fields )
{
	size_t i;

	fprintf( f, "[\n" );
	for( i = 0; i < list.size(); i++ )
	{
		const DumpServer &s = list[i];

		fprintf( f, "  {\n" );
		fprintf( f, "    \"endereco\": \"%s\",\n", JsonEscape( s.addr ).c_str() );
		fprintf( f, "    \"porta\": %d", s.port );
		if( fields & DUMP_F_NAME ) fprintf( f, ",\n    \"nome\": \"%s\"", JsonEscape( s.name ).c_str() );
		if( fields & DUMP_F_MAP ) fprintf( f, ",\n    \"mapa\": \"%s\"", JsonEscape( s.map ).c_str() );
		if( fields & DUMP_F_PLAYERS )
			fprintf( f, ",\n    \"jogadores\": %d,\n    \"maxplayers\": %d,\n    \"bots\": %d",
				s.players, s.maxplayers, s.bots );
		if( fields & DUMP_F_PING ) fprintf( f, ",\n    \"ping\": %d", s.ping );
		if( fields & DUMP_F_VERSION ) fprintf( f, ",\n    \"versao\": \"%s\"", JsonEscape( s.version ).c_str() );
		if( fields & DUMP_F_RULES )
		{
			size_t r;

			fprintf( f, ",\n    \"regras\": {" );
			for( r = 0; r < s.rules.size(); r++ )
				fprintf( f, "%s\"%s\": \"%s\"", r ? ", " : "",
					JsonEscape( s.rules[r].first ).c_str(), JsonEscape( s.rules[r].second ).c_str() );
			fprintf( f, "}" );
		}
		if( fields & DUMP_F_PLAYERLIST )
		{
			size_t p;

			fprintf( f, ",\n    \"jogadores_online\": [" );
			for( p = 0; p < s.playerNames.size(); p++ )
				fprintf( f, "%s\"%s\"", p ? ", " : "", JsonEscape( s.playerNames[p] ).c_str() );
			fprintf( f, "]" );
		}
		fprintf( f, "\n  }%s\n", ( i + 1 < list.size() ) ? "," : "" );
	}
	fprintf( f, "]\n" );
	return true;
}

static bool SaveDump( const std::vector<DumpServer> &list, const DumpOptions &opt )
{
	std::string path = BuildFileName( opt );
	FILE *f;

	{
		std::lock_guard<std::mutex> lock( g_mutex );
		g_lastFile = path;
	}

	f = fopen( path.c_str(), "w" );
	if( !f )
	{
		Log( "erro: nao consegui abrir %s", path.c_str() );
		return false;
	}

	if( opt.format == DUMP_FMT_CSV )
		WriteCsv( f, list, opt.fields );
	else if( opt.format == DUMP_FMT_JSON )
		WriteJson( f, list, opt.fields );
	else
		WriteTxt( f, list, opt.fields );

	fclose( f );
	Log( "salvo: %s (%d servidores)", path.c_str(), (int)list.size() );
	return true;
}

/* ------------------------------------------------------------------ */
/* thread do dump                                                     */
/* ------------------------------------------------------------------ */

static void DumpWorker( DumpOptions opt )
{
	std::vector<Candidate> cands;
	std::vector<DumpServer> out;
	size_t i;

	g_cancel = false;
	g_progressDone = 0;
	g_progressTotal = 0;
	g_found = 0;
	SetStatus( "Coletando servidores..." );

	if( opt.source == DUMP_SRC_MASTER )
		CollectMaster( &cands, opt.timeoutMs );
	else if( opt.source == DUMP_SRC_LAN )
		CollectLan( &cands, opt.timeoutMs );
	else if( opt.source == DUMP_SRC_HISTORY )
		CollectHistory( &cands );
	else
		ParseManual( opt.manual, &cands );

	if( opt.maxServers > 0 && (int)cands.size() > opt.maxServers )
		cands.resize( opt.maxServers );

	g_progressTotal = (int)cands.size();
	Log( "consultando %d servidor(es)", (int)cands.size() );

	if( cands.empty() )
	{
		SetStatus( "Nenhum servidor encontrado" );
		g_running = false;
		return;
	}

	for( i = 0; i < cands.size(); i++ )
	{
		DumpServer sv;

		if( g_cancel )
			break;

		if( QueryInfo( cands[i].ip.c_str(), cands[i].port, &sv, opt.timeoutMs ) )
		{
			if( opt.fields & DUMP_F_PLAYERLIST )
				QueryPlayers( cands[i].ip.c_str(), cands[i].port, &sv, opt.timeoutMs );
			if( opt.fields & DUMP_F_RULES )
				QueryRules( cands[i].ip.c_str(), cands[i].port, &sv, opt.timeoutMs );

			out.push_back( sv );
			g_found = (int)out.size();
			Log( "ok: %s:%d  %s  (%d/%d)", sv.addr.c_str(), sv.port,
				sv.name.c_str(), sv.players, sv.maxplayers );
		}
		else
		{
			Log( "sem resposta: %s:%d", cands[i].ip.c_str(), cands[i].port );
		}

		g_progressDone = (int)( i + 1 );
	}

	if( g_cancel )
		SetStatus( "Cancelado" );
	else if( SaveDump( out, opt ) )
		SetStatus( "Concluido" );
	else
		SetStatus( "Falha ao salvar" );

	g_running = false;
}

/* ------------------------------------------------------------------ */
/* painel                                                             */
/* ------------------------------------------------------------------ */

static int FieldFlags( void )
{
	return cv_fields ? (int)cv_fields->value : 0;
}

static void SetFieldFlag( int bit, bool on )
{
	int fields = FieldFlags();

	if( on )
		fields |= bit;
	else
		fields &= ~bit;

	gEngfuncs.Cvar_SetValue( "cl_dump_fields", (float)fields );
}

static void StartDump( void )
{
	DumpOptions opt;

	if( g_running )
		return;

	opt.path = ( cv_path && cv_path->string && cv_path->string[0] ) ? cv_path->string : g_pathBuf;
	opt.format = cv_format ? (int)cv_format->value : 0;
	opt.fields = FieldFlags();
	opt.timeoutMs = cv_timeout ? (int)cv_timeout->value : 400;
	opt.maxServers = cv_max ? (int)cv_max->value : 200;
	opt.source = cv_source ? (int)cv_source->value : 1;
	opt.manual = ( cv_manual && cv_manual->string && cv_manual->string[0] ) ? cv_manual->string : g_manualBuf;

	if( opt.timeoutMs < 100 ) opt.timeoutMs = 100;
	if( opt.maxServers < 1 ) opt.maxServers = 200;

	{
		std::lock_guard<std::mutex> lock( g_mutex );
		g_log.clear();
		g_lastFile.clear();
	}

	g_running = true;
	g_thread = std::thread( DumpWorker, opt );
}

/* conteudo do dump, desenhado dentro de uma tab do HUB (sem janela propria) */
void ServerDump_DrawContent( void )
{
	int format, source, timeoutMs, maxServers;
	int fields = FieldFlags();
	std::string status, lastFile, log;
	int done, total, found;

	ImGui::TextColored( ImVec4( 0.4f, 0.9f, 1.0f, 1.0f ), "Dump de servidores" );
	ImGui::Separator();

	source = cv_source ? (int)cv_source->value : 1;
	if( ImGui::Combo( "Fonte", &source, "Master server (internet)\0Varredura LAN\0Lista manual\0Historico (console_history)\0" ) )
		gEngfuncs.Cvar_SetValue( "cl_dump_source", (float)source );

	if( source == DUMP_SRC_MANUAL )
	{
		if( ImGui::InputTextMultiline( "##manual", g_manualBuf, sizeof( g_manualBuf ),
			ImVec2( -1.0f, 80.0f ) ) )
			gEngfuncs.Cvar_Set( "cl_dump_manual", g_manualBuf );
		ImGui::TextUnformatted( "Um por linha: ip:porta (porta padrao 27015)" );
	}

	ImGui::Separator();
	ImGui::TextUnformatted( "O que dumpar" );
	{
		bool bAddr = ( fields & DUMP_F_ADDRESS ) != 0;
		bool bName = ( fields & DUMP_F_NAME ) != 0;
		bool bMap = ( fields & DUMP_F_MAP ) != 0;
		bool bPl = ( fields & DUMP_F_PLAYERS ) != 0;
		bool bPing = ( fields & DUMP_F_PING ) != 0;
		bool bRules = ( fields & DUMP_F_RULES ) != 0;
		bool bList = ( fields & DUMP_F_PLAYERLIST ) != 0;
		bool bVer = ( fields & DUMP_F_VERSION ) != 0;

		if( ImGui::Checkbox( "Endereco", &bAddr ) ) SetFieldFlag( DUMP_F_ADDRESS, bAddr );
		ImGui::SameLine();
		if( ImGui::Checkbox( "Nome", &bName ) ) SetFieldFlag( DUMP_F_NAME, bName );
		ImGui::SameLine();
		if( ImGui::Checkbox( "Mapa", &bMap ) ) SetFieldFlag( DUMP_F_MAP, bMap );

		if( ImGui::Checkbox( "Jogadores", &bPl ) ) SetFieldFlag( DUMP_F_PLAYERS, bPl );
		ImGui::SameLine();
		if( ImGui::Checkbox( "Ping", &bPing ) ) SetFieldFlag( DUMP_F_PING, bPing );
		ImGui::SameLine();
		if( ImGui::Checkbox( "Versao", &bVer ) ) SetFieldFlag( DUMP_F_VERSION, bVer );

		if( ImGui::Checkbox( "Regras (rules)", &bRules ) ) SetFieldFlag( DUMP_F_RULES, bRules );
		ImGui::SameLine();
		if( ImGui::Checkbox( "Lista de jogadores", &bList ) ) SetFieldFlag( DUMP_F_PLAYERLIST, bList );

		if( ImGui::Button( "Marcar tudo" ) )
		{
			gEngfuncs.Cvar_SetValue( "cl_dump_fields",
				(float)( DUMP_F_ADDRESS | DUMP_F_NAME | DUMP_F_MAP | DUMP_F_PLAYERS |
					DUMP_F_PING | DUMP_F_RULES | DUMP_F_PLAYERLIST | DUMP_F_VERSION ) );
		}
		ImGui::SameLine();
		if( ImGui::Button( "Limpar" ) )
			gEngfuncs.Cvar_SetValue( "cl_dump_fields", (float)DUMP_F_ADDRESS );
	}

	ImGui::Separator();
	format = cv_format ? (int)cv_format->value : 0;
	if( ImGui::Combo( "Formato", &format, "TXT\0CSV\0JSON\0" ) )
		gEngfuncs.Cvar_SetValue( "cl_dump_format", (float)format );

	if( ImGui::InputText( "Destino", g_pathBuf, sizeof( g_pathBuf ) ) )
		gEngfuncs.Cvar_Set( "cl_dump_path", g_pathBuf );
	ImGui::TextUnformatted( "Diretorio (gera servers_data_hora) ou arquivo completo." );

	timeoutMs = cv_timeout ? (int)cv_timeout->value : 400;
	if( ImGui::SliderInt( "Timeout (ms)", &timeoutMs, 100, 2000 ) )
		gEngfuncs.Cvar_SetValue( "cl_dump_timeout", (float)timeoutMs );

	maxServers = cv_max ? (int)cv_max->value : 200;
	if( ImGui::SliderInt( "Maximo de servidores", &maxServers, 1, 1000 ) )
		gEngfuncs.Cvar_SetValue( "cl_dump_max", (float)maxServers );

	ImGui::Separator();
	if( g_running )
	{
		if( ImGui::Button( "Cancelar" ) )
			g_cancel = true;
	}
	else
	{
		if( ImGui::Button( "Iniciar dump" ) )
			StartDump();
	}

	ImGui::SameLine();
	if( ImGui::Button( "Abrir pasta do jogo" ) )
		gEngfuncs.pfnClientCmd( "toggleconsole\n" );

	{
		std::lock_guard<std::mutex> lock( g_mutex );
		status = g_status;
		lastFile = g_lastFile;
		log = g_log;
		done = g_progressDone;
		total = g_progressTotal;
		found = g_found;
	}

	ImGui::Text( "Status: %s", status.c_str() );
	ImGui::Text( "Progresso: %d/%d   encontrados: %d", done, total, found );
	if( total > 0 )
		ImGui::ProgressBar( (float)done / (float)total, ImVec2( -1.0f, 0.0f ) );
	if( !lastFile.empty() )
		ImGui::TextColored( ImVec4( 0.5f, 1.0f, 0.6f, 1.0f ), "Arquivo: %s", lastFile.c_str() );

	ImGui::Separator();
	ImGui::TextUnformatted( "Log" );
	ImGui::BeginChild( "##dumplog", ImVec2( -1.0f, 140.0f ), true );
	ImGui::TextUnformatted( log.c_str() );
	ImGui::EndChild();
}

static void Cmd_DumpServer( void )
{
	/* o dump vive na tab Misc > Servers do HUB */
	Hub_ShowTab( 3, 1 );
	Hub_Open( 1 );
}

static void Cmd_DumpRun( void )
{
	StartDump();
}

static void Cmd_DumpCancel( void )
{
	g_cancel = true;
}

/* ------------------------------------------------------------------ */
/* init                                                               */
/* ------------------------------------------------------------------ */

void ServerDump_Shutdown( void )
{
	g_shutdown = true;
	g_cancel = true;

	if( g_thread.joinable() )
		g_thread.join();
}

void ServerDump_Tick( void )
{
	static double first;
	static double lastLog;
	static int openChecked;

	/* cl_dump_open e aplicado depois do init (exec da linha de comando) */
	if( !openChecked )
	{
		cvar_t *o = (cvar_t *)gEngfuncs.pfnGetCvarPointer( "cl_dump_open" );

		if( o )
		{
			openChecked = 1;
			if( o->value > 0.5f )
				g_open = true;
		}
	}

	if( gEngfuncs.GetClientTime() - lastLog > 5.0 )
	{
		lastLog = gEngfuncs.GetClientTime();
		gEngfuncs.Con_Printf( "dump tick: auto=%.0f running=%d done=%d t=%.1f\n",
			cv_auto ? cv_auto->value : -1.f, g_running ? 1 : 0, g_autoDone ? 1 : 0,
			gEngfuncs.GetClientTime() );
	}

	if( !cv_auto || cv_auto->value <= 0.f || g_autoDone )
		return;

	if( first <= 0.0 )
	{
		first = gEngfuncs.GetClientTime();
		return;
	}

	if( !g_running && gEngfuncs.GetClientTime() - first >= cv_auto->value )
	{
		g_autoDone = true;
		StartDump();
	}
}

void ServerDump_Init( void )
{
	cv_path    = gEngfuncs.pfnRegisterVariable( "cl_dump_path", "dumps", FCVAR_ARCHIVE );
	cv_format  = gEngfuncs.pfnRegisterVariable( "cl_dump_format", "0", FCVAR_ARCHIVE );
	cv_fields  = gEngfuncs.pfnRegisterVariable( "cl_dump_fields", "31", FCVAR_ARCHIVE );
	cv_source  = gEngfuncs.pfnRegisterVariable( "cl_dump_source", "1", FCVAR_ARCHIVE );
	cv_timeout = gEngfuncs.pfnRegisterVariable( "cl_dump_timeout", "400", FCVAR_ARCHIVE );
	cv_max     = gEngfuncs.pfnRegisterVariable( "cl_dump_max", "200", FCVAR_ARCHIVE );
	cv_manual  = gEngfuncs.pfnRegisterVariable( "cl_dump_manual", "", FCVAR_ARCHIVE );
	cv_auto    = gEngfuncs.pfnRegisterVariable( "cl_dump_auto", "0", FCVAR_ARCHIVE );

	snprintf( g_pathBuf, sizeof( g_pathBuf ), "%s", cv_path ? cv_path->string : "dumps" );
	snprintf( g_manualBuf, sizeof( g_manualBuf ), "%s", cv_manual ? cv_manual->string : "" );

	gEngfuncs.Con_Printf( "dump init: path=%s source=%d auto=%d fields=%d open=%d\n",
		cv_path ? cv_path->string : "?",
		cv_source ? (int)cv_source->value : -1,
		cv_auto ? (int)cv_auto->value : -1,
		cv_fields ? (int)cv_fields->value : -1,
		g_open ? 1 : 0 );

	gEngfuncs.pfnAddCommand( "dump_servidor", Cmd_DumpServer );
	gEngfuncs.pfnAddCommand( "dump_servidor_run", Cmd_DumpRun );
	gEngfuncs.pfnAddCommand( "dump_servidor_cancel", Cmd_DumpCancel );

	{
		cvar_t *open = gEngfuncs.pfnRegisterVariable( "cl_dump_open", "0", FCVAR_ARCHIVE );

		if( open && open->value > 0.5f )
			g_open = true;
	}
	/* sem painel proprio: o HUB desenha o dump na tab Servers */
}

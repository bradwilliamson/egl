/*
Copyright (C) 2025 EGL Team
Based on Q2Pro by Andrey Nazarov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#include "cl_local.h"
#include "cl_serverbrowser.h"
#include "cl_httpfetch.h"

static serverSlot_t *g_servers[MAX_STATUS_SERVERS];
static int g_numServers = 0;

static cVar_t *sb_sortservers;
static cVar_t *sb_pingrate;
static cVar_t *sb_usemasters;
static cVar_t *sb_uselan;
static cVar_t *sb_debug;

#define SB_MAX_MASTERS 4
static cVar_t *sb_master[SB_MAX_MASTERS];

static unsigned g_pingTimestamp;
static int g_pingStage;
static int g_pingIndex;
static int g_pingExtra;
static int g_pingTime;

// Exported for menu access
serverSlot_t **g_serverSlots = g_servers;

// Forward declarations
static void SB_FreeSlot(serverSlot_t *slot);
static serverSlot_t *SB_FindSlot(const netAdr_t *search, int *index_p);
static uint32 SB_ColorForStatus(const char *infostring, unsigned ping);
static void SB_AddServer(const netAdr_t *address, const char *hostname);
static void SB_ClearServers(void);
static void SB_FinishPingStage(void);
static void SB_CalcPingRate(void);
static void SB_BeginPingIfNeeded(void);
static void SB_QueryMasters(void);
static void SB_RefreshMasterDefaults(void);

static qBool SB_IsHTTPMaster(const char *s)
{
	if (!s || !s[0])
		return qFalse;
	return (!Q_strnicmp(s, "http://", 7) || !Q_strnicmp(s, "https://", 8)) ? qTrue : qFalse;
}

static qBool SB_LooksLikeTextServerList(const byte *data, size_t len)
{
	size_t i;
	int dots = 0;
	int colons = 0;
	int digits = 0;
	int newlines = 0;
	int nonPrintable = 0;

	if (!data || !len)
		return qFalse;

	// Text server lists look like lines of "ip:port" or "quake2://ip:port".
	// Binary lists are raw 6-byte entries (4-byte IP + 2-byte port).
	// Key distinction: text has dots, colons, and mostly ASCII digits.
	size_t checkLen = (len > 256) ? 256 : len;
	for (i = 0; i < checkLen; i++) {
		byte c = data[i];
		if (c == '.')
			dots++;
		else if (c == ':')
			colons++;
		else if (c >= '0' && c <= '9')
			digits++;
		else if (c == '\n' || c == '\r')
			newlines++;
		else if (c < 32 || c > 126)
			nonPrintable++;
	}

	// Text format: many dots/colons/digits, some newlines, few non-printable.
	// Binary format: random bytes, few dots/colons/digits relative to size.
	// For text: expect at least 3 dots and 1 colon per ~20 chars for "1.2.3.4:27910\n"
	if (dots >= 3 && colons >= 1 && digits > (int)checkLen / 4 && nonPrintable < (int)checkLen / 10)
		return qTrue;

	return qFalse;
}

static void SB_AddServersFromText(const char *text)
{
	const char *p = text;
	char line[1024];

	if (!p)
		return;

	while (*p) {
		const char *eol = strchr(p, '\n');
		size_t n = eol ? (size_t)(eol - p) : strlen(p);
		while (n > 0 && (p[n - 1] == '\r' || p[n - 1] == ' ' || p[n - 1] == '\t'))
			n--;
		if (n >= sizeof(line))
			n = sizeof(line) - 1;
		memcpy(line, p, n);
		line[n] = 0;

		// Accept formats like:
		//  - ip:port
		//  - quake2://ip:port
		if (!Q_strnicmp(line, "quake2://", 9)) {
			SB_AddServer(NULL, line + 9);
		} else if (line[0]) {
			SB_AddServer(NULL, line);
		}

		if (!eol)
			break;
		p = eol + 1;
	}
}

static void SB_AddServersFromBinaryMasterReply(const char *sourceLabel, const byte *data, size_t len)
{
	const byte *payload = data;
	size_t payloadLen = len;
	netAdr_t from;

	memset(&from, 0, sizeof(from));
	from.naType = NA_LOOPBACK;
	from.port = BigShort(PORT_MASTER);

	// Some sources embed the usual connectionless header:
	//   0xFFFFFFFF + "servers..." + '\n' + <binary entries>
	if (payloadLen >= 4 && payload[0] == 0xFF && payload[1] == 0xFF && payload[2] == 0xFF && payload[3] == 0xFF) {
		size_t off = 4;
		while (off < payloadLen && payload[off] != '\n')
			off++;
		if (off < payloadLen && payload[off] == '\n')
			off++;
		payload += off;
		payloadLen -= off;
	}

	if (sb_debug && sb_debug->intVal >= 2)
		Com_Printf(0, "HTTP master '%s': %u byte(s) after header stripping\n", sourceLabel ? sourceLabel : "(unknown)", (unsigned)payloadLen);

	SrvBrowser_MasterServersResponse(&from, payload, payloadLen);
}

static void SB_QueryHTTPMaster(const char *url)
{
	byte *data = NULL;
	size_t len = 0;
	char err[256];

	if (!url || !url[0])
		return;

	Com_Printf(0, "Fetching server list via HTTP: %s\n", url);

	if (!CL_HTTPFetch(url, &data, &len, err, sizeof(err))) {
		Com_Printf(0, "HTTP fetch failed: %s (%s)\n", url, err[0] ? err : "unknown error");
		return;
	}

	Com_Printf(0, "HTTP fetch returned %u bytes\n", (unsigned)len);

	if (SB_LooksLikeTextServerList(data, len)) {
		Com_Printf(0, "Detected TEXT format server list\n");
		SB_AddServersFromText((const char *)data);
		if (g_numServers)
			SB_BeginPingIfNeeded();
	} else {
		Com_Printf(0, "Detected BINARY format server list (%u potential entries)\n", (unsigned)(len / 6));
		SB_AddServersFromBinaryMasterReply(url, data, len);
	}

	Mem_Free(data);
}

/*
=================
SB_FreeSlot
=================
*/
static void SB_FreeSlot(serverSlot_t *slot)
{
	if (slot->hostname)
		Mem_Free(slot->hostname);
	Mem_Free(slot);
}

/*
=================
SB_FindSlot
=================
*/
static serverSlot_t *SB_FindSlot(const netAdr_t *search, int *index_p)
{
	serverSlot_t *slot;
	int i;

	for (i = 0; i < g_numServers; i++) {
		slot = g_servers[i];
		// Explicit IP comparison (not using macro which may have issues)
		if (search->naType != slot->address.naType)
			continue;
		if (search->ip[0] != slot->address.ip[0] ||
		    search->ip[1] != slot->address.ip[1] ||
		    search->ip[2] != slot->address.ip[2] ||
		    search->ip[3] != slot->address.ip[3])
			continue;
		// Same IP found - now check port
		if (search->port != slot->address.port)
			continue;
		if (index_p)
			*index_p = i;
		return slot;
	}

	if (index_p)
		*index_p = -1;
	return NULL;
}

/*
=================
SB_ColorForStatus
=================
*/
static uint32 SB_ColorForStatus(const char *infostring, unsigned ping)
{
	if (atoi(Info_ValueForKey((char *)infostring, "needpass")) >= 1)
		return 0xFF0000FF; // Red for password

	if (ping < 30)
		return 0x00FF00FF; // Green for good ping

	return 0xFFFFFFFF; // White
}

/*
=================
SB_AddServer
=================
*/
static void SB_AddServer(const netAdr_t *address, const char *hostname)
{
	netAdr_t tmp;
	serverSlot_t *slot;
	static int duplicates = 0;
	static int badPorts = 0;

	if (g_numServers >= MAX_STATUS_SERVERS)
		return;

	if (!address) {
		if (!hostname)
			return;
		if (!NET_StringToAdr((char *)hostname, &tmp)) {
			Com_Printf(0, "Bad server address: %s\n", hostname);
			return;
		}
		address = &tmp;
	}

	// Ignore if already listed
	{
		serverSlot_t *existing = SB_FindSlot(address, NULL);
		if (existing) {
			duplicates++;
			if (sb_debug && sb_debug->intVal >= 1)
				Com_Printf(0, "Duplicate server: %s matches existing %d.%d.%d.%d:%d (total dupes: %d)\n", 
					NET_AdrToString((netAdr_t *)address),
					existing->address.ip[0], existing->address.ip[1], 
					existing->address.ip[2], existing->address.ip[3],
					BigShort(existing->address.port),
					duplicates);
			return;
		}
	}

	if (!hostname)
		hostname = NET_AdrToString((netAdr_t *)address);

	// Privileged ports not allowed
	if (BigShort(address->port) < 1024) {
		badPorts++;
		if (sb_debug && sb_debug->intVal >= 2)
			Com_Printf(0, "Bad server port: %s (total bad: %d)\n", hostname, badPorts);
		return;
	}

	slot = Mem_Alloc(sizeof(*slot));
	memset(slot, 0, sizeof(*slot));
	slot->status = SLOT_IDLE;
	slot->address = *address;
	slot->hostname = Mem_PoolStrDup(hostname, com_genericPool, 0);
	slot->color = 0xFFFFFFFF;
	slot->timestamp = Sys_Milliseconds();
	Q_snprintfz(slot->name, sizeof(slot->name), "%s\t???\t???\t?/?\t???", hostname);

	g_servers[g_numServers++] = slot;
}

/*
=================
SB_ClearServers
=================
*/
static void SB_ClearServers(void)
{
	int i;

	for (i = 0; i < g_numServers; i++) {
		SB_FreeSlot(g_servers[i]);
		g_servers[i] = NULL;
	}

	g_numServers = 0;
}

/*
=================
SB_FinishPingStage
=================
*/
static void SB_FinishPingStage(void)
{
	g_pingStage = 0;
	g_pingIndex = 0;
	g_pingExtra = 0;
}

/*
=================
SB_CalcPingRate
=================
*/
static void SB_CalcPingRate(void)
{
	int rate = Cvar_GetIntegerValue("sb_pingrate");
	if (!rate)
		rate = 10; // Default 10 packets/sec

	g_pingTime = (1000 * PING_STAGES) / (rate * g_pingStage);
}

static void SB_BeginPingIfNeeded(void)
{
	if (g_pingStage)
		return;
	if (!g_numServers)
		return;

	g_pingStage = PING_STAGES;
	g_pingIndex = 0;
	g_pingExtra = 0;
	SB_CalcPingRate();

	Com_Printf(0, "Pinging %d server(s)...\n", g_numServers);
}

static void SB_QueryMasters(void)
{
	int i;
	int queried = 0;

	// Ensure the client socket exists even when not connected.
	NET_Config(NET_CLIENT);

	SB_RefreshMasterDefaults();

	if (!sb_usemasters || !sb_usemasters->intVal)
		return;

	for (i = 0; i < SB_MAX_MASTERS; i++) {
		netAdr_t master;
		const char *s;

		if (!sb_master[i])
			continue;
		s = sb_master[i]->string;
		if (!s || !s[0])
			continue;

		if (SB_IsHTTPMaster(s)) {
			queried++;
			SB_QueryHTTPMaster(s);
			continue;
		}

		memset(&master, 0, sizeof(master));
		if (!NET_StringToAdr((char *)s, &master)) {
			Com_Printf(0, "Bad master address: %s\n", s);
			continue;
		}
		if (!master.port)
			master.port = BigShort(PORT_MASTER);

		queried++;
		Com_Printf(0, "Querying master %s...\n", NET_AdrToString(&master));
		if (sb_debug && sb_debug->intVal >= 2)
			Com_Printf(0, "Resolved master[%d]=%s -> %s\n", i + 1, s, NET_AdrToString(&master));
		// Try both common Quake2 master query formats.
		Netchan_OutOfBandPrint(NS_CLIENT, &master, "query\n");
		Netchan_OutOfBandPrint(NS_CLIENT, &master, Q_VarArgs("query %i\n", ORIGINAL_PROTOCOL_VERSION));
	}

	if (!queried)
		Com_Printf(0, "No master servers configured (sb_master1..%d).\n", SB_MAX_MASTERS);
}

static void SB_RefreshMasterDefaults(void)
{
	// Make sure we have useful defaults even if:
	// - config execution happens after SrvBrowser_Init
	// - sb_master* are archived with stale values
	const char *a0 = Cvar_GetStringValue("adr0");
	const char *a1 = Cvar_GetStringValue("adr1");
	const char *m1 = (sb_master[0] ? sb_master[0]->string : "");
	const char *m2 = (sb_master[1] ? sb_master[1]->string : "");

	if ((!m1[0] || !strcmp(m1, "192.246.40.37:27900"))) {
		if (a0 && a0[0])
			Cvar_Set("sb_master1", (char *)a0, qTrue);
		else
			Cvar_Set("sb_master1", "http://q2servers.com/?raw=2", qTrue);
	}

	if ((!m2[0] || !strcmp(m2, "192.246.40.37:27900"))) {
		if (a1 && a1[0])
			Cvar_Set("sb_master2", (char *)a1, qTrue);
		else
			Cvar_Set("sb_master2", "master.q2servers.com", qTrue);
	}
}

void SrvBrowser_MasterServersResponse(const netAdr_t *from, const byte *payload, size_t payloadLen)
{
	size_t i = 0;
	int parsed = 0;
	int skippedPort = 0;
	int skippedLoopback = 0;
	int beforeCount;
	qBool hasBackslashDelim = qFalse;

	if (sb_debug && sb_debug->intVal >= 1)
		Com_Printf(0, "Master reply from %s: %u bytes\n", NET_AdrToString((netAdr_t *)from), (unsigned)payloadLen);

	if (!payload || payloadLen < 6)
		return;

	beforeCount = g_numServers;

	// Detect format:
	// - Traditional Quake2 master replies use a single '\\' delimiter before each 6-byte entry.
	// - Some HTTP masters provide raw continuous 6-byte entries with no delimiters.
	// Do NOT rely on payload[0] alone: a raw list may legitimately start with 0x5C (92.*.*.*).
	if (payload[0] == '\\') {
		int hits = 0;
		int checks = 0;
		size_t off;
		for (off = 0; off < payloadLen && checks < 4; off += 7, checks++) {
			// Expected pattern: '\\' + 6 bytes per entry => delimiters at offsets 0,7,14,...
			if (off < payloadLen && payload[off] == '\\')
				hits++;
			else
				break;
			if (off + 7 > payloadLen)
				break;
		}
		// Require at least two consecutive delimiters to avoid false positives.
		if (hits >= 2)
			hasBackslashDelim = qTrue;
	}

	if (sb_debug && sb_debug->intVal >= 2)
		Com_Printf(0, "Master reply parse mode: %s\n", hasBackslashDelim ? "delimited" : "raw6");

	while (i < payloadLen) {
		netAdr_t adr;
		uint16 port;

		// Skip backslash delimiter if present (traditional format)
		if (hasBackslashDelim && i < payloadLen && payload[i] == '\\') {
			i++;
			// Also skip any trailing whitespace/newlines after backslash
			while (i < payloadLen && (payload[i] == '\n' || payload[i] == '\r' || payload[i] == ' '))
				i++;
		}

		if (i + 6 > payloadLen)
			break;

		memset(&adr, 0, sizeof(adr));
		adr.naType = NA_IP;
		memcpy(adr.ip, payload + i, 4);
		memcpy(&port, payload + i + 4, 2);
		adr.port = port; // Already in network byte order
		i += 6;
		parsed++;

		// End marker: 0.0.0.0:0
		if (adr.ip[0] == 0 && adr.ip[1] == 0 && adr.ip[2] == 0 && adr.ip[3] == 0 && adr.port == 0)
			break;

		// Validate: skip obviously invalid entries (privileged ports, loopback, etc.)
		{
			uint16 hostPort = BigShort(adr.port);
			if (hostPort < 1024 || hostPort > 65535) {
				skippedPort++;
				continue;
			}
			// Skip loopback (127.x.x.x)
			if (adr.ip[0] == 127) {
				skippedLoopback++;
				continue;
			}
		}

		SB_AddServer(&adr, NULL);
	}

	{
		int added = g_numServers - beforeCount;
		int duplicates = parsed - added - skippedPort - skippedLoopback;
		Com_Printf(0, "Master reply: parsed %d entries, added %d new (skipped: %d port, %d loopback, %d dupes), total %d.\n", 
			parsed, added, skippedPort, skippedLoopback, duplicates, g_numServers);
	}

	SB_BeginPingIfNeeded();
}

/*
=================
SrvBrowser_Init
=================
*/
void SrvBrowser_Init(void)
{
	sb_sortservers = Cvar_Register("sb_sortservers", "0", 0);
	sb_pingrate = Cvar_Register("sb_pingrate", "10", 0);
	sb_usemasters = Cvar_Register("sb_usemasters", "1", 0);
	sb_uselan = Cvar_Register("sb_uselan", "0", 0);
	sb_debug = Cvar_Register("sb_debug", "0", 0);

	// EGL historically uses adr0/adr1 for master hosts (see default configs).
	sb_master[0] = Cvar_Register("sb_master1", Cvar_GetStringValue("adr0"), 0);
	sb_master[1] = Cvar_Register("sb_master2", Cvar_GetStringValue("adr1"), 0);
	sb_master[2] = Cvar_Register("sb_master3", "", 0);
	sb_master[3] = Cvar_Register("sb_master4", "", 0);

	// If the user still has the old hardcoded id master (often dead in 2026),
	// or the sb_master slots are empty, prefer adr0/adr1 if provided.
	{
		const char *a0 = Cvar_GetStringValue("adr0");
		const char *a1 = Cvar_GetStringValue("adr1");
		const char *m1 = sb_master[0] ? sb_master[0]->string : "";
		const char *m2 = sb_master[1] ? sb_master[1]->string : "";

		if (a0 && a0[0] && (!m1[0] || !strcmp(m1, "192.246.40.37:27900")))
			Cvar_Set("sb_master1", (char *)a0, qTrue);
		if (a1 && a1[0] && (!m2[0] || !strcmp(m2, "192.246.40.37:27900")))
			Cvar_Set("sb_master2", (char *)a1, qTrue);
	}

	Cmd_AddCommand("sb_refresh", SrvBrowser_Refresh_f, "Refresh server list");
	Cmd_AddCommand("sb_list", SrvBrowser_List_f, "List servers in console");
}

/*
=================
SrvBrowser_Shutdown
=================
*/
void SrvBrowser_Shutdown(void)
{
	SB_ClearServers();
	Cmd_RemoveCommand("sb_refresh", "Refresh server list");
	Cmd_RemoveCommand("sb_list", "List servers in console");
}

/*
=================
SrvBrowser_Frame
=================
*/
void SrvBrowser_Frame(void)
{
	serverSlot_t *slot;
	int budget;

	if (!g_pingStage)
		return;

	g_pingExtra += cls.refreshFrameTime * 1000;

	// Allow multiple sends per frame so sb_pingrate isn't capped by FPS.
	// Keep a small cap to avoid flooding on stalls.
	budget = 32;
	while (budget-- > 0 && g_pingStage && g_pingExtra >= g_pingTime) {
		g_pingExtra -= g_pingTime;

		// Send out next status packet
		while (g_pingIndex < g_numServers) {
			slot = g_servers[g_pingIndex++];
			if (!slot)
				continue;
			if (slot->status > SLOT_PENDING)
				continue;
			slot->status = SLOT_PENDING;
			slot->timestamp = Sys_Milliseconds();
			Netchan_OutOfBandPrint(NS_CLIENT, &slot->address, "status");
			break;
		}

		if (g_pingIndex >= g_numServers) {
			g_pingIndex = 0;
			if (--g_pingStage == 0)
				SB_FinishPingStage();
			else
				SB_CalcPingRate();
		}
	}
}

/*
=================
SrvBrowser_StatusResponse

A server status response has been received, validated and parsed.
=================
*/
void SrvBrowser_StatusResponse(netAdr_t *from, const char *response)
{
	serverSlot_t *slot;
	char *hostname;
	const char *host, *mod, *map, *maxclients;
	unsigned timestamp, ping;
	const char *info = response;
	const char *s;
	int i;

	// See if already added
	slot = SB_FindSlot(from, &i);
	if (!slot) {
		// Reply to broadcast, create new slot
		if (g_numServers >= MAX_STATUS_SERVERS)
			return;
		i = g_numServers;
		hostname = Mem_PoolStrDup(NET_AdrToString(from), com_genericPool, 0);
		timestamp = g_pingTimestamp;
	} else {
		// Free previous data
		hostname = slot->hostname;
		timestamp = slot->timestamp;
		// Don't free slot itself, we'll reuse it
	}

	// Parse infostring
	host = Info_ValueForKey((char *)info, "hostname");
	if (!host[0])
		host = hostname;

	mod = Info_ValueForKey((char *)info, "game");
	if (!mod[0])
		mod = "baseq2";

	map = Info_ValueForKey((char *)info, "mapname");
	if (!map[0])
		map = "???";

	maxclients = Info_ValueForKey((char *)info, "maxclients");
	if (!maxclients[0])
		maxclients = "?";

	if (timestamp > Sys_Milliseconds())
		timestamp = Sys_Milliseconds();

	ping = Sys_Milliseconds() - timestamp;
	if (ping > 999)
		ping = 999;

	// Allocate or reuse slot
	if (!slot) {
		slot = Mem_Alloc(sizeof(*slot));
		memset(slot, 0, sizeof(*slot));
		slot->address = *from;
		slot->hostname = hostname;
		g_servers[i] = slot;
		g_numServers++;
	}

	slot->status = SLOT_VALID;
	slot->color = SB_ColorForStatus(info, ping);
	slot->timestamp = timestamp;
	Q_strncpyz(slot->infostring, info, sizeof(slot->infostring));

	// Parse players - skip past infostring to find player lines
	s = info;
	while (*s && *s != '\n')
		s++;
	if (*s == '\n')
		s++;

	slot->numPlayers = 0;
	while (*s && slot->numPlayers < MAX_STATUS_PLAYERS) {
		int score, playerping;
		const char *name;
		char *end;

		// Parse "score ping name"
		score = strtol(s, &end, 10);
		if (end == s)
			break;
		s = end;

		while (*s == ' ' || *s == '\t')
			s++;

		playerping = strtol(s, &end, 10);
		if (end == s)
			break;
		s = end;

		while (*s == ' ' || *s == '\t')
			s++;

		if (*s == '\"') {
			s++;
			name = s;
			while (*s && *s != '\"')
				s++;
			if (*s == '\"') {
				int len = s - name;
				if (len > 31)
					len = 31;
				Q_strncpyz(slot->players[slot->numPlayers].name, name, len + 1);
				s++;
			}
		} else {
			name = s;
			while (*s && *s != '\n' && *s != '\r')
				s++;
			int len = s - name;
			if (len > 31)
				len = 31;
			Q_strncpyz(slot->players[slot->numPlayers].name, name, len + 1);
		}

		slot->players[slot->numPlayers].score = score;
		slot->players[slot->numPlayers].ping = playerping;
		slot->numPlayers++;

		// Skip to next line
		while (*s && (*s == '\n' || *s == '\r'))
			s++;
	}

	// Format display name
	Q_snprintfz(slot->name, sizeof(slot->name), "%s\t%s\t%s\t%d/%s\t%u",
		host, mod, map, slot->numPlayers, maxclients, ping);

	if (sb_debug && sb_debug->intVal >= 1)
		Com_Printf(0, "Server response from %s: %s (ping %u)\n", NET_AdrToString(from), host, ping);
	
	// Notify cgame about the server - prepend calculated ping to info
	{
		char modifiedInfo[MAX_INFO_STRING + 64];
		Q_snprintfz(modifiedInfo, sizeof(modifiedInfo), "\\__srvping\\%u%s", ping, info);
		CL_CGModule_ParseServerStatus(hostname, modifiedInfo);
	}
}

/*
=================
SrvBrowser_Refresh_f
=================
*/
void SrvBrowser_Refresh_f(void)
{
	netAdr_t broadcast;

	Com_Printf(0, "Refreshing server list...\n");

	// Ensure the client socket exists even when not connected.
	NET_Config(NET_CLIENT);

	SB_FinishPingStage();
	SB_ClearServers();

	g_pingTimestamp = Sys_Milliseconds();

	// Optional LAN broadcast discovery
	if (sb_uselan && sb_uselan->intVal) {
		memset(&broadcast, 0, sizeof(broadcast));
		broadcast.naType = NA_BROADCAST;
		broadcast.port = BigShort(PORT_SERVER);
		SB_AddServer(&broadcast, NULL);
		SB_BeginPingIfNeeded();
	}

	// Master server discovery (online)
	SB_QueryMasters();

	if (!sb_usemasters->intVal && !(sb_uselan && sb_uselan->intVal))
		Com_Printf(0, "Note: both sb_usemasters and sb_uselan are disabled; list will remain empty.\n");
}

/*
=================
SrvBrowser_List_f
=================
*/
void SrvBrowser_List_f(void)
{
	int i;

	Com_Printf(0, "Server list (%d servers):\n", g_numServers);
	for (i = 0; i < g_numServers; i++) {
		serverSlot_t *slot = g_servers[i];
		Com_Printf(0, "%d: %s - %s\n", i, slot->hostname, 
			slot->status == SLOT_VALID ? "ONLINE" : "PENDING");
	}
}
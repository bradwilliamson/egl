/*
Copyright (C) 2025 EGL Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

//
// m_mp_serverbrowser.c
// Full-featured server browser with visual improvements
//

#include "m_local.h"
#include "sb_layout.h"

#define SB_MAX_VISIBLE_SERVERS	128
#define SB_MAX_VISIBLE_PLAYERS	10
#define SB_MAX_VISIBLE_INFO		8
#define SB_FILTER_MAXLEN		64
#define MAX_STATUS_PLAYERS		64

// Server info panel (key/value list)
#define SB_INFO_MAX_PAIRS		64
#define SB_INFO_KEY_MAX			48
#define SB_INFO_VAL_MAX			192

// Column definitions
#define COL_FAVORITE	0
#define COL_NAME		1
#define COL_MOD			2
#define COL_MAP			3
#define COL_PLAYERS		4
#define COL_PING		5
#define COL_MAX			6

// Server browser menu state
typedef struct {
	uiFrameWork_t	frameWork;
	
	// Selection and scrolling
	int				selectedServer;
	int				scrollOffset;
	int				infoScroll;
	int				numVisibleServers;
	
	// Panels
	int				showInfoPanel;
	int				showPlayersPanel;
	int			lastLayoutMode;
	int			savedShowInfoPanel;
	int			savedShowPlayersPanel;
	
	// Filter and sort
	char			filterText[SB_FILTER_MAXLEN];
	int				filterCursor;
	qBool			filterActive;
	int				sortColumn;
	qBool			sortAscending;
	qBool			sortDirty;
	int			lastSortTime;

	
	// Refresh state
	qBool			isRefreshing;
	unsigned		refreshStartTime;
	unsigned		refreshLastActivityTime;
	int				lastServerCount;
	
	// Status text
	char			statusLeft[128];
	char			statusRight[128];
	
	// Colors
	vec4_t			colorBackground;
	vec4_t			colorSelected;
	vec4_t			colorHeader;
	vec4_t			colorBorder;
	
	// Layout dimensions (calculated on draw)
	float			listX, listY, listW, listH;
	float			infoX, infoY, infoW, infoH;
	float			playersX, playersY, playersW, playersH;
	float			headerH;
	float			rowH;
	float			charW, charH;
} m_serverBrowser_t;

static m_serverBrowser_t m_serverBrowser;

static int ServerBrowser_MaxVisibleServerRows(void)
{
	float availH;
	int maxRows;

	availH = m_serverBrowser.listH - m_serverBrowser.headerH;
	if (m_serverBrowser.rowH <= 0.0f)
		return 1;
	maxRows = (int)(availH / m_serverBrowser.rowH);
	if (maxRows < 1)
		maxRows = 1;
	if (maxRows > SB_MAX_VISIBLE_SERVERS)
		maxRows = SB_MAX_VISIBLE_SERVERS;
	return maxRows;
}

// Forward declarations
static void ServerBrowser_Init(void);
static void ServerBrowser_Draw(void);
static struct sfx_s *ServerBrowser_Key(uiFrameWork_t *fw, keyNum_t keyNum);
static struct sfx_s *ServerBrowser_Close(void);
static void ServerBrowser_Refresh(void);
static void ServerBrowser_Connect(void);
static void ServerBrowser_UpdateStatus(void);
static qBool ServerBrowser_MatchesFilter(int serverIndex);
static void ServerBrowser_Sort(void);

static void ServerBrowser_MaybeApplyDeferredSort(void);
static void ServerBrowser_ToggleFavorite(void);

static void ServerBrowser_FavoritesEnsureCvar(void);
static qBool ServerBrowser_FavoritesListHas(const char *list, const char *address);
static void ServerBrowser_FavoritesListAdd(const char *address);
static void ServerBrowser_FavoritesListRemove(const char *address);

// Server data structure (local to cgame)
typedef struct cgServerSlot_s {
	char		address[64];
	char		*hostname;
	char		*mapName;
	char		*gameName;
	char		*infoString; // raw \\key\\value... status info (first line only)
	int			numPlayers;
	int			maxPlayers;
	int			ping;
	qBool		hasPassword;
	qBool		hasAnticheat;
	qBool		favorite;
	char		players[MAX_STATUS_PLAYERS][64];  // Player names
	int			playerScores[MAX_STATUS_PLAYERS];
	int			playerPings[MAX_STATUS_PLAYERS];
	qBool		valid;
} cgServerSlot_t;

static cgServerSlot_t g_servers[MAX_STATUS_SERVERS];
static int g_numServers = 0;
static int g_pingStartTime = 0;
static cVar_t *sb_favorites = NULL;
static cVar_t *sb_showmarkers = NULL;
static cVar_t *sb_layout = NULL;
static cVar_t *sb_listfrac = NULL;
static cVar_t *sb_fontscale_cvar = NULL;
static cVar_t *sb_compact_cvar = NULL;

static int g_sbLastClickTime = 0;
static int g_sbLastClickServer = -1;

static int ServerBrowser_ServerSlotSortCmp(const void *pa, const void *pb)
{
	const cgServerSlot_t *a = (const cgServerSlot_t *)pa;
	const cgServerSlot_t *b = (const cgServerSlot_t *)pb;
	int cmp = 0;
	int ap, bp;

	// Valid servers first
	if (a->valid != b->valid)
		return (b->valid ? 1 : 0) - (a->valid ? 1 : 0);

	// Favorites always float to the top (within validity)
	if (a->favorite != b->favorite)
		return (b->favorite ? 1 : 0) - (a->favorite ? 1 : 0);

	// Treat unknown ping as very high
	ap = (a->ping > 0) ? a->ping : 999999;
	bp = (b->ping > 0) ? b->ping : 999999;

	switch (m_serverBrowser.sortColumn) {
	case COL_FAVORITE:
		cmp = 0;
		break;
	case COL_NAME:
		cmp = Q_stricmp(a->hostname ? a->hostname : a->address, b->hostname ? b->hostname : b->address);
		break;
	case COL_MOD:
		cmp = Q_stricmp(a->gameName ? a->gameName : "baseq2", b->gameName ? b->gameName : "baseq2");
		break;
	case COL_MAP:
		cmp = Q_stricmp(a->mapName ? a->mapName : "", b->mapName ? b->mapName : "");
		break;
	case COL_PLAYERS:
		cmp = a->numPlayers - b->numPlayers;
		break;
	case COL_PING:
	default:
		cmp = ap - bp;
		break;
	}

	if (!m_serverBrowser.sortAscending)
		cmp = -cmp;

	// Tie-breakers for stable-ish ordering
	if (cmp == 0)
		cmp = Q_stricmp(a->hostname ? a->hostname : a->address, b->hostname ? b->hostname : b->address);
	if (cmp == 0)
		cmp = Q_stricmp(a->address, b->address);
	return cmp;
}

// Get server count
static int ServerBrowser_GetServerCount(void)
{
	return g_numServers;
}

// Get server by index
static cgServerSlot_t *ServerBrowser_GetServer(int index)
{
	if (index < 0 || index >= g_numServers)
		return NULL;
	return &g_servers[index];
}

// Find server by address
static cgServerSlot_t *ServerBrowser_FindServer(const char *address)
{
	int i;
	for (i = 0; i < g_numServers; i++) {
		if (!strcmp(g_servers[i].address, address))
			return &g_servers[i];
	}
	return NULL;
}

// Free server data
static void ServerBrowser_FreeServer(cgServerSlot_t *server)
{
	if (server->hostname) CG_MemFree(server->hostname);
	if (server->mapName) CG_MemFree(server->mapName);
	if (server->gameName) CG_MemFree(server->gameName);
	if (server->infoString) CG_MemFree(server->infoString);
	memset(server, 0, sizeof(*server));
}

static void ServerBrowser_FavoritesEnsureCvar(void)
{
	if (!sb_favorites)
		sb_favorites = cgi.Cvar_Register("sb_favorites", "", CVAR_ARCHIVE);
}

static void ServerBrowser_MarkersEnsureCvar(void)
{
	if (!sb_showmarkers)
		sb_showmarkers = cgi.Cvar_Register("sb_showmarkers", "0", CVAR_ARCHIVE);
}

static void ServerBrowser_LayoutEnsureCvars(void)
{
	if (!sb_layout)
		sb_layout = cgi.Cvar_Register("sb_layout", "0", CVAR_ARCHIVE);
	if (!sb_listfrac)
		sb_listfrac = cgi.Cvar_Register("sb_listfrac", "0.6", CVAR_ARCHIVE);
	if (!sb_fontscale_cvar)
		sb_fontscale_cvar = cgi.Cvar_Register("sb_fontscale", "0.6", CVAR_ARCHIVE);
	if (!sb_compact_cvar)
		sb_compact_cvar = cgi.Cvar_Register("sb_compact", "0", CVAR_ARCHIVE);
}

static qBool ServerBrowser_FavoritesListHas(const char *list, const char *address)
{
	const char *p;
	const char *tokStart;
	int tokLen;
	char token[64];

	if (!list || !list[0] || !address || !address[0])
		return qFalse;

	p = list;
	while (*p) {
		while (*p == ';' || *p == ',' || *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			p++;
		if (!*p)
			break;
		tokStart = p;
		while (*p && *p != ';' && *p != ',' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
			p++;
		tokLen = (int)(p - tokStart);
		if (tokLen <= 0)
			continue;
		if (tokLen >= (int)sizeof(token))
			tokLen = (int)sizeof(token) - 1;
		memcpy(token, tokStart, tokLen);
		token[tokLen] = '\0';
		if (!Q_stricmp(token, address))
			return qTrue;
	}

	return qFalse;
}

static void ServerBrowser_FavoritesListAdd(const char *address)
{
	const char *cur;
	char buf[2048];
	int len;

	ServerBrowser_FavoritesEnsureCvar();
	cur = cgi.Cvar_GetStringValue("sb_favorites");
	if (ServerBrowser_FavoritesListHas(cur, address))
		return;

	Q_strncpyz(buf, cur ? cur : "", sizeof(buf));
	len = (int)strlen(buf);
	if (len > 0 && buf[len - 1] != ';')
		Q_strcatz(buf, ";", sizeof(buf));
	Q_strcatz(buf, address, sizeof(buf));

	cgi.Cvar_Set("sb_favorites", buf, qTrue);
}

static void ServerBrowser_FavoritesListRemove(const char *address)
{
	const char *cur;
	const char *p;
	const char *tokStart;
	int tokLen;
	char out[2048];
	char token[64];
	qBool first;

	ServerBrowser_FavoritesEnsureCvar();
	cur = cgi.Cvar_GetStringValue("sb_favorites");
	if (!cur || !cur[0])
		return;

	out[0] = '\0';
	first = qTrue;

	p = cur;
	while (*p) {
		while (*p == ';' || *p == ',' || *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
			p++;
		if (!*p)
			break;
		tokStart = p;
		while (*p && *p != ';' && *p != ',' && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
			p++;
		tokLen = (int)(p - tokStart);
		if (tokLen <= 0)
			continue;
		if (tokLen >= (int)sizeof(token))
			tokLen = (int)sizeof(token) - 1;
		memcpy(token, tokStart, tokLen);
		token[tokLen] = '\0';
		if (!Q_stricmp(token, address))
			continue;
		if (!first)
			Q_strcatz(out, ";", sizeof(out));
		Q_strcatz(out, token, sizeof(out));
		first = qFalse;
	}

	cgi.Cvar_Set("sb_favorites", out, qTrue);
}

/*
=================
ServerBrowser_Init
=================
*/
static void ServerBrowser_Init(void)
{
	Com_Printf(0, "^2ServerBrowser_Init: Initializing server browser\n");
	memset(&m_serverBrowser, 0, sizeof(m_serverBrowser));
	
	// Initialize framework
	UI_StartFramework(&m_serverBrowser.frameWork, 0);
	UI_FinishFramework(&m_serverBrowser.frameWork, qFalse);
	
	// Set colors - green terminal theme
	m_serverBrowser.colorBackground[0] = 0.0f;
	// Slightly darker background so accent colors pop
	m_serverBrowser.colorBackground[1] = 0.32f;  // Dark green
	m_serverBrowser.colorBackground[2] = 0.0f;
	m_serverBrowser.colorBackground[3] = 0.9f;
	
	m_serverBrowser.colorSelected[0] = 0.0f;
	// Selected row: slightly brighter for separation, with neutral text on top
	m_serverBrowser.colorSelected[1] = 0.62f;  // Bright green
	m_serverBrowser.colorSelected[2] = 0.0f;
	m_serverBrowser.colorSelected[3] = 1.0f;
	
	m_serverBrowser.colorHeader[0] = 0.0f;
	m_serverBrowser.colorHeader[1] = 1.0f;  // Brightest green
	m_serverBrowser.colorHeader[2] = 0.0f;
	m_serverBrowser.colorHeader[3] = 1.0f;
	
	m_serverBrowser.colorBorder[0] = 0.0f;
	// Borders should frame content, not compete with it
	m_serverBrowser.colorBorder[1] = 0.42f;
	m_serverBrowser.colorBorder[2] = 0.0f;
	m_serverBrowser.colorBorder[3] = 1.0f;
	
	// Initialize state
	m_serverBrowser.selectedServer = 0;
	m_serverBrowser.scrollOffset = 0;
	m_serverBrowser.infoScroll = 0;
	m_serverBrowser.sortColumn = COL_PING;
	m_serverBrowser.sortAscending = qTrue;
	m_serverBrowser.sortDirty = qFalse;
	m_serverBrowser.lastSortTime = 0;
	ServerBrowser_LayoutEnsureCvars();
	m_serverBrowser.lastLayoutMode = cgi.Cvar_GetIntegerValue("sb_layout");
	m_serverBrowser.savedShowInfoPanel = qTrue;
	m_serverBrowser.savedShowPlayersPanel = qTrue;
	if (cgi.Cvar_GetIntegerValue("sb_layout") != 0) {
		// Wide/Q2Pro-like: list-only.
		m_serverBrowser.showInfoPanel = qFalse;
		m_serverBrowser.showPlayersPanel = qFalse;
	} else {
		m_serverBrowser.showInfoPanel = qTrue;
		m_serverBrowser.showPlayersPanel = qTrue;
	}
	m_serverBrowser.filterActive = qFalse;
	m_serverBrowser.filterText[0] = '\0';
	m_serverBrowser.filterCursor = 0;
	
	// Start refresh
	Com_Printf(0, "^2ServerBrowser_Init: About to call ServerBrowser_Refresh()...\n");
	ServerBrowser_Refresh();
	
	// Update status
	ServerBrowser_UpdateStatus();
	
	Com_Printf(0, "^2ServerBrowser_Init: Complete. g_numServers=%d\n", g_numServers);
	Com_Printf(0, "^2ServerBrowser_Init: frameWork.initialized=%d\n", m_serverBrowser.frameWork.initialized);
}

static void ServerBrowser_SortEnsureSelectionVisible(void)
{
	int i;
	int visibleIndex;
	int firstMatch;
	qBool haveVisibleFromScroll;

	if (g_numServers <= 0) {
		m_serverBrowser.selectedServer = -1;
		m_serverBrowser.scrollOffset = 0;
		return;
	}

	if (m_serverBrowser.selectedServer < 0)
		m_serverBrowser.selectedServer = 0;
	if (m_serverBrowser.selectedServer >= g_numServers)
		m_serverBrowser.selectedServer = g_numServers - 1;

	if (m_serverBrowser.scrollOffset < 0)
		m_serverBrowser.scrollOffset = 0;
	if (m_serverBrowser.scrollOffset >= g_numServers)
		m_serverBrowser.scrollOffset = g_numServers - 1;

	// If selection is filtered out, snap to the first visible server.
	if (!ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer)) {
		firstMatch = -1;
		for (i = 0; i < g_numServers; i++) {
			if (ServerBrowser_MatchesFilter(i)) {
				firstMatch = i;
				break;
			}
		}
		if (firstMatch < 0) {
			m_serverBrowser.selectedServer = -1;
			m_serverBrowser.scrollOffset = 0;
			return;
		}
		m_serverBrowser.selectedServer = firstMatch;
	}

	// If scroll starts past all visible (filtered) entries, reset it.
	haveVisibleFromScroll = qFalse;
	for (i = m_serverBrowser.scrollOffset; i < g_numServers; i++) {
		if (ServerBrowser_MatchesFilter(i)) {
			haveVisibleFromScroll = qTrue;
			break;
		}
	}
	if (!haveVisibleFromScroll)
		m_serverBrowser.scrollOffset = 0;

	if (m_serverBrowser.selectedServer < m_serverBrowser.scrollOffset)
		m_serverBrowser.scrollOffset = m_serverBrowser.selectedServer;

	visibleIndex = 0;
	for (i = m_serverBrowser.scrollOffset; i <= m_serverBrowser.selectedServer && i < g_numServers; i++) {
		if (ServerBrowser_MatchesFilter(i))
			visibleIndex++;
	}
	if (visibleIndex <= 0)
		return;

	while (visibleIndex > ServerBrowser_MaxVisibleServerRows() && m_serverBrowser.scrollOffset < m_serverBrowser.selectedServer) {
		m_serverBrowser.scrollOffset++;
		visibleIndex = 0;
		for (i = m_serverBrowser.scrollOffset; i <= m_serverBrowser.selectedServer && i < g_numServers; i++) {
			if (ServerBrowser_MatchesFilter(i))
				visibleIndex++;
		}
	}
}

static int ServerBrowser_VisibleCharCount(const char *s)
{
	int count;

	if (!s)
		return 0;

	count = 0;
	while (*s) {
		if (((*s) & 127) == COLOR_ESCAPE && s[1]) {
			if (((s[1]) & 127) == COLOR_ESCAPE) {
				count++;
				s += 2;
				continue;
			}
			s += 2;
			continue;
		}
		count++;
		s++;
	}

	return count;
}

static void ServerBrowser_CopyTruncateColor(const char *in, char *out, size_t outSize, int maxVisible)
{
	const char *s;
	char *d;
	size_t left;
	int totalVisible;
	int limit;
	int visible;

	if (!out || outSize == 0)
		return;
	out[0] = '\0';
	if (!in)
		return;
	if (maxVisible <= 0)
		return;

	totalVisible = ServerBrowser_VisibleCharCount(in);
	limit = maxVisible;
	if (totalVisible > maxVisible && maxVisible >= 4)
		limit = maxVisible - 3;

	s = in;
	d = out;
	left = outSize;
	visible = 0;

	while (*s && left > 1 && visible < limit) {
		if (((*s) & 127) == COLOR_ESCAPE && s[1]) {
			if (((s[1]) & 127) == COLOR_ESCAPE) {
				if (left <= 2)
					break;
				*d++ = *s++;
				*d++ = *s++;
				left -= 2;
				visible++;
				continue;
			}
			if (left <= 2)
				break;
			*d++ = *s++;
			*d++ = *s++;
			left -= 2;
			continue;
		}

		*d++ = *s++;
		left--;
		visible++;
	}

	if (totalVisible > maxVisible && maxVisible >= 4) {
		if (left > 1) {
			*d++ = '.';
			left--;
		}
		if (left > 1) {
			*d++ = '.';
			left--;
		}
		if (left > 1) {
			*d++ = '.';
			left--;
		}
	}

	*d = '\0';
}

static int ServerBrowser_ParseInfoPairs(const char *info, char keys[SB_INFO_MAX_PAIRS][SB_INFO_KEY_MAX], char values[SB_INFO_MAX_PAIRS][SB_INFO_VAL_MAX], int maxPairs)
{
	const char *p;
	const char *ks, *vs;
	int count;
	int klen, vlen;

	if (!info || !info[0] || maxPairs <= 0)
		return 0;

	p = info;
	if (*p == '\\')
		p++;
	count = 0;

	while (*p && *p != '\n' && count < maxPairs) {
		ks = p;
		while (*p && *p != '\\' && *p != '\n')
			p++;
		klen = (int)(p - ks);
		if (*p == '\\')
			p++;

		vs = p;
		while (*p && *p != '\\' && *p != '\n')
			p++;
		vlen = (int)(p - vs);
		if (*p == '\\')
			p++;

		if (klen <= 0)
			continue;
		if (klen >= SB_INFO_KEY_MAX)
			klen = SB_INFO_KEY_MAX - 1;
		if (vlen >= SB_INFO_VAL_MAX)
			vlen = SB_INFO_VAL_MAX - 1;

		memcpy(keys[count], ks, klen);
		keys[count][klen] = '\0';
		memcpy(values[count], vs, vlen);
		values[count][vlen] = '\0';
		count++;
	}

	return count;
}

static void ServerBrowser_ComputeColWidths(float w, float colW[COL_MAX])
{
	int compact;
	int layoutMode;
	float minNameW;
	float fixedW;
	float mapW;

	// Column widths for cleaner spacing with MOD+MAP visible
	compact = cgi.Cvar_GetIntegerValue("sb_compact");
	layoutMode = cgi.Cvar_GetIntegerValue("sb_layout");
	// Track mode transitions so switching sb_layout back to 0 restores panels.
	if (layoutMode != m_serverBrowser.lastLayoutMode) {
		if (layoutMode != 0) {
			m_serverBrowser.savedShowInfoPanel = m_serverBrowser.showInfoPanel;
			m_serverBrowser.savedShowPlayersPanel = m_serverBrowser.showPlayersPanel;
			m_serverBrowser.showInfoPanel = qFalse;
			m_serverBrowser.showPlayersPanel = qFalse;
		} else {
			m_serverBrowser.showInfoPanel = m_serverBrowser.savedShowInfoPanel;
			m_serverBrowser.showPlayersPanel = m_serverBrowser.savedShowPlayersPanel;
			// If both were off, default to panels on.
			if (!m_serverBrowser.showInfoPanel && !m_serverBrowser.showPlayersPanel) {
				m_serverBrowser.showInfoPanel = qTrue;
				m_serverBrowser.showPlayersPanel = qTrue;
			}
		}
		m_serverBrowser.lastLayoutMode = layoutMode;
	}
	minNameW = m_serverBrowser.charW * 20;
	mapW = m_serverBrowser.charW * 8;

	colW[COL_FAVORITE] = m_serverBrowser.charW * 2;
	colW[COL_PING] = m_serverBrowser.charW * 6;
	colW[COL_PLAYERS] = m_serverBrowser.charW * 5;
	colW[COL_MOD] = m_serverBrowser.charW * 8;
	colW[COL_MAP] = mapW;

	// Wide/Q2Pro-like list modes prioritize server name readability.
	// sb_layout:
	//   0: split view (respect sb_compact)
	//   1: wide list-only, hide MOD+MAP
	//   2+: wide list-only, show MAP (hide MOD)
	if (layoutMode != 0) {
		colW[COL_PING] = m_serverBrowser.charW * 5;
		colW[COL_MOD] = 0;
		if (layoutMode == 1)
			colW[COL_MAP] = 0;
		compact = 1; // avoid re-enabling MOD below
	}

	fixedW = colW[COL_FAVORITE] + colW[COL_PING] + colW[COL_PLAYERS];
	colW[COL_NAME] = w - fixedW - colW[COL_MOD] - colW[COL_MAP];

	// Compact mode / narrow layouts: drop MOD first, but keep MAP fixed-width.
	if (compact || colW[COL_NAME] < minNameW) {
		colW[COL_MOD] = 0;
		colW[COL_NAME] = w - fixedW - colW[COL_MAP];
	}

	// Clamp to sane values (avoid negative widths)
	if (colW[COL_NAME] < 0)
		colW[COL_NAME] = 0;
	if (colW[COL_MAP] < 0)
		colW[COL_MAP] = 0;
}

static void ServerBrowser_SetSortColumn(int col)
{
	if (col < 0 || col >= COL_MAX)
		return;

	if (col == m_serverBrowser.sortColumn) {
		m_serverBrowser.sortAscending = !m_serverBrowser.sortAscending;
	} else {
		m_serverBrowser.sortColumn = col;
		// Sensible defaults per column
		switch (col) {
		case COL_PLAYERS:
		case COL_FAVORITE:
			m_serverBrowser.sortAscending = qFalse;
			break;
		case COL_PING:
		default:
			m_serverBrowser.sortAscending = qTrue;
			break;
		}
	}

	m_serverBrowser.sortDirty = qTrue;
}

/*
=================
ServerBrowser_UpdateStatus
=================
*/
static void ServerBrowser_UpdateStatus(void)
{
	int totalServers = 0, totalPlayers = 0;
	int i;
	cgServerSlot_t *server;
	
	// Count valid servers and total players
	for (i = 0; i < g_numServers; i++) {
		server = ServerBrowser_GetServer(i);
		if (!server || !server->valid)
			continue;
			
		totalServers++;
		totalPlayers += server->numPlayers;
	}
	
	// Update status strings
	Q_snprintfz(m_serverBrowser.statusLeft, sizeof(m_serverBrowser.statusLeft),
		"%d server%s | %d player%s",
		totalServers, totalServers == 1 ? "" : "s",
		totalPlayers, totalPlayers == 1 ? "" : "s");
	
	// Right status shows key hints
	if (m_serverBrowser.filterActive) {
		Q_snprintfz(m_serverBrowser.statusRight, sizeof(m_serverBrowser.statusRight),
			"Type to filter | ENTER=Apply | ESC=Cancel");
	} else if (m_serverBrowser.isRefreshing) {
		Q_snprintfz(m_serverBrowser.statusRight, sizeof(m_serverBrowser.statusRight),
			"Refreshing... | SPACE=Refresh | F=Favorite");
	} else if (m_serverBrowser.selectedServer >= 0) {
		Q_snprintfz(m_serverBrowser.statusRight, sizeof(m_serverBrowser.statusRight),
			"ENTER=Connect | SPACE=Refresh | F=Favorite");
	} else {
		Q_snprintfz(m_serverBrowser.statusRight, sizeof(m_serverBrowser.statusRight),
			"SPACE=Refresh All | ESC=Close");
	}
}

static qBool SB_ParsePlayerLine(const char *line, int *outScore, int *outPing, char *outName, size_t outNameSize)
{
	const char *p;
	char *end;
	long score;
	long ping;
	const char *nameStart;
	const char *nameEnd;
	size_t nameLen;

	if (outScore)
		*outScore = 0;
	if (outPing)
		*outPing = 0;
	if (outName && outNameSize > 0)
		outName[0] = '\0';

	if (!line || !outScore || !outPing || !outName || outNameSize == 0)
		return qFalse;

	p = line;
	while (*p == ' ' || *p == '\t')
		p++;

	score = strtol(p, &end, 10);
	if (end == p)
		return qFalse;
	p = end;
	while (*p == ' ' || *p == '\t')
		p++;

	ping = strtol(p, &end, 10);
	if (end == p)
		return qFalse;
	p = end;
	while (*p == ' ' || *p == '\t')
		p++;

	nameStart = p;
	nameEnd = NULL;

	if (*nameStart == '"') {
		nameStart++;
		nameEnd = strchr(nameStart, '"');
		if (!nameEnd) {
			// Malformed: treat rest of line as name
			nameEnd = nameStart + strlen(nameStart);
		}
	} else {
		nameEnd = nameStart + strlen(nameStart);
	}

	// Trim trailing whitespace and optional \r
	while (nameEnd > nameStart && (nameEnd[-1] == ' ' || nameEnd[-1] == '\t' || nameEnd[-1] == '\r'))
		nameEnd--;

	nameLen = (size_t)(nameEnd - nameStart);
	if (nameLen >= outNameSize)
		nameLen = outNameSize - 1;

	if (nameLen > 0)
		memcpy(outName, nameStart, nameLen);
	outName[nameLen] = '\0';

	*outScore = (int)score;
	*outPing = (int)ping;
	return qTrue;
}

/*
=================
ServerBrowser_AddFromStatusResponse

Called when client receives a server status response
Adapted from UI_ParseServerStatus in m_mp_join.c
=================
*/
qBool ServerBrowser_AddFromStatusResponse(const char *address, const char *infostring)
{
	cgServerSlot_t *server;
	const char *hostname, *mapname, *gamename, *maxclients, *srvping;
	int ping;
	char *info;
	const char *nl;
	int infoLen;
	char infoLine[2048];
	char lineBuf[256];
	int playerCount = 0;
	
	// Allow data collection even if menu isn't open yet
	// This way responses can be cached for when the user opens the browser
	
	if (!address || !address[0] || !infostring || !infostring[0])
		return qFalse;
	
	// Find or create server slot
	ServerBrowser_FavoritesEnsureCvar();
	server = ServerBrowser_FindServer(address);
	if (!server) {
		if (g_numServers >= MAX_STATUS_SERVERS)
			return qFalse;
		server = &g_servers[g_numServers++];
		Q_strncpyz(server->address, address, sizeof(server->address));
	} else {
		// Free old data
		ServerBrowser_FreeServer(server);
		Q_strncpyz(server->address, address, sizeof(server->address));
	}
	
	// Parse infostring
	// Keep a raw copy of the \\key\\value... section for the Server Info panel (stop at newline).
	nl = strchr(infostring, '\n');
	infoLen = nl ? (int)(nl - infostring) : (int)strlen(infostring);
	if (infoLen < 0)
		infoLen = 0;
	if (infoLen >= (int)sizeof(infoLine))
		infoLen = (int)sizeof(infoLine) - 1;
	memcpy(infoLine, infostring, infoLen);
	infoLine[infoLen] = '\0';
	server->infoString = CG_TagStrDup(infoLine, CGTAG_MENU);

	hostname = Info_ValueForKey(infostring, "hostname");
	if (!hostname || !hostname[0])
		hostname = address;
	server->hostname = CG_TagStrDup(hostname, CGTAG_MENU);
	
	mapname = Info_ValueForKey(infostring, "mapname");
	if (!mapname || !mapname[0])
		mapname = "---";
	server->mapName = CG_TagStrDup(mapname, CGTAG_MENU);
	
	gamename = Info_ValueForKey(infostring, "gamename");
	if (!gamename || !gamename[0])
		gamename = "baseq2";
	server->gameName = CG_TagStrDup(gamename, CGTAG_MENU);
	
	maxclients = Info_ValueForKey(infostring, "maxclients");
	server->maxPlayers = atoi(maxclients);
	if (server->maxPlayers <= 0)
		server->maxPlayers = 8;  // Default
	
	server->hasPassword = (atoi(Info_ValueForKey(infostring, "needpass")) > 0);
	server->hasAnticheat = (atoi(Info_ValueForKey(infostring, "anticheat")) > 0);
	
	// Use pre-calculated ping from client (passed via __srvping key)
	srvping = Info_ValueForKey(infostring, "__srvping");
	if (srvping && srvping[0]) {
		ping = atoi(srvping);
	} else if (g_pingStartTime > 0) {
		// Fallback to old method
		ping = cgi.Sys_Milliseconds() - g_pingStartTime;
	} else {
		ping = 0;
	}
	if (ping < 0) ping = 0;
	if (ping > 9999) ping = 9999;
	server->ping = ping;

	if (m_serverBrowser.isRefreshing)
		m_serverBrowser.refreshLastActivityTime = cgi.Sys_Milliseconds();
	
	// Parse player list from remaining data (after first newline)
	// Format: "score ping name\n"
	info = strchr(infostring, '\n');
	if (info) {
		info++;  // Skip newline
		while (*info && playerCount < MAX_STATUS_PLAYERS) {
			// Read one line
			int i = 0;
			while (*info && *info != '\n' && i < (int)sizeof(lineBuf) - 1) {
				lineBuf[i++] = *info++;
			}
			lineBuf[i] = '\0';
			if (*info == '\n') info++;
			
			if (lineBuf[0]) {
				int score = 0, playerPing = 0;
				char name[64];
				if (SB_ParsePlayerLine(lineBuf, &score, &playerPing, name, sizeof(name))) {
					server->playerScores[playerCount] = score;
					server->playerPings[playerCount] = playerPing;
					Q_strncpyz(server->players[playerCount], name, sizeof(server->players[0]));
					playerCount++;
				}
			}
		}
	}
	
	server->numPlayers = playerCount;
	server->valid = qTrue;
	server->favorite = ServerBrowser_FavoritesListHas(cgi.Cvar_GetStringValue("sb_favorites"), server->address);
	m_serverBrowser.sortDirty = qTrue;
	
	return qTrue;
}

/*
=================
ServerBrowser_Refresh
=================
*/
static void ServerBrowser_Refresh(void)
{
	m_serverBrowser.isRefreshing = qTrue;
	m_serverBrowser.refreshStartTime = cgi.Sys_Milliseconds();
	m_serverBrowser.refreshLastActivityTime = m_serverBrowser.refreshStartTime;
	m_serverBrowser.lastServerCount = g_numServers;
	g_pingStartTime = cgi.Sys_Milliseconds();
	
	// Clear existing servers
	int i;
	for (i = 0; i < g_numServers; i++) {
		ServerBrowser_FreeServer(&g_servers[i]);
	}
	g_numServers = 0;
	m_serverBrowser.lastServerCount = 0;
	m_serverBrowser.selectedServer = 0;
	m_serverBrowser.scrollOffset = 0;
	m_serverBrowser.sortDirty = qTrue;
	
	// Trigger refresh via console command
	Com_Printf(0, "^2ServerBrowser_Refresh: Sending 'sb_refresh' command\n");
	cgi.Cbuf_AddText("sb_refresh\n");
	Com_Printf(0, "^2ServerBrowser_Refresh: Command sent\n");
}

/*
=================
ServerBrowser_MatchesFilter
=================
*/
static qBool ServerBrowser_MatchesFilter(int serverIndex)
{
	cgServerSlot_t *server;
	char lowerHostname[256];
	char lowerFilter[SB_FILTER_MAXLEN];
	
	if (m_serverBrowser.filterText[0] == '\0')
		return qTrue;
	
	server = ServerBrowser_GetServer(serverIndex);
	if (!server || !server->hostname)
		return qFalse;
	
	// Convert both to lowercase for case-insensitive comparison
	Q_strncpyz(lowerHostname, server->hostname, sizeof(lowerHostname));
	Q_strlwr(lowerHostname);
	
	Q_strncpyz(lowerFilter, m_serverBrowser.filterText, sizeof(lowerFilter));
	Q_strlwr(lowerFilter);
	
	// Simple substring search
	return (strstr(lowerHostname, lowerFilter) != NULL);
}

/*
=================
ServerBrowser_Connect
=================
*/
static void ServerBrowser_Connect(void)
{
	cgServerSlot_t *server;
	char cmd[256];
	
	if (m_serverBrowser.selectedServer < 0)
		return;
	
	server = ServerBrowser_GetServer(m_serverBrowser.selectedServer);
	if (!server || !server->valid)
		return;
	
	// Connect to server
	Q_snprintfz(cmd, sizeof(cmd), "connect %s\n", server->address);
	cgi.Cbuf_AddText(cmd);
	
	M_ForceMenuOff();
}

/*
=================
ServerBrowser_ToggleFavorite
=================
*/
static void ServerBrowser_ToggleFavorite(void)
{
	cgServerSlot_t *server;

	if (m_serverBrowser.selectedServer < 0)
		return;
	server = ServerBrowser_GetServer(m_serverBrowser.selectedServer);
	if (!server)
		return;

	server->favorite = !server->favorite;
	if (server->favorite)
		ServerBrowser_FavoritesListAdd(server->address);
	else
		ServerBrowser_FavoritesListRemove(server->address);
	m_serverBrowser.sortDirty = qTrue;
}

/*
=================
ServerBrowser_DrawHeader
=================
*/
static void ServerBrowser_DrawHeader(float x, float y, float w)
{
	float colW[COL_MAX];
	float curX;
	const char *headers[COL_MAX] = {"F", "SERVER", "MOD", "MAP", "PLR", "PING"};
	char headerText[64];
	int i;
	float textY;
	vec4_t headerBg;
	vec4_t headerTextColor;
	vec4_t headerUnderline;
	float headerScale;
	float headerCharH;
	int drewAny;
	
	ServerBrowser_ComputeColWidths(w, colW);
	
	// Draw header background (darker for readability)
	Vec4Set(headerBg, 0.0f, 0.23f, 0.0f, 1.0f);
	Vec4Set(headerTextColor, 0.85f, 1.0f, 0.85f, 1.0f);
	Vec4Set(headerUnderline, 0.35f, 1.0f, 0.35f, 0.75f);
	cgi.R_DrawFill(x, y, w, m_serverBrowser.headerH, headerBg);
	// Subtle top/bottom accents
	cgi.R_DrawFill(x, y, w, 1, m_serverBrowser.colorBorder);
	cgi.R_DrawFill(x, y + m_serverBrowser.headerH - 1, w, 1, m_serverBrowser.colorBorder);
	// Very subtle underline to make the header pop
	if (m_serverBrowser.headerH >= 6)
		cgi.R_DrawFill(x, y + m_serverBrowser.headerH - 3, w, 1, headerUnderline);

	// Use the same scale as the rows (based on m_serverBrowser.charW/charH)
	headerScale = SB_ScaleFromCharCell8(m_serverBrowser.charW, UIFT_SCALE);
	headerCharH = m_serverBrowser.charH;
	textY = y + (m_serverBrowser.headerH - headerCharH) * 0.5f;
	if (textY < y + 1)
		textY = y + 1;
	
	// Draw column headers
	curX = x;
	drewAny = 0;
	for (i = 0; i < COL_MAX; i++) {
		if (colW[i] <= 0)
			continue;
		Q_strncpyz(headerText, headers[i], sizeof(headerText));
		
		// Add sort indicator
		if (i == m_serverBrowser.sortColumn) {
			Q_strcatz(headerText, m_serverBrowser.sortAscending ? " ^" : " v", sizeof(headerText));
		}
		
		// Column separator (skip left edge / hidden columns)
		if (drewAny)
			cgi.R_DrawFill(curX, y, 1, m_serverBrowser.headerH, m_serverBrowser.colorBorder);

		// No shadow for crispness at small sizes; match row padding
		cgi.R_DrawString(NULL, (float)((int)(curX + 2)), (float)((int)(textY)), headerScale, headerScale, FS_SECONDARY, headerText, headerTextColor);
		curX += colW[i];
		drewAny = 1;
	}
}

/*
=================
ServerBrowser_DrawServerRow
=================
*/
static void ServerBrowser_DrawServerRow(float x, float y, float w, int serverIndex, qBool selected)
{
	cgServerSlot_t *server;
	float colW[COL_MAX];
	float curX;
	vec4_t rowColor;
	vec4_t baseTextColor;
	vec4_t nameTextColor;
	vec4_t metaTextColor;
	vec4_t pingTextColor;
	vec4_t mapTextColor;
	vec4_t favBoxColor;
	vec4_t selectedOutline;
	char text[256];
	char nameBuf[512];
	char prefix[32];
	const char *mapName, *hostname, *modName;
	int ping;
	float sbScale;
	float textY;
	int maxVisible;
	float availPx;
	float boxW, boxH;
	float boxX, boxY;
	float strW;
	float drawX;
	vec4_t shadowColor;
	int drawFlags;
	
	server = ServerBrowser_GetServer(serverIndex);
	if (!server)
		return;
	
	ServerBrowser_ComputeColWidths(w, colW);
	sbScale = SB_ScaleFromCharCell8(m_serverBrowser.charW, UIFT_SCALE);
	textY = y + (m_serverBrowser.rowH - m_serverBrowser.charH) * 0.5f;
	if (textY < y + 1)
		textY = y + 1;

	// Selected rows get a subtle shadow to keep text readable on bright highlight.
	Vec4Set(shadowColor, 0.0f, 0.0f, 0.0f, 0.90f);
	drawFlags = selected ? FS_SECONDARY : 0;
	
	// Background color
	if (selected) {
		memcpy(rowColor, m_serverBrowser.colorSelected, sizeof(vec4_t));
	} else {
		// Alternate row colors
		rowColor[0] = 0;
		rowColor[1] = (serverIndex % 2) ? 0.17f : 0.13f;
		rowColor[2] = 0;
		rowColor[3] = 1.0f;
	}
	cgi.R_DrawFill(x, y, w, m_serverBrowser.rowH, rowColor);

	// Subtle separation for selected row
	if (selected) {
		Vec4Set(selectedOutline, 0.0f, 0.0f, 0.0f, 0.35f);
		cgi.R_DrawFill(x, y, w, 1, selectedOutline);
		cgi.R_DrawFill(x, y + m_serverBrowser.rowH - 1, w, 1, selectedOutline);
	}
	
	// Neutral base text for readability; use accent colors only where meaningful.
	if (selected) {
		// Brighter text on selection highlight (with shadow), so it doesn't go "black".
		Vec4Set(baseTextColor, 0.95f, 0.95f, 0.95f, 1.0f);
		Vec4Set(nameTextColor, 1.00f, 1.00f, 1.00f, 1.0f);
		Vec4Set(metaTextColor, 0.88f, 0.88f, 0.88f, 1.0f);
	} else if (server->valid) {
		Vec4Set(baseTextColor, 0.85f, 0.85f, 0.85f, 1.0f);
		Vec4Set(nameTextColor, 0.92f, 0.92f, 0.92f, 1.0f);
		Vec4Set(metaTextColor, 0.72f, 0.72f, 0.72f, 1.0f);
	} else {
		Vec4Set(baseTextColor, 0.55f, 0.55f, 0.55f, 1.0f);
		Vec4Copy(baseTextColor, nameTextColor);
		Vec4Copy(baseTextColor, metaTextColor);
	}

	// Ping gets the strong color coding.
	if (server->valid) {
		ping = server->ping;
		if (ping < 50) {
			Vec4Set(pingTextColor, 0.45f, 1.00f, 0.45f, 1.0f);  // Green
		} else if (ping < 100) {
			Vec4Set(pingTextColor, 1.00f, 0.92f, 0.35f, 1.0f);  // Yellow
		} else if (ping < 150) {
			Vec4Set(pingTextColor, 1.00f, 0.65f, 0.20f, 1.0f);  // Orange
		} else {
			Vec4Set(pingTextColor, 1.00f, 0.35f, 0.35f, 1.0f);  // Red (less bleeding)
		}
	} else {
		Vec4Copy(metaTextColor, pingTextColor);
	}

	// Map column slightly dimmer than hostname
	if (selected) {
		Vec4Copy(baseTextColor, mapTextColor);
	} else {
		Vec4Set(mapTextColor, metaTextColor[0] * 0.90f, metaTextColor[1] * 0.90f, metaTextColor[2] * 0.90f, metaTextColor[3]);
	}
	
	// Draw columns
	curX = x;
	
	// Favorite star (slightly bolder so it reads as a feature)
	if (server->favorite) {
		vec4_t starColor;

		// Filled box behind the star to make it feel intentional.
		if (selected)
			Vec4Set(favBoxColor, 0.30f, 1.00f, 0.30f, 0.55f);
		else
			Vec4Set(favBoxColor, 0.22f, 0.90f, 0.22f, 0.45f);
		boxW = (m_serverBrowser.charW * 1.1f);
		boxH = (m_serverBrowser.charH * 0.90f);
		if (boxW < 10)
			boxW = 10;
		if (boxH < 10)
			boxH = 10;
		boxX = (float)((int)(curX + 1));
		boxY = (float)((int)(y + (m_serverBrowser.rowH - boxH) * 0.5f));
		cgi.R_DrawFill(boxX, boxY, boxW, boxH, favBoxColor);

		if (selected)
			Vec4Set(starColor, 0.95f, 1.0f, 0.95f, 1.0f);
		else
			Vec4Set(starColor, 0.75f, 1.0f, 0.75f, 1.0f);
		cgi.R_DrawString(NULL, (float)((int)(curX + 2)), (float)((int)(textY)), sbScale, sbScale, 0, "*", starColor);
		cgi.R_DrawString(NULL, (float)((int)(curX + 3)), (float)((int)(textY)), sbScale, sbScale, 0, "*", starColor);
	}
	curX += colW[COL_FAVORITE];
	
	// Server name - allow longer names with smaller font
	hostname = server->hostname ? server->hostname : server->address;
	prefix[0] = '\0';
	// Optional markers (colored) like Q2Pro's info emphasis.
	ServerBrowser_MarkersEnsureCvar();
	SB_BuildMarkerPrefix(prefix, (int)sizeof(prefix), cgi.Cvar_GetIntegerValue("sb_showmarkers") != 0,
				server->hasPassword, server->hasAnticheat);
	Q_snprintfz(nameBuf, sizeof(nameBuf), "%s%s", prefix, hostname);
	availPx = colW[COL_NAME] - 6;
	maxVisible = (m_serverBrowser.charW > 0) ? (int)(availPx / m_serverBrowser.charW) : 0;
	ServerBrowser_CopyTruncateColor(nameBuf, text, sizeof(text), maxVisible);
	if (selected)
		cgi.R_DrawString(NULL, (float)((int)(curX + 3)), (float)((int)(textY + 1)), sbScale, sbScale, 0, text, shadowColor);
	cgi.R_DrawString(NULL, (float)((int)(curX + 2)), (float)((int)(textY)), sbScale, sbScale, drawFlags, text, nameTextColor);
	curX += colW[COL_NAME];
	
	// Mod name
	if (colW[COL_MOD] > 0) {
		modName = server->gameName ? server->gameName : "baseq2";
		availPx = colW[COL_MOD] - 6;
		maxVisible = (m_serverBrowser.charW > 0) ? (int)(availPx / m_serverBrowser.charW) : 0;
		ServerBrowser_CopyTruncateColor(modName, text, sizeof(text), maxVisible);
		if (selected)
			cgi.R_DrawString(NULL, (float)((int)(curX + 3)), (float)((int)(textY + 1)), sbScale, sbScale, 0, text, shadowColor);
		cgi.R_DrawString(NULL, (float)((int)(curX + 2)), (float)((int)(textY)), sbScale, sbScale, drawFlags, text, metaTextColor);
		curX += colW[COL_MOD];
	}
	
	// Map name
	if (colW[COL_MAP] > 0) {
		mapName = server->mapName ? server->mapName : "---";
		availPx = colW[COL_MAP] - 6;
		maxVisible = (m_serverBrowser.charW > 0) ? (int)(availPx / m_serverBrowser.charW) : 0;
		ServerBrowser_CopyTruncateColor(mapName, text, sizeof(text), maxVisible);
		if (selected)
			cgi.R_DrawString(NULL, (float)((int)(curX + 3)), (float)((int)(textY + 1)), sbScale, sbScale, 0, text, shadowColor);
		cgi.R_DrawString(NULL, (float)((int)(curX + 2)), (float)((int)(textY)), sbScale, sbScale, drawFlags, text, mapTextColor);
		curX += colW[COL_MAP];
	}
	
	// Players
	if (server->valid) {
		Q_snprintfz(text, sizeof(text), "%d/%d", server->numPlayers, server->maxPlayers);
	} else {
		Q_strncpyz(text, "---", sizeof(text));
	}
	// R_DrawString returns character count, not pixel width.
	strW = SB_TextWidthPx_FromGlyphCount(cgi.R_DrawString(NULL, 0, 0, sbScale, sbScale, 0, text, metaTextColor), m_serverBrowser.charW);
	drawX = SB_RightAlignTextX(curX, colW[COL_PLAYERS], 2.0f, strW, 2.0f);
	if (selected)
		cgi.R_DrawString(NULL, (float)((int)(drawX + 1)), (float)((int)(textY + 1)), sbScale, sbScale, 0, text, shadowColor);
	cgi.R_DrawString(NULL, (float)((int)(drawX)), (float)((int)(textY)), sbScale, sbScale, drawFlags, text, metaTextColor);
	curX += colW[COL_PLAYERS];
	
	// Ping
	if (server->valid) {
		Q_snprintfz(text, sizeof(text), "%d", server->ping);
	} else {
		Q_strncpyz(text, "---", sizeof(text));
	}
	strW = SB_TextWidthPx_FromGlyphCount(cgi.R_DrawString(NULL, 0, 0, sbScale, sbScale, 0, text, pingTextColor), m_serverBrowser.charW);
	drawX = SB_RightAlignTextX(curX, colW[COL_PING], 2.0f, strW, 2.0f);
	if (selected)
		cgi.R_DrawString(NULL, (float)((int)(drawX + 1)), (float)((int)(textY + 1)), sbScale, sbScale, 0, text, shadowColor);
	cgi.R_DrawString(NULL, (float)((int)(drawX)), (float)((int)(textY)), sbScale, sbScale, drawFlags, text, pingTextColor);
}

/*
=================
ServerBrowser_DrawList
=================
*/
static void ServerBrowser_DrawList(void)
{
	float x, y, w, h;
	float curY;
	int i;
	qBool selected;
	int visibleCount = 0;
	int maxVisible;
	vec4_t dividerColor;
	
	x = m_serverBrowser.listX;
	y = m_serverBrowser.listY;
	w = m_serverBrowser.listW;
	h = m_serverBrowser.listH;

	// Draw background
	cgi.R_DrawFill(x, y, w, h, m_serverBrowser.colorBackground);
	
	// Draw header
	curY = y;
	ServerBrowser_DrawHeader(x, curY, w);
	curY += m_serverBrowser.headerH;
	maxVisible = ServerBrowser_MaxVisibleServerRows();
	
	// Draw server rows
	for (i = m_serverBrowser.scrollOffset; i < g_numServers && visibleCount < maxVisible; i++) {
		if (!ServerBrowser_MatchesFilter(i))
			continue;

		selected = (i == m_serverBrowser.selectedServer);
		ServerBrowser_DrawServerRow(x, curY, w, i, selected);
		
		curY += m_serverBrowser.rowH;
		visibleCount++;
	}
	
	// Draw border
	// Slightly de-emphasize the list/right-panel divider specifically.
	Vec4Copy(m_serverBrowser.colorBorder, dividerColor);
	dividerColor[1] *= 0.88f;
	// Top
	cgi.R_DrawFill(x, y, w, 1, m_serverBrowser.colorBorder);
	// Bottom
	cgi.R_DrawFill(x, y + h - 1, w, 1, m_serverBrowser.colorBorder);
	// Left
	cgi.R_DrawFill(x, y, 1, h, m_serverBrowser.colorBorder);
	// Right
	cgi.R_DrawFill(x + w - 1, y, 1, h, dividerColor);
	
	// Update visible count
	m_serverBrowser.numVisibleServers = visibleCount;
}

/*
=================
ServerBrowser_DrawInfoPanel
=================
*/
static void ServerBrowser_DrawInfoPanel(void)
{
	cgServerSlot_t *server;
	float x, y, w, h;
	float curY;
	float sbScale;
	char text[256];
	char keys[SB_INFO_MAX_PAIRS][SB_INFO_KEY_MAX];
	char values[SB_INFO_MAX_PAIRS][SB_INFO_VAL_MAX];
	int numPairs;
	int totalLines;
	int visibleLines;
	int maxScroll;
	int lineIndex;
	int row;
	float keyColW;
	float keyX, valX;
	float textY;
	float availH;
	float dividerX;
	int keyChars;
	int maxKeyChars;
	int maxValChars;
	vec4_t keyColor;
	vec4_t valColor;
	vec4_t dividerColor;
	vec4_t scrollTrack;
	vec4_t scrollThumb;
	float trackX, trackY, trackW, trackH;
	float thumbH, thumbY;
	static int lastSelected = -9999;
	
	if (!m_serverBrowser.showInfoPanel)
		return;
	
	if (m_serverBrowser.selectedServer < 0)
		return;
	
	server = ServerBrowser_GetServer(m_serverBrowser.selectedServer);
	if (!server || !server->valid)
		return;

	// Reset scroll when selection changes
	if (m_serverBrowser.selectedServer != lastSelected) {
		m_serverBrowser.infoScroll = 0;
		lastSelected = m_serverBrowser.selectedServer;
	}
	
	x = m_serverBrowser.infoX;
	y = m_serverBrowser.infoY;
	w = m_serverBrowser.infoW;
	h = m_serverBrowser.infoH;

	// Match the main list: scale is based on an 8px font cell so UI_SCALE is preserved.
	sbScale = SB_ScaleFromCharCell8(m_serverBrowser.charW, UIFT_SCALE);
	
	// Draw background
	cgi.R_DrawFill(x, y, w, h, m_serverBrowser.colorBackground);
	
	// Draw border
	Vec4Copy(m_serverBrowser.colorBorder, dividerColor);
	dividerColor[1] *= 0.88f;
	cgi.R_DrawFill(x, y, w, 1, m_serverBrowser.colorBorder);
	cgi.R_DrawFill(x, y + h - 1, w, 1, m_serverBrowser.colorBorder);
	cgi.R_DrawFill(x, y, 1, h, dividerColor);
	cgi.R_DrawFill(x + w - 1, y, 1, h, m_serverBrowser.colorBorder);
	
	curY = y + 5;
	
	// Title
	cgi.R_DrawString(NULL, (float)((int)(x + 5)), (float)((int)(curY)), sbScale, sbScale, 0, "SERVER INFO", m_serverBrowser.colorHeader);
	curY += m_serverBrowser.charH + 5;

	// Column header (Q2Pro-style)
	{
		const float contentX = x + 5.0f;
		const float contentW = w - 10.0f;
		const float gapPx = 10.0f;
		const int minKeyChars = 10;
		const int maxKeyCharsClamp = 24;

		keyColW = contentW * 0.40f;
		// Snap the key/value split to whole character cells (and clamp it) so high
		// resolutions don't push the Value column off into empty space.
		if (m_serverBrowser.charW > 0.0f) {
			keyChars = (int)((keyColW - 6.0f) / m_serverBrowser.charW);
			if (keyChars < minKeyChars)
				keyChars = minKeyChars;
			if (keyChars > maxKeyCharsClamp)
				keyChars = maxKeyCharsClamp;
			keyColW = (float)keyChars * m_serverBrowser.charW;
		}

		keyX = (float)((int)(contentX));
		dividerX = (float)((int)(contentX + keyColW + 4.0f));
		valX = (float)((int)(contentX + keyColW + gapPx));
	}
	Vec4Set(keyColor, 0.75f, 0.95f, 0.75f, 1.0f);
	Vec4Copy(Q_colorWhite, valColor);
	Vec4Copy(m_serverBrowser.colorBorder, dividerColor);
	dividerColor[1] *= 0.88f;

	cgi.R_DrawString(NULL, keyX, (float)((int)(curY)), sbScale, sbScale, 0, "Key", keyColor);
	cgi.R_DrawString(NULL, valX, (float)((int)(curY)), sbScale, sbScale, 0, "Value", keyColor);
	curY += m_serverBrowser.charH + 2;
	cgi.R_DrawFill(x + 2, curY - 2, w - 4, 1, dividerColor);

	// Parse pairs from the raw info string (first line of the status response)
	numPairs = ServerBrowser_ParseInfoPairs(server->infoString ? server->infoString : "", keys, values, SB_INFO_MAX_PAIRS);
	// Always include address at top
	totalLines = 1 + numPairs;

	// Visible lines based on panel height
	availH = (y + h - 5) - curY;
	visibleLines = (m_serverBrowser.charH > 0) ? (int)(availH / m_serverBrowser.charH) : 0;
	if (visibleLines < 1)
		return;
	if (visibleLines > SB_INFO_MAX_PAIRS + 1)
		visibleLines = SB_INFO_MAX_PAIRS + 1;

	maxScroll = totalLines - visibleLines;
	if (maxScroll < 0)
		maxScroll = 0;
	if (m_serverBrowser.infoScroll < 0)
		m_serverBrowser.infoScroll = 0;
	if (m_serverBrowser.infoScroll > maxScroll)
		m_serverBrowser.infoScroll = maxScroll;

	// Column divider
	cgi.R_DrawFill(dividerX, curY - 2, 1, (float)((int)((y + h - 5) - (curY - 2))), dividerColor);

	maxKeyChars = SB_InfoPanelMaxKeyChars(keyColW, m_serverBrowser.charW);
	maxValChars = SB_InfoPanelMaxValueChars(w - 10.0f, keyColW + 10.0f, m_serverBrowser.charW, (maxScroll > 0));

	row = 0;
	lineIndex = m_serverBrowser.infoScroll;
	while (row < visibleLines && lineIndex < totalLines) {
		textY = curY + (row * m_serverBrowser.charH);

		if (lineIndex == 0) {
			ServerBrowser_CopyTruncateColor("Address", text, sizeof(text), maxKeyChars);
			cgi.R_DrawString(NULL, keyX, (float)((int)(textY)), sbScale, sbScale, 0, text, keyColor);
			ServerBrowser_CopyTruncateColor(server->address, text, sizeof(text), maxValChars);
			cgi.R_DrawString(NULL, valX, (float)((int)(textY)), sbScale, sbScale, 0, text, valColor);
		} else {
			ServerBrowser_CopyTruncateColor(keys[lineIndex - 1], text, sizeof(text), maxKeyChars);
			cgi.R_DrawString(NULL, keyX, (float)((int)(textY)), sbScale, sbScale, 0, text, keyColor);
			ServerBrowser_CopyTruncateColor(values[lineIndex - 1], text, sizeof(text), maxValChars);
			cgi.R_DrawString(NULL, valX, (float)((int)(textY)), sbScale, sbScale, 0, text, valColor);
		}

		row++;
		lineIndex++;
	}

	// Optional scrollbar if needed
	if (maxScroll > 0) {
		trackW = 5;
		trackX = x + w - 7;
		trackY = curY - 2;
		trackH = (float)((int)((y + h - 5) - trackY));
		Vec4Set(scrollTrack, 0.0f, 0.18f, 0.0f, 0.90f);
		Vec4Set(scrollThumb, 0.70f, 1.0f, 0.70f, 0.70f);
		cgi.R_DrawFill(trackX, trackY, trackW, trackH, scrollTrack);
		thumbH = trackH * ((float)visibleLines / (float)totalLines);
		if (thumbH < 10)
			thumbH = 10;
		thumbY = trackY + (trackH - thumbH) * ((float)m_serverBrowser.infoScroll / (float)maxScroll);
		cgi.R_DrawFill(trackX, thumbY, trackW, thumbH, scrollThumb);
	}
}

/*
=================
ServerBrowser_DrawPlayersPanel
=================
*/
static void ServerBrowser_DrawPlayersPanel(void)
{
	cgServerSlot_t *server;
	float x, y, w, h;
	float curY;
	float sbScale;
	vec4_t dividerColor;
	float textX;
	
	if (!m_serverBrowser.showPlayersPanel)
		return;
	
	if (m_serverBrowser.selectedServer < 0)
		return;
	
	server = ServerBrowser_GetServer(m_serverBrowser.selectedServer);
	if (!server || !server->valid)
		return;
	
	x = m_serverBrowser.playersX;
	y = m_serverBrowser.playersY;
	w = m_serverBrowser.playersW;
	h = m_serverBrowser.playersH;

	// Match the main list: scale is based on an 8px font cell so UI_SCALE is preserved.
	sbScale = SB_ScaleFromCharCell8(m_serverBrowser.charW, UIFT_SCALE);
	
	// Draw background
	cgi.R_DrawFill(x, y, w, h, m_serverBrowser.colorBackground);
	
	// Draw border
	Vec4Copy(m_serverBrowser.colorBorder, dividerColor);
	dividerColor[1] *= 0.88f;
	cgi.R_DrawFill(x, y, w, 1, m_serverBrowser.colorBorder);
	cgi.R_DrawFill(x, y + h - 1, w, 1, m_serverBrowser.colorBorder);
	cgi.R_DrawFill(x, y, 1, h, dividerColor);
	cgi.R_DrawFill(x + w - 1, y, 1, h, m_serverBrowser.colorBorder);
	
	curY = y + 5;
	textX = (float)((int)(x + 5));
	
	// Title
	cgi.R_DrawString(NULL, textX, (float)((int)(curY)), sbScale, sbScale, 0, "PLAYERS", m_serverBrowser.colorHeader);
	curY += m_serverBrowser.charH + 5;
	
	// Player list
	if (server->numPlayers > 0) {
		int i, visiblePlayers = 0;
		char text[128];
		
		// Header
		cgi.R_DrawString(NULL, textX, (float)((int)(curY)), sbScale, sbScale, 0, "Score  Ping  Name", Q_colorWhite);
		curY += m_serverBrowser.charH + 2;
		
		// Draw players
		for (i = 0; i < server->numPlayers && visiblePlayers < SB_MAX_VISIBLE_PLAYERS; i++) {
			Q_snprintfz(text, sizeof(text), "%5d  %4d  %s", 
				server->playerScores[i], 
				server->playerPings[i], 
				server->players[i]);
			cgi.R_DrawString(NULL, textX, (float)((int)(curY)), sbScale, sbScale, 0, text, Q_colorWhite);
			curY += m_serverBrowser.charH;
			visiblePlayers++;
		}
	} else {
		cgi.R_DrawString(NULL, textX, (float)((int)(curY)), sbScale, sbScale, 0, "No players", Q_colorWhite);
	}
}

/*
=================
ServerBrowser_DrawStatusBar
=================
*/
static void ServerBrowser_DrawStatusBar(float y)
{
	float x = 10;
	float w = cg.refConfig.vidWidth - 20;
	float h = m_serverBrowser.charH + 4;
	float rightW;
	vec4_t rightColor;
	
	// Draw background
	cgi.R_DrawFill(x, y, w, h, m_serverBrowser.colorBackground);
	cgi.R_DrawFill(x, y, w, 1, m_serverBrowser.colorBorder);
	
	// Left status
	cgi.R_DrawString(NULL, x + 5, y + 2, UIFT_SCALE, UIFT_SCALE, 0, m_serverBrowser.statusLeft, Q_colorWhite);

	// Right status should be useful but not compete with the list/header.
	if (m_serverBrowser.filterActive)
		Vec4Copy(Q_colorWhite, rightColor);
	else
		Vec4Set(rightColor, 0.72f, 0.92f, 0.72f, 1.0f);
	
	// Right status
	// R_DrawString returns character count, not pixel width.
	rightW = (float)cgi.R_DrawString(NULL, 0, 0, UIFT_SCALE, UIFT_SCALE, 0, m_serverBrowser.statusRight, rightColor) * UIFT_SIZE;
	cgi.R_DrawString(NULL, x + w - rightW - 5, y + 2, UIFT_SCALE, UIFT_SCALE, 0, m_serverBrowser.statusRight, rightColor);
}

/*
=================
ServerBrowser_DrawFilterBar
=================
*/
static void ServerBrowser_DrawFilterBar(float y)
{
	float x = m_serverBrowser.listX;
	float w = m_serverBrowser.listW;
	float h = m_serverBrowser.charH + 4;
	char text[128];
	
	if (!m_serverBrowser.filterActive)
		return;
	
	// Draw background
	cgi.R_DrawFill(x, y, w, h, m_serverBrowser.colorSelected);
	
	// Draw filter text
	Q_snprintfz(text, sizeof(text), "Filter: %s_", m_serverBrowser.filterText);
	cgi.R_DrawString(NULL, x + 5, y + 2, UIFT_SCALE, UIFT_SCALE, 0, text, Q_colorBlack);
}

static void ServerBrowser_UpdateLayout(void)
{
	float screenW, screenH;
	float marginX, marginY;
	float panelGap;
	float fontScale;
	float rightPanelX, rightPanelW, rightPanelH;
	float listFrac;
	float statusY;
	int layoutMode;

	// Get screen dimensions
	screenW = cg.refConfig.vidWidth;
	screenH = cg.refConfig.vidHeight;

	ServerBrowser_LayoutEnsureCvars();

	// Font/grid sizing
	fontScale = cgi.Cvar_GetFloatValue("sb_fontscale");
	if (fontScale <= 0.0f)
		fontScale = 1.0f;
	if (fontScale < 0.5f)
		fontScale = 0.5f;
	if (fontScale > 1.5f)
		fontScale = 1.5f;

	m_serverBrowser.charW = UIFT_SIZE * fontScale;
	m_serverBrowser.charH = UIFT_SIZE * fontScale;
	m_serverBrowser.headerH = m_serverBrowser.charH + (3 * UI_SCALE);
	m_serverBrowser.rowH = m_serverBrowser.charH + (1 * UI_SCALE);

	marginX = 10;
	marginY = 10;
	panelGap = 10;

	layoutMode = cgi.Cvar_GetIntegerValue("sb_layout");
	listFrac = cgi.Cvar_GetFloatValue("sb_listfrac");
	if (listFrac <= 0.0f)
		listFrac = 0.6f;
	if (listFrac < 0.50f)
		listFrac = 0.50f;
	if (listFrac > 0.90f)
		listFrac = 0.90f;

	// Main server list
	m_serverBrowser.listX = marginX;
	m_serverBrowser.listY = marginY + 20;  // Leave room for title
	statusY = screenH - marginY - m_serverBrowser.charH - 4;
	// Use the available vertical space down to just above the status bar.
	m_serverBrowser.listH = (statusY - 8) - m_serverBrowser.listY;
	if (m_serverBrowser.listH < (m_serverBrowser.headerH + m_serverBrowser.rowH))
		m_serverBrowser.listH = (m_serverBrowser.headerH + m_serverBrowser.rowH);

	// Layout modes:
	//   sb_layout 0: split view, list width is sb_listfrac (default 0.6)
	//   sb_layout 1+: wide/Q2Pro-like list-only
	if (layoutMode != 0) {
		m_serverBrowser.showInfoPanel = qFalse;
		m_serverBrowser.showPlayersPanel = qFalse;
	}

	if (!m_serverBrowser.showInfoPanel && !m_serverBrowser.showPlayersPanel) {
		m_serverBrowser.listW = screenW - (marginX * 2);
	} else {
		m_serverBrowser.listW = screenW * listFrac;
	}

	// Info and players panels (right side)
	rightPanelX = m_serverBrowser.listX + m_serverBrowser.listW + panelGap;
	rightPanelW = screenW - rightPanelX - marginX;
	if (rightPanelW < 0)
		rightPanelW = 0;

	if (m_serverBrowser.showInfoPanel && m_serverBrowser.showPlayersPanel) {
		rightPanelH = (m_serverBrowser.listH - panelGap) / 2;
	} else {
		rightPanelH = m_serverBrowser.listH;
	}

	m_serverBrowser.infoX = rightPanelX;
	m_serverBrowser.infoY = m_serverBrowser.listY;
	m_serverBrowser.infoW = m_serverBrowser.showInfoPanel ? rightPanelW : 0;
	m_serverBrowser.infoH = m_serverBrowser.showInfoPanel ? rightPanelH : 0;

	m_serverBrowser.playersX = rightPanelX;
	if (m_serverBrowser.showInfoPanel && m_serverBrowser.showPlayersPanel)
		m_serverBrowser.playersY = m_serverBrowser.infoY + m_serverBrowser.infoH + panelGap;
	else
		m_serverBrowser.playersY = m_serverBrowser.listY;
	m_serverBrowser.playersW = m_serverBrowser.showPlayersPanel ? rightPanelW : 0;
	m_serverBrowser.playersH = m_serverBrowser.showPlayersPanel ? rightPanelH : 0;
}

/*
=================
ServerBrowser_Draw
=================
*/
static void ServerBrowser_Draw(void)
{
	float screenW, screenH;
	float marginY;
	static int lastDbgPrintTime = 0;

	if (cgi.Cvar_GetIntegerValue("sb_debug") >= 2) {
		if (!lastDbgPrintTime || cg.realTime - lastDbgPrintTime > 1000) {
			Com_Printf(0, "^2ServerBrowser_Draw: %d server slots\n", g_numServers);
			lastDbgPrintTime = cg.realTime;
		}
	}
	
	if (!m_serverBrowser.frameWork.initialized)
		ServerBrowser_Init();

	// Keep render and input consistent: apply deferred sorts *before* drawing.
	ServerBrowser_MaybeApplyDeferredSort();

	ServerBrowser_UpdateLayout();
	screenW = cg.refConfig.vidWidth;
	screenH = cg.refConfig.vidHeight;
	marginY = 10;
	
	// Draw title
	cgi.R_DrawString(NULL, screenW / 2 - 150, marginY, UIFT_SCALEMED, UIFT_SCALEMED, 0, "JOIN ONLINE SERVERS", m_serverBrowser.colorHeader);
	
	// Draw filter bar if active
	if (m_serverBrowser.filterActive) {
		ServerBrowser_DrawFilterBar(m_serverBrowser.listY - m_serverBrowser.charH - 8);
	}
	
	// Draw main components
	ServerBrowser_DrawList();
	ServerBrowser_DrawInfoPanel();
	ServerBrowser_DrawPlayersPanel();
	
	// Draw status bar
	ServerBrowser_DrawStatusBar(screenH - marginY - m_serverBrowser.charH - 4);
	
	// Update status
	ServerBrowser_UpdateStatus();
	
	// Check if refresh completed
	if (m_serverBrowser.isRefreshing) {
		const unsigned quietPeriodMs = 1500;
		const unsigned hardTimeoutMs = 8000;
		unsigned now = cgi.Sys_Milliseconds();
		unsigned elapsedTotal = now - m_serverBrowser.refreshStartTime;
		unsigned elapsedSinceActivity = now - m_serverBrowser.refreshLastActivityTime;

		if (g_numServers != m_serverBrowser.lastServerCount) {
			m_serverBrowser.lastServerCount = g_numServers;
			m_serverBrowser.refreshLastActivityTime = now;
			elapsedSinceActivity = 0;
		}

		// Only apply quiet-period completion after we've seen activity beyond the start tick.
		if (elapsedTotal >= hardTimeoutMs ||
			(m_serverBrowser.refreshLastActivityTime > m_serverBrowser.refreshStartTime && elapsedSinceActivity >= quietPeriodMs)) {
			m_serverBrowser.isRefreshing = qFalse;
		}
	}
}

/*
=================
ServerBrowser_Key
=================
*/
static struct sfx_s *ServerBrowser_Key(uiFrameWork_t *fw, keyNum_t keyNum)
{
	int i, visibleIndex;
	float colW[COL_MAX];
	float mx, my;
	int wheelSteps;
	int step;
	int key;
	
	if (!m_serverBrowser.frameWork.initialized)
		ServerBrowser_Init();

	// Avoid input selecting/connecting against an array that will be resorted next frame.
	ServerBrowser_MaybeApplyDeferredSort();
	// Keep mouse hit-testing consistent even if layout changed since last draw.
	ServerBrowser_UpdateLayout();
	
	// Mouse wheel scrolling (Q2Pro-style): only when cursor is over the list
	if (keyNum == K_MWHEELUP || keyNum == K_MWHEELDOWN) {
		mx = uiState.cursorX;
		my = uiState.cursorY;

		if (mx >= m_serverBrowser.listX && mx <= (m_serverBrowser.listX + m_serverBrowser.listW) &&
			my >= (m_serverBrowser.listY + m_serverBrowser.headerH) &&
			my <= (m_serverBrowser.listY + m_serverBrowser.listH)) {
			wheelSteps = 3;

			if (keyNum == K_MWHEELUP) {
				for (step = 0; step < wheelSteps; step++) {
					if (m_serverBrowser.selectedServer > 0) {
						do {
							m_serverBrowser.selectedServer--;
						} while (m_serverBrowser.selectedServer > 0 &&
								 !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer));

						if (m_serverBrowser.selectedServer < m_serverBrowser.scrollOffset)
							m_serverBrowser.scrollOffset = m_serverBrowser.selectedServer;
					}
				}
				return uiMedia.sounds.menuMove;
			}
			else {
				for (step = 0; step < wheelSteps; step++) {
					if (m_serverBrowser.selectedServer < g_numServers - 1) {
						do {
							m_serverBrowser.selectedServer++;
						} while (m_serverBrowser.selectedServer < g_numServers - 1 &&
								 !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer));

						visibleIndex = 0;
						for (i = m_serverBrowser.scrollOffset; i <= m_serverBrowser.selectedServer && i < g_numServers; i++) {
							if (ServerBrowser_MatchesFilter(i))
								visibleIndex++;
						}
						if (visibleIndex > ServerBrowser_MaxVisibleServerRows())
							m_serverBrowser.scrollOffset++;
					}
				}
				return uiMedia.sounds.menuMove;
			}
		}
			// Info panel scroll (when hovering the info panel)
			if (m_serverBrowser.showInfoPanel &&
				mx >= m_serverBrowser.infoX && mx <= (m_serverBrowser.infoX + m_serverBrowser.infoW) &&
				my >= m_serverBrowser.infoY && my <= (m_serverBrowser.infoY + m_serverBrowser.infoH)) {
				wheelSteps = 3;
				if (keyNum == K_MWHEELUP)
					m_serverBrowser.infoScroll -= wheelSteps;
				else
					m_serverBrowser.infoScroll += wheelSteps;
				return uiMedia.sounds.menuMove;
			}

		return NULL;
	}

	// Filter mode handling
	if (m_serverBrowser.filterActive) {
		if (keyNum == K_ESCAPE) {
			m_serverBrowser.filterActive = qFalse;
			m_serverBrowser.filterText[0] = '\0';
			m_serverBrowser.filterCursor = 0;
			return uiMedia.sounds.menuOut;
		} else if (keyNum == K_ENTER || keyNum == K_KP_ENTER) {
			m_serverBrowser.filterActive = qFalse;
			// Apply filter by reselecting first visible server
			for (i = 0; i < g_numServers; i++) {
				if (ServerBrowser_MatchesFilter(i)) {
					m_serverBrowser.selectedServer = i;
					m_serverBrowser.scrollOffset = 0;
					break;
				}
			}
			return uiMedia.sounds.menuIn;
		} else if (keyNum == K_BACKSPACE) {
			if (m_serverBrowser.filterCursor > 0) {
				m_serverBrowser.filterText[--m_serverBrowser.filterCursor] = '\0';
			}
			return NULL;
		} else if (keyNum >= 32 && keyNum < 127) {
			// Printable character
			if (m_serverBrowser.filterCursor < SB_FILTER_MAXLEN - 1) {
				m_serverBrowser.filterText[m_serverBrowser.filterCursor++] = keyNum;
				m_serverBrowser.filterText[m_serverBrowser.filterCursor] = '\0';
			}
			return NULL;
		}
		return NULL;
	}
	
	// Normal mode handling
	key = (int)keyNum;
	switch (key) {
	case K_ESCAPE:
		M_PopMenu();
		return uiMedia.sounds.menuOut;

	case K_MOUSE1:
		mx = uiState.cursorX;
		my = uiState.cursorY;

		// Header click -> sort
		if (mx >= m_serverBrowser.listX && mx <= (m_serverBrowser.listX + m_serverBrowser.listW) &&
			my >= m_serverBrowser.listY && my <= (m_serverBrowser.listY + m_serverBrowser.headerH)) {
			int col;
			ServerBrowser_ComputeColWidths(m_serverBrowser.listW, colW);
			col = SB_HeaderHitTestColumn(m_serverBrowser.listX, colW, COL_MAX, mx);
			if (col >= 0) {
				ServerBrowser_SetSortColumn(col);
				ServerBrowser_Sort();
				m_serverBrowser.sortDirty = qFalse;
				m_serverBrowser.lastSortTime = cg.realTime;
				return uiMedia.sounds.menuMove;
			}
			return NULL;
		}

		// Row click -> select (and toggle favorite if clicking favorite column)
		if (mx >= m_serverBrowser.listX && mx <= (m_serverBrowser.listX + m_serverBrowser.listW) &&
			my >= (m_serverBrowser.listY + m_serverBrowser.headerH) &&
			my <= (m_serverBrowser.listY + m_serverBrowser.listH)) {
			int row = (int)((my - (m_serverBrowser.listY + m_serverBrowser.headerH)) / m_serverBrowser.rowH);
			int serverIndex = -1;
			int rowCount = 0;
			int scan;
			float curX;

			if (row < 0)
				return NULL;

			{
				int maxVisible = ServerBrowser_MaxVisibleServerRows();
				for (scan = m_serverBrowser.scrollOffset; scan < g_numServers && rowCount < maxVisible; scan++) {
				if (!ServerBrowser_MatchesFilter(scan))
					continue;
				if (rowCount == row) {
					serverIndex = scan;
					break;
				}
				rowCount++;
				}
			}

			if (serverIndex >= 0) {
				// Click favorite column toggles favorite
				ServerBrowser_ComputeColWidths(m_serverBrowser.listW, colW);
				curX = m_serverBrowser.listX;
				if (mx >= curX && mx < (curX + colW[COL_FAVORITE])) {
					m_serverBrowser.selectedServer = serverIndex;
					ServerBrowser_ToggleFavorite();
					return uiMedia.sounds.menuMove;
				}

				// Double-click connects
				if (serverIndex == g_sbLastClickServer && (cg.realTime - g_sbLastClickTime) < 350) {
					m_serverBrowser.selectedServer = serverIndex;
					ServerBrowser_Connect();
					g_sbLastClickTime = 0;
					g_sbLastClickServer = -1;
					return uiMedia.sounds.menuIn;
				}

				// Select server
				m_serverBrowser.selectedServer = serverIndex;
				m_serverBrowser.infoScroll = 0;
				ServerBrowser_SortEnsureSelectionVisible();
				g_sbLastClickTime = cg.realTime;
				g_sbLastClickServer = serverIndex;
				return uiMedia.sounds.menuMove;
			}
		}
		return NULL;

	case K_MOUSE2:
		// Right-click toggles favorite on the selected row (if any)
		if (m_serverBrowser.selectedServer >= 0) {
			ServerBrowser_ToggleFavorite();
			return uiMedia.sounds.menuMove;
		}
		return NULL;
	
	case K_UPARROW:
	case K_KP_UPARROW:
		// Move selection up
		if (m_serverBrowser.selectedServer > 0) {
			do {
				m_serverBrowser.selectedServer--;
			} while (m_serverBrowser.selectedServer > 0 && 
					 !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer));
			m_serverBrowser.infoScroll = 0;
			
			// Scroll if needed
			if (m_serverBrowser.selectedServer < m_serverBrowser.scrollOffset) {
				m_serverBrowser.scrollOffset = m_serverBrowser.selectedServer;
			}
		}
		return uiMedia.sounds.menuMove;
	
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		// Move selection down
		if (m_serverBrowser.selectedServer < g_numServers - 1) {
			do {
				m_serverBrowser.selectedServer++;
			} while (m_serverBrowser.selectedServer < g_numServers - 1 && 
					 !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer));
			m_serverBrowser.infoScroll = 0;
			
			// Scroll if needed
			visibleIndex = 0;
			for (i = m_serverBrowser.scrollOffset; i <= m_serverBrowser.selectedServer; i++) {
				if (ServerBrowser_MatchesFilter(i))
					visibleIndex++;
			}
			if (visibleIndex > ServerBrowser_MaxVisibleServerRows()) {
				m_serverBrowser.scrollOffset++;
			}
		}
		return uiMedia.sounds.menuMove;
	
	case K_PGUP:
	case K_KP_PGUP:
		// Page up
		for (i = 0; i < ServerBrowser_MaxVisibleServerRows() && m_serverBrowser.selectedServer > 0; i++) {
			m_serverBrowser.selectedServer--;
			while (m_serverBrowser.selectedServer > 0 && 
				   !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer)) {
				m_serverBrowser.selectedServer--;
			}
		}
		m_serverBrowser.scrollOffset = m_serverBrowser.selectedServer;
		m_serverBrowser.infoScroll = 0;
		return uiMedia.sounds.menuMove;
	
	case K_PGDN:
	case K_KP_PGDN:
		// Page down
		for (i = 0; i < ServerBrowser_MaxVisibleServerRows() && m_serverBrowser.selectedServer < g_numServers - 1; i++) {
			m_serverBrowser.selectedServer++;
			while (m_serverBrowser.selectedServer < g_numServers - 1 && 
				   !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer)) {
				m_serverBrowser.selectedServer++;
			}
		}
		m_serverBrowser.infoScroll = 0;
		ServerBrowser_SortEnsureSelectionVisible();
		return uiMedia.sounds.menuMove;
	
	case K_HOME:
	case K_KP_HOME:
		// First server
		m_serverBrowser.selectedServer = 0;
		m_serverBrowser.scrollOffset = 0;
		m_serverBrowser.infoScroll = 0;
		while (m_serverBrowser.selectedServer < g_numServers - 1 && 
			   !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer)) {
			m_serverBrowser.selectedServer++;
		}
		return uiMedia.sounds.menuMove;
	
	case K_END:
	case K_KP_END:
		// Last server
		m_serverBrowser.selectedServer = g_numServers - 1;
		m_serverBrowser.infoScroll = 0;
		while (m_serverBrowser.selectedServer > 0 &&
			   !ServerBrowser_MatchesFilter(m_serverBrowser.selectedServer)) {
			m_serverBrowser.selectedServer--;
		}
		m_serverBrowser.scrollOffset = m_serverBrowser.selectedServer;
		return uiMedia.sounds.menuMove;
	
	case K_ENTER:
	case K_KP_ENTER:
		// Connect to server
		ServerBrowser_Connect();
		return uiMedia.sounds.menuIn;
	
	case K_SPACE:
		// Refresh
		ServerBrowser_Refresh();
		return uiMedia.sounds.menuIn;
	
	case 'f':
	case 'F':
		// Toggle favorite
		ServerBrowser_ToggleFavorite();
		return uiMedia.sounds.menuMove;
	
	case 'r':
	case 'R':
		// Refresh selected server only
		// TODO: Implement single server refresh
		return uiMedia.sounds.menuIn;
	
	case '/':
		// Activate filter
		m_serverBrowser.filterActive = qTrue;
		return uiMedia.sounds.menuIn;
	
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
		// Sort by column
		ServerBrowser_SetSortColumn(key - '1');
		ServerBrowser_Sort();
		m_serverBrowser.sortDirty = qFalse;
		m_serverBrowser.lastSortTime = cg.realTime;
		return uiMedia.sounds.menuMove;
	
	case K_TAB:
		// Toggle panels, or if in wide mode, return to split view.
		ServerBrowser_LayoutEnsureCvars();
		if (cgi.Cvar_GetIntegerValue("sb_layout") != 0) {
			cgi.Cvar_Set("sb_layout", "0", qTrue);
			m_serverBrowser.showInfoPanel = qTrue;
			m_serverBrowser.showPlayersPanel = qTrue;
			m_serverBrowser.savedShowInfoPanel = qTrue;
			m_serverBrowser.savedShowPlayersPanel = qTrue;
			m_serverBrowser.lastLayoutMode = 0;
		} else {
			m_serverBrowser.showInfoPanel = !m_serverBrowser.showInfoPanel;
			m_serverBrowser.showPlayersPanel = !m_serverBrowser.showPlayersPanel;
			m_serverBrowser.savedShowInfoPanel = m_serverBrowser.showInfoPanel;
			m_serverBrowser.savedShowPlayersPanel = m_serverBrowser.showPlayersPanel;
		}
		return uiMedia.sounds.menuMove;
	}
	
	return NULL;
}

/*
=================
ServerBrowser_Close
=================
*/
static struct sfx_s *ServerBrowser_Close(void)
{
	return uiMedia.sounds.menuOut;
}

/*
=================
ServerBrowser_Sort
=================
*/
static void ServerBrowser_Sort(void)
{
	char selectedAddr[64];
	qBool haveSelected = qFalse;
	int i;
	const cgServerSlot_t *selected;

	if (g_numServers <= 1)
		return;

	if (m_serverBrowser.selectedServer >= 0 && m_serverBrowser.selectedServer < g_numServers) {
		selected = &g_servers[m_serverBrowser.selectedServer];
		Q_strncpyz(selectedAddr, selected->address, sizeof(selectedAddr));
		haveSelected = (selectedAddr[0] != '\0');
	}

	qsort(g_servers, g_numServers, sizeof(g_servers[0]), ServerBrowser_ServerSlotSortCmp);

	if (haveSelected) {
		for (i = 0; i < g_numServers; i++) {
			if (!strcmp(g_servers[i].address, selectedAddr)) {
				m_serverBrowser.selectedServer = i;
				break;
			}
		}
	}

	ServerBrowser_SortEnsureSelectionVisible();
}

static void ServerBrowser_MaybeApplyDeferredSort(void)
{
	if (SB_ShouldApplyDeferredSort(m_serverBrowser.sortDirty, cg.realTime, m_serverBrowser.lastSortTime, 200)) {
		ServerBrowser_Sort();
		m_serverBrowser.lastSortTime = cg.realTime;
		m_serverBrowser.sortDirty = qFalse;
	}
}

/*
=================
UI_ServerBrowserMenu_f
=Com_Printf(0, "^2UI_ServerBrowserMenu_f: Opening server browser\n");
	================
*/
void UI_ServerBrowserMenu_f(void)
{
	ServerBrowser_Init();
	M_PushMenu(&m_serverBrowser.frameWork, ServerBrowser_Draw, ServerBrowser_Close, ServerBrowser_Key);
}

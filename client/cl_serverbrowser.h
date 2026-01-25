/*
Copyright (C) 2025 EGL Team
Based on Q2Pro by Andrey Nazarov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#ifndef CL_SERVERBROWSER_H
#define CL_SERVERBROWSER_H

#include <stddef.h>

#define MAX_STATUS_SERVERS  1024
#define MAX_STATUS_PLAYERS  64
#define PING_STAGES         3

typedef struct playerStatus_s {
	int		score;
	int		ping;
	char	name[32];
} playerStatus_t;

typedef struct serverSlot_s {
	enum {
		SLOT_IDLE,
		SLOT_PENDING,
		SLOT_ERROR,
		SLOT_VALID
	} status;
	netAdr_t			address;
	char				*hostname;		// Original domain name for favorites
	char				infostring[MAX_INFO_STRING];
	int					numPlayers;
	playerStatus_t		players[MAX_STATUS_PLAYERS];
	unsigned			timestamp;
	uint32				color;
	char				name[256];		// Formatted display name
} serverSlot_t;

// API functions
void	SrvBrowser_Init(void);
void	SrvBrowser_Shutdown(void);
void	SrvBrowser_Frame(void);
void	SrvBrowser_StatusResponse(netAdr_t *from, const char *response);

// Master server response handling (connectionless "servers" reply)
void	SrvBrowser_MasterServersResponse(const netAdr_t *from, const byte *payload, size_t payloadLen);

// Console commands
void	SrvBrowser_Refresh_f(void);
void	SrvBrowser_List_f(void);

// Menu access
extern serverSlot_t **g_serverSlots;

#endif // CL_SERVERBROWSER_H
/*
Copyright (C) 2025 EGL Team
Based on Q2Pro by Andrey Nazarov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

#include "gui_local.h"
#include "cl_serverbrowser.h"

// Note: This is a simplified placeholder for the server browser menu.
// EGL uses a different GUI system than Q2Pro's immediate-mode menus.
// This file provides console command integration and can be expanded
// to integrate with EGL's GUI system later.

/*
=================
M_Menu_Servers_f

Console command to open server browser
=================
*/
void M_Menu_Servers_f(void)
{
	Com_Printf(0, "Server Browser Menu\n");
	Com_Printf(0, "Use 'sb_refresh' to refresh server list\n");
	Com_Printf(0, "Use 'sb_list' to list servers in console\n");
	Com_Printf(0, "Use 'connect <address>' to connect to a server\n");
	
	// TODO: Open GUI-based server browser menu when EGL GUI system is integrated
	// For now, just refresh the server list
	SrvBrowser_Refresh_f();
}
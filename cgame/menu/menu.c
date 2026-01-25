/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// menu.c
//

#include "m_local.h"

menuState_t	menuState;

cVar_t	*ui_jsMenuPage;
cVar_t	*ui_jsSortItem;
cVar_t	*ui_jsSortMethod;

static void	*cmd_menuMain;

static void	*cmd_menuGame;
static void	*cmd_menuLoadGame;
static void	*cmd_menuSaveGame;
static void	*cmd_menuCredits;

static void	*cmd_menuMultiplayer;
static void	*cmd_menuDLOptions;
static void	*cmd_menuJoinServer;
static void	*cmd_menuAddressBook;
static void	*cmd_menuPlayerConfig;
static void	*cmd_menuStartServer;
static void	*cmd_menuDMFlags;

static void	*cmd_menuOptions;
static void	*cmd_menuControls;
static void	*cmd_menuEffects;
static void	*cmd_menuGloom;
static void	*cmd_menuHUD;
static void	*cmd_menuInput;
static void	*cmd_menuMisc;
static void	*cmd_menuScreen;
static void	*cmd_menuSound;

static void	*cmd_menuVideo;
static void	*cmd_menuGLExts;
static void	*cmd_menuVidSettings;

static void	*cmd_menuQuit;

static void	*cmd_pushmenu;

static void	*cmd_startSStatus;

/*
=================
UI_PushMenu_f

Compatibility command used by shipped configs (e.g. "pushmenu servers +http://...").
=================
*/
static void UI_PushMenu_f (void)
{
	int			argc;
	const char	*menuName;
	const char	*url;
	int			i;

	argc = cgi.Cmd_Argc ();
	if (argc < 2) {
		Com_Printf (0, "Usage: pushmenu <menu> [ +http(s)://url ]\n");
		return;
	}

	menuName = cgi.Cmd_Argv (1);
	if (!Q_strnicmp (menuName, "menu_", 5))
		menuName += 5;

	url = NULL;
	for (i = 2; i < argc; i++) {
		const char *arg = cgi.Cmd_Argv (i);
		if (!arg || !arg[0])
			continue;

		if (arg[0] == '+')
			arg++;

		if (!Q_strnicmp (arg, "http://", 7) || !Q_strnicmp (arg, "https://", 8)) {
			url = arg;
			break;
		}
	}

	if (!Q_stricmp (menuName, "servers") || !Q_stricmp (menuName, "serverbrowser")) {
		if (url) {
			cgi.Cvar_Set ("sb_master1", (char *)url, qFalse);
			cgi.Cvar_Set ("sb_master2", "", qFalse);
			cgi.Cvar_Set ("sb_master3", "", qFalse);
			cgi.Cvar_Set ("sb_master4", "", qFalse);
			cgi.Cvar_Set ("sb_usemasters", "1", qFalse);
			cgi.Cvar_Set ("sb_uselan", "0", qFalse);
		}

		UI_ServerBrowserMenu_f ();
		cgi.Cbuf_AddText ("sb_refresh\n");
		return;
	}

	if (!Q_stricmp (menuName, "main"))
		return UI_MainMenu_f ();
	if (!Q_stricmp (menuName, "game"))
		return UI_GameMenu_f ();
	if (!Q_stricmp (menuName, "loadgame"))
		return UI_LoadGameMenu_f ();
	if (!Q_stricmp (menuName, "savegame"))
		return UI_SaveGameMenu_f ();
	if (!Q_stricmp (menuName, "credits"))
		return UI_CreditsMenu_f ();
	if (!Q_stricmp (menuName, "multiplayer"))
		return UI_MultiplayerMenu_f ();
	if (!Q_stricmp (menuName, "dloptions"))
		return UI_DLOptionsMenu_f ();
	if (!Q_stricmp (menuName, "joinserver"))
		return UI_JoinServerMenu_f ();
	if (!Q_stricmp (menuName, "addressbook"))
		return UI_AddressBookMenu_f ();
	if (!Q_stricmp (menuName, "playerconfig"))
		return UI_PlayerConfigMenu_f ();
	if (!Q_stricmp (menuName, "startserver"))
		return UI_StartServerMenu_f ();
	if (!Q_stricmp (menuName, "dmflags"))
		return UI_DMFlagsMenu_f ();
	if (!Q_stricmp (menuName, "options"))
		return UI_OptionsMenu_f ();
	if (!Q_stricmp (menuName, "controls"))
		return UI_ControlsMenu_f ();
	if (!Q_stricmp (menuName, "effects"))
		return UI_EffectsMenu_f ();
	if (!Q_stricmp (menuName, "gloom"))
		return UI_GloomMenu_f ();
	if (!Q_stricmp (menuName, "hud"))
		return UI_HUDMenu_f ();
	if (!Q_stricmp (menuName, "input"))
		return UI_InputMenu_f ();
	if (!Q_stricmp (menuName, "misc"))
		return UI_MiscMenu_f ();
	if (!Q_stricmp (menuName, "screen"))
		return UI_ScreenMenu_f ();
	if (!Q_stricmp (menuName, "sound"))
		return UI_SoundMenu_f ();
	if (!Q_stricmp (menuName, "video"))
		return UI_VideoMenu_f ();
	if (!Q_stricmp (menuName, "glexts"))
		return UI_GLExtsMenu_f ();
	if (!Q_stricmp (menuName, "vidsettings"))
		return UI_VIDSettingsMenu_f ();
	if (!Q_stricmp (menuName, "quit"))
		return UI_QuitMenu_f ();

	Com_Printf (0, "Unknown menu '%s' for pushmenu\n", menuName);
}

/*
=============================================================================

	MENU INITIALIZATION/SHUTDOWN

=============================================================================
*/

/*
=================
M_Init
=================
*/
void JoinMenu_StartSStatus (void); // FIXME
void M_Init (void)
{
	int		i;
	Com_DevPrintf (0, "[cginit] M_Init begin\n");

   	// Register cvars
	for (i=0 ; i<MAX_ADDRBOOK_SAVES ; i++)
		cgi.Cvar_Register (Q_VarArgs ("adr%i", i),				"",			CVAR_ARCHIVE);
	Com_DevPrintf (0, "[cginit] M_Init addrbook cvars registered\n");

	ui_jsMenuPage		= cgi.Cvar_Register ("ui_jsMenuPage",	"0",		CVAR_ARCHIVE);
	ui_jsSortItem		= cgi.Cvar_Register ("ui_jsSortItem",	"0",		CVAR_ARCHIVE);
	ui_jsSortMethod		= cgi.Cvar_Register ("ui_jsSortMethod",	"0",		CVAR_ARCHIVE);

	// Add commands
	cmd_menuMain		= cgi.Cmd_AddCommand ("menu_main",			UI_MainMenu_f,				"Opens the main menu");

	cmd_menuGame		= cgi.Cmd_AddCommand ("menu_game",			UI_GameMenu_f,				"Opens the single player menu");
	cmd_menuLoadGame	= cgi.Cmd_AddCommand ("menu_loadgame",		UI_LoadGameMenu_f,			"Opens the load game menu");
	cmd_menuSaveGame	= cgi.Cmd_AddCommand ("menu_savegame",		UI_SaveGameMenu_f,			"Opens the save game menu");
	cmd_menuCredits		= cgi.Cmd_AddCommand ("menu_credits",		UI_CreditsMenu_f,			"Opens the credits menu");

	cmd_menuMultiplayer	= cgi.Cmd_AddCommand ("menu_multiplayer",	UI_MultiplayerMenu_f,		"Opens the multiplayer menu");
	cmd_menuDLOptions	= cgi.Cmd_AddCommand ("menu_dloptions",		UI_DLOptionsMenu_f,			"Opens the download options menu");
	cmd_menuJoinServer	= cgi.Cmd_AddCommand ("menu_joinserver",	UI_JoinServerMenu_f,		"Opens the join server menu");
	cmd_menuAddressBook	= cgi.Cmd_AddCommand ("menu_addressbook",	UI_AddressBookMenu_f,		"Opens the address book menu");
	cmd_menuPlayerConfig= cgi.Cmd_AddCommand ("menu_playerconfig",	UI_PlayerConfigMenu_f,		"Opens the player configuration menu");
	cmd_menuStartServer	= cgi.Cmd_AddCommand ("menu_startserver",	UI_StartServerMenu_f,		"Opens the start server menu");
	cmd_menuDMFlags		= cgi.Cmd_AddCommand ("menu_dmflags",		UI_DMFlagsMenu_f,			"Opens the deathmatch flags menu");

	cmd_menuOptions		= cgi.Cmd_AddCommand ("menu_options",		UI_OptionsMenu_f,			"Opens the options menu");
	cmd_menuControls	= cgi.Cmd_AddCommand ("menu_controls",		UI_ControlsMenu_f,			"Opens the controls menu");
	cmd_menuEffects		= cgi.Cmd_AddCommand ("menu_effects",		UI_EffectsMenu_f,			"Opens the effects menu");
	cmd_menuGloom		= cgi.Cmd_AddCommand ("menu_gloom",			UI_GloomMenu_f,				"Opens the gloom menu");
	cmd_menuHUD			= cgi.Cmd_AddCommand ("menu_hud",			UI_HUDMenu_f,				"Opens the hud menu");
	cmd_menuInput		= cgi.Cmd_AddCommand ("menu_input",			UI_InputMenu_f,				"Opens the input menu");
	cmd_menuMisc		= cgi.Cmd_AddCommand ("menu_misc",			UI_MiscMenu_f,				"Opens the misc menu");
	cmd_menuScreen		= cgi.Cmd_AddCommand ("menu_screen",		UI_ScreenMenu_f,			"Opens the screen menu");
	cmd_menuSound		= cgi.Cmd_AddCommand ("menu_sound",			UI_SoundMenu_f,				"Opens the sound menu");

	cmd_menuVideo		= cgi.Cmd_AddCommand ("menu_video",			UI_VideoMenu_f,				"Opens the video menu");
	cmd_menuGLExts		= cgi.Cmd_AddCommand ("menu_glexts",		UI_GLExtsMenu_f,			"Opens the opengl extensions menu");
	cmd_menuVidSettings	= cgi.Cmd_AddCommand ("menu_vidsettings",	UI_VIDSettingsMenu_f,		"Opens the video settings menu");

	cmd_menuQuit		= cgi.Cmd_AddCommand ("menu_quit",			UI_QuitMenu_f,				"Opens the quit menu");

	cmd_pushmenu		= cgi.Cmd_AddCommand ("pushmenu",			UI_PushMenu_f,				"Pushes a menu by name (compatibility)");

	cmd_startSStatus	= cgi.Cmd_AddCommand ("ui_startSStatus",	JoinMenu_StartSStatus,		"");
	Com_DevPrintf (0, "[cginit] M_Init commands registered\n");
}


/*
=================
M_Shutdown
=================
*/
void M_Shutdown (void)
{
	// Don't play the menu exit sound
	menuState.playExitSound = qFalse;

	// Get rid of the menu
	M_ForceMenuOff ();

	// Remove commands
	cgi.Cmd_RemoveCommand ("menu_main", cmd_menuMain);

	cgi.Cmd_RemoveCommand ("menu_game", cmd_menuGame);
	cgi.Cmd_RemoveCommand ("menu_loadgame", cmd_menuLoadGame);
	cgi.Cmd_RemoveCommand ("menu_savegame", cmd_menuSaveGame);
	cgi.Cmd_RemoveCommand ("menu_credits", cmd_menuCredits);

	cgi.Cmd_RemoveCommand ("menu_multiplayer", cmd_menuMultiplayer);
	cgi.Cmd_RemoveCommand ("menu_dloptions", cmd_menuDLOptions);
	cgi.Cmd_RemoveCommand ("menu_joinserver", cmd_menuJoinServer);
	cgi.Cmd_RemoveCommand ("menu_addressbook", cmd_menuAddressBook);
	cgi.Cmd_RemoveCommand ("menu_playerconfig", cmd_menuPlayerConfig);
	cgi.Cmd_RemoveCommand ("menu_startserver", cmd_menuStartServer);
	cgi.Cmd_RemoveCommand ("menu_dmflags", cmd_menuDMFlags);

	cgi.Cmd_RemoveCommand ("menu_options", cmd_menuOptions);
	cgi.Cmd_RemoveCommand ("menu_controls", cmd_menuControls);
	cgi.Cmd_RemoveCommand ("menu_effects", cmd_menuEffects);
	cgi.Cmd_RemoveCommand ("menu_gloom", cmd_menuGloom);
	cgi.Cmd_RemoveCommand ("menu_hud", cmd_menuHUD);
	cgi.Cmd_RemoveCommand ("menu_input", cmd_menuInput);
	cgi.Cmd_RemoveCommand ("menu_misc", cmd_menuMisc);
	cgi.Cmd_RemoveCommand ("menu_screen", cmd_menuScreen);
	cgi.Cmd_RemoveCommand ("menu_sound", cmd_menuSound);

	cgi.Cmd_RemoveCommand ("menu_video", cmd_menuVideo);
	cgi.Cmd_RemoveCommand ("menu_glexts", cmd_menuGLExts);
	cgi.Cmd_RemoveCommand ("menu_vidsettings", cmd_menuVidSettings);

	cgi.Cmd_RemoveCommand ("menu_quit", cmd_menuQuit);

	cgi.Cmd_RemoveCommand ("pushmenu", cmd_pushmenu);

	cgi.Cmd_RemoveCommand ("ui_startSStatus", cmd_startSStatus);
}

/*
=============================================================================

	PUBLIC FUNCTIONS

=============================================================================
*/

/*
=================
M_Refresh
=================
*/
void M_Refresh (void)
{
	// Delay playing the enter sound until after the menu has
	// been drawn, to avoid delay while caching images
	if (menuState.playEnterSound) {
		cgi.Snd_StartLocalSound (uiMedia.sounds.menuIn, 1);
		menuState.playEnterSound = qFalse;
	}
	else if (uiState.newCursorItem) {
		// Play menu open sound
		cgi.Snd_StartLocalSound (uiMedia.sounds.menuMove, 1);
		uiState.newCursorItem = qFalse;
	}
}


/*
=================
M_ForceMenuOff
=================
*/
void M_ForceMenuOff (void)
{
	cg.menuOpen = qFalse;

	// Unpause
	cgi.Cvar_Set ("paused", "0", qFalse);

	// Play exit sound
	if (menuState.playExitSound) {
		cgi.Snd_StartLocalSound (uiMedia.sounds.menuOut, 1);
		menuState.playExitSound = qFalse;
	}

	// Update mouse position
	UI_UpdateMousePos ();

	// Kill the interfaces
	UI_ForceAllOff ();
}


/*
=================
M_PopMenu
=================
*/
void M_PopMenu (void)
{
	UI_PopInterface ();
}

/*
=============================================================================

	LOCAL FUNCTIONS

=============================================================================
*/

/*
=================
M_PushMenu
=================
*/
void M_PushMenu (uiFrameWork_t *frameWork, void (*drawFunc) (void), struct sfx_s *(*closeFunc) (void), struct sfx_s *(*keyFunc) (uiFrameWork_t *fw, keyNum_t keyNum))
{
	// Pause single-player games
	if (cgi.Cvar_GetFloatValue ("maxclients") == 1 && cgi.Com_ServerState ())
		cgi.Cvar_Set ("paused", "1", qFalse);

	menuState.playEnterSound = qTrue;
	menuState.playExitSound = qTrue;

	UI_PushInterface (frameWork, drawFunc, closeFunc, keyFunc);

	cg.menuOpen = qTrue;
}


/*
=================
M_KeyHandler
=================
*/
struct sfx_s *M_KeyHandler (uiFrameWork_t *fw, keyNum_t keyNum)
{
	if (keyNum == K_ESCAPE) {
		M_PopMenu ();
		return NULL;
	}

	return UI_DefaultKeyFunc (fw, keyNum);
}

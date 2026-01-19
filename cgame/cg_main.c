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
// cg_main.c
//

#include "cg_local.h"

cgState_t	cg;
cgMedia_t	cgMedia;
uiMedia_t	uiMedia;

#ifdef CGAME_HARD_LINKED
extern cVar_t	*cg_advInfrared;
extern cVar_t	*cg_brassTime;
extern cVar_t	*cg_decals;
extern cVar_t	*cg_decalBurnLife;
extern cVar_t	*cg_decalFadeTime;
extern cVar_t	*cg_decalLife;
extern cVar_t	*cg_decalLOD;
extern cVar_t	*cg_decalMax;
extern cVar_t	*cg_mapEffects;
extern cVar_t	*cl_add_particles;
extern cVar_t	*cg_particleCulling;
extern cVar_t	*cg_particleGore;
extern cVar_t	*cg_particleMax;
extern cVar_t	*cg_particleShading;
extern cVar_t	*cg_particleSmokeLinger;
extern cVar_t	*cg_railCoreRed;
extern cVar_t	*cg_railCoreGreen;
extern cVar_t	*cg_railCoreBlue;
extern cVar_t	*cg_railSpiral;
extern cVar_t	*cg_railSpiralRed;
extern cVar_t	*cg_railSpiralGreen;
extern cVar_t	*cg_railSpiralBlue;
extern cVar_t	*cg_thirdPerson;
extern cVar_t	*cg_thirdPersonAngle;
extern cVar_t	*cg_thirdPersonClip;
extern cVar_t	*cg_thirdPersonDist;

extern cVar_t	*cl_explorattle;
extern cVar_t	*cl_explorattle_scale;
extern cVar_t	*cl_footsteps;
extern cVar_t	*cl_gun;
extern cVar_t	*cl_noskins;
extern cVar_t	*cl_predict;
extern cVar_t	*cl_showmiss;
extern cVar_t	*cl_vwep;

extern cVar_t	*gender_auto;
extern cVar_t	*gender;
extern cVar_t	*hand;
extern cVar_t	*skin;

extern cVar_t	*glm_advgas;
extern cVar_t	*glm_advstingfire;
extern cVar_t	*glm_blobtype;
extern cVar_t	*glm_bluestingfire;
extern cVar_t	*glm_flashpred;
extern cVar_t	*glm_flwhite;
extern cVar_t	*glm_forcecache;
extern cVar_t	*glm_jumppred;
extern cVar_t	*glm_showclass;

extern cVar_t	*cl_testblend;
extern cVar_t	*cl_testentities;
extern cVar_t	*cl_testlights;
extern cVar_t	*cl_testparticles;

extern cVar_t	*r_hudScale;

extern cVar_t	*crosshair;
extern cVar_t	*ch_alpha;
extern cVar_t	*ch_pulse;
extern cVar_t	*ch_scale;
extern cVar_t	*ch_red;
extern cVar_t	*ch_green;
extern cVar_t	*ch_blue;
extern cVar_t	*ch_xOffset;
extern cVar_t	*ch_yOffset;

extern cVar_t	*cl_showfps;
extern cVar_t	*cl_showping;
extern cVar_t	*cl_showtime;

extern cVar_t	*con_chatHud;
extern cVar_t	*con_chatHudLines;
extern cVar_t	*con_chatHudPosX;
extern cVar_t	*con_chatHudPosY;
extern cVar_t	*con_chatHudShadow;
extern cVar_t	*con_notifyfade;
extern cVar_t	*con_notifylarge;
extern cVar_t	*con_notifylines;
extern cVar_t	*con_notifytime;
extern cVar_t	*con_alpha;
extern cVar_t	*con_clock;
extern cVar_t	*con_drop;
extern cVar_t	*con_scroll;

extern cVar_t	*scr_conspeed;
extern cVar_t	*scr_centertime;
extern cVar_t	*scr_showpause;
extern cVar_t	*scr_hudalpha;

extern cVar_t	*scr_netgraph;
extern cVar_t	*scr_timegraph;
extern cVar_t	*scr_debuggraph;
extern cVar_t	*scr_graphheight;
extern cVar_t	*scr_graphscale;
extern cVar_t	*scr_graphshift;
extern cVar_t	*scr_graphalpha;

extern cVar_t	*viewsize;
extern cVar_t	*gl_polyblend;
#endif

// ====================================================================

/*
==================
CG_SetRefConfig
==================
*/
void CG_SetRefConfig (refConfig_t *inConfig)
{
	cg.refConfig = *inConfig;

	// Force a cg.hudScale update
	r_hudScale->modified = qTrue;
}


/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars (qBool forceUpdate)
{
	// HUD scale
	if (r_hudScale->modified || forceUpdate) {
		r_hudScale->modified = qFalse;
		if (r_hudScale->floatVal <= 0)
			cgi.Cvar_VariableSetValue (r_hudScale, 1, qTrue);

		cg.hudScale[0] = r_hudScale->floatVal;
		cg.hudScale[1] = r_hudScale->floatVal;
	}

	// cg_brassTime
	if (cg_brassTime->modified) {
		cg_brassTime->modified = qFalse;
		if (cg_brassTime->floatVal < 0)
			cgi.Cvar_VariableSetValue (cg_brassTime, 0, qTrue);
	}

	// cg_decalBurnLife
	if (cg_decalBurnLife->modified) {
		cg_decalBurnLife->modified = qFalse;
		if (cg_decalBurnLife->floatVal < 0)
			cgi.Cvar_VariableSetValue (cg_decalBurnLife, 0, qTrue);
	}

	// cg_decalFadeTime
	if (cg_decalFadeTime->modified) {
		cg_decalFadeTime->modified = qFalse;
		if (cg_decalFadeTime->floatVal < 0)
			cgi.Cvar_VariableSetValue (cg_decalFadeTime, 0, qTrue);
	}

	// cg_decalLife
	if (cg_decalLife->modified) {
		cg_decalLife->modified = qFalse;
		if (cg_decalLife->floatVal < 0)
			cgi.Cvar_VariableSetValue (cg_decalLife, 0, qTrue);
	}

	// cg_decalMax
	if (cg_decalMax->modified) {
		cg_decalMax->modified = qFalse;
		if (cg_decalMax->intVal > MAX_REF_DECALS)
			cgi.Cvar_VariableSetValue (cg_decalMax, MAX_REF_DECALS, qTrue);
		else if (cg_decalMax->intVal < 0)
			cgi.Cvar_VariableSetValue (cg_decalMax, 0, qTrue);
	}

	// cg_particleMax
	if (cg_particleMax->modified) {
		cg_particleMax->modified = qFalse;
		if (cg_particleMax->intVal > MAX_PARTICLES)
			cgi.Cvar_VariableSetValue (cg_particleMax, MAX_PARTICLES, qTrue);
		else if (cg_particleMax->intVal < 0)
			cgi.Cvar_VariableSetValue (cg_particleMax, 0, qTrue);
	}

	// cg_particleGore
	if (cg_particleGore->modified || forceUpdate) {
		cg_particleGore->modified = qFalse;
		if (cg_particleGore->floatVal < 0.0f)
			cgi.Cvar_VariableSetValue(cg_particleGore, 0.0f, qTrue);
		else if (cg_particleGore->floatVal > 10.0f)
			cgi.Cvar_VariableSetValue(cg_particleGore, 10.0f, qTrue);

		// 0.0-10.0 -> 0.0-1.0
		cg.goreScale = cg_particleGore->floatVal * 0.1f;
	}

	// cg_particleSmokeLinger
	if (cg_particleSmokeLinger->modified || forceUpdate) {
		cg_particleSmokeLinger->modified = qFalse;
		if (cg_particleSmokeLinger->floatVal < 0.0f)
			cgi.Cvar_VariableSetValue(cg_particleSmokeLinger, 0.0f, qTrue);
		else if (cg_particleSmokeLinger->floatVal > 10.0f)
			cgi.Cvar_VariableSetValue(cg_particleSmokeLinger, 10.0f, qTrue);

		// 0.0-10.0 -> 0.0-1.0
		cg.smokeLingerScale = cg_particleGore->floatVal * 0.1f;
	}
}

/*
=======================================================================

	CONSOLE FUNCTIONS

=======================================================================
*/

/*
=================
CG_Skins_f

Load or download any custom player skins and models
=================
*/
static void CG_Skins_f (void)
{
	int		i;

	if (cg.currGameMod == GAME_MOD_GLOOM) {
		Com_Printf (PRNT_WARNING, "Command cannot be used in Gloom (considered cheating).\n");
		return;
	}

	for (i=0 ; i<MAX_CS_CLIENTS ; i++) {
		if (!cg.configStrings[CS_PLAYERSKINS+i][0])
			continue;

		Com_Printf (0, "client %i: %s\n", i, cg.configStrings[CS_PLAYERSKINS+i]); 
		cgi.R_UpdateScreen ();
		cgi.Sys_SendKeyEvents ();	// pump message loop
		CG_ParseClientinfo (i);
	}
}


/*
=================
CG_ThirdPerson_f
=================
*/
static void CG_ThirdPerson_f (void)
{
	cgi.Cvar_SetValue ("cg_thirdPerson", !cg_thirdPerson->intVal, qFalse);
}

/*
=======================================================================

	INIT / SHUTDOWN

=======================================================================
*/

static void	*cmd_glmCache;
static void	*cmd_skins;
static void	*cmd_thirdPerson;
static void *cmd_say;
static void *cmd_say_team;
static void *cmd_wave;
static void *cmd_inven;
static void *cmd_kill;
static void *cmd_use;
static void *cmd_drop;
static void *cmd_info;
static void *cmd_prog;
static void *cmd_give;
static void *cmd_god;
static void *cmd_notarget;
static void *cmd_noclip;
static void *cmd_invuse;
static void *cmd_invprev;
static void *cmd_invnext;
static void *cmd_invdrop;
static void *cmd_weapnext;
static void *cmd_weapprev;

/*
=================
CG_CopyConfigStrings

This is necessary to make certain configstrings are consistant between
CGame and the client. Sometimes while connecting a configstring is sent
just before the CGame module is loaded.
=================
*/
void CG_CopyConfigStrings (void)
{
	int		num;

	for (num=0 ; num<MAX_CFGSTRINGS ; num++) {
		cgi.GetConfigString (num, cg.configStrings[num], MAX_CFGSTRLEN);

		CG_ParseConfigString (num, cg.configStrings[num]);
	}
}


/*
==================
CG_RegisterMain
==================
*/
static void CG_RegisterMain (void)
{
	cg_advInfrared			= cgi.Cvar_Register ("cg_advInfrared",			"1",			CVAR_ARCHIVE);
	cg_brassTime			= cgi.Cvar_Register ("cg_brassTime",			"10",			CVAR_ARCHIVE);
	cg_decals				= cgi.Cvar_Register ("cg_decals",				"1",			CVAR_ARCHIVE);
	cg_decalBurnLife		= cgi.Cvar_Register ("cg_decalBurnLife",		"10",			CVAR_ARCHIVE);
	cg_decalFadeTime		= cgi.Cvar_Register ("cg_decalFadeTime",		"1",			CVAR_ARCHIVE);
	cg_decalLife			= cgi.Cvar_Register ("cg_decalLife",			"60",			CVAR_ARCHIVE);
	cg_decalLOD				= cgi.Cvar_Register ("cg_decalLOD",				"1",			CVAR_ARCHIVE);
	cg_decalMax				= cgi.Cvar_Register ("cg_decalMax",				"4096",			CVAR_ARCHIVE);
	cg_mapEffects			= cgi.Cvar_Register ("cg_mapEffects",			"1",			CVAR_ARCHIVE);
	cl_add_particles		= cgi.Cvar_Register ("cl_particles",			"1",			0);
	cg_particleCulling		= cgi.Cvar_Register ("cg_particleCulling",		"1",			CVAR_ARCHIVE);
	cg_particleGore			= cgi.Cvar_Register ("cg_particleGore",			"3",			CVAR_ARCHIVE);
	cg_particleMax			= cgi.Cvar_Register ("cg_particleMax",			"8192",			CVAR_ARCHIVE);
	cg_particleShading		= cgi.Cvar_Register ("cg_particleShading",		"1",			CVAR_ARCHIVE);
	cg_particleSmokeLinger	= cgi.Cvar_Register ("cg_particleSmokeLinger",	"3",			CVAR_ARCHIVE);
	cg_railCoreRed			= cgi.Cvar_Register ("cg_railCoreRed",			"0.75",			CVAR_ARCHIVE);
	cg_railCoreGreen		= cgi.Cvar_Register ("cg_railCoreGreen",		"1",			CVAR_ARCHIVE);
	cg_railCoreBlue			= cgi.Cvar_Register ("cg_railCoreBlue",			"1",			CVAR_ARCHIVE);
	cg_railSpiral			= cgi.Cvar_Register ("cg_railSpiral",			"1",			CVAR_ARCHIVE);
	cg_railSpiralRed		= cgi.Cvar_Register ("cg_railSpiralRed",		"0",			CVAR_ARCHIVE);
	cg_railSpiralGreen		= cgi.Cvar_Register ("cg_railSpiralGreen",		"0.5",			CVAR_ARCHIVE);
	cg_railSpiralBlue		= cgi.Cvar_Register ("cg_railSpiralBlue",		"0.9",			CVAR_ARCHIVE);

	if (cg.currGameMod == GAME_MOD_GLOOM)
		cg_thirdPerson		= cgi.Cvar_Register ("cg_thirdPerson",			"0",			CVAR_ARCHIVE|CVAR_CHEAT);
	else
		cg_thirdPerson		= cgi.Cvar_Register ("cg_thirdPerson",			"0",			CVAR_ARCHIVE);
	cg_thirdPersonAngle		= cgi.Cvar_Register ("cg_thirdPersonAngle",		"30",			CVAR_ARCHIVE);
	cg_thirdPersonClip		= cgi.Cvar_Register ("cg_thirdPersonClip",		"1",			CVAR_ARCHIVE);
	cg_thirdPersonDist		= cgi.Cvar_Register ("cg_thirdPersonDist",		"50",			CVAR_ARCHIVE);

	cl_explorattle			= cgi.Cvar_Register ("cl_explorattle",			"1",			CVAR_ARCHIVE);
	cl_explorattle_scale	= cgi.Cvar_Register ("cl_explorattle_scale",	"0.3",			CVAR_ARCHIVE);
	cl_footsteps			= cgi.Cvar_Register ("cl_footsteps",			"1",			0);
	cl_gun					= cgi.Cvar_Register ("cl_gun",					"1",			0);
	cl_noskins				= cgi.Cvar_Register ("cl_noskins",				"0",			CVAR_CHEAT);
	cl_predict				= cgi.Cvar_Register ("cl_predict",				"1",			0);
	cl_showmiss				= cgi.Cvar_Register ("cl_showmiss",				"0",			0);
	cl_vwep					= cgi.Cvar_Register ("cl_vwep",					"1",			CVAR_ARCHIVE);

	gender_auto				= cgi.Cvar_Register ("gender_auto",				"1",			CVAR_ARCHIVE);
	gender					= cgi.Cvar_Register ("gender",					"male",			CVAR_USERINFO|CVAR_ARCHIVE);
	hand					= cgi.Cvar_Register ("hand",					"0",			CVAR_USERINFO|CVAR_ARCHIVE);
	skin					= cgi.Cvar_Register ("skin",					"male/grunt",	CVAR_USERINFO|CVAR_ARCHIVE);

	glm_advgas				= cgi.Cvar_Register ("glm_advgas",				"0",			CVAR_ARCHIVE);
	glm_advstingfire		= cgi.Cvar_Register ("glm_advstingfire",		"1",			CVAR_ARCHIVE);
	glm_blobtype			= cgi.Cvar_Register ("glm_blobtype",			"1",			CVAR_ARCHIVE);
	glm_bluestingfire		= cgi.Cvar_Register ("glm_bluestingfire",		"0",			CVAR_ARCHIVE);
	glm_flashpred			= cgi.Cvar_Register ("glm_flashpred",			"0",			CVAR_ARCHIVE);
	glm_flwhite				= cgi.Cvar_Register ("glm_flwhite",				"1",			CVAR_ARCHIVE);
	glm_forcecache			= cgi.Cvar_Register ("glm_forcecache",			"0",			CVAR_ARCHIVE);
	glm_jumppred			= cgi.Cvar_Register ("glm_jumppred",			"0",			CVAR_ARCHIVE);
	glm_showclass			= cgi.Cvar_Register ("glm_showclass",			"1",			CVAR_ARCHIVE);

	cl_testblend			= cgi.Cvar_Register ("cl_testblend",			"0",			CVAR_CHEAT);
	cl_testentities			= cgi.Cvar_Register ("cl_testentities",			"0",			CVAR_CHEAT);
	cl_testlights			= cgi.Cvar_Register ("cl_testlights",			"0",			CVAR_CHEAT);
	cl_testparticles		= cgi.Cvar_Register ("cl_testparticles",		"0",			CVAR_CHEAT);

	r_hudScale				= cgi.Cvar_Register ("r_hudScale",				"1",			CVAR_ARCHIVE);
	r_fontScale				= cgi.Cvar_Register ("r_fontScale",				"1",			CVAR_ARCHIVE);

	// Effect scaling (renderer owns r_effectscale; registering here caches the pointers for cgame)
	r_effectscale			= cgi.Cvar_Register ("r_effectscale",			"1.0",			CVAR_ARCHIVE);
	r_effectscale_blaster	= cgi.Cvar_Register ("r_effectscale_blaster",	"1.0",			CVAR_ARCHIVE);
	r_effectscale_bfg		= cgi.Cvar_Register ("r_effectscale_bfg",		"1.0",			CVAR_ARCHIVE);
	r_effectscale_explo	= cgi.Cvar_Register ("r_effectscale_explo",	"1.0",			CVAR_ARCHIVE);
	r_effectscale_ion		= cgi.Cvar_Register ("r_effectscale_ion",		"1.0",			CVAR_ARCHIVE);
	r_effectscale_phalanx	= cgi.Cvar_Register ("r_effectscale_phalanx",	"1.0",			CVAR_ARCHIVE);
	r_effectscale_rail		= cgi.Cvar_Register ("r_effectscale_rail",		"1.0",			CVAR_ARCHIVE);
	r_effectscale_smoke	= cgi.Cvar_Register ("r_effectscale_smoke",	"1.0",			CVAR_ARCHIVE);
	r_effectscale_spark	= cgi.Cvar_Register ("r_effectscale_spark",	"1.0",			CVAR_ARCHIVE);

	if (cg.currGameMod == GAME_MOD_DDAY)
		crosshair			= cgi.Cvar_Register ("crosshair",				"2",			CVAR_ARCHIVE|CVAR_CHEAT);
	else
		crosshair			= cgi.Cvar_Register ("crosshair",				"2",			CVAR_ARCHIVE);
	ch_alpha				= cgi.Cvar_Register ("ch_alpha",				"1",			CVAR_ARCHIVE);
	ch_pulse				= cgi.Cvar_Register ("ch_pulse",				"1",			CVAR_ARCHIVE);
	ch_scale				= cgi.Cvar_Register ("ch_scale",				"1",			CVAR_ARCHIVE);
	ch_red					= cgi.Cvar_Register ("ch_red",					"1",			CVAR_ARCHIVE);
	ch_green				= cgi.Cvar_Register ("ch_green",				"1",			CVAR_ARCHIVE);
	ch_blue					= cgi.Cvar_Register ("ch_blue",					"1",			CVAR_ARCHIVE);
	ch_xOffset				= cgi.Cvar_Register ("ch_xOffset",				"0",			CVAR_ARCHIVE);
	ch_yOffset				= cgi.Cvar_Register ("ch_yOffset",				"0",			CVAR_ARCHIVE);

	cl_showfps				= cgi.Cvar_Register ("cl_showfps",				"1",			CVAR_ARCHIVE);
	cl_showping				= cgi.Cvar_Register ("cl_showping",				"1",			CVAR_ARCHIVE);
	cl_showtime				= cgi.Cvar_Register ("cl_showtime",				"1",			CVAR_ARCHIVE);

	con_chatHud				= cgi.Cvar_Register ("con_chatHud",				"0",			CVAR_ARCHIVE);
	con_chatHudLines		= cgi.Cvar_Register ("con_chatHudLines",		"4",			CVAR_ARCHIVE);
	con_chatHudPosX			= cgi.Cvar_Register ("con_chatHudPosX",			"8",			CVAR_ARCHIVE);
	con_chatHudPosY			= cgi.Cvar_Register ("con_chatHudPosY",			"-14",			CVAR_ARCHIVE);
	con_chatHudShadow		= cgi.Cvar_Register ("con_chatHudShadow",		"0",			CVAR_ARCHIVE);
	con_notifyfade			= cgi.Cvar_Register ("con_notifyfade",			"1",			CVAR_ARCHIVE);
	con_notifylarge			= cgi.Cvar_Register ("con_notifylarge",			"0",			CVAR_ARCHIVE);
	con_notifylines			= cgi.Cvar_Register ("con_notifylines",			"4",			CVAR_ARCHIVE);
	con_notifytime			= cgi.Cvar_Register ("con_notifytime",			"3",			CVAR_ARCHIVE);
	con_alpha				= cgi.Cvar_Register ("con_alpha",				"1",			CVAR_ARCHIVE);
	con_clock				= cgi.Cvar_Register ("con_clock",				"1",			CVAR_ARCHIVE);
	con_drop				= cgi.Cvar_Register ("con_drop",				"0.5",			CVAR_ARCHIVE);
	con_scroll				= cgi.Cvar_Register ("con_scroll",				"2",			CVAR_ARCHIVE);

	scr_conspeed			= cgi.Cvar_Register ("scr_conspeed",			"3",			0);
	scr_centertime			= cgi.Cvar_Register ("scr_centertime",			"2.5",			0);
	scr_showpause			= cgi.Cvar_Register ("scr_showpause",			"1",			0);
	scr_hudalpha			= cgi.Cvar_Register ("scr_hudalpha",			"1",			CVAR_ARCHIVE);

	scr_netgraph			= cgi.Cvar_Register ("netgraph",				"0",			0);
	scr_timegraph			= cgi.Cvar_Register ("timegraph",				"0",			0);
	scr_debuggraph			= cgi.Cvar_Register ("debuggraph",				"0",			0);
	scr_graphheight			= cgi.Cvar_Register ("graphheight",				"32",			0);
	scr_graphscale			= cgi.Cvar_Register ("graphscale",				"1",			0);
	scr_graphshift			= cgi.Cvar_Register ("graphshift",				"0",			0);
	scr_graphalpha			= cgi.Cvar_Register ("scr_graphalpha",			"0.6",			CVAR_ARCHIVE);

	viewsize				= cgi.Cvar_Register ("viewsize",				"100",			CVAR_ARCHIVE);

	if (cg.currGameMod == GAME_MOD_GLOOM)
		gl_polyblend		= cgi.Cvar_Register ("gl_polyblend",			"1",			CVAR_CHEAT);
	else
		gl_polyblend		= cgi.Cvar_Register ("gl_polyblend",			"1",			0);

	gender->modified = qFalse; // clear this so we know when user sets it manually

	cmd_glmCache	= cgi.Cmd_AddCommand ("glmcache",		CG_CacheGloomMedia,	"Forces caching of Gloom media right now");
	cmd_skins		= cgi.Cmd_AddCommand ("skins",			CG_Skins_f,			"Lists skins of players connected");
	cmd_thirdPerson	= cgi.Cmd_AddCommand ("thirdPerson",	CG_ThirdPerson_f,	"Toggles the third person camera");

	// Userinfo cvars
	cgi.Cvar_Register ("fov",			"90",			CVAR_USERINFO|CVAR_ARCHIVE);
	cgi.Cvar_Register ("password",		"",				CVAR_USERINFO);
	cgi.Cvar_Register ("spectator",		"0",			CVAR_USERINFO);
	cgi.Cvar_Register ("msg",			"0",			CVAR_USERINFO|CVAR_ARCHIVE);
	cgi.Cvar_Register ("name",			"unnamed",		CVAR_USERINFO|CVAR_ARCHIVE);
	cgi.Cvar_Register ("rate",			"8000",			CVAR_USERINFO|CVAR_ARCHIVE);
	cgi.Cvar_Register ("gender",		"male",			CVAR_USERINFO|CVAR_ARCHIVE);
	cgi.Cvar_Register ("hand",			"0",			CVAR_USERINFO|CVAR_ARCHIVE);
	cgi.Cvar_Register ("skin",			"",				CVAR_USERINFO|CVAR_ARCHIVE);

	// Register our commands
	cmd_say			= cgi.Cmd_AddCommand ("say",			CG_Say_Preprocessor,	"");
	cmd_say_team	= cgi.Cmd_AddCommand ("say_team",		CG_Say_Preprocessor,	"");

	/*
	** Forward to server commands...
	** The only thing this does is allow command completion to work -- all unknown
	** commands are automatically forwarded to the server
	*/
	cmd_wave		= cgi.Cmd_AddCommand ("wave",			NULL,					"");
	cmd_inven		= cgi.Cmd_AddCommand ("inven",			NULL,					"");
	cmd_kill		= cgi.Cmd_AddCommand ("kill",			NULL,					"");
	cmd_use			= cgi.Cmd_AddCommand ("use",			NULL,					"");
	cmd_drop		= cgi.Cmd_AddCommand ("drop",			NULL,					"");
	cmd_info		= cgi.Cmd_AddCommand ("info",			NULL,					"");
	cmd_prog		= cgi.Cmd_AddCommand ("prog",			NULL,					"");
	cmd_give		= cgi.Cmd_AddCommand ("give",			NULL,					"");
	cmd_god			= cgi.Cmd_AddCommand ("god",			NULL,					"");
	cmd_notarget	= cgi.Cmd_AddCommand ("notarget",		NULL,					"");
	cmd_noclip		= cgi.Cmd_AddCommand ("noclip",			NULL,					"");
	cmd_invuse		= cgi.Cmd_AddCommand ("invuse",			NULL,					"");
	cmd_invprev		= cgi.Cmd_AddCommand ("invprev",		NULL,					"");
	cmd_invnext		= cgi.Cmd_AddCommand ("invnext",		NULL,					"");
	cmd_invdrop		= cgi.Cmd_AddCommand ("invdrop",		NULL,					"");
	cmd_weapnext	= cgi.Cmd_AddCommand ("weapnext",		NULL,					"");
	cmd_weapprev	= cgi.Cmd_AddCommand ("weapprev",		NULL,					"");
}


/*
==================
CG_RemoveCmds
==================
*/
static void CG_RemoveCmds (void)
{
	cgi.Cmd_RemoveCommand ("glmcache",		cmd_glmCache);
	cgi.Cmd_RemoveCommand ("skins",			cmd_skins);
	cgi.Cmd_RemoveCommand ("thirdPerson",	cmd_thirdPerson);

	cgi.Cmd_RemoveCommand ("say",			cmd_say);
	cgi.Cmd_RemoveCommand ("say_team",		cmd_say_team);
	cgi.Cmd_RemoveCommand ("wave",			cmd_wave);
	cgi.Cmd_RemoveCommand ("inven",			cmd_inven);
	cgi.Cmd_RemoveCommand ("kill",			cmd_kill);
	cgi.Cmd_RemoveCommand ("use",			cmd_use);
	cgi.Cmd_RemoveCommand ("drop",			cmd_drop);
	cgi.Cmd_RemoveCommand ("info",			cmd_info);
	cgi.Cmd_RemoveCommand ("prog",			cmd_prog);
	cgi.Cmd_RemoveCommand ("give",			cmd_give);
	cgi.Cmd_RemoveCommand ("god",			cmd_god);
	cgi.Cmd_RemoveCommand ("notarget",		cmd_notarget);
	cgi.Cmd_RemoveCommand ("noclip",		cmd_noclip);
	cgi.Cmd_RemoveCommand ("invuse",		cmd_invuse);
	cgi.Cmd_RemoveCommand ("invprev",		cmd_invprev);
	cgi.Cmd_RemoveCommand ("invnext",		cmd_invnext);
	cgi.Cmd_RemoveCommand ("invdrop",		cmd_invdrop);
	cgi.Cmd_RemoveCommand ("weapnext",		cmd_weapnext);
	cgi.Cmd_RemoveCommand ("weapprev",		cmd_weapprev);
}


/*
==================
CG_LoadMap
==================
*/
void CG_LoadMap (int playerNum, int serverProtocol, int protocolMinorVersion, qBool attractLoop, qBool strafeHack, refConfig_t *inConfig)
{
	// Default values
	cg.frameCount = 1;
	cg.gloomCheckClass = qFalse;
	cg.gloomClassType = GLM_OBSERVER;
	cg.playerNum = playerNum;
	cg.serverProtocol = serverProtocol;
	cg.protocolMinorVersion = protocolMinorVersion;
	cg.attractLoop = attractLoop;	// true if demo playback
	cg.strafeHack = strafeHack; // god damnit

	// Video settings
	cg.refConfig = *inConfig;

	// Copy config strings
	CG_CopyConfigStrings ();

	// Init media
	CG_InitBaseMedia ();

	cg.mapLoading = qTrue;
	cg.mapLoaded = qFalse;

	CG_MapInit ();

	cg.mapLoading = qFalse;
	cg.mapLoaded = qTrue;
}


/*
==================
CG_Init
==================
*/
void CG_Init (void)
{
	char	*gameDir;
	Com_DevPrintf (0, "[cginit] CG_Init begin\n");

	// Clear everything
	memset (&cg, 0, sizeof (cgState_t));
	memset (&cgMedia, 0, sizeof (cgMedia_t));
	memset (&uiMedia, 0, sizeof (uiMedia));
	Com_DevPrintf (0, "[cginit] cleared state\n");

	CG_ClearEntities ();
	Com_DevPrintf (0, "[cginit] CG_ClearEntities done\n");

	// This nastiness is done because I don't
	// feel like compiling several cgame libraries...
	gameDir = cgi.FS_Gamedir ();
	if (strstr (gameDir, "dday"))
		cg.currGameMod = GAME_MOD_DDAY;
	else if (strstr (gameDir, "giex"))
		cg.currGameMod = GAME_MOD_GIEX;
	else if (strstr (gameDir, "gloom"))
		cg.currGameMod = GAME_MOD_GLOOM;
	else if (strstr (gameDir, "lox"))
		cg.currGameMod = GAME_MOD_LOX;
	else if (strstr (gameDir, "rogue"))
		cg.currGameMod = GAME_MOD_ROGUE;
	else if (strstr (gameDir, "xatrix"))
		cg.currGameMod = GAME_MOD_XATRIX;
	else
		cg.currGameMod = GAME_MOD_DEFAULT;
	Com_DevPrintf (0, "[cginit] game mod detected (%d)\n", cg.currGameMod);

	// Update refConfig
	Com_DevPrintf (0, "[cginit] before R_GetRefConfig\n");
	cgi.R_GetRefConfig (&cg.refConfig);
	Com_DevPrintf (0, "[cginit] after R_GetRefConfig (%dx%d)\n", cg.refConfig.vidWidth, cg.refConfig.vidHeight);

	// Copy config strings
	Com_DevPrintf (0, "[cginit] before CG_CopyConfigStrings\n");
	CG_CopyConfigStrings ();
	Com_DevPrintf (0, "[cginit] after CG_CopyConfigStrings\n");

	// Initialize early needed media
	Com_DevPrintf (0, "[cginit] before CG_InitBaseMedia\n");
	CG_InitBaseMedia ();
	Com_DevPrintf (0, "[cginit] after CG_InitBaseMedia\n");

	// Register cvars and commands
	Com_DevPrintf (0, "[cginit] before V_Register\n");
	V_Register ();
	Com_DevPrintf (0, "[cginit] after V_Register\n");
	Com_DevPrintf (0, "[cginit] before CG_WeapRegister\n");
	CG_WeapRegister ();
	Com_DevPrintf (0, "[cginit] after CG_WeapRegister\n");
	Com_DevPrintf (0, "[cginit] before CG_RegisterMain\n");
	CG_RegisterMain ();
	Com_DevPrintf (0, "[cginit] after CG_RegisterMain\n");

	// Check cvar sanity
	Com_DevPrintf (0, "[cginit] before CG_UpdateCvars\n");
	CG_UpdateCvars (qTrue);
	Com_DevPrintf (0, "[cginit] after CG_UpdateCvars\n");

	// Location system init
	Com_DevPrintf (0, "[cginit] before CG_LocationInit\n");
	CG_LocationInit ();
	Com_DevPrintf (0, "[cginit] after CG_LocationInit\n");

	// Map FX init
	Com_DevPrintf (0, "[cginit] before CG_MapFXInit\n");
	CG_MapFXInit ();
	Com_DevPrintf (0, "[cginit] after CG_MapFXInit\n");

	// Load the UI
	Com_DevPrintf (0, "[cginit] before UI_Init\n");
	UI_Init ();
	Com_DevPrintf (0, "[cginit] after UI_Init\n");
	Com_DevPrintf (0, "[cginit] CG_Init done\n");
}


/*
==================
CG_Shutdown
==================
*/
void CG_Shutdown (void)
{
	// Shutdown map fx
	CG_MapFXShutdown ();

	// Clean out entities
	CG_ClearEntities ();

	// Remove commands
	CG_RemoveCmds ();
	V_Unregister ();
	CG_WeapUnregister ();

	// Shutdown media
	CG_ShutdownMap ();

	// Shutdown the UI
	UI_Shutdown ();

	// Shutdown locations
	CG_LocationShutdown ();

	// The above function calls should release all of this anyways, but this is to be certain
	CG_FreeTag (CGTAG_ANY);
	CG_FreeTag (CGTAG_MAPFX);
	CG_FreeTag (CGTAG_MENU);
}

//======================================================================

#ifndef CGAME_HARD_LINKED
/*
==================
Com_Printf
==================
*/
void Com_Printf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		text[MAX_COMPRINT];

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	// Print
	cgi.Com_Printf (flags, text);
}


/*
==================
Com_DevPrintf
==================
*/
void Com_DevPrintf (comPrint_t flags, char *fmt, ...)
{
	va_list		argptr;
	char		text[MAX_COMPRINT];

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	// Print
	cgi.Com_DevPrintf (flags, text);
}


/*
==================
Com_Error
==================
*/
NO_RETURN void Com_Error (comError_t code, char *fmt, ...)
{
	va_list		argptr;
	char		text[MAX_COMPRINT];

	// Evaluate args
	va_start (argptr, fmt);
	vsnprintf (text, sizeof (text), fmt, argptr);
	va_end (argptr);

	// Print
	cgi.Com_Error (code, text);
}
#endif // CGAME_HARD_LINKED

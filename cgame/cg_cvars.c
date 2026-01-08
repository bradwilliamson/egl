/*
 * cg_cvars.c
 *
 * Defines cVar_t* globals for the cgame DLL build.
 *
 * In a hard-linked build these live in the main executable, so we only
 * provide definitions when CGAME_HARD_LINKED is not set.
 */

#include "cg_local.h"

#ifndef CGAME_HARD_LINKED

cVar_t	*cg_advInfrared;
cVar_t	*cg_brassTime;
cVar_t	*cg_decals;
cVar_t	*cg_decalBurnLife;
cVar_t	*cg_decalFadeTime;
cVar_t	*cg_decalLife;
cVar_t	*cg_decalLOD;
cVar_t	*cg_decalMax;
cVar_t	*cg_mapEffects;
cVar_t	*cl_add_particles;
cVar_t	*cg_particleCulling;
cVar_t	*cg_particleGore;
cVar_t	*cg_particleMax;
cVar_t	*cg_particleShading;
cVar_t	*cg_particleSmokeLinger;
cVar_t	*cg_railCoreRed;
cVar_t	*cg_railCoreGreen;
cVar_t	*cg_railCoreBlue;
cVar_t	*cg_railSpiral;
cVar_t	*cg_railSpiralRed;
cVar_t	*cg_railSpiralGreen;
cVar_t	*cg_railSpiralBlue;
cVar_t	*cg_thirdPerson;
cVar_t	*cg_thirdPersonAngle;
cVar_t	*cg_thirdPersonClip;
cVar_t	*cg_thirdPersonDist;

cVar_t	*cl_explorattle;
cVar_t	*cl_explorattle_scale;
cVar_t	*cl_footsteps;
cVar_t	*cl_gun;
cVar_t	*cl_noskins;
cVar_t	*cl_predict;
cVar_t	*cl_showmiss;
cVar_t	*cl_vwep;

cVar_t	*crosshair;

cVar_t	*gender_auto;
cVar_t	*gender;
cVar_t	*hand;
cVar_t	*skin;

cVar_t	*glm_advgas;
cVar_t	*glm_advstingfire;
cVar_t	*glm_blobtype;
cVar_t	*glm_bluestingfire;
cVar_t	*glm_flashpred;
cVar_t	*glm_flwhite;
cVar_t	*glm_forcecache;
cVar_t	*glm_jumppred;
cVar_t	*glm_showclass;

cVar_t	*cl_testblend;
cVar_t	*cl_testentities;
cVar_t	*cl_testlights;
cVar_t	*cl_testparticles;

cVar_t	*r_hudScale;
cVar_t	*r_fontScale;

cVar_t	*r_effectscale;
cVar_t	*r_effectscale_blaster;
cVar_t	*r_effectscale_bfg;
cVar_t	*r_effectscale_explo;
cVar_t	*r_effectscale_ion;
cVar_t	*r_effectscale_phalanx;
cVar_t	*r_effectscale_rail;
cVar_t	*r_effectscale_smoke;
cVar_t	*r_effectscale_spark;

cVar_t	*ch_alpha;
cVar_t	*ch_pulse;
cVar_t	*ch_scale;
cVar_t	*ch_red;
cVar_t	*ch_green;
cVar_t	*ch_blue;
cVar_t	*ch_xOffset;
cVar_t	*ch_yOffset;

cVar_t	*cl_showfps;
cVar_t	*cl_showping;
cVar_t	*cl_showtime;

cVar_t	*con_chatHud;
cVar_t	*con_chatHudLines;
cVar_t	*con_chatHudPosX;
cVar_t	*con_chatHudPosY;
cVar_t	*con_chatHudShadow;
cVar_t	*con_notifyfade;
cVar_t	*con_notifylarge;
cVar_t	*con_notifylines;
cVar_t	*con_notifytime;
cVar_t	*con_alpha;
cVar_t	*con_clock;
cVar_t	*con_drop;
cVar_t	*con_scroll;

cVar_t	*scr_conspeed;
cVar_t	*scr_centertime;
cVar_t	*scr_showpause;
cVar_t	*scr_hudalpha;

cVar_t	*scr_netgraph;
cVar_t	*scr_timegraph;
cVar_t	*scr_debuggraph;
cVar_t	*scr_graphheight;
cVar_t	*scr_graphscale;
cVar_t	*scr_graphshift;
cVar_t	*scr_graphalpha;

cVar_t	*viewsize;
cVar_t	*gl_polyblend;

#endif

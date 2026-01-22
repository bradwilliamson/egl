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
// m_opts_input.c
//

#include "m_local.h"

/*
=======================================================================

	INPUT MENU

=======================================================================
*/

typedef struct m_inputMenu_s {
	// Menu items
	uiFrameWork_t	frameWork;

	uiImage_t		banner;
	uiAction_t		header;

	uiList_t		always_run_toggle;
	uiList_t		joystick_toggle;
	uiList_t		joystick_auto_toggle;

	uiSlider_t	joy_deadzone_slider;
	uiSlider_t	joy_deadzone_amount;
	uiSlider_t	joy_move_scale_slider;
	uiSlider_t	joy_move_scale_amount;
	uiSlider_t	joy_look_scale_slider;
	uiSlider_t	joy_look_scale_amount;
	uiList_t		joy_invert_y_toggle;
	uiSlider_t	joy_trigger_threshold_slider;
	uiSlider_t	joy_trigger_threshold_amount;

	uiSlider_t		ui_sensitivity_slider;
	uiSlider_t		ui_sensitivity_amount;
	uiSlider_t		sensitivity_slider;
	uiSlider_t		sensitivity_amount;
	uiList_t		maccel_list;
	uiList_t		autosensitivity_toggle;
	uiList_t		invert_mouse_toggle;
	uiList_t		lookspring_toggle;
	uiList_t		lookstrafe_toggle;
	uiList_t		freelook_toggle;

	uiAction_t		back_action;
} m_inputMenu_t;

static m_inputMenu_t	m_inputMenu;

static void AlwaysRunFunc (void *unused)
{
	cgi.Cvar_SetValue ("cl_run", m_inputMenu.always_run_toggle.curValue, qFalse);
}

static void JoystickFunc (void *unused)
{
	cgi.Cvar_SetValue ("in_joystick", m_inputMenu.joystick_toggle.curValue, qFalse);
}

static void JoystickAutoFunc (void *unused)
{
	cgi.Cvar_SetValue ("in_joystick_auto", m_inputMenu.joystick_auto_toggle.curValue, qFalse);
}

static void JoyDeadzoneFunc (void *unused)
{
	cgi.Cvar_SetValue ("joy_deadzone", m_inputMenu.joy_deadzone_slider.curValue / 100.0F, qFalse);
	m_inputMenu.joy_deadzone_amount.generic.name = cgi.Cvar_GetStringValue ("joy_deadzone");
}

static void JoyMoveScaleFunc (void *unused)
{
	cgi.Cvar_SetValue ("joy_move_scale", m_inputMenu.joy_move_scale_slider.curValue / 100.0F, qFalse);
	m_inputMenu.joy_move_scale_amount.generic.name = cgi.Cvar_GetStringValue ("joy_move_scale");
}

static void JoyLookScaleFunc (void *unused)
{
	cgi.Cvar_SetValue ("joy_look_scale", m_inputMenu.joy_look_scale_slider.curValue, qFalse);
	m_inputMenu.joy_look_scale_amount.generic.name = cgi.Cvar_GetStringValue ("joy_look_scale");
}

static void JoyInvertYFunc (void *unused)
{
	cgi.Cvar_SetValue ("joy_invert_y", m_inputMenu.joy_invert_y_toggle.curValue, qFalse);
}

static void JoyTriggerThresholdFunc (void *unused)
{
	cgi.Cvar_SetValue ("joy_trigger_threshold", m_inputMenu.joy_trigger_threshold_slider.curValue / 100.0F, qFalse);
	m_inputMenu.joy_trigger_threshold_amount.generic.name = cgi.Cvar_GetStringValue ("joy_trigger_threshold");
}

static void UISensFunc (void *unused)
{
	cgi.Cvar_SetValue ("ui_sensitivity", m_inputMenu.ui_sensitivity_slider.curValue / 2.0F, qFalse);
	m_inputMenu.ui_sensitivity_amount.generic.name = cgi.Cvar_GetStringValue ("ui_sensitivity");
}

static void SensitivityFunc (void *unused)
{
	cgi.Cvar_SetValue ("sensitivity", m_inputMenu.sensitivity_slider.curValue / 2.0F, qFalse);
	m_inputMenu.sensitivity_amount.generic.name = cgi.Cvar_GetStringValue ("sensitivity");
}

static void MouseAccelFunc (void *unused)
{
	cgi.Cvar_SetValue ("m_accel", m_inputMenu.maccel_list.curValue, qFalse);
}

static void InvertMouseFunc (void *unused)
{
	cgi.Cvar_SetValue ("m_pitch", (cgi.Cvar_GetFloatValue ("m_pitch")) * -1, qFalse);
	m_inputMenu.invert_mouse_toggle.curValue		= (!!(cgi.Cvar_GetFloatValue ("m_pitch") < 0));
}

static void AutoSensFunc (void *unused)
{
	cgi.Cvar_SetValue ("autosensitivity", m_inputMenu.autosensitivity_toggle.curValue, qFalse);
}

static void LookspringFunc (void *unused)
{
	cgi.Cvar_SetValue ("lookspring", m_inputMenu.lookspring_toggle.curValue, qFalse);
}

static void LookstrafeFunc (void *unused)
{
	cgi.Cvar_SetValue ("lookstrafe", m_inputMenu.lookstrafe_toggle.curValue, qFalse);
}

static void FreeLookFunc (void *unused)
{
	cgi.Cvar_SetValue ("freelook", m_inputMenu.freelook_toggle.curValue, qFalse);
}


/*
=============
InputMenu_SetValues
=============
*/
static void InputMenu_SetValues (void)
{
	cgi.Cvar_SetValue ("cl_run",				clamp (cgi.Cvar_GetIntegerValue ("cl_run"), 0, 1), qFalse);
	m_inputMenu.always_run_toggle.curValue		= cgi.Cvar_GetIntegerValue ("cl_run");

	cgi.Cvar_SetValue ("in_joystick",			clamp (cgi.Cvar_GetIntegerValue ("in_joystick"), 0, 1), qFalse);
	m_inputMenu.joystick_toggle.curValue		= cgi.Cvar_GetIntegerValue ("in_joystick");

	cgi.Cvar_SetValue ("in_joystick_auto",			clamp (cgi.Cvar_GetIntegerValue ("in_joystick_auto"), 0, 1), qFalse);
	m_inputMenu.joystick_auto_toggle.curValue		= cgi.Cvar_GetIntegerValue ("in_joystick_auto");

	cgi.Cvar_SetValue ("joy_deadzone",			clamp (cgi.Cvar_GetFloatValue ("joy_deadzone"), 0.0f, 0.50f), qFalse);
	m_inputMenu.joy_deadzone_slider.curValue		= (cgi.Cvar_GetFloatValue ("joy_deadzone")) * 100.0f;
	m_inputMenu.joy_deadzone_amount.generic.name		= cgi.Cvar_GetStringValue ("joy_deadzone");

	cgi.Cvar_SetValue ("joy_move_scale",			clamp (cgi.Cvar_GetFloatValue ("joy_move_scale"), 0.25f, 3.00f), qFalse);
	m_inputMenu.joy_move_scale_slider.curValue		= (cgi.Cvar_GetFloatValue ("joy_move_scale")) * 100.0f;
	m_inputMenu.joy_move_scale_amount.generic.name		= cgi.Cvar_GetStringValue ("joy_move_scale");

	cgi.Cvar_SetValue ("joy_look_scale",			clamp (cgi.Cvar_GetIntegerValue ("joy_look_scale"), 50, 5000), qFalse);
	m_inputMenu.joy_look_scale_slider.curValue		= cgi.Cvar_GetIntegerValue ("joy_look_scale");
	m_inputMenu.joy_look_scale_amount.generic.name		= cgi.Cvar_GetStringValue ("joy_look_scale");

	cgi.Cvar_SetValue ("joy_invert_y",			clamp (cgi.Cvar_GetIntegerValue ("joy_invert_y"), 0, 1), qFalse);
	m_inputMenu.joy_invert_y_toggle.curValue		= cgi.Cvar_GetIntegerValue ("joy_invert_y");

	cgi.Cvar_SetValue ("joy_trigger_threshold",			clamp (cgi.Cvar_GetFloatValue ("joy_trigger_threshold"), 0.01f, 1.00f), qFalse);
	m_inputMenu.joy_trigger_threshold_slider.curValue	= (cgi.Cvar_GetFloatValue ("joy_trigger_threshold")) * 100.0f;
	m_inputMenu.joy_trigger_threshold_amount.generic.name	= cgi.Cvar_GetStringValue ("joy_trigger_threshold");

	m_inputMenu.ui_sensitivity_slider.curValue		= (cgi.Cvar_GetFloatValue ("ui_sensitivity")) * 2;
	m_inputMenu.ui_sensitivity_amount.generic.name = cgi.Cvar_GetStringValue ("ui_sensitivity");
	m_inputMenu.sensitivity_slider.curValue		= (cgi.Cvar_GetFloatValue ("sensitivity")) * 2;
	m_inputMenu.sensitivity_amount.generic.name	= cgi.Cvar_GetStringValue ("sensitivity");

	cgi.Cvar_SetValue ("m_accel",					clamp (cgi.Cvar_GetIntegerValue ("m_accel"), 0, 2), qFalse);
	m_inputMenu.maccel_list.curValue				= cgi.Cvar_GetIntegerValue ("m_accel");

	m_inputMenu.invert_mouse_toggle.curValue		= (!!(cgi.Cvar_GetFloatValue ("m_pitch") < 0));

	cgi.Cvar_SetValue ("autosensitivity",			clamp (cgi.Cvar_GetIntegerValue ("autosensitivity"), 0, 1), qFalse);
	m_inputMenu.autosensitivity_toggle.curValue	= cgi.Cvar_GetIntegerValue ("autosensitivity");

	cgi.Cvar_SetValue ("lookspring",			clamp (cgi.Cvar_GetIntegerValue ("lookspring"), 0, 1), qFalse);
	m_inputMenu.lookspring_toggle.curValue		= cgi.Cvar_GetIntegerValue ("lookspring");

	cgi.Cvar_SetValue ("lookstrafe",			clamp (cgi.Cvar_GetIntegerValue ("lookstrafe"), 0, 1), qFalse);
	m_inputMenu.lookstrafe_toggle.curValue		= cgi.Cvar_GetIntegerValue ("lookstrafe");

	cgi.Cvar_SetValue ("freelook",				clamp (cgi.Cvar_GetIntegerValue ("freelook"), 0, 1), qFalse);
	m_inputMenu.freelook_toggle.curValue		= cgi.Cvar_GetIntegerValue ("freelook");
}


/*
=============
InputMenu_Init
=============
*/
static void InputMenu_Init (void)
{
	static char *yesno_names[] = {
		"no",
		"yes",
		0
	};

	static char *maccel_items[] = {
		"no accel",
		"normal",
		"os values",
		0
	};

	static char *onoff_names[] = {
		"off",
		"on",
		0
	};

	UI_StartFramework (&m_inputMenu.frameWork, FWF_CENTERHEIGHT);

	m_inputMenu.banner.generic.type		= UITYPE_IMAGE;
	m_inputMenu.banner.generic.flags	= UIF_NOSELECT|UIF_CENTERED;
	m_inputMenu.banner.generic.name		= NULL;
	m_inputMenu.banner.mat			= uiMedia.banners.options;

	m_inputMenu.header.generic.type		= UITYPE_ACTION;
	m_inputMenu.header.generic.flags	= UIF_NOSELECT|UIF_CENTERED|UIF_MEDIUM|UIF_SHADOW;
	m_inputMenu.header.generic.name		= "Input Settings";

	m_inputMenu.always_run_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.always_run_toggle.generic.name		= "Always run";
	m_inputMenu.always_run_toggle.generic.callBack	= AlwaysRunFunc;
	m_inputMenu.always_run_toggle.itemNames			= yesno_names;
	m_inputMenu.always_run_toggle.generic.statusBar	= "Always Run";

	m_inputMenu.joystick_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.joystick_toggle.generic.name		= "Use gamepad";
	m_inputMenu.joystick_toggle.generic.callBack	= JoystickFunc;
	m_inputMenu.joystick_toggle.itemNames			= yesno_names;
	m_inputMenu.joystick_toggle.generic.statusBar	= "Enable gamepad input";

	m_inputMenu.joystick_auto_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.joystick_auto_toggle.generic.name		= "Auto enable";
	m_inputMenu.joystick_auto_toggle.generic.callBack	= JoystickAutoFunc;
	m_inputMenu.joystick_auto_toggle.itemNames			= yesno_names;
	m_inputMenu.joystick_auto_toggle.generic.statusBar	= "Auto-enable gamepad when one is detected";

	m_inputMenu.joy_deadzone_slider.generic.type		= UITYPE_SLIDER;
	m_inputMenu.joy_deadzone_slider.generic.name		= "Pad deadzone";
	m_inputMenu.joy_deadzone_slider.generic.callBack	= JoyDeadzoneFunc;
	m_inputMenu.joy_deadzone_slider.minValue			= 0;
	m_inputMenu.joy_deadzone_slider.maxValue			= 50;
	m_inputMenu.joy_deadzone_slider.generic.statusBar	= "Left/right stick deadzone";
	m_inputMenu.joy_deadzone_amount.generic.type		= UITYPE_ACTION;
	m_inputMenu.joy_deadzone_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_inputMenu.joy_move_scale_slider.generic.type		= UITYPE_SLIDER;
	m_inputMenu.joy_move_scale_slider.generic.name		= "Pad move";
	m_inputMenu.joy_move_scale_slider.generic.callBack	= JoyMoveScaleFunc;
	m_inputMenu.joy_move_scale_slider.minValue			= 25;
	m_inputMenu.joy_move_scale_slider.maxValue			= 300;
	m_inputMenu.joy_move_scale_slider.generic.statusBar	= "Gamepad movement scale";
	m_inputMenu.joy_move_scale_amount.generic.type		= UITYPE_ACTION;
	m_inputMenu.joy_move_scale_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_inputMenu.joy_look_scale_slider.generic.type		= UITYPE_SLIDER;
	m_inputMenu.joy_look_scale_slider.generic.name		= "Pad look";
	m_inputMenu.joy_look_scale_slider.generic.callBack	= JoyLookScaleFunc;
	m_inputMenu.joy_look_scale_slider.minValue			= 50;
	m_inputMenu.joy_look_scale_slider.maxValue			= 5000;
	m_inputMenu.joy_look_scale_slider.generic.statusBar	= "Gamepad look speed";
	m_inputMenu.joy_look_scale_amount.generic.type		= UITYPE_ACTION;
	m_inputMenu.joy_look_scale_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_inputMenu.joy_invert_y_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.joy_invert_y_toggle.generic.name		= "Pad invert Y";
	m_inputMenu.joy_invert_y_toggle.generic.callBack	= JoyInvertYFunc;
	m_inputMenu.joy_invert_y_toggle.itemNames			= yesno_names;
	m_inputMenu.joy_invert_y_toggle.generic.statusBar	= "Invert gamepad look up/down";

	m_inputMenu.joy_trigger_threshold_slider.generic.type		= UITYPE_SLIDER;
	m_inputMenu.joy_trigger_threshold_slider.generic.name		= "Pad triggers";
	m_inputMenu.joy_trigger_threshold_slider.generic.callBack	= JoyTriggerThresholdFunc;
	m_inputMenu.joy_trigger_threshold_slider.minValue			= 1;
	m_inputMenu.joy_trigger_threshold_slider.maxValue			= 100;
	m_inputMenu.joy_trigger_threshold_slider.generic.statusBar	= "Trigger threshold for digital bind events";
	m_inputMenu.joy_trigger_threshold_amount.generic.type		= UITYPE_ACTION;
	m_inputMenu.joy_trigger_threshold_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_inputMenu.ui_sensitivity_slider.generic.type		= UITYPE_SLIDER;
	m_inputMenu.ui_sensitivity_slider.generic.name		= "UI speed";
	m_inputMenu.ui_sensitivity_slider.generic.callBack	= UISensFunc;
	m_inputMenu.ui_sensitivity_slider.minValue			= 1;
	m_inputMenu.ui_sensitivity_slider.maxValue			= 10;
	m_inputMenu.ui_sensitivity_slider.generic.statusBar	= "Menu mouse cursor sensitivity";
	m_inputMenu.ui_sensitivity_amount.generic.type		= UITYPE_ACTION;
	m_inputMenu.ui_sensitivity_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_inputMenu.sensitivity_slider.generic.type			= UITYPE_SLIDER;
	m_inputMenu.sensitivity_slider.generic.name			= "Mouse speed";
	m_inputMenu.sensitivity_slider.generic.callBack		= SensitivityFunc;
	m_inputMenu.sensitivity_slider.minValue				= 2;
	m_inputMenu.sensitivity_slider.maxValue				= 52;
	m_inputMenu.sensitivity_slider.generic.statusBar	= "Mouse Sensitivity";
	m_inputMenu.sensitivity_amount.generic.type			= UITYPE_ACTION;
	m_inputMenu.sensitivity_amount.generic.flags		= UIF_LEFT_JUSTIFY|UIF_NOSELECT;

	m_inputMenu.maccel_list.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.maccel_list.generic.name		= "Mouse accel";
	m_inputMenu.maccel_list.generic.callBack	= MouseAccelFunc;
	m_inputMenu.maccel_list.itemNames			= maccel_items;
	m_inputMenu.maccel_list.generic.statusBar	= "Mouse Acceleration options";

	m_inputMenu.autosensitivity_toggle.generic.type			= UITYPE_SPINCONTROL;
	m_inputMenu.autosensitivity_toggle.generic.name			= "Auto sensitivity";
	m_inputMenu.autosensitivity_toggle.generic.callBack		= AutoSensFunc;
	m_inputMenu.autosensitivity_toggle.itemNames			= yesno_names;
	m_inputMenu.autosensitivity_toggle.generic.statusBar	= "FOV auto-affects mouse sensitivity";

	m_inputMenu.invert_mouse_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.invert_mouse_toggle.generic.name		= "Invert mouse";
	m_inputMenu.invert_mouse_toggle.generic.callBack	= InvertMouseFunc;
	m_inputMenu.invert_mouse_toggle.itemNames			= yesno_names;
	m_inputMenu.invert_mouse_toggle.generic.statusBar	= "Invert Mouse";

	m_inputMenu.lookspring_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.lookspring_toggle.generic.name		= "Lookspring";
	m_inputMenu.lookspring_toggle.generic.callBack	= LookspringFunc;
	m_inputMenu.lookspring_toggle.itemNames			= onoff_names;
	m_inputMenu.lookspring_toggle.generic.statusBar	= "Lookspring";

	m_inputMenu.lookstrafe_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.lookstrafe_toggle.generic.name		= "Lookstrafe";
	m_inputMenu.lookstrafe_toggle.generic.callBack	= LookstrafeFunc;
	m_inputMenu.lookstrafe_toggle.itemNames			= onoff_names;
	m_inputMenu.lookstrafe_toggle.generic.statusBar	= "Lookstrafe";

	m_inputMenu.freelook_toggle.generic.type		= UITYPE_SPINCONTROL;
	m_inputMenu.freelook_toggle.generic.name		= "Free look";
	m_inputMenu.freelook_toggle.generic.callBack	= FreeLookFunc;
	m_inputMenu.freelook_toggle.itemNames			= onoff_names;
	m_inputMenu.freelook_toggle.generic.statusBar	= "Free Look";

	m_inputMenu.back_action.generic.type		= UITYPE_ACTION;
	m_inputMenu.back_action.generic.flags		= UIF_CENTERED|UIF_LARGE|UIF_SHADOW;
	m_inputMenu.back_action.generic.name		= "< Back";
	m_inputMenu.back_action.generic.callBack	= Menu_Pop;
	m_inputMenu.back_action.generic.statusBar	= "Back a menu";

	InputMenu_SetValues ();

	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.banner);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.header);

	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.always_run_toggle);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joystick_toggle);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joystick_auto_toggle);

	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_deadzone_slider);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_deadzone_amount);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_move_scale_slider);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_move_scale_amount);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_look_scale_slider);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_look_scale_amount);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_invert_y_toggle);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_trigger_threshold_slider);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.joy_trigger_threshold_amount);

	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.ui_sensitivity_slider);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.ui_sensitivity_amount);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.sensitivity_slider);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.sensitivity_amount);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.maccel_list);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.autosensitivity_toggle);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.invert_mouse_toggle);

	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.lookspring_toggle);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.lookstrafe_toggle);
	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.freelook_toggle);

	UI_AddItem (&m_inputMenu.frameWork,		&m_inputMenu.back_action);

	UI_FinishFramework (&m_inputMenu.frameWork, qTrue);
}


/*
=============
InputMenu_Close
=============
*/
static struct sfx_s *InputMenu_Close (void)
{
	return uiMedia.sounds.menuOut;
}


/*
=============
InputMenu_Draw
=============
*/
static void InputMenu_Draw (void)
{
	float	y;

	// Initialize if necessary
	if (!m_inputMenu.frameWork.initialized)
		InputMenu_Init ();

	// Dynamically position
	m_inputMenu.frameWork.x		= cg.refConfig.vidWidth * 0.5f;
	m_inputMenu.frameWork.y		= 0;

	m_inputMenu.banner.generic.x			= 0;
	m_inputMenu.banner.generic.y			= 0;

	y = m_inputMenu.banner.height * UI_SCALE;

	m_inputMenu.header.generic.x					= 0;
	m_inputMenu.header.generic.y					= y += UIFT_SIZEINC;
	m_inputMenu.always_run_toggle.generic.x			= 0;
	m_inputMenu.always_run_toggle.generic.y			= y += UIFT_SIZEINC + UIFT_SIZEINCMED;
	m_inputMenu.joystick_toggle.generic.x			= 0;
	m_inputMenu.joystick_toggle.generic.y			= y += UIFT_SIZEINC;
	m_inputMenu.joystick_auto_toggle.generic.x		= 0;
	m_inputMenu.joystick_auto_toggle.generic.y		= y += UIFT_SIZEINC;

	m_inputMenu.joy_deadzone_slider.generic.x			= 0;
	m_inputMenu.joy_deadzone_slider.generic.y			= y += UIFT_SIZEINC;
	m_inputMenu.joy_deadzone_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_inputMenu.joy_deadzone_amount.generic.y			= y;

	m_inputMenu.joy_move_scale_slider.generic.x			= 0;
	m_inputMenu.joy_move_scale_slider.generic.y			= y += UIFT_SIZEINC;
	m_inputMenu.joy_move_scale_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_inputMenu.joy_move_scale_amount.generic.y			= y;

	m_inputMenu.joy_look_scale_slider.generic.x			= 0;
	m_inputMenu.joy_look_scale_slider.generic.y			= y += UIFT_SIZEINC;
	m_inputMenu.joy_look_scale_amount.generic.x			= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_inputMenu.joy_look_scale_amount.generic.y			= y;

	m_inputMenu.joy_invert_y_toggle.generic.x			= 0;
	m_inputMenu.joy_invert_y_toggle.generic.y			= y += UIFT_SIZEINC;

	m_inputMenu.joy_trigger_threshold_slider.generic.x	= 0;
	m_inputMenu.joy_trigger_threshold_slider.generic.y	= y += UIFT_SIZEINC;
	m_inputMenu.joy_trigger_threshold_amount.generic.x	= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_inputMenu.joy_trigger_threshold_amount.generic.y	= y;
	m_inputMenu.ui_sensitivity_slider.generic.x		= 0;
	m_inputMenu.ui_sensitivity_slider.generic.y		= y += (UIFT_SIZEINC*2);
	m_inputMenu.ui_sensitivity_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_inputMenu.ui_sensitivity_amount.generic.y		= y;
	m_inputMenu.sensitivity_slider.generic.x		= 0;
	m_inputMenu.sensitivity_slider.generic.y		= y += UIFT_SIZEINC;
	m_inputMenu.sensitivity_amount.generic.x		= (UIFT_SIZE * (SLIDER_RANGE + 5));
	m_inputMenu.sensitivity_amount.generic.y		= y;
	m_inputMenu.maccel_list.generic.x				= 0;
	m_inputMenu.maccel_list.generic.y				= y += UIFT_SIZEINC;
	m_inputMenu.autosensitivity_toggle.generic.x	= 0;
	m_inputMenu.autosensitivity_toggle.generic.y	= y += UIFT_SIZEINC;
	m_inputMenu.invert_mouse_toggle.generic.x		= 0;
	m_inputMenu.invert_mouse_toggle.generic.y		= y += UIFT_SIZEINC;
	m_inputMenu.lookspring_toggle.generic.x			= 0;
	m_inputMenu.lookspring_toggle.generic.y			= y += (UIFT_SIZEINC*2);
	m_inputMenu.lookstrafe_toggle.generic.x			= 0;
	m_inputMenu.lookstrafe_toggle.generic.y			= y += UIFT_SIZEINC;
	m_inputMenu.freelook_toggle.generic.x			= 0;
	m_inputMenu.freelook_toggle.generic.y			= y += UIFT_SIZEINC;
	m_inputMenu.back_action.generic.x				= 0;
	m_inputMenu.back_action.generic.y				= y += UIFT_SIZEINC + UIFT_SIZEINCLG;

	// Render
	UI_DrawInterface (&m_inputMenu.frameWork);
}


/*
=============
UI_InputMenu_f
=============
*/
void UI_InputMenu_f (void)
{
	InputMenu_Init ();
	M_PushMenu (&m_inputMenu.frameWork, InputMenu_Draw, InputMenu_Close, M_KeyHandler);
}

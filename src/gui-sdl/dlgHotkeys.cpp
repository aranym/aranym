/*
 * dlgHotkeys.cpp - dialog for editing Hotkeys settings
 *
 * Copyright (c) 2003-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *
 * gui-sdl original code and ideas borrowed from Hatari emulator
 * disk_image() borrowed from Bochs project, IIRC
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "parameters.h"
#include "sdlgui.h"

enum HOTKEYSDLG {
	box_main,
	box_hotkeys,
	label_hotkeys,
	setup_label,
	setup_key,
	SETUP,
	quit_label,
	quit_key,
	QUIT,
	reboot_label,
	reboot_key,
	REBOOT,
	debug_label,
	debug_key,
	DEBUG,
	screenshot_label,
	screenshot_key,
	SCREENSHOT,
	fullscreen_label,
	fullscreen_key,
	FULLSCREEN,
	APPLY,
	CANCEL
};

static char key_setup[30];
static char key_quit[30];
static char key_reboot[30];
static char key_debug[30];
static char key_screenshot[30];
static char key_fullscreen[30];

/* The hotkeys dialog: */
static SGOBJ hotkeysdlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 40,25, NULL },
	{ SGBOX, 0, 0, 1,2, 38,19, NULL },
	{ SGTEXT, 0, 0, 12,1, 16,1, " Hotkeys Editor " },
	{ SGTEXT, 0, 0, 2,3, 6,1, "Setup" },
	{ SGTEXT, 0, 0, 13,3, 26,1, key_setup },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 3, 6,1, "Change" },
	{ SGTEXT, 0, 0, 2,5, 6,1, "Quit" },
	{ SGTEXT, 0, 0, 13,5, 26,1, key_quit },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 5, 6,1, "Change" },
	{ SGTEXT, 0, 0, 2,7, 6,1, "Reboot" },
	{ SGTEXT, 0, 0, 13,7, 26,1, key_reboot },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 7, 6,1, "Change" },
	{ SGTEXT, 0, 0, 2,9, 6,1, "Debug" },
	{ SGTEXT, 0, 0, 13,9, 26,1, key_debug },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 9, 6,1, "Change" },
	{ SGTEXT, 0, 0, 2,11, 6,1, "ScreenShot" },
	{ SGTEXT, 0, 0, 13,11, 26,1, key_screenshot },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 11, 6,1, "Change" },
	{ SGTEXT, 0, 0, 2,13, 6,1, "FullScreen" },
	{ SGTEXT, 0, 0, 13,13, 26,1, key_fullscreen },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 32, 13, 6,1, "Change" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 8,23, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 28,23, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

static SGOBJ presskeydlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 15,3, NULL },
	{ SGTEXT, 0, 0, 2,1, 11,1, "Press a key" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

extern void SDLGui_DrawDialog(SGOBJ *);

SDL_keysym getKey()
{
	SDL_keysym keysym;
	keysym.sym = SDLK_UNKNOWN;

	SDLGui_DrawDialog(presskeydlg);
	do {
		SDL_Event e = getEvent(hotkeysdlg, NULL);
		if (e.type == SDL_KEYDOWN) {
			keysym.sym = e.key.keysym.sym;
			keysym.mod = e.key.keysym.mod;
			if (keysym.sym >= SDLK_NUMLOCK && keysym.sym <= SDLK_COMPOSE) {
				keysym.sym = SDLK_UNKNOWN;;
			}
		}
	} while(keysym.sym == SDLK_UNKNOWN);

	return keysym;
}

char *show_modifiers(SDLMod mods)
{
	static char buffer[80];
	*buffer = 0;
	if (mods & KMOD_LSHIFT) strcat(buffer, "LS+");
	if (mods & KMOD_RSHIFT) strcat(buffer, "RS+");
	if (mods & KMOD_LCTRL) strcat(buffer, "LC+");
	if (mods & KMOD_RCTRL) strcat(buffer, "RC+");
	if (mods & KMOD_LALT) strcat(buffer, "LA+");
	if (mods & KMOD_RALT) strcat(buffer, "RA+");
	return buffer;
}

#define UPDATE_BUTTON(Button) snprintf(key_ ## Button, sizeof(key_ ## Button), "%s%s", show_modifiers(hotkeys.Button.mod), SDL_GetKeyName(hotkeys.Button.sym));
void Dialog_HotkeysDlg()
{
	int but = 0;
	bx_hotkeys_t hotkeys = bx_options.hotkeys;
	do {
		// show current GUI hotkey
		UPDATE_BUTTON(setup);
		UPDATE_BUTTON(quit);
		UPDATE_BUTTON(reboot);
		UPDATE_BUTTON(debug);
		UPDATE_BUTTON(screenshot);
		UPDATE_BUTTON(fullscreen);
		but = SDLGui_DoDialog(hotkeysdlg);
		switch(but) {
			case SETUP:
				hotkeys.setup = getKey();
				break;
			case QUIT:
				hotkeys.quit = getKey();
				break;
			case REBOOT:
				hotkeys.reboot = getKey();
				break;
			case DEBUG:
				hotkeys.debug = getKey();
				break;
			case SCREENSHOT:
				hotkeys.screenshot = getKey();
				break;
			case FULLSCREEN:
				hotkeys.fullscreen = getKey();
				break;
		}
	} while(but != APPLY && but != CANCEL);
	if (but == APPLY) {
		bx_options.hotkeys = hotkeys;
	}
}

/*
vim:ts=4:sw=4:
*/

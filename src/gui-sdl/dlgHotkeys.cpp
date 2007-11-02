/*
 * dlgHotkeys.cpp - dialog for editing Hotkeys settings
 *
 * Copyright (c) 2003-2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "input.h"

static bx_hotkeys_t hotkeys;

enum HOTKEYSDLG {
	box_main,
	box_hotkeys,
	label_hotkeys,
	SETUP,
	setup_key,
	QUIT,
	quit_key,
	REBOOT,
	reboot_key,
	UNGRAB,
	ungrab_key,
	DEBUG,
	debug_key,
	SCREENSHOT,
	screenshot_key,
	FULLSCREEN,
	fullscreen_key,
	HELP,
	APPLY,
	CANCEL
};

static char key_setup[30];
static char key_quit[30];
static char key_reboot[30];
static char key_ungrab[30];
static char key_debug[30];
static char key_screenshot[30];
static char key_fullscreen[30];

/* The hotkeys dialog: */
static SGOBJ hotkeysdlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 40,22, NULL },
	{ SGBOX, 0, 0, 1,2, 38,16, NULL },
	{ SGTEXT, 0, 0, 12,1, 16,1, " Hotkeys Editor " },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 3, 11,1, "Setup" },
	{ SGTEXT, 0, 0, 14,3, 25,1, key_setup },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 5, 11,1, "Quit" },
	{ SGTEXT, 0, 0, 14,5, 25,1, key_quit },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 7, 11,1, "Reboot" },
	{ SGTEXT, 0, 0, 14,7, 25,1, key_reboot },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 9, 11,1, "Ungrab" },
	{ SGTEXT, 0, 0, 14,9, 25,1, key_ungrab },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 11, 11,1, "Debug" },
	{ SGTEXT, 0, 0, 14,11, 25,1, key_debug },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 13, 11,1, "Screenshot" },
	{ SGTEXT, 0, 0, 14,13, 25,1, key_screenshot },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 15, 11,1, "Fullscreen" },
	{ SGTEXT, 0, 0, 14,15, 25,1, key_fullscreen },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2,20, 6,1, "Help" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 20,20, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 30,20, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

static const char *HELP_TEXT = "Define hotkeys for certain functions:\n"
"\n"
"Setup .... you are using it right now\n"
"Quit ..... quits ARAnyM\n"
"Reboot ... restarts virtual machine\n"
"Ungrab ... release mouse and keyboard\n"
"Debug .... internal debugger\n"
"Screenshot save screen image to file\n"
"Fullscreen switch from/to window mode\n"
"\n"
"LS = Left Shift, LC = Left Ctrl,\n"
"RA = Right Alt, RM = Right Meta.\n"
"\n"
"Shifter + [Enter] key => Shifter only";

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
		SDL_Event e = getEvent();
		if (e.type == SDL_KEYDOWN) {
			keysym.sym = e.key.keysym.sym;
			keysym.mod = (SDLMod)(e.key.keysym.mod & HOTKEYS_MOD_MASK);
			if (keysym.sym >= SDLK_NUMLOCK && keysym.sym <= SDLK_COMPOSE) {
				keysym.sym = SDLK_UNKNOWN;
			}
		}
	} while(keysym.sym == SDLK_UNKNOWN);

	// special hack: Enter key = no key needed, just modifiers
	if (keysym.sym == SDLK_RETURN)
		keysym.sym = SDLK_UNKNOWN;

	return keysym;
}

char *displayKeysym(SDL_keysym keysym, char *buffer)
{
	*buffer = 0;
	SDLMod mods = keysym.mod;
	if (mods & KMOD_LSHIFT) strcat(buffer, "LS+");
	if (mods & KMOD_RSHIFT) strcat(buffer, "RS+");
	if (mods & KMOD_LCTRL) strcat(buffer, "LC+");
	if (mods & KMOD_RCTRL) strcat(buffer, "RC+");
	if (mods & KMOD_LALT) strcat(buffer, "LA+");
	if (mods & KMOD_RALT) strcat(buffer, "RA+");
	if (mods & KMOD_LMETA) strcat(buffer, "LM+");
	if (mods & KMOD_RMETA) strcat(buffer, "RM+");
	if (keysym.sym) {
		strcat(buffer, SDL_GetKeyName(keysym.sym));
	}
	else {
		// mod keys only, remove last plus sign
		int len = strlen(buffer);
		if (len > 0 && buffer[len-1] == '+')
			buffer[len-1] = '\0';
	}
	return buffer;
}

static void Dialog_HotkeysDlg_Init(void)
{
	hotkeys = bx_options.hotkeys;
}

static void Dialog_HotkeysDlg_Confirm(void)
{
	bx_options.hotkeys = hotkeys;
}

static void Dialog_HotkeysDlg_Close(void)
{
}

#define UPDATE_BUTTON(Button) displayKeysym(hotkeys.Button, key_ ## Button)
void Dialog_HotkeysDlg()
{
	int but = 0;

	Dialog_HotkeysDlg_Init();
	do {
		// show current GUI hotkey
		UPDATE_BUTTON(setup);
		UPDATE_BUTTON(quit);
		UPDATE_BUTTON(reboot);
		UPDATE_BUTTON(ungrab);
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
			case UNGRAB:
				hotkeys.ungrab = getKey();
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

			case HELP:
				SDLGui_Alert(HELP_TEXT, ALERT_OK);
				break;
		}
	} while(but != APPLY && but != CANCEL);

	if (but == APPLY) {
		Dialog_HotkeysDlg_Confirm();
	}

	Dialog_HotkeysDlg_Close();
}

/*
vim:ts=4:sw=4:
*/

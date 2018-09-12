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
#include "dlgKeypress.h"
#include "dlgHotkeys.h"
#include "dlgAlert.h"

#define UPDATE_BUTTON(Button) keysymToString(key_ ## Button, &hotkeys.Button)


static char key_setup[HOTKEYS_STRING_SIZE];
static char key_quit[HOTKEYS_STRING_SIZE];
static char key_warmreboot[HOTKEYS_STRING_SIZE];
static char key_coldreboot[HOTKEYS_STRING_SIZE];
static char key_ungrab[HOTKEYS_STRING_SIZE];
static char key_debug[HOTKEYS_STRING_SIZE];
static char key_screenshot[HOTKEYS_STRING_SIZE];
static char key_fullscreen[HOTKEYS_STRING_SIZE];
static char key_sound[HOTKEYS_STRING_SIZE];

/* The hotkeys dialog: */
#define SDLGUI_INCLUDE_HOTKEYSDLG
#include "sdlgui.sdl"

static const char *HELP_TEXT = "Define hotkeys for certain functions:\n"
"\n"
"Setup ....... you are using it right now\n"
"Quit ........ quit ARAnyM\n"
"Warm-Reboot . restart virtual machine\n"
"Cold-Reboot . power-on virtual machine\n"
"Ungrab ...... release mouse and keyboard\n"
"Debug ....... invoke internal debugger\n"
"Screenshot... save screen image to file\n"
"Fullscreen... switch from/to window mode\n"
"Sound........ Toggle sound on/off\n"
"\n"
"LS = Left Shift, LC = Left Ctrl,\n"
"RA = Right Alt, RM = Right Meta.\n"
"\n"
"Shifter + [Enter] key => Shifter only";

DlgHotkeys::DlgHotkeys(SGOBJ *dlg)
	: Dialog(dlg), state(STATE_MAIN), dlgKeypress(NULL)
{
	hotkeys = bx_options.hotkeys;

	// show current GUI hotkey
	UPDATE_BUTTON(setup);
	UPDATE_BUTTON(quit);
	UPDATE_BUTTON(warmreboot);
	UPDATE_BUTTON(coldreboot);
	UPDATE_BUTTON(ungrab);
	UPDATE_BUTTON(debug);
	UPDATE_BUTTON(screenshot);
	UPDATE_BUTTON(fullscreen);
	UPDATE_BUTTON(sound);
}

DlgHotkeys::~DlgHotkeys()
{
}

int DlgHotkeys::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch(return_obj) {
		case SETUP:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_SETUP;
			break;
		case QUIT:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_QUIT;
			break;
		case WARMREBOOT:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_WARMREBOOT;
			break;
		case COLDREBOOT:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_COLDREBOOT;
			break;
		case UNGRAB:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_UNGRAB;
			break;
		case DEBUG:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_DEBUG;
			break;
		case SCREENSHOT:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_SCREENSHOT;
			break;
		case FULLSCREEN:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_FULLSCREEN;
			break;
		case SOUND:
			dlgKeypress = (DlgKeypress *) DlgKeypressOpen();
			SDLGui_Open(dlgKeypress);
			state = STATE_SOUND;
			break;

		case HELP:
			SDLGui_Open(DlgAlertOpen(HELP_TEXT, ALERT_OK));
			break;

		case APPLY:
			confirm();
			/* fall through */
		case CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;
	}

	return retval;
}

void DlgHotkeys::confirm(void)
{
	bx_options.hotkeys = hotkeys;
}

void DlgHotkeys::idle(void)
{
	// show current GUI hotkey
	UPDATE_BUTTON(setup);
	UPDATE_BUTTON(quit);
	UPDATE_BUTTON(warmreboot);
	UPDATE_BUTTON(coldreboot);
	UPDATE_BUTTON(ungrab);
	UPDATE_BUTTON(debug);
	UPDATE_BUTTON(screenshot);
	UPDATE_BUTTON(fullscreen);
	UPDATE_BUTTON(sound);

	/* Force redraw */
	init();
} 

void DlgHotkeys::processResult(void)
{
	switch(state) {
		case STATE_SETUP:
			if (dlgKeypress) {
				hotkeys.setup = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_QUIT:
			if (dlgKeypress) {
				hotkeys.quit = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_WARMREBOOT:
			if (dlgKeypress) {
				hotkeys.warmreboot = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_COLDREBOOT:
			if (dlgKeypress) {
				hotkeys.coldreboot = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_UNGRAB:
			if (dlgKeypress) {
				hotkeys.ungrab = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_DEBUG:
			if (dlgKeypress) {
				hotkeys.debug = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_SCREENSHOT:
			if (dlgKeypress) {
				hotkeys.screenshot = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_FULLSCREEN:
			if (dlgKeypress) {
				hotkeys.fullscreen = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
		case STATE_SOUND:
			if (dlgKeypress) {
				hotkeys.sound = dlgKeypress->getPressedKey();
				dlgKeypress = NULL;
			}
			break;
	}
	state = STATE_MAIN;
}

Dialog *DlgHotkeysOpen(void)
{
	return new DlgHotkeys(hotkeysdlg);
}

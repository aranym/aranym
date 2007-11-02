/*
 * dlgMain.cpp - Main Setup dialog 
 *
 * Copyright (c) 2002-2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "parameters.h"			// load/saveSettings()

#ifdef OS_darwin
	extern void refreshMenuKeys();
#endif

/* The main dialog: */
enum MAINDLG {
	box_main,
	box_setup,
	text_main,
	ABOUT,
	DISCS,
	HOTKEYS,
	KEYBOARD,
	OS,
	VIDEO,
	NETWORK,
	text_conffile,
	LOAD,
	SAVE,
	box_hotkeys,
	text_hotkeys,
	REBOOT,
	SHUTDOWN,
	FULLSCREEN,
	SCREENSHOT,
	HELP,
	CLOSE
};

SGOBJ maindlg[] = {
	{SGBOX, SG_BACKGROUND, 0, 0, 0, 40, 25, NULL},
	{SGBOX, 0, 0, 1, 2, 38, 15, NULL},
	{SGTEXT, 0, 0, 14, 1, 14, 1, " ARAnyM SETUP "},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 3, 17, 1, "About"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 5, 17, 1, "Disks"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 7, 17, 1, "Hotkeys"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 9, 17, 1, "Keyboard + mouse"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 2, 11, 17, 1, "Operating System"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 21, 3, 17, 1, "Video"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 21, 5, 17, 1, "Networking"},
	{SGTEXT, 0, 0, 4, 15, 12, 1, "Config file:"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 17, 15, 6, 1, "Load"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 25, 15, 6, 1, "Save"},
	{SGBOX, 0, 0, 1, 19, 38, 5, NULL},
	{SGTEXT, 0, 0, 9, 18, 22, 1, " Quick Access Buttons "},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 3, 20, 10, 1, "Reboot"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 3, 22, 10, 1, "Shutdown"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 15, 20, 12, 1, "Fullscreen"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 15, 22, 12, 1, "Screenshot"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 29, 20, 8, 1, "Help"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 29, 22, 8, 1, "Close"},
	{-1, 0, 0, 0, 0, 0, 0, NULL}
};

static const char *ABOUT_TEXT =
"               ARAnyM\n"
"               ======\n"
"\n"
"ARAnyM as an Open Source project has\n"
"been developed by a loosely knit team\n"
"of hackers accross the Net. Please see\n"
"the doc file AUTHORS for more info.\n"
"\n"
"This program is free software; you can\n"
"redistribute it and/or modify it under\n"
"the terms of the GNU General Public\n"
"License as published by the Free Soft-\n"
"ware Foundation; either version 2 of\n"
"the License, or (at your option) any\n"
"later version.\n"
"\n"
"This program is distributed in the\n"
"hope that it will be useful, but\n"
"WITHOUT ANY WARRANTY. See the GNU Ge-\n"
"neral Public License for more details.";

static const char *HELP_TEXT = 
"          ARAnyM SETUP menu:\n"
"          ------------------\n"
"Changes must be confirmed with APPLY first. Note that all changes are applied immediately to a running ARAnyM.\n"
"Some changes require system reboot in order to take effect. It's actually safest to always reboot after any change.";

static bool bReboot;
static bool bShutdown;

extern void Dialog_DiscDlg();
extern void Dialog_HotkeysDlg();
extern void Dialog_KeyboardDlg();
extern void Dialog_OsDlg();
extern void Dialog_VideoDlg();
extern void Dialog_NetworkDlg();

static char path[MAX_FILENAME_LENGTH] = "";

void LoadSettings()
{
	if (strlen(path) == 0) {
		strncpy(path, getConfigFile(), sizeof(path));
		path[sizeof(path) - 1] = '\0';
	}
	if (SDLGui_FileSelect(path, false)) {
		loadSettings(path);
	}
}

void SaveSettings()
{
	if (strlen(path) == 0) {
		strncpy(path, getConfigFile(), sizeof(path));
		path[sizeof(path) - 1] = '\0';
	}
	if (SDLGui_FileSelect(path, true)) {
		saveSettings(path);
	}
}

void Dialog_MainDlg()
{
	int retbut;

	bReboot = bShutdown = false;

	bool closeDialog = false;
	do {
		retbut = SDLGui_DoDialog(maindlg);
		switch (retbut) {
		case ABOUT:
			SDLGui_Alert(ABOUT_TEXT, ALERT_OK);
			break;

		case DISCS:
			Dialog_DiscDlg();
			break;

		case KEYBOARD:
			Dialog_KeyboardDlg();
			break;

		case HOTKEYS:
			Dialog_HotkeysDlg();
			break;
		case OS:
			Dialog_OsDlg();
			break;
		case VIDEO:
			Dialog_VideoDlg();
			break;
		case NETWORK:
			Dialog_NetworkDlg();
			break;
		case LOAD:
			LoadSettings();
			break;

		case SAVE:
			// make sure users understand which setting they're saving
			// best by allowing this Save button only after the "Apply" was used.
			SaveSettings();
			break;

		case REBOOT:
			bReboot = true;
			closeDialog = true;
			break;

		case SHUTDOWN:
			bShutdown = true;
			closeDialog = true;
			break;

		case FULLSCREEN:
			bx_options.video.fullscreen = !bx_options.video.fullscreen;
			closeDialog = true;
			break;

		case SCREENSHOT:
			host->hostScreen.makeSnapshot();
			closeDialog = true;
			break;

		case HELP:
			SDLGui_Alert(HELP_TEXT, ALERT_OK);
			break;

		case CLOSE:
			closeDialog = true;
			break;
		}
	}
	while (!closeDialog);

#ifdef OS_darwin
	refreshMenuKeys();
#endif
}

/*-----------------------------------------------------------------------*/

int GUImainDlg()
{
	Dialog_MainDlg();
	if (bReboot)
		return STATUS_REBOOT;
	else if (bShutdown)
		return STATUS_SHUTDOWN;

	return 0;
}

/*
vim:ts=4:sw=4:
*/

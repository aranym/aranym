#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "parameters.h"			// load/saveSettings()

#define Screen_Save()		{ hostScreen.lock(); hostScreen.saveBackground(); hostScreen.unlock(); }
#define Screen_SetFullUpdate()
#define Screen_Draw()		{ hostScreen.lock(); hostScreen.restoreBackground(); hostScreen.unlock(); }

/* The main dialog: */
enum MAINDLG {
	box_main,
	box_setup,
	text_main,
	ABOUT,
	DISCS,
	HOTKEYS,
	KEYBOARD,
	TOS,
	VIDEO,
	MEMORY,
	HOSTFS,
	CDROM,
	INOUT,
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
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 3, 3, 15, 1, "About"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 3, 5, 15, 1, "Disks"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 3, 7, 15, 1, "Hotkeys"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 3, 9, 15, 1, "Keyboard&mouse"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 3, 11, 15, 1, "TOS"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 3, 15, 1, "Video"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 5, 15, 1, "JIT CPU"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 7, 15, 1, "Host FS"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 9, 15, 1, "Host CD/DVD"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 11, 15, 1, "Input/Output"},
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

	Screen_Save();

	hostScreen.lock();
	SDL_ShowCursor(SDL_ENABLE);
	hostScreen.unlock();

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
		case TOS:
		case VIDEO:
		case MEMORY:
		case HOSTFS:
		case CDROM:
		case INOUT:
			SDLGui_Alert("Unimplemented yet.", ALERT_OK);
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
			hostScreen.toggleFullScreen();
			closeDialog = true;
			break;

		case SCREENSHOT:
			hostScreen.makeSnapshot();
			closeDialog = true;
			break;

		case HELP:
			SDLGui_Alert(HELP_TEXT, ALERT_OK);
			break;

		case CLOSE:
			closeDialog = true;
			break;
		}
		Screen_SetFullUpdate();
		Screen_Draw();
	}
	while (!closeDialog);

	hostScreen.lock();
	SDL_ShowCursor(SDL_DISABLE);
	hostScreen.unlock();
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

#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "parameters.h"			// load/saveSettings()

#define Screen_Save()		{ hostScreen.lock(); hostScreen.saveBackground(); hostScreen.unlock(); }
#define Screen_SetFullUpdate()
#define Screen_Draw()		{ hostScreen.lock(); hostScreen.restoreBackground(); hostScreen.unlock(); }

extern int Dialog_AlertDlg(const char *);

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

static bool bReboot;
static bool bShutdown;

extern void Dialog_AboutDlg();
extern void Dialog_DiscDlg();
extern void Dialog_HotkeysDlg();
extern void Dialog_KeyboardDlg();
extern void Dialog_HelpDlg();

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
			Dialog_AboutDlg();
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
			Dialog_AlertDlg("Unimplemented yet.");
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
			Dialog_HelpDlg();
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

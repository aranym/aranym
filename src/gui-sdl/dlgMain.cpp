#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "parameters.h"			// load/saveSettings()

#define Screen_Save()		{ hostScreen.lock(); hostScreen.saveBackground(); hostScreen.unlock(); }
#define Screen_SetFullUpdate()
#define Screen_Draw()		{ hostScreen.lock(); hostScreen.restoreBackground(); hostScreen.unlock(); }

bx_options_t gui_options;

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
	text_apply,
	APPLY,
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
	{SGTEXT, 0, 0, 14, 1, 12, 1, "ARAnyM SETUP"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 4, 3, 14, 1, "About"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 4, 5, 14, 1, "Disks"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 4, 7, 14, 1, "Hotkeys"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 4, 9, 14, 1, "Keyboard"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 4, 11, 14, 1, "TOS"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 3, 14, 1, "Video"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 5, 14, 1, "JIT CPU"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 7, 14, 1, "Host FS"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 9, 14, 1, "Host CD/DVD"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 22, 11, 14, 1, "Input/Output"},
	{SGTEXT, 0, 0, 4, 13, 26, 1, "To activate changes click"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 30, 13, 6, 1, "Apply"},
	{SGTEXT, 0, 0, 4, 15, 12, 1, "Config file:"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 17, 15, 6, 1, "Load"},
	{SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 25, 15, 6, 1, "Save"},
	{SGBOX, 0, 0, 1, 19, 38, 5, NULL},
	{SGTEXT, 0, 0, 10, 18, 20, 1, "Quick Access Buttons"},
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

	// preload bx settings
	gui_options = bx_options;

	Screen_Save();

	if (SDLGui_PrepareFont() == -1)
		return;

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

		case HOTKEYS:
		case KEYBOARD:
		case TOS:
		case VIDEO:
		case MEMORY:
		case HOSTFS:
		case CDROM:
		case INOUT:
			Dialog_KeyboardDlg();
			break;

		case LOAD:
			LoadSettings();
			gui_options = bx_options;	// preload bx settings
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

		case APPLY:
			bx_options = gui_options;	// apply bx settings
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

	SDLGui_FreeFont();
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

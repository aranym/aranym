#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "parameters.h"		// load/saveSettings()

#define Screen_Save()		{ hostScreen.lock(); hostScreen.saveBackground(); hostScreen.unlock(); }
#define Screen_SetFullUpdate()
#define Screen_Draw()		{ hostScreen.lock(); hostScreen.restoreBackground(); hostScreen.unlock(); }

bx_options_t gui_options;

/* The main dialog: */
enum MAINDLG {
	box_main,
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
	text_aranym,
	REBOOT,
	SHUTDOWN,
	text_config,
	LOAD,
	SAVE,
	APPLY,
	CLOSE
};

SGOBJ maindlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGTEXT, 0, 0, 12,1, 16,1, "ARAnyM main menu" },
  { SGBUTTON, 0, 0, 4,4, 14,1, "About" },
  { SGBUTTON, 0, 0, 4,6, 14,1, "Disks" },
  { SGBUTTON, 0, 0, 4,8, 14,1, "Hotkeys" },
  { SGBUTTON, 0, 0, 4,10, 14,1, "Keyboard" },
  { SGBUTTON, 0, 0, 4,12, 14,1, "TOS" },
  { SGBUTTON, 0, 0, 22,4, 14,1, "Video" },
  { SGBUTTON, 0, 0, 22,6, 14,1, "JIT CPU" },
  { SGBUTTON, 0, 0, 22,8, 14,1, "Host FS" },
  { SGBUTTON, 0, 0, 22,10, 14,1, "CD-ROM" },
  { SGBUTTON, 0, 0, 22,12, 14,1, "Input/Output" },
  { SGTEXT, 0, 0, 4,18, 10,1, "ARAnyM:" },
  { SGBUTTON, 0, 0, 2,20, 10,1, "Reboot" },
  { SGBUTTON, 0, 0, 2,22, 10,1, "Shutdown" },
  { SGTEXT, 0, 0, 17,18, 7,1, "Config:" },
  { SGBUTTON, 0, 0, 17,20, 7,1, "Load" },
  { SGBUTTON, 0, 0, 17,22, 7,1, "Save" },
  { SGBUTTON, 0, 0, 30,20, 8,1, "Apply" },
  { SGBUTTON, 0, 0, 30,22, 8,1, "Close" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

static bool bReboot;
static bool bShutdown;

extern void Dialog_AboutDlg();
extern void Dialog_DiscDlg();
extern void Dialog_KeyboardDlg();

static char path[MAX_FILENAME_LENGTH]="";

void LoadSettings()
{
	if (strlen(path) == 0) {
		strncpy(path, getConfigFile(), sizeof(path));
		path[sizeof(path)-1] = '\0';
	}
	if (SDLGui_FileSelect(path, false)) {
		loadSettings(path);
	}
}

void SaveSettings()
{
	if (strlen(path) == 0) {
		strncpy(path, getConfigFile(), sizeof(path));
		path[sizeof(path)-1] = '\0';
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
  do
  {
    retbut = SDLGui_DoDialog(maindlg);
    switch(retbut)
    {
      case ABOUT:
      	Dialog_AboutDlg();
        break;

      case DISCS:
        Dialog_DiscDlg();
        break;

      case KEYBOARD:
        Dialog_KeyboardDlg();
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

      case APPLY:
        // apply bx settings
        bx_options = gui_options;
        break;

	  case CLOSE:
        closeDialog = true;
	  	break;
    }
    Screen_SetFullUpdate();
    Screen_Draw();
  }
  while(!closeDialog);

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

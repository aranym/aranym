#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "parameters.h"		// load/saveSettings()

#define Screen_Save()		{ hostScreen.lock(); hostScreen.saveBackground(); hostScreen.unlock(); }
#define Screen_SetFullUpdate()
#define Screen_Draw()		{ hostScreen.lock(); hostScreen.restoreBackground(); hostScreen.unlock(); }

/* The main dialog: */
enum {
	maindlg_border,
	maindlg_text_main,
	MAINDLG_ABOUT,
	MAINDLG_DISCS,
	MAINDLG_KEYBD,
	maindlg_text_aranym,
	MAINDLG_REBOOT,
	MAINDLG_SHUTDOWN,
	maindlg_text_config,
	MAINDLG_LOAD,
	MAINDLG_SAVE,
	MAINDLG_OK,
	MAINDLG_CANCEL
};

SGOBJ maindlg[] =
{
  { SGBOX, 0, 0, 0,0, 36,20, NULL },
  { SGTEXT, 0, 0, 10,1, 16,1, "ARAnyM main menu" },
  { SGBUTTON, 0, 0, 4,4, 12,1, "About" },
  { SGBUTTON, 0, 0, 4,6, 12,1, "Disks" },
  { SGBUTTON, 0, 0, 4,8, 12,1, "Keyboard" },
/*
  { SGBUTTON, 0, 0, 4,10, 12,1, "Screen" },
  { SGBUTTON, 0, 0, 4,12, 12,1, "Sound" },
  { SGBUTTON, 0, 0, 20,4, 12,1, "CPU" },
  { SGBUTTON, 0, 0, 20,6, 12,1, "Memory" },
  { SGBUTTON, 0, 0, 20,8, 12,1, "Joysticks" },
  { SGBUTTON, 0, 0, 20,10, 12,1, "Keyboard" },
  { SGBUTTON, 0, 0, 20,12, 12,1, "Devices" },
*/
  { SGTEXT, 0, 0, 4,14, 10,1, "ARAnyM:" },
  { SGBUTTON, 0, 0, 2,16, 10,1, "Reboot" },
  { SGBUTTON, 0, 0, 2,18, 10,1, "Shutdown" },
  { SGTEXT, 0, 0, 15,14, 7,1, "Config:" },
  { SGBUTTON, 0, 0, 15,16, 7,1, "Load" },
  { SGBUTTON, 0, 0, 15,18, 7,1, "Save" },
  { SGBUTTON, 0, 0, 26,16, 8,1, "OK" },
  { SGBUTTON, 0, 0, 26,18, 8,1, "Cancel" },
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

int Dialog_MainDlg()
{
  int retbut;

  bReboot = bShutdown = false;

  Screen_Save();

  if (SDLGui_PrepareFont() == -1)
  	return 0;

  hostScreen.lock();
  SDL_ShowCursor(SDL_ENABLE);
  hostScreen.unlock();

  do
  {
    retbut = SDLGui_DoDialog(maindlg);
    switch(retbut)
    {
      case MAINDLG_ABOUT:
      	Dialog_AboutDlg();
        break;

      case MAINDLG_DISCS:
        Dialog_DiscDlg();
        break;

      case MAINDLG_KEYBD:
        Dialog_KeyboardDlg();
        break;

      case MAINDLG_LOAD:
        LoadSettings();
      	break;

      case MAINDLG_SAVE:
        SaveSettings();
      	break;

      case MAINDLG_REBOOT:
        bReboot = true;
        break;

      case MAINDLG_SHUTDOWN:
        bShutdown = true;
        break;
    }
    Screen_SetFullUpdate();
    Screen_Draw();
  }
  while(retbut!=MAINDLG_OK && retbut!=MAINDLG_CANCEL && !bShutdown && !bReboot);

  hostScreen.lock();
  SDL_ShowCursor(SDL_DISABLE);
  hostScreen.unlock();

  SDLGui_FreeFont();

  return(retbut==MAINDLG_OK);
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

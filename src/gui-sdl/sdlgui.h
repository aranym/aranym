/*
  Hatari

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Header for the tiny graphical user interface for Hatari.
*/

#ifndef _SDLGUI_H
#define _SDLGUI_H

#include "host.h"

#include <SDL.h>

class HostSurface;
class Dialog;

enum
{
  SGBOX,
  SGTEXT,
  SGEDITFIELD,
  SGBUTTON,
  SGCHECKBOX,
  SGPOPUP
};


/* Object flags: */
#define SG_TOUCHEXIT     1
#define SG_EXIT          2
#define SG_BUTTON_RIGHT  4
#define SG_DEFAULT       8
#define SG_SELECTABLE   16
#define SG_BACKGROUND   32
#define SG_RADIO        64

/* Object states: */
#define SG_SELECTED   1
#define SG_HIDDEN     2
#define SG_DISABLED   4

/* Special characters: */
#define SGCHECKBOX_RADIO_NORMAL   12
#define SGCHECKBOX_RADIO_NORMAl   13
#define SGCHECKBOX_RADIO_SELECTED 14
#define SGCHECKBOX_RADIO_SELECTEd 15
#define SGCHECKBOX_NORMAL         28
#define SGCHECKBOX_NORMAl         29
#define SGCHECKBOX_SELECTED       30
#define SGCHECKBOX_SELECTEd       31
#define SGARROWUP    1
#define SGARROWDOWN  2
#define SGFOLDER     5


typedef struct
{
  int type;             /* What type of object */
  int flags;            /* Object flags */
  int state;		/* Object state */
  int x, y;             /* The offset to the upper left corner */
  unsigned int w, h;             /* Width and height */
  char *txt;            /* Text string */
}  SGOBJ;

typedef struct
{
  int object;
  int position;
  int blink_counter;
  bool blink_state;
} cursor_state;

enum
{
  SG_FIRST_EDITFIELD,
  SG_PREVIOUS_EDITFIELD,
  SG_NEXT_EDITFIELD,
  SG_LAST_EDITFIELD
};

bool SDLGui_Init(void);
int SDLGui_UnInit(void);
int SDLGui_DoDialog(SGOBJ *dlg);
int SDLGui_PrepareFont(void);
void SDLGui_FreeFont(void);
int SDLGui_FileSelect(char *path_and_name, bool bAllowNew);
SDL_Rect *SDLGui_GetFirstBackgroundRect(void);
SDL_Rect *SDLGui_GetNextBackgroundRect(void);

#define STATUS_REBOOT	1
#define STATUS_SHUTDOWN	2
int GUImainDlg();

typedef enum { ALERT_OK, ALERT_OKCANCEL } alert_type;
extern bool SDLGui_Alert(const char *, alert_type type);

void SDLGui_setGuiPos(int guix, int guiy);
HostSurface *SDLGui_getSurface(void);

/* dlgHotkeys.cpp */
char *displayKeysym(SDL_keysym keysym, char *buffer);

/* stuff needed by dialog.cpp */
int SDLGui_FindEditField(SGOBJ *dlg, int objnum, int mode);
void SDLGui_DrawDialog(SGOBJ *dlg);
int SDLGui_FindObj(SGOBJ *dlg, int fx, int fy);
bool SDLGui_UpdateObjState(SGOBJ *dlg, int clicked_obj, int original_state,
                           int x, int y);
void SDLGui_SelectRadioObject(SGOBJ *dlg, int clicked_obj);
void SDLGui_ClickEditField(SGOBJ *dlg, cursor_state *cursor, int clicked_obj, int x);
void SDLGui_DrawObject(SGOBJ *dlg, int objnum);
void SDLGui_RefreshObj(SGOBJ *dlg, int objnum);
int SDLGui_FindDefaultObj(SGOBJ *dlg);
void SDLGui_MoveCursor(SGOBJ *dlg, cursor_state *cursor, int mode);
void SDLGui_DeselectButtons(SGOBJ *dlg);

int SDLGui_DoEvent(const SDL_Event &event);
void SDLGui_Open(Dialog *new_dlg);
void SDLGui_Close(void);
bool SDLGui_isClosed(void);

#endif /* _SDLGUI_H */

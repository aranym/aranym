/*
  Hatari

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Header for the tiny graphical user interface for Hatari.
*/

#ifndef _SDLGUI_H
#define _SDLGUI_H

#include "host.h"

#include "SDL_compat.h"

class HostSurface;
class Dialog;

enum
{
  SGBOX,
  SGTEXT,
  SGEDITFIELD,
  SGBUTTON,
  SGRADIOBUT,
  SGCHECKBOX,
  SGPOPUP,
  SGSCROLLBAR
};


/* Object flags: */
#define SG_TOUCHEXIT    0x0001   /* Exit immediately when mouse button is pressed down */
#define SG_EXIT         0x0002   /* Exit when mouse button has been pressed (and released) */
#define SG_BUTTON_RIGHT 0x0004   /* Text is aligned right */
#define SG_DEFAULT      0x0008   /* Marks a default button, selectable with Enter & Return keys */
#define SG_CANCEL       0x0010   /* Marks a cancel button, selectable with ESC key */
#define SG_SHORTCUT     0x0020   /* Marks a shortcut button, selectable with masked letter */
#define SG_SELECTABLE   0x0100
#define SG_BACKGROUND   0x0200
#define SG_RADIO        0x0400
#define SG_SMALLTEXT    0x0800

/* Object states: */
#define SG_SELECTED    0x0001
#define SG_MOUSEDOWN   0x0002
#define SG_FOCUSED     0x0004   /* Marks an object that has selection focus */
#define SG_WASFOCUSED  0x0008   /* Marks an object that had selection focus & its bg needs redraw */
#define SG_HIDDEN      0x0100
#define SG_DISABLED    0x0200

/* special shortcut keys, something that won't conflict with text shortcuts */
#define SG_SHORTCUT_LEFT	'<'
#define SG_SHORTCUT_RIGHT	'>'
#define SG_SHORTCUT_UP  	'^'
#define SG_SHORTCUT_DOWN	'|'

/* Special characters: */
#define SGARROWUP                1
#define SGARROWDOWN              2
#define SGFOLDER                 5


typedef struct
{
  int type;             /* What type of object */
  int flags;            /* Object flags */
  int state;            /* Object state */
  int x, y;             /* The offset to the upper left corner */
  unsigned int w, h;    /* Width and height (for scrollbar : height and position) */
  const char *txt;      /* Text string */
  int shortcut;         /* shortcut key */
}  SGOBJ;

typedef struct
{
  int object;
  int position;
  Uint32 blink_counter;
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
void SDLGui_DeselectAndRedraw(SGOBJ *dlg, int obj);
int SDLGui_TextLen(const char *txt);
int SDLGui_ByteLen(const char *txt, int pos);

int SDLGui_DoEvent(const SDL_Event &event);
void SDLGui_Open(Dialog *new_dlg);
void SDLGui_Close(void);
bool SDLGui_isClosed(void);

#endif /* _SDLGUI_H */

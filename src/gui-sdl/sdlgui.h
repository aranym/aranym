/*
  Hatari

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Header for the tiny graphical user interface for Hatari.
*/

#ifndef _SDLGUI_H
#define _SDLGUI_H

#include <SDL.h>
#include "host.h"

enum
{
  SGBOX,
  SGTEXT,
  SGEDITFIELD,
  SGBUTTON,
  SGRADIOBUT,
  SGCHECKBOX,
  SGPOPUP
};


/* Object flags: */
#define SG_TOUCHEXIT    1
#define SG_EXIT         2  /* Not yet tested */
#define SG_BUTTON_RIGHT 4
#define SG_DEFAULT      8

/* Object states: */
#define SG_SELECTED   1
#define SG_HIDDEN     2

/* Special characters: */
#define SGRADIOBUTTON_NORMAL    12
#define SGRADIOBUTTON_SELECTED  13
#define SGCHECKBOX_NORMAL    14
#define SGCHECKBOX_SELECTED  15
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

#define SCRLOCK	hostScreen.lock()
#define SCRUNLOCK	hostScreen.unlock()

bool SDLGui_Init(void);
int SDLGui_UnInit(void);
int SDLGui_DoDialog(SGOBJ *dlg);
int SDLGui_PrepareFont(void);
void SDLGui_FreeFont(void);
int SDLGui_FileSelect(char *path_and_name, bool bAllowNew);

#define STATUS_REBOOT	1
#define STATUS_SHUTDOWN	2
int GUImainDlg();

#endif /* _SDLGUI_H */

/*
  Hatari

  Header for the tiny graphical user interface for Hatari.
*/

#ifndef _SDLGUI_H
#define _SDLGUI_H

#include <SDL/SDL.h>

enum
{
  SGBOX,
  SGTEXT,
  SGBUTTON,
  SGRADIOBUT,
  SGCHECKBOX,
  SGPOPUP
};


/* Object flags: */
#define SG_TOUCHEXIT  1
#define SG_EXIT       2  /* Not yet tested */

/* Object states: */
#define SG_SELECTED   1


typedef struct
{
  int type;             /* What type of object */
  int flags;            /* Object flags */
  int state;		/* Object state */
  int x, y;             /* The offset to the upper left corner */
  int w, h;             /* Width and height */
  char *txt;            /* Text string */
}  SGOBJ;


bool SDLGui_Init(void);
int SDLGui_UnInit(void);
int SDLGui_DoDialog(SGOBJ *dlg);
int SDLGui_PrepareFont(void);
void SDLGui_CenterDlg(SGOBJ *dlg);
int SDLGui_FileSelect(char *path_and_name);

#endif /* _SDLGUI_H */

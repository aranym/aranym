#include "sdlgui.h"

/* The keyboard dialog: */
SGOBJ keyboarddlg[] =
{
  { SGBOX, 0, 0, 0,0, 30,8, NULL },
  { SGTEXT, 0, 0, 2,4, 25,1, "Sorry, not yet supported." },
  { SGBUTTON, SG_EXIT|SG_DEFAULT, 0, 5,6, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

void Dialog_KeyboardDlg()
{
  SDLGui_DoDialog(keyboarddlg);
}

/*
  ARAnyM - dlgHelp.c

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  Show information about the program and its license.
*/

#include "sdlgui.h"

/* The "Help"-dialog: */
SGOBJ helpdlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGTEXT, 0, 0, 12,1, 16,1, "ARAnyM Help Page" },
  { SGTEXT, 0, 0, 12,2, 16,1, "================" },
  { SGTEXT, 0, 0, 1,4, 38,1, "ARAnyM Hotkeys:" },
  { SGTEXT, 0, 0, 1,5, 15,1, "---------------" },
  { SGTEXT, 0, 0, 1,6, 38,1, "  Pause/Break: open up this SETUP" },
  { SGTEXT, 0, 0, 1,7, 38,1, "  Alt+Ctrl+Esc: release mouse and kbd" },
  { SGTEXT, 0, 0, 1,8, 38,1, "" },
  { SGTEXT, 0, 0, 1,10, 38,1, "ARAnyM SETUP menu:" },
  { SGTEXT, 0, 0, 1,11, 18,1,"------------------" },
  { SGTEXT, 0, 0, 1,12, 38,1,"  Changes must be confirmed with APPLY" },
  { SGTEXT, 0, 0, 1,13, 38,1,"  first. Note that all changes are" },
  { SGTEXT, 0, 0, 1,14, 38,1,"  applied immediately to a running" },
  { SGTEXT, 0, 0, 1,15, 38,1,"  ARAnyM." },
  { SGTEXT, 0, 0, 1,16, 38,1,"  Some changes require system" },
  { SGTEXT, 0, 0, 1,17, 38,1,"  reboot in order to take effect." },
  { SGTEXT, 0, 0, 1,18, 38,1,"  It's actually safest to always" },
  { SGTEXT, 0, 0, 1,19, 38,1,"  reboot after any change." },
  { SGBUTTON, SG_EXIT|SG_DEFAULT, 0, 16,23, 8,1, "OK" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};



/*-----------------------------------------------------------------------*/
/*
  Show the "help" dialog:
*/
void Dialog_HelpDlg()
{
  SDLGui_DoDialog(helpdlg);
}

/*
 * dlgHostfs.cpp - dialog for HostFS support
 *
 * Copyright (c) 2003-2009 ARAnyM dev team (see AUTHORS)
 *
 * originally taken from the hatari project, thanks Thothy!
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "dlgHostfs.h"
#include "dlgAlert.h"
#include "dlgFileSelect.h"
#include "nf_base.h"
#include <assert.h>
#include "verify.h"

#define DEBUG 0
#include "debug.h"

#ifdef HOSTFS_SUPPORT

#define DLG_WIDTH			76

#define ENTRY_COUNT 		15 /* (HOSTFSDLG_LASTENTRY - HOSTFSDLG_FIRSTENTRY + 1) */
#define ENTRY_LENGTH		sizeof(bx_options.aranymfs.drive[0].configPath)


static char dlgfilenames[ENTRY_COUNT][ENTRY_LENGTH + 1];
static char dlgdrives[ENTRY_COUNT][3];

/* The dialog data: */
#define SDLGUI_INCLUDE_HOSTFSDLG
#include "sdlgui.sdl"

verify((HOSTFSDLG_LASTENTRY - HOSTFSDLG_FIRSTENTRY + 1) == ENTRY_COUNT);
verify((HOSTFSDLG_LASTCASE - HOSTFSDLG_FIRSTCASE + 1) == ENTRY_COUNT);
verify((HOSTFSDLG_LASTCLEAR - HOSTFSDLG_FIRSTCLEAR + 1) == ENTRY_COUNT);

static const char *HELP_TEXT =
	"For accessing data on a host filesystem you can map the host fs to a logical drive in TOS or MiNT.\n"
	"To assign a drive click on a [Path] and select a directory (or type in a name).\n"
	"\n"
	"You have to reboot Aranym for the settings to take effect.";

/*-----------------------------------------------------------------------*/
/*
  Show and process a file selection dialog.
  Returns TRUE if user selected "OK", FALSE if "cancel".
  bAllowNew: TRUE if the user is allowed to insert new file names.
*/

DlgHostfs::DlgHostfs(SGOBJ *dlg)
	: Dialog(dlg),
	  dlgFileSelect(NULL),
	  selected(0),
	  refreshentries(true),
	  ypos(0),
	  pressed_ok(false),
	  redraw(true)
{
	options = bx_options.aranymfs;
}

void DlgHostfs::confirm(void)
{
	char *ptrLast;
	int len;
	
	for (int i = 0; i < HOSTFS_MAX_DRIVES; i++)
	{
		len = strlen(options.drive[i].rootPath);
		if (len > 0)
		{
			ptrLast = options.drive[i].rootPath + len - 1;
			if (*ptrLast != DIRSEPARATOR[0])
				strcat(options.drive[i].rootPath, DIRSEPARATOR);
		}
		strcpy(options.drive[i].configPath, options.drive[i].rootPath);
		if (len > 0)
		{
			if (options.drive[i].halfSensitive)
				strcat(options.drive[i].configPath, ":");
		}
	}
	bx_options.aranymfs = options;
}

void DlgHostfs::idle(void)
{
	if (refreshentries) {
		refreshEntries();
	}

	/* Force redraw ? */
	if (redraw) {
		init();
		redraw = false;
	}
}

int DlgHostfs::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch (return_obj) {
		case HOSTFSDLG_UP:
			updateEntries();
			/* Scroll up */
			if (ypos > 0) {
				--ypos;
				refreshentries = true;
			}
			break;
		case HOSTFSDLG_DOWN:
			updateEntries();
			/* Scroll down */
			if (ypos < (HOSTFS_MAX_DRIVES - ENTRY_COUNT)) {
				++ypos;
				refreshentries = true;
			}
			break;

		case HELP:
			updateEntries();
			SDLGui_Open(DlgAlertOpen(HELP_TEXT, ALERT_OK));
			break;

		case HOSTFSDLG_OKAY:
			updateEntries();
			pressed_ok = true;
			confirm();
			/* fall through */
		case HOSTFSDLG_CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;

		default:
			break;
	}

	/* Has the user clicked on the path? */
	if ((return_obj >= HOSTFSDLG_FIRSTENTRY) && (return_obj < (HOSTFSDLG_FIRSTENTRY + ENTRY_COUNT))) {
		selected = return_obj - HOSTFSDLG_FIRSTENTRY + ypos;
		strcpy(tmpname, options.drive[selected].rootPath);
		SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, false));
	}

	if ((return_obj >= HOSTFSDLG_FIRSTCASE) && (return_obj < (HOSTFSDLG_FIRSTCASE + ENTRY_COUNT))) {
		options.drive[return_obj - HOSTFSDLG_FIRSTCASE + ypos].halfSensitive = (dlg[return_obj].state & SG_SELECTED) == 0;
	}

	if ((return_obj >= HOSTFSDLG_FIRSTCLEAR) && (return_obj < (HOSTFSDLG_FIRSTCLEAR + ENTRY_COUNT))) {
		selected = return_obj - HOSTFSDLG_FIRSTCLEAR + ypos;
		options.drive[selected].rootPath[0] = '\0';
		dlgfilenames[selected][0] = '\0';
		refreshentries = true;
	}

	if (refreshentries) {
		refreshEntries();
	}

	return retval;
}

void DlgHostfs::refreshEntries(void)
{
	if (refreshentries) {
		int i;

		/* Copy entries to dialog: */
		for (i = 0; i < ENTRY_COUNT; i++) {
			if ((i + ypos) < HOSTFS_MAX_DRIVES) {
				strcpy(dlgfilenames[i], options.drive[i + ypos].rootPath);
				dlg[HOSTFSDLG_FIRSTENTRY + i].flags = (SG_SELECTABLE | SG_EXIT);
				dlg[HOSTFSDLG_FIRSTENTRY + i].type = SGEDITFIELD;
				strcpy(dlgdrives[i], "A:");
				dlgdrives[i][0] = DriveToLetter(i + ypos);
				dlg[HOSTFSDLG_FIRSTDRIVE + i].txt = dlgdrives[i];
				dlg[HOSTFSDLG_FIRSTCASE + i].state = !options.drive[i + ypos].halfSensitive && options.drive[i + ypos].rootPath[0] ? SG_SELECTED : 0;
			} else {
				/* Clear entry */
				dlgfilenames[i][0] = 0;
				dlg[HOSTFSDLG_FIRSTENTRY + i].flags = 0;
				dlg[HOSTFSDLG_FIRSTENTRY + i].type = SGTEXT;
				dlg[HOSTFSDLG_FIRSTDRIVE + i].txt = "";
				dlg[HOSTFSDLG_FIRSTCASE + i].state = 0;
			}
			dlg[HOSTFSDLG_FIRSTENTRY + i].state = 0;
		}

		refreshentries = false;
		redraw = true;
	}
}

void DlgHostfs::updateEntries(void)
{
	int i;
	
	/* Copy entries from dialog to options: */
	for (i = 0; i < ENTRY_COUNT; i++) {
		if ((i + ypos) < HOSTFS_MAX_DRIVES) {
			strcpy(options.drive[i + ypos].rootPath, dlgfilenames[i]);
		}
	}
}

bool DlgHostfs::pressedOk(void)
{
	return pressed_ok;
}

void DlgHostfs::processResult(void)
{
	struct stat st;
	
	if (dlgFileSelect && dlgFileSelect->pressedOk() &&
	    stat(tmpname, &st) == 0 &&
	    S_ISDIR(st.st_mode))
	{
		strcpy(options.drive[selected].rootPath, tmpname);
		refreshentries = true;
	}
	dlgFileSelect = NULL;
}

#else /* Function and variables for when HOSTFS not present */

#define hostfsdlg nohostfsdlg

#define SDLGUI_INCLUDE_NOHOSTFSDLG
#include "sdlgui.sdl"


int DlgHostfs::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	if (return_obj == OK)
		retval = Dialog::GUI_CLOSE;

	return retval;
}


void DlgHostfs::processResult(void)
{
	dlgFileSelect = NULL;
}

DlgHostfs::DlgHostfs(SGOBJ *dlg)
	: Dialog(dlg),
	  dlgFileSelect(NULL),
	  selected(0),
	  refreshentries(false),
	  ypos(0),
	  pressed_ok(false),
	  redraw(false)
{
}

void DlgHostfs::confirm(void)
{
}

void DlgHostfs::idle(void)
{
}

#endif /* HOSTFS_SUPPORT */

DlgHostfs::~DlgHostfs()
{
}

Dialog *DlgHostfsOpen()
{
	return new DlgHostfs(hostfsdlg);
}

// don't remove this modeline with intended formatting for vim:ts=4:sw=4:



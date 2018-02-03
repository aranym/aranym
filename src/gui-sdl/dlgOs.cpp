/*
 * dlgOs.cpp - Operating System dialog 
 *
 * Copyright (c) 2004-2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "dlgOs.h"
#include "dlgFileSelect.h"
#include "dlgAlert.h"

#define DEBUG 0
#include "debug.h"

static char tos_path[sizeof(bx_options.tos.tos_path)];
static char emutos_path[sizeof(bx_options.tos.emutos_path)];
static char snapshot_dir_display[sizeof(bx_options.snapshot_dir)];

#define SDLGUI_INCLUDE_OSDLG
#include "sdlgui.sdl"

DlgOs::DlgOs(SGOBJ *dlg)
	: Dialog(dlg),
	  state(STATE_MAIN),
	  dlgAlert(NULL),
	  dlgFileSelect(NULL)
{
	/* Set up dialog to actual values: */
	tos_options = bx_options.tos;
	osdlg[TOSCONSOLE].state = tos_options.redirect_CON ? SG_SELECTED : 0;
	osdlg[MCH_ARANYM].state = tos_options.cookie_mch == 0x50000 ? SG_SELECTED : 0;
	osdlg[MCH_FALCON].state = tos_options.cookie_mch == 0x30000 ? SG_SELECTED : 0;
	File_ShrinkName(tos_path, tos_options.tos_path, osdlg[MCH_TOS_PATH].w);
	osdlg[MCH_TOS_PATH].txt = tos_path;
	File_ShrinkName(emutos_path, tos_options.emutos_path, osdlg[MCH_EMUTOS_PATH].w);
	osdlg[MCH_EMUTOS_PATH].txt = emutos_path;
	
	strcpy(snapshot_dir, bx_options.snapshot_dir);
	File_ShrinkName(snapshot_dir_display, snapshot_dir, osdlg[SNAPSHOT_DIR].w);
	osdlg[SNAPSHOT_DIR].txt = snapshot_dir_display;
}

DlgOs::~DlgOs()
{
}

int DlgOs::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch (state)
	{
		case STATE_MAIN:
			retval = processDialogMain();
			break;
		case STATE_CONFIRM:
			state = STATE_MAIN;
			if (dlgAlert)
			{
				if (dlgAlert->pressedOk())
				{
					confirm();
					retval = Dialog::GUI_CLOSE;
				}
				delete dlgAlert;
				dlgAlert = NULL;
			}
			break;
		case STATE_FSEL_TOS:
		case STATE_FSEL_EMUTOS:
		case STATE_FSEL_SNAPSHOT_DIR:
			break;
	}

	return retval;
}

int DlgOs::processDialogMain(void)
{
	int retval = Dialog::GUI_CONTINUE;

	D(bug("Os: process dialogmain, return_obj=%d", return_obj));

	switch (return_obj)
	{
		case MCH_TOS_BROWSE:
			strcpy(tmpname, tos_options.tos_path);
			SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, false));
			state = STATE_FSEL_TOS;
			break;

		case MCH_TOS_CLEAR:
			strcpy(tos_options.tos_path, "");
			strcpy(tos_path, "");
			state = STATE_MAIN;
			break;

		case MCH_EMUTOS_BROWSE:
 			strcpy(tmpname, tos_options.emutos_path);
			SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, false));
			state = STATE_FSEL_EMUTOS;
			break;

		case MCH_EMUTOS_CLEAR:
			strcpy(tos_options.emutos_path, "");
			strcpy(emutos_path, "");
			state = STATE_MAIN;
			break;

		case SNAPSHOT_DIR:
 			strcpy(tmpname, snapshot_dir);
			SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, false));
			state = STATE_FSEL_SNAPSHOT_DIR;
			break;
			
		case APPLY:
			if (!File_Exists(tos_options.tos_path) &&
				!File_Exists(tos_options.emutos_path))
			{
				dlgAlert = (DlgAlert *) DlgAlertOpen("No operating system found.\nARAnyM will not be able to boot!\nContinue?", ALERT_OKCANCEL);
				SDLGui_Open(dlgAlert);
				state = STATE_CONFIRM;
			}
			if (!dlgAlert)
			{
				confirm();
				retval = Dialog::GUI_CLOSE;
			}
			break;
		case CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;
	}

	return retval;
}

void DlgOs::processResultTos(void)
{
	if (dlgFileSelect && dlgFileSelect->pressedOk())
	{
		strcpy(tos_options.tos_path, tmpname);
		File_ShrinkName(tos_path, tmpname, osdlg[MCH_TOS_PATH].w);
	}
}

void DlgOs::processResultEmutos(void)
{
	if (dlgFileSelect && dlgFileSelect->pressedOk())
	{
		strcpy(tos_options.emutos_path, tmpname);
		File_ShrinkName(emutos_path, tmpname, osdlg[MCH_EMUTOS_PATH].w);
	}
}

void DlgOs::processResultSnapshotDir(void)
{
	struct stat st;
	
	if (dlgFileSelect && dlgFileSelect->pressedOk() &&
	    stat(tmpname, &st) == 0 &&
	    S_ISDIR(st.st_mode))
	{
		strcpy(snapshot_dir, tmpname);
		File_ShrinkName(snapshot_dir_display, tmpname, osdlg[SNAPSHOT_DIR].w);
	}
}

void DlgOs::processResult(void)
{
	D(bug("Os: process result, state=%d", state));

	switch (state)
	{
		case STATE_FSEL_TOS:
			processResultTos();
			dlgFileSelect = NULL;
			state = STATE_MAIN;
			break;
		case STATE_FSEL_EMUTOS:
			processResultEmutos();
			dlgFileSelect = NULL;
			state = STATE_MAIN;
			break;
		case STATE_FSEL_SNAPSHOT_DIR:
			processResultSnapshotDir();
			dlgFileSelect = NULL;
			state = STATE_MAIN;
			break;
		case STATE_CONFIRM:
			break;
		default:
			dlgFileSelect = NULL;
			state = STATE_MAIN;
			break;
	}
}

void DlgOs::confirm(void)
{
	tos_options.redirect_CON = (osdlg[TOSCONSOLE].state & SG_SELECTED);
	tos_options.cookie_mch = (osdlg[MCH_ARANYM].state & SG_SELECTED) ? 0x50000 : 0x30000;
	bx_options.tos = tos_options;
	strcpy(bx_options.snapshot_dir, snapshot_dir);
}

Dialog *DlgOsOpen(void)
{
	return new DlgOs(osdlg);
}

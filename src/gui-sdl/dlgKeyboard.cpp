/*
 * dlgKeyboard.cpp - Keyboard dialog 
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
#include "dlgKeyboard.h"

/* The keyboard dialog: */
#define SDLGUI_INCLUDE_KEYBOARDDLG
#include "sdlgui.sdl"


DlgKeyboard::DlgKeyboard(SGOBJ *dlg)
	: Dialog(dlg)
{
	keyboarddlg[EIFFEL].state = bx_options.ikbd.wheel_eiffel ? SG_SELECTED : 0;
	keyboarddlg[ARROWKEYS].state = bx_options.ikbd.wheel_eiffel ? 0 : SG_SELECTED;
	keyboarddlg[MILAN_ALTGR].state = bx_options.ikbd.altgr ? SG_SELECTED : 0;
	keyboarddlg[ATARI_ALT].state = bx_options.ikbd.altgr ? 0 : SG_SELECTED;
}

DlgKeyboard::~DlgKeyboard()
{
}

int DlgKeyboard::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch(return_obj) {
		case APPLY:
			confirm();
			/* fall through */
		case CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;
	}

	return retval;
}

void DlgKeyboard::confirm(void)
{
	bx_options.ikbd.wheel_eiffel = (keyboarddlg[EIFFEL].state & SG_SELECTED);
	bx_options.ikbd.altgr = (keyboarddlg[MILAN_ALTGR].state & SG_SELECTED);
}

Dialog *DlgKeyboardOpen(void)
{
	return new DlgKeyboard(keyboarddlg);
}

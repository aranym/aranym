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

enum KEYBMOUSEDLG {
	box_main,
	box_wheel,
	text_wheel,
	ARROWKEYS,
	EIFFEL,
	box_altgr,
	text_altgr,
	ATARI_ALT,
	MILAN_ALTGR,
	APPLY,
	CANCEL
};

/* The keyboard dialog: */
static SGOBJ keyboarddlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 42,13, NULL },

	{ SGBOX, 0, 0, 2,2, 38,2, NULL },
	{ SGTEXT, 0, 0, 3,1, 18,1, " Host mouse wheel " },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 3,3, 10+3,1, "Arrow keys" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 19,3, 16+3,1, "Eiffel scancodes" },

	{ SGBOX, 0, 0, 2,7, 38,2, NULL },
	{ SGTEXT, 0, 0, 3,6, 16,1, " Host AltGr key " },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 3,8, 9+3,1, "Atari Alt" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 19,8, 11+3,1, "Milan AltGr" },

	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 7,11, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 28,11, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

DlgKeyboard::DlgKeyboard(SGOBJ *dlg)
	: Dialog(dlg)
{
	keyboarddlg[bx_options.ikbd.wheel_eiffel ? EIFFEL : ARROWKEYS].state |= SG_SELECTED;
	keyboarddlg[bx_options.ikbd.altgr ? MILAN_ALTGR : ATARI_ALT].state |= SG_SELECTED;
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

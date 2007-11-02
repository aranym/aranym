/*
 * dlgKeyboard.cpp - Keyboard dialog 
 *
 * Copyright (c) 2004-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 40,25, NULL },
	{ SGBOX, 0, 0, 1,2, 38,3, NULL },
	{ SGTEXT, 0, 0, 2,3, 17,1, "Host mouse wheel:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,2, 20,1, "Arrow keys" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,4, 20,1, "Eiffel scancodes" },
	{ SGBOX, 0, 0, 1,6, 38,3, NULL },
	{ SGTEXT, 0, 0, 2,7, 17,1, "Host AltGr key:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,6, 20,1, "Atari Alt" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,8, 20,1, "Milan AltGr" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 8,23, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 28,23, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

static void Dialog_KeyboardDlg_Init(void)
{
	keyboarddlg[bx_options.ikbd.wheel_eiffel ? EIFFEL : ARROWKEYS].state |= SG_SELECTED;
	keyboarddlg[bx_options.ikbd.altgr ? MILAN_ALTGR : ATARI_ALT].state |= SG_SELECTED;
}

static void Dialog_KeyboardDlg_Close(int but)
{
	if (but!=APPLY) {
		return;
	}

	bx_options.ikbd.wheel_eiffel = (keyboarddlg[EIFFEL].state & SG_SELECTED);
	bx_options.ikbd.altgr = (keyboarddlg[MILAN_ALTGR].state & SG_SELECTED);
}

void Dialog_KeyboardDlg()
{
	Dialog_KeyboardDlg_Init();
	int but = SDLGui_DoDialog(keyboarddlg);
	Dialog_KeyboardDlg_Close(but);
}

/*
vim:ts=4:sw=4:
*/

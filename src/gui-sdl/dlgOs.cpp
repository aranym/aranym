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

enum OSDLG {
	box_main,
	box_wheel,
	text_wheel,
	TOSCONSOLE,
	text_mch,
	MCH_ARANYM,
	MCH_FALCON,
	APPLY,
	CANCEL
};

static SGOBJ osdlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 40,25, NULL },
	{ SGBOX, 0, 0, 1,2, 38,3, NULL },
	{ SGTEXT, 0, 0, 2,3, 12,1, "TOS patches:" },
	{ SGCHECKBOX, SG_SELECTABLE, 0, 15,2, 22,1, "CON: redirect > stdout" },
	{ SGTEXT, 0, 0, 15,4, 5,1, "_MCH:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 21,4, 6,1, "ARAnyM" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 31,4, 6,1, "Falcon" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 8,23, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 28,23, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

void Dialog_OsDlg()
{
	// init
	osdlg[TOSCONSOLE].state = bx_options.tos.redirect_CON ? SG_SELECTED : 0;
	osdlg[MCH_ARANYM].state = bx_options.tos.cookie_mch == 0x50000 ? SG_SELECTED : 0;
	osdlg[MCH_FALCON].state = bx_options.tos.cookie_mch == 0x30000 ? SG_SELECTED : 0;
	// apply
	if (SDLGui_DoDialog(osdlg) == APPLY) {
		bx_options.tos.redirect_CON = (osdlg[TOSCONSOLE].state & SG_SELECTED);
		bx_options.tos.cookie_mch = (osdlg[MCH_ARANYM].state & SG_SELECTED) ? 0x50000 : 0x30000;
	}
}

/*
vim:ts=4:sw=4:
*/

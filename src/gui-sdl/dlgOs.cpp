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
#include "dlgOs.h"

#define SDLGUI_INCLUDE_OSDLG
#include "sdlgui.sdl"

DlgOs::DlgOs(SGOBJ *dlg)
	: Dialog(dlg)
{
	osdlg[TOSCONSOLE].state = bx_options.tos.redirect_CON ? SG_SELECTED : 0;
	osdlg[MCH_ARANYM].state = bx_options.tos.cookie_mch == 0x50000 ? SG_SELECTED : 0;
	osdlg[MCH_FALCON].state = bx_options.tos.cookie_mch == 0x30000 ? SG_SELECTED : 0;
}

DlgOs::~DlgOs()
{
}

int DlgOs::processDialog(void)
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

void DlgOs::confirm(void)
{
	bx_options.tos.redirect_CON = (osdlg[TOSCONSOLE].state & SG_SELECTED);
	bx_options.tos.cookie_mch = (osdlg[MCH_ARANYM].state & SG_SELECTED) ? 0x50000 : 0x30000;
}

Dialog *DlgOsOpen(void)
{
	return new DlgOs(osdlg);
}

/*
 * dlgUsb.h - USB selection dialog
 *
 * Copyright (c) 2012 David Galvez. ARAnyM development team (see AUTHORS).
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

#ifndef DLGUSB_H
#define DLGUSB_H

#define MAX_NUMBER_VIRT_DEV	9
#define MAX_PRODUCT_LENGTH	32


#include "dialog.h"
#include "../natfeat/usbhost.h"

#ifdef USBHOST_SUPPORT
void enable_buttons(void);
void disable_buttons(void);	

class DlgAlert;
#endif

class DlgUsb: public Dialog
{
	private:
		enum {
			STATE_MAIN,
			STATE_ALERT,
		};
		int state;

		DlgAlert *dlgAlert;

		void confirm(void);
 
	public:
		DlgUsb(SGOBJ *dlg);
		~DlgUsb();

		int processDialog(void);
};

Dialog *DlgUsbOpen(void);

#endif /* DLGUSB_H */

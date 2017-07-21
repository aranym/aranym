/*
 * dlgUsb.cpp - USB selection dialog
 *
 * Copyright (c) 2012-2015 David Galvez. ARAnyM development team (see AUTHORS).
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
#include "dlgAlert.h"
#include "dlgUsb.h"

#define DEBUG 0
#include "debug.h"

#ifdef USBHOST_SUPPORT
/* Static variables */

static char product[ENTRY_COUNT][MAX_PRODUCT_LENGTH];
static bool init_flag = false;
static const char *ALERT_TEXT =

"        !!! ALERT !!!\n"
"\n"
"Getting a new USB device list\n"
"will disconnect any USB device\n"
"connected to Aranym\n"
"";

#define SDLGUI_INCLUDE_USBDLG
#include "sdlgui.sdl"

#define PLUG_BUTTON_OFFSET	PLUG_0
#define CONNECTED_INFO_OFFSET	CONNECTED_0

/* Local functions */

int DlgUsb::check_if_devices_connected(void)
{
	int i = 0;

	while (i < USB_MAX_DEVICE) {
		if (virtual_device[i].connected == true)
			return 1;
		i++;
	}

	return 0;
}


void DlgUsb::enable_buttons(void)
{
	int i = 0;

	while (i < ENTRY_COUNT) {
		if (virtual_device[i + ypos].virtdev_available == true)
			dlg[PLUG_BUTTON_OFFSET + i].state &= ~SG_DISABLED;
		else
			usbdlg[PLUG_BUTTON_OFFSET + i].state |= SG_DISABLED;
			
		i++;
	}
}


void DlgUsb::disable_buttons(void)
{
	int i = 0;

	while (i < ENTRY_COUNT) {
		if (virtual_device[i + ypos].connected == false)
			usbdlg[PLUG_BUTTON_OFFSET + i].state |= SG_DISABLED;
		else
			usbdlg[PLUG_BUTTON_OFFSET + i].state &= ~SG_DISABLED;
		i++;
	}
}


void DlgUsb::reset_buttons_and_state(void)
{
	int i = 0;

	while (i < ENTRY_COUNT) {
		dlg[PLUG_BUTTON_OFFSET + i].state |= SG_DISABLED;
		dlg[CONNECTED_INFO_OFFSET + i].state |= SG_DISABLED;
		i++;
	}

	init_flag = true;
}


void DlgUsb::clean_product_strings(void)
{
	int i = 0;

	while (i < ENTRY_COUNT) {
		product[i][0] = '\0';
		i++;
	}
}

/* Public functions */

int DlgUsb::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;
	int virtdev_idx, virtdev_position;
	int32 r;

	if (state == STATE_MAIN) { /* Process main USB dialog */
		switch(return_obj) {

			case OK:
				retval = Dialog::GUI_CLOSE;
				break;

			case GET_DEVICE_LIST:
				if ((r = check_if_devices_connected())) {
					state = STATE_ALERT;
					dlgAlert = (DlgAlert *) DlgAlertOpen(ALERT_TEXT, ALERT_OKCANCEL);
					SDLGui_Open(dlgAlert);
				} else {
					ypos = 0;
					reset_buttons_and_state();
					clean_product_strings();
					usbhost_free_usb_devices();
					usbhost_get_device_list();
					refreshentries = true;
				}
				break;

			case USBHOSTDLG_UP:
				/* Scroll up */
				if (ypos > 0) {
					--ypos;
					refreshentries = true;
				}
				break;

			case USBHOSTDLG_DOWN:
				/* Scroll down */
				if (ypos < (USB_MAX_DEVICE - ENTRY_COUNT)) {
					++ypos;
					refreshentries = true;
				}
				break;
		}

		virtdev_idx = return_obj - PLUG_BUTTON_OFFSET + ypos;
		virtdev_position = return_obj - PLUG_BUTTON_OFFSET;
		/* User clicked on PLUG/UNPLUG buttons */
		if ((return_obj >= PLUG_BUTTON_OFFSET) && (return_obj <= PLUG_BUTTON_OFFSET + ENTRY_COUNT)) {
			if (virtual_device[virtdev_idx].connected == false) {
				if (usbhost_claim_device(virtdev_idx) != -1) {
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_INFO_OFFSET + virtdev_position].state &= ~SG_DISABLED;
					virtual_device[virtdev_idx].connected = true;
					if ((++number_ports_used == NUMBER_OF_PORTS))
							refreshentries = true;
				}
			}
			else {
				if (usbhost_release_device(virtdev_idx) != -1) {
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_INFO_OFFSET + virtdev_position].state |= SG_DISABLED;
					virtual_device[virtdev_idx].connected = false;
					if (--number_ports_used < NUMBER_OF_PORTS)
						refreshentries = true;
				}
			}
		}
	}
	else { /* Process Alert dialog */
		state = STATE_MAIN;
		D(bug("dlgUsb: Process Alert dialog"));
		if (dlgAlert && dlgAlert->pressedOk()) {
			reset_buttons_and_state();
			clean_product_strings();
			usbhost_free_usb_devices();
			usbhost_get_device_list();
			refreshentries = true;
		}
	}
	if (refreshentries) {
		refreshEntries();
	}

	return_obj = -1;
	return retval;
}


void DlgUsb::refreshEntries(void)
{
	if (refreshentries) {
		int i;

		for (i = 0; i < ENTRY_COUNT; i++) {
			if ((i + ypos) < USB_MAX_DEVICE) {
				/* Copy entries to dialog: */
				strcpy(product[i], virtual_device[i + ypos].product_name);
				/* Grey/Ungrey CONNECTED info string */
				if (virtual_device[i + ypos].connected == true) {
					dlg[CONNECTED_INFO_OFFSET + i].state &= ~SG_DISABLED;
				}
				else {
					dlg[CONNECTED_INFO_OFFSET + i].state |= SG_DISABLED;
				}
				/* Enable/disable PLUG/UNPLUG buttons */
				if (number_ports_used < NUMBER_OF_PORTS)
					enable_buttons();
				else
					disable_buttons();
			}
			else {
				/* Clear entry */
			}
		}
		refreshentries = false;
	}
}

DlgUsb::DlgUsb(SGOBJ *dlg)
	: Dialog(dlg),
	  state(STATE_MAIN),
	  ypos(0),
	  refreshentries(true)
{
	if (init_flag == false) {
		reset_buttons_and_state();
		usbhost_init_libusb();
	}
	refreshEntries();
	for (int i = 0; i < ENTRY_COUNT; i++) {
		if ((virtual_device[i].virtdev_available == true && number_ports_used < NUMBER_OF_PORTS) ||
		    (virtual_device[i].connected         == true)) {
			dlg[PLUG_BUTTON_OFFSET + i].state &= ~SG_DISABLED;
		}
	}
}

#else /* Function and variables for when USB not present */

#define usbdlg nousbdlg

#define SDLGUI_INCLUDE_NOUSBDLG
#include "sdlgui.sdl"


int DlgUsb::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	if (return_obj == OK)
		retval = Dialog::GUI_CLOSE;

	return retval;
}


DlgUsb::DlgUsb(SGOBJ *dlg)
	: Dialog(dlg)
{

}
#endif /* USBHOST_SUPPORT */


DlgUsb::~DlgUsb()
{

}


/* Private functios */

void DlgUsb::confirm(void)
{

}


Dialog *DlgUsbOpen(void)
{
	return new DlgUsb(usbdlg);
}

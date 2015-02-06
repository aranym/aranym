/*
 * dlgUsb.cpp - USB selection dialog
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

#include "config.h"
#include "sysdeps.h"
#include "sdlgui.h"
#include "dlgAlert.h"
#include "dlgUsb.h"

#define DEBUG 0
#include "debug.h"

#ifdef USBHOST_SUPPORT
/* External functions (usbhost.cpp) */

extern void usbhost_init_libusb(void);
extern int32 usbhost_get_device_list(void);
extern void usbhost_free_usb_devices(void);
extern int usbhost_claim_device(int virtdev_index);
extern int usbhost_release_device(int virtdev_index);


/* External variables (usbhost.cpp) */

extern char product[USB_MAX_DEVICE][MAX_PRODUCT_LENGTH];
extern int number_ports_used;
extern virtual_usbdev_t virtual_device[USB_MAX_DEVICE];


/* Static variables */

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


/* Local functions */

int DlgUsb::check_if_devices_connected(void)
{
	int i = 0;

	while (i < MAX_NUMBER_VIRT_DEV) {
		if (virtual_device[i].connected == true)
			return 1;
		i++;
	}

	return 0;
}


void DlgUsb::enable_buttons(void)
{
	int i = 0;

	while (i < MAX_NUMBER_VIRT_DEV) {
		if (virtual_device[i].virtdev_available == true)
			dlg[PLUG_0 + i].state &= ~SG_DISABLED;
		i++;
	}
}


void disable_buttons(void)
{
	int i = 0;

	while (i < MAX_NUMBER_VIRT_DEV) {
		if (virtual_device[i].connected == false)
			usbdlg[PLUG_0 + i].state |= SG_DISABLED;
		i++;
	}
}


void DlgUsb::reset_buttons_and_state(void)
{
	dlg[PLUG_0].state |= SG_DISABLED;
	dlg[PLUG_1].state |= SG_DISABLED;
	dlg[PLUG_2].state |= SG_DISABLED;
	dlg[PLUG_3].state |= SG_DISABLED;
	dlg[PLUG_4].state |= SG_DISABLED;
	dlg[PLUG_5].state |= SG_DISABLED;
	dlg[PLUG_6].state |= SG_DISABLED;
	dlg[PLUG_7].state |= SG_DISABLED;
	dlg[PLUG_8].state |= SG_DISABLED;

	dlg[CONNECTED_0].state |= SG_DISABLED;
	dlg[CONNECTED_1].state |= SG_DISABLED;
	dlg[CONNECTED_2].state |= SG_DISABLED;
	dlg[CONNECTED_3].state |= SG_DISABLED;
	dlg[CONNECTED_4].state |= SG_DISABLED;
	dlg[CONNECTED_5].state |= SG_DISABLED;
	dlg[CONNECTED_6].state |= SG_DISABLED;
	dlg[CONNECTED_7].state |= SG_DISABLED;
	dlg[CONNECTED_8].state |= SG_DISABLED;

	init_flag = true;
}


void DlgUsb::clean_product_strings(void)
{
	int i = 0;

	while (i < USB_MAX_DEVICE) {
		product[i][0] = '\0';
		i++;
	}
}

/* Public functions */

int DlgUsb::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;
	int virtdev_idx = return_obj - PLUG_BUTTON_OFFSET;
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
					reset_buttons_and_state();
					clean_product_strings();
					usbhost_free_usb_devices();
					usbhost_get_device_list();
					enable_buttons();
				}
				break;

			case PLUG_0:
				if (dlg[CONNECTED_0].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_0].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_0].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_1:
				if (dlg[CONNECTED_1].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_1].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_1].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_2:
				if (dlg[CONNECTED_2].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_2].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_2].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_3:
				if (dlg[CONNECTED_3].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_3].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_3].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_4:
				if (dlg[CONNECTED_4].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_4].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_4].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_5:
				if (dlg[CONNECTED_5].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_5].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_5].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_6:
				if (dlg[CONNECTED_6].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_6].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_6].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_7:
				if (dlg[CONNECTED_7].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_7].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_7].state |= SG_DISABLED;
					enable_buttons();
				}
				break;

			case PLUG_8:
				if (dlg[CONNECTED_8].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_8].state &= ~SG_DISABLED;
					disable_buttons();
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_8].state |= SG_DISABLED;
					enable_buttons();
				}
				break;
		}
	} else { /* Process Alert dialog */
		state = STATE_MAIN;
		D(bug("dlgUsb: Process Alert dialog"));
		if (dlgAlert && dlgAlert->pressedOk()) {
			reset_buttons_and_state();
			clean_product_strings();
			usbhost_free_usb_devices();
			usbhost_get_device_list();
			enable_buttons();
		}
	}

	return_obj = -1;
	return retval;
}


DlgUsb::DlgUsb(SGOBJ *dlg)
	: Dialog(dlg), state(STATE_MAIN)
{
	if (init_flag == false) {
		reset_buttons_and_state();
		usbhost_init_libusb();
	}

	for (int i = 0; i < MAX_NUMBER_VIRT_DEV; i++) {
		if ((virtual_device[i].virtdev_available == true && number_ports_used <= NUMBER_OF_PORTS) ||
		    (virtual_device[i].connected         == true && number_ports_used >  NUMBER_OF_PORTS)) {
			dlg[PLUG_0 + i].state &= ~SG_DISABLED;
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

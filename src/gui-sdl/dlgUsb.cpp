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
enum DLG {
	box_main,
	box_usb,
	text_usb,
	usb_desc0,
	usb_desc1,
	usb_desc2,
	usb_desc3,
	usb_desc4,
	usb_desc5,
	usb_desc6,
	usb_desc7,
	usb_desc8,
	PLUG_0,
	PLUG_1,
	PLUG_2,
	PLUG_3,
	PLUG_4,
	PLUG_5,
	PLUG_6,
	PLUG_7,
	PLUG_8,	
	GET_DEVICE_LIST,
	OK,
	CONNECTED_0,
	CONNECTED_1,
	CONNECTED_2,
	CONNECTED_3,
	CONNECTED_4,
	CONNECTED_5,
	CONNECTED_6,
	CONNECTED_7,
	CONNECTED_8,
};


/* External functions (usbhost.cpp) */

extern int32 usbhost_get_device_list(void);
extern void usbhost_free_usb_devices(void);
extern int8 usbhost_claim_device(int8 virtdev_index);
extern int8 usbhost_release_device(int8 virtdev_index);


/* External variables (usbhost.cpp) */

extern char product[USB_MAX_DEVICE][MAX_PRODUCT_LENGTH];
extern int8 number_ports_used;
extern virtual_usbdev_t virtual_device[USB_MAX_DEVICE];


/* Static variables */

static int8 init_flag;
static const char *ALERT_TEXT =

"        !!! ALERT !!!\n"
"\n"
"Getting a new USB device list\n"
"will disconnect any USB device\n"
"connected to Aranym\n"
"";

static SGOBJ dlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 64,24, NULL },
	{ SGBOX, 0, 0, 2, 2, 60, 18, NULL},
	{ SGTEXT, 0, 0, 26, 1, 13, 1, " USB Devices " },
	{ SGTEXT, 0, 0, 4, 3, 8, 1, product[0] },
	{ SGTEXT, 0, 0, 4, 5, 8, 1, product[1] },
	{ SGTEXT, 0, 0, 4, 7, 8, 1, product[2] },
	{ SGTEXT, 0, 0, 4, 9, 8, 1, product[3] },
	{ SGTEXT, 0, 0, 4, 11, 8, 1, product[4] },
	{ SGTEXT, 0, 0, 4, 13, 8, 1, product[5] },
	{ SGTEXT, 0, 0, 4, 15, 8, 1, product[6] },
	{ SGTEXT, 0, 0, 4, 17, 8, 1, product[7] },
	{ SGTEXT, 0, 0, 4, 19, 8, 1, product[8] },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 3, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 5, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 7, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 9, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 11, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 13, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 15, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 17, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 46, 19, 13,1, "Plug/Unplug" },
	{ SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 4,22, 16,1, "Get new list" },
	{ SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 43,22, 16,1, "OK" },
	{ SGTEXT, 0, 0, 36, 3, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 5, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 7, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 9, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 11, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 13, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 15, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 17, 8, 1, "CONNECTED" },
	{ SGTEXT, 0, 0, 36, 19, 8, 1, "CONNECTED" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};
#define PLUG_BUTTON_OFFSET	12


/* Local functions */

int8 check_if_devices_connected(void)
{
	int8 i = 0;

	while (i < MAX_NUMBER_VIRT_DEV) {
		if (virtual_device[i].connected == true)
			return 1;
		i++;
	}

	return 0;
}


void enable_buttons(void)
{
	int8 i = 0;

	while (i < MAX_NUMBER_VIRT_DEV) {
		if (virtual_device[i].virtdev_available == true)
			dlg[PLUG_0 + i].state &= ~SG_DISABLED;
		i++;
	}
}


void disable_buttons(void)
{
	int8 i = 0;

	while (i < MAX_NUMBER_VIRT_DEV) {
		if (virtual_device[i].connected == false)
			dlg[PLUG_0 + i].state |= SG_DISABLED;
		i++;
	}
}


void reset_buttons_and_state(void)
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


void clean_product_strings(void)
{
	int8 i = 0;

	while (i < USB_MAX_DEVICE) {
		product[i][0] = '\0';
		i++;
	}
}

/* Public functions */

int DlgUsb::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;
	int8 virtdev_idx = return_obj - PLUG_BUTTON_OFFSET;
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
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_0].state |= SG_DISABLED;
				}
				break;

			case PLUG_1:
				if (dlg[CONNECTED_1].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_1].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_1].state |= SG_DISABLED;
				}
				break;

			case PLUG_2:
				if (dlg[CONNECTED_2].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_2].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_2].state |= SG_DISABLED;
				}
				break;

			case PLUG_3:
				if (dlg[CONNECTED_3].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_3].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_3].state |= SG_DISABLED;
				}
				break;

			case PLUG_4:
				if (dlg[CONNECTED_4].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_4].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_4].state |= SG_DISABLED;
				}
				break;

			case PLUG_5:
				if (dlg[CONNECTED_5].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_5].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_5].state |= SG_DISABLED;
				}
				break;

			case PLUG_6:
				if (dlg[CONNECTED_6].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_6].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_6].state |= SG_DISABLED;
				}
				break;

			case PLUG_7:
				if (dlg[CONNECTED_7].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_7].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_7].state |= SG_DISABLED;
				}
				break;

			case PLUG_8:
				if (dlg[CONNECTED_8].state & SG_DISABLED) {
					if (usbhost_claim_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device plugged"));
					dlg[CONNECTED_8].state &= ~SG_DISABLED;
				}
				else {
					if (usbhost_release_device(virtdev_idx) == -1)
						break;
					D(bug("dlgUsb: Device unplugged"));
					dlg[CONNECTED_8].state |= SG_DISABLED;
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
	int8 i;

	if (init_flag == false)
		reset_buttons_and_state();

	for (i = 0; i < MAX_NUMBER_VIRT_DEV; i++) {
		if ((virtual_device[i].virtdev_available == true && number_ports_used <= NUMBER_OF_PORTS) ||
		    (virtual_device[i].connected         == true && number_ports_used >  NUMBER_OF_PORTS)) {
			dlg[PLUG_0 + i].state &= ~SG_DISABLED;
		}
	}
}

#else /* Function and variables for when USB not present */

enum DLG {
	box_main,
	usb_text0,
	usb_text1,
	usb_text2,
	usb_text3,
	usb_text4,
	usb_text5,
	OK
};

static SGOBJ dlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0, 0, 48, 11, NULL },
	{ SGTEXT, 0, 0, 16, 1, 13, 1, "NO USB SUPPORT" },
	{ SGTEXT, 0, 0, 2, 3, 13, 1, "Aranym has been compiled without USB support," },
	{ SGTEXT, 0, 0, 2, 4, 13, 1, "if  you want to  have USB support  in Aranym" },
	{ SGTEXT, 0, 0, 2, 5, 13, 1, "you   need   libusb  for  your platform  and" },
	{ SGTEXT, 0, 0, 2, 6, 13, 1, "compile  Aranym again  with --enable-usbhost" },
	{ SGTEXT, 0, 0, 2, 7, 13, 1, "when running the configure script" },
	{ SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 19,9, 6,1, "OK" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};


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
	return new DlgUsb(dlg);
}

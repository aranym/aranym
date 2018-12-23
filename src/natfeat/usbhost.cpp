/*
 * USB host chip emulation
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
#include "usbhost.h"
#include "../../atari/usbhost/usbhost_nfapi.h"

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"
#include <SDL_thread.h>

/*--- Defines ---*/

#define EINVFN	-32

#define min1_t(type,x,y)	\
	({ type __x = (x); type __y = (y); __x < __y ? __x : __y; })

/* Galvez: added to avoid the variables being shadowed warning */
#define min2_t(type,x,y)	\
	({ type __a = (x); type __b = (y); __a < __b ? __a : __b; })

/* USBHOst runs at interrupt level 5 by default but can be reconfigured */
#if 0
# define INTLEVEL	3
# define TRIGGER_INTERRUPT	TriggerInt3()
#else
# define INTLEVEL	5
# define TRIGGER_INTERRUPT	TriggerInt5()
#endif

/* --- Virtual Root Hub ---------------------------------------------------- */

/* Device descriptor */

static uint8 root_hub_dev_des[] = {
	0x12,			/*  uint8  bLength; */
	0x01,			/*  uint8  bDescriptorType; Device */
	0x00,			/*  uint16 bcdUSB; v1.1 */
	0x02,
	0x09,			/*  uint8  bDeviceClass; HUB_CLASSCODE */
	0x00,			/*  uint8  bDeviceSubClass; */
	0x00,			/*  uint8  bDeviceProtocol; */
	0x08,			/*  uint8  bMaxPacketSize0; 8 Bytes */
	0x00,			/*  uint16 idVendor; */
	0x00,
	0x00,			/*  uint16 idProduct; */
	0x00,
	0x00,			/*  uint16 bcdDevice; */
	0x00,
	0x00,			/*  uint8  iManufacturer; */
	0x01,			/*  uint8  iProduct; */
	0x00,			/*  uint8  iSerialNumber; */
	0x01			/*  uint8  bNumConfigurations; */
};

/* Configuration descriptor */

static uint8 root_hub_config_des[] = {
	0x09,			/*  uint8  bLength; */
	0x02,			/*  uint8  bDescriptorType; Configuration */
	0x19,			/*  uint16 wTotalLength; */
	0x00,
	0x01,			/*  uint8  bNumInterfaces; */
	0x01,			/*  uint8  bConfigurationValue; */
	0x00,			/*  uint8  iConfiguration; */
	0x40,			/*  uint8  bmAttributes;
				   Bit 7: Bus-powered, 6: Self-powered, 5 Remote-wakwup, 4..0: resvd */
	0x00,			/*  uint8  MaxPower; */

	/* interface */
	0x09,			/*  uint8  if_bLength; */
	0x04,			/*  uint8  if_bDescriptorType; Interface */
	0x00,			/*  uint8  if_bInterfaceNumber; */
	0x00,			/*  uint8  if_bAlternateSetting; */
	0x01,			/*  uint8  if_bNumEndpoints; */
	0x09,			/*  uint8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,			/*  uint8  if_bInterfaceSubClass; */
	0x00,			/*  uint8  if_bInterfaceProtocol; */
	0x00,			/*  uint8  if_iInterface; */

	/* endpoint */
	0x07,			/*  uint8  ep_bLength; */
	0x05,			/*  uint8  ep_bDescriptorType; Endpoint */
	0x81,			/*  uint8  ep_bEndpointAddress; IN Endpoint 1 */
	0x03,			/*  uint8  ep_bmAttributes; Interrupt */
	0x00,			/*  uint16 ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
	0x02,
	0xff			/*  uint8  ep_bInterval; 255 ms */
};

/* Standard string descriptors */

static uint8 root_hub_str_index0[] = {
	0x04,			/*  uint8  bLength; */
	0x03,			/*  uint8  bDescriptorType; String-descriptor */
	0x09,			/*  uint8  lang ID */
	0x04,			/*  uint8  lang ID */
};

static uint8 root_hub_str_index1[] = {
	0x22,			/*  uint8  bLength; */
	0x03,			/*  uint8  bDescriptorType; String-descriptor */
	'A',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'R',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'A',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'N',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'Y',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'M',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	' ',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	' ',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'R',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'o',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'o',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	't',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	' ',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'H',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'u',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
	'b',			/*  uint8  Unicode */
	0,			/*  uint8  Unicode */
};

/* Class descriptor */
static uint8 root_hub_class_des[] = {
	0x09,			/* uint8 bDescLength*/
	0x29,			/* uint8 bDescriptorType: 0x29 for hub */
	0x02,			/* uint8 bNbrPorts: 2 downstream ports */
	0x09,			/* uint16 wHubCharacteristics */
				/* Bit 0,1: individual port power switching */
				/* Bit 2: stand alone hub */
				/* Bit 3,4: individual port overcurrent protection */
	0x00,			/* Bit 8,15: reserved */
	0xff,			/* uint8 bPwrOn2PwrGood */
	0x00,			/* unit8 bHubContrCurrent */
	0x00,			/* uint8 DeviceRemovable */
	0xff,			/* uint8 PortPwrCtrlMask */
};

/* Class status */
static uint8 root_hub_class_st[] = {
	0x00,			/* uint16 wHubStatus */
				/* Bit 0: Local power supply good */
				/* Bit 1: No overcurrent condition extsts */
	0x00,
	0x00,			/* uint16 wHubChange */
				/* Bit 0: No change has occurred to Local Power Status */
				/* Bit 1: No change has occurred to Local Power Status */
	0x00,
};

/*--- Global variables ---*/

virtual_usbdev_t virtual_device[USB_MAX_DEVICE];
int number_ports_used;
int reset_flag;
roothub_t roothub;
uint32 port_status[NUMBER_OF_PORTS];

static libusb_device **devs;
static libusb_device *dev;
static libusb_device_handle *devh[USB_MAX_DEVICE];
static bool init_flag = false;

/*--- Functions ---*/

void fill_port_status(unsigned int port_number, bool connected)
{
	D(bug("USBHost: fill_port_status()"));

	if (connected)
		port_status[port_number] |= RH_PS_CCS;		/* Device attached */
	else
		port_status[port_number] &= ~RH_PS_CCS;		/* Device dettached */

	port_status[port_number] |= RH_PS_CSC;			/* connect status change */

	D(bug("USBHost: (After) P%d port_status %x", port_number, port_status[port_number]));
}


int32 usbhost_get_device_list(void)
{
	int idx_dev = 0, idx_virtdev = 0;
	ssize_t cnt;

	number_ports_used = 0;

	cnt = libusb_get_device_list(NULL, &devs);
	D(bug("USBHost: Number of USB devices attached to host bus: %d", (int)cnt));
	if (cnt < 0)
		return (int32)cnt;

	while ((dev = devs[idx_dev]) != NULL) {
		int r = libusb_open(dev, &devh[idx_dev]);
		if (r < 0) {
			D(bug("USBHost: Failed to open the device %d", idx_dev));
			goto try_another;
		}
		D(bug("USBHost: Device %d opened", idx_dev));

		r = libusb_get_device_descriptor(dev, &virtual_device[idx_virtdev].dev_desc);
		if (r < 0) {
			D(bug("USBHost: Unable to get device descriptor %d", r));
			goto try_another;
		}

		if (virtual_device[idx_virtdev].dev_desc.bDeviceClass == LIBUSB_CLASS_HUB) {
			D(bug("USBHost: Device is a HUB"));
			goto try_another;
		}

		r = libusb_get_string_descriptor_ascii(
						devh[idx_dev],
						virtual_device[idx_virtdev].dev_desc.iProduct,
						(unsigned char *)virtual_device[idx_virtdev].product_name,
						MAX_PRODUCT_LENGTH);
		if (r < 0) {
			D(bug("USBHost: Unable to get string descriptor %d", r));
			goto try_another;
		}

		r = libusb_get_config_descriptor(dev, 0, &virtual_device[idx_virtdev].config_desc);
		if (r < 0) {
			D(bug("USBHost: Unable to get configuration descriptor %d", r));
			goto try_another;
		}
		D(bug("USBHost: Number of interfaces %d", virtual_device[idx_virtdev].config_desc->bNumInterfaces));

		virtual_device[idx_virtdev].idx_dev = idx_dev;
		virtual_device[idx_virtdev].virtdev_available = true;
		idx_virtdev++;
try_another:
		idx_dev++;
	}

	return 0;
}


int usbhost_release_device(int virtdev_index)
{
	D(bug("USBHost: Release device %d", virtdev_index));

	int r = -1;
	int dev_index, if_index;
	int port_number;
	int no_of_ifaces;

	dev_index = virtual_device[virtdev_index].idx_dev;
	no_of_ifaces = virtual_device[virtdev_index].config_desc->bNumInterfaces;

	for (if_index = 0; if_index < no_of_ifaces; if_index++) {
		r = libusb_release_interface(devh[dev_index], if_index);
		if (r < 0) {
			D(bug("USBHost: Releasing interface %d failed", if_index));
		}
	}
	virtual_device[virtdev_index].connected = false;
	port_number = virtual_device[virtdev_index].port_number;

	roothub.port[port_number].busy = false;
	roothub.port[port_number].atari_dev_idx = -1;
	fill_port_status(port_number, virtual_device[virtdev_index].connected);

	return r;
}


void usbhost_free_usb_devices(void)
{
	int i = 0;

	while (devs[i] != NULL) {
		if (virtual_device[i].connected)
			usbhost_release_device(i);

		libusb_close(devh[i]);
		devh[i] = NULL;
		i++;
	}

	libusb_free_device_list(devs,0);
	memset(&virtual_device, 0, USB_MAX_DEVICE * sizeof(virtual_usbdev_t));
}


int usbhost_claim_device(int virtdev_index)
{
	D(bug("USBHost: Claim device %d", virtdev_index));

	int r = -1;
	int dev_index, if_index;
	int no_of_ifaces;

	dev_index = virtual_device[virtdev_index].idx_dev;
	no_of_ifaces = virtual_device[virtdev_index].config_desc->bNumInterfaces;

	for (if_index = 0; if_index < no_of_ifaces; if_index++ ) {
		r = libusb_kernel_driver_active(devh[dev_index], if_index);
		if (r < 0) {
			D(bug("USBHost: Checking if kernel driver is active failed. Error %d", r));
			return -1;
		}

		if (r == 1) {
		D(bug("USBHost: Kernel driver active for interface %d", if_index));
			D(bug("USBHost: Trying to detach it"));
			r = libusb_detach_kernel_driver(devh[dev_index], if_index);
		} else goto claim;
		if (r < 0) {
			D(bug("USBHost: Driver detaching failed"));
			return -1;
		}

claim:	r = libusb_claim_interface(devh[dev_index], if_index);
		if (r < 0) {
		D(bug("USBHost: Claiming interface number %d failed", if_index));
		}
	}

	virtual_device[virtdev_index].connected = true;

	for (int i = 0; i < NUMBER_OF_PORTS; i++) {
		if (roothub.port[i].busy == true)
			continue;
		roothub.port[i].busy = true;
		roothub.port[i].port_number = i;
		roothub.port[i].libusb_dev_idx = dev_index;
		virtual_device[virtdev_index].port_number = i;
		break;
	}

	fill_port_status(virtual_device[virtdev_index].port_number, virtual_device[virtdev_index].connected);

	return r;
}


void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			panicbug("USBHost: failed to get device descriptor");
			return;
		}

		printf("USBHost: %04x:%04x (bus %d, device %d)\n",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));
	}
}


int trigger_interrupt(void *)
{
	D(bug("USBHost: send interrupt"));

	for (;;) {
		for (int port_number = 0; port_number < NUMBER_OF_PORTS; port_number++) {
			if ((port_status[port_number] & RH_PS_CSC)) {
				TRIGGER_INTERRUPT;
				D(bug("USBHost: trigger interrupt. port_status[%d] %x", port_number, port_status[port_number]));
			}
		}

		SDL_Delay(250);
	}

	return 0;
}


void usbhost_init_libusb(void)
{
	if (init_flag)
		return;
	
	if (libusb_init(NULL)) {
		D(bug("USBHost: Imposible to start libusb"));
		return;
	}

	libusb_set_debug(NULL, 3);
	
	for (int i = 0; i < USB_MAX_DEVICE; i++) {
		virtual_device[i].connected = false;
		virtual_device[i].virtdev_available = false;
	}

	for (int i = 0; i < NUMBER_OF_PORTS; i++) {
		roothub.port[i].libusb_dev_idx = 0;
		roothub.port[i].atari_dev_idx = -1;
	}

	SDL_CreateNamedThread(trigger_interrupt, "USB", NULL);

	usbhost_get_device_list();

	init_flag = true;
}

/*--- Private functions ---*/

/*--- Support functions ---*/

int32 USBHost::aranym_submit_rh_msg(uint32 pipe, memptr buffer, int32 transfer_len,
				    devrequest *cmd)
{
	D(bug("USBHost: aranym_submit_rh_msg()"));

	int32 leni = transfer_len;
	int32 len = 0;
	uint32 datab[4];
	uint8 *data_buf = (uint8 *) datab;
	int32 stat = 0;

	uint16 bmRType_bReq;
	uint16 wValue;
	uint16 wIndex;
	uint16 wLength;

	if (usb_pipeint(pipe)) {
		D(bug("USBHost: Root-Hub submit IRQ: NOT implemented"));
		return -1;
	}

	bmRType_bReq = cmd->requesttype | (cmd->request << 8);
	wValue = cmd->value;
	wIndex = cmd->index;
	wLength = cmd->length;

	D(bug("USBHost: --- HUB ----------------------------------------"));
	D(bug("USBHost: submit rh urb, req=%x val=%#x index=%#x len=%d",
	    bmRType_bReq, wValue, wIndex, wLength));
	D(bug("USBHost: ------------------------------------------------"));

	switch (bmRType_bReq) {
		case RH_GET_STATUS:
			D(bug("USBHost: RH_GET_STATUS"));
			*data_buf = (SDL_BYTEORDER == SDL_BIG_ENDIAN) ? 
					(uint16) SDL_Swap16(1) : (uint16)(1) ;
			len = 2;
			break;

		case RH_GET_STATUS | RH_INTERFACE:
			D(bug("USBHost: RH_GET_STATUS | RH_INTERFACE"));
			*data_buf = (SDL_BYTEORDER == SDL_BIG_ENDIAN) ? 
					(uint16) SDL_Swap16(0) : (uint16)(0) ;
			len = 2;
			break;

		case RH_GET_STATUS | RH_ENDPOINT:
			D(bug("USBHost: RH_GET_STATUS | RH_ENDPOINT"));
			*data_buf = (SDL_BYTEORDER == SDL_BIG_ENDIAN) ? 
					(uint16) SDL_Swap16(0) : (uint16)(0) ;
			len = 2;
			break;

		case RH_GET_STATUS | RH_CLASS:
			D(bug("USBHost: RH_GET_STATUS | RH_CLASS"));
			data_buf = root_hub_class_st;
			len = 4;
			break;

		case RH_GET_STATUS | RH_OTHER | RH_CLASS:
			D(bug("USBHost: RH_GET_STATUS | RH_OTHER | RH_CLASS"));
			*(uint32 *)data_buf = (SDL_BYTEORDER == SDL_BIG_ENDIAN) ? 
						SDL_Swap32(port_status[wIndex - 1]) :
						port_status[wIndex - 1];
			len = 4;
			break;

		case RH_CLEAR_FEATURE | RH_ENDPOINT:
			D(bug("USBHost: RH_CLEAR_FEATURE | RH_ENDPOINT"));
			switch (wValue) {
				case RH_ENDPOINT_STALL:
					D(bug("USBHost: C_HUB_ENDPOINT_STALL"));
					len = 0;
					break;
			}
			break;

		case RH_CLEAR_FEATURE | RH_CLASS:
			D(bug("USBHost: RH_CLEAR_FEATURE | RH_CLASS"));
			switch (wValue) {
				case RH_C_HUB_LOCAL_POWER:
					D(bug("USBHost: C_HUB_LOCAL_POWER"));
					len = 0;
					break;

				case RH_C_HUB_OVER_CURRENT:
					D(bug("USBHost: C_HUB_OVER_CURRENT"));
					len = 0;
					break;
			}
			break;

		case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
			D(bug("USBHost: RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS"));
			switch (wValue) {
				case RH_PORT_ENABLE:
					port_status[wIndex - 1] &= ~RH_PS_PES;
					len = 0;
					break;

				case RH_PORT_SUSPEND:
					port_status[wIndex - 1] |= RH_PS_PSS;
					len = 0;
					break;

				case RH_PORT_POWER:
					len = 0;
					break;

				case RH_C_PORT_CONNECTION:
					port_status[wIndex - 1] &= ~RH_PS_CSC;
					len = 0;
					break;

				case RH_C_PORT_ENABLE:
					port_status[wIndex - 1] &= ~RH_PS_PESC;
					len = 0;
					break;

				case RH_C_PORT_SUSPEND:
					len = 0;
					break;

				case RH_C_PORT_OVER_CURRENT:
					port_status[wIndex - 1] &= ~RH_PS_OCIC;
					len = 0;
					break;

				case RH_C_PORT_RESET:
					port_status[wIndex - 1] &= ~RH_PS_PRSC;
					len = 0;
					break;

				default:
					D(bug("USBHost: invalid wValue"));
					stat = USB_ST_STALLED;
			}
			break;

		case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
			D(bug("USBHost: RH_SET_FEATURE | RH_OTHER | RH_CLASS"));
			switch (wValue) {
				case RH_PORT_SUSPEND:
					len = 0;
					break;

				case RH_PORT_RESET:
					port_status[wIndex - 1] |= RH_PS_PRS;
					len = 0;
					break;

				case RH_PORT_POWER:
					port_status[wIndex - 1] |= RH_PS_PPS;
					len = 0;
					break;

				case RH_PORT_ENABLE:
					len = 0;
					break;

				default:
					D(bug("USBHost: invalid wValue"));
					stat = USB_ST_STALLED;
			}
			break;

		case RH_SET_ADDRESS:
			D(bug("USBHost: RH_SET_ADDRESS"));
			rh_devnum = wValue;
			len = 0;
			break;

		case RH_GET_DESCRIPTOR:
			D(bug("USBHost: RH_GET_DESCRIPTOR: %x, %d", wValue, wLength));
			switch (wValue) {
				case (USB_DT_DEVICE << 8):	/* device descriptor */
					len = min1_t(uint32, leni,
												min2_t(uint32, sizeof(root_hub_dev_des), wLength));
					data_buf = root_hub_dev_des;
					break;

				case (USB_DT_CONFIG << 8):	/* configuration descriptor */
					len = min1_t(uint32, leni,
												min2_t(uint32, sizeof(root_hub_config_des), wLength));
					data_buf = root_hub_config_des;
					break;

				case ((USB_DT_STRING << 8) | 0x00):	/* string 0 descriptors */
					len = min1_t(uint32, leni,
												min2_t(uint32, sizeof(root_hub_str_index0), wLength));
					data_buf = root_hub_str_index0;
					break;

				case ((USB_DT_STRING << 8) | 0x01):	/* string 1 descriptors */
					len = min1_t(uint32, leni,
												min2_t(uint32, sizeof(root_hub_str_index1), wLength));
					data_buf = root_hub_str_index1;
					break;

				default:
					D(bug("USBHost: invalid wValue"));

					stat = USB_ST_STALLED;
			}
			break;

		case RH_GET_DESCRIPTOR | RH_CLASS:
			D(bug("USBHost: RH_GET_DESCRIPTOR | RH_CLASS"));

			data_buf = root_hub_class_des;
			len = min1_t(uint32, leni,
				    min2_t(uint32, data_buf[0], wLength));
			break;

		case RH_GET_CONFIGURATION:
			D(bug("USBHost: RH_GET_CONFIGURATION"));
			*(uint8 *) data_buf = 0x01;
			len = 1;
			break;

		case RH_SET_CONFIGURATION:
			D(bug("USBHost: RH_SET_CONFIGURATION"));
			len = 0;
			break;

		default:
			D(bug("USBHost: *** *** *** unsupported root hub command *** *** ***"));
			stat = USB_ST_STALLED;
	}

	len = min1_t(uint32, len, leni);

	if(buffer != 0)
		Host2Atari_memcpy(buffer, data_buf, len);

	return (stat ? -1 : len);
}


int32 USBHost::rh_port_status(memptr rh)
{
	for (int i = 0; i < NUMBER_OF_PORTS; i++) {
		WriteInt32(rh + 4 * i, port_status[i]);
	}

	return 0;
}


/*--- API USB functions ---*/

int32 USBHost::usb_lowlevel_init(void)
{
	D(bug("\nUSBHost: usb_lowlevel_init()"));

	rh_devnum = 0;

	reset();

	return 0;
}


int32 USBHost::usb_lowlevel_stop(void)
{
	D(bug("\nUSBHost: usb_lowlevel_stop()"));

	return 0;
}


int32 USBHost::submit_control_msg(uint32 pipe, memptr buffer,
				int32 len, memptr devrequest)
{
	D(bug("\nUSBHost: submit_control_msg()"));

	struct devrequest *cmd;
	uint8 *tempbuff;

	int32 r = -1;
	unsigned int dev_idx = 0;
	int i;

	uint16 bmRType;
	uint16 bReq;
	uint16 wValue;
	uint16 wIndex;
	uint16 wLength;

	cmd = (struct devrequest *)Atari2HostAddr(devrequest);
	tempbuff = (uint8 *) Atari2HostAddr(buffer);

	int32 devnum = usb_pipedevice(pipe);
	D(bug("USBHost: devnum %d ", devnum));

	/* Control message is for the HUB? */
	if (devnum == rh_devnum)
		return aranym_submit_rh_msg(pipe, buffer, len, cmd);

	bmRType = cmd->requesttype;
	bReq = cmd->request;
	wValue = cmd->value;
	wIndex = cmd->index;
	wLength = cmd->length;

	D(bug("USBHost: bmRType %x, bReq %x, wValue %x, wIndex %x, wLength %x", bmRType, bReq, wValue, wIndex, wLength));

	/* Don't allow to change device's addresses set by the host OS */
	if (bReq == USB_REQ_SET_ADDRESS) {
		for (i = 0; i < NUMBER_OF_PORTS; i++) {
			/*
			 * We need to be careful to don't assign to an empty port an already connected
			 * device during the virtual machine reboot. For this we've created the reset_flag
			 * which indicates that we are setting USB devices during a reboot scenario.
			 */
			if (((roothub.port[i].atari_dev_idx < 0) && !reset_flag) || ((roothub.port[i].atari_dev_idx >= 0) && reset_flag)) {
				roothub.port[i].atari_dev_idx = wValue;
				if (reset_flag)
					reset_flag = 0;
				break;
			}
		}
		D(bug("USBHost: Attempt to set device address"));
		r = 0;
	}
	else if ((bReq == USB_REQ_SET_CONFIGURATION) && (wValue > 1)) {	/* We only support one configuration per device */
		D(bug("USBHost: Attempt to change to configuration number %d ", wValue));
		r = -1;
	}
	else {
		for (i = 0; i < NUMBER_OF_PORTS; i++) {
			if (roothub.port[i].atari_dev_idx == devnum) {
				dev_idx = roothub.port[i].libusb_dev_idx;
				D(bug("USBHost: dev_idx %d ", dev_idx));
				break;
			}
		}
		r = libusb_control_transfer(devh[dev_idx], bmRType, bReq, wValue, wIndex, tempbuff, wLength, 0);
		D(bug("USBHost: bytes transmitted %d ", r));
	}

	return r;
}


int32 USBHost::submit_int_msg(uint32 /* pipe */, memptr /* buffer */,
				int32 /* len*/, int32 /* interval*/)
{
	D(bug("\nUSBHost: submit_int_msg()"));
	D(bug("\nUSBHost: Function not supported yet"));

	return -1;
}


int32 USBHost::submit_bulk_msg(uint32 pipe, memptr buffer, int32 len)
{
	D(bug("\nUSBHost: submit_bulk_msg()"));

	uint8 *tempbuff;

	int32 dir_out;
	uint8 endpoint;
	int32 devnum;
	int32 transferred;
	unsigned int dev_idx = 0;
	int i;
	int32 r;

	tempbuff = (uint8 *)Atari2HostAddr(buffer);

	dir_out = usb_pipeout(pipe);
	endpoint = usb_pipeendpoint(pipe) | (dir_out ? LIBUSB_ENDPOINT_OUT : LIBUSB_ENDPOINT_IN);
	devnum = usb_pipedevice(pipe);
	D(bug("USBHost: devnum %d ", devnum));
	D(bug("USBHost: pipe %x ", pipe));
	D(bug("USBHost: --- BULK -----------------------------------------------"));
	D(bug("USBHost: dev=%d endpoint=%d endpoint address= %x buf=%p size=%d dir_out=%d",
	    usb_pipedevice(pipe), usb_pipeendpoint(pipe), endpoint, tempbuff, len, dir_out));

	for (i = 0; i < NUMBER_OF_PORTS; i++) {
		if (roothub.port[i].atari_dev_idx == devnum) {
			dev_idx = roothub.port[i].libusb_dev_idx;
			D(bug("USBHost: dev_idx %d ", dev_idx));
			break;
		}
	}

	r = libusb_bulk_transfer(devh[dev_idx], endpoint, tempbuff, len, &transferred, 1000);
	D(bug("USBHost: return: %d len: %d transferred: %d", r, len, transferred));

	return r;
}

/*--- Public functions ---*/

/* reset, called upon OS reboot */
void USBHost::reset()
{
	D(bug("USBHost: reset"));

	for (int i = 0; i < NUMBER_OF_PORTS; i++) {
		port_status[i] |= (RH_PS_PPS | RH_PS_PES);
		if (port_status[i] & RH_PS_CCS) {
			reset_flag = 1;
			port_status[i] |= RH_PS_CSC;
		}
	}
}

/*--- Dispatcher ---*/
int32 USBHost::dispatch(uint32 fncode)
{
	int32 ret;

	D(bug("USBHost: dispatch(%u)", fncode));
	ret = EINVFN;

	switch(fncode) {
		case GET_VERSION:
			D(bug("USBHost: getVersion"));
			ret = USBHOST_NFAPI_VERSION;
			break;

		case USBHOST_INTLEVEL:
			D(bug("USBHost: getINTlevel"));
			ret = INTLEVEL;
			break;

		case USBHOST_RH_PORT_STATUS:
			ret = rh_port_status(getParameter(0));
			break;

		case USBHOST_LOWLEVEL_INIT:
			ret = usb_lowlevel_init();
			break;

		case USBHOST_LOWLEVEL_STOP:
			ret = usb_lowlevel_stop();
			break;

		case USBHOST_SUBMIT_CONTROL_MSG:
			ret = submit_control_msg(getParameter(0), getParameter(1), getParameter(2),
						getParameter(3));
			break;

		case USBHOST_SUBMIT_INT_MSG:
			ret = submit_int_msg(getParameter(0), getParameter(1), getParameter(2),
						getParameter(3));
			break;

		case USBHOST_SUBMIT_BULK_MSG:
			ret = submit_bulk_msg(getParameter(0), getParameter(1), getParameter(2));
			break;
		default:
			D(bug("USBHost: unimplemented function #%d", fncode));
			break;
	}

	D(bug("USBHost: function returning with 0x%08x", ret));

	return ret;
}


/*--- Constructor/destructor functions ---*/

USBHost::USBHost()
{
	for (int i = 0; i < NUMBER_OF_PORTS; i++) {
		port_status[i] = 0x0000; 	/* Clean before use */
	}
	D(bug("USBHost: created"));
}


USBHost::~USBHost()
{
	int i = USB_MAX_DEVICE - 1;
	unsigned int port_number = 0;

	if (init_flag) {
		while (i >= 0) {
			if (devh[i] != NULL) {
			D(bug("USBHost: Trying to close device %d", i));

			while (port_number < NUMBER_OF_PORTS) {
				if (roothub.port[port_number].libusb_dev_idx == i) {
					if (libusb_release_interface(devh[i], roothub.port[port_number].interface) < 0) {
						D(bug("USBHost: unable to release device interface"));
					}
					if (libusb_attach_kernel_driver(devh[i], roothub.port[port_number].interface) < 0) {
						D(bug("USBHost: unable to reattach kernel driver to interface"));
					}
					break;
				}
				port_number++;
			}
			libusb_close(devh[i]);
			devh[i] = NULL;
			D(bug("USBHost: %d device closed", i));
			}
			i--;
		}

		usbhost_free_usb_devices();
		libusb_exit(NULL);
	}

	D(bug("USBHost: destroyed"));
}


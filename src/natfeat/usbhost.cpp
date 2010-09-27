/*
	USB host chip emulation

	ARAnyM (C) 2010 David Gálvez

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "usbhost.h"
#include "../../atari/usbhost/usbhost_nfapi.h"

#define DEBUG 0
#include "debug.h"

#include <SDL_endian.h>

/*--- Defines ---*/

#define EINVFN	-32

#define min1_t(type,x,y)	\
	({ type __x = (x); type __y = (y); __x < __y ? __x : __y; })

/* Galvez: added to avoid the variables being shadowed warning */
#define min2_t(type,x,y)	\
	({ type __a = (x); type __b = (y); __a < __b ? __a : __b; })

/*--- Types ---*/

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

/*--- Constructor/destructor functions ---*/

USBHost::USBHost()
{
	D(bug("USBHost: created"));
}

USBHost::~USBHost()
{
	D(bug("USBHost: destroyed"));
}


/*--- Public functions ---*/


int32 USBHost::dispatch(uint32 fncode)
{
	int32 ret;

	D(bug("USBHost: dispatch(%u)", fncode));
	ret = EINVFN;

	switch(fncode) {
		case USBHOST_LOWLEVEL_INIT:
			ret = usb_lowlevel_init();
			break;
		case USBHOST_LOWLEVEL_STOP:
			ret = usb_lowlevel_stop();
			break;
		case USBHOST_SUBMIT_CONTROL_MSG:
			ret = submit_control_msg(getParameter(0), getParameter(1), getParameter(2),
						getParameter(3), getParameter(4));
			break;
		case USBHOST_SUBMIT_INT_MSG:
			ret = submit_int_msg(getParameter(0), getParameter(1), getParameter(2),
						getParameter(3), getParameter(4));
			break;
		case USBHOST_SUBMIT_BULK_MSG:
			ret = submit_bulk_msg(getParameter(0), getParameter(1), getParameter(2),
						getParameter(3));
			break;
		default:
			D(bug("USBHost: unimplemented function #%d", fncode));
			break;
	}

	D(bug("USBHost: function returning with 0x%08x", ret));
	
	return ret;
}

/*--- Support functions ---*/

void USBHost::print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			panicbug("failed to get device descriptor");
			return;
		}

		printf("%04x:%04x (bus %d, device %d)\n",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));
	}
}

int8 USBHost::claim_device(int8 device_index, int32 interface )
{
	D(bug("USBHost: claim_device()"));
	int8 r;

	r = libusb_kernel_driver_active(devh[device_index], interface);
	if (r < 0) {
		D(bug("checking if kernel driver is active failed. Error %d", r));
		return -1;
	}

	if (r == 1) {
		D(bug("Kernel driver active for interface"));
		D(bug("Trying to detach it"));
		r = libusb_detach_kernel_driver(devh[device_index], interface);
	} else goto claim;
	
	if (r < 0) {
		D(bug("driver detaching failed"));
		return -1;
	}

claim:	r = libusb_claim_interface(devh[device_index], interface);
	if (r < 0)
		return -1;
 
	return r;
}

int8 USBHost::check_device(libusb_device *dev, uint8 idx )
{
	D(bug("USBHost: check_device()"));
	
	struct libusb_device_descriptor desc;
	struct libusb_config_descriptor *conf;
	uint32 datab[4];
	uint8 *data = (uint8 *) datab;

	int8 r;
	uint8 i = 0;

	if (libusb_get_device_descriptor(dev, &desc) != 0) {
		D(bug("USBHost: unable to get device descriptor"));
		return -1;
	}

	switch (desc.bDeviceClass) {
		case LIBUSB_CLASS_HUB:
			D(bug("USBHost: device is a hub"));
			D(bug("USBHost: it doesn't interest us"));
			return -1;
		
		case LIBUSB_CLASS_AUDIO:
			D(bug("USBHost: audio device "));
			D(bug("USBHost: it doesn't interest us"));
			return -1;

		case LIBUSB_CLASS_COMM:
			D(bug("USBHost: communication device"));
			D(bug("USBHost: it doesn't interest us"));
			return -1;

		case LIBUSB_CLASS_HID:
			D(bug("USBHost: human interface device"));
			D(bug("USBHost: it doesn't interest us"));
			return -1;

		case LIBUSB_CLASS_PRINTER: 
			D(bug("USBHost: printer"));
			D(bug("USBHost: it doesn't interest us"));
			return -1;

		case LIBUSB_CLASS_PTP:
			D(bug("USBHost: picture transfer protocol device"));
			D(bug("USBHost: it doesn't interest us"));
			return -1;
		
		case LIBUSB_CLASS_MASS_STORAGE:
			D(bug("USBHost: mass storage device"));
			break;

		case LIBUSB_CLASS_DATA:
			D(bug("USBHost: data class device"));
			D(bug("USBHost: it doesn't interest us"));
			return -1;

		case LIBUSB_CLASS_PER_INTERFACE:
			D(bug("USBHost: class per interface device"));
			break;

		case LIBUSB_CLASS_VENDOR_SPEC:
			D(bug("USBHost: vendor specific device"));
			D(bug("USBHost: it doesn't interest us"));
			return -1;

		default:
			D(bug("USBHost: unknown device"));
			return -1;
	}

	if (libusb_get_active_config_descriptor(dev, &conf) != 0) {
		D(bug("USBHost: unable to get active configuration descriptor"));
		return -1;
	} 
	else {
		D(bug("USBHost: Number of interfaces %d", conf->bNumInterfaces));
	}	
	
	uint8 bInterfaceClass[conf->bNumInterfaces];
	
	while (i < conf->bNumInterfaces) {
		if (libusb_get_descriptor(devh[idx], LIBUSB_DT_INTERFACE, i, data, sizeof(data)) > -1) {
			bInterfaceClass[i] = data[5];
			i++;
		}			
		else {
			D(bug("USBHost: unable to get interface descriptor"));
			return -1;
		}	
	}

	for (i = 0; i < conf->bNumInterfaces; i++)
	{
		switch (bInterfaceClass[i]) {
			case LIBUSB_CLASS_HUB:
				D(bug("USBHost: device is a hub"));
				D(bug("USBHost: it doesn't interest us"));
				continue;
		
			case LIBUSB_CLASS_AUDIO:
				D(bug("USBHost: audio device "));
				D(bug("USBHost: it doesn't interest us"));
				continue;
	
			case LIBUSB_CLASS_COMM:
				D(bug("USBHost: communication device"));
				D(bug("USBHost: it doesn't interest us"));
				continue;
	
			case LIBUSB_CLASS_HID:
				D(bug("USBHost: human interface device"));
				D(bug("USBHost: it doesn't interest us"));
				continue;
	
			case LIBUSB_CLASS_PRINTER: 
				D(bug("USBHost: printer"));
				D(bug("USBHost: it doesn't interest us"));
				continue;
	
			case LIBUSB_CLASS_PTP:
				D(bug("USBHost: picture transfer protocol device"));
				D(bug("USBHost: it doesn't interest us"));
				continue;
			
			case LIBUSB_CLASS_MASS_STORAGE:
				D(bug("USBHost: mass storage device"));
				break;
	
			case LIBUSB_CLASS_DATA:
				D(bug("USBHost: data class device"));
				D(bug("USBHost: it doesn't interest us"));
				continue;
	
			case LIBUSB_CLASS_VENDOR_SPEC:
				D(bug("USBHost: vendor specific device"));
				D(bug("USBHost: it doesn't interest us"));
				continue;
	
			default:
				D(bug("USBHost: unknown device"));
				continue;
		}
		/* OK, we are interested in this interface */
		/* Let's check if the interface is available for use */
		r = claim_device(idx, i);
		if (r < 0) {
			D(bug("unable to claim interface %d for device %d error %d", i, idx, r));
		}
		else return 0; /* We found an interface that we want */
	}
	return -1;
}

void USBHost::fill_port_status(uint8 port_number)
{
	D(bug("USBHost: fill_port_status()"));
	
	uint32 buffer;

	roothub.port[port_number].wPortStatus |= (RH_PS_CCS | RH_PS_PES);	/* Device attached */
	roothub.port[port_number].wPortChange |= RH_PS_CSC;			/* connect status change */

	D(bug("wPortStatus %x", roothub.port[port_number].wPortStatus));
	D(bug("wPortChange %x", roothub.port[port_number].wPortChange));

	libusb_control_transfer(devh[rh_devnum], 
				RH_CLASS | RH_OTHER, 
				RH_GET_STATUS, 0, 
				roothub.port[port_number].device_index,
				(uint8 *)&buffer, 4, 
				0);

	buffer = buffer & (RH_PS_LSDA | RH_PS_FSDA);

	D(bug("buffer %x", buffer));
	roothub.port[port_number].wPortStatus |= (uint16)buffer;

	D(bug("wPortStatus %x", roothub.port[port_number].wPortStatus));
}

int32 USBHost::aranym_submit_rh_msg(usb_device *dev, uint32 pipe,
				 memptr buffer, int32 transfer_len,
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
		D(bug("Root-Hub submit IRQ: NOT implemented"));
		return 0;
	}


	bmRType_bReq = cmd->requesttype | (cmd->request << 8);
	wValue = swap_16(cmd->value);
	wIndex = swap_16(cmd->index);
	wLength = swap_16(cmd->length);

	D(bug("--- HUB ----------------------------------------"));
	D(bug("submit rh urb, req=%x val=%#x index=%#x len=%d",
	    bmRType_bReq, wValue, wIndex, wLength));
	D(bug("------------------------------------------------"));

	switch (bmRType_bReq) {
		case RH_GET_STATUS:
			D(bug("RH_GET_STATUS"));

			*data_buf = (uint16)swap_16(1);
			len = 2;
			break;

		case RH_GET_STATUS | RH_INTERFACE:
			D(bug("RH_GET_STATUS | RH_INTERFACE"));

			*data_buf = (uint16)swap_16(0);
			len = 2;
			break;

		case RH_GET_STATUS | RH_ENDPOINT:
			D(bug("RH_GET_STATUS | RH_ENDPOINT"));

			*data_buf = (uint16)swap_16(0);
			len = 2;
			break;

		case RH_GET_STATUS | RH_CLASS:
			D(bug("RH_GET_STATUS | RH_CLASS"));
#if 0			/* get real values from host hub */
			libusb_control_transfer(devh[rh_devnum],
						cmd->requesttype, 
						cmd->request, wValue, 
						wIndex,
						data_buf, wLength, 
						0);
#endif		
			D(bug("Hub Descriptor %x", *(uint32 *) data_buf));
			
			data_buf = root_hub_class_st;   /* you want hardcoded values */
			len = 4;
			break;

		case RH_GET_STATUS | RH_OTHER | RH_CLASS:
			D(bug("RH_GET_STATUS | RH_OTHER | RH_CLASS"));
#if 0			/* get real values from host hub */			
			libusb_control_transfer(devh[rh_devnum], 
						cmd->requesttype, 
						cmd->request, wValue, 
						roothub.port[wIndex - 1].device_index,
						data_buf, wLength, 
						0);
#endif
		
			*data_buf = roothub.port[wIndex - 1].wPortStatus;
			*(data_buf + 2) = roothub.port[wIndex - 1].wPortChange;
			
			D(bug("WPortStatus %x", *data_buf));
			D(bug("WPortChange %x", *(data_buf + 2)));
			len = 4;
			break;

		case RH_CLEAR_FEATURE | RH_ENDPOINT:
			D(bug("RH_CLEAR_FEATURE | RH_ENDPOINT"));

			switch (wValue) {
				case RH_ENDPOINT_STALL:
					D(bug("C_HUB_ENDPOINT_STALL"));
					len = 0;
					break;
			}
			break;

		case RH_CLEAR_FEATURE | RH_CLASS:
			D(bug("RH_CLEAR_FEATURE | RH_CLASS"));

			switch (wValue) {
				case RH_C_HUB_LOCAL_POWER:
					D(bug("C_HUB_LOCAL_POWER"));
					
					len = 0;
					break;

				case RH_C_HUB_OVER_CURRENT:
					D(bug("C_HUB_OVER_CURRENT"));

					len = 0;
					break;
			}
			break;

		case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
			D(bug("RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS"));

			switch (wValue) {
				case RH_PORT_ENABLE:
//					idx = (wIndex && 0x00ff) - 1;
//					roothub.port[idx].wPortStatus &= ~RH_PS_PES;

					len = 0;
					break;

				case RH_PORT_SUSPEND:
//					idx = (wIndex && 0x00ff) - 1;
//					roothub.port[idx].wPortStatus |= RH_PS_PSS;
					
					len = 0;
					break;

				case RH_PORT_POWER:
				
					len = 0;
					break;

				case RH_C_PORT_CONNECTION:
				
					len = 0;
					break;

				case RH_C_PORT_ENABLE:
					
					len = 0;
					break;

				case RH_C_PORT_SUSPEND:
			
					len = 0;
					break;

				case RH_C_PORT_OVER_CURRENT:
					
					len = 0;
					break;

				case RH_C_PORT_RESET:
				roothub.port[cmd->index - 1].wPortChange &= 0xfffe;
					
					len = 0;
					break;

				default:
					D(bug("invalid wValue"));
					stat = USB_ST_STALLED;
			}
			break;

		case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
			D(bug("RH_SET_FEATURE | RH_OTHER | RH_CLASS"));

			switch (wValue) {
				case RH_PORT_SUSPEND:

					len = 0;
					break;

				case RH_PORT_RESET:
					roothub.port[cmd->index - 1].wPortChange &= 0xfffe;
					
					len = 0;
					break;

				case RH_PORT_POWER:

					len = 0;
					break;

				case RH_PORT_ENABLE:

					len = 0;
					break;

				default:
					D(bug("invalid wValue"));
					stat = USB_ST_STALLED;
			}
			break;

		case RH_SET_ADDRESS:
			D(bug("RH_SET_ADDRESS"));

			rh_devnum = wValue;
			len = 0;
			break;

		case RH_GET_DESCRIPTOR:
			D(bug("RH_GET_DESCRIPTOR: %x, %d", wValue, wLength));

			switch (wValue) {
				case (USB_DT_DEVICE << 8):	/* device descriptor */
					len = min1_t(uint32,
				    	     	     leni, min2_t(uint32,
						     		  sizeof(root_hub_dev_des),
					      		  	  wLength));
					data_buf = root_hub_dev_des;
					break;

				case (USB_DT_CONFIG << 8):	/* configuration descriptor */
					len = min1_t(uint32,
						     leni, min2_t(uint32,
								  sizeof(root_hub_config_des),
								  wLength));
					data_buf = root_hub_config_des;
					break;

				case ((USB_DT_STRING << 8) | 0x00):	/* string 0 descriptors */
					len = min1_t(uint32,
						     leni, min2_t(uint32,
								  sizeof(root_hub_str_index0),
								  wLength));
					data_buf = root_hub_str_index0;
					break;

				case ((USB_DT_STRING << 8) | 0x01):	/* string 1 descriptors */
					len = min1_t(uint32,
						     leni, min2_t(uint32,
								  sizeof(root_hub_str_index1),
								  wLength));
					data_buf = root_hub_str_index1;
					break;

				default:
					D(bug("invalid wValue"));

					stat = USB_ST_STALLED;
			}
			break;
		
		case RH_GET_DESCRIPTOR | RH_CLASS:
			D(bug("RH_GET_DESCRIPTOR | RH_CLASS"));
	
			data_buf = root_hub_class_des;
			len = min1_t(uint32, leni,
				    min2_t(uint32, data_buf[0], wLength));
			break;

		case RH_GET_CONFIGURATION:
			D(bug("RH_GET_CONFIGURATION"));

			*(uint8 *) data_buf = 0x01;
			len = 1;
			break;

		case RH_SET_CONFIGURATION:
			D(bug("RH_SET_CONFIGURATION"));

			len = 0;
			break;

		default:
			D(bug("*** *** *** unsupported root hub command *** *** ***"));
			stat = USB_ST_STALLED;
	}
	
	len = min1_t(int32, len, leni);

	Host2Atari_memcpy(buffer, data_buf, len);

	dev->act_len = len;
	dev->status = stat;
	D(bug("dev act_len %d, status %ld", dev->act_len, dev->status));

	return stat;
}


/*--- USB functions ---*/

int32 USBHost::usb_lowlevel_init(void)
{
	D(bug("\nUSBHost: usb_lowlevel_init()"));
	
	libusb_device *dev;
	ssize_t cnt;
	int8 i = 0, r;
	int8 port_number = 0; 
	rh_devnum = 0;
	int32 interface = 0;

	for(i = 0; i < NUMBER_OF_PORTS; i++) {
		roothub.port[i].wPortStatus = 0x0000; 	/* Clean struct members before use */
		roothub.port[i].wPortChange = 0x0000;
		roothub.port[i].device_index = 0;
	}

	if (libusb_init(NULL))
		return -1;

	libusb_set_debug(NULL, 3); 	

	cnt = libusb_get_device_list(NULL, &devs);
	D(bug("USBHost: cnt: %d", cnt));
	if (cnt < 0)
		return (int32) cnt;

	i = 0;
	while ((dev = devs[i]) != NULL) {
		r = libusb_open(dev, &devh[i]);
		if (r < 0) {
			D(bug("\nFailed to open the device %d\n\r", i));
			goto try_another;
		}
		
		D(bug("\nDevice %d opened", i));
		
		/* Check the device and if we like it let's try to claim it for us */
		r = check_device(dev, i); 

		if (r < 0) {
			D(bug("Device %d doesn't have any interface good for us", i));
			goto try_another;
		}
					
		D(bug("Device %d assigned to port %d", i, port_number + 1));
		
		roothub.port[port_number].interface = interface;
		roothub.port[port_number].device_index = i;
		fill_port_status(port_number);
		port_number++;
		/* we stop when we have 2 devices(one for each port) */
		if (port_number > NUMBER_OF_PORTS - 1)
			break;

try_another:		i++;
	}

	total_num_handles = i;
	D(bug("\nNumber of handles %d \n\r" \
		 "Number of ports assigened %d\r", 
		 i, port_number > 0 ? port_number : 0));
	
	return 0;
}

int32 USBHost::usb_lowlevel_stop(void)
{
	D(bug("\nUSBHost: usb_lowlevel_stop()"));
	
	int8 i = total_num_handles - 1;
	uint8 port_number = 0;

	while (i >= 0) {
		D(bug("Trying to close device %d \r", i));
		
		while (port_number < NUMBER_OF_PORTS) {
			if (roothub.port[port_number].device_index == i) {
				if (libusb_release_interface(devh[i], roothub.port[port_number].interface) < 0) {
					D(bug("unable to release device interface"));
				}
				if (libusb_attach_kernel_driver(devh[i], roothub.port[port_number].interface) < 0) {
					D(bug("unable to reattach kernel driver to interface"));
				}
				break;
			}
			port_number++;
		}
		libusb_close(devh[i]);		
		D(bug("%d device closed\r", i));
		i--;
	}
	libusb_free_device_list(devs, 1);
	libusb_exit(NULL);
	return 0;
}

int32 USBHost::submit_control_msg(memptr usb_device, uint32 pipe, memptr buffer,
				int32 len, memptr devrequest)
{
	D(bug("\nUSBHost: submit_control_msg()"));

	struct usb_device *tmp_usb_device;
	struct devrequest *cmd;
	uint8 *tempbuff;

	int32 r;
	uint8 dev_idx = 0;

	uint16 bmRType;
	uint16 bReq;
	uint16 wValue;
	uint16 wIndex;
	uint16 wLength;

	tmp_usb_device = (struct usb_device *)Atari2HostAddr(usb_device);
	cmd = (struct devrequest *)Atari2HostAddr(devrequest);
	tempbuff = (uint8 *) Atari2HostAddr(buffer);

	int32 devnum = usb_pipedevice(pipe);
	D(bug("devnum %d ", devnum));	

	/* Control message is for the HUB? */
	if (devnum == rh_devnum)
		return aranym_submit_rh_msg(tmp_usb_device, pipe, buffer, len, cmd);

	bmRType = cmd->requesttype;
	bReq = cmd->request;
	wValue = swap_16(cmd->value);
	wIndex = swap_16(cmd->index);
	wLength = swap_16(cmd->length);	

	D(bug("bmRType %x, bReq %x, wValue %x, wIndex %x, wLength %x", bmRType, bReq, wValue, wIndex, wLength));

	dev_idx = roothub.port[0].device_index;
	D(bug("dev_idx %d ", dev_idx));

	if (bReq == USB_REQ_SET_ADDRESS) {	/* The only msg we don't want to pass to the device */
		D(bug("SET_ADDRESS msg %x ", bReq));	
		r = 0;
	}
	else
		r = libusb_control_transfer(devh[dev_idx], bmRType, bReq, wValue, wIndex, tempbuff, wLength, 0);	
	
	if (r >= 0)
		tmp_usb_device->status = 0;
	
	tmp_usb_device->act_len = r;
	
	D(bug("bytes transmited %d ", r));

	return r;
}

int32 USBHost::submit_int_msg(memptr /* usb_device */, uint32 /* pipe */, memptr /* buffer */,
				int32 /* len */, int32 /* interval */)
{
	D(bug("\nUSBHost: submit_int_msg()"));
	D(bug("\nUSBHost: Function not supported yet"));
	
	return -1;
}

int32 USBHost::submit_bulk_msg(memptr usb_device, uint32 pipe, memptr buffer,
				int32 len)
{
	D(bug("\nUSBHost: submit_bulk_msg()"));

	struct usb_device *tmp_usb_device;
	uint8 *tempbuff;
	
	int32 dir_out;
	uint8 endpoint;
	int32 devnum;
	int32 transferred;
	uint8 dev_idx = 0;
	int32 r;


	tmp_usb_device = (struct usb_device *)Atari2HostAddr(usb_device);
	tempbuff = (uint8 *)Atari2HostAddr(buffer);
	
	dir_out = usb_pipeout(pipe);
	endpoint = usb_pipeendpoint(pipe) | (dir_out ? LIBUSB_ENDPOINT_OUT : LIBUSB_ENDPOINT_IN);
	devnum = usb_pipedevice(pipe);
	D(bug("devnum %d ", devnum));
	D(bug("pipe %x ", pipe));
	
	

	D(bug("--- BULK -----------------------------------------------"));
	D(bug("dev=%ld endpoint=%ld endpoint address= %x buf=%p size=%d dir_out=%d",
	    usb_pipedevice(pipe), usb_pipeendpoint(pipe), endpoint, tempbuff, len, dir_out));


	dev_idx = roothub.port[devnum - 2].device_index;
	D(bug("dev_idx %d ", dev_idx));

	r = libusb_bulk_transfer(devh[dev_idx], endpoint, tempbuff, len, &transferred, 1000);	
	D(bug("return: %d len: %d transferred: %d", r, len, transferred));

	if (r >= 0)
		tmp_usb_device->status = 0;

	tmp_usb_device->act_len = r;

	return 0;
}


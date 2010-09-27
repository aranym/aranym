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

#ifndef _USBHOST_H
#define _USBHOST_H

/*--- Includes ---*/

#include <libusb-1.0/libusb.h>

#include "nf_base.h"
#include "../../atari/usbhost/usb.h"

/*--- definitions ---*/

/* --- USB HUB constants (not OHCI-specific) -------------------- */

/* destination of request */
#define RH_INTERFACE               0x01
#define RH_ENDPOINT                0x02
#define RH_OTHER                   0x03

#define RH_CLASS                   0x20
#define RH_VENDOR                  0x40

/* Requests: bRequest << 8 | bmRequestType */
#define RH_GET_STATUS           0x0080
#define RH_CLEAR_FEATURE        0x0100
#define RH_SET_FEATURE          0x0300
#define RH_SET_ADDRESS          0x0500
#define RH_GET_DESCRIPTOR       0x0680
#define RH_SET_DESCRIPTOR       0x0700
#define RH_GET_CONFIGURATION    0x0880
#define RH_SET_CONFIGURATION    0x0900
#define RH_GET_STATE            0x0280
#define RH_GET_INTERFACE        0x0A80
#define RH_SET_INTERFACE        0x0B00
#define RH_SYNC_FRAME           0x0C80
/* Our Vendor Specific Request */
#define RH_SET_EP               0x2000

/* Hub port features */
#define RH_PORT_CONNECTION         0x00
#define RH_PORT_ENABLE             0x01
#define RH_PORT_SUSPEND            0x02
#define RH_PORT_OVER_CURRENT       0x03
#define RH_PORT_RESET              0x04
#define RH_PORT_POWER              0x08
#define RH_PORT_LOW_SPEED          0x09

#define RH_C_PORT_CONNECTION       0x10
#define RH_C_PORT_ENABLE           0x11
#define RH_C_PORT_SUSPEND          0x12
#define RH_C_PORT_OVER_CURRENT     0x13
#define RH_C_PORT_RESET            0x14

/* wPortStatus bits */
#define		RH_PS_CCS	(1 << 0)	/* current connect status */
#define		RH_PS_PES	(1 << 1)	/* port enable status */
#define		RH_PS_PSS	(1 << 2)	/* port suspend status */
#define		RH_PS_POCI	(1 << 3)	/* port over current
						   indicator */
#define		RH_PS_PRS	(1 << 4)	/* port reset status */
#define		RH_PS_PPS	(1 << 8)	/* port power status */
#define		RH_PS_LSDA	(1 << 9)	/* low:1 speed device attached */
#define		RH_PS_FSDA	(1 << 10)	/* full:0 high:1 speed device attached */

/* wPortChange bits */
#define		RH_PS_CSC	(1 << 0)	/* connect status change */
#define		RH_PS_OCIC	(1 << 1)	/* over current indicator
						   change */


/* Hub features */
#define RH_C_HUB_LOCAL_POWER       0x00
#define RH_C_HUB_OVER_CURRENT      0x01

#define RH_DEVICE_REMOTE_WAKEUP    0x00
#define RH_ENDPOINT_STALL          0x01

#define RH_ACK                     0x01
#define RH_REQ_ERR                 -1
#define RH_NACK                    0x00


#define USB_MAX_DEVICE	32

#define NUMBER_OF_PORTS		2

/*--- Types ---*/
typedef struct {
	uint8 port_number;
	uint8 device_index;
	uint16 wPortStatus;
	uint16 wPortChange;
	int32 interface;
} port_t;

typedef struct {
	port_t port[NUMBER_OF_PORTS];
	uint8 number_of_roothub;
} roothub_t;


/*--- Low level API functions that need to be supported ---*/
/*

	int usb_lowlevel_init(void)

	int usb_lowlevel_stop(void)

	int submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
				int len, int interval)

	int submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
			        int len, struct devrequest *setup)
      
	int submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		    		int len)
*/

/*--- Class ---*/

class USBHost : public NF_Base
{
private:
	
	libusb_device **devs;
	struct libusb_device_handle *devh[USB_MAX_DEVICE];
	int32 rh_devnum;					/* address of Root Hub endpoint */
	int8 total_num_handles;

	roothub_t roothub;

	void print_devs(libusb_device **devs);
	int32 aranym_submit_rh_msg(usb_device *dev, uint32 pipe,
				   memptr buffer, int32 transfer_len,
				   devrequest *cmd);
	int8 check_device(libusb_device *dev, uint8 idx);
	void fill_port_status(uint8 port_number);
	int8 claim_device(int8 device_index, int32 interface);

	int32 usb_lowlevel_init(void);
	int32 usb_lowlevel_stop(void);
	int32 submit_control_msg(memptr usb_device, uint32 pipe, memptr buffer,
			         int32 len, memptr devrequest);
	int32 submit_int_msg(memptr usb_device, uint32 pipe, memptr buffer,
			     int32 len, int32 interval);
	int32 submit_bulk_msg(memptr usb_device, uint32 pipe, memptr buffer,
		    	      int32 len);

public:
	USBHost();
	~USBHost();
	const char *name() { return "USBHOST"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

#endif /* _USBHOST_H */

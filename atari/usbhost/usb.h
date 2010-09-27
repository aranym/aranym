/*
 * Modified for Aranym Native Features by David Gálvez 2010
 * Modified from u-boot sources for Atari by Didier Mequignon 2009
 *
 *
 * (C) Copyright 2001
 * Denis Peter, MPL AG Switzerland
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Note: Part of this code has been derived from linux
 *
 */
#ifndef _USB_H_
#define _USB_H_

#include "../../atari/usbhost/usb_defs.h"

/* Everything is aribtrary */
#define USB_ALTSETTINGALLOC		4
#define USB_MAXALTSETTING		128	/* Hard limit */

#define USB_MAX_DEVICE			32
#define USB_MAXCONFIG			8
#define USB_MAXINTERFACES		8
#define USB_MAXENDPOINTS		16
#define USB_MAXCHILDREN			8	/* This is arbitrary */
#define USB_MAX_HUB			16

#define USB_CNTL_TIMEOUT 100	/* 100ms timeout */

/* String descriptor */
struct usb_string_descriptor {
	uint8	bLength;
	uint8	bDescriptorType;
	uint16	wData[1];
} __attribute__ ((packed));

/* device request (setup) */
struct devrequest {
	uint8	requesttype;
	uint8	request;
	uint16	value;
	uint16	index;
	uint16	length;
} __attribute__ ((packed));

/* All standard descriptors have these 2 fields in common */
struct usb_descriptor_header {
	uint8	bLength;
	uint8	bDescriptorType;
} __attribute__ ((packed));

/* Device descriptor */
struct usb_device_descriptor {
	uint8	bLength;
	uint8	bDescriptorType;
	uint16	bcdUSB;
	uint8	bDeviceClass;
	uint8	bDeviceSubClass;
	uint8	bDeviceProtocol;
	uint8	bMaxPacketSize0;
	uint16	idVendor;
	uint16	idProduct;
	uint16	bcdDevice;
	uint8	iManufacturer;
	uint8	iProduct;
	uint8	iSerialNumber;
	uint8	bNumConfigurations;
} __attribute__ ((packed));

/* Endpoint descriptor */
struct usb_endpoint_descriptor {
	uint8	bLength;
	uint8	bDescriptorType;
	uint8	bEndpointAddress;
	uint8	bmAttributes;
	uint16	wMaxPacketSize;
	uint8	bInterval;
	uint8	bRefresh;
	uint8	bSynchAddress;
} __attribute__ ((packed)) __attribute__ ((aligned(2)));

/* Interface descriptor */
struct usb_interface_descriptor {
	uint8	bLength;
	uint8	bDescriptorType;
	uint8	bInterfaceNumber;
	uint8	bAlternateSetting;
	uint8	bNumEndpoints;
	uint8	bInterfaceClass;
	uint8	bInterfaceSubClass;
	uint8	bInterfaceProtocol;
	uint8	iInterface;

	uint8	no_of_ep;
	uint8	num_altsetting;
	uint8	act_altsetting;

	struct usb_endpoint_descriptor ep_desc[USB_MAXENDPOINTS];
} __attribute__ ((packed));


/* Configuration descriptor information.. */
struct usb_config_descriptor {
	uint8	bLength;
	uint8	bDescriptorType;
	uint16	wTotalLength;
	uint8	bNumInterfaces;
	uint8	bConfigurationValue;
	uint8	iConfiguration;
	uint8	bmAttributes;
	uint8	MaxPower;

	uint8	no_of_if;	/* number of interfaces */
	struct usb_interface_descriptor if_desc[USB_MAXINTERFACES];
} __attribute__ ((packed));

enum {
	/* Maximum packet size; encoded as 0,1,2,3 = 8,16,32,64 */
	PACKET_SIZE_8   = 0,
	PACKET_SIZE_16  = 1,
	PACKET_SIZE_32  = 2,
	PACKET_SIZE_64  = 3,
};

struct usb_device {
	int32	devnum;			/* Device number on USB bus */
	int32	speed;			/* full/low/high */
	int8	mf[32];			/* manufacturer */
	int8	prod[32];		/* product */
	int8	serial[32];		/* serial number */

	/* Maximum packet size; one of: PACKET_SIZE_* */
	int32 maxpacketsize;
	/* one bit for each endpoint ([0] = IN, [1] = OUT) */
	uint32 toggle[2];
	/* endpoint halts; one bit per endpoint # & direction;
	 * [0] = IN, [1] = OUT
	 */
	uint32 halted[2];
	int32 epmaxpacketin[16];		/* INput endpoint specific maximums */
	int32 epmaxpacketout[16];		/* OUTput endpoint specific maximums */

	int32 configno;			/* selected config number */
	struct usb_device_descriptor descriptor; /* Device Descriptor */
	struct usb_config_descriptor config; /* config descriptor */

	int32 have_langid;		/* whether string_langid is valid yet */
	int32 string_langid;		/* language ID for strings */
	memptr irq_handle;
	uint32 irq_status;
	int32 irq_act_len;		/* transfered bytes */
	memptr privptr;
	/*
	 * Child devices -  if this is a hub device
	 * Each instance needs its own set of data structures.
	 */
	uint32 status;
	int32 act_len;			/* transfered bytes */
	int32 maxchild;			/* Number of ports if hub */
	int32 portnr;
	memptr  parent;
	memptr  children[USB_MAXCHILDREN];
};


/* big endian -> little endian conversion */
/* some CPUs are already little endian e.g. the ARM920T */
#define __swap_16(x) \
	({ unsigned short x_ = (unsigned short)x; \
	 (unsigned short)( \
		((x_ & 0x00FFU) << 8) | ((x_ & 0xFF00U) >> 8)); \
	})
#define __swap_32(x) \
	({ unsigned long x_ = (unsigned long)x; \
	 (unsigned long)( \
		((x_ & 0x000000FFUL) << 24) | \
		((x_ & 0x0000FF00UL) <<	 8) | \
		((x_ & 0x00FF0000UL) >>	 8) | \
		((x_ & 0xFF000000UL) >> 24)); \
	})

#ifdef __LITTLE_ENDIAN
# define swap_16(x) (x)
# define swap_32(x) (x)
#else
# define swap_16(x) __swap_16(x)
# define swap_32(x) __swap_32(x)
#endif

/*
 * Calling this entity a "pipe" is glorifying it. A USB pipe
 * is something embarrassingly simple: it basically consists
 * of the following information:
 *  - device number (7 bits)
 *  - endpoint number (4 bits)
 *  - current Data0/1 state (1 bit)
 *  - direction (1 bit)
 *  - speed (2 bits)
 *  - max packet size (2 bits: 8, 16, 32 or 64)
 *  - pipe type (2 bits: control, interrupt, bulk, isochronous)
 *
 * That's 18 bits. Really. Nothing more. And the USB people have
 * documented these eighteen bits as some kind of glorious
 * virtual data structure.
 *
 * Let's not fall in that trap. We'll just encode it as a simple
 * unsigned int. The encoding is:
 *
 *  - max size:		bits 0-1	(00 = 8, 01 = 16, 10 = 32, 11 = 64)
 *  - direction:	bit 7		(0 = Host-to-Device [Out],
 *					(1 = Device-to-Host [In])
 *  - device:		bits 8-14
 *  - endpoint:		bits 15-18
 *  - Data0/1:		bit 19
 *  - speed:		bit 26		(0 = Full, 1 = Low Speed, 2 = High)
 *  - pipe type:	bits 30-31	(00 = isochronous, 01 = interrupt,
 *					 10 = control, 11 = bulk)
 *
 * Why? Because it's arbitrary, and whatever encoding we select is really
 * up to us. This one happens to share a lot of bit positions with the UHCI
 * specification, so that much of the uhci driver can just mask the bits
 * appropriately.
 */
/* Create various pipes... */
#define create_pipe(dev,endpoint) \
		(((dev)->devnum << 8) | (endpoint << 15) | \
		((dev)->speed << 26) | (dev)->maxpacketsize)
#define default_pipe(dev) ((dev)->speed << 26)

#define usb_sndctrlpipe(dev, endpoint)	((PIPE_CONTROL << 30) | \
					 create_pipe(dev, endpoint))
#define usb_rcvctrlpipe(dev, endpoint)	((PIPE_CONTROL << 30) | \
					 create_pipe(dev, endpoint) | \
					 USB_DIR_IN)
#define usb_sndisocpipe(dev, endpoint)	((PIPE_ISOCHRONOUS << 30) | \
					 create_pipe(dev, endpoint))
#define usb_rcvisocpipe(dev, endpoint)	((PIPE_ISOCHRONOUS << 30) | \
					 create_pipe(dev, endpoint) | \
					 USB_DIR_IN)
#define usb_sndbulkpipe(dev, endpoint)	((PIPE_BULK << 30) | \
					 create_pipe(dev, endpoint))
#define usb_rcvbulkpipe(dev, endpoint)	((PIPE_BULK << 30) | \
					 create_pipe(dev, endpoint) | \
					 USB_DIR_IN)
#define usb_sndintpipe(dev, endpoint)	((PIPE_INTERRUPT << 30) | \
					 create_pipe(dev, endpoint))
#define usb_rcvintpipe(dev, endpoint)	((PIPE_INTERRUPT << 30) | \
					 create_pipe(dev, endpoint) | \
					 USB_DIR_IN)
#define usb_snddefctrl(dev)		((PIPE_CONTROL << 30) | \
					 default_pipe(dev))
#define usb_rcvdefctrl(dev)		((PIPE_CONTROL << 30) | \
					 default_pipe(dev) | \
					 USB_DIR_IN)

/* The D0/D1 toggle bits */
#define usb_gettoggle(dev, ep, out) (((dev)->toggle[out] >> ep) & 1)
#define usb_dotoggle(dev, ep, out)  ((dev)->toggle[out] ^= (1 << ep))
#define usb_settoggle(dev, ep, out, bit) ((dev)->toggle[out] = \
						((dev)->toggle[out] & \
						 ~(1 << ep)) | ((bit) << ep))

/* Endpoint halt control/status */
#define usb_endpoint_out(ep_dir)	(((ep_dir >> 7) & 1) ^ 1)
#define usb_endpoint_halt(dev, ep, out) ((dev)->halted[out] |= (1 << (ep)))
#define usb_endpoint_running(dev, ep, out) ((dev)->halted[out] &= ~(1 << (ep)))
#define usb_endpoint_halted(dev, ep, out) ((dev)->halted[out] & (1 << (ep)))

#define usb_packetid(pipe)	(((pipe) & USB_DIR_IN) ? USB_PID_IN : \
				 USB_PID_OUT)

#define usb_pipeout(pipe)	((((pipe) >> 7) & 1) ^ 1)
#define usb_pipein(pipe)	(((pipe) >> 7) & 1)
#define usb_pipedevice(pipe)	(((pipe) >> 8) & 0x7f)
#define usb_pipe_endpdev(pipe)	(((pipe) >> 8) & 0x7ff)
#define usb_pipeendpoint(pipe)	(((pipe) >> 15) & 0xf)
#define usb_pipedata(pipe)	(((pipe) >> 19) & 1)
#define usb_pipespeed(pipe)	(((pipe) >> 26) & 3)
#define usb_pipeslow(pipe)	(usb_pipespeed(pipe) == USB_SPEED_LOW)
#define usb_pipetype(pipe)	(((pipe) >> 30) & 3)
#define usb_pipeisoc(pipe)	(usb_pipetype((pipe)) == PIPE_ISOCHRONOUS)
#define usb_pipeint(pipe)	(usb_pipetype((pipe)) == PIPE_INTERRUPT)
#define usb_pipecontrol(pipe)	(usb_pipetype((pipe)) == PIPE_CONTROL)
#define usb_pipebulk(pipe)	(usb_pipetype((pipe)) == PIPE_BULK)


/*************************************************************************
 * Hub Stuff
 */
struct usb_port_status {
	uint16 wPortStatus;
	uint16 wPortChange;
} __attribute__ ((packed));

struct usb_hub_status {
	uint16 wHubStatus;
	uint16 wHubChange;
} __attribute__ ((packed));


/* Hub descriptor */
struct usb_hub_descriptor {
	uint8  bLength;
	uint8  bDescriptorType;
	uint8  bNbrPorts;
	uint16 wHubCharacteristics;
	uint8  bPwrOn2PwrGood;
	uint8  bHubContrCurrent;
	uint8  DeviceRemovable[(USB_MAXCHILDREN+1+7)/8];
	uint8  PortPowerCtrlMask[(USB_MAXCHILDREN+1+7)/8];
	/* DeviceRemovable and PortPwrCtrlMask want to be variable-length
	   bitmaps that hold max 255 entries. (bit0 is ignored) */
} __attribute__ ((packed));


struct usb_hub_device {
	struct usb_device *pusb_dev;
	struct usb_hub_descriptor desc;
};

#endif /*_USB_H_ */

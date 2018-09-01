/*
	NatFeat host NF_STDERR /dev/nfstderr, MetaDOS BOS driver

	ARAnyM (C) 2005 STanda Opichal

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

/*--- Include ---*/

#include <stdlib.h>
#include <string.h>

#include "../natfeat/nf_ops.h"
#include "metados_bos.h"

/*--- Defines ---*/

#undef ENOSYS
#define ENOSYS	-32

#ifndef DEV_CONSOLE
#define DEV_CONSOLE	2
#endif

#define DRIVER_NAME	"ARAnyM NF_STDERR driver"
#define VERSION	"v0.1"

static const char device_name[METADOS_BOSDEVICE_NAMELEN]={
	"nfstderr"
};

/*--- Functions prototypes ---*/

void *init_driver(void);

metados_bosheader_t *asm_init_devices(void);
long asm_xopen(metaopen_t *metaopen);
long asm_xclose(void);
long asm_xread(void *buffer, unsigned long first, unsigned short length);
long asm_xwrite(void *buffer, unsigned long first, unsigned short length);
long asm_xseek(unsigned long offset);
long asm_xstatus(metastatus_t *ext_status);
long asm_xioctl(unsigned long magic, unsigned short opcode, void *buffer);

long xopen(metados_bosheader_t *device, metaopen_t *metaopen);
long xclose(metados_bosheader_t *device);
long xread(metados_bosheader_t *device, void *buffer, unsigned long first, unsigned long length);
long xwrite(metados_bosheader_t *device, void *buffer, unsigned long first, unsigned long length);
long xseek(metados_bosheader_t *device, unsigned long offset);
long xstatus(metados_bosheader_t *device, metastatus_t *ext_status);
long xioctl(metados_bosheader_t *device, unsigned long magic, unsigned long opcode, void *buffer);

metados_bosheader_t *init_devices(unsigned long phys_letter, unsigned long phys_channel);

static void press_any_key(void);

/*--- Local variables ---*/

unsigned long nf_stderr_id = 0;
long __CDECL (*nf_call)(long id, ...) = 0UL;

static metados_bosheader_t * (*device_init_f)(void)=asm_init_devices;

/*--- Functions ---*/

void *init_driver(void)
{
	const struct nf_ops *nf_ops;
	Cconws(
		"\033p " DRIVER_NAME " " VERSION " \033q\r\n"
		"Copyright (c) ARAnyM Development Team, " __DATE__ "\r\n"
	);

	/* Init the ID to 0 */
	nf_stderr_id = 0;

	nf_ops = nf_init();
	if ( ! nf_ops ) {
		Cconws("__NF cookie not present on this system\r\n");
		press_any_key();
		return &device_init_f;
	}

	nf_stderr_id = nf_ops->get_id("NF_STDERR");
	if (nf_stderr_id == 0) {
		Cconws("NF_STDERR function not present on this system\r\n");
		press_any_key();
		return &device_init_f;
	}

	/* store the nf_call pointer for faster calls */
	nf_call = nf_ops->call;

	return &device_init_f;
}

static void press_any_key(void)
{
	Cconws("- Press any key to continue -\r\n");
	Crawcin();
}

metados_bosheader_t *init_devices(unsigned long phys_letter, unsigned long phys_channel)
{
	unsigned char *newdevice;
	metados_bosheader_t *DefaultDevice;
	metados_bosfunctions_t *DefaultFunctions;

	if (nf_stderr_id == 0) {
		Cconws(" ARAnyM NF_STDERR device driver unavailable\r\n");
		return (metados_bosheader_t *)-39;
	}

	newdevice = (unsigned char *)Malloc(sizeof(metados_bosheader_t)+sizeof(metados_bosfunctions_t));
	if (newdevice == NULL) {
		return (metados_bosheader_t *)-39;
	}

	DefaultDevice = (metados_bosheader_t *) newdevice;
	DefaultFunctions = (metados_bosfunctions_t *) (newdevice + sizeof(metados_bosheader_t));

	DefaultDevice->next=NULL;
	DefaultDevice->attrib=0;
	DefaultDevice->phys_letter=phys_letter;
	DefaultDevice->dma_channel=phys_channel;
	DefaultDevice->functions=DefaultFunctions;
	DefaultDevice->functions->init=(long (*)(metainit_t *metainit)) 0xffffffffUL;
	DefaultDevice->functions->open=asm_xopen;
	DefaultDevice->functions->close=asm_xclose;
	DefaultDevice->functions->read=asm_xread;
	DefaultDevice->functions->write=asm_xwrite;
	DefaultDevice->functions->seek=asm_xseek;
	DefaultDevice->functions->status=asm_xstatus;
	DefaultDevice->functions->ioctl=asm_xioctl;
	DefaultDevice->functions->function08=(long (*)(void)) 0xffffffffUL;
	DefaultDevice->functions->function09=(long (*)(void)) 0xffffffffUL;
	DefaultDevice->functions->function0a=(long (*)(void)) 0xffffffffUL;
	DefaultDevice->functions->startaudio=(void*)0xffffffffUL;
	DefaultDevice->functions->stopaudio=(void*)0xffffffffUL;
	DefaultDevice->functions->setsongtime=(void*)0xffffffffUL;
	DefaultDevice->functions->gettoc=(void*)0xffffffffUL;
	DefaultDevice->functions->discinfo=(void*)0xffffffffUL;
	DefaultDevice->status=0;
	strncpy(DefaultDevice->name, device_name, METADOS_BOSDEVICE_NAMELEN);

	return DefaultDevice;
}

long xopen(metados_bosheader_t *device, metaopen_t *metaopen)
{
	metaopen->name = device->name;
	metaopen->reserved[0] = metaopen->reserved[1] = metaopen->reserved[2] = 0;

	return 0;
}

long xclose(metados_bosheader_t *device)
{
	return 0;
}

long xread(metados_bosheader_t *device, void *buffer, unsigned long first, unsigned long length)
{
	return 0;
}

long xwrite(metados_bosheader_t *device, void *buf, unsigned long first, unsigned long length)
{
	char outb[2] = { '\0', '\0' };
	long nwrite = 0;

	/**
	 * WARNING: this driver doesn't expect the incomming data first/length
	 * to be in bytes and NOT blocks of 2048 bytes as specified in the
	 * .BOS API specification.
	 *
	 * This is to me an acceptable hack to create .BOS character device
	 * drivers for BetaDOS. 
	 **/
	unsigned long bytes = length;
	while (bytes > 0)
	{
		/* call host os to print the data */

		/* byte by byte because of NF_STDERR operates with
		 * char* as with a string and not a byte array
		 * ('\0' would terminate the output)
		 **/
		outb[0] = ((char*)buf)[nwrite];
		nf_call(nf_stderr_id, outb);

		nwrite++;
		bytes--;
	}
	
	return nwrite;
}

long xseek(metados_bosheader_t *device, unsigned long offset)
{
	return 0;
}

long xstatus(metados_bosheader_t *device, metastatus_t *extended_status)
{
	if (extended_status != NULL) {
		memset(extended_status, 0, sizeof(metastatus_t));
	}

	return 0;
}

long xioctl(metados_bosheader_t *device, unsigned long magic, unsigned long opcode, void *buffer)
{
	if (magic != METADOS_IOCTL_MAGIC) {
		return ENOSYS;
	}

	switch (opcode) {
		case METADOS_IOCTL_BOSINFO:
			/* this device is character device */
			((bos_info_t*)buffer)->flags = BOS_INFO_ISTTY;
			return 0;
	}
	
	return ENOSYS;
}


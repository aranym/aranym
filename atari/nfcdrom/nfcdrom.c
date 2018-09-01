/*
	NatFeat host CD-ROM access, MetaDOS BOS driver

	ARAnyM (C) 2003 Patrice Mandin

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

#include <mint/cookie.h>
#include <mint/osbind.h>

#include "../natfeat/nf_ops.h"
#include "metados_bos.h"
#include "nfcdrom_nfapi.h"

/*--- Defines ---*/

#ifndef EINVFN
#define EINVFN	-32
#endif

#ifndef DEV_CONSOLE
#define DEV_CONSOLE	2
#endif

#define DRIVER_NAME	"ARAnyM host CD-ROM driver"
#define VERSION	"v0.2"

static const char device_name[METADOS_BOSDEVICE_NAMELEN]={
	"ARAnyM CD-ROM driver"
};

/*--- Functions prototypes ---*/

metados_bosheader_t *asm_init_devices(void);
long asm_cd_open(metaopen_t *metaopen);
long asm_cd_close(void);
long asm_cd_read(void *buffer, unsigned long first, unsigned short length);
long asm_cd_write(void *buffer, unsigned long first, unsigned short length);
long asm_cd_seek(unsigned long offset);
long asm_cd_status(metastatus_t *ext_status);
long asm_cd_ioctl(unsigned long magic, unsigned short opcode, void *buffer);
long asm_cd_startaudio(unsigned short dummy, metatracks_t *tracks);
long asm_cd_stopaudio(void);
long asm_cd_setsongtime(unsigned short dummy, unsigned long start_msf, unsigned long end_msf);
long asm_cd_gettoc(unsigned short dummy, metatocentry_t *toc_header);
long asm_cd_discinfo(metadiscinfo_t *discinfo);

long cd_open(metados_bosheader_t *device, metaopen_t *metaopen);
long cd_close(metados_bosheader_t *device);
long cd_read(metados_bosheader_t *device, void *buffer, unsigned long first, unsigned long length);
long cd_write(metados_bosheader_t *device, void *buffer, unsigned long first, unsigned long length);
long cd_seek(metados_bosheader_t *device, unsigned long offset);
long cd_status(metados_bosheader_t *device, metastatus_t *ext_status);
long cd_ioctl(metados_bosheader_t *device, unsigned long magic, unsigned long opcode, void *buffer);
long cd_startaudio(metados_bosheader_t *device, unsigned long dummy, metatracks_t *tracks);
long cd_stopaudio(metados_bosheader_t *device);
long cd_setsongtime(metados_bosheader_t *device, unsigned long dummy, unsigned long start_msf, unsigned long end_msf);
long cd_gettoc(metados_bosheader_t *device, unsigned long dummy, metatocentry_t *toc_header);
long cd_discinfo(metados_bosheader_t *device, metadiscinfo_t *discinfo);

metados_bosheader_t *init_devices(unsigned long phys_letter, unsigned long phys_channel);

static void press_any_key(void);

/*--- Local variables ---*/

static const struct nf_ops *nfOps;
static unsigned long nfCdRomId;
static unsigned long drives_mask;

static metados_bosheader_t * (*device_init_f)(void)=asm_init_devices;

/*--- Functions prototypes ---*/

void *init_driver(void);

/*--- Functions ---*/

void *init_driver(void)
{
	int i;
	char letter[2]={0,0};

	Cconws(
		"\033p " DRIVER_NAME " " VERSION " \033q\r\n"
		"Copyright (c) ARAnyM Development Team, " __DATE__ "\r\n"
	);

	drives_mask=0;

	nfOps = nf_init();
	if (!nfOps) {
		Cconws("__NF cookie not present on this system\r\n");
		press_any_key();
		return &device_init_f;
	}

	/* List present drives */
	nfCdRomId=nfOps->get_id("CDROM");
	if (nfCdRomId == 0) {
		Cconws("NF CD-ROM functions not present on this system\r\n");
		press_any_key();
		return &device_init_f;
	}

	drives_mask=nfOps->call(NFCDROM(NFCD_DRIVESMASK));
	Cconws(" Host drives present: ");
	for (i='A'; i<='Z'; i++) {
		if (drives_mask & (1<<(i-'A'))) {
			letter[0]=i;
			Cconws(letter);			
		}		
	}
	Cconws("\r\n");

	return &device_init_f;
}

static void press_any_key(void)
{
	Cconws("- Press any key to continue -\r\n");
	Crawcin();
}

metados_bosheader_t *init_devices(unsigned long phys_letter, unsigned long phys_channel)
{
	unsigned char *newcdrom;
	metados_bosheader_t *DefaultCdrom;
	metados_bosfunctions_t *DefaultFunctions;

	if (nfCdRomId == 0) {
		Cconws(" ARAnyM host CD-ROM driver unavailable\r\n");
		return (metados_bosheader_t *)-39;
	}

	newcdrom = (unsigned char *)Malloc(sizeof(metados_bosheader_t)+sizeof(metados_bosfunctions_t));
	if (newcdrom == NULL) {
		return (metados_bosheader_t *)-39;
	}

	DefaultCdrom = (metados_bosheader_t *) newcdrom;
	DefaultFunctions = (metados_bosfunctions_t *) (newcdrom + sizeof(metados_bosheader_t));

	DefaultCdrom->next=NULL;
	DefaultCdrom->attrib=0;
	DefaultCdrom->phys_letter=phys_letter;
	DefaultCdrom->dma_channel=phys_channel;
	DefaultCdrom->functions=DefaultFunctions;
	DefaultCdrom->functions->init=(long (*)(metainit_t *metainit)) 0xffffffffUL;
	DefaultCdrom->functions->open=asm_cd_open;
	DefaultCdrom->functions->close=asm_cd_close;
	DefaultCdrom->functions->read=asm_cd_read;
	DefaultCdrom->functions->write=asm_cd_write;
	DefaultCdrom->functions->seek=asm_cd_seek;
	DefaultCdrom->functions->status=asm_cd_status;
	DefaultCdrom->functions->ioctl=asm_cd_ioctl;
	DefaultCdrom->functions->function08=(long (*)(void)) 0xffffffffUL;
	DefaultCdrom->functions->function09=(long (*)(void)) 0xffffffffUL;
	DefaultCdrom->functions->function0a=(long (*)(void)) 0xffffffffUL;
	DefaultCdrom->functions->startaudio=asm_cd_startaudio;
	DefaultCdrom->functions->stopaudio=asm_cd_stopaudio;
	DefaultCdrom->functions->setsongtime=asm_cd_setsongtime;
	DefaultCdrom->functions->gettoc=asm_cd_gettoc;
	DefaultCdrom->functions->discinfo=asm_cd_discinfo;
	DefaultCdrom->status=0;
	strncpy(DefaultCdrom->name, device_name, METADOS_BOSDEVICE_NAMELEN);

	return DefaultCdrom;
}

long cd_open(metados_bosheader_t *device, metaopen_t *metaopen)
{
	metaopen->name = device->name;
	metaopen->reserved[0] = metaopen->reserved[1] = metaopen->reserved[2] = 0;

	return nfOps->call(NFCDROM(NFCD_OPEN), device, metaopen);
}

long cd_close(metados_bosheader_t *device)
{
	return nfOps->call(NFCDROM(NFCD_CLOSE), device);
}

long cd_read(metados_bosheader_t *device, void *buffer, unsigned long first, unsigned long length)
{
	return nfOps->call(NFCDROM(NFCD_READ), device, buffer, first, length);
}

long cd_write(metados_bosheader_t *device, void *buffer, unsigned long first, unsigned long length)
{
	return nfOps->call(NFCDROM(NFCD_WRITE), device, buffer, first, length);
}

long cd_seek(metados_bosheader_t *device, unsigned long offset)
{
	return nfOps->call(NFCDROM(NFCD_SEEK), device, offset);
}

long cd_status(metados_bosheader_t *device, metastatus_t *extended_status)
{
	if (extended_status != NULL) {
		memset(extended_status, 0, sizeof(metastatus_t));
	}

	return nfOps->call(NFCDROM(NFCD_STATUS), device, extended_status);
}

long cd_ioctl(metados_bosheader_t *device, unsigned long magic, unsigned long opcode, void *buffer)
{
	if (magic != METADOS_IOCTL_MAGIC) {
		return EINVFN;
	}
	
	return nfOps->call(NFCDROM(NFCD_IOCTL), device, opcode, buffer);
}

long cd_startaudio(metados_bosheader_t *device, unsigned long dummy, metatracks_t *tracks)
{
	return nfOps->call(NFCDROM(NFCD_STARTAUDIO), device, dummy, tracks);
}

long cd_stopaudio(metados_bosheader_t *device)
{
	return nfOps->call(NFCDROM(NFCD_STOPAUDIO), device);
}

long cd_setsongtime(metados_bosheader_t *device, unsigned long dummy, unsigned long start_msf, unsigned long end_msf)
{
	return nfOps->call(NFCDROM(NFCD_SETSONGTIME), device, dummy, start_msf, end_msf);
}

long cd_gettoc(metados_bosheader_t *device, unsigned long dummy, metatocentry_t *toc_header)
{
	return nfOps->call(NFCDROM(NFCD_GETTOC), device, dummy, toc_header);
}

long cd_discinfo(metados_bosheader_t *device, metadiscinfo_t *discinfo)
{
	return nfOps->call(NFCDROM(NFCD_DISCINFO), device, discinfo);
}

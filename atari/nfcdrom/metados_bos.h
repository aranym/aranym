/*
	MetaDOS BOS driver definitions

	ARAnyM (C) 2003 Patrice Mandin

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#ifndef _METADOS_BOS_H
#define _METADOS_BOS_H

#include "metados.h"

#define METADOS_BOSDEVICE_NAMELEN	32

typedef struct {
	long (*init)(metainit_t *metainit);
	long (*open)(metaopen_t *metaopen);
	long (*close)(void);
	long (*read)(void *buffer, unsigned long first, unsigned short length);
	long (*write)(void *buffer, unsigned long first, unsigned short length);
	long (*seek)(unsigned long offset);
	long (*status)(metastatus_t *extended_status);
	long (*ioctl)(unsigned long magic, unsigned short opcode, void *buffer);
	long (*function08)(void);
	long (*function09)(void);
	long (*function0a)(void);
	long (*startaudio)(unsigned short dummy, metatracks_t *tracks);
	long (*stopaudio)(void);
	long (*setsongtime)(unsigned short dummy, unsigned long start_msf, unsigned long end_msf);
	long (*gettoc)(unsigned short dummy, metatocentry_t *tocheader);
	long (*discinfo)(metadiscinfo_t *discinfo);
} __attribute__((packed)) metados_bosfunctions_t;

typedef struct {
	void *next;
	unsigned long attrib;
	unsigned short phys_letter;
	unsigned short dma_channel;
	unsigned short sub_device;
	metados_bosfunctions_t *functions;
	unsigned short status;
	unsigned long reserved[2];
	char name[METADOS_BOSDEVICE_NAMELEN];
} __attribute__((packed)) metados_bosheader_t;

#endif /* _METADOS_BOS_H */

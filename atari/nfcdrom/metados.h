/*
	Metados

	Copyright (C) 2002	Patrice Mandin

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

#ifndef _METADOS_H
#define _METADOS_H

#include <mint/falcon.h>	/* for trap_14_xxx macros */

#ifndef OSBIND_CLOBBER_LIST
#define OSBIND_CLOBBER_LIST	"d0", "d1", "d2", "a0", "a1", "a2", "memory"
#endif

#ifndef trap_14_wwlwl
#define trap_14_wwlwl(n,a,b,c,d)	\
__extension__	\
({	\
	register long retvalue __asm__("d0");	\
	short _a = (short)(a);	\
	long _b = (long)(b);	\
	short _c = (short)(c);	\
	long _d = (long)(d);	\
	\
	__asm__ volatile (	\
		"movl	%5,sp@-\n\t"	\
		"movw	%4,sp@-\n\t"	\
		"movl	%3,sp@-\n\t"	\
		"movw	%2,sp@-\n\t"	\
		"movw	%1,sp@-\n\t"	\
		"trap	#14\n\t"	\
		"lea	sp@(14),sp"	\
		: "=r"(retvalue)	\
		: "g"(n), "r"(_a), "r"(_b), "r"(_c), "r"(_d)	\
		: OSBIND_CLOBBER_LIST	\
	);	\
	retvalue;	\
})
#endif

/*--- Functions prototypes ---*/

#define Metainit(buffer)	\
	(void)trap_14_wl((short)0x30,(long)buffer)
#define Metaopen(drive,buffer)	\
	(long)trap_14_wwl((short)0x31,(short)drive,(long)buffer)
#define Metaclose(drive)	\
	(void)trap_14_ww((short)0x32,(short)drive)
#define Metaread(drive,buffer,first_block,nb_blocks)	\
	(long)trap_14_wwllw((short)0x33,(short)drive,(long)buffer,(long)first_block,(short)nb_blocks)
#define Metawrite(drive,buffer,first_block,nb_blocks)	\
	(long)trap_14_wwllw((short)0x34,(short)drive,(long)buffer,(long)first_block,(short)nb_blocks)
#define Metaseek(drive,dummy,offset)	\
	(long)trap_14_wwll((short)0x35,(short)(drive),(long)(dummy),(long)(offset))
#define Metastatus(void)	\
	(long)trap_14_wl((short)0x36,(long)(buffer))
#define Metaioctl(drive,magic,opcode,buffer)	\
	(long)trap_14_wwlwl((short)(0x37),(short)(drive),(long)(magic),(short)(opcode),(long)(buffer))

#define Metastartaudio(drive,dummy,metatracks_t_p)	\
	(long)trap_14_wwwl((short)(0x3b),(short)(drive),(short)(dummy),(long)(metatracks_t_p))
#define Metastopaudio(drive)	\
	(long)trap_14_ww((short)(0x3c),(short)(drive))
#define Metasetsongtime(drive,dummy,start,end)	\
	(long)trap_14_wwwll((short)(0x3d),(short)(drive),(short)(dummy),(long)(start),(long)(end))
#define Metagettoc(drive,dummy,metatocentry_t_p)	\
	(long)trap_14_wwwl((short)(0x3e),(short)(drive),(short)(dummy),(long)(metatocentry_t_p))
#define Metadiscinfo(drive,metadiscinfo_t_p)	\
	(long)trap_14_wwl((short)(0x3f),(short)(drive),(long)(metadiscinfo_t_p))

#define METADOS_IOCTL_MAGIC	(('F'<<24)|('C'<<16)|('T'<<8)|'L')

/*--- Types ---*/

typedef struct {
	unsigned short version;
	unsigned long magic;
	char *log2phys;
} metainfo_t __attribute__((packed));

typedef struct {
	unsigned long drives_map;
	char *version;
	unsigned long reserved;		
	metainfo_t *info;
} metainit_t __attribute__((packed));

typedef struct {
	char *name;
	unsigned long reserved[3];
} metaopen_t __attribute__((packed));

typedef struct {
	char ext_status[32];
} metastatus_t __attribute__((packed));

typedef struct {
	unsigned char count;
	unsigned char first;
} metatracks_t __attribute__((packed));

typedef struct {	/* TOC entry for MetaGetToc() function */
	unsigned char track;
	unsigned char minute;
	unsigned char second;
	unsigned char frame;
} metatocentry_t __attribute__((packed));

typedef struct {
	unsigned char reserved;
	unsigned char minute;
	unsigned char second;
	unsigned char frame;
} metadisc_msf_t __attribute__((packed));

typedef struct {	/* Discinfo for MetaDiscInfo() function */
	unsigned char disctype, first, last, current;
	metadisc_msf_t	relative;
	metadisc_msf_t	absolute;
	metadisc_msf_t	end;
	unsigned char index, reserved1[3];
	unsigned long reserved2[123];
} metadiscinfo_t __attribute__((packed));

#endif /* _METADOS_H */

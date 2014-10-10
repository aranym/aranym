/*
	NatFeat JPEG decoder

	ARAnyM (C) 2005 Patrice Mandin

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
#include "../nfpci/nfpci_cookie.h"
#include "nfjpeg_nfapi.h"
#include "jpgdh.h"

/*--- Defines ---*/

#ifndef EINVFN
#define EINVFN	-32
#endif

#ifndef DEV_CONSOLE
#define DEV_CONSOLE	2
#endif

#ifndef S_WRITE
#define S_WRITE	1
#endif

#ifndef MX_PREFTTRAM
#define MX_PREFTTRAM	3
#endif

#define DRIVER_NAME	"ARAnyM host JPEG driver"
#define VERSION	"v0.4"

#define CALLJPEGROUTINE(jpgd_ptr,func_ptr)	\
__extension__	\
({	\
	register long retvalue __asm__("d0");	\
	\
	__asm__ volatile (	\
		"movl	%1,a0\n\t"	\
		"jbsr	%2@"	\
		: "=r"(retvalue)	\
		: "a"(jpgd_ptr), "a"(func_ptr)	\
		: "a0", "memory" \
	);	\
	retvalue; \
})

/*--- Functions prototypes ---*/

JPGD_ENUM JpegDecOpenDriver(struct _JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM JpegDecCloseDriver(struct _JPGD_STRUCT *jpgd_ptr);
long JpegDecGetStructSize(void);
JPGD_ENUM JpegDecGetImageInfo(struct _JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM JpegDecGetImageSize(struct _JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM JpegDecDecodeImage(struct _JPGD_STRUCT *jpgd_ptr);

JPGD_ENUM OpenDriver(struct _JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM CloseDriver(struct _JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM GetImageInfo(struct _JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM GetImageSize(struct _JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM DecodeImage(struct _JPGD_STRUCT *jpgd_ptr);

static void install_jpeg(void);
static void press_any_key(void);

static void *Atari_MxAlloc(unsigned long size);

/*--- Local variables ---*/

static struct nf_ops *nfOps;
static unsigned long nfJpegId;

static JPGDDRV_STRUCT nfjpeg_cookie = {
	0x00000001,
	OpenDriver,
	CloseDriver,
	JpegDecGetStructSize,
	GetImageInfo,
	GetImageSize,
	DecodeImage
};

/*--- Functions ---*/

void install_driver(unsigned long resident_length)
{
	(void) Cconws(
		"\033p " DRIVER_NAME " " VERSION " \033q\r\n"
		"Copyright (c) ARAnyM Development Team, " __DATE__ "\r\n"
	);

	/* Check if _JPD already installed */
	if (cookie_present(C__JPD, NULL)) {
		(void) Cconws("A JPEG driver is already installed on this system\r\n");
		press_any_key();
		return;
	}	

	/* Check if NF is present for NFJPEG */
	nfOps = nf_init();
	if (!nfOps) {
		(void) Cconws("Native Features not present on this system\r\n");
		press_any_key();
		return;
	}

	nfJpegId = nfOps->get_id("JPEG");
	if (nfJpegId==0) {
		(void) Cconws("NF JPEG functions not present on this system\r\n");
		press_any_key();
		return;
	}	

	install_jpeg();

	Ptermres(resident_length, 0);
	for(;;);	/* Never ending loop, should not go there */
}

static void press_any_key(void)
{
	(void) Cconws("- Press any key to continue -\r\n");
	Crawcin();
}

static void install_jpeg(void)
{
	/* Add our cookie */
	cookie_add(C__JPD, (unsigned long)&nfjpeg_cookie);
}

JPGD_ENUM JpegDecOpenDriver(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_OPENDRIVER), jpgd_ptr);
}

JPGD_ENUM JpegDecCloseDriver(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_CLOSEDRIVER), jpgd_ptr);
}

long JpegDecGetStructSize(void)
{
	return sizeof(struct _JPGD_STRUCT);
}

JPGD_ENUM JpegDecGetImageInfo(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_GETIMAGEINFO), jpgd_ptr);
}

JPGD_ENUM JpegDecGetImageSize(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_GETIMAGESIZE), jpgd_ptr);
}

JPGD_ENUM JpegDecDecodeImage(struct _JPGD_STRUCT *jpgd_ptr)
{
	int row_length, y;

	row_length = jpgd_ptr->XLoopCounter * 16 * 16 * jpgd_ptr->OutComponents;
	jpgd_ptr->OutTmpHeight = 0;
	jpgd_ptr->MCUsCounter = jpgd_ptr->XLoopCounter * jpgd_ptr->YLoopCounter;

	if (jpgd_ptr->OutFlag==0) {
		/* Allocate memory to hold complete image */
		if (jpgd_ptr->OutPointer==NULL) {
			return NOTENOUGHMEMORY;
		}

		jpgd_ptr->OutTmpPointer = jpgd_ptr->OutPointer;
		for (y=0;y<jpgd_ptr->YLoopCounter;y++) {
			/* Decode Y row in OutTmpPointer */
			nfOps->call(NFJPEG(NFJPEG_DECODEIMAGE), jpgd_ptr, y);

			if (jpgd_ptr->UserRoutine) {
				CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->UserRoutine);
			}

			jpgd_ptr->OutTmpPointer += row_length;
			jpgd_ptr->OutTmpHeight += 16;
			jpgd_ptr->MCUsCounter -= jpgd_ptr->XLoopCounter;
		}

		jpgd_ptr->MFDBAddress = jpgd_ptr->OutPointer;
	} else {
		unsigned char *filename="output.tga";
		long handle;

		jpgd_ptr->OutTmpPointer = Atari_MxAlloc(jpgd_ptr->XLoopCounter * 16 * 16 * 4);
		if (jpgd_ptr->OutTmpPointer==NULL) {
			return NOTENOUGHMEMORY;
		}

		/* Open file */
		if (jpgd_ptr->Create) {
			handle = CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->Create);
		} else {
			if (jpgd_ptr->OutPointer) {
				filename = jpgd_ptr->OutPointer;
			}
			handle = Fopen(filename, S_WRITE);
		}
		if (handle<0) {
			Mfree(jpgd_ptr->OutTmpPointer);
			return (JPGD_ENUM) handle;
		}
		jpgd_ptr->OutHandle = handle & 0xffff;

		/* Write part of image to file */
		for (y=0;y<jpgd_ptr->YLoopCounter;y++) {
			/* Decode Y row in OutTmpPointer */
			nfOps->call(NFJPEG(NFJPEG_DECODEIMAGE), jpgd_ptr, y);
			jpgd_ptr->OutTmpHeight = jpgd_ptr->MFDBPixelHeight - y*16;
			if (jpgd_ptr->OutTmpHeight > 16) {
				jpgd_ptr->OutTmpHeight = 16;
			}

			if (jpgd_ptr->UserRoutine) {
				CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->UserRoutine);
			}

			if (jpgd_ptr->Write) {
				if (CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->Write)<0) {
					break;
				}
			} else {
				Fwrite(jpgd_ptr->OutHandle, row_length, jpgd_ptr->OutTmpPointer);
			}

			jpgd_ptr->MCUsCounter -= jpgd_ptr->XLoopCounter;
		}

		/* Close file */
		if (jpgd_ptr->Close) {
			CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->Close);
		} else  {
			Fclose(jpgd_ptr->OutHandle);
		}

		Mfree(jpgd_ptr->OutTmpPointer);
	}

	return 0;
}

static void *Atari_MxAlloc(unsigned long size)
{
	if (((Sversion()&0xFF)>=0x01) | (Sversion()>=0x1900)) {
		return (void *)Mxalloc(size, MX_PREFTTRAM);
	}

	return (void *)Malloc(size);
}

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

#include "../natfeat/natfeat.h"
#include "../nfpci/nfpci_cookie.h"
#include "nfjpeg_nfapi.h"
#include "jpgdh.h"

/*--- Defines ---*/

#ifndef EINVFN
#define EINVFN	-32
#endif

#ifndef C___NF
#define C___NF	0x5f5f4e46L
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
#define VERSION	"v0.1"

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

static const char *jpd_already_installed="A JPEG driver is already installed on this system\r\n";
static const char *nf_not_present="NatFeats not present on this system";

/*--- Functions ---*/

void install_driver(unsigned long resident_length)
{
	unsigned long dummy, cookie_nf;
	unsigned short nf_present;

	Cconws(
		"\033p " DRIVER_NAME " " VERSION " \033q\r\n"
		"Copyright (c) ARAnyM Development Team, " __DATE__ "\r\n"
	);

	/* Check if _JPD already installed */
	if (cookie_present(C__JPD, NULL)) {
		Cconws(jpd_already_installed);
		press_any_key();
		return;
	}	

	nf_present=(Getcookie(C___NF, &dummy) == C_FOUND);

	/* Check if NF is present for PCI */
	if (!cookie_present(C___NF, &cookie_nf)) {
		Cconws(nf_not_present);
		press_any_key();
		return;
	}	

	nfJpegId = nfGetID(("JPEG"));
	if (nfJpegId==0) {
		Cconws(nf_not_present);
		press_any_key();
		return;
	}

	install_jpeg();

	Ptermres(resident_length, 0);
	for(;;);	/* Never ending loop, should not go there */
}

static void press_any_key(void)
{
	Cconws("- Press any key to continue -");
	while (Bconstat(DEV_CONSOLE)==0);
}

static void install_jpeg(void)
{
	/* Add our cookie */
	cookie_add(C__JPD, (unsigned long)&nfjpeg_cookie);
}

JPGD_ENUM JpegDecOpenDriver(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfCall((NFJPEG(NFJPEG_OPENDRIVER), jpgd_ptr));
}

JPGD_ENUM JpegDecCloseDriver(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfCall((NFJPEG(NFJPEG_CLOSEDRIVER), jpgd_ptr));
}

long JpegDecGetStructSize(void)
{
	return sizeof(struct _JPGD_STRUCT);
}

JPGD_ENUM JpegDecGetImageInfo(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfCall((NFJPEG(NFJPEG_GETIMAGEINFO), jpgd_ptr));
}

JPGD_ENUM JpegDecGetImageSize(struct _JPGD_STRUCT *jpgd_ptr)
{
	return nfCall((NFJPEG(NFJPEG_GETIMAGESIZE), jpgd_ptr));
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
			nfCall((NFJPEG(NFJPEG_DECODEIMAGE), jpgd_ptr, y));

			if (jpgd_ptr->UserRoutine) {
				jpgd_ptr->UserRoutine(jpgd_ptr);
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
			handle = jpgd_ptr->Create(jpgd_ptr);
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
			nfCall((NFJPEG(NFJPEG_DECODEIMAGE), jpgd_ptr, y));

			if (jpgd_ptr->UserRoutine) {
				jpgd_ptr->UserRoutine(jpgd_ptr);
			}

			if (jpgd_ptr->Write) {
				if ((jpgd_ptr->Write(jpgd_ptr))<0) {
					break;
				}
			} else {
				Fwrite(jpgd_ptr->OutHandle, row_length, jpgd_ptr->OutTmpPointer);
			}

			jpgd_ptr->MCUsCounter -= jpgd_ptr->XLoopCounter;
			jpgd_ptr->OutTmpHeight += 16;
		}

		/* Close file */
		if (jpgd_ptr->Close) {
			jpgd_ptr->Close(jpgd_ptr);
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
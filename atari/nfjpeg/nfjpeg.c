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

static __inline__ long CALLJPEGROUTINE(JPGD_STRUCT *jpgd_ptr, short (*func_ptr)(JPGD_STRUCT *ptr))
{
	register short retvalue __asm__("d0");

	__asm__ volatile (
		" movl	%[jpgd_ptr],%%a0\n"
		" jbsr	%[func_ptr]@\n"
		: "=r"(retvalue)
		: [jpgd_ptr]"a"(jpgd_ptr), [func_ptr]"a"(func_ptr)
		/* the called functions have Pure-C calling conventions, and might trash d2 */
		: __CLOBBER_RETURN("d0") "d1", "d2", "a0", "a1", "cc", "memory"
	);
	return retvalue;
}

/*--- Functions prototypes ---*/

JPGD_ENUM JpegDecOpenDriver(JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM JpegDecCloseDriver(JPGD_STRUCT *jpgd_ptr);
long JpegDecGetStructSize(void);
JPGD_ENUM JpegDecGetImageInfo(JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM JpegDecGetImageSize(JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM JpegDecDecodeImage(JPGD_STRUCT *jpgd_ptr);

JPGD_ENUM OpenDriver(JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM CloseDriver(JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM GetImageInfo(JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM GetImageSize(JPGD_STRUCT *jpgd_ptr);
JPGD_ENUM DecodeImage(JPGD_STRUCT *jpgd_ptr);

static void install_jpeg(void);

/*--- Local variables ---*/

static const struct nf_ops *nfOps;
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

static void *Atari_MxAlloc(unsigned long size)
{
	void *ptr = (void *)Mxalloc(size, MX_PREFTTRAM);
	if ((long)ptr == -32)
		ptr = (void *)Malloc(size);
	return ptr;
}

void install_driver(unsigned long resident_length)
{
	(void) Cconws(
		"\033p " DRIVER_NAME " " VERSION " \033q\r\n"
		"Copyright (c) ARAnyM Development Team, " __DATE__ "\r\n"
	);

	/* Check if _JPD already installed */
	if (cookie_present(C__JPD, NULL))
	{
		(void) Cconws("A JPEG driver is already installed on this system\r\n");
		return;
	}	

	/* Check if NF is present for NFJPEG */
	nfOps = nf_init();
	if (!nfOps)
	{
		(void) Cconws("Native Features not present on this system\r\n");
		return;
	}

	nfJpegId = nfOps->get_id("JPEG");
	if (nfJpegId == 0)
	{
		(void) Cconws("NF JPEG functions not present on this system\r\n");
		return;
	}	

	if (nfOps->call(NFJPEG(GET_VERSION)) != ARANFJPEG_NFAPI_VERSION)
	{
		(void) Cconws("NF JPEG: wrong interface version\r\n");
		return;
	}

	{
		long size = nfOps->call(NFJPEG(NFJPEG_GETSTRUCTSIZE));
		if (size != -32 && size != sizeof(JPGD_STRUCT))
			(void) Cconws("NF JPEG: warning: struct size mismatch\r\n");
	}

	install_jpeg();

	Ptermres(resident_length, 0);
	for(;;);	/* Never ending loop, should not go there */
}

static void install_jpeg(void)
{
	/* Add our cookie */
	cookie_add(C__JPD, (unsigned long)&nfjpeg_cookie);
}

JPGD_ENUM JpegDecOpenDriver(JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_OPENDRIVER), jpgd_ptr);
}

JPGD_ENUM JpegDecCloseDriver(JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_CLOSEDRIVER), jpgd_ptr);
}

long JpegDecGetStructSize(void)
{
	long size = nfOps->call(NFJPEG(NFJPEG_GETSTRUCTSIZE));
	if (size == -32)
		size = sizeof(JPGD_STRUCT);
	return size;
}

JPGD_ENUM JpegDecGetImageInfo(JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_GETIMAGEINFO), jpgd_ptr);
}

JPGD_ENUM JpegDecGetImageSize(JPGD_STRUCT *jpgd_ptr)
{
	return nfOps->call(NFJPEG(NFJPEG_GETIMAGESIZE), jpgd_ptr);
}

JPGD_ENUM JpegDecDecodeImage(JPGD_STRUCT *jpgd_ptr)
{
	long row_length;
	JPGD_ENUM err = NOERROR;
	short xMCUs = 16;
	short yRows = 16;
	short yLoop, y;
	const struct nf_ops *ops = nfOps;

	row_length = (long)jpgd_ptr->XLoopCounter * xMCUs * yRows * jpgd_ptr->OutPixelSize;
	jpgd_ptr->OutTmpHeight = 0;
	yLoop = jpgd_ptr->YLoopCounter;
	jpgd_ptr->MCUsCounter = jpgd_ptr->XLoopCounter * yLoop;

	if (jpgd_ptr->OutFlag == 0)
	{
		/* Allocate memory to hold complete image */
		if (jpgd_ptr->OutPointer == NULL)
		{
			return NOTENOUGHMEMORY;
		}

		jpgd_ptr->OutTmpPointer = jpgd_ptr->OutPointer;
		for (y = 0; y < yLoop; y++)
		{
			jpgd_ptr->OutTmpHeight = jpgd_ptr->MFDBPixelHeight - y * yRows;
			if (jpgd_ptr->OutTmpHeight > yRows)
				jpgd_ptr->OutTmpHeight = yRows;

			/* Decode Y row in OutTmpPointer */
			jpgd_ptr->User[0] = y;
			err = ops->call(NFJPEG(NFJPEG_DECODEIMAGE), jpgd_ptr, (long)y);
			if (err != NOERROR)
				break;

			jpgd_ptr->MCUsCounter -= jpgd_ptr->XLoopCounter;
			if (jpgd_ptr->UserRoutine)
			{
				if (CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->UserRoutine) != 0)
				{
					err = USERABORT;
					break;
				}
			}

			jpgd_ptr->OutTmpPointer += row_length;
		}

		jpgd_ptr->MFDBAddress = jpgd_ptr->OutPointer;
	} else
	{
		const char *filename="output.tga";
		short handle;

		jpgd_ptr->OutTmpPointer = Atari_MxAlloc(jpgd_ptr->XLoopCounter * xMCUs * yRows * 4);
		if (jpgd_ptr->OutTmpPointer == NULL)
		{
			return NOTENOUGHMEMORY;
		}

		/* Open file */
		if (jpgd_ptr->Create)
		{
			handle = CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->Create);
		} else
		{
			if (jpgd_ptr->OutPointer)
			{
				filename = jpgd_ptr->OutPointer;
			}
			handle = Fcreate(filename, 0);
		}
		if (handle < 0)
		{
			Mfree(jpgd_ptr->OutTmpPointer);
			jpgd_ptr->OutTmpPointer = NULL;
			return (JPGD_ENUM) handle;
		}
		jpgd_ptr->OutHandle = handle;

		if (jpgd_ptr->Create == 0 &&
			jpgd_ptr->Write == 0 &&
			jpgd_ptr->Close == 0)
		{
			unsigned char tga_header[18];

			tga_header[ 0] = 0; /* length of image ID field - we don't have one */
			tga_header[ 1] = 0; /* color map type - none */
			tga_header[ 2] = jpgd_ptr->OutPixelSize == 1 ? 3 : 2; /* image type - uncompressed grayscale/true color */
			tga_header[ 3] = 0; /* color map specification */
			tga_header[ 4] = 0;
			tga_header[ 5] = 0;
			tga_header[ 6] = 0;
			tga_header[ 7] = 0;
			tga_header[ 8] = 0; /* X-origin */
			tga_header[ 9] = 0;
			tga_header[10] = 0; /* Y-origin */
			tga_header[11] = 0;
			tga_header[12] = jpgd_ptr->MFDBPixelWidth; /* image width */
			tga_header[13] = jpgd_ptr->MFDBPixelWidth >> 8;
			tga_header[14] = jpgd_ptr->MFDBPixelHeight; /* image height */
			tga_header[15] = jpgd_ptr->MFDBPixelHeight >> 8;
			tga_header[16] = jpgd_ptr->OutPixelSize * 8; /* bits per pixel */
			tga_header[17] = 0x20; /* image descriptor: origin in upper left, no alpha */
			if (Fwrite(jpgd_ptr->OutHandle, sizeof(tga_header), tga_header) != sizeof(tga_header))
			{
				err = DISKFULL;
				yLoop = 0;
			}
		}

		/* Write part of image to file */
		for (y = 0; y < yLoop; y++)
		{
			jpgd_ptr->OutTmpHeight = jpgd_ptr->MFDBPixelHeight - y * yRows;
			if (jpgd_ptr->OutTmpHeight > yRows)
				jpgd_ptr->OutTmpHeight = yRows;

			/* Decode Y row in OutTmpPointer */
			err = ops->call(NFJPEG(NFJPEG_DECODEIMAGE), jpgd_ptr, (long)y);
			if (err != NOERROR)
				break;

			jpgd_ptr->MCUsCounter -= jpgd_ptr->XLoopCounter;
			if (jpgd_ptr->UserRoutine)
			{
				if (CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->UserRoutine) != 0)
				{
					err = USERABORT;
					break;
				}
			}

			if (jpgd_ptr->Write)
			{
				if (CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->Write) < 0)
				{
					err = DISKFULL;
					break;
				}
			} else
			{
				long out_width = jpgd_ptr->MFDBPixelWidth * jpgd_ptr->OutPixelSize;
				long in_width = (long)jpgd_ptr->XLoopCounter * xMCUs * jpgd_ptr->OutPixelSize;
				short row;
				const char *in = jpgd_ptr->OutTmpPointer;

				for (row = 0; row < jpgd_ptr->OutTmpHeight && err == NOERROR; row++)
				{
					if (Fwrite(jpgd_ptr->OutHandle, out_width, in) != out_width)
						err = DISKFULL;
					in += in_width;
				}
				if (err != NOERROR)
					break;
			}
		}

		/* Close file */
		if (jpgd_ptr->Close)
		{
			CALLJPEGROUTINE(jpgd_ptr, jpgd_ptr->Close);
		} else
		{
			Fclose(jpgd_ptr->OutHandle);
		}

		Mfree(jpgd_ptr->OutTmpPointer);
		jpgd_ptr->OutTmpPointer = NULL;
	}

	return err;
}

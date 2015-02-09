/*
 * STanda 2004
 *
 * GPL
 */

#ifndef _FVDIDRV_NFAPI_H
#define _FVDIDRV_NFAPI_H

/* if you change anything in the enum {} below you have to increase
   this FVDIDRV_NFAPI_VERSION!

   fVDI v0.960 driver API, fVDI Natfeat v1.400
*/
#define FVDIDRV_NFAPI_VERSION    0x14000960L

enum {
	FVDI_GET_VERSION = 0,	/* subID = 0 */
	FVDI_GET_PIXEL = 1,
	FVDI_PUT_PIXEL = 2,
	FVDI_MOUSE = 3,
	FVDI_EXPAND_AREA = 4,
	FVDI_FILL_AREA = 5,
	FVDI_BLIT_AREA = 6,
	FVDI_LINE = 7,
	FVDI_FILL_POLYGON = 8,
	FVDI_GET_HWCOLOR = 9,
	FVDI_SET_COLOR = 10,
	FVDI_GET_FBADDR = 11,
	FVDI_SET_RESOLUTION = 12,
	FVDI_GET_WIDTH = 13,
	FVDI_GET_HEIGHT = 14,
	FVDI_OPENWK = 15,
	FVDI_CLOSEWK = 16,
	FVDI_GETBPP = 17,
	FVDI_EVENT = 18,
	FVDI_TEXT_AREA = 19
#if 0
	, FVDI_GETCOMPONENT = 20
#endif
};

extern unsigned long nfFvdiDrvId;

#endif /* _FVDIDRV_NFAPI_H */

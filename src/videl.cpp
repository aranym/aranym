/*
 * $Header$
 *
 * Joy 2001
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "hostscreen.h"
#include "videl.h"
#include "hardware.h"
#include "parameters.h"

// from hardware.cpp
extern HostScreen hostScreen;

static const int HW = 0xff8200;


VIDEL::VIDEL()
{
	// SelectVideoMode();
	// reasonable default values
	width = 640;
	height = 480;
	od_posledni_zmeny = 0;

	host_colors_uptodate = false;
}

void VIDEL::init() {
	hostScreen.setWindowSize( width, height );
}

// monitor writting to Falcon color palette registers
void VIDEL::handleWrite(uaecptr addr, uint8 value)
{
	BASE_IO::handleWrite(addr, value);
	if (addr >= 0xff9800 && addr < 0xffa200)	// Falcon palette color registers
		host_colors_uptodate = false;
}

long VIDEL::getVideoramAddress()
{
	return (handleRead(HW + 1) << 16) | (handleRead(HW + 3) << 8) |
		handleRead(HW + 0x0d);
}

int VIDEL::getScreenBpp()
{
	int f_shift = handleReadW(HW + 0x66);
	int st_shift = handleReadW(HW + 0x60);
	/* to get bpp, we must examine f_shift and st_shift.
	 * f_shift is valid if any of bits no. 10, 8 or 4
	 * is set. Priority in f_shift is: 10 ">" 8 ">" 4, i.e.
	 * if bit 10 set then bit 8 and bit 4 don't care...
	 * If all these bits are 0 get display depth from st_shift
	 * (as for ST and STE)
	 */
	int bits_per_pixel = 1;
	if (f_shift & 0x400)		/* 2 colors */
		bits_per_pixel = 1;
	else if (f_shift & 0x100)	/* hicolor */
		bits_per_pixel = 16;
	else if (f_shift & 0x010)	/* 8 bitplanes */
		bits_per_pixel = 8;
	else if (st_shift == 0)
		bits_per_pixel = 4;
	else if (st_shift == 0x100)
		bits_per_pixel = 2;
	else						/* if (st_shift == 0x200) */
		bits_per_pixel = 1;

	return bits_per_pixel;
}

int VIDEL::getScreenWidth()
{
	return handleReadW(HW + 0x10) * 16 / getScreenBpp();
}

int VIDEL::getScreenHeight()
{
	int vdb = handleReadW(HW + 0xa8);
	int vde = handleReadW(HW + 0xaa);
	int vmode = handleReadW(HW + 0xc2);

	/* visible y resolution:
	 * Graphics display starts at line VDB and ends at line
	 * VDE. If interlace mode off unit of VC-registers is
	 * half lines, else lines.
	 */
	int yres = vde - vdb;
	if (!(vmode & 0x02))		// interlace
		yres >>= 1;
	if (vmode & 0x01)			// double
		yres >>= 1;

	return yres;
}


void VIDEL::updateColors()
{
	if (!host_colors_uptodate) {
		// Prepare the native format color values
		int vdi2pix[256] =
			{ 0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1 };
		for (int i = 16; i < 256; i++)
			vdi2pix[i] = i;
		
		// map the colortable into the correct pixel format
#define TOS_COLORS(i)	handleRead(0xff9800 + (i))
		for (int i = 0; i < 256; i++) {
			int offset = i /*vdi2pix[i]*/ << 2;
			hostScreen.setPaletteColor( i,
										TOS_COLORS(offset),
										TOS_COLORS(offset + 1),
										TOS_COLORS(offset + 3) );
		}

		// special hack for 1 and 2 bitplane modes
		switch (getScreenBpp()) {
		case 1:				// set the black in 1 plane mode to the index 1
			hostScreen.mapPaletteColor( 1, 15 );
			break;
		case 2:				// set the black to the index 3 in 2 plane mode (4 colors)
			hostScreen.mapPaletteColor( 3, 15 );
			break;
		}

		host_colors_uptodate = true;
	}
}

void VIDEL::renderScreen()
{
	int vw = getScreenWidth();
	int vh = getScreenHeight();
	if (od_posledni_zmeny > 2) {
		if (vw > 0 && vw != width) {
			width = vw;
			od_posledni_zmeny = 0;
		}
		if (vh > 0 && vh != height) {
			height = vh;
			od_posledni_zmeny = 0;
		}
	}
	if (od_posledni_zmeny == 3) {
		hostScreen.setWindowSize( width, height );
	}
	if (od_posledni_zmeny < 4) {
		od_posledni_zmeny++;
		return;
	}

	if (!hostScreen.renderBegin())
		return;

	VideoRAMBaseHost = (uint8 *) hostScreen.getVideoramAddress();
	uint16 *fvram = (uint16 *) get_real_address_direct(this->getVideoramAddress());
	uint16 *hvram = (uint16 *) VideoRAMBaseHost;
	int bpp = getScreenBpp();

	uint8 destBPP = hostScreen.getBpp();
	if (bpp < 16) {
		updateColors();

		// The SDL colors blitting...
		// FIXME: The destBPP tests should probably not
		//        be inside the loop... (reorganise?)
		uint16 words[bpp];
		int screenlen = vw * vh / 16;
		for (int i = 0; i < screenlen; i++) {
			// perform byteswap (prior to plane to chunky conversion)
			for (int l = 0; l < bpp; l++) {
				uint16 b = fvram[i * bpp + l];
				words[l] = (b >> 8) | ((b & 0xff) << 8);	// byteswap
			}

			// To support other formats see
			// the SDL video examples (docs 1.1.7)
			if (destBPP == 2) {
				// bitplane to chunky conversion
				for (int j = 0; j < 16; j++) {
					uint8 color = 0;
					for (int l = bpp - 1; l >= 0; l--) {
						color <<= 1;
						color |= (words[l] >> (15 - j)) & 1;
					}
					((uint16 *) hvram)[(i * 16 + j)] = (uint16) hostScreen.getPaletteColor(color);
				}
			}
			else if (destBPP == 4) {
				// bitplane to chunky conversion
				for (int j = 0; j < 16; j++) {
					uint8 color = 0;
					for (int l = bpp - 1; l >= 0; l--) {
						color <<= 1;
						color |= (words[l] >> (15 - j)) & 1;
					}
					((uint32 *) hvram)[(i * 16 + j)] = (uint32) hostScreen.getPaletteColor(color);
				}
			}
			// FIXME: support for destBPP other than 2 or 4 BPP is missing
		}
	}
	else {
		int screenlen = vw * vh;
		// Falcon TC (High Color)
		if (destBPP == 2) {
			if ( /* videocard memory in Motorola endian format */ false) {
				memcpy(hvram, fvram, screenlen * 2);
			}
			else {
				for (int i = 0; i < screenlen; i++) {
					// byteswap
					int data = fvram[i];
					((uint16 *) hvram)[i] = (data >> 8) | ((data & 0xff) << 8);
				}
			}

		}
		else if (destBPP == 4) {
			for (int i = 0; i < screenlen; i++) {
				// The byteswap is done by correct shifts (not so obvious) 
				int data = fvram[i];
				((uint32 *) hvram)[i] =
					hostScreen.getColor(
						(uint8) ((data >> 5) & 0xf8),
						(uint8) ( ((data & 0x07) << 5) |
								  ((data >> 11) & 0x3c)),
						(uint8) (data & 0xf8));
			}
		}
		// FIXME: support for destBPP other than 2 or 4 BPP is missing
	}

    hostScreen.renderEnd();

	hostScreen.update();
}


/*
 * $Log$
 * Revision 1.14  2001/06/18 08:15:23  standa
 * lockScreen() moved to the very begining of the renderScreen() method.
 * unlockScreen() call added before updateScreen() call (again ;()
 *
 * Revision 1.13  2001/06/17 21:23:35  joy
 * late init() added
 * tried to fix colors in 2,4 bit color depth - didn't help
 *
 * Revision 1.12  2001/06/15 14:14:46  joy
 * VIDEL palette registers are now processed by the VIDEL object.
 *
 * Revision 1.11  2001/06/13 07:12:39  standa
 * Various methods renamed to conform the sementics.
 * Added videl fuctions needed for VDI driver.
 *
 * Revision 1.10  2001/06/13 06:22:21  standa
 * Another comment fixed.
 *
 */

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

#define DEBUG 0
#include "debug.h"

#include "gfxprimitives.h"


// from host.cpp
extern HostScreen hostScreen;

// Support for other than 2 byte hostscreen bpp is disabled by default
#define SUPPORT_MULTIPLEDESTBPP


static const uae_u32 HW = 0xff8200UL;


VIDEL::VIDEL()
{
	// SelectVideoMode();
	// reasonable default values
	width = 640;
	height = 480;
	doRender = true; // the rendering is on by default (VIDEL does the bitplane to chunky conversion)

	od_posledni_zmeny = 0;

	hostColorsSync = false;
}

void VIDEL::init() {
	hostScreen.setWindowSize( width, height, 16 );
}

// monitor writting to Falcon and ST/E color palette registers
void VIDEL::handleWrite(uaecptr addr, uint8 value)
{
	BASE_IO::handleWrite(addr, value);

	if ((addr >= 0xff9800 && addr < 0xffa200) || (addr >= 0xff8240 && addr < 0xff8260))
		hostColorsSync = false;

	if ((addr & ~3) == HW)	// Atari tries to change the VideoRAM address (after a RESET?)
		doRender = true;	// that's a sign that Videl should render the screen
}

long VIDEL::getVideoramAddress()
{
	return (handleRead(HW + 1) << 16) | (handleRead(HW + 3) << 8) | handleRead(HW + 0x0d);
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
	if (hostColorsSync)
		return;

	D(bug("ColorUpdate in progress"));

	// Test the ST compatible set or not.
	bool stCompatibleColorPalette = false;

	int st_shift = handleReadW(HW + 0x60);
	if (st_shift == 0) {		   // bpp == 4
		int hreg = handleReadW(HW + 0x82); // Too lame!
		if (hreg == 0x10 | hreg == 0x17 | hreg == 0x3e)	  // Better way how to make out ST LOW mode wanted
			stCompatibleColorPalette = true;

		D(bug("ColorUpdate %x", hreg));
	}
	else if (st_shift == 0x100)	   // bpp == 2
		stCompatibleColorPalette = true;
	else						   // bpp == 1	// if (st_shift == 0x200)
		stCompatibleColorPalette = true;

	// map the colortable into the correct pixel format

#define F_COLORS(i) handleRead(0xff9800 + (i))
#define STE_COLORS(i)	handleRead(0xff8240 + (i))

	if (!stCompatibleColorPalette) {
		for (int i = 0; i < 256; i++) {
			int offset = i << 2;
			hostScreen.setPaletteColor( i,
										F_COLORS(offset),
										F_COLORS(offset + 1),
										F_COLORS(offset + 3) );
		}
		hostScreen.updatePalette( 256 );
	} else {
		for (int i = 0; i < 16; i++) {
			int offset = i << 1;

			// fprintf(stderr,"HS: setColor: %03d,%6x - %x,%x,%x\n", index, sdl_colors[index], red, green, blue );

			hostScreen.setPaletteColor( i,
										(STE_COLORS(offset) << 5) | ((STE_COLORS(offset) << 1 ) & 0x8),
										((STE_COLORS(offset + 1) << 1) & 0xE0) | ((STE_COLORS(offset + 1) >> 3) & 0x8),
										((STE_COLORS(offset + 1) << 5) & 0xE0) | ((STE_COLORS(offset + 1) << 1) & 0x8));
		}
		hostScreen.updatePalette( 16 );
	}

	hostColorsSync = true;
}

void VIDEL::renderScreenNoFlag()
{
	int vw	 = getScreenWidth();
	int vh	 = getScreenHeight();
	int vbpp = getScreenBpp();

	if (od_posledni_zmeny > 2) {
		if (vw > 0 && vw != width) {
			D(bug("CH width %d", width));
			width = vw;
			od_posledni_zmeny = 0;
		}
		if (vh > 0 && vh != height) {
			D(bug("CH height %d", width));
			height = vh;
			od_posledni_zmeny = 0;
		}
		if (vbpp != bpp) {
			D(bug("CH bpp %d", vbpp));
			bpp = vbpp;
			od_posledni_zmeny = 0;
		}
	}
	if (od_posledni_zmeny == 3) {
#ifdef SUPPORT_MULTIPLEDESTBPP
		hostScreen.setWindowSize( width, height, bpp >= 8 ? bpp : 8 );
#else
		hostScreen.setWindowSize( width, height, 16 );
#endif // SUPPORT_MULTIPLEDESTBPP
	}
	if (od_posledni_zmeny < 4) {
		od_posledni_zmeny++;
		return;
	}

	if (!hostScreen.renderBegin())
		return;

	long atariVideoRAM = direct_truecolor ? ARANYMVRAMSTART : this->getVideoramAddress();
	uint16 *fvram = (uint16 *) get_real_address_direct(atariVideoRAM);
	VideoRAMBaseHost = (uint8 *) hostScreen.getVideoramAddress();
	uint16 *hvram = (uint16 *) VideoRAMBaseHost;

	if (bpp < 16) {
		updateColors();

		// The SDL colors blitting...
		// FIXME!!! The SDL hvram need not to be line aligned (see the surface->format->pitch!!!)
		int	  planeWordCount = ((vw+15)>>4) * vh;
		uint8 color[16];

#ifndef SUPPORT_MULTIPLEDESTBPP
		for (int32 w = 0; w < planeWordCount; w++) {
			hostScreen.bitplaneToChunky( &fvram[ w*bpp ], bpp, color );

			// note: by enroling this loop into 16 getPaletteColor calls we lost 500 dryhstones ;(
			//		 so we should leave it as is.
			int startBitIndex = w * 16;
			int colIdx = 0;
			for (int j = startBitIndex; j < startBitIndex + 16; j++)
				((uint16 *)hvram)[ j ] = (uint16) hostScreen.getPaletteColor( color[ colIdx++ ] );
		}
#else
		// To support other formats see
		// the SDL video examples (docs 1.1.7)
		//
		// FIXME: The byte swap could be done here by enrolling the loop into 2 each by 8 pixels
		switch ( hostScreen.getBpp() ) {
			case 1:
				for (int32 w = 0; w < planeWordCount; w++) {
					hostScreen.bitplaneToChunky( &fvram[ w*bpp ], bpp, color );

					int startBitIndex = w * 16;
					int colIdx = 0;
					for (int j = startBitIndex; j < startBitIndex + 16; j++)
						((uint8 *)hvram)[ j ] = color[ colIdx++ ];
				}
				break;
			case 2:
				for (int32 w = 0; w < planeWordCount; w++) {
					hostScreen.bitplaneToChunky( &fvram[ w*bpp ], bpp, color );

					// note: by enroling this loop into 16 getPaletteColor calls we lost 500 dryhstones ;(
					//		 so we should leave it as is.
					int startBitIndex = w * 16;
					int colIdx = 0;
					for (int j = startBitIndex; j < startBitIndex + 16; j++)
						((uint16 *)hvram)[ j ] = (uint16) hostScreen.getPaletteColor( color[ colIdx++ ] );
				}
				break;
			case 3:
				for (int32 w = 0; w < planeWordCount; w++) {
					hostScreen.bitplaneToChunky( &fvram[ w*bpp ], bpp, color );

					int startBitIndex = w * 16;
					int colIdx = 0;
					for (int j = startBitIndex; j < startBitIndex + 16; j++) {
						// use the variable due to the putBpp24Pixel macro usage
						uint32 tmpColor = hostScreen.getPaletteColor( color[ colIdx++ ] );
						putBpp24Pixel( (uint32)hvram + j*3, tmpColor );
					}
				}
				break;
			case 4:
				for (int32 w = 0; w < planeWordCount; w++) {
					hostScreen.bitplaneToChunky( &fvram[ w*bpp ], bpp, color );

					int startBitIndex = w * 16;
					int colIdx = 0;
					for (int j = startBitIndex; j < startBitIndex + 16; j++)
						((uint32 *)hvram)[ j ] = (uint32) hostScreen.getPaletteColor( color[ colIdx++ ] );
				}
				break;
		}
#endif // SUPPORT_MULTIPLEDESTBPP

	} else {
		// Falcon TC (High Color)
		int planeWordCount = vw * vh;

#ifdef SUPPORT_MULTIPLEDESTBPP
		uint8 destBPP = hostScreen.getBpp();
		if (destBPP == 2) {
#endif // SUPPORT_MULTIPLEDESTBPP

			// in direct_truecolor mode we set the Videl VIDEORAM directly to the host vram
			if (! direct_truecolor) {
				if ( /* videocard memory in Motorola endian format */ false) {
					memcpy(hvram, fvram, planeWordCount << 1);
				}
				else {
					for (int w = 0; w < planeWordCount; w++) {
						// byteswap
						int data = fvram[w];
						((uint16 *) hvram)[w] = (data >> 8) | ((data & 0xff) << 8);
					}
				}
			}

#ifdef SUPPORT_MULTIPLEDESTBPP

		}
		else if (destBPP == 3) {
			// THIS is the only correct way to do the Falcon TC to SDL conversion!!! (for little endian machines)
			for (int w = 0; w < planeWordCount; w++) {
				// The byteswap is done by correct shifts (not so obvious)
				int data = fvram[w];
				uint32 tmpColor =
				hostScreen.getColor(
									(uint8) (data & 0xf8),
									(uint8) ( ((data & 0x07) << 5) |
											  ((data >> 11) & 0x3c)),
									(uint8) ((data >> 5) & 0xf8));
				putBpp24Pixel( (uint32)hvram + w*3, tmpColor );
			}
		}
		else if (destBPP == 4) {
			// THIS is the only correct way to do the Falcon TC to SDL conversion!!! (for little endian machines)
			for (int w = 0; w < planeWordCount; w++) {
				// The byteswap is done by correct shifts (not so obvious)
				int data = fvram[w];
				((uint32 *) hvram)[w] =
				hostScreen.getColor(
									(uint8) (data & 0xf8),
									(uint8) ( ((data & 0x07) << 5) |
											  ((data >> 11) & 0x3c)),
									(uint8) ((data >> 5) & 0xf8));
			}
		}
		// FIXME: support for destBPP other than 2 or 4 BPP is missing

#endif // SUPPORT_MULTIPLEDESTBPP

	}

	hostScreen.renderEnd();

	hostScreen.update( false );
}


/*
 * $Log$
 * Revision 1.28  2001/10/30 22:59:34  standa
 * The resolution change is now possible through the fVDI driver.
 *
 * Revision 1.27  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.26  2001/10/16 20:28:43  standa
 * The #define SUPPORT_MULTIPLEDESTBPP support extended to 16, 24 and 32 destBpp.
 * Fixed RGB vs BRG bug in videl rendering.
 * SDL_BYTEORDER handling improved in put24BitPixel macro.
 *
 * Revision 1.25  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 * Revision 1.24  2001/10/01 22:22:41  standa
 * bitplaneToChunky conversion moved into HostScreen (inline - should be no performance penalty).
 * fvdidrv/blitArea form memory works in TC.
 *
 * Revision 1.23  2001/09/30 23:07:39  standa
 * Just the loop variables renamed from i to w.
 *
 * Revision 1.22  2001/09/21 14:15:05  joy
 * detect VideoRAM change and turn on Videl rendering again (after a fVDI session during the reset sequence, for example).
 * do not render on 16-bit host screen the Falcon TC mode if the direct_truecolor is set.
 *
 * Revision 1.21  2001/09/19 22:59:44  standa
 * Some debug stuff; st_compatible_mode variable name changed to stCompatibleColorPalette.
 *
 * Revision 1.20  2001/09/08 23:33:47  joy
 * atariVideoRAM is at ARANYMVRAMSTART if direct_truecolor is enabled.
 *
 * Revision 1.19  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *		 The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *		 was created.
 * fVDIDriver got first version of drawLine and fillArea (thanks to SDL_gfxPrimitives).
 *
 * Revision 1.18  2001/08/15 06:48:57  standa
 * ST compatible modes VIDEL patch by Ctirad.
 *
 * Revision 1.17  2001/08/09 12:35:43  standa
 * Forced commit to sync the CVS. ChangeLog should contain all details.
 *
 * Revision 1.16  2001/06/18 20:04:34  standa
 * vdi2fix removed. comments should take the cvs care of ;)
 *
 * Revision 1.15  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
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

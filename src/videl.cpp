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

// Support for other than 2 byte hostscreen bpp is disabled by default
#undef SUPPORT_MULTIPLEDESTBPP


static const int HW = 0xff8200;


VIDEL::VIDEL()
{
	// SelectVideoMode();
	// reasonable default values
	width = 640;
	height = 480;
	doRender = true; // the rendering is on by default (VIDEL does the bitplane to chunky converision)

	od_posledni_zmeny = 0;

	hostColorsSync = false;
}

void VIDEL::init() {
	hostScreen.setWindowSize( width, height );
}

// monitor writting to Falcon and ST/E color palette registers
void VIDEL::handleWrite(uaecptr addr, uint8 value)
{
	BASE_IO::handleWrite(addr, value);

	if ((addr >= 0xff9800 && addr < 0xffa200) || (addr >= 0xff8240 && addr < 0xff8260))
		hostColorsSync = false;
}

long VIDEL::getVideoramAddress()
{
	return (handleRead(HW + 1) << 16) | (handleRead(HW + 3) << 8) |	handleRead(HW + 0x0d);
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
	if (!hostColorsSync) {
		// Test the ST compatible set or not.
		bool st_compatible_mode = false;

		int st_shift = handleReadW(HW + 0x60);
		if (st_shift == 0) {		   // bpp == 4
			int hreg = handleReadW(HW + 0x82); // Too lame!
			if (hreg == 0x17 | hreg == 0x3e)   // Better way how to make out ST LOW mode wanted
				st_compatible_mode = true;
		}
		else if (st_shift == 0x100)	   // bpp == 2
			st_compatible_mode = true;
		else						   // bpp == 1	// if (st_shift == 0x200)
			st_compatible_mode = true;

		// map the colortable into the correct pixel format

#define F_COLORS(i) handleRead(0xff9800 + (i))
#define STE_COLORS(i)	handleRead(0xff8240 + (i))

		if (!st_compatible_mode) {
			for (int i = 0; i < 256; i++) {
				int offset = i << 2;
				hostScreen.setPaletteColor( i,
					F_COLORS(offset),
					F_COLORS(offset + 1),
					F_COLORS(offset + 3) );
			}
		} else {
			for (int i = 0; i < 15; i++) {
				int offset = i << 1;
				hostScreen.setPaletteColor( i,
					(STE_COLORS(offset) << 5) | ((STE_COLORS(offset) << 1 ) & 0x8),
					((STE_COLORS(offset + 1) << 1) & 0xE0) | ((STE_COLORS(offset + 1) >> 3) & 0x8),
					((STE_COLORS(offset + 1) << 5) & 0xE0) | ((STE_COLORS(offset + 1) << 1) & 0x8));
			}
		}
	}

	hostColorsSync = true;
}

void VIDEL::renderScreenNoFlag()
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

	long atariVideoRAM = direct_truecolor ? ARANYMVRAMSTART : this->getVideoramAddress();
	uint16 *fvram = (uint16 *) get_real_address_direct(atariVideoRAM);
	VideoRAMBaseHost = (uint8 *) hostScreen.getVideoramAddress();
	uint16 *hvram = (uint16 *) VideoRAMBaseHost;
	int bpp = getScreenBpp();

#ifdef SUPPORT_MULTIPLEDESTBPP
	uint8 destBPP = hostScreen.getBpp();
#endif // SUPPORT_MULTIPLEDESTBPP

	if (bpp < 16) {
		updateColors();

		// The SDL colors blitting...
		// FIXME: The destBPP tests should probably not
		//		  be inside the loop... (reorganise?)
		int planeWordCount = vw * vh / 16;

#define NEWCHUNKYCONV
#ifdef NEWCHUNKYCONV
		for (int i = 1; i <= planeWordCount; i++) {
			int wordEndIndex = i * bpp - 1;

			// bitplane to chunky conversion.. (the whole word at once)
			uint8 color[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

			for (int l = wordEndIndex; l > wordEndIndex - bpp; l--) {
				uint16 data = fvram[l]; // note: this is about 2000 dryhstones sppedup (the local variable)

				color[ 0] <<= 1;  color[ 0] |= (data >>	 7) & 1;
				color[ 1] <<= 1;  color[ 1] |= (data >>	 6) & 1;
				color[ 2] <<= 1;  color[ 2] |= (data >>	 5) & 1;
				color[ 3] <<= 1;  color[ 3] |= (data >>	 4) & 1;
				color[ 4] <<= 1;  color[ 4] |= (data >>	 3) & 1;
				color[ 5] <<= 1;  color[ 5] |= (data >>	 2) & 1;
				color[ 6] <<= 1;  color[ 6] |= (data >>	 1) & 1;
				color[ 7] <<= 1;  color[ 7] |= (data >>	 0) & 1;

				color[ 8] <<= 1;  color[ 8] |= (data >> 15) & 1;
				color[ 9] <<= 1;  color[ 9] |= (data >> 14) & 1;
				color[10] <<= 1;  color[10] |= (data >> 13) & 1;
				color[11] <<= 1;  color[11] |= (data >> 12) & 1;
				color[12] <<= 1;  color[12] |= (data >> 11) & 1;
				color[13] <<= 1;  color[13] |= (data >> 10) & 1;
				color[14] <<= 1;  color[14] |= (data >>	 9) & 1;
				color[15] <<= 1;  color[15] |= (data >>	 8) & 1;
			}

			// To support other formats see
			// the SDL video examples (docs 1.1.7)
			//
			// FIXME: The byte swap could be done here by enrolling the loop into 2 each by 8 pixels

#ifdef SUPPORT_MULTIPLEDESTBPP
			if (destBPP == 2) {
#endif // SUPPORT_MULTIPLEDESTBPP

				// note: by enroling this loop into 16 getPaletteColor calls we lost 500 dryhstones ;(
				//		 so we should leave it as is.
				int endBitIndex = i * 16;
				int colIdx = 0;
				for (int j = endBitIndex - 16; j < endBitIndex; j++) {
					((uint16 *) hvram)[ j ] = (uint16) hostScreen.getPaletteColor( color[ colIdx++ ] );
				}

#ifdef SUPPORT_MULTIPLEDESTBPP
			}
			else if (destBPP == 4) {
				int endBitIndex = i * 16;
				int colIdx = 0;
				for (int j = endBitIndex - 16; j < endBitIndex; j++)
					((uint32 *) hvram)[ j ] = (uint32) hostScreen.getPaletteColor( color[ colIdx++ ] );
			}
			// FIXME: support for destBPP other than 2 or 4 BPP is missing

#endif // SUPPORT_MULTIPLEDESTBPP

		}  // for( int i; ...

#else // NEWCHUNKYCONV

		// FIXME: remove --- old slower

		uint16 words[bpp];
		for (int i = 0; i < planeWordCount; i++) {

#ifdef SUPPORT_MULTIPLEDESTBPP
			if (destBPP == 2) {
#endif // SUPPORT_MULTIPLEDESTBPP

				// perform byteswap (prior to plane to chunky conversion)
				for (int l = 0; l < bpp; l++) {
					uint16 b = fvram[i * bpp + l];
					words[l] = (b >> 8) | ((b & 0xff) << 8);	// byteswap
				}

				for (int j = 0; j < 16; j++) {
					uint8 color = 0;
					for (int l = bpp - 1; l >= 0; l--) {
						color <<= 1;
						color |= (words[l] >> (15 - j)) & 1;
					}
					((uint16 *) hvram)[(i * 16 + j)] = (uint16) hostScreen.getPaletteColor(color);
				}
#ifdef SUPPORT_MULTIPLEDESTBPP
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
#endif // SUPPORT_MULTIPLEDESTBPP

		}  // for( int i; ...

#endif // NEWCHUNKYCONV

	} else {
		// Falcon TC (High Color)
		int planeWordCount = vw * vh;

#ifdef SUPPORT_MULTIPLEDESTBPP
		if (destBPP == 2) {
#endif // SUPPORT_MULTIPLEDESTBPP

			if ( /* videocard memory in Motorola endian format */ false) {
				memcpy(hvram, fvram, planeWordCount << 1);
			} else {
				for (int i = 0; i < planeWordCount; i++) {
					// byteswap
					int data = fvram[i];
					((uint16 *) hvram)[i] = (data >> 8) | ((data & 0xff) << 8);
				}
			}

#ifdef SUPPORT_MULTIPLEDESTBPP

		}
		else if (destBPP == 4) {
			// THIS is the only correct way to do the Falcon TC to SDL conversion!!! (for little endian machines)
			for (int i = 0; i < planeWordCount; i++) {
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

#endif // SUPPORT_MULTIPLEDESTBPP

	}

	hostScreen.renderEnd();

	hostScreen.update( false );
}


/*
 * $Log$
 * Revision 1.19  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *       The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *       was created.
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

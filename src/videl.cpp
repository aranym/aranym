/*
 * videl.cpp - Falcon VIDEL emulation
 *
 * Copyright (c) 2001-2005 ARAnyM developer team (see AUTHORS)
 *
 * Authors:
 *  joy		Petr Stehlik
 *	standa	Standa Opichal
 *	pmandin	Patrice Mandin
 * 
 * VIDEL HW regs inspired by Linux-m68k kernel (linux/drivers/video/atafb.c)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "host.h"
#include "hostscreen.h"
#include "videl.h"
#include "hardware.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

#define HW	getHWoffset()
#define VIDEL_COLOR_REGS_BEGIN	0xff9800
#define VIDEL_COLOR_REGS_END	0xffa200

VIDEL::VIDEL(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	reset();
}

bool VIDEL::isMyHWRegister(memptr addr)
{
	// FALCON VIDEL COLOR REGISTERS
	if (addr >= VIDEL_COLOR_REGS_BEGIN && addr < VIDEL_COLOR_REGS_END)
		return true;

	return BASE_IO::isMyHWRegister(addr);
}

// Called upon startup and when CPU encounters a RESET instruction.
void VIDEL::reset()
{
	// default resolution to boot with
	width = 640;
	height = 480;

	doRender = true; // the rendering is on by default (VIDEL does the bitplane to chunky conversion)

	since_last_change = 0;

	hostColorsSync = false;

	/* Autozoom */
	zoomwidth=prev_scrwidth=0;
	zoomheight=prev_scrheight=0;
	zoomxtable=NULL;
	zoomytable=NULL;

	hostScreen.setWindowSize( width, height, 8 );
}

// monitor writing to Falcon and ST/E color palette registers
void VIDEL::handleWrite(uint32 addr, uint8 value)
{
	BASE_IO::handleWrite(addr, value);

	if ((addr >= VIDEL_COLOR_REGS_BEGIN && addr < VIDEL_COLOR_REGS_END) ||
		(addr >= 0xff8240 && addr < 0xff8260)) {
		hostColorsSync = false;
		return;
	}

	if ((addr & ~1) == HW+0x60) {
		D(bug("VIDEL st_shift: %06x = %d ($%02x)", addr, value, value));
		// writing to st_shift changed scn_width and vid_mode
		// BASE_IO::handleWrite(HW+0x10, 0x0028);
		// BASE_IO::handleWrite(HW+0xc2, 0x0008);
	}
	else if ((addr & ~1) == HW+0x66) {
		D(bug("VIDEL f_shift: %06x = %d ($%02x)", addr, value, value));
		// IMPORTANT:
		// set st_shift to 0, so we can tell the screen-depth if f_shift==0.
		// Writing 0 to f_shift enables 4 plane Falcon mode but
		// doesn't set st_shift. st_shift!=0 (!=4planes) is impossible
		// with Falcon palette so we set st_shift to 0 manually.
		if (handleReadW(HW+0x66) == 0) {
			BASE_IO::handleWrite(HW+0x60, 0);
		}
	}

	// D(bug("VIDEL write: %06x = %d ($%02x)", addr, value, value));

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
	else /* if (st_shift == 0x200) */
		bits_per_pixel = 1;

	// D(bug("Videl works in %d bpp, f_shift=%04x, st_shift=%d", bits_per_pixel, f_shift, st_shift));

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
	int r,g,b;

#define F_COLORS(i) handleRead(VIDEL_COLOR_REGS_BEGIN + (i))
#define STE_COLORS(i)	handleRead(0xff8240 + (i))

	if (!stCompatibleColorPalette) {
		for (int i = 0; i < 256; i++) {
			int offset = i << 2;
			r = F_COLORS(offset) & 0xfc;
			r |= r>>6;
			g = F_COLORS(offset + 1) & 0xfc;
			g |= g>>6;
			b = F_COLORS(offset + 3) & 0xfc;
			b |= b>>6;
			hostScreen.setPaletteColor(i, r,g,b);
		}
		hostScreen.updatePalette( 256 );
	} else {
		for (int i = 0; i < 16; i++) {
			int offset = i << 1;
			r = STE_COLORS(offset) & 0x0f;
			r = ((r & 7)<<1)|(r>>3);
			r |= r<<4;
			g = (STE_COLORS(offset + 1)>>4) & 0x0f;
			g = ((g & 7)<<1)|(g>>3);
			g |= g<<4;
			b = STE_COLORS(offset + 1) & 0x0f;
			b = ((b & 7)<<1)|(b>>3);
			b |= b<<4;
			hostScreen.setPaletteColor(i, r,g,b);
		}
		hostScreen.updatePalette( 16 );
	}

	hostColorsSync = true;
}

void VIDEL::renderScreen(void)
{
	if ( doRender )
		renderScreenNoFlag();

#ifdef ENABLE_OPENGL
	if (bx_options.opengl.enabled) {
		hostScreen.OpenGLUpdate();

		SDL_GL_SwapBuffers();
	}
#endif	/* ENABLE_OPENGL */
}

void VIDEL::renderScreenNoFlag()
{
	int vw	 = getScreenWidth();
	int vh	 = getScreenHeight();
	int vbpp = getScreenBpp();

	if (since_last_change > 2) {
		if (vw > 0 && vw != width) {
			D(bug("CH width %d", width));
			width = vw;
			since_last_change = 0;
		}
		if (vh > 0 && vh != height) {
			D(bug("CH height %d", width));
			height = vh;
			since_last_change = 0;
		}
		if (vbpp != bpp) {
			D(bug("CH bpp %d", vbpp));
			bpp = vbpp;
			since_last_change = 0;
		}
	}
	if (since_last_change == 3) {
		hostScreen.setWindowSize( width, height, bpp >= 8 ? bpp : 8 );
	}
	if (since_last_change < 4) {
		since_last_change++;
		return;
	}

	if (!hostScreen.renderBegin())
		return;

	if (bx_options.autozoom.enabled) {
		renderScreenZoom();
	} else {
		renderScreenNoZoom();
	}

	hostScreen.renderEnd();

	hostScreen.update( false );
}


void VIDEL::renderScreenNoZoom()
{
	int vw	 = getScreenWidth();
	int vh	 = getScreenHeight();

	int lineoffset = handleReadW(HW + 0x0e) & 0x01ff; // 9 bits
	int linewidth = handleReadW(HW + 0x10) & 0x03ff; // 10 bits
	/* 
	   I think this implementation is naive: 
	   indeed, I suspect that we should instead skip lineoffset
	   words each time we have read "more" than linewidth words
	   (possibly "more" because of the number of bit planes).
	   Moreover, the 1 bit plane mode is particular;
	   while doing some experiments on my Falcon, it seems to
	   behave like the 4 bit planes mode.
	   At last, we have also to take into account the 4 bits register
	   located at the word $ffff8264 (bit offset). This register makes
	   the semantics of the lineoffset register change a little.
	   int bitoffset = handleReadW(HW + 0x64) & 0x000f;
	   The meaning of this register in True Color mode is not clear
	   for me at the moment (and my experiments on the Falcon don't help
	   me).
	*/
	int nextline = linewidth + lineoffset;

	int scrpitch = hostScreen.getPitch();

	long atariVideoRAM = this->getVideoramAddress();

	uint16 *fvram = (uint16 *) Atari2HostAddr(atariVideoRAM);
	VideoRAMBaseHost = (uint8 *) hostScreen.getVideoramAddress();
	uint8 *hvram = VideoRAMBaseHost;

	/* Clip to SDL_Surface dimensions */
	int scrwidth = hostScreen.getWidth();
	int scrheight = hostScreen.getHeight();
	int vw_clip = vw;
	int vh_clip = vh;
	if (vw>scrwidth) vw_clip = scrwidth;
	if (vh>scrheight) vh_clip = scrheight;	

	/* Center screen */
	hvram += ((scrheight-vh_clip)>>1)*scrpitch;
	hvram += ((scrwidth-vw_clip)>>1)*hostScreen.getBpp();

	/* Render */
	if (bpp < 16) {
		/* Bitplanes modes */
		if (!hostColorsSync)
			updateColors();

		// The SDL colors blitting...
		uint8 color[16];

		// FIXME: The byte swap could be done here by enrolling the loop into 2 each by 8 pixels
		switch ( hostScreen.getBpp() ) {
			case 1:
				{
					uint16 *fvram_line = fvram;
					uint8 *hvram_line = hvram;

					for (int h = 0; h < vh_clip; h++) {
						uint16 *fvram_column = fvram_line;
						uint8 *hvram_column = hvram_line;

						for (int w = 0; w < (vw_clip+15)>>4; w++) {
							hostScreen.bitplaneToChunky( fvram_column, bpp, color );

							memcpy(hvram_column, color, 16);

							hvram_column += 16;
							fvram_column += bpp;
						}

						hvram_line += scrpitch;
						fvram_line += nextline;
					}
				}
				break;
			case 2:
				{
					uint16 *fvram_line = fvram;
					uint16 *hvram_line = (uint16 *)hvram;

					for (int h = 0; h < vh_clip; h++) {
						uint16 *fvram_column = fvram_line;
						uint16 *hvram_column = hvram_line;

						for (int w = 0; w < (vw_clip+15)>>4; w++) {
							hostScreen.bitplaneToChunky( fvram_column, bpp, color );

							for (int j=0; j<16; j++) {
								*hvram_column++ = hostScreen.getPaletteColor( color[j] );
							}

							fvram_column += bpp;
						}

						hvram_line += scrpitch>>1;
						fvram_line += nextline;
					}
				}
				break;
			case 3:
				{
					uint16 *fvram_line = fvram;
					uint8 *hvram_line = hvram;

					for (int h = 0; h < vh_clip; h++) {
						uint16 *fvram_column = fvram_line;
						uint8 *hvram_column = hvram_line;

						for (int w = 0; w < (vw_clip+15)>>4; w++) {
							hostScreen.bitplaneToChunky( fvram_column, bpp, color );

							for (int j=0; j<16; j++) {
								uint32 tmpColor = hostScreen.getPaletteColor( color[j] );
								putBpp24Pixel( hvram_column, tmpColor );
								hvram_column += 3;
							}

							fvram_column += bpp;
						}

						hvram_line += scrpitch;
						fvram_line += nextline;
					}
				}
				break;
			case 4:
				{
					uint16 *fvram_line = fvram;
					uint32 *hvram_line = (uint32 *)hvram;

					for (int h = 0; h < vh_clip; h++) {
						uint16 *fvram_column = fvram_line;
						uint32 *hvram_column = hvram_line;

						for (int w = 0; w < (vw_clip+15)>>4; w++) {
							hostScreen.bitplaneToChunky( fvram_column, bpp, color );

							for (int j=0; j<16; j++) {
								*hvram_column++ = hostScreen.getPaletteColor( color[j] );
							}

							fvram_column += bpp;
						}

						hvram_line += scrpitch>>2;
						fvram_line += nextline;
					}
				}
				break;
		}

	} else {

		// Falcon TC (High Color)
		switch ( hostScreen.getBpp() )  {
			case 1:
				{
					/* FIXME: when Videl switches to 16bpp, set the palette to 3:3:2 */
					uint16 *fvram_line = fvram;
					uint8 *hvram_line = hvram;

					for (int h = 0; h < vh_clip; h++) {
						uint16 *fvram_column = fvram_line;
						uint8 *hvram_column = hvram_line;

						for (int w = 0; w < vw_clip; w++) {
							int tmp;
							
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
							tmp = *fvram_column;
#else
							tmp = ((*fvram_column) >> 8) | ((*fvram_column) << 8);
#endif
							*hvram_column = ((tmp>>13) & 7) << 5;
							*hvram_column |= ((tmp>>8) & 7) << 2;
							*hvram_column |= ((tmp>>2) & 3);

							hvram_column++;
							fvram_column++;
						}

						hvram_line += scrpitch;
						fvram_line += nextline;
					}
				}
				break;
			case 2:
				{
					uint16 *fvram_line = fvram;
					uint16 *hvram_line = (uint16 *)hvram;

					for (int h = 0; h < vh_clip; h++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
						//FIXME: here might be a runtime little/big video endian switch like:
						//      if ( /* videocard memory in Motorola endian format */ false)
						memcpy(hvram_line, fvram_line, vw_clip<<1);
#else
						uint16 *fvram_column = fvram_line;
						uint16 *hvram_column = hvram_line;

						for (int w = 0; w < vw_clip; w++) {
							// byteswap
							int data = *fvram_column++;
							*hvram_column++ = (data >> 8) | (data << 8);
						}
#endif // SDL_BYTEORDER == SDL_BIG_ENDIAN

						hvram_line += scrpitch>>1;
						fvram_line += nextline;
					}
				}
				break;
			case 3:
				{
					uint16 *fvram_line = fvram;
					uint8 *hvram_line = hvram;

					for (int h = 0; h < vh_clip; h++) {
						uint16 *fvram_column = fvram_line;
						uint8 *hvram_column = hvram_line;

						for (int w = 0; w < vw_clip; w++) {
							int data = *fvram_column++;

							uint32 tmpColor =
								hostScreen.getColor(
									(uint8) (data & 0xf8),
									(uint8) ( ((data & 0x07) << 5) |
											  ((data >> 11) & 0x3c)),
									(uint8) ((data >> 5) & 0xf8));
							
							putBpp24Pixel( hvram_column, tmpColor );

							hvram_column += 3;
						}

						hvram_line += scrpitch;
						fvram_line += nextline;
					}
				}
				break;
			case 4:
				{
					uint16 *fvram_line = fvram;
					uint32 *hvram_line = (uint32 *)hvram;

					for (int h = 0; h < vh_clip; h++) {
						uint16 *fvram_column = fvram_line;
						uint32 *hvram_column = hvram_line;

						for (int w = 0; w < vw_clip; w++) {
							int data = *fvram_column++;

							*hvram_column++ =
								hostScreen.getColor(
									(uint8) (data & 0xf8),
									(uint8) ( ((data & 0x07) << 5) |
											  ((data >> 11) & 0x3c)),
									(uint8) ((data >> 5) & 0xf8));
						}

						hvram_line += scrpitch>>2;
						fvram_line += nextline;
					}
				}
				break;
		}
	}
}


void VIDEL::renderScreenZoom()
{
	int i, j, w, h, cursrcline;

	/* Atari screen infos */
	int vw	 = getScreenWidth();
	int vh	 = getScreenHeight();
	uint16 *fvram = (uint16 *) Atari2HostAddr(this->getVideoramAddress());
	if ((vw<32) || (vh<32)) return;

	int lineoffset = handleReadW(HW + 0x0e) & 0x01ff; // 9 bits
	int linewidth = handleReadW(HW + 0x10) & 0x03ff; // 10 bits
	/* same remark as before: too naive */
	int nextline = linewidth + lineoffset;

	/* Host screen infos */
	int scrpitch = hostScreen.getPitch();
	int scrwidth = hostScreen.getWidth();
	int scrheight = hostScreen.getHeight();
	int scrbpp = hostScreen.getBpp();
	uint8 *hvram = (uint8 *) hostScreen.getVideoramAddress();

	/* Integer zoom coef ? */
	if ((bx_options.autozoom.integercoefs) && (scrwidth>=vw) && (scrheight>=vh)) {
		int coefx = scrwidth/vw;
		int coefy = scrheight/vh;

		scrwidth = vw * coefx;
		scrheight = vh * coefy;

		/* Center screen */
		hvram += ((hostScreen.getHeight()-scrheight)>>1)*scrpitch;
		hvram += ((hostScreen.getWidth()-scrwidth)>>1)*scrbpp;
	}

	/* New zoom ? */
	if ((zoomwidth != vw) || (scrwidth != prev_scrwidth)) {
		if (zoomxtable) {
			delete zoomxtable;
		}
		zoomxtable = new int[scrwidth];
		for (i=0; i<scrwidth; i++) {
			zoomxtable[i] = (vw*i)/scrwidth;
		}
		zoomwidth = vw;
		prev_scrwidth = scrwidth;
	}
	if ((zoomheight != vh) || (scrheight != prev_scrheight)) {
		if (zoomytable) {
			delete zoomytable;
		}
		zoomytable = new int[scrheight];
		for (i=0; i<scrheight; i++) {
			zoomytable[i] = (vh*i)/scrheight;
		}
		zoomheight = vh;
		prev_scrheight = scrheight;
	}

	cursrcline = -1;

	if (bpp<16) {
		if (!hostColorsSync)
			updateColors();

		uint8 color[16];

		/* Bitplanes modes */
		switch(scrbpp) {
			case 1:
				{
					/* One complete planar 2 chunky line */
					uint8 *p2cline = new uint8[vw];

					uint16 *fvram_line;
					uint8 *hvram_line = hvram;

					for (h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-scrpitch, scrwidth*scrbpp);
						} else {
							uint16 *fvram_column = fvram_line;
							uint8 *hvram_column = p2cline;

							/* Convert a new line */
							for (w=0; w < (vw+15)>>4; w++) {
								hostScreen.bitplaneToChunky( fvram_column, bpp, color );

								memcpy(hvram_column, color, 16);

								hvram_column += 16;
								fvram_column += bpp;
							}
							
							/* Zoom a new line */
							for (w=0; w<scrwidth; w++) {
								hvram_line[w] = p2cline[zoomxtable[w]];
							}
						}

						hvram_line += scrpitch;
						cursrcline = zoomytable[h];
					}

					delete p2cline;
				}
				break;
			case 2:
				{
					/* One complete planar 2 chunky line */
					uint16 *p2cline = new uint16[vw];

					uint16 *fvram_line = fvram;
					uint16 *hvram_line = (uint16 *)hvram;

					for (h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-(scrpitch>>1), scrwidth*scrbpp);
						} else {
							uint16 *fvram_column = fvram_line;
							uint16 *hvram_column = p2cline;

							/* Convert a new line */
							for (w=0; w < (vw+15)>>4; w++) {
								hostScreen.bitplaneToChunky( fvram_column, bpp, color );

								for (j=0; j<16; j++) {
									*hvram_column++ = hostScreen.getPaletteColor( color[j] );
								}

								fvram_column += bpp;
							}
							
							/* Zoom a new line */
							for (w=0; w<scrwidth; w++) {
								hvram_line[w] = p2cline[zoomxtable[w]];
							}
						}

						hvram_line += scrpitch>>1;
						cursrcline = zoomytable[h];
					}

					delete p2cline;
				}
				break;
			case 3:
				{
					/* One complete planar 2 chunky line */
					uint8 *p2cline = new uint8[vw*3];

					uint16 *fvram_line;
					uint8 *hvram_line = hvram;

					for (h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-scrpitch, scrwidth*scrbpp);
						} else {
							uint16 *fvram_column = fvram_line;
							uint8 *hvram_column = p2cline;

							/* Convert a new line */
							for (w=0; w < (vw+15)>>4; w++) {
								hostScreen.bitplaneToChunky( fvram_column, bpp, color );

								for (int j=0; j<16; j++) {
									uint32 tmpColor = hostScreen.getPaletteColor( color[j] );
									putBpp24Pixel( hvram_column, tmpColor );
									hvram_column += 3;
								}

								fvram_column += bpp;
							}
							
							/* Zoom a new line */
							for (w=0; w<scrwidth; w++) {
								hvram_line[w*3] = p2cline[zoomxtable[w]*3];
								hvram_line[w*3+1] = p2cline[zoomxtable[w]*3+1];
								hvram_line[w*3+2] = p2cline[zoomxtable[w]*3+2];
							}
						}

						hvram_line += scrpitch;
						cursrcline = zoomytable[h];
					}

					delete p2cline;
				}
				break;
			case 4:
				{
					/* One complete planar 2 chunky line */
					uint32 *p2cline = new uint32[vw];

					uint16 *fvram_line;
					uint32 *hvram_line = (uint32 *)hvram;

					for (h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-(scrpitch>>2), scrwidth*scrbpp);
						} else {
							uint16 *fvram_column = fvram_line;
							uint32 *hvram_column = p2cline;

							/* Convert a new line */
							for (w=0; w < (vw+15)>>4; w++) {
								hostScreen.bitplaneToChunky( fvram_column, bpp, color );

								for (j=0; j<16; j++) {
									*hvram_column++ = hostScreen.getPaletteColor( color[j] );
								}

								fvram_column += bpp;
							}
							
							/* Zoom a new line */
							for (w=0; w<scrwidth; w++) {
								hvram_line[w] = p2cline[zoomxtable[w]];
							}
						}

						hvram_line += scrpitch>>2;
						cursrcline = zoomytable[h];
					}

					delete p2cline;
				}
				break;
		}
	} else {
		/* Falcon TrueColour mode */

		switch(scrbpp) {
			case 1:
				{
					/* FIXME: when Videl switches to 16bpp, set the palette to 3:3:2 */
					uint16 *fvram_line;
					uint8 *hvram_line = hvram;

					for (int h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						uint16 *fvram_column = fvram_line;
						uint8 *hvram_column = hvram_line;

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-scrpitch, scrwidth*scrbpp);
						} else {
							for (int w = 0; w < scrwidth; w++) {
								uint16 srcword;
								uint8 dstbyte;
							
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
								srcword = fvram_column[zoomxtable[w]];
#else
								srcword = (fvram_column[zoomxtable[w]] >> 8) | (fvram_column[zoomxtable[w]] << 8);
#endif
								dstbyte = ((srcword>>13) & 7) << 5;
								dstbyte |= ((srcword>>8) & 7) << 2;
								dstbyte |= ((srcword>>2) & 3);

								*hvram_column++ = dstbyte;
							}
						}

						hvram_line += scrpitch;
						cursrcline = zoomytable[h];
					}
				}
				break;
			case 2:
				{
					uint16 *fvram_line;
					uint16 *hvram_line = (uint16 *)hvram;

					for (int h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						uint16 *fvram_column = fvram_line;
						uint16 *hvram_column = hvram_line;

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-(scrpitch>>1), scrwidth*scrbpp);
						} else {
							for (int w = 0; w < scrwidth; w++) {
								uint16 srcword;
							
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
								srcword = fvram_column[zoomxtable[w]];
#else
								srcword = (fvram_column[zoomxtable[w]] >> 8) | (fvram_column[zoomxtable[w]] << 8);
#endif
								*hvram_column++ = srcword;
							}
						}

						hvram_line += scrpitch>>1;
						cursrcline = zoomytable[h];
					}
				}
				break;
			case 3:
				{
					uint16 *fvram_line;
					uint8 *hvram_line = hvram;

					for (int h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						uint16 *fvram_column = fvram_line;
						uint8 *hvram_column = hvram_line;

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-scrpitch, scrwidth*scrbpp);
						} else {
							for (int w = 0; w < scrwidth; w++) {
								uint16 srcword;
								uint32 dstlong;
							
								srcword = fvram_column[zoomxtable[w]];

								dstlong = hostScreen.getColor(
										(uint8) (srcword & 0xf8),
										(uint8) ( ((srcword & 0x07) << 5) |
											  ((srcword >> 11) & 0x3c)),
										(uint8) ((srcword >> 5) & 0xf8));

								putBpp24Pixel( hvram_column, dstlong );
								hvram_column += 3;
							}
						}

						hvram_line += scrpitch;
						cursrcline = zoomytable[h];
					}
				}
				break;
			case 4:
				{
					uint16 *fvram_line;
					uint32 *hvram_line = (uint32 *)hvram;

					for (int h = 0; h < scrheight; h++) {
						fvram_line = fvram + (zoomytable[h] * nextline);

						uint16 *fvram_column = fvram_line;
						uint32 *hvram_column = hvram_line;

						/* Recopy the same line ? */
						if (zoomytable[h] == cursrcline) {
							memcpy(hvram_line, hvram_line-(scrpitch>>2), scrwidth*scrbpp);
						} else {
							for (int w = 0; w < scrwidth; w++) {
								uint16 srcword;
							
								srcword = fvram_column[zoomxtable[w]];

								*hvram_column++ =
									hostScreen.getColor(
										(uint8) (srcword & 0xf8),
										(uint8) ( ((srcword & 0x07) << 5) |
											  ((srcword >> 11) & 0x3c)),
										(uint8) ((srcword >> 5) & 0xf8));
							}
						}

						hvram_line += scrpitch>>2;
						cursrcline = zoomytable[h];
					}
				}
				break;
		}
	}
}

/*
vim:ts=4:sw=4:
*/

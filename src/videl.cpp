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

// from host.cpp
extern HostScreen hostScreen;

static const uint32 HW = 0xff8200UL;


VIDEL::VIDEL()
{
	// default resolution to boot with
	width = 640;
	height = 480;

	doRender = true; // the rendering is on by default (VIDEL does the bitplane to chunky conversion)

	since_last_change = 0;

	hostColorsSync = false;

	/* Autozoom */
	zoomwidth=0;
	zoomheight=0;
	zoomxtable=NULL;
	zoomytable=NULL;
}

void VIDEL::init()
{
	// preset certain VIDEL registers so that it displays something even
	// if the VIDEL was not initialized fully and correctly (current EmuTOS)
	// the following values were read from TOS VGA2 resolution (640x480x1).
	// for bpp detection (default 1)
	handleWriteW(HW+0x60, 0x0000);
	handleWriteW(HW+0x66, 0x0400);
	handleWriteW(HW+0x82, 0x00c6);
	// for width (default 640)
	handleWriteW(HW+0x10, 0x0028);
	// lineoffset is zero
	handleWriteW(HW+0x0e, 0x0000);
	// linewidth in words
	handleWriteW(HW+0x10, 0x0028);
	// for height
	handleWriteW(HW+0xa8, 0x003f);
	handleWriteW(HW+0xaa, 0x03ff);
	handleWriteW(HW+0xc2, 0x0008);

	hostScreen.setWindowSize( width, height, 16 );
}

// monitor writting to Falcon and ST/E color palette registers
void VIDEL::handleWrite(uint32 addr, uint8 value)
{
	BASE_IO::handleWrite(addr, value);

	if ((addr >= 0xff9800 && addr < 0xffa200) || (addr >= 0xff8240 && addr < 0xff8260))
		hostColorsSync = false;
	else
		D(bug("VIDEL write: %06x = %d ($%02x)", addr, value, value));

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

	if (bx_options.video.autozoom) {
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

	int lineoffset = handleReadW(HW + 0x0e);
	int linewidth = handleReadW(HW + 0x10);
	int nextline = linewidth + lineoffset;

	int scrpitch = hostScreen.getPitch();

	long atariVideoRAM = this->getVideoramAddress();
#ifdef DIRECT_TRUECOLOR
	if (bx_options.video.direct_truecolor)
		atariVideoRAM = ARANYMVRAMSTART;
#endif
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
					// in direct_truecolor mode we set the Videl VIDEORAM directly to the host vram
#ifdef DIRECT_TRUECOLOR
					if (! bx_options.video.direct_truecolor)
#endif
					{
						uint16 *fvram_line = fvram;
						uint16 *hvram_line = (uint16 *)hvram;

						for (int h = 0; h < vh_clip; h++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
							//FIXME: here might be a runtime little/big video endian switch like:
							//      if ( /* videocard memory in Motorola endian format */ false) {
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

	int lineoffset = handleReadW(HW + 0x0e);
	int linewidth = handleReadW(HW + 0x10);
	int nextline = linewidth + lineoffset;

	/* Host screen infos */
	int scrpitch = hostScreen.getPitch();
	int scrwidth = hostScreen.getWidth();
	int scrheight = hostScreen.getHeight();
	int scrbpp = hostScreen.getBpp();
	uint8 *hvram = (uint8 *) hostScreen.getVideoramAddress();

	/* Integer zoom coef ? */
	if ((bx_options.video.autozoomint) && (scrwidth>vw) && (scrheight>vh)) {
		int coefx = scrwidth/vw;
		int coefy = scrheight/vh;

		scrwidth = vw * coefx;
		scrheight = vh * coefy;

		/* Center screen */
		hvram += ((hostScreen.getHeight()-scrheight)>>1)*scrpitch;
		hvram += ((hostScreen.getWidth()-scrwidth)>>1)*scrbpp;
	}

	/* New zoom ? */
	if (zoomwidth != vw) {
		if (zoomxtable) {
			delete zoomxtable;
		}
		zoomxtable = new int[scrwidth];
		for (i=0; i<scrwidth; i++) {
			zoomxtable[i] = (vw*i)/scrwidth;
		}
		zoomwidth = vw;
	}
	if (zoomheight != vh) {
		if (zoomytable) {
			delete zoomytable;
		}
		zoomytable = new int[scrheight];
		for (i=0; i<scrheight; i++) {
			zoomytable[i] = (vh*i)/scrheight;
		}
		zoomheight = vh;
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
						fvram_line = fvram + (zoomytable[h] * vw);

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
						fvram_line = fvram + (zoomytable[h] * vw);

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
						fvram_line = fvram + (zoomytable[h] * vw);

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
						fvram_line = fvram + (zoomytable[h] * vw);

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
 * $Log$
 * Revision 1.44  2002/12/01 10:26:12  pmandin
 * Autozoom bugfix
 *
 * Revision 1.43  2002/09/24 18:59:50  pmandin
 * Autozoom done
 *
 * Revision 1.42  2002/09/24 16:27:07  pmandin
 * Small bugfix
 *
 * Revision 1.41  2002/09/24 16:08:24  pmandin
 * Bugfixes+preliminary autozoom support
 *
 * Revision 1.39  2002/09/23 09:23:15  pmandin
 * Render to/from any bpp, using screen pitch
 *
 * Revision 1.38  2002/06/24 17:08:48  standa
 * The pointer arithmetics fixed. The memptr usage introduced in my code.
 *
 * Revision 1.37  2002/02/28 20:43:33  joy
 * uae_ vars replaced with uint's
 *
 * Revision 1.36  2002/01/17 14:59:19  milan
 * cleaning in HW <-> memory communication
 * support for JIT CPU
 *
 * Revision 1.35  2001/12/17 08:33:00  standa
 * Thread synchronization added. The check_event and fvdidriver actions are
 * synchronized each to other.
 *
 * Revision 1.34  2001/12/14 12:15:18  joy
 * double line didn't work due to my stupid idea. Fixed.
 * Debugginng of write access to VIDEL registers added.
 *
 * Revision 1.33  2001/12/07 15:46:47  joy
 * VIDEL registers are preinitialized partially to allow VIDEL unaware EmuTOS to display something.
 *
 * Revision 1.32  2001/12/03 20:56:07  standa
 * The gfsprimitives library files removed. All the staff was moved and
 * adjusted directly into the HostScreen class.
 *
 * Revision 1.31  2001/11/18 21:23:20  standa
 * The little/big endian compiletime check. No runtime for big endian graphic
 * cards on little enddian machines, but I think there is no such case.
 *
 * Revision 1.30  2001/11/11 22:03:09  joy
 * direct truecolor is optional (compile time configurable)
 *
 * Revision 1.29  2001/11/04 23:17:08  standa
 * 8bit destination surface support in VIDEL. Blit routine optimalization.
 * Bugfix in compatibility modes palette copying.
 *
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

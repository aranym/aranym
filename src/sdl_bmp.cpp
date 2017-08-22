/*
	BMP loading

	(C) 2007 ARAnyM developer team

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

#include "sysdeps.h"

#include "SDL_compat.h"

#include "dirty_rects.h"
#include "host_surface.h"
#include "logo.h"
#include "hostscreen.h"
#include "host.h"

#define DEBUG 0
#include "debug.h"


/* Compression encodings for BMP files */
#ifndef BI_RGB
#define BI_RGB		0
#define BI_RLE8		1
#define BI_RLE4		2
#define BI_BITFIELDS	3
#endif


static void CorrectAlphaChannel(SDL_Surface * surface)
{
	/* Check to see if there is any alpha channel data */
	SDL_bool hasAlpha = SDL_FALSE;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int alphaChannelOffset = 0;
#else
	int alphaChannelOffset = 3;
#endif
	Uint8 *alpha = ((Uint8 *) surface->pixels) + alphaChannelOffset;
	Uint8 *end = alpha + surface->h * surface->pitch;

	while (alpha < end)
	{
		if (*alpha != 0)
		{
			hasAlpha = SDL_TRUE;
			break;
		}
		alpha += 4;
	}

	if (!hasAlpha)
	{
		alpha = ((Uint8 *) surface->pixels) + alphaChannelOffset;
		while (alpha < end)
		{
			*alpha = SDL_ALPHA_OPAQUE;
			alpha += 4;
		}
	}
}

SDL_Surface *mySDL_LoadBMP_RW(SDL_RWops * src, int freesrc)
{
	SDL_bool was_error;
	Sint64 fp_offset = 0;
	int bmpPitch;
	int i, pad;
	SDL_Surface *surface;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	SDL_Palette *palette;
	Uint8 *bits;
	Uint8 *top,	*end;
	SDL_bool topDown;
	int ExpandBMP;
	SDL_bool correctAlpha = SDL_FALSE;

	/* The Win32 BMP file header (14 bytes) */
	char magic[2];
	/* Uint32 bfSize = 0; */
	/* Uint16 bfReserved1 = 0; */
	/* Uint16 bfReserved2 = 0; */
	Uint32 bfOffBits = 0;

	/* The Win32 BITMAPINFOHEADER struct (40 bytes) */
	Uint32 biSize = 0;
	Sint32 biWidth = 0;
	Sint32 biHeight = 0;
	/* Uint16 biPlanes = 0; */
	Uint16 biBitCount = 0;
	Uint32 biCompression = 0;
	/* Uint32 biSizeImage = 0; */
	/* Sint32 biXPelsPerMeter = 0; */
	/* Sint32 biYPelsPerMeter = 0; */
	Uint32 biClrUsed = 0;
	/* Uint32 biClrImportant = 0; */

	/* Make sure we are passed a valid data source */
	surface = NULL;
	was_error = SDL_FALSE;
	if (src == NULL)
	{
		was_error = SDL_TRUE;
		goto done;
	}

	/* Read in the BMP file header */
	fp_offset = SDL_RWtell(src);
	SDL_ClearError();
	if (SDL_RWread(src, magic, 1, 2) != 2)
	{
		SDL_Error(SDL_EFREAD);
		was_error = SDL_TRUE;
		goto done;
	}
	if (SDL_strncmp(magic, "BM", 2) != 0)
	{
		SDL_SetError("File is not a Windows BMP file");
		was_error = SDL_TRUE;
		goto done;
	}
	/* bfSize = */
	SDL_ReadLE32(src);
	/* bfReserved1 = */ SDL_ReadLE16(src);
	/* bfReserved2 = */ SDL_ReadLE16(src);
	bfOffBits = SDL_ReadLE32(src);

	/* Read the Win32 BITMAPINFOHEADER */
	biSize = SDL_ReadLE32(src);
	if (biSize == 12)
	{
		biWidth = (Uint32) SDL_ReadLE16(src);
		biHeight = (Uint32) SDL_ReadLE16(src);
		/* biPlanes = */ SDL_ReadLE16(src);
		biBitCount = SDL_ReadLE16(src);
		biCompression = BI_RGB;
	} else
	{
		const unsigned int headerSize = 40;

		biWidth = SDL_ReadLE32(src);
		biHeight = SDL_ReadLE32(src);
		/* biPlanes = */ SDL_ReadLE16(src);
		biBitCount = SDL_ReadLE16(src);
		biCompression = SDL_ReadLE32(src);
		/* biSizeImage = */ SDL_ReadLE32(src);
		/* biXPelsPerMeter = */ SDL_ReadLE32(src);
		/* biYPelsPerMeter = */ SDL_ReadLE32(src);
		biClrUsed = SDL_ReadLE32(src);
		/* biClrImportant = */ SDL_ReadLE32(src);

		if (biSize > headerSize)
		{
			SDL_RWseek(src, (biSize - headerSize), RW_SEEK_CUR);
		}
	}
	if (biHeight < 0)
	{
		topDown = SDL_TRUE;
		biHeight = -biHeight;
	} else
	{
		topDown = SDL_FALSE;
	}

	/* Check for read error */
	if (SDL_strcmp(SDL_GetError(), "") != 0)
	{
		was_error = SDL_TRUE;
		goto done;
	}

	/* Expand 1 and 4 bit bitmaps to 8 bits per pixel */
	switch (biBitCount)
	{
	case 1:
	case 4:
		ExpandBMP = biBitCount;
		biBitCount = 8;
		break;
	default:
		ExpandBMP = 0;
		break;
	}

	/* We don't support any BMP compression right now */
	Rmask = Gmask = Bmask = Amask = 0;
	switch (biCompression)
	{
	case BI_RGB:
		/* If there are no masks, use the defaults */
		if (bfOffBits == (14 + biSize))
		{
			/* Default values for the BMP format */
			switch (biBitCount)
			{
			case 15:
			case 16:
				Rmask = 0x7C00;
				Gmask = 0x03E0;
				Bmask = 0x001F;
				break;
			case 24:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				Rmask = 0x000000FF;
				Gmask = 0x0000FF00;
				Bmask = 0x00FF0000;
#else
				Rmask = 0x00FF0000;
				Gmask = 0x0000FF00;
				Bmask = 0x000000FF;
#endif
				break;
			case 32:
				/* We don't know if this has alpha channel or not */
				correctAlpha = SDL_TRUE;
				Amask = 0xFF000000;
				Rmask = 0x00FF0000;
				Gmask = 0x0000FF00;
				Bmask = 0x000000FF;
				break;
			default:
				break;
			}
			break;
		}
		/* Fall through -- read the RGB masks */

	case BI_BITFIELDS:
		switch (biBitCount)
		{
		case 15:
		case 16:
			Rmask = SDL_ReadLE32(src);
			Gmask = SDL_ReadLE32(src);
			Bmask = SDL_ReadLE32(src);
			break;
		case 32:
			Rmask = SDL_ReadLE32(src);
			Gmask = SDL_ReadLE32(src);
			Bmask = SDL_ReadLE32(src);
			Amask = SDL_ReadLE32(src);
			break;
		default:
			break;
		}
		break;
	default:
		SDL_SetError("Compressed BMP files not supported");
		was_error = SDL_TRUE;
		goto done;
	}

	/* Create a compatible surface, note that the colors are RGB ordered */
	surface = SDL_CreateRGBSurface(0, biWidth, biHeight, biBitCount, Rmask, Gmask, Bmask, Amask);
	if (surface == NULL)
	{
		was_error = SDL_TRUE;
		goto done;
	}

	/* Load the palette, if any */
	palette = (surface->format)->palette;
	if (palette)
	{
		if (biClrUsed == 0)
		{
			biClrUsed = 1 << biBitCount;
		}
		if ((int) biClrUsed > palette->ncolors)
		{
			palette->ncolors = biClrUsed;
			palette->colors = (SDL_Color *) SDL_realloc(palette->colors, palette->ncolors * sizeof(*palette->colors));
			if (!palette->colors)
			{
				SDL_OutOfMemory();
				was_error = SDL_TRUE;
				goto done;
			}
		} else if ((int) biClrUsed < palette->ncolors)
		{
			palette->ncolors = biClrUsed;
		}
		if (biSize == 12)
		{
			for (i = 0; i < (int) biClrUsed; ++i)
			{
				SDL_RWread(src, &palette->colors[i].b, 1, 1);
				SDL_RWread(src, &palette->colors[i].g, 1, 1);
				SDL_RWread(src, &palette->colors[i].r, 1, 1);
#if SDL_VERSION_ATLEAST(2, 0, 0)
				palette->colors[i].a = SDL_ALPHA_OPAQUE;
#else
				palette->colors[i].unused = SDL_ALPHA_OPAQUE;
#endif
			}
		} else
		{
			for (i = 0; i < (int) biClrUsed; ++i)
			{
				SDL_RWread(src, &palette->colors[i].b, 1, 1);
				SDL_RWread(src, &palette->colors[i].g, 1, 1);
				SDL_RWread(src, &palette->colors[i].r, 1, 1);
#if SDL_VERSION_ATLEAST(2, 0, 0)
				SDL_RWread(src, &palette->colors[i].a, 1, 1);
#else
				SDL_RWread(src, &palette->colors[i].unused, 1, 1);
#endif

				/* According to Microsoft documentation, the fourth element
				   is reserved and must be zero, so we shouldn't treat it as
				   alpha.
				 */
#if SDL_VERSION_ATLEAST(2, 0, 0)
				palette->colors[i].a = SDL_ALPHA_OPAQUE;
#else
				palette->colors[i].unused = SDL_ALPHA_OPAQUE;
#endif
			}
		}
	}

	/* Read the surface pixels.  Note that the bmp image is upside down */
	if (SDL_RWseek(src, fp_offset + bfOffBits, RW_SEEK_SET) < 0)
	{
		SDL_Error(SDL_EFSEEK);
		was_error = SDL_TRUE;
		goto done;
	}
	top = (Uint8 *) surface->pixels;
	end = (Uint8 *) surface->pixels + (surface->h * surface->pitch);
	switch (ExpandBMP)
	{
	case 1:
		bmpPitch = (biWidth + 7) >> 3;
		pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
		break;
	case 4:
		bmpPitch = (biWidth + 1) >> 1;
		pad = (((bmpPitch) % 4) ? (4 - ((bmpPitch) % 4)) : 0);
		break;
	default:
		pad = ((surface->pitch % 4) ? (4 - (surface->pitch % 4)) : 0);
		break;
	}
	if (topDown)
	{
		bits = top;
	} else
	{
		bits = end - surface->pitch;
	}
	while (bits >= top && bits < end)
	{
		switch (ExpandBMP)
		{
		case 1:
		case 4:
			{
				Uint8 pixel = 0;
				int shift = (8 - ExpandBMP);

				for (i = 0; i < surface->w; ++i)
				{
					if (i % (8 / ExpandBMP) == 0)
					{
						if (!SDL_RWread(src, &pixel, 1, 1))
						{
							SDL_SetError("Error reading from BMP");
							was_error = SDL_TRUE;
							goto done;
						}
					}
					*(bits + i) = (pixel >> shift);
					pixel <<= ExpandBMP;
				}
			}
			break;

		default:
			if ((int)SDL_RWread(src, bits, 1, surface->pitch) != (int)surface->pitch)
			{
				SDL_Error(SDL_EFREAD);
				was_error = SDL_TRUE;
				goto done;
			}
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			/* Byte-swap the pixels if needed. Note that the 24bpp
			   case has already been taken care of above. */
			switch (biBitCount)
			{
			case 15:
			case 16:
				{
					Uint16 *pix = (Uint16 *) bits;

					for (i = 0; i < surface->w; i++)
						pix[i] = SDL_Swap16(pix[i]);
				}
				break;

			case 32:
				{
					Uint32 *pix = (Uint32 *) bits;

					for (i = 0; i < surface->w; i++)
						pix[i] = SDL_Swap32(pix[i]);
				}
				break;
			}
#endif
			break;
		}
		/* Skip padding bytes, ugh */
		if (pad)
		{
			Uint8 padbyte;

			for (i = 0; i < pad; ++i)
			{
				SDL_RWread(src, &padbyte, 1, 1);
			}
		}
		if (topDown)
		{
			bits += surface->pitch;
		} else
		{
			bits -= surface->pitch;
		}
	}
	if (correctAlpha)
	{
		CorrectAlphaChannel(surface);
	}
  done:
	if (was_error)
	{
		if (src)
		{
			SDL_RWseek(src, fp_offset, RW_SEEK_SET);
		}
		SDL_FreeSurface(surface);
		surface = NULL;
	}
	if (freesrc && src)
	{
		SDL_RWclose(src);
	}
	return surface;
}

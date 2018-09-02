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

#include "sysdeps.h"

#ifdef NFJPEG_SUPPORT
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfjpeg.h"
#include "../../atari/nfjpeg/nfjpeg_nfapi.h"
#define ARANYM_NFJPEG
#include "../../atari/nfjpeg/jpgdh.h"
#undef ARANYM_NFJPEG

#include "SDL_compat.h"
#include <SDL_rwops.h>
#if defined(HAVE_JPEGLIB)
#elif defined(HAVE_SDL_IMAGE)
#include <SDL_image.h>
#else
 #error "no jpeg library found"
#endif
#include "toserror.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- jpeglib functions ---*/

#if defined(HAVE_JPEGLIB)

#define INPUT_BUFFER_SIZE 4096
typedef struct {
	struct jpeg_source_mgr pub;

	SDL_RWops *ctx;
	Uint8 buffer[INPUT_BUFFER_SIZE];
} my_source_mgr;

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */
static void init_source(j_decompress_ptr cinfo)
{
	/* We don't actually need to do anything */
	(void) cinfo;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 */
static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
	my_source_mgr *src = (my_source_mgr *) cinfo->src;
	int nbytes;

	nbytes = SDL_RWread(src->ctx, src->buffer, 1, INPUT_BUFFER_SIZE);
	if (nbytes <= 0)
	{
		/* Insert a fake EOI marker */
		src->buffer[0] = 0xFF;
		src->buffer[1] = JPEG_EOI;
		nbytes = 2;
	}
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;

	return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return. If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */
static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	my_source_mgr *src = (my_source_mgr *) cinfo->src;

	/* Just a dumb implementation for now.	Could use fseek() except
	 * it doesn't work on pipes. Not clear that being smart is worth
	 * any trouble anyway --- large skips are infrequent.
	 */
	if (num_bytes > 0)
	{
		while (num_bytes > (long) src->pub.bytes_in_buffer)
		{
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) src->pub.fill_input_buffer(cinfo);
			/* note we assume that fill_input_buffer will never
			 * return FALSE, so suspension need not be handled.
			 */
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.
 */
static void term_source(j_decompress_ptr cinfo)
{
	/* We don't actually need to do anything */
	(void) cinfo;
}

void JpegDriver::my_error_exit(j_common_ptr cinfo)
{
	struct my_error_mgr *err = (struct my_error_mgr *)cinfo->err;
	longjmp(err->escape, 1);
}

static void output_no_message(j_common_ptr cinfo)
{
	(void) cinfo;
}

static void jpeg_rw_src(j_decompress_ptr cinfo, SDL_RWops *ctx)
{
	my_source_mgr *src;

	if (cinfo->src == NULL) /* first time for this JPEG object? */
	{
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_source_mgr));
	}

	src = (my_source_mgr *) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->ctx = ctx;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

SDL_Surface *JpegDriver::load_jpeg(SDL_RWops *src)
{
	Sint64 start;
	JSAMPROW rowptr[1];
	SDL_Surface *volatile surface = NULL;
	j_decompress_ptr cinfo;
	
	memset(&jpeg, 0, sizeof(jpeg));
	cinfo = &jpeg;
	cinfo->err = jpeg_std_error(&jerr.errmgr);
	cinfo->err->error_exit = my_error_exit;
	cinfo->err->output_message = output_no_message;
	
	if (src == NULL)
	{
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}
	start = SDL_RWtell(src);

	if (setjmp(jerr.escape))
	{
		/* If we get here, libjpeg found an error */
		jpeg_destroy_decompress(&jpeg);
		if (surface != NULL)
		{
			SDL_FreeSurface(surface);
		}
		SDL_RWseek(src, start, RW_SEEK_SET);
		return NULL;
	}

	jpeg_create_decompress(cinfo);
	jpeg_rw_src(cinfo, src);
	jpeg_read_header(cinfo, TRUE);

	if (cinfo->num_components == 4)
	{
		/* Set 32-bit Raw output */
		cinfo->out_color_space = JCS_CMYK;
		cinfo->quantize_colors = FALSE;
		jpeg_calc_output_dimensions(cinfo);

		/* Allocate an output surface to hold the image */
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
				cinfo->output_width, cinfo->output_height, 32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
#else
				0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF
#endif
				);
	} else
	{
		/* Set 24-bit RGB output */
		cinfo->out_color_space = JCS_RGB;
		cinfo->quantize_colors = FALSE;
#ifdef FAST_JPEG
		cinfo->scale_num   = 1;
		cinfo->scale_denom = 1;
		cinfo->dct_method = JDCT_FASTEST;
		cinfo->do_fancy_upsampling = FALSE;
#endif
		jpeg_calc_output_dimensions(cinfo);

		/* Allocate an output surface to hold the image */
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
				cinfo->output_width, cinfo->output_height, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				0x0000FF, 0x00FF00, 0xFF0000,
#else
				0xFF0000, 0x00FF00, 0x0000FF,
#endif
				0);
	}

	if (surface == NULL)
	{
		jpeg_destroy_decompress(cinfo);
		SDL_RWseek(src, start, RW_SEEK_SET);
		return NULL;
	}

	/* Decompress the image */
	jpeg_start_decompress(cinfo);
	while (cinfo->output_scanline < cinfo->output_height)
	{
		rowptr[0] = (JSAMPROW)(Uint8 *)surface->pixels + cinfo->output_scanline * surface->pitch;
		jpeg_read_scanlines(cinfo, rowptr, (JDIMENSION) 1);
	}
	jpeg_finish_decompress(cinfo);
	jpeg_destroy_decompress(cinfo);

	return surface;
}

#endif

/*--- Constructor/destructor functions ---*/

JpegDriver::JpegDriver()
{
	memset(images, 0, sizeof(images));
	D(bug("nfjpeg: created"));
}

JpegDriver::~JpegDriver()
{
	int i;

	for (i = 1; i <= MAX_NFJPEG_IMAGES; i++)
	{
		if (images[i].src)
		{
			SDL_FreeSurface(images[i].src);
			images[i].src = NULL;
		}
		images[i].used = false;
	}
	D(bug("nfjpeg: destroyed"));
}

/*--- Public functions ---*/

int32 JpegDriver::dispatch(uint32 fncode)
{
	int32 ret;

	D(bug("nfjpeg: dispatch(%u)", fncode));
	ret = TOS_EINVFN;

	switch(fncode)
	{
		case GET_VERSION:
			ret = ARANFJPEG_NFAPI_VERSION;
			break;
		case NFJPEG_OPENDRIVER:
			ret = open_driver(getParameter(0));
			break;
		case NFJPEG_GETSTRUCTSIZE:
			ret = sizeof(JPGD_STRUCT);
			break;
		case NFJPEG_CLOSEDRIVER:
			ret = close_driver(getParameter(0));
			break;
		case NFJPEG_GETIMAGEINFO:
			ret = get_image_info(getParameter(0));
			break;
		case NFJPEG_GETIMAGESIZE:
			ret = get_image_size(getParameter(0));
			break;
		case NFJPEG_DECODEIMAGE:
			ret = decode_image(getParameter(0),getParameter(1));
			break;
		default:
			D(bug("nfjpeg: unimplemented function #%d", fncode));
			break;
	}
	D(bug("nfjpeg: function returning with 0x%08x", ret));
	return ret;
}

int32 JpegDriver::open_driver(memptr jpeg_ptr)
{
	JPGD_STRUCT *jpgd;
	int i, j;

	D(bug("nfjpeg: open_driver(0x%08x)",jpeg_ptr));

	jpgd = (JPGD_STRUCT *)Atari2HostAddr(jpeg_ptr);

	/* Find a free handle */
	j = 0;
	for (i = 1; i <= MAX_NFJPEG_IMAGES; i++)
	{
		if (!images[i].used)
		{
			j = i;
			break;
		}
	}

	if (j == 0)
	{
		return DECODERBUSY;
	}

	jpgd->handle = j;
	images[j].used = true;
	return NOERROR;
}

int32 JpegDriver::close_driver(memptr jpeg_ptr)
{
	JPGD_STRUCT *jpgd;

	D(bug("nfjpeg: close_driver(0x%08x)",jpeg_ptr));

	jpgd = (JPGD_STRUCT *)Atari2HostAddr(jpeg_ptr);
	if (jpgd->handle <= 0 || jpgd->handle > MAX_NFJPEG_IMAGES || !images[jpgd->handle].used)
		return DRIVERCLOSED;
	if (images[jpgd->handle].src)
	{
		SDL_FreeSurface(images[jpgd->handle].src);
		images[jpgd->handle].src=NULL;
	}
	images[jpgd->handle].used = false;
	return NOERROR;
}

int32 JpegDriver::get_image_info(memptr jpeg_ptr)
{
	JPGD_STRUCT *jpgd;

	D(bug("nfjpeg: get_image_info(0x%08x)",jpeg_ptr));

	jpgd = (JPGD_STRUCT *)Atari2HostAddr(jpeg_ptr);
	if (jpgd->handle <= 0 || jpgd->handle > MAX_NFJPEG_IMAGES || !images[jpgd->handle].used)
		return DRIVERCLOSED;

	if (images[jpgd->handle].src == NULL)
	{
		/* Damn, we need to decode it with SDL_image */
		if (!load_image(jpgd, SDL_SwapBE32(jpgd->InPointer), SDL_SwapBE32(jpgd->InSize)))
		{
			return NOTENOUGHMEMORY;
		}
	}
	return NOERROR;
}

int32 JpegDriver::get_image_size(memptr jpeg_ptr)
{
	JPGD_STRUCT *jpgd;
	int image_size;
	SDL_Surface *surface;

	D(bug("nfjpeg: get_image_size(0x%08x)",jpeg_ptr));

	jpgd = (JPGD_STRUCT *)Atari2HostAddr(jpeg_ptr);
	if (jpgd->handle <= 0 || jpgd->handle > MAX_NFJPEG_IMAGES || !images[jpgd->handle].used)
		return DRIVERCLOSED;

	if (images[jpgd->handle].src == NULL)
	{
		/* Damn, we need to decode it with SDL_image */
		if (!load_image(jpgd, SDL_SwapBE32(jpgd->InPointer), SDL_SwapBE32(jpgd->InSize)))
		{
			return NOTENOUGHMEMORY;
		}
	}

	/* Recalculate OutSize and MFDBWordSize */
	surface = images[jpgd->handle].src;
	image_size = ((surface->w + xMCUs - 1) / xMCUs) * xMCUs;
	image_size *= SDL_SwapBE16(jpgd->OutPixelSize);
	jpgd->MFDBWordSize = SDL_SwapBE16(image_size>>1);

	image_size *= surface->h;
	jpgd->OutSize = SDL_SwapBE32(image_size);

	D(bug("nfjpeg: get_image_size() = %dx%dx%d -> %d",
		surface->w, surface->h, SDL_SwapBE16(jpgd->OutPixelSize), image_size));
	return NOERROR;
}

int32 JpegDriver::decode_image(memptr jpeg_ptr, uint32 row)
{
	JPGD_STRUCT *jpgd;
	unsigned char *dest,*src, *src_line;
	int width, height, r,g,b,x,y, line_length;
	SDL_Surface *surface;
	SDL_PixelFormat *format;
	memptr addr;

	D(bug("nfjpeg: decode_image(0x%08x,%d)",jpeg_ptr,row));

	jpgd = (JPGD_STRUCT *)Atari2HostAddr(jpeg_ptr);
	if (jpgd->handle <= 0 || jpgd->handle > MAX_NFJPEG_IMAGES || !images[jpgd->handle].used)
		return DRIVERCLOSED;

	if (images[jpgd->handle].src == NULL)
	{
		/* Damn, we need to decode it with SDL_image */
		if (!load_image(jpgd, SDL_SwapBE32(jpgd->InPointer), SDL_SwapBE32(jpgd->InSize)))
		{
			return NOTENOUGHMEMORY;
		}
	}

	addr = SDL_SwapBE32(jpgd->OutTmpPointer);
	surface = images[jpgd->handle].src;
	src = (unsigned char *)surface->pixels;
	src += surface->pitch * row * yRows;
	width = SDL_SwapBE16(jpgd->MFDBPixelWidth);
	line_length = SDL_SwapBE16(jpgd->MFDBWordSize)*2;
	height = surface->h - row * yRows;
	if (height > yRows)	/* not last row ? */
	{
		height = yRows;
	}
	format = surface->format;
	if (!valid_address(addr, true, height * line_length))
		return NOTENOUGHMEMORY;
	dest = (unsigned char *)Atari2HostAddr(addr);

	D(bug("nfjpeg: decode_image(), rows %d to %d", row * yRows, row * yRows + height - 1));
	D(bug("nfjpeg: decode_image(), OutPixelSize=%d,WordSize=%d,Width=%d",
		SDL_SwapBE16(jpgd->OutPixelSize),
		SDL_SwapBE16(jpgd->MFDBWordSize),
		SDL_SwapBE16(jpgd->MFDBPixelWidth)
	));

	if (jpgd->OutFlag == 0)
	{
		switch(SDL_SwapBE16(jpgd->OutPixelSize))
		{
		case 1:	/* Luminance */
			for (y = 0; y < height; y++)
			{
				unsigned char *dst_line;

				src_line = src;
				dst_line = dest;
				for (x = 0; x < width; x++)
				{
					read_rgb(format, src_line, &r, &g, &b);
					*dst_line++ = (r*30+g*59+b*11)/100;
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		case 2:	/* R5G6B5 (big endian) */
			for (y = 0; y < height; y++)
			{
				uint16 *dst_line;
				uint16 pixel;
				
				src_line = src;
				dst_line = (uint16 *)dest;
				for (x = 0; x < width; x++) {
					read_rgb(format, src_line, &r, &g, &b);
					pixel = ((r>>3)<<11) | ((g>>2)<<5) | (b>>3);
					*dst_line++ = SDL_SwapBE16(pixel);
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		case 3:	/* R8G8B8 (big endian) */
			for (y = 0; y < height; y++)
			{
				unsigned char *dst_line;

				src_line = src;
				dst_line = dest;
				for (x = 0; x < width; x++)
				{
					read_rgb(format, src_line, &r, &g, &b);
					*dst_line++ = r;
					*dst_line++ = g;
					*dst_line++ = b;
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		case 4:	/* FIXME A8R8G8B8 or R8G8B8A8 or something else ? */
			for (y = 0; y < height; y++)
			{
				uint32 *dst_line;
				uint32 pixel;

				src_line = src;
				dst_line = (uint32 *)dest;
				for (x = 0; x < width; x++)
				{
					read_rgb(format, src_line, &r, &g, &b);
					pixel = (r<<16) | (g<<8) | b;
					*dst_line++ = SDL_SwapBE32(pixel);
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		}
	} else
	{
		/* disk output means TGA format and we must write BGR */
		switch(SDL_SwapBE16(jpgd->OutPixelSize))
		{
		case 1:	/* Luminance */
			for (y = 0; y < height; y++)
			{
				unsigned char *dst_line;

				src_line = src;
				dst_line = dest;
				for (x = 0; x < width; x++)
				{
					read_rgb(format, src_line, &r, &g, &b);
					*dst_line++ = (r*30+g*59+b*11)/100;
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		case 2:	/* A1R5G5B5 (little endian) */
			for (y = 0; y < height; y++)
			{
				uint16 *dst_line;
				uint16 pixel;

				src_line = src;
				dst_line = (uint16 *)dest;
				for (x = 0; x < width; x++) {
					read_rgb(format, src_line, &r, &g, &b);
					pixel = ((r>>3)<<10) | ((g>>3)<<5) | (b>>3);
					*dst_line++ = SDL_SwapLE16(pixel);
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		case 3:	/* R8G8B8 (little endian) */
			for (y = 0; y < height; y++)
			{
				unsigned char *dst_line;

				src_line = src;
				dst_line = dest;
				for (x = 0; x < width; x++)
				{
					read_rgb(format, src_line, &r, &g, &b);
					*dst_line++ = b;
					*dst_line++ = g;
					*dst_line++ = r;
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		case 4:
			for (y = 0; y < height; y++)
			{
				unsigned char *dst_line;

				src_line = src;
				dst_line = dest;
				for (x = 0; x < width; x++)
				{
					read_rgb(format, src_line, &r, &g, &b);
					*dst_line++ = b;
					*dst_line++ = g;
					*dst_line++ = r;
					*dst_line++ = 0xff;
					src_line+=format->BytesPerPixel;
				}

				src += surface->pitch;
				dest += line_length;
			}
			break;
		}
	}
	return NOERROR;
}

bool JpegDriver::load_image(JPGD_STRUCT *jpgd, memptr addr, uint32 size)
{
	SDL_RWops *src;
	SDL_Surface *surface;
	int width, height, image_size;
	uint8 *buffer;

	D(bug("nfjpeg: load_image()"));

	if (!valid_address(addr, false, size))
		return false;
	buffer = Atari2HostAddr(addr);

	/* Load image from memory */
	src = SDL_RWFromMem(buffer, size);
	if (src == NULL)
	{
		D(bug("nfjpeg: load_image() failed in SDL_RWFromMem()"));
		return false;
	}
#if defined(HAVE_JPEGLIB)
	surface = load_jpeg(src);
#elif defined(HAVE_SDL_IMAGE)
	surface = IMG_Load_RW(src, 0);
#endif
	SDL_FreeRW(src);
	if (surface == NULL)
	{
		panicbug("nfjpeg: load_image() failed");
		return false;
	}

	D(bug("nfjpeg: %dx%dx%d,%d image", surface->w, surface->h, surface->format->BitsPerPixel,surface->format->BytesPerPixel));
	D(bug("nfjpeg: A=0x%08x, R=0x%08x, G=0x%08x, B=0x%08x",
		surface->format->Amask, surface->format->Rmask,
		surface->format->Gmask, surface->format->Bmask
	));

	images[jpgd->handle].src = surface;

	/* Fill values */
	jpgd->InComponents = SDL_SwapBE16(3); /* RGB */

	width = ((surface->w + xMCUs - 1) / xMCUs) * xMCUs;
	jpgd->XLoopCounter = SDL_SwapBE16(width / xMCUs);

	height = ((surface->h + yRows - 1) / yRows) * yRows;
	jpgd->YLoopCounter = SDL_SwapBE16(height / yRows);

	jpgd->MFDBAddress = 0;

	jpgd->MFDBPixelWidth = SDL_SwapBE16(surface->w);

	jpgd->MFDBPixelHeight = SDL_SwapBE16(surface->h);

	jpgd->MFDBWordSize = SDL_SwapBE16((width * SDL_SwapBE16(jpgd->OutPixelSize))>>1);

	jpgd->MFDBFormatFlag = 0;

	jpgd->MFDBBitPlanes = SDL_SwapBE16(jpgd->OutPixelSize) * 8;

	jpgd->MFDBReserved1 = jpgd->MFDBReserved2 = jpgd->MFDBReserved3 = 0;

	image_size = width * surface->h * SDL_SwapBE16(jpgd->OutPixelSize);
	jpgd->OutSize = SDL_SwapBE32(image_size);

	return true;
}

void JpegDriver::read_rgb(SDL_PixelFormat *format, void *src, int *r, int *g, int *b)
{
	uint32 color;

	color = 0;
	switch(format->BytesPerPixel)
	{
		case 1:
			{
				unsigned char *tmp;
				
				tmp = (unsigned char *)src;
				color = *tmp;
			}
			break;
		case 2:
			{
				uint16 *tmp;
				
				tmp = (uint16 *)src;
				color = *tmp;
			}
			break;
		case 3:
			{
				unsigned char *tmp;
				
				tmp = (unsigned char *)src;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				color = (tmp[2]<<16)|(tmp[1]<<8)|tmp[0];
#else
				color = (tmp[0]<<16)|(tmp[1]<<8)|tmp[2];
#endif
			}
			break;
		case 4:
			{
				uint32 *tmp;
				
				tmp = (uint32 *)src;
				color = *tmp;
			}
			break;
	}

	*r = color & (format->Rmask);
	*r >>= format->Rshift;
	*r <<= format->Rloss;
	*g = color & (format->Gmask);
	*g >>= format->Gshift;
	*g <<= format->Gloss;
	*b = color & (format->Bmask);
	*b >>= format->Bshift;
	*b <<= format->Bloss;
}
#endif /* NFJPEG_SUPPORT */

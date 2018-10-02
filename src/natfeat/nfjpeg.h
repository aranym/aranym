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

#ifndef NFJPEG_H
#define NFJPEG_H

/*--- Includes ---*/

#include "SDL_compat.h"

#include "nf_base.h"
#include "parameters.h"
#define ARANYM_NFJPEG
#include "../../atari/nfjpeg/jpgdh.h"
#undef ARANYM_NFJPEG
#ifdef HAVE_JPEGLIB
#ifdef __cplusplus
extern "C" {
#endif
#include <jpeglib.h>
#ifdef __cplusplus
}
#endif
#endif

/*--- Defines ---*/

#define MAX_NFJPEG_IMAGES	32

/*--- Types ---*/

typedef struct {
	bool used;
	SDL_Surface *src;	/* Image loaded by SDL_image */
} nfjpeg_image_t;

/*--- Class ---*/

class JpegDriver : public NF_Base
{
private:
	nfjpeg_image_t	images[MAX_NFJPEG_IMAGES+1];
#ifdef HAVE_JPEGLIB
	struct my_error_mgr {
	    struct jpeg_error_mgr errmgr;
	    jmp_buf escape;
	};
	struct my_error_mgr jerr;
	struct jpeg_decompress_struct jpeg;
	static void my_error_exit(j_common_ptr cinfo);
	SDL_Surface *load_jpeg(SDL_RWops *src);
#endif

	static const int xMCUs = 16;
	static const int yRows = 16;

	int32 open_driver(memptr jpeg_ptr);
	int32 close_driver(memptr jpeg_ptr);
	int32 get_image_info(memptr jpeg_ptr);
	int32 get_image_size(memptr jpeg_ptr);
	int32 decode_image(memptr jpeg_ptr, uint32 row);

	bool load_image(JPGD_STRUCT *jpgd_ptr, memptr addr, uint32 size);
	void read_rgb(SDL_PixelFormat *format, void *src, int *r, int *g, int *b);

public:
	JpegDriver();
	virtual ~JpegDriver();

	const char *name() { return "JPEG"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

#endif /* NFJPEG_H */

#include "fvdi.h"
#include "relocate.h"
#include "driver.h"


int nf_initialize(void);

long CDECL c_get_videoramaddress(void);
void CDECL c_set_resolution(long width, long height, long depth, long freq);
long CDECL c_get_width(void);
long CDECL c_get_height(void);
void CDECL c_openwk(Virtual *vwk);
void CDECL c_closewk(Virtual *vwk);

/* color bit organization */
char none[] = {0};
char r_8[] = {8};
char g_8[] = {8};
char b_8[] = {8};
char r_16[] = {5, 3, 4, 5, 6, 7};
char g_16[] = {6, 13, 14, 15, 0, 1, 2};
char b_16[] = {5, 8, 9, 10, 11, 12};
char r_16f[] = {5, 11, 12, 13, 14, 15};
char g_16f[] = {6, 5, 6, 7, 8, 9, 10};
char b_16f[] = {5, 0, 1, 2, 3, 4};
char r_32[] = {8, 16, 17, 18, 19, 20, 21, 22, 23};
char g_32[] = {8,  8,  9, 10, 11, 12, 13, 14, 15};
char b_32[] = {8,  0,  1,  2,  3,  4,  5,  6,  7};
char r_32f[] = {8,  8,  9, 10, 11, 12, 13, 14, 15};
char g_32f[] = {8, 16, 17, 18, 19, 20, 21, 22, 23};
char b_32f[] = {8, 24, 25, 26, 27, 28, 29, 30, 31};

/**
 * Mode *graphics_mode
 *
 * bpp     The number of bits per pixel
 *
 * flags   Various information (OR together the appropriate ones)
 *           CHECK_PREVIOUS - Ask fVDI to look at the previous graphics mode
 *                            set by the ROM VDI (I suppose.. *standa*)
 *           CHUNKY         - Pixels are chunky
 *           TRUE_COLOUR    - Pixel value is colour value (no palette)
 *
 * bits    Poperly set up MBits structure:
 *           red, green, blue,  - Pointers to arrays containing the number of
 *           alpa, genlock,       of bits and the corresponding bit numbers
 *           unused               (the latter only for true colour modes)
 *
 * code    Driver dependent value
 *
 * format  Type of graphics mode
 *           0 - interleaved
 *           2 - packed pixels
 *
 * clut    Type of colour look up table
 *           1 - hardware
 *           2 - software
 *
 * org     Pixel bit organization (OR together the appropriate ones)
 *           0x01 - usual bit order
 *           0x80 - Intel byte order
 **/
Mode mode[7] = /* FIXME: big and little endian differences. */
{
	                   /* ... 0, interleaved, hardware clut, usual bit order */
	{ 1, CHECK_PREVIOUS, {r_8,   g_8,   b_8,    none, none, none}, 0, 0, 1, 1},
	{ 2, CHECK_PREVIOUS, {r_8,   g_8,   b_8,    none, none, none}, 0, 0, 1, 1},
	{ 4, CHECK_PREVIOUS, {r_8,   g_8,   b_8,    none, none, none}, 0, 0, 1, 1},
	{ 8, CHECK_PREVIOUS, {r_8,   g_8,   b_8,    none, none, none}, 0, 0, 1, 1},

	          /* ... 0, packed pixels, software clut (none), usual bit order */
	{16, CHECK_PREVIOUS | CHUNKY | TRUE_COLOUR,
                              {r_16f, g_16f, b_16f, none, none, none}, 0, 2, 2, 1},
	{24, CHECK_PREVIOUS | CHUNKY | TRUE_COLOUR,
                              {r_32f, g_32f, b_32f, none, none, none}, 0, 2, 2, 1},
	{32, CHECK_PREVIOUS | CHUNKY | TRUE_COLOUR,
                              {r_32,  g_32,  b_32,  none, none, none}, 0, 2, 2, 1}
};

extern Device device;

char driver_name[] = "NatFeat/ARAnyM 2005-01-24 (xx bit)";

struct {
	short used; /* Whether the mode option was used or not. */
	short width;
	short height;
	short bpp;
	short freq;
} resolution = {0, 640, 480, 16, 85};

struct {
	short width;
	short height;
} pixel;

extern Driver *me;
extern Access *access;

extern short *loaded_palette;

extern short colours[][3];
extern void CDECL initialize_palette(Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
extern void CDECL c_initialize_palette(Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
extern long tokenize(char *value);

long wk_extend = 0;

short accel_s = 0;
short accel_c = A_SET_PIX | A_GET_PIX | A_MOUSE | A_LINE | A_BLIT | A_FILL | A_EXPAND | A_FILLPOLY | A_SET_PAL | A_GET_COL;

Mode *graphics_mode = &mode[1];

short debug = 0;

extern void *c_write_pixel;
extern void *c_read_pixel;
extern void *c_line_draw;
extern void *c_expand_area;
extern void *c_fill_area;
extern void *c_fill_polygon;
extern void *c_blit_area;
extern void *c_mouse_draw;
extern void *c_set_colours_8, *c_set_colours_16, *c_set_colours_32;
extern void *c_get_colours_8, *c_get_colours_16, *c_get_colours_32;
extern void *c_get_colour_8, *c_get_colour_16, *c_get_colour_32;

void *write_pixel_r = &c_write_pixel;
void *read_pixel_r  = &c_read_pixel;
void *line_draw_r   = &c_line_draw;
void *expand_area_r = &c_expand_area;
void *fill_area_r   = &c_fill_area;
void *fill_poly_r   = &c_fill_polygon;
void *blit_area_r   = &c_blit_area;
void *text_area_r   = 0;
void *mouse_draw_r  = &c_mouse_draw;
void *set_colours_r = &c_set_colours_16;
void *get_colours_r = &c_get_colours_16;
void *get_colour_r  = &c_get_colour_16;

long set_mode(const char **ptr);
long set_scrninfo(const char **ptr);


Option options[] = {
	{"mode",       set_mode,          -1},  /* mode WIDTHxHEIGHTxDEPTH@FREQ */
	{"scrninfo",   set_scrninfo,      -1},  /* scrninfo fb, make vq_scrninfo return values regarding actual fb layout */
	{"debug",      &debug,             2}   /* debug, turn on debugging aids */
};


char *get_num(char *token, short *num)
{
	char buf[10], c;
	int i;

	*num = -1;
	if (!*token)
		return token;
	for(i = 0; i < 10; i ++) {
		c = buf[i] = *token++;
		if ((c < '0') || (c > '9'))
			break;
	}
	if (i > 5)
		return token;

	buf[i] = '\0';
	*num = access->funcs.atol(buf);
	return token;
}


long set_mode(const char **ptr)
{
	char token[80], *tokenptr;
	
	if (!(*ptr = access->funcs.skip_space(*ptr)))
		;		/* *********** Error, somehow */
	*ptr = access->funcs.get_token(*ptr, token, 80);

	tokenptr = token;
	tokenptr = get_num(tokenptr, &resolution.width);
	tokenptr = get_num(tokenptr, &resolution.height);
	tokenptr = get_num(tokenptr, &resolution.bpp);
	tokenptr = get_num(tokenptr, &resolution.freq);

	resolution.used = 1;
	
	switch (resolution.bpp) {
	case 1:
		graphics_mode = &mode[0];
		driver_name[28] = '1';
		break;
	case 2:
		graphics_mode = &mode[1];
		driver_name[28] = '2';
		break;
	case 4:
		graphics_mode = &mode[2];
		driver_name[28] = '4';
		break;
	case 8:
		driver_name[28] = '8';
	case 16:
	case 24:
	case 32:
		graphics_mode = &mode[resolution.bpp / 8 + 2];
		break;
	default:
		resolution.bpp = 16;		/* Default as 16 bit */
	}

	switch (resolution.bpp) {
	case 16:
		set_colours_r = &c_set_colours_16;
		get_colours_r = &c_get_colours_16;
		get_colour_r  = &c_get_colour_16;
		driver_name[27] = '1';
		driver_name[28] = '6';
		break;
	case 24:
	case 32:
		set_colours_r = &c_set_colours_32;
		get_colours_r = &c_get_colours_32;
		get_colour_r  = &c_get_colour_32;
		driver_name[27] = '3';
		driver_name[28] = '2';
		break;
	/* indexed color modes */
	default:
		set_colours_r = &c_set_colours_8;
		get_colours_r = &c_get_colours_8;
		get_colour_r  = &c_get_colour_8;
		driver_name[27] = ' ';
		break;
	}

	return 1;
}


void setup_scrninfo(Device *device, Mode *graphics_mode); /* from init.c */

long set_scrninfo(const char** ptr)
{
	char token[80];

	if (!(*ptr = access->funcs.skip_space(*ptr)))
		;		/* *********** Error, somehow */
	*ptr = access->funcs.get_token(*ptr, token, 80);

	if (access->funcs.equal(token, "fb")) {
		mode[4].bits.red = r_16;
		mode[4].bits.green = g_16;
		mode[4].bits.blue = b_16;
		mode[4].org = 0x81;
		mode[5].bits.red = r_32;
		mode[5].bits.green = g_32;
		mode[5].bits.blue = b_32;
		mode[5].org = 0x81;
		mode[6].bits.red = r_32;
		mode[6].bits.green = g_32;
		mode[6].bits.blue = b_32;
		mode[6].org = 0x81;
	} else {
		mode[4].bits.red = r_16f;
		mode[4].bits.green = g_16f;
		mode[4].bits.blue = b_16f;
		mode[4].org = 0x01;
		mode[5].bits.red = r_32f;
		mode[5].bits.green = g_32f;
		mode[5].bits.blue = b_32f;
		mode[5].org = 0x01;
		mode[6].bits.red = r_32f;
		mode[6].bits.green = g_32f;
		mode[6].bits.blue = b_32f;
		mode[6].org = 0x01;
	}

	if (me && me->device)
		setup_scrninfo(me->device, graphics_mode);

	return 1;
}


/*
 * Handle any driver specific parameters
 */
long check_token(char *token, const char **ptr)
{
	int i;
	int normal;
	char *xtoken;

	xtoken = token;
	switch (token[0]) {
	case '+':
		xtoken++;
		normal = 1;
		break;
	case '-':
		xtoken++;
		normal = 0;
		break;
	default:
		normal = 1;
		break;
	}

	for(i = 0; i < sizeof(options) / sizeof(Option); i++) {
		if (access->funcs.equal(xtoken, options[i].name)) {
			switch (options[i].type) {
			case -1:	  /* Function call */
				return ((long (*)(const char **))options[i].varfunc)(ptr);
			case 0:	  /* Default 1, set to 0 */
				*(short *)options[i].varfunc = 1 - normal;
				return 1;
			case 1:	 /* Default 0, set to 1 */
				*(short *)options[i].varfunc = normal;
				return 1;
			case 2:	 /* Increase */
				*(short *)options[i].varfunc += -1 + 2 * normal;
				return 1;
			case 3:
				if (!(*ptr = access->funcs.skip_space(*ptr)))
					;	 /* *********** Error, somehow */
				*ptr = access->funcs.get_token(*ptr, token, 80);
				*(short *)options[i].varfunc = token[0];
				return 1;
			}
		}
	}

	return 0;
}


static void setup_wk(Virtual *vwk)
{
	Workstation *wk = vwk->real_address;

	/* update the settings */
	wk->screen.mfdb.width = resolution.width;
	wk->screen.mfdb.height = resolution.height;
	wk->screen.mfdb.bitplanes = resolution.bpp;

	/*
	 * Some things need to be changed from the
	 * default workstation settings.
	 */
	wk->screen.mfdb.address = (void *)c_get_videoramaddress();
	wk->screen.mfdb.wdwidth = ((long)wk->screen.mfdb.width * wk->screen.mfdb.bitplanes) / 16;
	wk->screen.wrap = wk->screen.mfdb.width * (wk->screen.mfdb.bitplanes / 8);

	wk->screen.coordinates.max_x = wk->screen.mfdb.width - 1;
	wk->screen.coordinates.max_y = (wk->screen.mfdb.height & 0xfff0) - 1;	/* Desktop can't deal with non-16N heights */

	wk->screen.look_up_table = 0;			/* Was 1 (???)	Shouldn't be needed (graphics_mode) */
	wk->screen.mfdb.standard = 0;

	if (pixel.width > 0)			/* Starts out as screen width */
		wk->screen.pixel.width = (pixel.width * 1000L) / wk->screen.mfdb.width;
	else								   /*	or fixed DPI (negative) */
		wk->screen.pixel.width = 25400 / -pixel.width;

	if (pixel.height > 0)		/* Starts out as screen height */
		wk->screen.pixel.height = (pixel.height * 1000L) / wk->screen.mfdb.height;
	else									/*	 or fixed DPI (negative) */
		wk->screen.pixel.height = 25400 / -pixel.height;
	
	device.address		= wk->screen.mfdb.address;
	device.byte_width	= wk->screen.wrap;

	/**
	 * The following needs to be here due to bpp > 8 modes where the SDL
	 * palette needs the appropriate SDL_surface->format to be set prior
	 * use i.e. _after_ the resolution change
	 **/
	c_initialize_palette(vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);
}


static void initialize_wk(Virtual *vwk)
{
	Workstation *wk = vwk->real_address;

	if (loaded_palette)
		access->funcs.copymem(loaded_palette, colours, 256 * 3 * sizeof(short));

	/*
	 * This code needs more work.
	 * Especially if there was no VDI started since before.
	 */

	if (wk->screen.palette.size != 256) {	/* Started from different graphics mode? */
		Colour *old_palette_colours = wk->screen.palette.colours;
		wk->screen.palette.colours = (Colour *)access->funcs.malloc(256L * sizeof(Colour), 3);	/* Assume malloc won't fail. */
		if (wk->screen.palette.colours) {
			wk->screen.palette.size = 256;
			if (old_palette_colours)
				access->funcs.free(old_palette_colours);	/* Release old (small) palette (a workaround) */
		} else
			wk->screen.palette.colours = old_palette_colours;
	}


	pixel.width = wk->screen.pixel.width;
	pixel.height = wk->screen.pixel.height;
}


/*
 * Do whatever setup work might be necessary on boot up
 * and which couldn't be done directly while loading.
 * Supplied is the default fVDI virtual workstation.
 */
void CDECL initialize(Virtual *vwk)
{
	vwk = me->default_vwk;  /* This is what we're interested in */
	
	if (!nf_initialize()) {
		access->funcs.puts("  No or incompatible NatFeat fVDI!");
		access->funcs.puts("\x0d\x0a");
		return;
	}

	initialize_wk(vwk);

	setup_wk(vwk);

	if (debug > 2) {
		char buf[10];
		access->funcs.puts("  fb_base  = $");
		access->funcs.ltoa(buf, c_get_videoramaddress(), 16);
		access->funcs.puts(buf);
		access->funcs.puts("\x0d\x0a");
	}
}


/*
 *
 */
long CDECL setup(long type, long value)
{
	long ret;

	ret = -1;
	switch((int)type) {
	case Q_NAME:
		ret = (long)driver_name;
		break;
	case S_DRVOPTION:
		ret = tokenize((char *)value);
		break;
	}

	return ret;
}


/*
 * Initialize according to parameters (boot and sent).
 * Create new (or use old) Workstation and default Virtual.
 * Supplied is the default fVDI virtual workstation.
 */
Virtual* CDECL opnwk(Virtual *vwk)
{
	vwk = me->default_vwk;  /* This is what we're interested in */

	/* switch off VIDEL */
	c_openwk(vwk);

	if (resolution.used) {
		resolution.bpp = graphics_mode->bpp; /* Table value (like rounded down) --- e.g. no 23bit but 16 etc */

		c_set_resolution(resolution.width, resolution.height, resolution.bpp, resolution.freq);
	} else {
		/* FIXME: Hack to get it working after boot in less than 16bit */
		resolution.bpp = graphics_mode->bpp; /* 16 bit by default */
	}

	/* update the width/height if restricted by the native part */
	resolution.width = c_get_width();
	resolution.height = c_get_height();

	setup_wk(vwk);

	return 0;
}


/*
 * 'Deinitialize'
 */
void CDECL clswk(Virtual *vwk)
{
	c_closewk(vwk);
}


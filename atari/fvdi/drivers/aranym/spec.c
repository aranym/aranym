#include "fvdi.h"
#include "relocate.h"
#include "driver.h"


extern long CDECL c_get_videoramaddress(void); /* STanda */
extern void CDECL c_set_resolution(long width, long height, long depth, long freq); /* STanda */
extern int nf_initialize();

#if 0
char r_16[] = {5, 15, 14, 13, 12, 11};
char g_16[] = {6, 10, 9, 8, 7, 6, 5};
char b_16[] = {5, 4, 3, 2, 1, 0};
char none[] = {0};

char red[] = {5, 11, 12, 13, 14, 15};
char green[] = {5, 6, 7, 8, 9, 10};
char blue[] = {5, 0, 1, 2, 3, 4};
char alpha[] = {0};
char genlock[] = {0};
char unused[] = {1, 5};
#endif

char r_8[] = {8};
char g_8[] = {8};
char b_8[] = {8};
#if 0
char r_16[] = {5, 7, 6, 5, 4, 3};
char g_16[] = {6, 2, 1, 0, 15, 14, 13};
char b_16[] = {5, 12, 11, 10, 9, 8};
char r_16f[] = {5, 15, 14, 13, 12, 11};
char g_16f[] = {6, 10, 9, 8, 7, 6, 5};
char b_16f[] = {5, 4, 3, 2, 1, 0};
char r_32[] = {8, 15, 14, 13, 12, 11, 10,  9,  8};
char g_32[] = {8, 23, 22, 21, 20, 19, 18, 17, 16};
char b_32[] = {8, 31, 30, 29, 28, 27, 26, 25, 24};
char r_32f[] = {8, 23, 22, 21, 20, 19, 18, 17, 16};
char g_32f[] = {8, 15, 14, 13, 12, 11, 10,  9, 8};
char b_32f[] = {8,  7,  6,  5,  4,  3,  2,  1, 0};
#else
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
#endif
char none[] = {0};

Mode mode[7] = /* FIXME: big and little endian differences. */
	{
	 { 1, CHUNKY | CHECK_PREVIOUS,               {r_8,   g_8,   b_8,   none, none, none}, 0, 2, 2, 1},
	 { 2, CHUNKY | CHECK_PREVIOUS,               {r_8,   g_8,   b_8,   none, none, none}, 0, 2, 2, 1},
	 { 4, CHUNKY | CHECK_PREVIOUS,               {r_8,   g_8,   b_8,   none, none, none}, 0, 2, 2, 1},
	 { 8, CHUNKY | CHECK_PREVIOUS,               {r_8,   g_8,   b_8,   none, none, none}, 0, 2, 2, 1},
	 {16, CHUNKY | CHECK_PREVIOUS | TRUE_COLOUR, {r_16f, g_16f, b_16f, none, none, none}, 0, 2, 2, 1},
	 {24, CHUNKY | CHECK_PREVIOUS | TRUE_COLOUR, {r_32f, g_32f, b_32f, none, none, none}, 0, 2, 2, 1},
	 {32, CHUNKY | CHECK_PREVIOUS | TRUE_COLOUR, {r_32,  g_32,  b_32,  none, none, none}, 0, 2, 2, 1}};

extern Device device;

char driver_name[] = "NatFeat/ARAnyM 2002-10-21 (xx bit, shadow)";

struct {
	short used; /* Whether the mode option was used or not. */
	short width;
	short height;
	short bpp;
	short freq;
} resolution = {0, 640, 480, 16, 85};


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

short shadow = 1;
short nf_check = 1;

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


#if 0
short cache_img = 0;
short cache_from_screen = 0;
#endif

long set_mode(const char **ptr);
long set_scrninfo(const char **ptr);


Option options[] = {
#if 0
	{"aesbuf",     set_aesbuf,        -1},  /* aesbuf address, set AES background buffer address */
	{"screen",     set_screen,        -1},  /* screen address, set old screen address */
	{"imgcache",   &cache_img,         1},  /* imgcache, turn on caching of images blitted to the screen */
	{"screencache",&cache_from_screen, 1},  /* screencache, turn on caching of images blitted from the screen */
#endif
	{"mode",       set_mode,          -1},  /* mode WIDTHxHEIGHTxDEPTH@FREQ */
	{"noshadow",   &shadow,            0},  /* noshadow, do not use a RAM buffer */
	{"assumenf",   &nf_check,          0},  /* assumenf, do not look for __NF cookie */
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
		break;
	case 2:
		graphics_mode = &mode[1];
		break;
	case 4:
		graphics_mode = &mode[2];
		break;
	default:
		resolution.bpp = 16;		/* Default as 16 bit */
	case 8:
	case 16:
	case 24:
	case 32:
		graphics_mode = &mode[resolution.bpp / 8 + 2];
		break;
	}

	switch (resolution.bpp) {
	case 8:
		set_colours_r = &c_set_colours_8;
		get_colours_r = &c_get_colours_8;
		get_colour_r  = &c_get_colour_8;
		driver_name[27] = '8';
		driver_name[28] = ' ';
		break;
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
	}

	return 1;
}


#if 0
long set_aesbuf(const char **ptr)
{
	char token[80];

	if (!(*ptr = access->funcs.skip_space(*ptr)))
		;		/* *********** Error, somehow */
	*ptr = access->funcs.get_token(*ptr, token, 80);
	aes_buffer = access->funcs.atol(token);

	return 1;
}

long set_screen(const char **ptr)
{
	char token[80];

	if (!(*ptr = access->funcs.skip_space(*ptr)))
		;		/* *********** Error, somehow */
	*ptr = access->funcs.get_token(*ptr, token, 80);
	old_screen = access->funcs.atol(token);

	return 1;
}
#endif


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
		mode[5].org = 0x81;
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


/*
 * Do whatever setup work might be necessary on boot up
 * and which couldn't be done directly while loading.
 * Supplied is the default fVDI virtual workstation.
 */
void CDECL initialize(Virtual *vwk)
{
	Workstation *wk;
	int old_palette_size;
	Colour *old_palette_colours;
	long fb_base;

	if (nf_check) {
		long nf_value;
		nf_value = access->funcs.get_cookie("__NF", 0);
		if (nf_value == -1) {
			access->funcs.puts("  Could not find NatFeat cookie!");
			access->funcs.puts("\x0d\x0a");
			return;
		}
	}
	if (!nf_initialize()) {
		access->funcs.puts("  No or incompatible NatFeat fVDI!");
		access->funcs.puts("\x0d\x0a");
		return;
	}

	fb_base = c_get_videoramaddress();

#if 0
	debug = access->funcs.misc(0, 1, 0);
#endif

	if (debug > 2) {
		char buf[10];
		access->funcs.puts("  fb_base  = $");
		access->funcs.ltoa(buf, fb_base, 16);
		access->funcs.puts(buf);
		access->funcs.puts("\x0d\x0a");
	}

	vwk = me->default_vwk;	/* This is what we're interested in */
	wk = vwk->real_address;


	if (resolution.used) {
		resolution.bpp = graphics_mode->bpp; /* Table value (like rounded down) --- e.g. no 23bit but 16 etc */

		wk->screen.mfdb.width = resolution.width;
		wk->screen.mfdb.height = resolution.height;
	} else {
		/* FIXME: Hack to get it working after boot in less than 16bit */
		resolution.bpp = graphics_mode->bpp; /* 16 bit by default */
	}


	/*
	 * Some things need to be changed from the	
	 * default workstation settings.
	 */
	wk->screen.mfdb.address = (void *)fb_base;
	wk->screen.mfdb.wdwidth = ((long)wk->screen.mfdb.width * resolution.bpp) / 16;
	wk->screen.mfdb.bitplanes = resolution.bpp;
	wk->screen.wrap = wk->screen.mfdb.width * (resolution.bpp / 8);

	wk->screen.coordinates.max_x = wk->screen.mfdb.width - 1;
	wk->screen.coordinates.max_y = (wk->screen.mfdb.height & 0xfff0) - 1;	/* Desktop can't deal with non-16N heights */

	wk->screen.look_up_table = 0;			/* Was 1 (???)	Shouldn't be needed (graphics_mode) */
	wk->screen.mfdb.standard = 0;
	if (wk->screen.pixel.width > 0)			/* Starts out as screen width */
		wk->screen.pixel.width = (wk->screen.pixel.width * 1000L) / wk->screen.mfdb.width;
	else								   /*	or fixed DPI (negative) */
		wk->screen.pixel.width = 25400 / -wk->screen.pixel.width;
	if (wk->screen.pixel.height > 0)		/* Starts out as screen height */
		wk->screen.pixel.height = (wk->screen.pixel.height * 1000L) / wk->screen.mfdb.height;
	else									/*	 or fixed DPI (negative) */
		wk->screen.pixel.height = 25400 / -wk->screen.pixel.height;
	

	/*
	 * This code needs more work.
	 * Especially if there was no VDI started since before.
	 */

	if (loaded_palette)
		access->funcs.copymem(loaded_palette, colours, 256 * 3 * sizeof(short));
	if ((old_palette_size = wk->screen.palette.size) != 256) {	/* Started from different graphics mode? */
		old_palette_colours = wk->screen.palette.colours;
		wk->screen.palette.colours = (Colour *)access->funcs.malloc(256L * sizeof(Colour), 3);	/* Assume malloc won't fail. */
		if (wk->screen.palette.colours) {
			wk->screen.palette.size = 256;
			if (old_palette_colours)
				access->funcs.free(old_palette_colours);	/* Release old (small) palette (a workaround) */
		} else
			wk->screen.palette.colours = old_palette_colours;
	}

	if (debug > 1) {
		access->funcs.puts("  Setting up palette");
		access->funcs.puts("\x0d\x0a");
	}

	if (accel_c & A_SET_PAL)
		c_initialize_palette(vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);
	else
		initialize_palette(vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);

	device.byte_width = wk->screen.wrap;
	device.address = wk->screen.mfdb.address;

	if (!wk->screen.shadow.address) {
		driver_name[33] = ')';
		driver_name[34] = 0;
	}
}

/*
 *
 */
long CDECL setup(long type, long value)
{
	long ret;

	ret = -1;
	switch(type) {
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
	if (resolution.used) {
		c_set_resolution(resolution.width, resolution.height, resolution.bpp, resolution.freq);
		device.address = (void*)c_get_videoramaddress();
		me->default_vwk->real_address->screen.mfdb.address = device.address;
	}

	return 0;
}

/*
 * 'Deinitialize'
 */
void CDECL clswk(Virtual *vwk)
{
}


#include "fvdi.h"
#include "relocate.h"
#include "driver.h"

#define ARANYM

#ifdef ARANYM
#define ARANYMVRAMSTART 0xf0000000UL
extern void CDECL set_resolution(long width, long height, long depth, long freq); /* STanda */
extern void CDECL debug_aranym(long freq); /* STanda */
#endif


#if 0
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

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
char r_32f[] = {8,  8,  9, 10, 11, 12, 13, 14, 15};
char r_32[] = {8, 16, 17, 18, 19, 20, 21, 22, 23};
char b_32f[] = {8, 24, 25, 26, 27, 28, 29, 30, 31};
char g_32f[] = {8, 16, 17, 18, 19, 20, 21, 22, 23};
char g_32[] = {8,  8,  9, 10, 11, 12, 13, 14, 15};
char b_32[] = {8,  0,  1,  2,  3,  4,  5,  6,  7};
#endif
char none[] = {0};

Mode mode[4] = /* FIXME: big and little endian differences. */
	{{ 8, CHUNKY | CHECK_PREVIOUS,               {r_8,  g_8,  b_8,  none, none, none}, 0, 2, 1, 1},
	 {16, CHUNKY | CHECK_PREVIOUS | TRUE_COLOUR, {r_16f, g_16f, b_16f, none, none, none}, 0, 2, 2, 1}, /*DEPTH_SUPPORT_565*/
	 {24, CHUNKY | CHECK_PREVIOUS | TRUE_COLOUR, {r_32f, g_32f, b_32f, none, none, none}, 0, 2, 2, 1}, /*DEPTH_SUPPORT_RGB*/
	 {32, CHUNKY | CHECK_PREVIOUS | TRUE_COLOUR, {r_32, g_32, b_32, none, none, none}, 0, 2, 2, 1}}; /*DEPTH_SUPPORT_ARGB*/

extern Device device;

char driver_name[] = "ARAnyM 2001-10-30 (|| bit)";

#ifdef ARANYM
struct {
	short used; /* Whether the mode option was used or not. */
	short width;
	short height;
	short bpp;
	short freq;
} resolution = {0, 640, 480, 16, 85};
#endif


extern Driver *me;
extern Access *access;

extern short *loaded_palette;

extern short colours[][3];
extern void CDECL initialize_palette(Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
extern void CDECL c_initialize_palette(Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
extern long tokenize( char* value );

long wk_extend = 0;

short accel_s = A_SET_PIX | A_GET_PIX | A_MOUSE | A_LINE | A_BLIT | A_FILL | A_EXPAND | A_FILLPOLY;
short accel_c = A_SET_PAL | A_GET_COL;

Mode *graphics_mode = &mode[1];

short debug = 0;

short shadow = 0;

#ifdef ARANYM
extern void *c_set_colours_8, *c_set_colours_16, *c_set_colours_32;
extern void *c_get_colours_8, *c_get_colours_16, *c_get_colours_32;
extern void *c_get_colour_8, *c_get_colour_16, *c_get_colour_32;

void *set_colours_r = &c_set_colours_16;
void *get_colours_r = &c_get_colours_16;
void *get_colour_r  = &c_get_colour_16;
#endif


#if 0
short cache_img = 0;
short cache_from_screen = 0;

long mode_no = 0;

extern short max_mode;

extern Modes mode_tab[];

char *preset[] = {"640x480x8@60	   ", "800x600x8@70    ", "1024x768x8@70   ", "1152x864x8@70   ",
                  "800x600x16@70   ", "1024x768x16@70  ", "1152x864x16@70  ", "1280x1024x16@70 ",
                  "800x600x32@70   ", "1024x768x32@70  "};
#endif

long set_mode(const char **ptr);


Option options[] = {
#if 0
   {"aesbuf",     set_aesbuf,     -1},  /* aesbuf address, set AES background buffer address */
   {"screen",	  set_screen,	  -1},  /* screen address, set old screen address */
   {"imgcache",	  &cache_img,	   1},  /* imgcache, turn on caching of images blitted to the screen */
   {"screencache",&cache_from_screen, 1},  /* screencache, turn on caching of images blitted from the screen */
#endif
   {"mode",       set_mode,       -1},  /* mode key/<n>/WIDTHxHEIGHTxDEPTH@FREQ */
   {"shadow",     &shadow,         1},  /* shadow, use a FastRAM buffer */
   {"debug",      &debug,          2}   /* debug, turn on debugging aids */
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


#if RAGE
int search_mode(char *token)
{
	short width, height, bpp, freq;
	int b, w, f;

	token = get_num(token, &width);
	token = get_num(token, &height);
	token = get_num(token, &bpp);
	token = get_num(token, &freq);


	b = 0;
	while ((mode_tab[b].bpp != -1) &&
		   ((unsigned int)mode_tab[b + 1].bpp <= bpp))
		b++;
	w = 0;
	while ((mode_tab[b].res_freq[w].width != -1) &&
		   ((unsigned int)mode_tab[b].res_freq[w + 1].width <= width))
		w++;
	f = 0;
	while ((f < mode_tab[b].res_freq[w].n) &&
		   ((unsigned int)mode_tab[b].res_freq[w].freqs[f + 1].frequency <= freq))
		f++;

	return &mode_tab[b].res_freq[w].freqs[f] - res_info;
}
#endif


long set_mode(const char **ptr)
{
	char token[80], *tokenptr;
	short depth;
	
	if (!(*ptr = access->funcs.skip_space(*ptr)))
		;		/* *********** Error, somehow */
	*ptr = access->funcs.get_token(*ptr, token, 80);


#ifdef ARANYM
	tokenptr = token;
	tokenptr = get_num(tokenptr, &resolution.width);
	tokenptr = get_num(tokenptr, &resolution.height);
	tokenptr = get_num(tokenptr, &resolution.bpp);
	tokenptr = get_num(tokenptr, &resolution.freq);

	depth = (resolution.bpp / 8) - 1;
	resolution.used = 1;
#else
	rage_mode = -1;
	if (access->funcs.equal(token, "key"))
		rage_mode = access->funcs.misc(0, 0, 0) - '0';		/* Fetch key value */
	else if (!token[1])
		rage_mode = access->funcs.atol(token);		/* Single character mode */
	if (rage_mode != -1)
		tokenptr = preset[rage_mode];
	else
		tokenptr = token;
	rage_mode = search_mode(tokenptr);

	if ((rage_mode < 0) || (rage_mode > max_mode))
		rage_mode = 0;

	depth = (res_info[rage_mode].pll[3] & 3) - 1;
#endif

	graphics_mode = &mode[depth];

	switch (depth) {
	case 0:
		set_colours_r = &c_set_colours_8;
		get_colours_r = &c_get_colours_8;
		get_colour_r  = &c_get_colour_8;
#if 0
		driver_name[27] = '8';
		driver_name[28] = ' ';
#endif
		break;
	case 1:
		set_colours_r = &c_set_colours_16;
		get_colours_r = &c_get_colours_16;
		get_colour_r  = &c_get_colour_16;
#if 0
		driver_name[27] = '1';
		driver_name[28] = '6';
#endif
		break;
	case 2:
	case 3:
		set_colours_r = &c_set_colours_32;
		get_colours_r = &c_get_colours_32;
		get_colour_r  = &c_get_colour_32;
#if 0
		driver_name[27] = '3';
		driver_name[28] = '2';
#endif
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
#ifdef FAST
	char *buf;
#endif
	int old_palette_size;
	Colour *old_palette_colours;
#ifdef FAST
	int fast_w_bytes;
#endif
#ifdef ARANYM
	long fb_base = ARANYMVRAMSTART;
#endif
#if 0
	int i;
#endif

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


#ifdef ARANYM
	if (resolution.used) {
		resolution.bpp = graphics_mode->bpp; /* Table value (like rounded down) --- e.g. no 23bit but 16 etc */
		set_resolution( resolution.width, resolution.height, resolution.bpp, resolution.freq );

		wk->screen.mfdb.width = resolution.width;
		wk->screen.mfdb.height = resolution.height;
	} else {
		/* FIXME: Hack to get it working after boot in less than 16bit */
		resolution.bpp = graphics_mode->bpp; /* 16bit bz default */
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
#endif

	wk->screen.look_up_table = 0;			/* Was 1 (???)	Shouldn't be needed (graphics_mode) */
	wk->screen.mfdb.standard = 0;
	if (wk->screen.pixel.width > 0)		   /* Starts out as screen width */
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


#ifdef ARANYM
#if 0
	debug_aranym(wk->screen.mfdb.width); /* STanda: ARAnyM emul_op debug call (20) */
	debug_aranym(wk->screen.mfdb.bitplanes); /* STanda: ARAnyM emul_op debug call (20) */
#endif
#endif


#ifdef FAST
	if (shadow) {
 #if 0			/* It's not clear that this is a good idea */
		fast_w_bytes = (wk->screen.wrap + 15) & 0xfffffff0;
 #else
		fast_w_bytes = wk->screen.wrap;
 #endif
		buf = (char *)access->funcs.malloc((long)fast_w_bytes * wk->screen.mfdb.height + 255, 1);
		if (buf) {
			wk->screen.shadow.buffer = buf;
			wk->screen.shadow.address = (void *)(((long)buf + 255) & 0xffffff00L);
			wk->screen.shadow.wrap = fast_w_bytes;
		} else {
			access->funcs.error("Can't allocate FastRAM!", 0);
			wk->screen.shadow.buffer = 0;
			wk->screen.shadow.address = 0;
		}
 #ifndef BOTH
		wk->screen.mfdb.address = wk->screen.shadow.address;
 #endif
	}
#endif
	if (!wk->screen.shadow.address)
		driver_name[20] = 0;

	if (debug > 1) {
		access->funcs.puts("  RageII initialization completed!");
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
	return 0;
}

/*
 * 'Deinitialize'
 */
void CDECL clswk(Virtual *vwk)
{
}


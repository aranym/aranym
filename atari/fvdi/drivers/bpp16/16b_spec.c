#include "fvdi.h"
#include "relocate.h"

#include "driver.h"

#if 1
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

char r_16[] = {5, 15, 14, 13, 12, 11};
char g_16[] = {6, 10, 9, 8, 7, 6, 5};
char b_16[] = {5, 4, 3, 2, 1, 0};
char none[] = {0};
#if 0
char red[] = {5, 11, 12, 13, 14, 15};
char green[] = {5, 6, 7, 8, 9, 10};
char blue[] = {5, 0, 1, 2, 3, 4};
char alpha[] = {0};
char genlock[] = {0};
char unused[] = {1, 5};
#endif

Mode mode[1] =
	{{16, CHUNKY | CHECK_PREVIOUS | TRUE_COLOUR, {r_16, g_16, b_16, none, none, none}, 0,  2, 2, 1}};

extern Device device;

char driver_name[] = "ARAnyM Falcon TC 2001-06-11 (shadow)";

extern Driver *me;
extern Access *access;

extern short *loaded_palette;

extern short colours[][3];
extern void CDECL initialize_palette(Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
extern void CDECL c_initialize_palette(Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
extern void *c_set_colours;		/* Just to check if the routine is available */

long wk_extend = 0;

short accel_s = A_SET_PIX | A_GET_PIX | A_MOUSE | A_LINE | A_BLIT | A_FILL | A_EXPAND;
short accel_c = A_SET_PAL | A_GET_COL;

Mode *graphics_mode = &mode[0];

short debug = 0;

short shadow = 0;

#if 0
short cache_img = 0;
short cache_from_screen = 0;

long mode_no = 0;

extern short max_mode;

extern Modes mode_tab[];

char *preset[] = {"640x480x8@60    ", "800x600x8@70    ", "1024x768x8@70   ", "1152x864x8@70   ",
                  "800x600x16@70   ", "1024x768x16@70  ", "1152x864x16@70  ", "1280x1024x16@70 ",
                  "800x600x32@70   ", "1024x768x32@70  "};

long set_mode(const char **ptr);
#endif

Option options[] = {
#if 0
   {"mode",       set_mode,       -1},  /* mode key/<n>/WIDTHxHEIGHTxDEPTH@FREQ */
   {"aesbuf",     set_aesbuf,     -1},  /* aesbuf address, set AES background buffer address */
   {"screen",     set_screen,     -1},  /* screen address, set old screen address */
   {"imgcache",   &cache_img,      1},  /* imgcache, turn on caching of images blitted to the screen */
   {"screencache",&cache_from_screen, 1},  /* screencache, turn on caching of images blitted from the screen */
#endif
   {"shadow",     &shadow,         1},  /* shadow, use a FastRAM buffer */
   {"debug",      &debug,          2}   /* debug, turn on debugging aids */
};


#if 0
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


long set_mode(const char **ptr)
{
	char token[80], *tokenptr;

	if (!(*ptr = access->funcs.skip_space(*ptr)))
		;		/* *********** Error, somehow */
	*ptr = access->funcs.get_token(*ptr, token, 80);

	mode_no = -1;
	if (access->funcs.equal(token, "key"))
		mode_no = access->funcs.misc(0, 0, 0) - '0';		/* Fetch key value */
	else if (!token[1])
		mode_no = access->funcs.atol(token);		/* Single character mode */
	if (mode_no != -1)
		tokenptr = preset[mode_no];
	else
		tokenptr = token;
	mode_no = search_mode(tokenptr);

	if ((mode_no < 0) || (mode_no > max_mode))
		mode_no = 0;

	return 1;
}

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
         case -1:     /* Function call */
            return ((long (*)(const char **))options[i].varfunc)(ptr);
         case 0:      /* Default 1, set to 0 */
            *(short *)options[i].varfunc = 1 - normal;
            return 1;
         case 1:     /* Default 0, set to 1 */
            *(short *)options[i].varfunc = normal;
            return 1;
         case 2:     /* Increase */
            *(short *)options[i].varfunc += -1 + 2 * normal;
            return 1;
         case 3:
           if (!(*ptr = access->funcs.skip_space(*ptr)))
              ;  /* *********** Error, somehow */
            *ptr = access->funcs.get_token(*ptr, token, 80);
           *(short *)options[i].varfunc = token[0];
           return 1;
         }
      }
   }

   return 0;
}


#if 0
void check_token(char *token, const char **ptr)
{
	if (access->funcs.equal(token, "shadow"))
		shadow = 1;
}
#endif


/*
 * Do whatever setup work might be necessary on boot up
 * and which couldn't be done directly while loading.
 * Supplied is the default fVDI virtual workstation.
 */
void CDECL initialize(Virtual *vwk)
{
	Workstation *wk;
	char *buf;
	int old_palette_size;
	Colour *old_palette_colours;
	int fast_w_bytes;
#if 0
	int i;
#endif

#if 0
	debug = access->funcs.misc(0, 1, 0);
#endif
	
	vwk = me->default_vwk;	/* This is what we're interested in */	
	wk = vwk->real_address;

	wk->screen.look_up_table = 0;			/* Was 1 (???)  Shouldn't be needed (graphics_mode) */
	wk->screen.mfdb.standard = 0;
	if (wk->screen.pixel.width > 0)        /* Starts out as screen width */
		wk->screen.pixel.width = (wk->screen.pixel.width * 1000L) / wk->screen.mfdb.width;
	else                                   /*   or fixed DPI (negative) */
		wk->screen.pixel.width = 25400 / -wk->screen.pixel.width;
	if (wk->screen.pixel.height > 0)        /* Starts out as screen height */
		wk->screen.pixel.height = (wk->screen.pixel.height * 1000L) / wk->screen.mfdb.height;
	else                                    /*   or fixed DPI (negative) */
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
	if (*(short *)&c_set_colours != 0x4e75)		/* Look for C... */
		c_initialize_palette(vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);
	else
		initialize_palette(vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);

#if 0
	if ((old_palette_size = wk->screen.palette.size) != 256) {	/* Started from different graphics mode? */
		wk->screen.palette.size = 256;
		old_palette_colours = wk->screen.palette.colours;
		wk->screen.palette.colours = (Colour *)access->funcs.malloc(256 * sizeof(Colour), 3);	/* Assume malloc won't fail. */
		if (wk->screen.palette.colours) {
#if 0
			for(i = 0; i < 256; i += old_palette_size)	/* Fill out entire palette with the old colours */
				access->funcs.copymem(old_palette_colours, &wk->screen.palette.colours[i], old_palette_size * sizeof(Colour));
#else
			if (*(short *)&c_set_colours != 0x4e75)		/* Look for C... */
				c_initialize_palette(vwk, 0, 256, colours, wk->screen.palette.colours);
			else
				initialize_palette(vwk, 0, 256, colours, wk->screen.palette.colours);
#endif
			access->funcs.free(old_palette_colours);		/* Release old (small) palette (a workaround) */
		}
	}
#endif

#if 0
	device.format = 2;	/* Packed pixels */
	device.clut = 2;	/* Software CLUT */
	device.bit_depth = 16;
	device.dummy1 = 0;
	device.colours = 32768;
#endif
	device.byte_width = wk->screen.wrap;
	device.address = wk->screen.mfdb.address;
#if 0
	device.bits.organization = 2;	/* Falcon bit order */
	device.dummy2 = 0;
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

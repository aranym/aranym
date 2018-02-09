/*
 * sdlgui.cpp - a GEM like graphical user interface
 *
 * Copyright (c) 2001 Thomas Huth - taken from his hatari project
 * Copyright (c) 2002-2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "sdlgui.h"
#include "file.h"
#include "tools.h"
#include "host_surface.h"
#include "hostscreen.h"
#include "host.h"
#include "main.h"
#include "maptab.h"
#ifdef OS_darwin
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <cstdlib>
#include <stack>

#include "font.h"
#include "fontsmall.h"
#include "dialog.h"
#include "dlgMain.h"
#include "dlgAlert.h"

#define DEBUG 0
#include "debug.h"

#define sdlscrn		gui_surf

static SDL_Surface *fontgfx=NULL;
static SDL_Surface *fontgfxsmall=NULL;
/* layout of the converted surface */
#define FONTCOLS 128
#define FONTROWS ((FONTCHARS + FONTCOLS - 1) / FONTCOLS)

static std::stack<Dialog *> dlgStack;

/* gui surface */
static HostSurface *gui_hsurf = NULL;
static SDL_Surface *gui_surf = NULL;
static int gui_x = 0, gui_y = 0; /* gui position */

/* current dialog to draw */
static Dialog *gui_dlg = NULL;

static SDL_Color gui_palette[4] = {
	{0,0,0,255},
	{128,128,128,255},
	{192,192,192, 255},
	{255, 255, 255, 255}
};

enum {
	blackc = 0,
	darkgreyc,
	greyc,
	whitec
};

/* the glyph to display for undefined characters */
#define REPLACEMENT_GLYPH 0x6ff

// Stores current dialog coordinates
static SDL_Rect DialogRect = {0, 0, 0, 0};

#if 0
// Used by SDLGui_Get[First|Next]BackgroundRect()
static SDL_Rect BackgroundRect = {0, 0, 0, 0};
static int BackgroundRectCounter;
enum
{
  SG_BCKGND_RECT_BEGIN,
  SG_BCKGND_RECT_TOP,
  SG_BCKGND_RECT_LEFT,
  SG_BCKGND_RECT_RIGHT,
  SG_BCKGND_RECT_BOTTOM,
  SG_BCKGND_RECT_END
};
#endif

/* Special characters: */
/*
 * These are used for radio & checkboxes.
 * Each graphic is composed of two characters,
 * which are encoded as 0x600-0x0607 so they
 * don't conflict with any other character.
 */
static char const SGCHECKBOX_RADIO_NORMAL[4] = { '\330', '\200', '\330', '\201' };
static char const SGCHECKBOX_RADIO_SELECTED[4] = { '\330', '\202', '\330', '\203' };
static char const SGCHECKBOX_NORMAL[4] = { '\330', '\204', '\330', '\205' };
static char const SGCHECKBOX_SELECTED[4] = { '\330', '\206', '\330', '\207' };

/*-----------------------------------------------------------------------*/
/*
  Load an 1 plane XBM into a 8 planes SDL_Surface.
*/
static SDL_Surface *SDLGui_LoadXBM(const Uint8 *srcbits, int width, int height, int form_width)
{
	SDL_Surface *bitmap;
	Uint8 *dstbits;
	int ascii;

	/* Allocate the bitmap */
	bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, FONTCOLS * width, FONTROWS * height, 8, 0, 0, 0, 0);
	if ( bitmap == NULL )
	{
		panicbug("Couldn't allocate bitmap: %s", SDL_GetError());
		return(NULL);
	}

	dstbits = (Uint8 *)bitmap->pixels;

	/* Copy the pixels */
	for (ascii = 0; ascii < FONTCHARS; ascii++)
	{
		int y0 = ascii / FONTCOLS;
		int x0 = ascii % FONTCOLS;
		int x, y;
		
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				int off = ascii * width + x;
				int bit = off & 7;
				dstbits[(y0 * height + y) * bitmap->pitch + x0 * width + x] =
					(srcbits[(off >> 3) + y * form_width] & (0x80 >> bit)) ? 1 : 0;
			}
		}
	}

	return bitmap;
}

/*-----------------------------------------------------------------------*/
/*
  Initialize the GUI.
*/
bool SDLGui_Init()
{
	// Load the font graphics
	char font_filename[256];
	unsigned char font_data_buffer[FONTHEIGHT * FORM_WIDTH];
	getConfFilename("font", font_filename, sizeof(font_filename));
	FILE *f = fopen(font_filename, "rb");
	const unsigned char *font_data = NULL;
	if (f != NULL) {
		if (fread(font_data_buffer, 1, sizeof(font_data_buffer), f) == sizeof(font_data_buffer)) {
			font_data = font_data_buffer;
		}
		fclose(f);
	}
	// If font can't be loaded use the internal one
	if (font_data == NULL) {
		font_data = font_bits;
	}
	
	fontgfx = SDLGui_LoadXBM(font_data, FONTWIDTH, FONTHEIGHT, FORM_WIDTH);
	if (fontgfx == NULL)
	{
		panicbug("Could not create font data");
		panicbug("ARAnyM GUI will not be available");
		return false;
	}
	fontgfxsmall = SDLGui_LoadXBM(fontsmall_bits, FONTSMALLWIDTH, FONTSMALLHEIGHT, FORMSMALL_WIDTH);

	gui_hsurf = host->video->createSurface(76*8+16,25*16+16,8);
	if (!gui_hsurf) {
		panicbug("Could not create surface for GUI");
		panicbug("ARAnyM GUI will not be available");
		return false;
	}

	gui_surf = gui_hsurf->getSdlSurface();

	gui_hsurf->setPalette(gui_palette, 0, 4);

//	gui_hsurf->setParam(HostSurface::SURF_ALPHA, bx_options.opengl.gui_alpha);
//	gui_hsurf->setParam(HostSurface::SURF_USE_ALPHA, 1);

	/* Set font color 0 as transparent */
	SDL_SetColorKey(fontgfx, SDL_SRCCOLORKEY, 0);
	if (fontgfxsmall)
		SDL_SetColorKey(fontgfxsmall, SDL_SRCCOLORKEY, 0);

#if 0 /* for testing */
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetPaletteColors(fontgfx->format->palette, &gui_palette[whitec], 0, 1);
	SDL_SetPaletteColors(fontgfx->format->palette, &gui_palette[blackc], 1, 1);
#else
	SDL_SetColors(fontgfx, &gui_palette[whitec], 0, 1);
	SDL_SetColors(fontgfx, &gui_palette[blackc], 1, 1);
#endif

	SDL_SaveBMP(fontgfx, "aranym-font.bmp");
#endif

	return true;
}


/*-----------------------------------------------------------------------*/
/*
  Uninitialize the GUI.
*/
int SDLGui_UnInit()
{
	if (gui_hsurf) {
		host->video->destroySurface(gui_hsurf);
		gui_hsurf = NULL;
	}

	if (fontgfx)
	{
		SDL_FreeSurface(fontgfx);
		fontgfx = NULL;
	}

	if (fontgfxsmall)
	{
		SDL_FreeSurface(fontgfxsmall);
		fontgfxsmall = NULL;
	}

	return 0;
}


/*-----------------------------------------------------------------------*/
/*
  Compute real coordinates for a given object.
  Note: centers dialog on screen.
*/
static void SDLGui_ObjCoord(SGOBJ *dlg, int objnum, SDL_Rect *rect)
{
  rect->x = dlg[objnum].x * FONTWIDTH;
  rect->y = dlg[objnum].y * FONTHEIGHT;
  if (dlg[objnum].flags & SG_SMALLTEXT)
  {
    rect->w = dlg[objnum].w * FONTSMALLWIDTH;
    rect->h = dlg[objnum].h * FONTSMALLHEIGHT;
    rect->y += 3;
  } else
  {
    rect->w = dlg[objnum].w * FONTWIDTH;
    rect->h = dlg[objnum].h * FONTHEIGHT;
  }
  rect->x += (sdlscrn->w - (dlg[0].w * FONTWIDTH)) / 2;
  rect->y += (sdlscrn->h - (dlg[0].h * FONTHEIGHT)) / 2;
}


/*-----------------------------------------------------------------------*/
/*
  Compute real coordinates for a given object.
  This one takes borders into account and give coordinates as seen by user
*/
void SDLGui_ObjFullCoord(SGOBJ *dlg, int objnum, SDL_Rect *coord)
{
  SDLGui_ObjCoord(dlg, objnum, coord);

  switch (dlg[objnum].type)
  {
    case SGBOX:
    case SGBUTTON:
      {
        // Take border into account
        int border_size;

        if (dlg[objnum].flags & SG_SELECTABLE)
        {
          if (dlg[objnum].flags & SG_DEFAULT)
            border_size = 4;
          else
            border_size = 3;
        }
        else
        {
          if (dlg[objnum].flags & SG_BACKGROUND)
            border_size = 6;
          else
            border_size = 5;
        }

        coord->x -= border_size;
        coord->y -= border_size;
        coord->w += (border_size * 2);
        coord->h += (border_size * 2);
      }
      break;
    case SGEDITFIELD:
      // Allow one more pixel to the right for cursor
      coord->w += 1;
      // There is a line below
      coord->h += 1;
      break;
  }
}


/*-----------------------------------------------------------------------*/
/*
  Refresh display at given coordinates.
  Unlike SDL_UpdateRect() this function can eat coords that goes beyond screen
  boundaries.
  "rect" will be modified to represent the area actually refreshed.
*/
void SDLGui_UpdateRect(SDL_Rect *rect)
{
  if (rect->x < 0)
  {
    rect->w += rect->x;
    rect->x = 0;
  }
  if ((rect->x + rect->w) > sdlscrn->w)
    rect->w = (sdlscrn->w - rect->x);

  if (rect->y < 0)
  {
    rect->h += rect->y;
    rect->y = 0;
  }
  if ((rect->y + rect->h) > sdlscrn->h)
    rect->h = (sdlscrn->h - rect->y);

  if ((rect->w > 0) && (rect->h > 0))
  {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    host->video->refreshScreenFromSurface(sdlscrn);
#else
    SDL_UpdateRects(sdlscrn, 1, rect);
#endif
  } else
  {
    rect->x = 0;
    rect->y = 0;
    rect->w = 0;
    rect->h = 0;
  }
}


/*-----------------------------------------------------------------------*/
/*
  Maps an SDL_Color to the screen format.
*/
Uint32 SDLGui_MapColor(int color)
{
  return SDL_MapRGB(sdlscrn->format,
  	gui_palette[color].r, gui_palette[color].g, gui_palette[color].b);
}


/*-----------------------------------------------------------------------*/
/*
  Refresh display to reflect an object change.
*/
void SDLGui_RefreshObj(SGOBJ *dlg, int objnum)
{
  SDL_Rect coord;

  SDLGui_ObjFullCoord(dlg, objnum, &coord);

  SDLGui_UpdateRect(&coord);
}


static unsigned short *host_to_atari(const char *src)
{
#ifdef OS_darwin
	/* MacOSX uses decomposed strings, normalize them first */
	CFMutableStringRef theString = CFStringCreateMutable(NULL, 0);
	CFStringAppendCString(theString, src, kCFStringEncodingUTF8);
	CFStringNormalize(theString, kCFStringNormalizationFormC);
	UniChar ch;
	CFIndex idx;
	CFIndex len = CFStringGetLength(theString);
	unsigned short *dst, *res;
	size_t count = strlen(src);
	
	res = (unsigned short *)malloc((count + 1) * sizeof(*res));
	dst = res;
	idx = 0;
	while (idx < len )
	{
		ch = CFStringGetCharacterAtIndex(theString, idx);
		if (ch >= FONTCHARS || (ch >= 0x80 && ch < 0xa0))
		{
			ch = REPLACEMENT_GLYPH;
		} else if (ch >= FONTCHARS)
		{
			ch = utf16_to_atari[ch];
			if (ch >= FONTCHARS)
				ch = REPLACEMENT_GLYPH;
		}
		*dst++ = ch;
		idx++;
	}
	*dst = '\0';
	CFRelease(theString);
#else
	unsigned short ch;
	size_t bytes;
	unsigned short *dst, *res;
	size_t count = strlen(src);
	
	res = (unsigned short *)malloc((count + 1) * sizeof(*res));
	dst = res;
	
	while (*src)
	{
		ch = (unsigned char) *src;
		if (ch < 0x80)
		{
			bytes = 1;
		} else if ((ch & 0xe0) == 0xc0)
		{
			ch = ((ch & 0x1f) << 6) | (src[1] & 0x3f);
			bytes = 2;
		} else
		{
			ch = ((((ch & 0x0f) << 6) | (src[1] & 0x3f)) << 6) | (src[2] & 0x3f);
			bytes = 3;
		}
		if (ch >= FONTCHARS || (ch >= 0x80 && ch < 0xa0))
		{
			ch = REPLACEMENT_GLYPH;
		} else if (ch >= 0x100)
		{
			ch = utf16_to_atari[ch];
			if (ch >= FONTCHARS)
				ch = REPLACEMENT_GLYPH;
		}
		*dst++ = ch;
		src += bytes;
	}
	*dst = '\0';
#endif
	return res;
}

/*-----------------------------------------------------------------------*/
/*
  Draw a text string.
*/
void SDLGui_Text(int x, int y, const char *txt, int col)
{
  int i;
  unsigned short c;
  SDL_Rect sr, dr;
  unsigned short *conv;
  
#if SDL_VERSION_ATLEAST(2, 0, 0)
  SDL_SetPaletteColors(fontgfx->format->palette, &gui_palette[col], 1, 1);
#else
  SDL_SetColors(fontgfx, &gui_palette[col], 1, 1);
#endif

  conv = host_to_atari(txt);
  for (i = 0; conv[i] != 0; i++)
  {
    c = conv[i];
    sr.x = FONTWIDTH * (c % FONTCOLS);
    sr.y = FONTHEIGHT * (c / FONTCOLS);
    sr.w = FONTWIDTH;
    sr.h = FONTHEIGHT;

    dr.x = x + (FONTWIDTH * i);
    dr.y = y;
    dr.w = FONTWIDTH;
    dr.h = FONTHEIGHT;

    SDL_BlitSurface(fontgfx, &sr, sdlscrn, &dr);
  }
  free(conv);
}

/*-----------------------------------------------------------------------*/
/*
  Draw a text string, using the smaller font.
*/
void SDLGui_TextSmall(int x, int y, const char *txt, int col)
{
  int i;
  unsigned short c;
  SDL_Rect sr, dr;
  unsigned short *conv;
  
  if (fontgfxsmall == NULL)
  {
    SDLGui_Text(x, y, txt, col);
    return;
  }
#if SDL_VERSION_ATLEAST(2, 0, 0)
  SDL_SetPaletteColors(fontgfxsmall->format->palette, &gui_palette[col], 1, 1);
#else
  SDL_SetColors(fontgfxsmall, &gui_palette[col], 1, 1);
#endif

  conv = host_to_atari(txt);
  for (i = 0; conv[i] != 0; i++)
  {
    c = conv[i];
    sr.x = FONTSMALLWIDTH * (c % FONTCOLS);
    sr.y = FONTSMALLHEIGHT * (c / FONTCOLS);
    sr.w = FONTSMALLWIDTH;
    sr.h = FONTSMALLHEIGHT;

    dr.x = x + (FONTSMALLWIDTH * i);
    dr.y = y;
    dr.w = FONTSMALLWIDTH;
    dr.h = FONTSMALLHEIGHT;

    SDL_BlitSurface(fontgfxsmall, &sr, sdlscrn, &dr);
  }
  free(conv);
}

/*-----------------------------------------------------------------------*/
/*
 * return the number of characters that are displayed
 */
int SDLGui_TextLen(const char *txt)
{
  unsigned short *conv;
  int i;

  conv = host_to_atari(txt);
  for (i = 0; conv[i] != 0; i++)
    ;
  free(conv);
  return i;
}

/*-----------------------------------------------------------------------*/
/*
 * return the number of up to a given character index
 */
int SDLGui_ByteLen(const char *txt, int pos)
{
	size_t bytes;
	const char *src = txt;
	unsigned char ch;
	
	while (pos > 0 && *src)
	{
		ch = *src;
		if (ch < 0x80)
		{
			bytes = 1;
		} else if ((ch & 0xe0) == 0xc0 && src[1])
		{
			bytes = 2;
		} else if (src[1] && src[2])
		{
			bytes = 3;
		} else
		{
			bytes = 1;
		}
		src += bytes;
		pos--;
	}
	
	return (int)(src - txt);
}

/*-----------------------------------------------------------------------*/
/*
  Draw a dialog text object.
*/
void SDLGui_DrawText(SGOBJ *tdlg, int objnum)
{
  SDL_Rect coord;
  int textc, backgroundc;

  if (tdlg[objnum].state & SG_SELECTED)
  {
    textc       = whitec;
    backgroundc = darkgreyc;
  }
  else if (tdlg[objnum].state & SG_DISABLED)
  {
    textc       = darkgreyc;
    backgroundc = greyc;
  }
  else
  {
    textc       = blackc;
    backgroundc = greyc;
  }
  
  SDLGui_ObjCoord(tdlg, objnum, &coord);
  SDL_FillRect(sdlscrn, &coord, SDLGui_MapColor(backgroundc));
  SDLGui_Text(coord.x, coord.y, tdlg[objnum].txt, textc);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog text object.
*/
void SDLGui_DrawTextSmall(SGOBJ *tdlg, int objnum)
{
  SDL_Rect coord;
  int textc, backgroundc;

  if (tdlg[objnum].state & SG_SELECTED)
  {
    textc       = whitec;
    backgroundc = darkgreyc;
  }
  else if (tdlg[objnum].state & SG_DISABLED)
  {
    textc       = darkgreyc;
    backgroundc = greyc;
  }
  else
  {
    textc       = blackc;
    backgroundc = greyc;
  }
  
  SDLGui_ObjCoord(tdlg, objnum, &coord);
  SDL_FillRect(sdlscrn, &coord, SDLGui_MapColor(backgroundc));
  SDLGui_TextSmall(coord.x, coord.y, tdlg[objnum].txt, textc);
}


/*-----------------------------------------------------------------------*/
/*
  Draw an edit field object.
*/
void SDLGui_DrawEditField(SGOBJ *edlg, int objnum)
{
  SDL_Rect coord;
  int textc;

  if (edlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  SDLGui_ObjCoord(edlg, objnum, &coord);
  coord.w += 1;
  SDL_FillRect(sdlscrn, &coord, SDLGui_MapColor(greyc));
  SDLGui_Text(coord.x, coord.y, edlg[objnum].txt, textc);

  // Draw a line below.
  coord.y = coord.y + coord.h;
  coord.h = 1;
  SDL_FillRect(sdlscrn, &coord, SDLGui_MapColor(darkgreyc));
}


/*-----------------------------------------------------------------------*/
/*
  Draw or erase cursor.
*/
void SDLGui_DrawCursor(SGOBJ *dlg, cursor_state *cursor)
{
  if (cursor->object != -1)
  {
    SDL_Rect coord;
    int cursorc;

    SDLGui_DrawEditField(dlg, cursor->object);

    if (cursor->blink_state)
      cursorc = blackc;
    else
      cursorc = greyc;

    SDLGui_ObjCoord(dlg, cursor->object, &coord);
    coord.x += (cursor->position * FONTWIDTH);
    coord.w = 1;
    SDL_FillRect(sdlscrn, &coord, SDLGui_MapColor(cursorc));

    SDLGui_RefreshObj(dlg, cursor->object);
  }
}


/*-----------------------------------------------------------------------*/
/*
  Draw a 3D effect around a given rectangle.
  Rectangle is updated to the full size of the new object.
*/
void SDLGui_Draw3DAround(SDL_Rect *coord, int upleftc, int downrightc, int cornerc, int width)
{
  SDL_Rect rect;
  int i;
  Uint32 upleftcol    = SDLGui_MapColor(upleftc);
  Uint32 downrightcol = SDLGui_MapColor(downrightc);
  Uint32 cornercol    = SDLGui_MapColor(cornerc);

  for ( i = 1 ; i <= width ; i++)
  {
    rect.x = coord->x - i;
    rect.y = coord->y - i;
    rect.w = coord->w + (i * 2) - 1;
    rect.h = 1;
    SDL_FillRect(sdlscrn, &rect, upleftcol);

    rect.x = coord->x - i;
    rect.y = coord->y - i;
    rect.w = 1;
    rect.h = coord->h + (i * 2) - 1;
    SDL_FillRect(sdlscrn, &rect, upleftcol);

    rect.x = coord->x - i + 1;
    rect.y = coord->y + coord->h - 1 + i;
    rect.w = coord->w + (i * 2) - 1;
    rect.h = 1;
    SDL_FillRect(sdlscrn, &rect, downrightcol);

    rect.x = coord->x + coord->w - 1 + i;
    rect.y = coord->y - i + 1;
    rect.w = 1;
    rect.h = coord->h + (i * 2) - 1;
    SDL_FillRect(sdlscrn, &rect, downrightcol);

    rect.x = coord->x + coord->w + i - 1;
    rect.y = coord->y - i;
    rect.w = 1;
    rect.h = 1;
    SDL_FillRect(sdlscrn, &rect, cornercol);

    rect.x = coord->x - i;
    rect.y = coord->y + coord->h + i - 1;
    rect.w = 1;
    rect.h = 1;
    SDL_FillRect(sdlscrn, &rect, cornercol);
  }

  coord->x -= width;
  coord->y -= width;
  coord->w += (width * 2);
  coord->h += (width * 2);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a colored box around a given rectangle.
  Rectangle is updated to the full size of the new object.
*/
void SDLGui_DrawBoxAround(SDL_Rect *coord, int color, int width)
{
  SDL_Rect rect;
  Uint32 col = SDLGui_MapColor(color);

  rect.x = coord->x - width;
  rect.y = coord->y - width;
  rect.w = coord->w + (width * 2);
  rect.h = width;
  SDL_FillRect(sdlscrn, &rect, col);

  rect.x = coord->x - width;
  rect.y = coord->y - width;
  rect.w = width;
  rect.h = coord->h + (width * 2);
  SDL_FillRect(sdlscrn, &rect, col);

  rect.x = coord->x + coord->w;
  rect.y = coord->y - width;
  rect.w = width;
  rect.h = coord->h + (width * 2);
  SDL_FillRect(sdlscrn, &rect, col);

  rect.x = coord->x - width;
  rect.y = coord->y + coord->h;
  rect.w = coord->w + (width * 2);
  rect.h = width;
  SDL_FillRect(sdlscrn, &rect, col);

  coord->x -= width;
  coord->y -= width;
  coord->w += (width * 2);
  coord->h += (width * 2);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a 3D box with given attributes.
*/
void SDLGui_Draw3DBox(SDL_Rect *coord,
                      int backgroundc,
                      int inboxc,
                      int upleftc,
                      int downrightc,
                      int outboxc,
                      int widthbackground,
                      int widthinbox,
                      int width3D1,
                      int width3D2,
                      int widthoutbox)
{
  SDL_Rect rect;

  // Draw background
  rect.x = coord->x - widthbackground;
  rect.y = coord->y - widthbackground*2;
  rect.w = coord->w + (widthbackground * 2);
  rect.h = coord->h + (widthbackground*2 * 2);
  SDL_FillRect(sdlscrn, &rect, SDLGui_MapColor(backgroundc));

  // Update coords
  coord->x -= widthbackground;
  coord->y -= widthbackground*5/2;
  coord->w += (widthbackground * 2);
  coord->h += (widthbackground*5/2 * 2);

  if (widthinbox > 0)
    SDLGui_DrawBoxAround(coord, inboxc, widthinbox);

  if (width3D1 > 0)
    SDLGui_Draw3DAround(coord, upleftc, downrightc, backgroundc, width3D1);

  if (width3D2 > 0)
    SDLGui_Draw3DAround(coord, downrightc, upleftc, backgroundc, width3D2);

  if (widthoutbox > 0)
    SDLGui_DrawBoxAround(coord, outboxc, widthoutbox);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog box object.
*/
void SDLGui_DrawBox(SGOBJ *bdlg, int objnum)
{
  SDL_Rect coord;
  int my_blackc, upleftc, downrightc;

  SDLGui_ObjCoord(bdlg, objnum, &coord);

  // Modify box drawing according to object state
  if (bdlg[objnum].state & SG_DISABLED)
    my_blackc = darkgreyc;
  else
    my_blackc = blackc;

  if (bdlg[objnum].state & SG_SELECTED)
  {
    upleftc    = darkgreyc;
    downrightc = whitec;
  }
  else
  {
    upleftc    = whitec;
    downrightc = darkgreyc;
  }

  // Draw box according to object flags
  switch (bdlg[objnum].flags & (SG_SELECTABLE | SG_DEFAULT | SG_BACKGROUND))
  {
    case (SG_SELECTABLE | SG_DEFAULT | SG_BACKGROUND):
    case (SG_SELECTABLE | SG_DEFAULT):
      SDLGui_Draw3DBox(&coord,
                       greyc, 0, upleftc, downrightc, my_blackc,
                       1, 0, 1, 0, 2);
      break;
    case (SG_SELECTABLE | SG_BACKGROUND):
    case SG_SELECTABLE:
      SDLGui_Draw3DBox(&coord,
                       greyc, 0, upleftc, downrightc, my_blackc,
                       1, 0, 1, 0, 1);
      break;
    case (SG_DEFAULT | SG_BACKGROUND):
    case SG_BACKGROUND:
      SDLGui_Draw3DBox(&coord,
                       greyc, my_blackc, upleftc, downrightc, darkgreyc,
                       0, 2, 3, 0, 1);
      break;
    case SG_DEFAULT:
    case 0:
      SDLGui_Draw3DBox(&coord,
                       greyc, 0, upleftc, downrightc, 0,
                       3, 0, 1, 1, 0);
      break;
  }
}


/*-----------------------------------------------------------------------*/
/*
  Draw a normal button.
*/
void SDLGui_DrawButton(SGOBJ *bdlg, int objnum)
{
  SDL_Rect coord;
  int x, y;
  int textc;

  SDLGui_ObjCoord(bdlg, objnum, &coord);

  x = coord.x + ((coord.w - (SDLGui_TextLen(bdlg[objnum].txt) * FONTWIDTH)) / 2);
  y = coord.y + ((coord.h - FONTHEIGHT) / 2);

  if (bdlg[objnum].state & SG_SELECTED)
  {
    x += 1;
    y += 1;
  }

  if (bdlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  SDLGui_DrawBox(bdlg, objnum);
  SDLGui_Text(x, y, bdlg[objnum].txt, textc);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a normal button.
*/
void SDLGui_DrawButtonSmall(SGOBJ *bdlg, int objnum)
{
  SDL_Rect coord;
  int x, y;
  int textc;

  SDLGui_ObjCoord(bdlg, objnum, &coord);

  x = coord.x + ((coord.w - (SDLGui_TextLen(bdlg[objnum].txt) * FONTSMALLWIDTH)) / 2);
  y = coord.y + ((coord.h - FONTSMALLHEIGHT) / 2);

  if (bdlg[objnum].state & SG_SELECTED)
  {
    x += 1;
    y += 1;
  }

  if (bdlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  SDLGui_DrawBox(bdlg, objnum);
  SDLGui_TextSmall(x, y, bdlg[objnum].txt, textc);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog check box object state.
*/
void SDLGui_DrawCheckBoxState(SGOBJ *cdlg, int objnum)
{
  Uint32 grey = SDLGui_MapColor(greyc);
  SDL_Rect coord;
  char str[5];
  int textc;

  SDLGui_ObjCoord(cdlg, objnum, &coord);

  if (cdlg[objnum].flags & SG_RADIO)
  {
    if (cdlg[objnum].state & SG_SELECTED) {
      str[0]=SGCHECKBOX_RADIO_SELECTED[0];
      str[1]=SGCHECKBOX_RADIO_SELECTED[1];
      str[2]=SGCHECKBOX_RADIO_SELECTED[2];
      str[3]=SGCHECKBOX_RADIO_SELECTED[3];
    }
    else {
      str[0]=SGCHECKBOX_RADIO_NORMAL[0];
      str[1]=SGCHECKBOX_RADIO_NORMAL[1];
      str[2]=SGCHECKBOX_RADIO_NORMAL[2];
      str[3]=SGCHECKBOX_RADIO_NORMAL[3];
    }
  }
  else
  {
    if (cdlg[objnum].state & SG_SELECTED) {
      str[0]=SGCHECKBOX_SELECTED[0];
      str[1]=SGCHECKBOX_SELECTED[1];
      str[2]=SGCHECKBOX_SELECTED[2];
      str[3]=SGCHECKBOX_SELECTED[3];
    }
    else {
      str[0]=SGCHECKBOX_NORMAL[0];
      str[1]=SGCHECKBOX_NORMAL[1];
      str[2]=SGCHECKBOX_NORMAL[2];
      str[3]=SGCHECKBOX_NORMAL[3];
    }
  }
  str[4]='\0';

  if (cdlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  coord.w = FONTWIDTH*2;
  coord.h = FONTHEIGHT;

  if (cdlg[objnum].flags & SG_BUTTON_RIGHT)
    coord.x += ((SDLGui_TextLen(cdlg[objnum].txt) + 1) * FONTWIDTH);

  SDL_FillRect(sdlscrn, &coord, grey);
  SDLGui_Text(coord.x, coord.y, str, textc);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog check box object.
*/
void SDLGui_DrawCheckBox(SGOBJ *cdlg, int objnum)
{
  SDL_Rect coord;
  int textc;

  SDLGui_ObjCoord(cdlg, objnum, &coord);

  if (!(cdlg[objnum].flags&SG_BUTTON_RIGHT))
    coord.x += (FONTWIDTH * 3); // 2 chars for the box plus 1 space

  if (cdlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  SDLGui_Text(coord.x, coord.y, cdlg[objnum].txt, textc);
  SDLGui_DrawCheckBoxState(cdlg, objnum);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog popup button object.
*/
void SDLGui_DrawPopupButton(SGOBJ *pdlg, int objnum)
{
  SDL_Rect coord;
  static char const downstr[2] = { SGARROWDOWN, 0 };
  int textc;

  if (pdlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  SDLGui_DrawBox(pdlg, objnum);

  SDLGui_ObjCoord(pdlg, objnum, &coord);

  SDLGui_Text(coord.x, coord.y, pdlg[objnum].txt, textc);
  SDLGui_Text(coord.x+coord.w-FONTWIDTH, coord.y, downstr, textc);
}


/*-----------------------------------------------------------------------*/
/*
  Draw an object.
*/
void SDLGui_DrawObject(SGOBJ *dlg, int objnum)
{
  switch (dlg[objnum].type)
  {
    case SGBOX:
      SDLGui_DrawBox(dlg, objnum);
      break;
    case SGTEXT:
      if (dlg[objnum].flags & SG_SMALLTEXT)
        SDLGui_DrawTextSmall(dlg, objnum);
      else
        SDLGui_DrawText(dlg, objnum);
      break;
    case SGEDITFIELD:
      SDLGui_DrawEditField(dlg, objnum);
      break;
    case SGBUTTON:
      if (dlg[objnum].flags & SG_SMALLTEXT)
        SDLGui_DrawButtonSmall(dlg, objnum);
      else
        SDLGui_DrawButton(dlg, objnum);
      break;
    case SGCHECKBOX:
    case SGRADIOBUT:
      SDLGui_DrawCheckBox(dlg, objnum);
      break;
    case SGPOPUP:
      SDLGui_DrawPopupButton(dlg, objnum);
      break;
  }
}


/*-----------------------------------------------------------------------*/
/*
  Draw a whole dialog.
*/
void SDLGui_DrawDialog(SGOBJ *dlg)
{
  int i;

  // Store dialog coordinates
  SDLGui_ObjFullCoord(dlg, 0, &DialogRect);

  for (i = 0 ; dlg[i].type != -1 ; i++)
  {
    if (dlg[i].state & SG_HIDDEN) continue;
    SDLGui_DrawObject(dlg, i);
  }
  SDLGui_RefreshObj(dlg, 0);
}

void SDLGui_setGuiPos(int guix, int guiy)
{
	gui_x = guix;
	gui_y = guiy;
}

HostSurface *SDLGui_getSurface(void)
{
	if (gui_dlg) {
		/* Process a TOUCHEXIT object ? */
		if (gui_dlg->isTouchExitPressed()) {
			/*int retval =*/ gui_dlg->processDialog();
		}

		/* Blink cursor ? */
		cursor_state *cursor = gui_dlg->getCursor();
		Uint32 cur_ticks = SDL_GetTicks();
		if (cur_ticks-cursor->blink_counter >= 500) {
			cursor->blink_counter = cur_ticks;
			cursor->blink_state = !cursor->blink_state;
		}

		/* Redraw updated dialog */
		SDLGui_DrawDialog(gui_dlg->getDialog());
		SDLGui_DrawCursor(gui_dlg->getDialog(), cursor);
	}

	if (gui_hsurf) {
		gui_hsurf->setDirtyRect(0,0,
			gui_hsurf->getWidth(),gui_hsurf->getHeight());
	}

	return gui_hsurf;
}

/*-----------------------------------------------------------------------*/
/*
  Search default object in a dialog.
*/
int SDLGui_FindDefaultObj(SGOBJ *dlg)
{
  int i = 0;

  while (dlg[i].type != -1)
  {
    if (dlg[i].flags & SG_DEFAULT)
      return i;
    i++;
  }

  return -1;
}


/*-----------------------------------------------------------------------*/
/*
  Search an object at given coordinates.
*/
int SDLGui_FindObj(SGOBJ *dlg, int fx, int fy)
{
  SDL_Rect coord;
  int end, i;
  int ob = -1;
  
  // Search end object in dialog
  i = 0;
  while (dlg[i++].type != -1) ;
  end = i;

  // Now check each object
  for (i = end-1 ; i >= 0 ; i--)
  {
    SDLGui_ObjFullCoord(dlg, i, &coord);

    if(fx >= coord.x &&
       fy >= coord.y &&
       fx < (coord.x + coord.w) &&
       fy < (coord.y + coord.h))
    {
      if (dlg[i].state & (SG_HIDDEN | SG_DISABLED)) continue;
      ob = i;
      break;
    }
  }

  return ob;
}


/*-----------------------------------------------------------------------*/
/*
  A radio object has been selected. Let's deselect any other in his group.
*/
void SDLGui_SelectRadioObject(SGOBJ *dlg, int clicked_obj)
{
  int obj;

  // Find first radio object in this group
  obj = clicked_obj;
  while (dlg[--obj].flags & SG_RADIO) ;

  // Update state
  while (dlg[++obj].flags & SG_RADIO)
  {
    // This code scan every object in the group. This allows to solve cases
    // where multiple objects were selected in the group by clicking one.
    if (obj != clicked_obj)
    {
		// Deselect this radio button
		SDLGui_DeselectAndRedraw(dlg, obj);
    }
  }
}


/*-----------------------------------------------------------------------*/
/*
  Update clicked object state depending on given mouse coordinates.
  Returns true if the mouse is over the object, false otherwise.
*/
bool SDLGui_UpdateObjState(SGOBJ *dlg, int clicked_obj, int original_state,
                           int x, int y)
{
  int obj;

  obj = SDLGui_FindObj(dlg, x, y);

  // Special case : user clicked on an already selected radio object
  // do not modify its state.
  // We handle it here because it allows to exit if the object is SG_EXIT or
  // SG_TOUCHEXIT without any additional test.
  if ((dlg[clicked_obj].flags & SG_RADIO) && (original_state & SG_SELECTED))
    return (obj == clicked_obj);

  if (((obj != clicked_obj) &&
       (dlg[clicked_obj].state != original_state)) ||
      ((obj == clicked_obj) &&
       (dlg[clicked_obj].state == original_state)))
  {
    if (dlg[clicked_obj].flags & SG_SELECTABLE)
    {
      dlg[clicked_obj].state ^= SG_SELECTED;
      SDLGui_DrawObject(dlg, clicked_obj);
      SDLGui_RefreshObj(dlg, clicked_obj);
    }
  }

  return (obj == clicked_obj);
}

void SDLGui_DeselectAndRedraw(SGOBJ *dlg, int obj)
{
    if (dlg[obj].flags & SG_SELECTABLE)
    {
		dlg[obj].state &= ~SG_SELECTED;
		SDLGui_DrawObject(dlg, obj);
		SDLGui_RefreshObj(dlg, obj);
    }
}

/*-----------------------------------------------------------------------*/
/*
  Search edit field in a dialog.
*/
int SDLGui_FindEditField(SGOBJ *dlg, int objnum, int mode)
{
  int i, j;

  switch (mode)
  {
    case SG_FIRST_EDITFIELD:
      i = 0;
      while (dlg[i].type != -1)
      {
        if ((dlg[i].type == SGEDITFIELD) &&
            ((dlg[i].state & (SG_HIDDEN | SG_DISABLED)) == 0))
          return i;
        i++;
      }
      break;

    case SG_PREVIOUS_EDITFIELD:
      i = objnum - 1;
      while (i >= 0)
      {
        if ((dlg[i].type == SGEDITFIELD) &&
            ((dlg[i].state & (SG_HIDDEN | SG_DISABLED)) == 0))
          return i;
        i--;
      }
      break;

    case SG_NEXT_EDITFIELD:
      i = objnum + 1;
      while (dlg[i].type != -1)
      {
        if ((dlg[i].type == SGEDITFIELD) &&
            ((dlg[i].state & (SG_HIDDEN | SG_DISABLED)) == 0))
          return i;
        i++;
      }
      break;

    case SG_LAST_EDITFIELD:
      i = objnum + 1;
      j = -1;
      while (dlg[i].type != -1)
      {
        if ((dlg[i].type == SGEDITFIELD) &&
            ((dlg[i].state & (SG_HIDDEN | SG_DISABLED)) == 0))
          j = i;
        i++;
      }
      if (j != -1)
        return j;
      break;
  }

  return objnum;
}


/*-----------------------------------------------------------------------*/
/*
  Move cursor to another edit field.
*/
void SDLGui_MoveCursor(SGOBJ *dlg, cursor_state *cursor, int mode)
{
  int new_object;

  new_object = SDLGui_FindEditField(dlg, cursor->object, mode);

  if (new_object != cursor->object)
  {
    /* Erase old cursor */
    cursor->blink_state = false;
    SDLGui_DrawCursor(dlg, cursor);

    cursor->object = new_object;
    cursor->position = SDLGui_TextLen(dlg[new_object].txt);
  }
  else
  {
    /* We stay in the same field */
    /* Move cursor to begin or end of text depending on mode */
    switch (mode)
    {
      case SG_FIRST_EDITFIELD:
      case SG_PREVIOUS_EDITFIELD:
        cursor->position = 0;
        break;

      case SG_NEXT_EDITFIELD:
      case SG_LAST_EDITFIELD:
        cursor->position = SDLGui_TextLen(dlg[new_object].txt);
        break;
    }
  }
}

/* Deselect all buttons */
void SDLGui_DeselectButtons(SGOBJ *dlg)
{
	int i = 0;
	while (dlg[i].type != -1) {
		if (dlg[i].type == SGBUTTON) {
			dlg[i].state &= ~SG_SELECTED;
		}
		++i;
	}
}

/*-----------------------------------------------------------------------*/
/*
  Handle mouse clicks on edit fields.
*/
void SDLGui_ClickEditField(SGOBJ *dlg, cursor_state *cursor, int clicked_obj, int x)
{
  SDL_Rect coord;
  int i, j;

  /* Erase old cursor */
  cursor->blink_state = false;
  SDLGui_DrawCursor(dlg, cursor);

  SDLGui_ObjFullCoord(dlg, clicked_obj, &coord);
  i = (x - coord.x + (FONTWIDTH / 2)) / FONTWIDTH;
  j = SDLGui_TextLen(dlg[clicked_obj].txt);

  cursor->object = clicked_obj;
  cursor->position = MIN(i, j);
  cursor->blink_state = true;
  cursor->blink_counter = 0;
  SDLGui_DrawCursor(dlg, cursor);
}

#if 0

/*-----------------------------------------------------------------------*/
/*
  Used to update screen while GUI is opened. Return a list of rectangles that
  covers the screen without overlaping the current dialog.
*/
SDL_Rect *SDLGui_GetFirstBackgroundRect(void)
{
  // Reset counter...
  BackgroundRectCounter = SG_BCKGND_RECT_BEGIN;
  // And returns first rectangle
  return SDLGui_GetNextBackgroundRect();
}


/*-----------------------------------------------------------------------*/
/*
  Returns next rectangle to be redrawn to update screen or NULL if we reached
  the end of the list.
  This code is "flying dialog" ready :)
  It will need some updating if we implement popup buttons handled by sdlgui,
  as the popup could be higher than the root box...
  I used some recursivity here to simplify the code.
*/
SDL_Rect *SDLGui_GetNextBackgroundRect(void)
{
  SDL_Rect *return_rect = NULL;

  switch (BackgroundRectCounter)
  {
    case SG_BCKGND_RECT_END:
      // Nothing to do : return_rect is already initialized to NULL.
      break;

    case SG_BCKGND_RECT_BEGIN:
      if (DialogRect.w == 0)
      {
        // The dialog is not drawn yet...
        // Let's redraw the full screen.
      	BackgroundRect.x = 0;
      	BackgroundRect.y = 0;
      	BackgroundRect.w = sdlscrn->w;
      	BackgroundRect.h = sdlscrn->h;
        return_rect = &BackgroundRect;
        // We reached the end of the list.
        BackgroundRectCounter = SG_BCKGND_RECT_END;
      }
      else
      {
        BackgroundRectCounter = SG_BCKGND_RECT_TOP;
        return_rect = SDLGui_GetNextBackgroundRect();
      }
      break;

    case SG_BCKGND_RECT_TOP:
      BackgroundRectCounter = SG_BCKGND_RECT_LEFT;
      if (DialogRect.y > 0)
      {
      	BackgroundRect.x = 0;
      	BackgroundRect.y = 0;
      	BackgroundRect.w = sdlscrn->w;
      	BackgroundRect.h = DialogRect.y;
        return_rect = &BackgroundRect;
      }
      else
        return_rect = SDLGui_GetNextBackgroundRect();
      break;

    case SG_BCKGND_RECT_LEFT:
      BackgroundRectCounter = SG_BCKGND_RECT_RIGHT;
      if (DialogRect.x > 0)
      {
        BackgroundRect.x = 0;
        BackgroundRect.y = (DialogRect.y > 0) ? DialogRect.y : 0;
        BackgroundRect.w = DialogRect.x;
        BackgroundRect.h =
          ((DialogRect.y + DialogRect.h) < (int)sdlscrn->h) ?
          (DialogRect.h + DialogRect.y - BackgroundRect.y) :
          (sdlscrn->h - DialogRect.y);
        return_rect = &BackgroundRect;
      }
      else
        return_rect = SDLGui_GetNextBackgroundRect();
      break;

    case SG_BCKGND_RECT_RIGHT:
      BackgroundRectCounter = SG_BCKGND_RECT_BOTTOM;
      if ((DialogRect.x + DialogRect.w) < (int)sdlscrn->w)
      {
        BackgroundRect.x = DialogRect.x + DialogRect.w;
        BackgroundRect.y = (DialogRect.y > 0) ? DialogRect.y : 0;
        BackgroundRect.w = sdlscrn->w - (DialogRect.x + DialogRect.w);
        BackgroundRect.h =
          ((DialogRect.y + DialogRect.h) < (int)sdlscrn->w) ?
          (DialogRect.h + DialogRect.y - BackgroundRect.y) :
          (sdlscrn->h - DialogRect.y);
        return_rect = &BackgroundRect;
      }
      else
        return_rect = SDLGui_GetNextBackgroundRect();
      break;

    case SG_BCKGND_RECT_BOTTOM:
      BackgroundRectCounter = SG_BCKGND_RECT_END;
      if ((DialogRect.y + DialogRect.h) < (int)sdlscrn->h)
      {
        // Bottom
        BackgroundRect.x = 0;
        BackgroundRect.y = DialogRect.y + DialogRect.h;
        BackgroundRect.w = sdlscrn->w;
        BackgroundRect.h = sdlscrn->h - (DialogRect.y + DialogRect.h);
        return_rect = &BackgroundRect;
      }
      else
        return_rect = SDLGui_GetNextBackgroundRect();
      break;
  }

  return return_rect;
}

#endif

/* Process event for a dialog */
int SDLGui_DoEvent(const SDL_Event &event)
{
	int retval = Dialog::GUI_CONTINUE;
	int num_dialogs=1;	/* Number of processDialog to call */

	if (gui_dlg) {
		gui_dlg->idle();

		switch(event.type) {
			case SDL_KEYDOWN:
				gui_dlg->keyPress(event);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				gui_dlg->mouseClick(event, gui_x, gui_y);
				break;
			case SDL_QUIT:
				return Dialog::GUI_CLOSE;
			case SDL_MOUSEMOTION:
				return Dialog::GUI_CONTINUE;
		}

		while (num_dialogs-->0) {
			retval = gui_dlg->processDialog();

			if (retval != Dialog::GUI_CONTINUE) {
				Dialog *prev_dlg = NULL;
				Dialog *curr_dlg = dlgStack.top();
				dlgStack.pop();
				if (!dlgStack.empty()) {
					prev_dlg = dlgStack.top();

					/* Process result of called dialog */
					prev_dlg->processResult();

					/* Continue displaying GUI with previous dialog */
					retval = Dialog::GUI_CONTINUE;
				}

				/* Close current dialog */
				delete curr_dlg;

				/* Set dialog to previous one */
				if (gui_dlg == curr_dlg)
					gui_dlg = prev_dlg;
				if (gui_dlg) {
					gui_dlg->init();
					++num_dialogs; /* Need to process result in the caller dialog */
				}
			}
		}
	}

	return retval;
}

/* Open a dialog */
void SDLGui_Open(Dialog *new_dlg)
{
	if (new_dlg==NULL) {
		/* Open main as default */
		new_dlg = DlgMainOpen();
		if (startupAlert)
		{
			dlgStack.push(new_dlg);
			new_dlg = DlgAlertOpen(startupAlert, ALERT_OK);
			free(startupAlert);
			startupAlert = NULL;
		}
	}

	dlgStack.push(new_dlg);

	gui_dlg = dlgStack.top();
	gui_dlg->init();
	gui_dlg->idle();	
}

void SDLGui_Close(void)
{
	/* Empty the dialog stack */
	while (!dlgStack.empty()) {
		Dialog *dlg = dlgStack.top();
		delete dlg;
		dlgStack.pop();
	}

	gui_dlg = NULL;
}

bool SDLGui_isClosed(void)
{
	return (gui_dlg == NULL);
}

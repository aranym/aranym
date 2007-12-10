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

#include <cstdlib>
#include <stack>

#include "font.h"
#include "dialog.h"
#include "dlgMain.h"

#define DEBUG 0
#include "debug.h"

#define sdlscrn		gui_surf

static SDL_Surface *fontgfx=NULL;
static int fontwidth, fontheight;   /* Height and width of the actual font */

static std::stack<Dialog *> dlgStack;

/* gui surface */
static HostSurface *gui_hsurf = NULL;
static SDL_Surface *gui_surf = NULL;
static int gui_x = 0, gui_y = 0; /* gui position */

/* current dialog to draw */
static Dialog *gui_dlg = NULL;

static SDL_Color gui_palette[4] = {
	{0,0,0,0},
	{128,128,128,0},
	{192,192,192, 0},
	{255, 255, 255, 0}
};

enum {
	blackc = 0,
	darkgreyc,
	greyc,
	whitec
};

// Stores current dialog coordinates
static SDL_Rect DialogRect = {0, 0, 0, 0};

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

/*-----------------------------------------------------------------------*/
/*
  Load an 1 plane XBM into a 8 planes SDL_Surface.
*/
static SDL_Surface *SDLGui_LoadXBM(Uint8 *srcbits)
{
  SDL_Surface *bitmap;
  Uint8 *dstbits;
  int x, y, srcpitch;
	int w = 128;
	int h = 256;

  /* Allocate the bitmap */
  bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
  if ( bitmap == NULL )
  {
    panicbug("Couldn't allocate bitmap: %s", SDL_GetError());
    return(NULL);
  }

  srcpitch = ((w + 7) / 8);
  dstbits = (Uint8 *)bitmap->pixels;
  int mask = 0x80;
  int charheight = h/16;

  /* Copy the pixels */
  for (y = 0 ; y < h ; y++)
  {
    for (x = 0 ; x < w ; x++)
    {
      int ascii = x/8 + (y / charheight) * charheight;
      int charline = y % charheight;
      dstbits[x] = (srcbits[ascii + 256*charline] & mask) ? 1 : 0;
      mask >>= 1;
      mask |= (mask << 8);
      mask &= 0x1FF;
    }
    dstbits += bitmap->pitch;
  }

  return(bitmap);
}

/*-----------------------------------------------------------------------*/
/*
  Initialize the GUI.
*/
bool SDLGui_Init()
{
	// Load the font graphics
	char font_filename[256];
	unsigned char font_data[4096];
	getConfFilename("font", font_filename, sizeof(font_filename));
	FILE *f = fopen(font_filename, "rb");
	bool font_loaded = false;
	if (f != NULL) {
		if (fread(font_data, 1, sizeof(font_data), f) == sizeof(font_data)) {
			font_loaded = true;
		}
		fclose(f);
	}
	// If font can't be loaded use the internal one
	if (! font_loaded) {
		memcpy(font_data, font_bits, sizeof(font_data));
	}
	
  fontgfx = SDLGui_LoadXBM(font_data);
  if (fontgfx == NULL)
  {
    panicbug("Could not create font data");
    panicbug("ARAnyM GUI will not be available");
    return false;
  }

	gui_hsurf = host->video->createSurface(320+16,400+16,8);
	if (!gui_hsurf) {
		panicbug("Could not create surface for GUI");
		panicbug("ARAnyM GUI will not be available");
		return false;
	}

	gui_surf = gui_hsurf->getSdlSurface();

	gui_hsurf->setPalette(gui_palette, 0, 4);

  /* Set font color 0 as transparent */
  SDL_SetColorKey(fontgfx, SDL_SRCCOLORKEY, 0);

  /* Get the font width and height: */
  fontwidth = fontgfx->w/16;
  fontheight = fontgfx->h/16;

  return true;
}


/*-----------------------------------------------------------------------*/
/*
  Uninitialize the GUI.
*/
int SDLGui_UnInit()
{
	if (gui_hsurf) {
		delete gui_hsurf;
		gui_hsurf = NULL;
	}

  if (fontgfx)
  {
    SDL_FreeSurface(fontgfx);
    fontgfx = NULL;
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
  rect->x = dlg[objnum].x * fontwidth;
  rect->y = dlg[objnum].y * fontheight;
  rect->w = dlg[objnum].w * fontwidth;
  rect->h = dlg[objnum].h * fontheight;

  rect->x += (sdlscrn->w - (dlg[0].w * fontwidth)) / 2;
  rect->y += (sdlscrn->h - (dlg[0].h * fontheight)) / 2;
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
    SDL_UpdateRects(sdlscrn, 1, rect);
  else
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


/*-----------------------------------------------------------------------*/
/*
  Draw a text string.
*/
void SDLGui_Text(int x, int y, const char *txt, int col)
{
  int i;
  char c;
  SDL_Rect sr, dr;

  SDL_SetColors(fontgfx, &gui_palette[col], 1, 1);

  for (i = 0 ; txt[i] != 0 ; i++)
  {
    c = txt[i];
    sr.x = fontwidth * (c % 16);
    sr.y = fontheight * (c / 16);
    sr.w = fontwidth;
    sr.h = fontheight;

    dr.x = x + (fontwidth * i);
    dr.y = y;
    dr.w = fontwidth;
    dr.h = fontheight;

    SDL_BlitSurface(fontgfx, &sr, sdlscrn, &dr);
  }
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
    coord.x += (cursor->position * fontwidth);
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
  coord->y -= widthbackground*2;
  coord->w += (widthbackground * 2);
  coord->h += (widthbackground*2 * 2);

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

  x = coord.x + ((coord.w - (strlen(bdlg[objnum].txt) * fontwidth)) / 2);
  y = coord.y + ((coord.h - fontheight) / 2);

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
  Draw a dialog check box object state.
*/
void SDLGui_DrawCheckBoxState(SGOBJ *cdlg, int objnum)
{
  Uint32 grey = SDLGui_MapColor(greyc);
  SDL_Rect coord;
  char str[3];
  int textc;

  SDLGui_ObjCoord(cdlg, objnum, &coord);

  if (cdlg[objnum].flags & SG_RADIO)
  {
    if (cdlg[objnum].state & SG_SELECTED) {
      str[0]=SGCHECKBOX_RADIO_SELECTED;
      str[1]=SGCHECKBOX_RADIO_SELECTEd;
    }
    else {
      str[0]=SGCHECKBOX_RADIO_NORMAL;
      str[1]=SGCHECKBOX_RADIO_NORMAl;
    }
  }
  else
  {
    if (cdlg[objnum].state & SG_SELECTED) {
      str[0]=SGCHECKBOX_SELECTED;
      str[1]=SGCHECKBOX_SELECTEd;
    }
    else {
      str[0]=SGCHECKBOX_NORMAL;
      str[1]=SGCHECKBOX_NORMAl;
    }
  }

  if (cdlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  str[2]='\0';

  coord.w = fontwidth*2;
  coord.h = fontheight;

  if (cdlg[objnum].flags & SG_BUTTON_RIGHT)
    coord.x += ((strlen(cdlg[objnum].txt) + 1) * fontwidth);

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
    coord.x += (fontwidth * 3); // 2 chars for the box plus 1 space

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
  const char *downstr = "\x02";
  int textc;

  if (pdlg[objnum].state & SG_DISABLED)
    textc = darkgreyc;
  else
    textc = blackc;

  SDLGui_DrawBox(pdlg, objnum);

  SDLGui_ObjCoord(pdlg, objnum, &coord);

  SDLGui_Text(coord.x, coord.y, pdlg[objnum].txt, textc);
  SDLGui_Text(coord.x+coord.w-fontwidth, coord.y, downstr, textc);
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
      SDLGui_DrawText(dlg, objnum);
      break;
    case SGEDITFIELD:
      SDLGui_DrawEditField(dlg, objnum);
      break;
    case SGBUTTON:
      SDLGui_DrawButton(dlg, objnum);
      break;
    case SGCHECKBOX:
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
		/* Blink cursor ? */
		cursor_state *cursor = gui_dlg->getCursor();
		Uint32 cur_ticks = SDL_GetTicks();
		if (cur_ticks-cursor->blink_counter >= 500) {
			cursor->blink_counter = cur_ticks;
			cursor->blink_state = !cursor->blink_state;
			SDLGui_DrawCursor(gui_dlg->getDialog(), cursor);
		}
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
  while (dlg[i++].type != -1);
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
  while (dlg[--obj].flags & SG_RADIO);

  // Update state
  while (dlg[++obj].flags & SG_RADIO)
  {
    // This code scan every object in the group. This allows to solve cases
    // where multiple objects were selected in the group by clicking one.
    if ((obj != clicked_obj) && (dlg[obj].state & SG_SELECTED))
    {
      // Deselect this radio button
      dlg[obj].state &= ~SG_SELECTED;
      SDLGui_DrawObject(dlg, obj);
      SDLGui_RefreshObj(dlg, obj);
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
    cursor->position = strlen(dlg[new_object].txt);
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
        cursor->position = strlen(dlg[new_object].txt);
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
  i = (x - coord.x + (fontwidth / 2)) / fontwidth;
  j = strlen(dlg[clicked_obj].txt);

  cursor->object = clicked_obj;
  cursor->position = MIN(i, j);
  cursor->blink_state = true;
  cursor->blink_counter = 0;
  SDLGui_DrawCursor(dlg, cursor);
}


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

/* Process event for a dialog */
int SDLGui_DoEvent(const SDL_Event &event)
{
	int retval = Dialog::GUI_CONTINUE;

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
		}

		retval = gui_dlg->processDialog();

		if (retval != Dialog::GUI_CONTINUE) {
			Dialog *prev_dlg = NULL;
			dlgStack.pop();
			if (!dlgStack.empty()) {
				prev_dlg = dlgStack.top();		

				/* Process result of called dialog */
				prev_dlg->processResult();

				/* Continue displaying GUI with previous dialog */
				retval = Dialog::GUI_CONTINUE;
			}

			/* Close current dialog */
			delete gui_dlg;

			/* Set dialog to previous one */
			gui_dlg = prev_dlg;
			if (gui_dlg) {
				gui_dlg->init();
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
	}

	dlgStack.push(new_dlg);

	gui_dlg = dlgStack.top();
	gui_dlg->init();	
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

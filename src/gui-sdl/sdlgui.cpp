/*
  Hatari

  A tiny graphical user interface for Hatari.
*/

#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "debug.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdlib>
#else
# include <stdlib.h>
#endif

#include "font8.h"

#define sdlscrn		hostScreen.getPhysicalSurface()
#define scrwidth	hostScreen.getWidth()
#define scrheight	hostScreen.getHeight()

static SDL_Surface *stdfontgfx=NULL;
static SDL_Surface *fontgfx=NULL;   /* The actual font graphics */
static int fontwidth, fontheight;   /* Height and width of the actual font */

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
  Load an XBM into a SDL_Surface.
*/
static SDL_Surface *SDLGui_LoadXBM(int w, int h, uint8 *bits)
{
	SDL_Surface *bitmap;
	uint8 *line;    /* Allocate the bitmap */
	bitmap = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 1, 0, 0, 0, 0);
	if ( bitmap == NULL ) {
		panicbug("Couldn't allocate bitmap: %s", SDL_GetError());
		return(NULL);
	}
	/* Copy the pixels */
	line = (uint8 *)bitmap->pixels;
	w = (w+7)/8;
	while ( h-- ) {
		memcpy(line, bits, w);
		/* X11 Bitmap images have the bits reversed */
		{
			int i, j;
			uint8 *buf;
			uint8 byte;
			for ( buf=line, i=0; i<w; ++i, ++buf ) {
				byte = *buf;
				*buf = 0;
				for ( j=7; j>=0; --j ) {
					*buf |= (byte&0x01)<<j;
					byte >>= 1;
				}
			}
		}
		line += bitmap->pitch;
		bits += w;
	}
	return(bitmap);
}

/*-----------------------------------------------------------------------*/
/*
  Initialize the GUI.
*/
bool SDLGui_Init()
{
  /* Load the font graphics: */
  stdfontgfx = SDLGui_LoadXBM(font8_width, font8_height, font8_bits);
  if( stdfontgfx==NULL )
  {
    panicbug("Could not create font data");
    panicbug("ARAnyM GUI will not be available");
    return false;
  }
  return true;
}


/*-----------------------------------------------------------------------*/
/*
  Uninitialize the GUI.
*/
int SDLGui_UnInit()
{
  if(stdfontgfx) {
    SDL_FreeSurface(stdfontgfx);
    stdfontgfx = NULL;
  }
  if(fontgfx) {
    SDL_FreeSurface(fontgfx);
    fontgfx = NULL;
  }

  return 0;
}


/*-----------------------------------------------------------------------*/
/*
  Prepare the font to suit the actual resolution.
*/
int SDLGui_PrepareFont()
{
  if( stdfontgfx == NULL )
  {
    panicbug("Error: The font has not been loaded!");
    return -1;
  }

  /* Convert the font graphics to the actual screen format */
  SCRLOCK;
  fontgfx = SDL_DisplayFormat(stdfontgfx);
  SCRUNLOCK;
  if( fontgfx==NULL )
  {
    panicbug("Could not convert font: %s", SDL_GetError() );
    return -1;
  }
  /* Set transparent pixel */
  SCRLOCK;
  SDL_SetColorKey(fontgfx, (SDL_SRCCOLORKEY|SDL_RLEACCEL), SDL_MapRGB(fontgfx->format,255,255,255));
  SCRUNLOCK;
  /* Get the font width and height: */
  fontwidth = fontgfx->w/16;
  fontheight = fontgfx->h/16;

  return 0;
}

/*-----------------------------------------------------------------------*/
/*
  Free the font.
*/
void SDLGui_FreeFont()
{
  if(fontgfx) {
    SDL_FreeSurface(fontgfx);
    fontgfx = NULL;
  }

  // No dialog on screen anymore -> update stored coordinates
  DialogRect.w = 0;
  DialogRect.y = 0;
}


/*-----------------------------------------------------------------------*/
/*
  Compute real coordinates for a given object.
  Note: centers dialog on screen.
*/
static void SDLGui_ObjCoord(SGOBJ *dlg, int objnum, SDL_Rect *rect)
{
  rect->x = dlg[objnum].x*fontwidth;
  rect->y = dlg[objnum].y*fontheight;
  rect->w = dlg[objnum].w*fontwidth;
  rect->h = dlg[objnum].h*fontheight;

  rect->x += (sdlscrn->w-(dlg[0].w*fontwidth))/2;
  rect->y += (sdlscrn->h-(dlg[0].h*fontheight))/2;
}


/*-----------------------------------------------------------------------*/
/*
  Compute real coordinates for a given object.
  This one takes borders into account and give coordinates as seen by user
*/
void SDLGui_ObjFullCoord(SGOBJ *dlg, int objnum, SDL_Rect *coord)
{
  SDLGui_ObjCoord(dlg, objnum, coord);

  switch( dlg[objnum].type )
  {
    case SGBOX:
    case SGBUTTON:
      // Take border into account
      if (dlg[objnum].flags & SG_DEFAULT)
      {
        // Wider border if default
        coord->x -= 4;
        coord->y -= 4;
        coord->w += 8;
        coord->h += 8;
      }
      else
      {
        coord->x -= 3;
        coord->y -= 3;
        coord->w += 6;
        coord->h += 6;
      }
      break;
    case SGEDITFIELD:
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
  Refresh display to reflect an object change.
*/
void SDLGui_RefreshObj(SGOBJ *dlg, int objnum)
{
  SDL_Rect coord;

  SDLGui_ObjFullCoord(dlg, objnum, &coord);

  SCRLOCK;
  SDLGui_UpdateRect(&coord);
  SCRUNLOCK;
}


/*-----------------------------------------------------------------------*/
/*
  Draw a text string.
*/
void SDLGui_Text(int x, int y, const char *txt)
{
  int i;
  char c;
  SDL_Rect sr, dr;

  SCRLOCK;
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
  SCRUNLOCK;
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog text object.
*/
void SDLGui_DrawText(SGOBJ *tdlg, int objnum)
{
  SDL_Rect coord;
  
  SDLGui_ObjCoord(tdlg, objnum, &coord);
  SDLGui_Text(coord.x, coord.y, tdlg[objnum].txt);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a edit field object.
*/
void SDLGui_DrawEditField(SGOBJ *edlg, int objnum)
{
  SDL_Rect coord;

  SDLGui_ObjCoord(edlg, objnum, &coord);

  SDLGui_Text(coord.x, coord.y, edlg[objnum].txt);

  // Draw a line below.
  coord.y = coord.y + coord.h;
  coord.h = 1;
  SDL_FillRect(sdlscrn, &coord, SDL_MapRGB(sdlscrn->format,128,128,128));
}


/*-----------------------------------------------------------------------*/
/*
  Draw a 3D box.
*/
void SDLGui_Draw3DBox(SDL_Rect *coord, Uint32 upleftc, Uint32 downrightc, int width3D, int borderwidth)
{
  SDL_Rect rect;
  Uint32 grey = SDL_MapRGB(sdlscrn->format,192,192,192);
  Uint32 black = SDL_MapRGB(sdlscrn->format,0,0,0);
  int backgroundwidth = 1;
  int i;

  SCRLOCK;

  rect.x = coord->x-backgroundwidth;
  rect.y = coord->y-backgroundwidth;
  rect.w = coord->w+backgroundwidth*2;
  rect.h = coord->h+backgroundwidth*2;
  SDL_FillRect(sdlscrn, &rect, grey);

  for ( i = backgroundwidth+1 ; i <= width3D+backgroundwidth ; i++)
  {
    rect.x = coord->x-i;
    rect.y = coord->y-i;
    rect.w = coord->w+i*2;
    rect.h = 1;
    SDL_FillRect(sdlscrn, &rect, upleftc);

    rect.x = coord->x-i;
    rect.y = coord->y-i;
    rect.w = 1;
    rect.h = coord->h+i*2;
    SDL_FillRect(sdlscrn, &rect, upleftc);

    rect.x = coord->x-i+1;
    rect.y = coord->y+coord->h-1+i;
    rect.w = coord->w+i*2-1;
    rect.h = 1;
    SDL_FillRect(sdlscrn, &rect, downrightc);

    rect.x = coord->x+coord->w-1+i;
    rect.y = coord->y-i+1;
    rect.w = 1;
    rect.h = coord->h+i*2-1;
    SDL_FillRect(sdlscrn, &rect, downrightc);
  }

  i = borderwidth+width3D+backgroundwidth;

  rect.x = coord->x-i;
  rect.y = coord->y-i;
  rect.w = coord->w+i*2;
  rect.h = borderwidth;
  SDL_FillRect(sdlscrn, &rect, black);

  rect.x = coord->x-i;
  rect.y = coord->y-i;
  rect.w = borderwidth;
  rect.h = coord->h+i*2;
  SDL_FillRect(sdlscrn, &rect, black);

  rect.x = coord->x+coord->w+i-borderwidth;
  rect.y = coord->y-i;
  rect.w = borderwidth;
  rect.h = coord->h+i*2;
  SDL_FillRect(sdlscrn, &rect, black);

  rect.x = coord->x-i;
  rect.y = coord->y+coord->h+i-borderwidth;
  rect.w = coord->w+i*2;
  rect.h = borderwidth;
  SDL_FillRect(sdlscrn, &rect, black);

  SCRUNLOCK;
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog box object.
*/
void SDLGui_DrawBox(SGOBJ *bdlg, int objnum)
{
  SDL_Rect coord;
  Uint32 upleftc, downrightc;
  Uint32 darkgreyc = SDL_MapRGB(sdlscrn->format,128,128,128);
  Uint32 whitec    = SDL_MapRGB(sdlscrn->format,255,255,255);

  if( bdlg[objnum].state&SG_SELECTED )
  {
    upleftc    = darkgreyc;
    downrightc = whitec;
  }
  else
  {
    upleftc    = whitec;
    downrightc = darkgreyc;
  }

  SDLGui_ObjCoord(bdlg, objnum, &coord);

  if (bdlg[objnum].flags & SG_DEFAULT)
    SDLGui_Draw3DBox(&coord, upleftc, downrightc, 1, 2);
  else
    SDLGui_Draw3DBox(&coord, upleftc, downrightc, 1, 1);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a normal button.
*/
void SDLGui_DrawButton(SGOBJ *bdlg, int objnum)
{
  SDL_Rect coord;
  int x,y;

  SDLGui_ObjCoord(bdlg, objnum, &coord);

  x = coord.x+((coord.w-(strlen(bdlg[objnum].txt)*fontwidth))/2);
  y = coord.y+((coord.h-fontheight)/2);

  if( bdlg[objnum].state&SG_SELECTED )
  {
    x += 1;
    y += 1;
  }

  SDLGui_DrawBox(bdlg, objnum);
  SDLGui_Text(x, y, bdlg[objnum].txt);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog radio button object state.
*/
void SDLGui_DrawRadioButtonState(SGOBJ *rdlg, int objnum)
{
  Uint32 grey = SDL_MapRGB(sdlscrn->format,192,192,192);
  SDL_Rect coord;
  char str[2];

  SDLGui_ObjCoord(rdlg, objnum, &coord);

  if( rdlg[objnum].state&SG_SELECTED )
  {
    str[0]=SGRADIOBUTTON_SELECTED;
  }
  else
  {
    str[0]=SGRADIOBUTTON_NORMAL;
  }
  str[1]='\0';

  coord.w = fontwidth;
  coord.h = fontheight;
  SDL_FillRect(sdlscrn, &coord, grey);
  SDLGui_Text(coord.x, coord.y, str);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog radio button object.
*/
void SDLGui_DrawRadioButton(SGOBJ *rdlg, int objnum)
{
  SDL_Rect coord;

  SDLGui_ObjCoord(rdlg, objnum, &coord);

  coord.x += (fontwidth*2);
  SDLGui_Text(coord.x, coord.y, rdlg[objnum].txt);

  SDLGui_DrawRadioButtonState(rdlg, objnum);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog check box object state.
*/
void SDLGui_DrawCheckBoxState(SGOBJ *cdlg, int objnum)
{
  Uint32 grey = SDL_MapRGB(sdlscrn->format,192,192,192);
  SDL_Rect coord;
  char str[2];

  SDLGui_ObjCoord(cdlg, objnum, &coord);

  if (cdlg[objnum].state&SG_SELECTED)
  {
    str[0]=SGCHECKBOX_SELECTED;
  }
  else
  {
    str[0]=SGCHECKBOX_NORMAL;
  }
  str[1]='\0';

  coord.w = fontwidth;
  coord.h = fontheight;

  if (cdlg[objnum].flags&SG_BUTTON_RIGHT)
    coord.x += ((strlen(cdlg[objnum].txt) + 1) * fontwidth);

  SDL_FillRect(sdlscrn, &coord, grey);
  SDLGui_Text(coord.x, coord.y, str);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog check box object.
*/
void SDLGui_DrawCheckBox(SGOBJ *cdlg, int objnum)
{
  SDL_Rect coord;

  SDLGui_ObjCoord(cdlg, objnum, &coord);

  if (!(cdlg[objnum].flags&SG_BUTTON_RIGHT))
    coord.x += (fontwidth * 2);

  SDLGui_Text(coord.x, coord.y, cdlg[objnum].txt);
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

  SDLGui_DrawBox(pdlg, objnum);

  SDLGui_ObjCoord(pdlg, objnum, &coord);

  SDLGui_Text(coord.x, coord.y, pdlg[objnum].txt);
  SDLGui_Text(coord.x+coord.w-fontwidth, coord.y, downstr);
}


/*-----------------------------------------------------------------------*/
/*
  Let the user insert text into an edit field object.
  NOTE: The dlg[objnum].txt must point to an an array that is big enough
  for dlg[objnum].w characters!
*/
void SDLGui_EditField(SGOBJ *dlg, int objnum)
{
  unsigned int cursorPos;               /* Position of the cursor in the edit field */
  int blinkState = 0;                   /* Used for cursor blinking */
  bool bStopEditing = false;            /* true if user wants to exit the edit field */
  char *txt;                            /* Shortcut for dlg[objnum].txt */
  SDL_Rect rect;
  Uint32 grey, cursorCol;
  SDL_Event event;

  grey = SDL_MapRGB(sdlscrn->format,192,192,192);
  cursorCol = SDL_MapRGB(sdlscrn->format,128,128,128);

  SDLGui_ObjCoord(dlg, objnum, &rect);
  // Add some place for cursor
  rect.w += fontwidth;

  txt = dlg[objnum].txt;
  cursorPos = strlen(txt);

  do
  {
    /* Look for events */
    if (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_USEREVENT)) == 0)
    {
      /* No event: Wait some time for cursor blinking */
      SDL_Delay(250);
      blinkState ^= 1;
    }
    else
    {
      /* Handle events */
      do
      {
        switch(event.user.code)
        {
          case SDL_MOUSEBUTTONDOWN:             /* Mouse pressed -> stop editing */
            bStopEditing = true;
            break;
          case SDL_KEYDOWN:                     /* Key pressed */
            int keysym = (int)event.user.data1;
            switch(keysym)
            {
              case SDLK_RETURN:
              case SDLK_KP_ENTER:
                bStopEditing = true;
                break;
              case SDLK_LEFT:
                if(cursorPos > 0)
                  cursorPos -= 1;
                break;
              case SDLK_RIGHT:
                if(cursorPos < strlen(txt))
                  cursorPos += 1;
                break;
              case SDLK_BACKSPACE:
                if(cursorPos > 0)
                {
                  memmove(&txt[cursorPos-1], &txt[cursorPos], strlen(&txt[cursorPos])+1);
                  cursorPos -= 1;
                }
                break;
              case SDLK_DELETE:
                if(cursorPos < strlen(txt))
                  memmove(&txt[cursorPos], &txt[cursorPos+1], strlen(&txt[cursorPos+1])+1);
                break;
              default:
                /* If it is a "good" key then insert it into the text field */
                if(keysym >= 32 && keysym < 256)
                {
                  if(strlen(txt) < dlg[objnum].w)
                  {
                    memmove(&txt[cursorPos+1], &txt[cursorPos], strlen(&txt[cursorPos])+1);
                    if(event.key.keysym.mod & (KMOD_LSHIFT|KMOD_RSHIFT))
                      txt[cursorPos] = toupper(keysym);
                    else
                      txt[cursorPos] = keysym;
                    cursorPos += 1;
                  }
                }
                break;
            }
            break;
        }
      }
      while(SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_USEREVENT))>0);

      blinkState = 1;
    }

    /* Redraw the text field: */
    SDL_FillRect(sdlscrn, &rect, grey);  /* Draw background */
    /* Draw the cursor: */
    if(blinkState && !bStopEditing)
    {
      SDL_Rect cursorrect;
      cursorrect.x = rect.x + cursorPos * fontwidth;  cursorrect.y = rect.y;
      cursorrect.w = fontwidth;  cursorrect.h = rect.h;
      SDL_FillRect(sdlscrn, &cursorrect, cursorCol);
    }
    SDLGui_Text(rect.x, rect.y, dlg[objnum].txt);  /* Draw text */
    SDLGui_RefreshObj(dlg, objnum);
  }
  while(!bStopEditing);
}


/*-----------------------------------------------------------------------*/
/*
  Draw an object.
*/
void SDLGui_DrawObject(SGOBJ *dlg, int objnum)
{
  switch( dlg[objnum].type )
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
    case SGRADIOBUT:
      SDLGui_DrawRadioButton(dlg, objnum);
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
      if (dlg[i].state & SG_HIDDEN) continue;
      ob = i;
      break;
    }
  }

  return ob;
}


/*-----------------------------------------------------------------------*/
/*
  A radio button has been selected. Let's deselect any other in his group.
*/
void SDLGui_SelectRadioButton(SGOBJ *dlg, int clicked_obj)
{
  int obj;

  // Find first radio button in this group
  obj = clicked_obj;
  while (dlg[--obj].type == SGRADIOBUT);

  // Update state
  while (dlg[++obj].type == SGRADIOBUT)
  {
    // This code scan every object in the group. This allows to solve cases
    // where multiple radio-buttons where selected in the group by clicking
    // one.
    if ((obj != clicked_obj) && (dlg[obj].state & SG_SELECTED))
    {
      // Deselect this radio button
      dlg[obj].state &= ~SG_SELECTED;
      SDLGui_DrawRadioButtonState(dlg, obj);
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

  // Special case : user clicked on an already selected radio button
  // do not modify its state.
  // We handle it here because it allows to exit if the object is SG_EXIT or
  // SG_TOUCHEXIT without any additional test.
  if ((dlg[clicked_obj].type == SGRADIOBUT) && (original_state & SG_SELECTED))
    return (obj == clicked_obj);

  if (((obj != clicked_obj) &&
       (dlg[clicked_obj].state != original_state)) ||
      ((obj == clicked_obj) &&
       (dlg[clicked_obj].state == original_state)))
  {
    switch (dlg[clicked_obj].type)
    {
      case SGBUTTON:
      case SGCHECKBOX:
      case SGPOPUP:
      case SGRADIOBUT:
        dlg[clicked_obj].state ^= SG_SELECTED;
        SDLGui_DrawObject(dlg, clicked_obj);
        SDLGui_RefreshObj(dlg, clicked_obj);
        break;
    }
  }

  return (obj == clicked_obj);
}

/*-----------------------------------------------------------------------*/
/*
  Handle mouse clicks.
*/
int SDLGui_MouseClick(SGOBJ *dlg, int fx, int fy)
{
  int clicked_obj;
  int return_obj = -1;
  int original_state = 0;
  int x, y;

  clicked_obj = SDLGui_FindObj(dlg, fx, fy);

  if (clicked_obj >= 0)
  {
    original_state = dlg[clicked_obj].state;
    SDLGui_UpdateObjState(dlg, clicked_obj, original_state, fx, fy);

    if (dlg[clicked_obj].flags & SG_TOUCHEXIT)
    {
      return_obj = clicked_obj;
      clicked_obj = -1;
    }
  }

  while (clicked_obj >= 0)
  {
    SDL_Event evnt;
    // SDL_PumpEvents() - not necessary, the main check_event thread calls it
    if (SDL_PeepEvents(&evnt, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_USEREVENT)))
    {
      switch (evnt.user.code)
      {
        case SDL_USEREVENT:
          // a signal that resolution has changed
          // Restore clicked object original state
          dlg[clicked_obj].state = original_state;

          // re-draw dialog
          SDLGui_DrawDialog(dlg);

          // Were done. Exit from mouse click handling.
          clicked_obj = -1;
          break;

        case SDL_MOUSEBUTTONUP:
          x = (int)evnt.user.data1;
          y = (int)evnt.user.data2;
          if (SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y))
          {
            // true if mouse button is released over clicked object.
            // If applicable, the object has been selected by
            // SDLGui_UpdateObjState(). Let's do additional handling here.

            // Exit if object is an SG_EXIT one.
            if (dlg[clicked_obj].flags & SG_EXIT)
              return_obj = clicked_obj;

            switch (dlg[clicked_obj].type)
            {
              case SGEDITFIELD:
                SDLGui_EditField(dlg, clicked_obj);
                break;

              case SGRADIOBUT:
                SDLGui_SelectRadioButton(dlg, clicked_obj);
                break;
            }
          }

          // Were done. Exit from mouse click handling.
          clicked_obj = -1;

          break;
      }
    }
    else
    {
      // No special event occured.
      // Update object state according to mouse coordinates.
      SDL_GetMouseState(&x, &y);
      SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y);

      // Wait a little to avoid eating CPU.
      SDL_Delay(100);
    }
  }

  return return_obj;
}


/*-----------------------------------------------------------------------*/
/*
  Handle key press.
*/
int SDLGui_KeyPress(SGOBJ *dlg, int keysym)
{
  int return_obj = -1;
  int obj;

  switch(keysym)
  {
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
      obj = SDLGui_FindDefaultObj(dlg);
      if (obj >= 0)
      {
        dlg[obj].state ^= SG_SELECTED;
        SDLGui_DrawObject(dlg, obj);
        SDLGui_RefreshObj(dlg, obj);
        if (dlg[obj].flags & (SG_EXIT | SG_TOUCHEXIT))
        {
          return_obj = obj;
          SDL_Delay(300);
        }
      }
      break;
  }

  return return_obj;
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
      	BackgroundRect.w = scrwidth;
      	BackgroundRect.h = scrheight;
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
      	BackgroundRect.w = scrwidth;
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
          ((DialogRect.y + DialogRect.h) < (int)scrheight) ?
          (DialogRect.h + DialogRect.y - BackgroundRect.y) :
          (scrheight - DialogRect.y);
        return_rect = &BackgroundRect;
      }
      else
        return_rect = SDLGui_GetNextBackgroundRect();
      break;

    case SG_BCKGND_RECT_RIGHT:
      BackgroundRectCounter = SG_BCKGND_RECT_BOTTOM;
      if ((DialogRect.x + DialogRect.w) < (int)scrwidth)
      {
        BackgroundRect.x = DialogRect.x + DialogRect.w;
        BackgroundRect.y = (DialogRect.y > 0) ? DialogRect.y : 0;
        BackgroundRect.w = scrwidth - (DialogRect.x + DialogRect.w);
        BackgroundRect.h =
          ((DialogRect.y + DialogRect.h) < (int)scrheight) ?
          (DialogRect.h + DialogRect.y - BackgroundRect.y) :
          (scrheight - DialogRect.y);
        return_rect = &BackgroundRect;
      }
      else
        return_rect = SDLGui_GetNextBackgroundRect();
      break;

    case SG_BCKGND_RECT_BOTTOM:
      BackgroundRectCounter = SG_BCKGND_RECT_END;
      if ((DialogRect.y + DialogRect.h) < (int)scrheight)
      {
        // Bottom
        BackgroundRect.x = 0;
        BackgroundRect.y = DialogRect.y + DialogRect.h;
        BackgroundRect.w = scrwidth;
        BackgroundRect.h = scrheight - (DialogRect.y + DialogRect.h);
        return_rect = &BackgroundRect;
      }
      else
        return_rect = SDLGui_GetNextBackgroundRect();
      break;
  }

  return return_rect;
}


/*-----------------------------------------------------------------------*/
/*
  Show and process a dialog. Returns the button number that has been
  pressed. Does NOT handle SDL_QUIT - you must handle it before you
  pass the input event to the SDL GUI.
*/
int SDLGui_DoDialog(SGOBJ *dlg)
{
  int return_obj = -1;
  int obj;
  int x, y;
  int keysym;

  /* Is the left mouse button still pressed? Yes -> Handle TOUCHEXIT objects here */

  // SDL_PumpEvents(); - don't call it here, it's not thread safe probably
  bool stillPressed = (SDL_GetMouseState(&x, &y) & SDL_BUTTON(1));
  obj = SDLGui_FindObj(dlg, x, y);
  if (stillPressed && (obj >= 0) && (dlg[obj].flags & SG_TOUCHEXIT))
  {
    // Mouse button is pressed over a TOUCHEXIT Button
    // Toogle its state before drawing anything (it has been deselected before).
    dlg[obj].state ^= SG_SELECTED;

    return_obj = obj;
  }

  SDLGui_DrawDialog(dlg);

  /* The main loop */
  while (return_obj < 0)
  {
    SDL_Event evnt;
    // SDL_PumpEvents() - not necessary, the main check_event thread calls it
    if (SDL_PeepEvents(&evnt, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_USEREVENT)))
    {
      switch(evnt.user.code)
      {
      	case SDL_KEYDOWN:
          keysym = (int)evnt.user.data1;
          return_obj = SDLGui_KeyPress(dlg, keysym);
      	  break;

        case SDL_USEREVENT:
          // a signal that resolution has changed
          SDLGui_DrawDialog(dlg);	// re-draw dialog
          break;

        case SDL_MOUSEBUTTONDOWN:
          x = (int)evnt.user.data1;
          y = (int)evnt.user.data2;
          return_obj = SDLGui_MouseClick(dlg, x, y);
          break;
      }
    }
    else
    {
      // No special event occured.
      // Wait a little to avoid eating CPU.
      SDL_Delay(100);
    }
  }

  if (dlg[return_obj].type == SGBUTTON)
  {
    // Deselect button...
    // BUG: This should be caller responsibility
    dlg[return_obj].state ^= SG_SELECTED;
  }

  return return_obj;
}

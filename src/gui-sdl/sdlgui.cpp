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

#define sdlscrn	hostScreen.getPhysicalSurface()

static SDL_Surface *stdfontgfx=NULL;
static SDL_Surface *fontgfx=NULL;   /* The actual font graphics */
static int fontwidth, fontheight;   /* Height and width of the actual font */

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

void SDLGui_FreeFont()
{
  if(fontgfx) {
    SDL_FreeSurface(fontgfx);
    fontgfx = NULL;
  }
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
  Draw a text string.
*/
void SDLGui_Text(int x, int y, const char *txt)
{
  int i;
  char c;
  SDL_Rect sr, dr;

  SCRLOCK;
  for(i=0; txt[i]!=0; i++)
  {
    c = txt[i];
    sr.x=fontwidth*(c%16);  sr.y=fontheight*(c/16);
    sr.w=fontwidth;         sr.h=fontheight;
    dr.x=x+i*fontwidth;     dr.y=y;
    dr.w=fontwidth;         dr.h=fontheight;
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
  SDL_FillRect(sdlscrn, &coord, SDL_MapRGB(sdlscrn->format,160,160,160));
}


/*-----------------------------------------------------------------------*/
/*
  Draw a 3D box.
*/
void SDLGui_Draw3DBox(SDL_Rect *coord, Uint32 upleftc, Uint32 downrightc)
{
  SDL_Rect rect;
  Uint32 grey = SDL_MapRGB(sdlscrn->format,192,192,192);
  Uint32 black = SDL_MapRGB(sdlscrn->format,0,0,0);

  SCRLOCK;

  rect.x = coord->x-3;          rect.y = coord->y-3;
  rect.w = coord->w+6;          rect.h = 1;
  SDL_FillRect(sdlscrn, &rect, black);

  rect.x = coord->x-3;          rect.y = coord->y-3;
  rect.w = 1;                   rect.h = coord->h+6;
  SDL_FillRect(sdlscrn, &rect, black);

  rect.x = coord->x+coord->w+2; rect.y = coord->y-3;
  rect.w = 1;                   rect.h = coord->h+6;
  SDL_FillRect(sdlscrn, &rect, black);

  rect.x = coord->x-3;          rect.y = coord->y+coord->h+2;
  rect.w = coord->w+6;          rect.h = 1;
  SDL_FillRect(sdlscrn, &rect, black);

  rect.x = coord->x-2;          rect.y = coord->y-2;
  rect.w = coord->w+4;          rect.h = 1;
  SDL_FillRect(sdlscrn, &rect, upleftc);

  rect.x = coord->x-2;          rect.y = coord->y-2;
  rect.w = 1;                   rect.h = coord->h+4;
  SDL_FillRect(sdlscrn, &rect, upleftc);

  rect.x = coord->x-1;          rect.y = coord->y+coord->h+1;
  rect.w = coord->w+3;          rect.h = 1;
  SDL_FillRect(sdlscrn, &rect, downrightc);

  rect.x = coord->x+coord->w+1; rect.y = coord->y-1;
  rect.w = 1;                   rect.h = coord->h+3;
  SDL_FillRect(sdlscrn, &rect, downrightc);

  rect.x = coord->x-1;          rect.y = coord->y-1;
  rect.w = coord->w+2;          rect.h = coord->h+2;
  SDL_FillRect(sdlscrn, &rect, grey);

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

  if( bdlg[objnum].state&SG_SELECTED )
  {
    upleftc = SDL_MapRGB(sdlscrn->format,128,128,128);
    downrightc = SDL_MapRGB(sdlscrn->format,255,255,255);
  }
  else
  {
    upleftc = SDL_MapRGB(sdlscrn->format,255,255,255);
    downrightc = SDL_MapRGB(sdlscrn->format,128,128,128);
  }

  SDLGui_ObjCoord(bdlg, objnum, &coord);

  SDLGui_Draw3DBox(&coord, upleftc, downrightc);
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
    SDL_UpdateRects(sdlscrn, 1, &rect);
  }
  while(!bStopEditing);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a whole dialog.
*/
void SDLGui_DrawDialog(SGOBJ *dlg)
{
  for(int i=0; dlg[i].type!=-1; i++ )
  {
    if (dlg[i].state & SG_HIDDEN) continue;

    switch( dlg[i].type )
    {
      case SGBOX:
        SDLGui_DrawBox(dlg, i);
        break;
      case SGTEXT:
        SDLGui_DrawText(dlg, i);
        break;
      case SGEDITFIELD:
        SDLGui_DrawEditField(dlg, i);
        break;
      case SGBUTTON:
        SDLGui_DrawButton(dlg, i);
        break;
      case SGRADIOBUT:
        SDLGui_DrawRadioButton(dlg, i);
        break;
      case SGCHECKBOX:
        SDLGui_DrawCheckBox(dlg, i);
        break;
      case SGPOPUP:
        SDLGui_DrawPopupButton(dlg, i);
        break;
    }
  }
  SCRLOCK;
  SDL_UpdateRect(sdlscrn, 0,0,0,0);
  SCRUNLOCK;
}


/*-----------------------------------------------------------------------*/
/*
  Refresh display to reflect an object change.
*/
void SDLGui_RefreshObj(SGOBJ *dlg, int objnum)
{
  SDL_Rect coord;

  SDLGui_ObjCoord(dlg, objnum, &coord);

  if ((dlg[objnum].type == SGBUTTON) ||
      (dlg[objnum].type == SGBOX))
  {
    // Take border into account
    coord.x -= 3;
    coord.y -= 3;
    coord.w += 6;
    coord.h += 6;
  }

  SCRLOCK;
  SDL_UpdateRects(sdlscrn, 1, &coord);
  SCRUNLOCK;
}


/*-----------------------------------------------------------------------*/
/*
  Search an object at a certain position.
*/
int SDLGui_FindObj(SGOBJ *dlg, int fx, int fy)
{
  SDL_Rect coord;

  int len, i;
  int ob = -1;
  
  len = 0;
  while( dlg[len].type!=-1)   len++;

  /* Now search for the object: */
  for(i=len; i>0; i--)
  {
    SDLGui_ObjCoord(dlg, i, &coord);

    if ((dlg[i].type == SGBUTTON) ||
        (dlg[i].type == SGBOX))
    {
      // Take border into account
      coord.x -= 3;
      coord.y -= 3;
      coord.w += 6;
      coord.h += 6;
    }

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
  Show and process a dialog. Returns the button number that has been
  pressed. Does NOT handle SDL_QUIT - you must handle it before you
  pass the input event to the SDL GUI.
*/
int SDLGui_DoDialog(SGOBJ *dlg)
{
  int oldbutton=0;
  int retbutton=0;
  int i;
  int x, y;
  int obj;

  SDLGui_DrawDialog(dlg);

  /* Is the left mouse button still pressed? Yes -> Handle TOUCHEXIT objects here */

  // SDL_PumpEvents(); - don't call it here, it's not thread safe probably
  bool stillPressed = (SDL_GetMouseState(&x, &y) & SDL_BUTTON(1));
  obj = SDLGui_FindObj(dlg, x, y);
  if(obj>0 && (dlg[obj].flags&SG_TOUCHEXIT) )
  {
    oldbutton = obj;
    if(stillPressed)
    {
      dlg[obj].state |= SG_SELECTED;
      return obj;
    }
  }

  /* The main loop */
  do
  {
    // SDL_PumpEvents() - not necessary, the main check_event thread calls it
    SDL_Event evnt;
    if (SDL_PeepEvents(&evnt, 1, SDL_GETEVENT, SDL_EVENTMASK(SDL_USEREVENT))) {
      x = (int)evnt.user.data1;
      y = (int)evnt.user.data2;
      switch(evnt.user.code)
      {
      	case SDL_KEYDOWN:
      	  break;

        case SDL_USEREVENT:		// a signal that resolution has changed
          SDLGui_DrawDialog(dlg);	// re-draw dialog
          break;

        case SDL_MOUSEBUTTONDOWN:
          obj = SDLGui_FindObj(dlg, x, y);
          if(obj>0)
          {
            if(dlg[obj].type==SGBUTTON)
            {
              dlg[obj].state |= SG_SELECTED;
              SDLGui_DrawButton(dlg, obj);
              SDLGui_RefreshObj(dlg, obj);
              oldbutton=obj;
            }
            if( dlg[obj].flags&SG_TOUCHEXIT )
            {
              dlg[obj].state |= SG_SELECTED;
              retbutton = obj;
            }
          }
          break;

        case SDL_MOUSEBUTTONUP:
          if (dlg[oldbutton].state & SG_SELECTED)
            // User clicked on a SGBUTTON and it is still selected when
            // he release mouse button.
            retbutton=oldbutton;
          else
          {
            obj = SDLGui_FindObj(dlg, x, y);
            if(obj>0)
            {
              switch(dlg[obj].type)
              {
                case SGBUTTON:
                  break;
                case SGEDITFIELD:
                  SDLGui_EditField(dlg, obj);
                  break;
                case SGRADIOBUT:
                  // Find first radio button in this group
                  i = obj-1;
                  while (dlg[i].type==SGRADIOBUT)
                    i--;
                  i+=1;
                  // Update state
                  while (dlg[i].type==SGRADIOBUT)
                  {
                    bool updated = false;
                    if ((i == obj) && (~dlg[i].state & SG_SELECTED))
                    {
                      // Select this radio button
                      dlg[obj].state |= SG_SELECTED;
                      updated = true;
                    }
                    else if ((i != obj) && (dlg[i].state & SG_SELECTED))
                    {
                      // Deselect this radio button
                      dlg[obj].state &= ~SG_SELECTED;
                      updated = true;
                    }

                    if (updated)
                    {
                      SDLGui_DrawRadioButtonState(dlg, obj);
                      SDLGui_RefreshObj(dlg, obj);

                      updated = false;
                    }

                    i++;
                  }
                  break;
                case SGCHECKBOX:
                  dlg[obj].state ^= SG_SELECTED;
                  SDLGui_DrawCheckBoxState(dlg, obj);
                  SDLGui_RefreshObj(dlg, obj);
                  break;
                case SGPOPUP:
                  dlg[obj].state |= SG_SELECTED;
                  SDLGui_DrawPopupButton(dlg, obj);
                  SDLGui_RefreshObj(dlg, obj);
                  retbutton=obj;
                  break;
              }
            }
          }
          if(oldbutton>0)
          {
            dlg[oldbutton].state &= ~SG_SELECTED;
            SDLGui_DrawButton(dlg, oldbutton);
            SDLGui_RefreshObj(dlg, oldbutton);
            oldbutton = 0;
          }
          if( dlg[obj].flags&SG_EXIT )
          {
            retbutton = obj;
          }
          break;
      }
    }
    else
    {
      // No special event occured
      if(oldbutton>0)
      {
        // User clicked on a SGBUTTON but did not release mouse button yet.
        // Let's update button state according to mouse position
        SDL_GetMouseState(&x, &y) & SDL_BUTTON(1);
        obj = SDLGui_FindObj(dlg, x, y);

        if ((obj != oldbutton) && (dlg[oldbutton].state & SG_SELECTED))
        {
          // The button is selected but the mouse is not on it anymore.
          // Deselect it.
          dlg[oldbutton].state &= ~SG_SELECTED;
          SDLGui_DrawButton(dlg, oldbutton);
          SDLGui_RefreshObj(dlg, oldbutton);
        }
        else if ((obj == oldbutton) && (~dlg[oldbutton].state & SG_SELECTED))
        {
          // The button is deselected but the mouse moved back on it.
          // Reselect it.
          dlg[oldbutton].state |= SG_SELECTED;
          SDLGui_DrawButton(dlg, oldbutton);
          SDLGui_RefreshObj(dlg, oldbutton);
        }
      }
    }
    SDL_Delay(100);
  }
  while(retbutton==0);

  return retbutton;
}

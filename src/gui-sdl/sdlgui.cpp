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

#include <SDL.h>

#define sdlscrn	hostScreen.getPhysicalSurface()

static SDL_Surface *stdfontgfx=NULL;
static SDL_Surface *fontgfx=NULL;   /* The actual font graphics */
static int fontwidth, fontheight;   /* Height and width of the actual font */

/*-----------------------------------------------------------------------*/
/*
  Initialize the GUI.
*/
bool SDLGui_Init()
{
  /* Load the font graphics: */
  stdfontgfx = SDL_LoadBMP(guifont_path);
  if( stdfontgfx==NULL )
  {
    panicbug("Could not load font %s", guifont_path);
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
  Center a dialog so that it appears in the middle of the screen.
  Note: We only store the coordinates in the root box of the dialog,
  all other objects in the dialog are positioned relatively to this one.
*/
static void SDLGui_CenterDlg(SGOBJ *dlg)
{
  dlg[0].x = (sdlscrn->w/fontwidth-dlg[0].w)/2;
  dlg[0].y = (sdlscrn->h/fontheight-dlg[0].h)/2;
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
  int x, y;
  x = (tdlg[0].x+tdlg[objnum].x)*fontwidth;
  y = (tdlg[0].y+tdlg[objnum].y)*fontheight;
  SDLGui_Text(x, y, tdlg[objnum].txt);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a edit field object.
*/
void SDLGui_DrawEditField(SGOBJ *edlg, int objnum)
{
  int x, y;
  SDL_Rect rect;

  x = (edlg[0].x+edlg[objnum].x)*fontwidth;
  y = (edlg[0].y+edlg[objnum].y)*fontheight;
  SDLGui_Text(x, y, edlg[objnum].txt);

  rect.x = x;    rect.y = y + edlg[objnum].h * fontwidth;
  rect.w = edlg[objnum].w * fontwidth;    rect.h = 1;
  SDL_FillRect(sdlscrn, &rect, SDL_MapRGB(sdlscrn->format,160,160,160));
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog box object.
*/
void SDLGui_DrawBox(SGOBJ *bdlg, int objnum)
{
  SDL_Rect rect;
  int x, y, w, h;
  Uint32 grey = SDL_MapRGB(sdlscrn->format,192,192,192);
  Uint32 upleftc, downrightc;

  x = bdlg[objnum].x*fontwidth;
  y = bdlg[objnum].y*fontheight;
  if(objnum>0)                    /* Since the root object is a box, too, */
  {                               /* we have to look for it now here and only */
    x += bdlg[0].x*fontwidth;     /* add its absolute coordinates if we need to */
    y += bdlg[0].y*fontheight;
  }
  w = bdlg[objnum].w*fontwidth;
  h = bdlg[objnum].h*fontheight;

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

  SCRLOCK;

  rect.x = x;  rect.y = y;
  rect.w = w;  rect.h = h;
  SDL_FillRect(sdlscrn, &rect, grey);

  rect.x = x;  rect.y = y-1;
  rect.w = w;  rect.h = 1;
  SDL_FillRect(sdlscrn, &rect, upleftc);

  rect.x = x-1;  rect.y = y;
  rect.w = 1;  rect.h = h;
  SDL_FillRect(sdlscrn, &rect, upleftc);

  rect.x = x;  rect.y = y+h;
  rect.w = w;  rect.h = 1;
  SDL_FillRect(sdlscrn, &rect, downrightc);

  rect.x = x+w;  rect.y = y;
  rect.w = 1;  rect.h = h;
  SDL_FillRect(sdlscrn, &rect, downrightc);

  SCRUNLOCK;
}


/*-----------------------------------------------------------------------*/
/*
  Draw a normal button.
*/
void SDLGui_DrawButton(SGOBJ *bdlg, int objnum)
{
  int x,y;

  SDLGui_DrawBox(bdlg, objnum);

  x = (bdlg[0].x+bdlg[objnum].x+(bdlg[objnum].w-strlen(bdlg[objnum].txt))/2)*fontwidth;
  y = (bdlg[0].y+bdlg[objnum].y+(bdlg[objnum].h-1)/2)*fontheight;

  if( bdlg[objnum].state&SG_SELECTED )
  {
    x+=1;
    y+=1;
  }
  SDLGui_Text(x, y, bdlg[objnum].txt);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog radio button object.
*/
void SDLGui_DrawRadioButton(SGOBJ *rdlg, int objnum)
{
  char str[80];
  int x, y;

  x = (rdlg[0].x+rdlg[objnum].x)*fontwidth;
  y = (rdlg[0].y+rdlg[objnum].y)*fontheight;

  if( rdlg[objnum].state&SG_SELECTED )
    str[0]=SGRADIOBUTTON_SELECTED;
   else
    str[0]=SGRADIOBUTTON_NORMAL;
  str[1]=' ';
  strcpy(&str[2], rdlg[objnum].txt);

  SDLGui_Text(x, y, str);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog check box object.
*/
void SDLGui_DrawCheckBox(SGOBJ *cdlg, int objnum)
{
  char str[80];
  int x, y;

  x = (cdlg[0].x+cdlg[objnum].x)*fontwidth;
  y = (cdlg[0].y+cdlg[objnum].y)*fontheight;

  if( cdlg[objnum].state&SG_SELECTED )
    str[0]=SGCHECKBOX_SELECTED;
   else
    str[0]=SGCHECKBOX_NORMAL;
  str[1]=' ';
  strcpy(&str[2], cdlg[objnum].txt);

  SDLGui_Text(x, y, str);
}


/*-----------------------------------------------------------------------*/
/*
  Draw a dialog popup button object.
*/
void SDLGui_DrawPopupButton(SGOBJ *pdlg, int objnum)
{
  int x, y, w, h;
  const char *downstr = "\x02";

  SDLGui_DrawBox(pdlg, objnum);

  x = (pdlg[0].x+pdlg[objnum].x)*fontwidth;
  y = (pdlg[0].y+pdlg[objnum].y)*fontheight;
  w = pdlg[objnum].w*fontwidth;
  h = pdlg[objnum].h*fontheight;

  SDLGui_Text(x, y, pdlg[objnum].txt);
  SDLGui_Text(x+w-fontwidth, y, downstr);
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

  rect.x = (dlg[0].x + dlg[objnum].x) * fontwidth;
  rect.y = (dlg[0].y + dlg[objnum].y) * fontheight;
  rect.w = (dlg[objnum].w + 1) * fontwidth - 1;
  rect.h = dlg[objnum].h * fontheight;

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
  SDLGui_CenterDlg(dlg);	// center dialog (screen size might have changed)
  for(int i=0; dlg[i].type!=-1; i++ )
  {
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
  Search an object at a certain position.
*/
int SDLGui_FindObj(SGOBJ *dlg, int fx, int fy)
{
  int len, i;
  int ob = -1;
  int xpos, ypos;
  
  len = 0;
  while( dlg[len].type!=-1)   len++;

  xpos = fx/fontwidth;
  ypos = fy/fontheight;
  /* Now search for the object: */
  for(i=len; i>0; i--)
  {
    if(xpos>=dlg[0].x+dlg[i].x &&
       ypos>=dlg[0].y+dlg[i].y &&
       xpos<(int)(dlg[0].x+dlg[i].x+dlg[i].w) &&
       ypos<(int)(dlg[0].y+dlg[i].y+dlg[i].h))
    {
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
  SDL_Rect rct;
  Uint32 grey;

  grey = SDL_MapRGB(sdlscrn->format,192,192,192);

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

        case SDL_USEREVENT:			// a signal that resolution has changed
          SDLGui_DrawDialog(dlg);	// re-draw dialog (re-center it)
          break;

        case SDL_MOUSEBUTTONDOWN:
          obj = SDLGui_FindObj(dlg, x, y);
          if(obj>0)
          {
            if(dlg[obj].type==SGBUTTON)
            {
              dlg[obj].state |= SG_SELECTED;
              SDLGui_DrawButton(dlg, obj);
              SCRLOCK;
              SDL_UpdateRect(sdlscrn, (dlg[0].x+dlg[obj].x)*fontwidth-2, (dlg[0].y+dlg[obj].y)*fontheight-2,
                             dlg[obj].w*fontwidth+4, dlg[obj].h*fontheight+4);
              SCRUNLOCK;
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
          obj = SDLGui_FindObj(dlg, x, y);
          if(obj>0)
          {
            switch(dlg[obj].type)
            {
              case SGBUTTON:
                if(oldbutton==obj)
                  retbutton=obj;
                break;
              case SGEDITFIELD:
                SDLGui_EditField(dlg, obj);
                break;
              case SGRADIOBUT:
                for(i=obj-1; i>0 && dlg[i].type==SGRADIOBUT; i--)
                {
                  dlg[i].state &= ~SG_SELECTED;  /* Deselect all radio buttons in this group */
                  rct.x = (dlg[0].x+dlg[i].x)*fontwidth;
                  rct.y = (dlg[0].y+dlg[i].y)*fontheight;
                  rct.w = fontwidth;  rct.h = fontheight;
                  SCRLOCK;
                  SDL_FillRect(sdlscrn, &rct, grey); /* Clear old */
                  SCRUNLOCK;
                  SDLGui_DrawRadioButton(dlg, i);
                  SCRLOCK;
                  SDL_UpdateRects(sdlscrn, 1, &rct);
                  SCRUNLOCK;
                }
                for(i=obj+1; dlg[i].type==SGRADIOBUT; i++)
                {
                  dlg[i].state &= ~SG_SELECTED;  /* Deselect all radio buttons in this group */
                  rct.x = (dlg[0].x+dlg[i].x)*fontwidth;
                  rct.y = (dlg[0].y+dlg[i].y)*fontheight;
                  rct.w = fontwidth;  rct.h = fontheight;
                  SCRLOCK;
                  SDL_FillRect(sdlscrn, &rct, grey); /* Clear old */
                  SCRUNLOCK;
                  SDLGui_DrawRadioButton(dlg, i);
                  SCRLOCK;
                  SDL_UpdateRects(sdlscrn, 1, &rct);
                  SCRUNLOCK;
                }
                dlg[obj].state |= SG_SELECTED;  /* Select this radio button */
                rct.x = (dlg[0].x+dlg[obj].x)*fontwidth;
                rct.y = (dlg[0].y+dlg[obj].y)*fontheight;
                rct.w = fontwidth;  rct.h = fontheight;
                SCRLOCK;
                SDL_FillRect(sdlscrn, &rct, grey); /* Clear old */
                SCRUNLOCK;
                SDLGui_DrawRadioButton(dlg, obj);
                SCRLOCK;
                SDL_UpdateRects(sdlscrn, 1, &rct);
                SCRUNLOCK;
                break;
              case SGCHECKBOX:
                dlg[obj].state ^= SG_SELECTED;
                rct.x = (dlg[0].x+dlg[obj].x)*fontwidth;
                rct.y = (dlg[0].y+dlg[obj].y)*fontheight;
                rct.w = fontwidth;  rct.h = fontheight;
                SCRLOCK;
                SDL_FillRect(sdlscrn, &rct, grey); /* Clear old */
                SCRUNLOCK;
                SDLGui_DrawCheckBox(dlg, obj);
                SCRLOCK;
                SDL_UpdateRects(sdlscrn, 1, &rct);
                SCRUNLOCK;
                break;
              case SGPOPUP:
                dlg[obj].state |= SG_SELECTED;
                SDLGui_DrawPopupButton(dlg, obj);
                SCRLOCK;
                SDL_UpdateRect(sdlscrn, (dlg[0].x+dlg[obj].x)*fontwidth-2, (dlg[0].y+dlg[obj].y)*fontheight-2,
                           dlg[obj].w*fontwidth+4, dlg[obj].h*fontheight+4);
                SCRUNLOCK;
                retbutton=obj;
                break;
            }
          }
          if(oldbutton>0)
          {
            dlg[oldbutton].state &= ~SG_SELECTED;
            SDLGui_DrawButton(dlg, oldbutton);
            SCRLOCK;
            SDL_UpdateRect(sdlscrn, (dlg[0].x+dlg[oldbutton].x)*fontwidth-2, (dlg[0].y+dlg[oldbutton].y)*fontheight-2,
                           dlg[oldbutton].w*fontwidth+4, dlg[oldbutton].h*fontheight+4);
            SCRUNLOCK;
            oldbutton = 0;
          }
          if( dlg[obj].flags&SG_EXIT )
          {
            retbutton = obj;
          }
          break;
      }
    }
    SDL_Delay(100);
  }
  while(retbutton==0);

  return retbutton;
}

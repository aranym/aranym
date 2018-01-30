/*
    clipbrd_x11.cpp - X11 clipbrd interaction.

    Copyright (C) 2009 Petr Stehlik of ARAnyM dev team

    Based on (copied from) SDL_Clipboard.c that should eventually
    replace our hackish implementation.
    http://playcontrol.net/ewing/jibberjabber/SDL_ClipboardPrototype.html

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "sysdeps.h"
#include <stdio.h>
#include <limits.h>

#include "SDL_compat.h"
#include "SDL_syswm.h"
#include "clipbrd.h"
#include "host.h"
#include "maptab.h"

#define DEBUG 0
#include "debug.h"

#if defined(__unix__) \
	|| defined (_BSD_SOURCE) || defined (_SYSV_SOURCE) \
        || defined (__FreeBSD__) || defined (__MACH__) \
        || defined (__OpenBSD__) || defined (__NetBSD__) \
        || defined (__bsdi__) || defined (__svr4__) \
        || defined (bsdi) || defined (__SVR4) \
		&& !defined(__QNXNTO__) \
		&& !defined(OSX_SCRAP) \
		&& !defined(DONT_USE_X11_SCRAP)
#define X11_SCRAP
#endif

#ifdef X11_SCRAP

typedef Atom scrap_type;

static Display *SDL_Display;
static Window SDL_window;
#if SDL_VERSION_ATLEAST(2, 0, 0)
#define Lock_Display()
#define Unlock_Display()
#else
static void (*Lock_Display)(void);
static void (*Unlock_Display)(void);
#endif


int init_aclip()
{
	SDL_SysWMinfo info;
	int retval;

	retval = -1;

	SDL_VERSION(&info.version);
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if ( SDL_GetWindowWMInfo(host->video->Window(), &info) )
#else
	if ( SDL_GetWMInfo(&info) )
#endif
	{
		/* Save the information for later use */
		if ( info.subsystem == SDL_SYSWM_X11 ) {
			SDL_Display = info.info.x11.display;
			SDL_window = info.info.x11.window;
#if !SDL_VERSION_ATLEAST(2, 0, 0)
			Lock_Display = info.info.x11.lock_func;
			Unlock_Display = info.info.x11.unlock_func;
#endif

			/* Enable the special window hook events */
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

			retval = 0;
		}
		else {
			SDL_SetError("SDL is not running on X11");
		}

	}
	return retval;
}

void write_aclip(char *src, size_t srclen)
{
	Lock_Display();
	unsigned short ch;
	
	size_t count = srclen;
	unsigned char *buf = new unsigned char[srclen * 3 + 1];
	unsigned char *dst = buf;
	while ( count > 0)
	{
		ch = (unsigned char)*src++;
		if (ch == 0)
			break;
		ch = atari_to_utf16[ch];
		if (ch < 0x80)
		{
			*dst++ = ch;
		} else if (ch < 0x800)
		{
			*dst++ = ((ch >> 6) & 0x3f) | 0xc0;
			*dst++ = (ch & 0x3f) | 0x80;
		} else 
		{
			*dst++ = ((ch >> 12) & 0x0f) | 0xe0;
			*dst++ = ((ch >> 6) & 0x3f) | 0x80;
			*dst++ = (ch & 0x3f) | 0x80;
		}
		count--;
	}

	XChangeProperty(SDL_Display, DefaultRootWindow(SDL_Display), XA_CUT_BUFFER0, XA_STRING, 8, PropModeReplace, buf, dst - buf);
	if (XGetSelectionOwner(SDL_Display, XA_PRIMARY) != SDL_window)
	{
		XSetSelectionOwner(SDL_Display, XA_PRIMARY, SDL_window, CurrentTime);
		if (XGetSelectionOwner(SDL_Display, XA_PRIMARY) == SDL_window)
			XStoreBytes(SDL_Display, (char *)buf, dst - buf);
	}
	delete [] buf;
	Unlock_Display();
}

char * read_aclip(size_t *dstlen)
{
	char *dst = NULL;
	*dstlen = 0;

	Window owner;
	Atom selection;
	Atom seln_type;
	int seln_format;
	unsigned long nbytes;
	unsigned long overflow;

	Lock_Display();
	owner = XGetSelectionOwner(SDL_Display, XA_PRIMARY);
	Unlock_Display();
	if ( (owner == None) || (owner == SDL_window) ) {
		owner = DefaultRootWindow(SDL_Display);
		selection = XA_CUT_BUFFER0;
	}
	else {
		owner = SDL_window;
		Lock_Display();
		selection = XInternAtom(SDL_Display, "SDL_SELECTION", False);
		XConvertSelection(SDL_Display, XA_PRIMARY, XA_STRING,
		                  selection, owner, CurrentTime);
		Unlock_Display();
	}

	unsigned char *psrc;
	Lock_Display();
	if ( XGetWindowProperty(SDL_Display, owner, selection, 0, INT_MAX/4,
	                        False, XA_STRING, &seln_type, &seln_format,
	                        &nbytes, &overflow, &psrc) == Success ) {
		if ( seln_type == XA_STRING ) {
			char *buf = new char[nbytes + 1];
			dst = buf;
			unsigned short ch;
			unsigned short c;
			size_t bytes;
			const unsigned char *src = psrc;
			
			while ( nbytes > 0 )
			{
				c = *src;
				ch = c;
				if (ch < 0x80)
				{
					bytes = 1;
				} else if ((ch & 0xe0) == 0xc0 || nbytes < 3)
				{
					ch = ((ch & 0x1f) << 6) | (src[1] & 0x3f);
					bytes = 2;
				} else
				{
					ch = ((((ch & 0x0f) << 6) | (src[1] & 0x3f)) << 6) | (src[2] & 0x3f);
					bytes = 3;
				}
				c = utf16_to_atari[ch];
				if (c >= 0x100)
				{
					charset_conv_error(ch);
					*buf++ = '?';
				} else
				{
					*buf++ = c;
				}
				src += bytes;
				nbytes -= bytes;
			}
			*buf = '\0';
			*dstlen = buf - dst;
		}
		XFree(psrc);
	}
	Unlock_Display();

	return dst;
}

/* The system message filter function -- handle clipboard messages */
int filter_aclip(const SDL_Event *event)
{
	/* Post all non-window manager specific events */
	if ( !SDL_window || event->type != SDL_SYSWMEVENT ) {
		return 1;
	}

	/* Handle window-manager specific clipboard events */
#if SDL_VERSION_ATLEAST(2, 0, 0)
	XEvent *xevent = &event->syswm.msg->msg.x11.event;
#else
	XEvent *xevent = &event->syswm.msg->event.xevent;
#endif
	switch (event->type)
	{
		/* Copy the selection from XA_CUT_BUFFER0 to the requested property */
	case SelectionRequest: {
			XSelectionRequestEvent *req;
			XEvent sevent;
			int seln_format;
			unsigned long nbytes;
			unsigned long overflow;
			unsigned char *seln_data;

			req = &xevent->xselectionrequest;
			sevent.xselection.type = SelectionNotify;
			sevent.xselection.display = req->display;
			sevent.xselection.selection = req->selection;
			sevent.xselection.target = None;
			sevent.xselection.property = None;
			sevent.xselection.requestor = req->requestor;
			sevent.xselection.time = req->time;
			if ( XGetWindowProperty(SDL_Display, DefaultRootWindow(SDL_Display),
			                        XA_CUT_BUFFER0, 0, INT_MAX/4, False, req->target,
			                        &sevent.xselection.target, &seln_format,
			                        &nbytes, &overflow, &seln_data) == Success ) {
				if ( sevent.xselection.target == req->target ) {
					if ( sevent.xselection.target == XA_STRING ) {
						if ( seln_data[nbytes-1] == '\0' )
							--nbytes;
					}
					XChangeProperty(SDL_Display, req->requestor, req->property,
					                sevent.xselection.target, seln_format, PropModeReplace,
					                seln_data, nbytes);
					sevent.xselection.property = req->property;
				}
				XFree(seln_data);
			}
			XSendEvent(SDL_Display, req->requestor, False, 0, &sevent);
		}
		break;
	}

	/* Post the event for X11 clipboard reading above */
	return 1;
}

#else /* X11_SCRAP */
int init_aclip() { return -1; }
void write_aclip(char *src, size_t srclen) { }
char * read_aclip(size_t *dstlen) { return NULL; }
int filter_aclip(const SDL_Event *event) { return 1; }
#endif /* X11_SCRAP */

// don't remove this modeline with intended formatting for vim:ts=4:sw=4:


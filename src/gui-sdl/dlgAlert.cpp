/*
 * dlgAlert.cpp - AES-like AlertBox 
 *
 * Copyright (c) 2004-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#define MAX_LINES 20

/* The "Alert"-dialog: */
SGOBJ alertdlg[1/*BACKGROUND*/ + MAX_LINES/*text*/ + 1/*OK*/ + 1/*Cancel*/ + 1/*NULL*/] =
{
  { SGBOX, SG_BACKGROUND, 0, 0,0, 40,25, NULL }
};

SGOBJ obj_text = { SGTEXT, 0, 0, 1,1, 38,1, NULL };
SGOBJ obj_but_ok = { SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 5,5, 8,1, "OK" };
SGOBJ obj_but_cancel = { SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 24,5, 8,1, "Cancel" };
SGOBJ obj_null = { -1, 0, 0, 0,0, 0,0, NULL };

/*
   Breaks long string to several strings of max_width, divided by '\0'
   and returns the number of lines you need to display the strings.
*/
int FormatTextToBox(char *text, int max_width)
{
	int lines=1;
	int delka = strlen(text);
	char *p;		/* pointer to begin of actual line */
	char *q;		/* pointer to start of next search */
	char *rozdel = text-1;	/* pointer to last place suitable for breaking the line */
	char *konec;	/* pointer to end of the text */
	q = p = text;
	konec = text + delka;

	while(q < konec) {		/* q was last place suitable for breaking */
		char *r = strpbrk(q, " \t/\\\n");	/* find next suitable place for the break */
		if (r == NULL)
			r = konec;		/* if there's no place then point to the end */

		if ((r-p) < max_width && *r != '\n') {		/* '\n' is always used for breaking */
			rozdel = r;		/* remember new place suitable for breaking */
			q++;
			continue;		/* search again */
		}

		if ((r-p) > max_width) {	/* too long line already? */
			if (p > rozdel)				/* bad luck - no place for the delimiter. Let's do it the strong way */
				rozdel = p + max_width;		/* we loose one character */
		}
		else
			rozdel = r;			/* break in this place */

		*rozdel = '\0';			/* BREAK */
		p = q = rozdel+1;		/* next line begins here */
		lines++;				/* increment line counter */
	}
	return lines;					/* return line counter */
}



/*-----------------------------------------------------------------------*/
/*
  Show the "alert" dialog:
*/
static char *orig_t = NULL;
static int ok_but_idx = -1;
static const char *gui_alert_text = NULL;
static alert_type gui_alert_type = ALERT_OK;

enum {
	SDLGUI_ALERT_TEXT,
	SDLGUI_ALERT_TYPE
};

static void SDLGui_Alert_SetParams(int num_param, void *param)
{
	switch(num_param) {
		case SDLGUI_ALERT_TEXT:
			gui_alert_text = (const char *)param;
			break;
		case SDLGUI_ALERT_TYPE:
			gui_alert_type = *((alert_type *) param);
			break;
	}
}

static void SDLGui_Alert_Init(void)
{
	char *t = (char *)malloc(strlen(gui_alert_text)+1);
	if (orig_t) {
		free(orig_t);
		orig_t = NULL;
	}
	orig_t = t;
	int lines;
	strcpy(t, gui_alert_text);
	lines = FormatTextToBox(t, obj_text.w);
	int idx = 1;
	for(int i=0; i<lines && i<MAX_LINES; i++) {
		obj_text.y = i+1;
		obj_text.txt = t;
		alertdlg[idx++] = obj_text;
		t += strlen(t)+1;
	}
	obj_but_ok.y = lines+2;
	obj_but_cancel.y = lines+2;
	ok_but_idx = idx;
	alertdlg[idx++] = obj_but_ok;
	if (gui_alert_type == ALERT_OKCANCEL) {
		alertdlg[idx++] = obj_but_cancel;
	}
	else {
		alertdlg[ok_but_idx].x = (alertdlg[0].w - alertdlg[ok_but_idx].w) / 2;
	}
	alertdlg[idx] = obj_null;
	alertdlg[0].h = lines+4;
}

static void SDLGui_Alert_Close(void)
{
	if (orig_t) {
		free(orig_t);
		orig_t = NULL;
	}
}

bool SDLGui_Alert(const char *text, alert_type type)
{
	SDLGui_Alert_SetParams(SDLGUI_ALERT_TEXT, (void *)text);
	SDLGui_Alert_SetParams(SDLGUI_ALERT_TYPE, &type);
	SDLGui_Alert_Init();
	bool ret = (SDLGui_DoDialog(alertdlg) == ok_but_idx);
	SDLGui_Alert_Close();
	return ret;
}

/*
vim:ts=4:sw=4:
*/

/*
 * dlgAlert.cpp - AES-like AlertBox 
 *
 * Copyright (c) 2004-2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include <string.h>
#include <stdlib.h>

#include "sdlgui.h"
#include "dlgAlert.h"

#define DLG_WIDTH 76
#define MAX_LINES 20

/* The "Alert"-dialog: */
SGOBJ alertdlg[1/*BACKGROUND*/ + MAX_LINES/*text*/ + 1/*OK*/ + 1/*Cancel*/ + 1/*NULL*/] =
{
  { SGBOX, SG_BACKGROUND, 0, 0,0, DLG_WIDTH,25, NULL, 0 },
  { -1, 0, 0, 0,0, 0,0, NULL, 0 }
};

SGOBJ obj_text = { SGTEXT, 0, 0, 1,1, DLG_WIDTH-2,1, NULL, 0 };
SGOBJ obj_but_ok = { SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, (DLG_WIDTH-8-8)/3,5, 8,1, "OK", 0 };
SGOBJ obj_but_cancel = { SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, (DLG_WIDTH-8-8)*2/3+8,5, 8,1, "Cancel", 0 };
SGOBJ obj_null = { -1, 0, 0, 0,0, 0,0, NULL, 0 };

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

DlgAlert::DlgAlert(SGOBJ *dlg, const char *_text, alert_type _type)
	: Dialog(dlg), text(_text), type(_type), orig_t(NULL), ok_but_idx(-1)
{
	char *t = (char *)malloc(strlen(text)+1);
	orig_t = t;
	strcpy(t, text);
	int max_linelen = 0;
	// break long text into '\0' terminated array of strings
	int lines = FormatTextToBox(t, obj_text.w);
	if (lines > MAX_LINES)
		lines = MAX_LINES;
	// build the dialog, find the longest line
	int idx = 1;
	for(int i=0; i<lines; i++) {
		obj_text.y = i+1;
		obj_text.txt = t;
		alertdlg[idx++] = obj_text;
		int str_len = strlen(t);
		if (str_len > max_linelen)
			max_linelen = str_len;
		t += str_len + 1;
	}
	
	// compute smallest possible dialog width
	int dlg_width = obj_but_ok.w + 2;
	if (type == ALERT_OKCANCEL) {
		dlg_width += obj_but_cancel.w + 2;
	}
	if (max_linelen+2 > dlg_width)
		dlg_width = max_linelen+2;
	
	// update dialog width
	alertdlg[0].w = dlg_width;
	for(int i=0; i<lines; i++) {
		alertdlg[1+i].w = max_linelen;
	}
	
	// update OK/Cancel buttons placement
	obj_but_ok.y = lines+2;
	obj_but_cancel.y = lines+2;
	if (type == ALERT_OKCANCEL) {
		int butwidth = obj_but_ok.w + obj_but_cancel.w;
		int space = alertdlg[0].w - butwidth;
		obj_but_ok.x = space / 3;
		obj_but_cancel.x = space * 2/3 + obj_but_ok.w;
	}
	else {
		obj_but_ok.x = (alertdlg[0].w - obj_but_ok.w) / 2;
	}
	
	// add OK/Cancel buttons to dialog
	ok_but_idx = idx;
	alertdlg[idx++] = obj_but_ok;
	if (type == ALERT_OKCANCEL) {
		alertdlg[idx++] = obj_but_cancel;
	}
	alertdlg[idx] = obj_null;
	alertdlg[0].h = lines+4;
}

DlgAlert::~DlgAlert()
{
	if (orig_t) {
		free(orig_t);
		orig_t = NULL;
	}
}

bool DlgAlert::pressedOk(void)
{
	return (return_obj == ok_but_idx);
}

void DlgAlert::processResult(void)
{
}

Dialog *DlgAlertOpen(const char *text, alert_type type)
{
	return new DlgAlert(alertdlg, text, type);
}

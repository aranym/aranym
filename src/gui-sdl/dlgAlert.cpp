/*
 * dlgAlert.cpp - AES-like AlertBox 
 *
 * Copyright (c) 2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#include "sdlgui.h"

#define MAX_LINES 4

static char dlglines[MAX_LINES][37];

/* The "Alert"-dialog: */
SGOBJ alertdlg[] =
{
  { SGBOX, SG_BACKGROUND, 0, 0,0, 38,7, NULL },
  { SGTEXT, 0, 0, 1,1, 36,1, dlglines[0] },
  { SGTEXT, 0, 0, 1,2, 36,1, dlglines[1] },
  { SGTEXT, 0, 0, 1,3, 36,1, dlglines[2] },
  { SGTEXT, 0, 0, 1,4, 36,1, dlglines[3] },
  { SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 5,5, 8,1, "OK" },
  { SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 24,5, 8,1, "Cancel" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

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
	char *rozdel;	/* pointer to last place suitable for breaking the line */
	char *konec;	/* pointer to end of the text */
	q = p = text;
	rozdel = text-1;/* pointer to last line break */
	konec = text + delka;

	if (delka > max_width) {
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
	}
	return lines;					/* return line counter */
}



/*-----------------------------------------------------------------------*/
/*
  Show the "alert" dialog:
*/
int Dialog_AlertDlg(const char *text)
{
  char *t = (char *)malloc(strlen(text)+1);
  char *orig_t = t;
  static int maxlen = sizeof(dlglines[0])-1;
  int lines;
  strcpy(t, text);
  lines = FormatTextToBox(t, maxlen);
  for(int i=0; i<MAX_LINES; i++) {
  	if (i < lines) {
    	strcpy(dlglines[i], t);
    	t += strlen(t)+1;
    }
    else {
    	dlglines[i][0] = '\0';
    }
  }
  free(orig_t);
  return (SDLGui_DoDialog(alertdlg) == 5);
}

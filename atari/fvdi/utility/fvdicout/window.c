/*
 * window.c
 *
 * Basisifunktionen fÅr Fensterverwaltung.
 *
 */

#include <string.h>
#include "global.h"
#include "window.h"
#include "textwin.h"
#include "av.h"
#include "console.h"

#ifndef WM_SHADED
#define WM_SHADED				0x5758
#define WM_UNSHADED			0x5759
#define WF_SHADE				0x575D
#endif

#include "window.h"

WINDOW	*gl_topwin;		/* oberstes Fenster */
WINDOW	*gl_winlist;		/* LIFO Liste der offenen Fenster */
int	 gl_winanz = 0;		/* Anzahl der offenen Fenster */


#if 0
int alert(int def, int undo, int num)
{
	graf_mouse(ARROW, NULL);
	return do_walert(def, undo, (char *)alertarray[num], " TosWin2 ");
	return 0;
}

#endif


void call_stguide(char *data, bool with_hyp)
{
}

void send_to(char *appname, char *str)
{
}

void term_proc(TEXTWIN *t);

void term_proc(TEXTWIN *t)
{
	exit_code = 0;
}


void title_window(WINDOW *w, char *title)
{
	if (w->title)
		free(w->title);
	w->title = strdup(title);
}

void draw_winicon(WINDOW *win)
{
}

void open_window(WINDOW *v, bool as_icon)
{
	v->handle = 1;
}

void change_window_gadgets(WINDOW *w, short newkind)
{
}


static void unlink_window(WINDOW *v)
{
	WINDOW **ptr, *w;

	/* find v in the window list, and unlink it */
	ptr = &gl_winlist;
	w = *ptr;
	while (w)
	{
		if (w == v)
		{
			*ptr = v->next;
			break;
		}
		ptr = &w->next;
		w = *ptr;
	}
	free(v);
}


static void
timer_expired (WINDOW* w, int topped)
{
}

static void noslid(WINDOW *w, short m)
{
}

static bool nokey(WINDOW *w, short code, short shft)
{
	return FALSE;
}

static bool nomouse(WINDOW *w, short clicks, short x, short y, short shift, short mbuttons)
{
	return FALSE;
}

static void clear_win(WINDOW *v, short x, short y, short w, short h)
{
	short temp[4];

	set_fillcolor (0);
	temp[0] = x;
	temp[1] = y;
	temp[2] = x + w - 1;
	temp[3] = y + h - 1;
	v_bar(vdi_handle, temp);
}

static WINDOW *get_top(void)
{
	WINDOW	*v;

	v = get_window(gl_winlist->handle);
	return v;
}

static void top_win(WINDOW *w)
{
	if (gl_topwin != w)
	{
		gl_topwin = w;
	}
}

static void ontop_win(WINDOW *w)
{
	gl_topwin = w;
}

static void untop_win(WINDOW *w)
{
	gl_topwin = get_top();
}

static void bottom_win(WINDOW *w)
{
	gl_topwin = get_top();
}

static void close_win(WINDOW *w)
{
	destroy_window(w);
}

static void full_win(WINDOW *v)
{
	v->flags ^= WFULLED;
}

static void move_win(WINDOW *v, short x, short y, short w, short h)
{
}

static void size_win(WINDOW *v, short x, short y, short w, short h)
{
	move_win(v, x, y, w, h);
}

static void iconify_win(WINDOW *v, short x, short y, short w, short h)
{
  	v->old_wkind = v->kind;
	v->prev = v->work;

	v->oldmouse = v->mouseinp;
	v->mouseinp = nomouse;
	v->flags |= WICONIFIED;
	v->flags &= ~WFULLED;

	(*v->bottomed)(v);
}

static void uniconify_win(WINDOW *v, short x, short y, short w, short h)
{
	v->mouseinp = v->oldmouse;
	v->flags &= ~WICONIFIED;
	(*v->topped)(v);
}

void uniconify_all(void)						/* alle Fenster auf */
{
	WINDOW *v;

	v = gl_winlist;
	while (v)
	{
		if (v->handle >= 0)
		{
			if (v->flags & WICONIFIED)
				(*v->uniconify)(v, -1, -1, -1, -1);
		}
		v = v->next;
	}
}

static void shade_win(WINDOW *v, short flag)
{
	switch (flag)
	{
		case 1 :
			v->flags |= WSHADED;
			break;

		case 0 :
			v->flags &= ~WSHADED;
			break;

		case -1 :
			v->flags &= ~WSHADED;
			(*v->topped)(v);
			break;

		default:
			debug("window.c, shade_win():\n Unbekanntes flag %d\n", flag);
	}
}

WINDOW *create_window(char *title, short kind,
					  short wx, short wy, short ww, short wh, 	/* Grîûe zum îffnen */
					  short max_w, short max_h)			/* max. Grîûe */
{
	WINDOW *v;
	int centerwin = 0;

	title = strdup(title);
	v = malloc(sizeof(WINDOW));
	if (!v)
		return v;

	v->handle = -1;
	v->kind = kind;

	if (wx == -1 || wy == -1)
		centerwin = 1;
	if (wx < gl_desk.g_x)
		wx = gl_desk.g_x;
	if (wy < gl_desk.g_y)
		wy = gl_desk.g_y;

	v->max_w = max_w;
	v->max_h = max_h;

	v->work.g_x = wx;
	v->work.g_y = wy;
	v->work.g_w = gl_desk.g_w;
	v->work.g_h = gl_desk.g_h;

	v->title = (char*) title;
	v->extra = NULL;
	v->flags = 0;
	v->redraw = 0;

	v->draw = clear_win;
	v->topped = top_win;
	v->ontopped = ontop_win;
	v->untopped = untop_win;
	v->bottomed = bottom_win;
	v->closed = close_win;
	v->fulled = full_win;
	v->sized = size_win;
	v->moved = move_win;
	v->iconify = iconify_win;
	v->uniconify = uniconify_win;
	v->shaded = shade_win;
	v->arrowed = noslid;
	v->vslid = noslid;
	v->keyinp = nokey;
	v->mouseinp = nomouse;
	v->oldfulled = v->fulled;
	v->oldmouse = nomouse;
	v->timer = timer_expired;

	v->next = gl_winlist;
	gl_winlist = v;

	return v;
}

void destroy_window(WINDOW *v)
{
	if (v->handle < 0)	/* already closed? */
		return;
	gl_winanz--;
	v->handle = -1;

	if (v->title)
		free(v->title);

	unlink_window(v);
}

WINDOW *get_window(short handle)
{
	WINDOW *w;

	if (handle < 0)
		return NULL;

	for (w = gl_winlist; w; w = w->next)
		if (w->handle == handle)
			return w;
	return NULL;
}

void update_window_lock(void) {
}

void update_window_unlock(void) {
}

void get_window_rect(WINDOW *win, short kind, GRECT *rect) {
	switch ( kind ) {
		case WF_FIRSTXYWH:
			rect->g_x = win->work.g_x;
			rect->g_y = win->work.g_y;
			rect->g_w = win->work.g_w;
			rect->g_h = win->work.g_h;
			break;
		case WF_NEXTXYWH:
			rect->g_w = 0;
			rect->g_h = 0;
			break;
	}
}

void mouse_show(void) {
}

short mouse_hide_if_needed(GRECT *rect) {
	return 1;
}


#include <mintbind.h>
#include <string.h>
#include <support.h>
#include <unistd.h>

#include "global.h"
#include "ansicol.h"
#include "textwin.h"
#include "proc.h"
#include "av.h"
#include "version.h"
#include "event.h"
#include "console.h"
#include "environ.h"
#include "toswin2.h"

#ifdef DEBUG
# include <syslog.h>
int do_debug = 0;
#endif

extern int __mint;

#ifdef __GNUC__
long _stksize = 32768;
#endif

# ifndef _cdecl
# define _cdecl         __CDECL
# endif


long con_fd = 0;
long con_log_fd = 0;

int draw_ticks = MAX_DRAW_TICKS;

TEXTWIN *con_win;


static void global_init_fvdi(void);
static void global_term_fvdi(void);


extern void *bconout_stub(void);
static long super_func(void);
void _cdecl handle_char (long c);

extern long old_bconout;

long super_func(void)
{
   old_bconout = *(long *)0x586;
   *(long *)0x586 = (long)bconout_stub;

   return 0;
}

void _cdecl handle_char (long c)
{
	(*con_win->output)(con_win, c&0xff);
	con_win->nbytes++;
	refresh_textwin(con_win, FALSE);
}

void open_console(void)
{
	WINCFG config;

	short work_out[57];
	vq_extnd(vdi_handle, 0, work_out);
	
	config.next = NULL;
	*config.progname = 0;
	*config.arg = 0;
	*config.title = 0;
	config.kind = 0;
	config.font_id = 1;
	config.font_pts = 10;
	config.col = (work_out[0]+1) / 8;
	config.row = (work_out[1]+1-20) / 16;
	config.scroll = 0;
	config.xpos = 0;
	config.ypos = 0;
	config.width = -1;
	config.height = -1;
	config.vt_mode = MODE_VT52;
	config.autoclose = FALSE;
	config.iconified = FALSE;
	config.fg_color = 1;
	config.bg_color = 0;
	config.vdi_colors = 1;
	config.char_tab = TAB_ATARI;

	con_win = create_textwin("Console", &config);

	/* Cursor muž an, sonst kommt die Ausgabe durcheinander!! */
	(*con_win->output)(con_win, '\033');
	(*con_win->output)(con_win, 'e');

	open_window(con_win->win, FALSE);
	refresh_textwin(con_win, FALSE);
}


short needs_redraw(TEXTWIN *t)
{
	return 1;
}


static void term_tw(int ret_code)
{
	textwin_term();
	global_term_fvdi();
	exit_app(ret_code);
}


static void global_init_fvdi(void)
{
    short work_out[57];
    exit_code = 0;

    vdi_handle = open_vwork(work_out);
    font_anz = work_out[10];
    if (gl_gdos)
        font_anz += vst_load_fonts(vdi_handle, 0);

    init_ansi_colors (work_out);
}

static void global_term_fvdi(void)
{
    if (gl_gdos)
        vst_unload_fonts(vdi_handle, 0);
    v_clsvwk(vdi_handle);
}


int main(int argc, char *argv[])
{
	init_app(NULL);
	global_init_fvdi();
	open_console();

#if 0
	{
		int count = 100;
		while( count-- ) {
			// printf("test?\r\n");
			write_text(con_win, "Test \033ptest test \033qtest console\r\n", -1 );
		}
	}
	//	fflush(stdout);
#else
	// HOOK in!!!
	Supexec(super_func);

	Ptermres(250*1024, 0);
#endif

	// dummy when Ptermres fails
	term_tw(0);

	return 0;
}



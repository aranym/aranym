#include "sdlgui.h"

enum KEYBMOUSEDLG {
	box_main,
	box_wheel,
	text_wheel,
	ARROWKEYS,
	EIFFEL,
	box_altgr,
	text_altgr,
	ATARI_ALT,
	MILAN_ALTGR,
	APPLY,
	CANCEL
};

/* The keyboard dialog: */
static SGOBJ keyboarddlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 40,25, NULL },
	{ SGBOX, 0, 0, 1,2, 38,3, NULL },
	{ SGTEXT, 0, 0, 2,3, 17,1, "Host mouse wheel:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,2, 20,1, "Arrow keys" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,4, 20,1, "Eiffel scancodes" },
	{ SGBOX, 0, 0, 1,6, 38,3, NULL },
	{ SGTEXT, 0, 0, 2,7, 17,1, "Host AltGr key:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,6, 20,1, "Atari Alt" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 20,8, 20,1, "Milan AltGr" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 8,23, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 28,23, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

void Dialog_KeyboardDlg()
{
	// init
	keyboarddlg[bx_options.ikbd.wheel_eiffel ? EIFFEL : ARROWKEYS].state |= SG_SELECTED;
	keyboarddlg[bx_options.ikbd.altgr ? MILAN_ALTGR : ATARI_ALT].state |= SG_SELECTED;
	// apply
	if (SDLGui_DoDialog(keyboarddlg) == APPLY) {
		bx_options.ikbd.wheel_eiffel = (keyboarddlg[EIFFEL].state & SG_SELECTED);
		bx_options.ikbd.altgr = (keyboarddlg[MILAN_ALTGR].state & SG_SELECTED);
	}
}

/*
vim:ts=4:sw=4:
*/

/*
	dialog.cpp - base class for a dialog

	Copyright (C) 2007 ARAnyM developer team

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
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef DIALOG_H
#define DIALOG_H 1

#include "SDL_compat.h"

#include "sdlgui.h"
#include "input.h"

class Dialog;

class Dialog {
	protected:
		SGOBJ *dlg;
		cursor_state cursor;
		int return_obj;
		int last_clicked_obj, last_state;
		int touch_exit_obj;
		virtual void handleHotkey(HOTKEY) { }

	public:
		enum {
			GUI_CONTINUE,	/* continue displaying dialog */
			GUI_CLOSE,	/* close current dialog */
			GUI_WARMREBOOT,	/* reboot aranym */
			GUI_COLDREBOOT,	/* reboot aranym */
			GUI_SHUTDOWN	/* shutdown aranym */	
		};

		Dialog(SGOBJ *new_dlg);
		virtual ~Dialog();

		SGOBJ *getDialog(void);
		cursor_state *getCursor(void);

		virtual void init(void);

		virtual void mouseClick(const SDL_Event &event, int gui_x, int gui_y);
		virtual void keyPress(const SDL_Event &event);
		virtual void idle(void);
		virtual int processDialog(void);

		virtual void processResult(void);
		bool isTouchExitPressed(void);
};

#endif /* DIALOG_H */

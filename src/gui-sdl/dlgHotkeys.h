/*
	dlgHotkeys.cpp - Hotkeys dialog

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

#ifndef DLGHOTKEYS_H
#define DLGHOTKEYS_H 1

#include "dialog.h"
#include "SDL_compat.h"

class DlgKeypress;

class DlgHotkeys: public Dialog
{
	private:
		enum {
			STATE_MAIN,
			STATE_SETUP,
			STATE_QUIT,
			STATE_WARMREBOOT,
			STATE_COLDREBOOT,
			STATE_UNGRAB,
			STATE_DEBUG,
			STATE_SCREENSHOT,
			STATE_FULLSCREEN,
			STATE_SOUND
		};

		bx_hotkeys_t hotkeys;
		int state;
		DlgKeypress *dlgKeypress;

		void confirm(void);
 		void idle(void);
 		void processResult(void);

	public:
		DlgHotkeys(SGOBJ *dlg);
		~DlgHotkeys();

		int processDialog(void);
};

Dialog *DlgHotkeysOpen(void);

#endif /* DLGHOTKEYS_H */

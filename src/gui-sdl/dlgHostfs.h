/*
	dlgHostfs.cpp - HostFS dialog

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

#ifndef DLGHOSTFS_H
#define DLGHOSTFS_H 1

#include "dialog.h"

class DlgAlert;
class DlgFileSelect;

class DlgHostfs: public Dialog
{
	private:

		bx_aranymfs_options_t options;
		
		DlgFileSelect *dlgFileSelect;
		char tmpname[MAX_FILENAME_LENGTH];
		int selected;
		
		/* Do we have to update the file names in the dialog? */
		bool refreshentries;

		/* The actual selection, -1 if none selected */
		int ypos;
		bool pressed_ok, redraw;

		void confirm(void);
 		void idle(void);
		void processResult(void);
		void refreshEntries(void);
		void updateEntries(void);

	public:
		DlgHostfs(SGOBJ *dlg);
		~DlgHostfs();

		int processDialog(void);
		bool pressedOk(void);

};

Dialog *DlgHostfsOpen(void);

#endif /* DLGHOSTFS_H */


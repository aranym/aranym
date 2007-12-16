/*
	dlgFileSelect.cpp - File selection dialog

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

#ifndef DLGFILESELECT_H
#define DLGFILESELECT_H 1

#include "dialog.h"

class DlgFileSelect: public Dialog
{
	private:
		char *path_and_name;

		char file_path[MAX_FILENAME_LENGTH];
		char file_fname[128];

		/* Do we have to reload the directory file list? */
		bool reloaddir;
		/* Do we have to update the file names in the dialog? */
		bool refreshentries;

		/* The actual selection, -1 if none selected */
		int selection, ypos;
		bool eol, pressed_ok, redraw;

		void confirm(void);
 		void idle(void);
 		void processResult(void);
		void refreshEntries(void);
 
	public:
		DlgFileSelect(SGOBJ *dlg, char *new_path_and_name, bool bAllowNew);
		~DlgFileSelect();

		int processDialog(void);
		bool pressedOk(void);
};

Dialog *DlgFileSelectOpen(char *path_and_name, bool bAllowNew);

#endif /* DLGFILESELECT_H */

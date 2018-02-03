/*
	dlgOs.cpp - OS selection dialog

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

#ifndef DLGOS_H
#define DLGOS_H 1

#include "dialog.h"

class DlgFileSelect;
class DlgAlert;

class DlgOs: public Dialog
{
	private:
		enum {
			STATE_MAIN,
			STATE_FSEL_TOS,
			STATE_FSEL_EMUTOS,
			STATE_FSEL_SNAPSHOT_DIR,
			STATE_CONFIRM,
		} state;

		char tmpname[MAX_FILENAME_LENGTH];
		bx_tos_options_t tos_options;
		char snapshot_dir[sizeof(bx_options.snapshot_dir)];
		
		DlgAlert *dlgAlert;
		DlgFileSelect *dlgFileSelect;

		int processDialogMain(void);

		void processResultTos(void);
		void processResultEmutos(void);
		void processResultTosClear(void);
		void processResultEmutosClear(void);
		void processResultSnapshotDir(void);

		void confirm(void);
 
	public:
		DlgOs(SGOBJ *dlg);
		~DlgOs();

		int processDialog(void);

		void processResult(void);
};

Dialog *DlgOsOpen(void);

#endif /* DLGOS_H */

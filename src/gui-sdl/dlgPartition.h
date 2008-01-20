/*
	dlgPartition.cpp - disk dialog

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

#ifndef DLGPARTITION_H
#define DLGPARTITION_H 1

#include "dialog.h"

class DlgAlert;
class DlgFileSelect;

class DlgPartition: public Dialog
{
	private:
		enum {
			STATE_MAIN,
			STATE_FSEL,
			STATE_CDI0,
			STATE_CDI1
		};

		char *cdi_path;
		char tmpname[MAX_FILENAME_LENGTH];
		int state, cdi_disk;
		long sizeMB;

		DlgAlert *dlgAlert;
		DlgFileSelect *dlgFileSelect;

		void confirm(void);

		void selectPartPath(int disk);
		int processDialogMain(void);
		int processDialogCdi0(void);
		int processDialogCdi1(void);

		void processResultFsel(int disk);

		void init_create_disk_image(int disk);
		bool create_disk_image(void);

	public:
		DlgPartition(SGOBJ *dlg);
		~DlgPartition();

		int processDialog(void);

		void processResult(void);
};

Dialog *DlgPartitionOpen(void);

#endif /* DLGPARTITION_H */

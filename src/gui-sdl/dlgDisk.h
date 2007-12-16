/*
	dlgDisk.cpp - disk dialog

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

#ifndef DLGDISK_H
#define DLGDISK_H 1

#include "dialog.h"

class DlgAlert;
class DlgFileSelect;

class DlgDisk: public Dialog
{
	private:
		enum {
			STATE_MAIN,
			STATE_FSEL_FD0,
			STATE_FSEL_IDE0,
			STATE_FSEL_IDE1,
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

		int processDialogMain(void);
		int processDialogCdi0(void);
		int processDialogCdi1(void);

		void processResultFd0(void);
		void processResultIde0(void);
		void processResultIde1(void);

		void init_create_disk_image(int disk);
		bool create_disk_image(void);

	public:
		DlgDisk(SGOBJ *dlg);
		~DlgDisk();

		int processDialog(void);

		void processResult(void);
};

Dialog *DlgDiskOpen(void);

#endif /* DLGDISK_H */

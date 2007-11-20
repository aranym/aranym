/*
	dlgVideo.cpp - Video selection dialog

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

#ifndef DLGVIDEO_H
#define DLGVIDEO_H 1

#include "dialog.h"

class DlgVideo: public Dialog
{
	private:
		void confirm(void);
 
	public:
		DlgVideo(SGOBJ *dlg);
		~DlgVideo();

		int processDialog(void);
};

Dialog *DlgVideoOpen(void);

#endif /* DLGVIDEO_H */

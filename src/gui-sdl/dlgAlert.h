/*
	dlgAlert.cpp - alert dialog

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

#ifndef DLGALERT_H
#define DLGALERT_H 1

#include "dialog.h"

class DlgAlert: public Dialog
{
	public:
		enum {
			SDLGUI_ALERT_TEXT,
			SDLGUI_ALERT_TYPE
		};

	private:
		const char *text;
		alert_type type;
		char *orig_t;
 		int ok_but_idx;
 
		void processResult(void);
 
	public:
		DlgAlert(SGOBJ *dlg, const char *text, alert_type type);
		~DlgAlert();

		bool pressedOk(void);
};

Dialog *DlgAlertOpen(const char *text, alert_type type);

#endif /* DLGALERT_H */

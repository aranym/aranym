/*
 * dlgFileselect.cpp - dialog for selecting files (fileselector box)
 *
 * Copyright (c) 2003-2009 ARAnyM dev team (see AUTHORS)
 *
 * originally taken from the hatari project, thanks Thothy!
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include "sdlgui.h"
#include "file.h"
#include "dlgFileSelect.h"
#include <assert.h>

#define DEBUG 0
#include "debug.h"

#define DLG_WIDTH			76

#define ENTRY_COUNT 		15 /* (SGFSDLG_LASTENTRY - SGFSDLG_FIRSTENTRY + 1) */
#define ENTRY_LENGTH		(DLG_WIDTH-5)


static char dlgpath[DLG_WIDTH-1], dlgfname[ENTRY_LENGTH-2];	/* File and path name in the dialog */
static char dlgfilenames[ENTRY_COUNT][ENTRY_LENGTH + 1];

/* The dialog data: */
#define SDLGUI_INCLUDE_FSDLG
#include "sdlgui.sdl"

struct listentry {
	char *filename;
	bool directory;
	struct listentry *next;
};

/* Create a sorted list from the directory listing of path */
struct listentry *create_list(char *path)
{
	struct listentry *list = NULL;
	struct listentry *newentry;
	DIR *dd;
	struct dirent *direntry;
	char tempstr[MAX_FILENAME_LENGTH];
	struct stat filestat;

	if ((dd = opendir(path)) == NULL)
		return NULL;

	while ((direntry = readdir(dd)) != NULL) {
		// skip "." name
		if (strcmp(direntry->d_name, ".") == 0)
			continue;

		/* Allocate enough memory to store a new list entry and
		   its filemane */
		newentry =
			(struct listentry *) malloc(sizeof(struct listentry) +
										strlen(direntry->d_name) + 1);

		/* Store filename */
		newentry->filename = (char *) (newentry + 1);
		strcpy(newentry->filename, direntry->d_name);

		/* Is this entry a directory ? */
		strcpy(tempstr, path);
		strcat(tempstr, newentry->filename);
		if (stat(tempstr, &filestat) == 0 && S_ISDIR(filestat.st_mode))
			newentry->directory = true;
		else
			newentry->directory = false;

		/* Search for right place to insert new entry */
		struct listentry **prev = &list;
		struct listentry *next = list;
		while (next != NULL) {
			/* directory first, then files */
			if ((newentry->directory == true)
				&& (next->directory == false))
				break;

			/* Sort by name */
			if ((newentry->directory == next->directory)
				&& (strcmp(newentry->filename, next->filename) < 0))
				break;

			prev = &(next->next);
			next = next->next;
		}

		/* Insert new entry */
		newentry->next = next;
		*prev = newentry;
	}

	if (closedir(dd) != 0) {
		D(bug("Error on closedir."));
	}

	return list;
}

/* Free memory allocated for each member of list */
void free_list(struct listentry *list)
{
	while (list != NULL) {
		struct listentry *temp = list;
		list = list->next;
		free(temp);
	}
}

/*-----------------------------------------------------------------------*/
/*
  Show and process a file selection dialog.
  Returns TRUE if user selected "OK", FALSE if "cancel".
  bAllowNew: TRUE if the user is allowed to insert new file names.
*/
static listentry *gui_file_list = NULL;

DlgFileSelect::DlgFileSelect(SGOBJ *dlg, char *new_path_and_name, bool bAllowNew)
	: Dialog(dlg), path_and_name(new_path_and_name)
	, reloaddir(true), refreshentries(true), selection(-1), ypos(0), eol(true)
	, pressed_ok(false), redraw(true)
{
	gui_file_list = NULL;

	fsdlg[SGFSDLG_FILENAME].type = bAllowNew ? SGEDITFIELD : SGTEXT;

	/* Prepare the path and filename variables */
	File_splitpath(path_and_name, file_path, file_fname, NULL);
	if (strlen(file_path) == 0) {
		assert(getcwd(file_path, sizeof(file_path)) != NULL);
		File_AddSlashToEndFileName(file_path);
	}
	File_ShrinkName(dlgpath, file_path, sizeof(dlgpath)-1);
	File_ShrinkName(dlgfname, file_fname, sizeof(dlgfname)-1);
}

DlgFileSelect::~DlgFileSelect()
{
	if (gui_file_list) {
		free_list(gui_file_list);
		gui_file_list = NULL;
	}
}

void DlgFileSelect::confirm(void)
{
	/* if user edited filename, use new one */
	char dlgfname2[ENTRY_LENGTH-2];
	File_ShrinkName(dlgfname2, file_fname, sizeof(dlgfname2)-1);
	if (strcmp(dlgfname, dlgfname2) != 0)
		strcpy(file_fname, dlgfname);

	File_makepath(path_and_name,file_path, file_fname, NULL);
}

void DlgFileSelect::idle(void)
{
	if (reloaddir || refreshentries) {
		refreshEntries();
	}

	/* Force redraw ? */
	if (redraw) {
		init();
		redraw = false;
	}
}

int DlgFileSelect::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch (return_obj) {
		case SGFSDLG_UPDIR:
			/* Change path to parent directory */
			if (strlen(file_path) > 2) {
				File_CleanFileName(file_path);
				char *ptr = strrchr(file_path, '/');
				if (ptr)
					*(ptr + 1) = 0;
				File_AddSlashToEndFileName(file_path);
				reloaddir = true;
				/* Copy the path name to the dialog */
				File_ShrinkName(dlgpath, file_path, sizeof(dlgpath)-1);
				/* Remove old selection */
				selection = -1;
				file_fname[0] = 0;
				dlgfname[0] = 0;
				ypos = 0;
			}
			break;
		case SGFSDLG_ROOTDIR:
			/* Change to root directory */
			strcpy(file_path, "/");
			reloaddir = true;
			strcpy(dlgpath, file_path);
			/* Remove old selection */
			selection = -1;
			file_fname[0] = 0;
			dlgfname[0] = 0;
			ypos = 0;
			break;
		case SGFSDLG_UP:
			/* Scroll up */
			if (ypos > 0) {
				--ypos;
				refreshentries = true;
			}
			break;
		case SGFSDLG_DOWN:
			/* Scroll down */
			if (eol == false) {
				++ypos;
				refreshentries = true;
			}
			break;

		case SGFSDLG_OKAY:
			pressed_ok = true;
			confirm();
			/* fall through */
		case SGFSDLG_CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;

		default:
			break;
	}

	/* Has the user clicked on a file or folder? */
	if ((return_obj >= SGFSDLG_FIRSTENTRY) && (return_obj <= SGFSDLG_LASTENTRY)) {
		char tempstr[MAX_FILENAME_LENGTH];
		struct stat filestat;
		struct listentry *temp = gui_file_list;
		int i;

		strcpy(tempstr, file_path);
		for (i = 0; i < ((return_obj - SGFSDLG_FIRSTENTRY) + ypos); i++)
			temp = temp->next;
		strcat(tempstr, temp->filename);
		if (stat(tempstr, &filestat) == 0 && S_ISDIR(filestat.st_mode)) {
			/* Set the new directory */
			strcpy(file_path, tempstr);
			if (strlen(file_path) >= 3) {
				if ((file_path[strlen(file_path) - 2] == '/')
					&& (file_path[strlen(file_path) - 1] == '.'))
				{
					/* Strip a single dot at the end of the path name */
					file_path[strlen(file_path) - 2] = 0;
				}
				if ((file_path[strlen(file_path) - 3] == '/')
					&& (file_path[strlen(file_path) - 2] == '.')
					&& (file_path[strlen(file_path) - 1] == '.'))
				{
					/* Handle the ".." folder */
					char *ptr;
					if (strlen(file_path) == 3) {
						file_path[1] = 0;
					} else {
						file_path[strlen(file_path) - 3] = 0;
						ptr = strrchr(file_path, '/');
						if (ptr)
							*(ptr + 1) = 0;
					}
				}
			}
			File_AddSlashToEndFileName(file_path);
			reloaddir = true;
			/* Copy the path name to the dialog */
			File_ShrinkName(dlgpath, file_path, sizeof(dlgpath)-1);
			selection = -1;	/* Remove old selection */
			// gui_file_fname[0] = 0;
			dlgfname[0] = 0;
			ypos = 0;
		} else {
			/* Select a file */
			selection = return_obj - SGFSDLG_FIRSTENTRY + ypos;
			strcpy(file_fname, temp->filename);
			File_ShrinkName(dlgfname, file_fname, sizeof(dlgfname)-1);
		}
	}

	if (reloaddir || refreshentries) {
		refreshEntries();
	}

	return retval;
}

void DlgFileSelect::refreshEntries(void)
{
	if (reloaddir) {
		if (strlen(file_path) >= MAX_FILENAME_LENGTH) {
			fprintf(stderr, "SDLGui_FileSelect: Path name too long!\n");
			return;
		}

		free_list(gui_file_list);

		/* Load directory entries: */
		gui_file_list = create_list(file_path);
		if (gui_file_list == NULL) {
			fprintf(stderr, "SDLGui_FileSelect: Path not found.\n");
			/* reset path and reload entries */
			strcpy(file_path, "/");
			strcpy(dlgpath, file_path);
			gui_file_list = create_list(file_path);
			if (gui_file_list == NULL)
				/* we're really lost if even root is unreadable */
				return;
		}
		reloaddir = false;
		refreshentries = true;
		redraw = true;
	}

	if (refreshentries) {
		struct listentry *temp = gui_file_list;
		int i;
		for (i = 0; i < ypos; i++)
			temp = temp->next;

		/* Copy entries to dialog: */
		for (i = 0; i < ENTRY_COUNT; i++) {
			if (temp != NULL) {
				char tempstr[MAX_FILENAME_LENGTH];
				/* Prepare entries: */
				strcpy(tempstr, "  ");
				strcat(tempstr, temp->filename);
				File_ShrinkName(dlgfilenames[i], tempstr,
					ENTRY_LENGTH);
				/* Mark folders: */
				if (temp->directory)
					dlgfilenames[i][0] = SGFOLDER;
				fsdlg[SGFSDLG_FIRSTENTRY + i].flags =
					(SG_SELECTABLE | SG_EXIT | SG_RADIO);
				temp = temp->next;
			} else {
				/* Clear entry */
				dlgfilenames[i][0] = 0;
				fsdlg[SGFSDLG_FIRSTENTRY + i].flags = 0;
			}
			fsdlg[SGFSDLG_FIRSTENTRY + i].state = 0;
		}

		eol = (temp == NULL);

		refreshentries = false;
		redraw = true;
	}
}

bool DlgFileSelect::pressedOk(void)
{
	return pressed_ok;
}

void DlgFileSelect::processResult(void)
{
}

Dialog *DlgFileSelectOpen(char *path_and_name, bool bAllowNew)
{
	return new DlgFileSelect(fsdlg, path_and_name, bAllowNew);
}

// don't remove this modeline with intended formatting for vim:ts=4:sw=4:


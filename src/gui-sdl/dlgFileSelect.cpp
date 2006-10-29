/*
 * dlgFileselect.cpp - dialog for selecting files (fileselector box)
 *
 * Copyright (c) 2003-2005 ARAnyM dev team (see AUTHORS)
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

#define DEBUG 0
#include "debug.h"

#include <cstdlib>
#include <cstring>
#include <SDL.h>

#define SGFSDLG_FILENAME   5
#define SGFSDLG_UPDIR      6
#define SGFSDLG_ROOTDIR    7
#define SGFSDLG_FIRSTENTRY 10
#define SGFSDLG_LASTENTRY  24
#define SGFSDLG_UP         25
#define SGFSDLG_DOWN       26
#define SGFSDLG_OKAY       27
#define SGFSDLG_CANCEL     28

#define ENTRY_COUNT (SGFSDLG_LASTENTRY - SGFSDLG_FIRSTENTRY + 1)
#define ENTRY_LENGTH 35


static char dlgpath[39], dlgfname[33];	/* File and path name in the dialog */
static char dlgfilenames[ENTRY_COUNT][ENTRY_LENGTH + 1];

/* The dialog data: */
static SGOBJ fsdlg[] = {
	{SGBOX, SG_BACKGROUND, 0, 0, 0, 40, 25, NULL},
	{SGTEXT, 0, 0, 13, 1, 13, 1, "Choose a file"},
	{SGTEXT, 0, 0, 1, 2, 7, 1, "Folder:"},
	{SGTEXT, 0, 0, 1, 3, 38, 1, dlgpath},
	{SGTEXT, 0, 0, 1, 4, 6, 1, "File:"},
	{SGTEXT, 0, 0, 7, 4, 31, 1, dlgfname},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 31, 1, 4, 1, ".."},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 36, 1, 3, 1, "/"},
	{SGBOX, 0, 0, 1, 6, 38, 15, NULL},
	{SGBOX, 0, 0, 38, 6, 1, 15, NULL},
	{SGTEXT, 0, 0, 2, 6, ENTRY_LENGTH, 1, dlgfilenames[0]},
	{SGTEXT, 0, 0, 2, 7, ENTRY_LENGTH, 1, dlgfilenames[1]},
	{SGTEXT, 0, 0, 2, 8, ENTRY_LENGTH, 1, dlgfilenames[2]},
	{SGTEXT, 0, 0, 2, 9, ENTRY_LENGTH, 1, dlgfilenames[3]},
	{SGTEXT, 0, 0, 2, 10, ENTRY_LENGTH, 1, dlgfilenames[4]},
	{SGTEXT, 0, 0, 2, 11, ENTRY_LENGTH, 1, dlgfilenames[5]},
	{SGTEXT, 0, 0, 2, 12, ENTRY_LENGTH, 1, dlgfilenames[6]},
	{SGTEXT, 0, 0, 2, 13, ENTRY_LENGTH, 1, dlgfilenames[7]},
	{SGTEXT, 0, 0, 2, 14, ENTRY_LENGTH, 1, dlgfilenames[8]},
	{SGTEXT, 0, 0, 2, 15, ENTRY_LENGTH, 1, dlgfilenames[9]},
	{SGTEXT, 0, 0, 2, 16, ENTRY_LENGTH, 1, dlgfilenames[10]},
	{SGTEXT, 0, 0, 2, 17, ENTRY_LENGTH, 1, dlgfilenames[11]},
	{SGTEXT, 0, 0, 2, 18, ENTRY_LENGTH, 1, dlgfilenames[12]},
	{SGTEXT, 0, 0, 2, 19, ENTRY_LENGTH, 1, dlgfilenames[13]},
	{SGTEXT, 0, 0, 2, 20, ENTRY_LENGTH, 1, dlgfilenames[14]},
	{SGBUTTON, SG_SELECTABLE | SG_TOUCHEXIT, 0, 38, 6, 1, 1, "\x01"},
	/* Arrow up */
	{SGBUTTON, SG_SELECTABLE | SG_TOUCHEXIT, 0, 38, 20, 1, 1, "\x02"},
	/* Arrow down */
	{SGBUTTON, SG_SELECTABLE | SG_EXIT | SG_DEFAULT, 0, 10, 23, 8, 1,
	 "OK"},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 24, 23, 8, 1, "Cancel"},
	{-1, 0, 0, 0, 0, 0, 0, NULL}
};

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

	if (closedir(dd) != 0)
		D(bug("Error on closedir."));

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
int SDLGui_FileSelect(char *path_and_name, bool bAllowNew)
{
	int i;
	int ypos = 0;
	struct listentry *list = NULL;
	/* The actual file and path names */
	char path[MAX_FILENAME_LENGTH], fname[128];
	/* Do we have to reload the directory file list? */
	bool reloaddir = true;
	/* Do we have to update the file names in the dialog? */
	bool refreshentries = true;
	int retbut;
	int oldcursorstate;
	/* The actual selection, -1 if none selected */
	int selection = -1;
	bool eol = true;

	if (bAllowNew)
		fsdlg[SGFSDLG_FILENAME].type = SGEDITFIELD;
	else
		fsdlg[SGFSDLG_FILENAME].type = SGTEXT;

	/* Prepare the path and filename variables */
	File_splitpath(path_and_name, path, fname, NULL);
	if (strlen(path) == 0) {
		getcwd(path, sizeof(path));
		File_AddSlashToEndFileName(path);
	}
	File_ShrinkName(dlgpath, path, 38);
	File_ShrinkName(dlgfname, fname, 32);

	/* Save old mouse cursor state and enable cursor anyway */
	SCRLOCK;
	oldcursorstate = SDL_ShowCursor(SDL_QUERY);
	if (oldcursorstate == SDL_DISABLE)
		SDL_ShowCursor(SDL_ENABLE);
	SCRUNLOCK;

	do {
		if (reloaddir) {
			if (strlen(path) >= MAX_FILENAME_LENGTH) {
				fprintf(stderr,
						"SDLGui_FileSelect: Path name too long!\n");
				return false;
			}

			free_list(list);

			/* Load directory entries: */
			list = create_list(path);
			if (list == NULL) {
				fprintf(stderr, "SDLGui_FileSelect: Path not found.\n");
				/* reset path and reload entries */
				strcpy(path, "/");
				strcpy(dlgpath, path);
				list = create_list(path);
				if (list == NULL)
					/* we're really lost if even root is
					   unreadable */
					return false;
			}
			reloaddir = false;
			refreshentries = true;
		}

		if (refreshentries) {
			struct listentry *temp = list;
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
				}
				else {
					/* Clear entry */
					dlgfilenames[i][0] = 0;
					fsdlg[SGFSDLG_FIRSTENTRY + i].flags = 0;
				}
				fsdlg[SGFSDLG_FIRSTENTRY + i].state = 0;
			}

			if (temp == NULL)
				eol = true;
			else
				eol = false;

			refreshentries = false;
		}

		/* Show dialog: */
		retbut = SDLGui_DoDialog(fsdlg);

		/* Has the user clicked on a file or folder? */
		if ((retbut >= SGFSDLG_FIRSTENTRY)
			&& (retbut <= SGFSDLG_LASTENTRY)) {
			char tempstr[MAX_FILENAME_LENGTH];
			struct stat filestat;
			struct listentry *temp = list;

			strcpy(tempstr, path);
			for (int i = 0; i < ((retbut - SGFSDLG_FIRSTENTRY) + ypos);
				 i++)
				temp = temp->next;
			strcat(tempstr, temp->filename);
			if (stat(tempstr, &filestat) == 0 && S_ISDIR(filestat.st_mode)) {
				/* Set the new directory */
				strcpy(path, tempstr);
				if (strlen(path) >= 3) {
					if ((path[strlen(path) - 2] == '/')
						&& (path[strlen(path) - 1] == '.'))
						/* Strip a single dot at the
						   end of the path name */
						path[strlen(path) - 2] = 0;
					if ((path[strlen(path) - 3] == '/')
						&& (path[strlen(path) - 2] == '.')
						&& (path[strlen(path) - 1] == '.')) {
						/* Handle the ".." folder */
						char *ptr;
						if (strlen(path) == 3)
							path[1] = 0;
						else {
							path[strlen(path) - 3] = 0;
							ptr = strrchr(path, '/');
							if (ptr)
								*(ptr + 1) = 0;
						}
					}
				}
				File_AddSlashToEndFileName(path);
				reloaddir = true;
				/* Copy the path name to the dialog */
				File_ShrinkName(dlgpath, path, 38);
				selection = -1;	/* Remove old selection */
				// fname[0] = 0;
				dlgfname[0] = 0;
				ypos = 0;
			}
			else {
				/* Select a file */
				selection = retbut - SGFSDLG_FIRSTENTRY + ypos;
				strcpy(fname, temp->filename);
				File_ShrinkName(dlgfname, fname, 32);
			}
		}
		else {
			/* Has the user clicked on another button? */
			switch (retbut) {
			case SGFSDLG_UPDIR:
				/* Change path to parent directory */
				if (strlen(path) > 2) {
					char *ptr;
					File_CleanFileName(path);
					ptr = strrchr(path, '/');
					if (ptr)
						*(ptr + 1) = 0;
					File_AddSlashToEndFileName(path);
					reloaddir = true;
					/* Copy the path name to the dialog */
					File_ShrinkName(dlgpath, path, 38);
					/* Remove old selection */
					selection = -1;
					fname[0] = 0;
					dlgfname[0] = 0;
					ypos = 0;
				}
				break;
			case SGFSDLG_ROOTDIR:
				/* Change to root directory */
				strcpy(path, "/");
				reloaddir = true;
				strcpy(dlgpath, path);
				/* Remove old selection */
				selection = -1;
				fname[0] = 0;
				dlgfname[0] = 0;
				ypos = 0;
				break;
			case SGFSDLG_UP:
				/* Scroll up */
				if (ypos > 0) {
					--ypos;
					refreshentries = true;
				}
				SDL_Delay(20);
				break;
			case SGFSDLG_DOWN:
				/* Scroll down */
				if (eol == false) {
					++ypos;
					refreshentries = true;
				}
				SDL_Delay(20);
				break;
			}					/* switch */
		}						/* other button code */

	}
	while ((retbut != SGFSDLG_OKAY) && (retbut != SGFSDLG_CANCEL)
		   && (retbut != -1));

	SCRLOCK;
	if (oldcursorstate == SDL_DISABLE)
		SDL_ShowCursor(SDL_DISABLE);
	SCRUNLOCK;

	{
		/* if user edited filename, use new one */
		char dlgfname2[33];
		File_ShrinkName(dlgfname2, fname, 32);
		if (strcmp(dlgfname, dlgfname2) != 0)
			strcpy(fname, dlgfname);
	}

	File_makepath(path_and_name, path, fname, NULL);

	free_list(list);

	return (retbut == SGFSDLG_OKAY);
}

/*
vim:ts=4:sw=4:
*/

/*
 * dlgPartition.cpp - dialog for editing Partition settings
 *
 * Copyright (c) 2008 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *
 * gui-sdl original code and ideas borrowed from Hatari emulator
 * disk_image() borrowed from Bochs project, IIRC
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
#include "ata.h"
#include "tools.h"
#include "dlgPartition.h"
#include "dlgAlert.h"
#include "dlgFileSelect.h"

#define DEBUG 0
#include "debug.h"

static bx_options_t gui_options;

#define PARTS	4

static char part_id[PARTS][3+1];
static char part_size[PARTS][7];
static char part_path[PARTS][80];

/* The disks dialog: */
#define SDLGUI_INCLUDE_PARTITIONDLG
#include "sdlgui.sdl"

static int PARTS_PATH[] = { PART0_PATH, PART1_PATH, PART2_PATH, PART3_PATH };
static int PARTS_PRESENT[] = { PART0_PRESENT, PART1_PRESENT, PART2_PRESENT, PART3_PRESENT };
static int PARTS_READONLY[] = { PART0_READONLY, PART1_READONLY, PART2_READONLY, PART3_READONLY };
static int PARTS_BYTESWAP[] = { PART0_BYTESWAP, PART1_BYTESWAP, PART2_BYTESWAP, PART3_BYTESWAP };

static const char *HELP_TEXT =
	"For creating new partition image click on the [SCSIx] and select a file (or type in a new filename).\n"
	"Then set the desired partition size in MegaBytes.\n"
	"At last click on [Create] and the partition image will be created.\n"
	"\n"
	"PartID can be one of GEM, BGM, RAW, F32, LNX, SWP or $XX where XX is the HEX code of partition type (example: $83 for Linux Ext2)";

static void setState(int index, int bits, bool set)
{
	if (set)
		partitiondlg[index].state |= bits;
	else
		partitiondlg[index].state &= ~bits;
}

static void setSelected(int index, bool set)
{
	setState(index, SG_SELECTED, set);
}

static bool getSelected(int index)
{
	return (partitiondlg[index].state & SG_SELECTED);
}

static void UpdateDiskParameters(int disk)
{
	char *fname = gui_options.disks[disk].path;
	int dlgpath_idx = PARTS_PATH[disk];

	File_ShrinkName(part_path[disk], fname,	partitiondlg[dlgpath_idx].w);

	strncpy(part_id[disk], gui_options.disks[disk].partID, sizeof(part_id[disk]));

	if (fname == NULL || strlen(fname) == 0) {
		strcpy(part_size[disk], "");
		return;
	}

	setState(dlgpath_idx, SG_DISABLED, false);

	struct stat buf;
	if (stat(fname, &buf) != 0) {
		strcpy(part_size[disk], "");
		setState(dlgpath_idx, SG_DISABLED, true);
		return;
	}
	
	off_t size = 0;
	if (S_ISREG(buf.st_mode)) {
		size = buf.st_size;
	}
	int sizeMB = ((size / 1024) + 512) / 1024;

	// output
	sprintf(part_size[disk], "%6d", sizeMB);
}

void DlgPartition::init_create_disk_image(int disk)
{
	cdi_path = gui_options.disks[disk].path;
	cdi_disk = disk;
	sizeMB = atoi(part_size[disk]);
	char text[250];
	sprintf(text, "Create partition image '%s' with size %ld MB?", cdi_path, sizeMB);
	dlgAlert = (DlgAlert *) DlgAlertOpen(text, ALERT_OKCANCEL);
	SDLGui_Open(dlgAlert);
	state = STATE_CDI0;
}

extern bool make_image(long sec, const char *filename);
bool DlgPartition::create_disk_image(void)
{
	D(bug("partition: creating disk image %ld MB", sizeMB));
	long sectors = sizeMB * 2048;
	bool ret = make_image(sectors, cdi_path);
	UpdateDiskParameters(cdi_disk);
	return ret;
}

/*-----------------------------------------------------------------------*/
/*
  Show and process the disc image dialog.
*/

DlgPartition::DlgPartition(SGOBJ *dlg)
	: Dialog(dlg), cdi_path(NULL), state(STATE_MAIN), cdi_disk(-1), sizeMB(0)
{
	memset(tmpname, 0, sizeof(tmpname));

	// preload bx settings
	gui_options = bx_options;

	/* Set up dialog to actual values: */
	for(int disk = 0; disk < PARTS; disk++) {
		partitiondlg[PARTS_PATH[disk]].txt = part_path[disk];
		UpdateDiskParameters(disk);
		setSelected(PARTS_PRESENT[disk], gui_options.disks[disk].present);
		setSelected(PARTS_READONLY[disk], gui_options.disks[disk].readonly);
		setSelected(PARTS_BYTESWAP[disk], gui_options.disks[disk].byteswap);
	}

	// note that File_Exists() checks were disabled since they didn't work
	// on /dev/fd0 or /dev/cdrom if no media were inserted
}

DlgPartition::~DlgPartition()
{
}

int DlgPartition::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;
	switch(state) {
		case STATE_MAIN:
			retval = processDialogMain();
			break;
		case STATE_CDI0:
			retval = processDialogCdi0();
			break;
		case STATE_CDI1:
			retval = processDialogCdi1();
			break;
	}
	return retval;
}

void DlgPartition::selectPartPath(int disk)
{
	strcpy(tmpname, gui_options.disks[disk].path);
	SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, true));
	state = STATE_FSEL;
	cdi_disk = disk;	// let's use the cdi_disk even for selecting the path
}

int DlgPartition::processDialogMain(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch (return_obj) {
		case PART0_BROWSE:
			selectPartPath(0);
			break;
		case PART1_BROWSE:
			selectPartPath(1);
			break;
		case PART2_BROWSE:
			selectPartPath(2);
			break;
		case PART3_BROWSE:
			selectPartPath(3);
			break;

		case PART0_CLEAR:
			part_path[0][0] = '\0';
			part_size[0][0] = '\0';
			setSelected(PARTS_PRESENT[0], false);
			setSelected(PARTS_READONLY[0], false);
			setSelected(PARTS_BYTESWAP[0], false);
			break;
		case PART1_CLEAR:
			part_path[1][0] = '\0';
			part_size[1][0] = '\0';
			setSelected(PARTS_PRESENT[1], false);
			setSelected(PARTS_READONLY[1], false);
			setSelected(PARTS_BYTESWAP[1], false);
			break;
		case PART2_CLEAR:
			part_path[2][0] = '\0';
			part_size[2][0] = '\0';
			setSelected(PARTS_PRESENT[2], false);
			setSelected(PARTS_READONLY[2], false);
			setSelected(PARTS_BYTESWAP[2], false);
			break;
		case PART3_CLEAR:
			part_path[3][0] = '\0';
			part_size[3][0] = '\0';
			setSelected(PARTS_PRESENT[3], false);
			setSelected(PARTS_READONLY[3], false);
			setSelected(PARTS_BYTESWAP[3], false);
			break;

		case PART0_GENERATE:
			init_create_disk_image(0);
			break;
		case PART1_GENERATE:
			init_create_disk_image(1);
			break;
		case PART2_GENERATE:
			init_create_disk_image(2);
			break;
		case PART3_GENERATE:
			init_create_disk_image(3);
			break;

		case HELP:
			SDLGui_Open(DlgAlertOpen(HELP_TEXT, ALERT_OK));
			break;

		case APPLY:
			confirm();
			/* fall through */
		case CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;
	}

	return retval;
}

int DlgPartition::processDialogCdi0(void)
{
	D(bug("partition: dialog create disk image, step 1"));

	int retval = Dialog::GUI_CONTINUE;
	state = STATE_MAIN;

	if (dlgAlert && dlgAlert->pressedOk()) {
		if (File_Exists(cdi_path)) {
			dlgAlert = (DlgAlert *) DlgAlertOpen("File Exists. Overwrite?", ALERT_OKCANCEL);
			SDLGui_Open(dlgAlert);
		}
		else {
			dlgAlert = NULL;
		}
		state = STATE_CDI1;
	}

	return retval;
}

int DlgPartition::processDialogCdi1(void)
{
	D(bug("partition: dialog create disk image, step 2"));

	int retval = Dialog::GUI_CONTINUE;
	state = STATE_MAIN;

	if (!dlgAlert || dlgAlert->pressedOk()) {
		int present = PARTS_PRESENT[cdi_disk];

		if (create_disk_image()) {
			setSelected(present, true);
		} else {
			part_path[cdi_disk][0] = 0;
		}
	}

	return retval;
}

void DlgPartition::processResultFsel(int disk)
{
	D(bug("disk: result part"));

	if (dlgFileSelect && dlgFileSelect->pressedOk()) {
		if (!File_DoesFileNameEndWithSlash(tmpname)
		    /*&& File_Exists(tmpname) */ )
		{
			safe_strncpy(gui_options.disks[disk].path, tmpname, sizeof(gui_options.disks[0].path));
			UpdateDiskParameters(disk);
			setSelected(PARTS_PRESENT[disk], true);
		} else {
			part_path[disk][0] = 0;
		}
	}
}

void DlgPartition::confirm(void)
{
	/* Read values from dialog */
	for(int disk = 0; disk < PARTS; disk++) {
		strncpy(gui_options.disks[disk].partID, part_id[disk], sizeof(gui_options.disks[disk].partID));
		gui_options.disks[disk].present = getSelected(PARTS_PRESENT[disk]);
		gui_options.disks[disk].readonly = getSelected(PARTS_READONLY[disk]);
		gui_options.disks[disk].byteswap = getSelected(PARTS_BYTESWAP[disk]);
	}

	bx_options = gui_options;
}

void DlgPartition::processResult(void)
{
	D(bug("partition: process result, state=%d", state));

	switch(state) {
		case STATE_FSEL:
			processResultFsel(cdi_disk);
			dlgFileSelect = NULL;
			state = STATE_MAIN;
			break;
		case STATE_CDI0:
		case STATE_CDI1:
			/* Will process it on next processDialog */
			break;
		default:
			dlgFileSelect = NULL;
			dlgAlert = NULL;
			state = STATE_MAIN;
			break;
	}
}

Dialog *DlgPartitionOpen(void)
{
	return new DlgPartition(partitiondlg);
}

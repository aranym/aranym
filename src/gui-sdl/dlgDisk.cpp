/*
 * dlgDisk.cpp - dialog for editing Disk settings
 *
 * Copyright (c) 2003-2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#include "config.h"
#include "sdlgui.h"
#include "file.h"
#include "ata.h"
#include "tools.h"
#include "hardware.h"			// for getFDC()
#include "debug.h"
#include "dlgDisk.h"
#include "dlgAlert.h"
#include "dlgFileSelect.h"

bx_options_t gui_options;

// floppy
static char ide0_name[22];		// size of this array defines also the GUI edit size
static char ide1_name[22];

static char ide0_size[6], ide0_cyl[6], ide0_head[3], ide0_spt[4];
static char ide1_size[6], ide1_cyl[6], ide1_head[3], ide1_spt[4];

static char *eject = "Eject";
static char *insert = "Insert";

static char floppy_path[80];
static char ide0_path[80];
static char ide1_path[80];

#define BAR130G		130000
#define MAXHEADS	16
#define MAXSPT		255
#define MAXCYLS		65535

/* The disks dialog: */
enum DISCDLG {
	box_main,
	box_floppy,
	text_floppy,
	FLOPPY_MOUNT,
	FLP_BROWSE,
	FLP_PATH,
	box_ide0,
	text_ide0,
	IDE0_NAME,
	IDE0_MOUNT,
	IDE0_BROWSE,
	IDE0_PATH,
	IDE0_PRESENT,
	IDE0_CDROM,
	IDE0_READONLY,
	IDE0_BYTESWAP,
	text_chs0,
	IDE0_CYL,
	IDE0_HEAD,
	IDE0_SEC,
	text_size0,
	IDE0_SIZE,
	text_size0mb,
	IDE0_GENERATE,
	box_ide1,
	text_ide1,
	IDE1_NAME,
	IDE1_MOUNT,
	IDE1_BROWSE,
	IDE1_PATH,
	IDE1_PRESENT,
	IDE1_CDROM,
	IDE1_READONLY,
	IDE1_BYTESWAP,
	text_chs1,
	IDE1_CYL,
	IDE1_HEAD,
	IDE1_SEC,
	text_size1,
	IDE1_SIZE,
	text_size1mb,
	IDE1_GENERATE,
	HELP,
	APPLY,
	CANCEL
};

static SGOBJ discdlg[] = {
	{SGBOX, SG_BACKGROUND, 0, 0, 0, 40, 25, NULL},
	{SGBOX, 0, 0, 1, 2, 38, 3, NULL},
	{SGTEXT, 0, 0, 2, 1, 14, 1, "Floppy disk A:"},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 30, 2, 8, 1, NULL},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 2, 4, 5, 1, "Path:"},
	{SGTEXT, 0, 0, 8, 4, 30, 1, NULL},
	{SGBOX, 0, 0, 1, 7, 38, 6, NULL},
	{SGTEXT, 0, 0, 2, 6, 6, 1, "IDE0:"},
	{SGEDITFIELD, 0, 0, 8, 6, sizeof(ide0_name) - 1, 1, ide0_name},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 30, 6, 8, 1, NULL},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 2, 8, 5, 1, "Path:"},
	{SGTEXT, 0, 0, 8, 8, 30, 1, NULL},
	{SGCHECKBOX, SG_SELECTABLE, 0, 28, 9, 8, 1, "Present"},
	{SGCHECKBOX, SG_SELECTABLE | SG_EXIT, 0, 28, 10, 8, 1, "CDROM"},
	{SGCHECKBOX, SG_SELECTABLE, 0, 28, 11, 8, 1, "ReadOnly"},
	{SGCHECKBOX, SG_SELECTABLE, 0, 28, 12, 8, 1, "ByteSwap"},
	{SGTEXT, 0, 0, 2, 10, 10, 1, "Geo C/H/S:"},
	{SGEDITFIELD, 0, 0, 12, 10, 5, 1, ide0_cyl},
	{SGEDITFIELD, 0, 0, 19, 10, 2, 1, ide0_head},
	{SGEDITFIELD, 0, 0, 24, 10, 3, 1, ide0_spt},
	{SGTEXT, 0, 0, 2, 12, 5, 1, "Size:"},
	{SGEDITFIELD, 0, 0, 8, 12, 5, 1, ide0_size},
	{SGTEXT, 0, 0, 14, 12, 2, 1, "MB"},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 17, 12, 10, 1, "Generate"},
	{SGBOX, 0, 0, 1, 15, 38, 6, NULL},
	{SGTEXT, 0, 0, 2, 14, 6, 1, "IDE1:"},
	{SGEDITFIELD, 0, 0, 8, 14, sizeof(ide1_name) - 1, 1, ide1_name},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 30, 14, 8, 1, NULL},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 2, 16, 5, 1, "Path:"},
	{SGTEXT, 0, 0, 8, 16, 30, 1, NULL},
	{SGCHECKBOX, SG_SELECTABLE, 0, 28, 17, 8, 1, "Present"},
	{SGCHECKBOX, SG_SELECTABLE | SG_EXIT, 0, 28, 18, 8, 1, "CDROM"},
	{SGCHECKBOX, SG_SELECTABLE, 0, 28, 19, 8, 1, "ReadOnly"},
	{SGCHECKBOX, SG_SELECTABLE, 0, 28, 20, 8, 1, "ByteSwap"},
	{SGTEXT, 0, 0, 2, 18, 10, 1, "Geo C/H/S:"},
	{SGEDITFIELD, 0, 0, 12, 18, 5, 1, ide1_cyl},
	{SGEDITFIELD, 0, 0, 19, 18, 2, 1, ide1_head},
	{SGEDITFIELD, 0, 0, 24, 18, 3, 1, ide1_spt},
	{SGTEXT, 0, 0, 2, 20, 5, 1, "Size:"},
	{SGEDITFIELD, 0, 0, 8, 20, 5, 1, ide1_size},
	{SGTEXT, 0, 0, 14, 20, 2, 1, "MB"},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 17, 20, 10, 1, "Generate"},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 2, 23, 6, 1, "Help"},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT | SG_DEFAULT, 0, 20, 23, 8, 1, "Apply"},
	{SGBUTTON, SG_SELECTABLE | SG_EXIT, 0, 30, 23, 8, 1, "Cancel"},
	{-1, 0, 0, 0, 0, 0, 0, NULL}
};

static const char *HELP_TEXT =
	"For creating new disk image click on the [Path] and select a file (or type in a new filename).\n"
	"\n"
	"Then set the desired [Size:] of the new disk in MegaBytes.\n"
	"\n"
	"At last click on Generate and the disk image will be created.\n"
	"\n" "Now you can reboot to your OS and partition this new disk.";

void setState(int index, int bits, bool set)
{
	if (set)
		discdlg[index].state |= bits;
	else
		discdlg[index].state &= ~bits;
}

void setSelected(int index, bool set)
{
	setState(index, SG_SELECTED, set);
}

bool getSelected(int index)
{
	return (discdlg[index].state & SG_SELECTED);
}

static void UpdateFloppyStatus(void)
{
	discdlg[FLOPPY_MOUNT].txt = getFDC()->is_floppy_inserted()? eject : insert;
}

static void HideDiskSettings(int handle, bool state)
{
	int start = (handle == 0) ? IDE0_READONLY : IDE1_READONLY;
	int end = (handle == 0) ? IDE0_GENERATE : IDE1_GENERATE;
	// READONLY, BYTESWAP, chs, CYL, HEAD, SEC, size, SIZE, sizemb
	for (int i = start; i <= end; i++) {
		setState(i, SG_HIDDEN, state);
	}
}

static void UpdateCDROMstatus(int handle)
{
	int index = (handle == 0) ? IDE0_MOUNT : IDE1_MOUNT;
	bool isCDROM = gui_options.atadevice[0][handle].isCDROM;
	if (isCDROM) {
		discdlg[index].txt =
			bx_hard_drive.get_cd_media_status(handle) ? eject : insert;
	}
	// MOUNT button visibility
	setState(index, SG_HIDDEN, !isCDROM);
}

static void RemountCDROM(int handle)
{
	if (gui_options.atadevice[0][handle].isCDROM) {
		bool status = bx_hard_drive.get_cd_media_status(handle);
		// fprintf(stderr, "handle=%d, status=%s\n", handle, status ? "in" : "out");
		bx_hard_drive.set_cd_media_status(handle, !status);
		UpdateCDROMstatus(handle);
	}
}

static off_t DiskImageSize(const char *fname)
{
	struct stat buf;
	if (stat(fname, &buf) != 0) {
		// error reading file
		return 0;
	}
	if (!S_ISREG(buf.st_mode)) {
		// not regular file (perhaps block device?)
		return 0;
	}
	return buf.st_size;
}

static void UpdateDiskParameters(int disk, bool updateCHS)
{
	const char *fname = gui_options.atadevice[0][disk].path;
	int cyl = gui_options.atadevice[0][disk].cylinders;
	int head = gui_options.atadevice[0][disk].heads;
	int spt = gui_options.atadevice[0][disk].spt;
	off_t size = DiskImageSize(fname);
	int sizeMB = ((size / 1024) + 512) / 1024;

	if (updateCHS) {
		if (size > 0 && sizeMB <= BAR130G) {
			head = MAXHEADS;
			spt = MAXSPT;
			int divisor = 512 * head * spt;	// 512 is sector size
			cyl = size / divisor;
		}
		else {
			head = spt = cyl = 0;
		}
		gui_options.atadevice[0][disk].cylinders = cyl;
		gui_options.atadevice[0][disk].heads = head;
		gui_options.atadevice[0][disk].spt = spt;
	}

	// output
	sprintf(disk == 0 ? ide0_size : ide1_size, "%5d", sizeMB);
	sprintf(disk == 0 ? ide0_cyl : ide1_cyl, "%5d", cyl);
	sprintf(disk == 0 ? ide0_head : ide1_head, "%2d", head);
	sprintf(disk == 0 ? ide0_spt : ide1_spt, "%3d", spt);
}

/* produce the image file */
bool make_image(long sec, const char *filename)
{
	FILE *fp;

	fp = fopen(filename, "w");
	if (fp == NULL) {
		// attempt to print an error
		panicbug("make_image: error while opening '%s' for writing", filename);
		return false;
	}

	/*
	 * seek to sec*512-1 and write a single character.
	 * can't just do: fseek(fp, 512*sec-1, SEEK_SET)
	 * because 512*sec may be too large for signed int.
	 */
	while (sec > 0) {
		/* temp <-- min(sec, 4194303)
		 * 4194303 is (int)(0x7FFFFFFF/512)
		 */
		long temp = ((sec < 4194303) ? sec : 4194303);
		fseek(fp, 512 * temp, SEEK_CUR);
		sec -= temp;
	}

	fseek(fp, -1, SEEK_CUR);
	if (fputc('\0', fp) == EOF) {
		fclose(fp);
		panicbug("ERROR: The just created disk image is not complete!");
		return false;
	}

	fclose(fp);
	return true;
}

void DlgDisk::init_create_disk_image(int disk)
{
	cdi_path = gui_options.atadevice[0][disk].path;
	cdi_disk = disk;
	sizeMB = atoi(disk == 0 ? ide0_size : ide1_size);
	if (sizeMB > BAR130G)
		sizeMB = BAR130G;
	char text[250];
	sprintf(text, "Create disk image '%s' with size %ld MB?", cdi_path, sizeMB);
	SDLGui_Open(DlgAlertOpen(text, ALERT_OKCANCEL));
	state = STATE_CDI0;
}

bool DlgDisk::create_disk_image(void)
{
	// create the file
	int cyl = sizeMB * 2048 / MAXHEADS / MAXSPT;
	if (cyl > MAXCYLS) cyl = MAXCYLS;
	long sectors = cyl * MAXHEADS * MAXSPT;
	bool ret = make_image(sectors, cdi_path);
	UpdateDiskParameters(cdi_disk, true);
	return ret;
}

/*-----------------------------------------------------------------------*/
/*
  Show and process the disc image dialog.
*/

DlgDisk::DlgDisk(SGOBJ *dlg)
	: Dialog(dlg), cdi_path(NULL), state(STATE_MAIN), cdi_disk(-1), sizeMB(0)
{
	memset(tmpname, 0, sizeof(tmpname));

	// preload bx settings
	gui_options = bx_options;

	/* Set up dialog to actual values: */
	// floppy
	File_ShrinkName(floppy_path, gui_options.floppy.path, discdlg[FLP_PATH].w);
	discdlg[FLP_PATH].txt = floppy_path;

	// IDE0
	File_ShrinkName(ide0_path, gui_options.atadevice[0][0].path,
					discdlg[IDE0_PATH].w);
	discdlg[IDE0_PATH].txt = ide0_path;
	safe_strncpy(ide0_name, gui_options.atadevice[0][0].model,
				 sizeof(ide0_name));
	UpdateDiskParameters(0, false);
	setSelected(IDE0_PRESENT, gui_options.atadevice[0][0].present);
	setSelected(IDE0_CDROM, gui_options.atadevice[0][0].isCDROM);
	setSelected(IDE0_READONLY, gui_options.atadevice[0][0].readonly);
	setSelected(IDE0_BYTESWAP, gui_options.atadevice[0][0].byteswap);
	HideDiskSettings(0, gui_options.atadevice[0][0].isCDROM);

	// IDE1
	File_ShrinkName(ide1_path, gui_options.atadevice[0][1].path,
					discdlg[IDE1_PATH].w);
	discdlg[IDE1_PATH].txt = ide1_path;
	safe_strncpy(ide1_name, gui_options.atadevice[0][1].model,
				 sizeof(ide1_name));
	UpdateDiskParameters(1, false);
	setSelected(IDE1_PRESENT, gui_options.atadevice[0][1].present);
	setSelected(IDE1_READONLY, gui_options.atadevice[0][1].readonly);
	setSelected(IDE1_CDROM, gui_options.atadevice[0][1].isCDROM);
	setSelected(IDE1_BYTESWAP, gui_options.atadevice[0][1].byteswap);
	HideDiskSettings(1, gui_options.atadevice[0][1].isCDROM);

	UpdateFloppyStatus();
	UpdateCDROMstatus(0);
	UpdateCDROMstatus(1);

	// note that File_Exists() checks were disabled since they didn't work
	// on /dev/fd0 or /dev/cdrom if no media were inserted
}

DlgDisk::~DlgDisk()
{
}

int DlgDisk::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;
	switch(state) {
		case STATE_MAIN:
			retval = processDialogMain();
			break;
		case STATE_FSEL_FD0:
			retval = processDialogFd0();
			break;
		case STATE_FSEL_IDE0:
			retval = processDialogIde0();
			break;
		case STATE_FSEL_IDE1:
			retval = processDialogIde1();
			break;
		case STATE_CDI0:
			retval = processDialogCdi0();
			break;
		default:
			break;
	}
	return retval;
}

int DlgDisk::processDialogMain(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch (return_obj) {
		case FLP_BROWSE:		/* Choose a new disc A: */
			strcpy(tmpname, gui_options.floppy.path);
			SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, false));
			state = STATE_FSEL_FD0;
			break;

		case IDE0_BROWSE:
			strcpy(tmpname, gui_options.atadevice[0][0].path);
			SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, true));
			state = STATE_FSEL_IDE0;
			break;

		case IDE1_BROWSE:
			strcpy(tmpname, gui_options.atadevice[0][1].path);
			SDLGui_Open(dlgFileSelect = (DlgFileSelect*)DlgFileSelectOpen(tmpname, true));
			state = STATE_FSEL_IDE1;
			break;

		case IDE0_GENERATE:
			init_create_disk_image(0);
			break;

		case IDE1_GENERATE:
			init_create_disk_image(1);
			break;

		case IDE0_CDROM:
			HideDiskSettings(0, getSelected(IDE0_CDROM));
			break;

		case IDE1_CDROM:
			HideDiskSettings(1, getSelected(IDE1_CDROM));
			break;

		case FLOPPY_MOUNT:
			if (getFDC()->is_floppy_inserted()) {
				getFDC()->remove_floppy();
			} else {
				if (!getFDC()->insert_floppy()) {
					// report error
				}
			}
			UpdateFloppyStatus();
			break;

		case IDE0_MOUNT:
			RemountCDROM(0);
			break;

		case IDE1_MOUNT:
			RemountCDROM(1);
			break;

		case HELP:
			SDLGui_Open(DlgAlertOpen(HELP_TEXT, ALERT_OK));
			break;

		case APPLY:
			confirm();
		case CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;
	}

	return retval;
}

int DlgDisk::processDialogFd0(void)
{
	int retval = Dialog::GUI_CONTINUE;

	if (dlgFileSelect && dlgFileSelect->pressedOk()) {
		if (!File_DoesFileNameEndWithSlash(tmpname)
			/*&& File_Exists(tmpname) */ ) {
			strcpy(gui_options.floppy.path, tmpname);
			File_ShrinkName(floppy_path, tmpname, discdlg[FLP_PATH].w);
		} else {
			floppy_path[0] = 0;
		}
	}

	state = STATE_MAIN;
	return retval;
}

int DlgDisk::processDialogIde0(void)
{
	int retval = Dialog::GUI_CONTINUE;

	if (dlgFileSelect && dlgFileSelect->pressedOk()) {
		if (!File_DoesFileNameEndWithSlash(tmpname)
			/*&& File_Exists(tmpname) */ ) {
			strcpy(gui_options.atadevice[0][0].path, tmpname);
			File_ShrinkName(ide0_path, tmpname, discdlg[IDE0_PATH].w);
			UpdateDiskParameters(0, true);
			setSelected(IDE0_PRESENT, true);
		} else {
			ide0_path[0] = 0;
		}
	}

	state = STATE_MAIN;
	return retval;
}

int DlgDisk::processDialogIde1(void)
{
	int retval = Dialog::GUI_CONTINUE;

	if (dlgFileSelect && dlgFileSelect->pressedOk()) {
		if (!File_DoesFileNameEndWithSlash(tmpname)
			/*&& File_Exists(tmpname) */ ) {
			strcpy(gui_options.atadevice[0][1].path, tmpname);
			File_ShrinkName(ide1_path, tmpname, discdlg[IDE1_PATH].w);
			UpdateDiskParameters(1, true);
			setSelected(IDE1_PRESENT, true);
		} else {
			ide1_path[0] = 0;
		}
	}

	state = STATE_MAIN;
	return retval;
}

int DlgDisk::processDialogCdi0(void)
{
	int retval = Dialog::GUI_CONTINUE;
	state = STATE_MAIN;

	if (0 /* Result of asking creation of disk image */) {
		if (File_Exists(cdi_path)) {
			SDLGui_Open(DlgAlertOpen("File Exists. Overwrite?", ALERT_OKCANCEL));
			state = STATE_CDI1;
		} else {
			/* Tell it like we overwrite */
			retval = processDialogCdi1();
		}
	}

	return retval;
}

int DlgDisk::processDialogCdi1(void)
{
	int retval = Dialog::GUI_CONTINUE;

	if (0 /* Result of asking to overwrite disk image */) {
		char *ide_path = (cdi_disk==0) ? ide0_path : ide1_path;
		int ide_select = (cdi_disk==0) ? IDE0_PRESENT : IDE1_PRESENT;
		int ide_numpath = (cdi_disk==0) ? IDE0_PATH : IDE1_PATH;

		if (create_disk_image()) {
			File_ShrinkName(ide_path, gui_options.atadevice[0][cdi_disk].path,
							discdlg[ide_numpath].w);
			setSelected(ide_select, true);
		} else {
			ide_path[0] = 0;
		}
	}

	state = STATE_MAIN;
	return retval;
}

void DlgDisk::confirm(void)
{
	/* Read values from dialog */
	int cyl, head, spt;
	safe_strncpy(gui_options.atadevice[0][0].model, ide0_name,
				 sizeof(gui_options.atadevice[0][0].model));
	sscanf(ide0_cyl, "%d", &cyl);
	sscanf(ide0_head, "%d", &head);
	sscanf(ide0_spt, "%d", &spt);
	if (cyl > MAXCYLS) cyl = MAXCYLS;
	if (head > MAXHEADS) head = MAXHEADS;
	if (spt > MAXSPT) spt = MAXSPT;
	gui_options.atadevice[0][0].cylinders = cyl;
	gui_options.atadevice[0][0].heads = head;
	gui_options.atadevice[0][0].spt = spt;
	gui_options.atadevice[0][0].present = getSelected(IDE0_PRESENT);
	gui_options.atadevice[0][0].readonly = getSelected(IDE0_READONLY);
	gui_options.atadevice[0][0].isCDROM = getSelected(IDE0_CDROM);
	gui_options.atadevice[0][0].byteswap = getSelected(IDE0_BYTESWAP);

	safe_strncpy(gui_options.atadevice[0][1].model, ide1_name,
				 sizeof(gui_options.atadevice[0][1].model));
	sscanf(ide1_cyl, "%d", &cyl);
	sscanf(ide1_head, "%d", &head);
	sscanf(ide1_spt, "%d", &spt);
	if (cyl > MAXCYLS) cyl = MAXCYLS;
	if (head > MAXHEADS) head = MAXHEADS;
	if (spt > MAXSPT) spt = MAXSPT;
	gui_options.atadevice[0][1].cylinders = cyl;
	gui_options.atadevice[0][1].heads = head;
	gui_options.atadevice[0][1].spt = spt;
	gui_options.atadevice[0][1].present = getSelected(IDE1_PRESENT);
	gui_options.atadevice[0][1].readonly = getSelected(IDE1_READONLY);
	gui_options.atadevice[0][1].isCDROM = getSelected(IDE1_CDROM);
	gui_options.atadevice[0][1].byteswap = getSelected(IDE1_BYTESWAP);

	bx_options = gui_options;
}

void DlgDisk::processResult(void)
{
}

Dialog *DlgDiskOpen(void)
{
	return new DlgDisk(discdlg);
}

#include "sdlgui.h"
#include "file.h"
#include "ata.h"
#include "tools.h"

extern bx_options_t gui_options;

// floppy
extern void remove_floppy();
extern bool insert_floppy();
extern bool is_floppy_inserted();

static char ide0_name[22];	// size of this array defines also the GUI edit size
static char ide1_name[22];

static char ide0_size[6], ide0_cyl[6], ide0_head[4], ide0_spt[3];
static char ide1_size[6], ide1_cyl[6], ide1_head[4], ide1_spt[3];

static char *eject = "Eject";
static char *insert = "Insert";

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
	IDE0_READONLY,
	IDE0_BYTESWAP,
	IDE0_CDROM,
	text_chs0,
	IDE0_CYL,
	IDE0_HEAD,
	IDE0_SEC,
	text_size0,
	IDE0_SIZE,
	text_size0mb,
//	IDE0_GENERATE,
	box_ide1,
	text_ide1,
	IDE1_NAME,
	IDE1_MOUNT,
	IDE1_BROWSE,
	IDE1_PATH,
	IDE1_PRESENT,
	IDE1_READONLY,
	IDE1_BYTESWAP,
	IDE1_CDROM,
	text_chs1,
	IDE1_CYL,
	IDE1_HEAD,
	IDE1_SEC,
	text_size1,
	IDE1_SIZE,
	text_size1mb,
//	IDE1_GENERATE,
	EXIT
};

SGOBJ discdlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGBOX, 0, 0, 1,1, 38,4, NULL },
  { SGTEXT, 0, 0, 2,1, 14,1, "Floppy disk A:" },
  { SGBUTTON, 0, 0, 30,1, 8,1, NULL },
  { SGBUTTON, 0, 0, 2,3, 5,1, "Path:" },
  { SGTEXT, 0, 0, 7,3, 31,1, NULL },
  { SGBOX, 0, 0,		  1,7, 38,7, NULL },
  { SGTEXT, 0, 0,		  2,7, 5,1, "IDE0:" },
  { SGEDITFIELD, 0, 0,	  8,7, sizeof(ide0_name)-1,1, ide0_name},
  { SGBUTTON, 0, 0,		 30,7, 8,1, NULL },
  { SGBUTTON, 0, 0,		  2,9, 5,1, "Path:" },
  { SGTEXT, 0, 0,		  7,9, 31,1, NULL },
  { SGCHECKBOX, 0, 0,	28,10, 8,1, "Present" },
  { SGCHECKBOX, 0, 0,	28,11, 8,1, "ReadOnly" },
  { SGCHECKBOX, 0, 0,	28,12, 8,1, "ByteSwap" },
  { SGCHECKBOX, 0, 0,	28,13, 8,1, "CDROM" },
  { SGTEXT, 0, 0,		 2,11, 10,1, "Geo C/H/S:" },
  { SGEDITFIELD, 0, 0,	12,11, 5,1, ide0_cyl},
  { SGEDITFIELD, 0, 0,	19,11, 3,1, ide0_head},
  { SGEDITFIELD, 0, 0,	24,11, 2,1, ide0_spt},
  { SGTEXT, 0, 0,		 2,13, 5,1, "Size:" },
  { SGEDITFIELD, 0, 0,	 8,13, 5,1, ide0_size},
  { SGTEXT, 0, 0,		14,13, 2,1, "MB" },
 // { SGBUTTON, 0, 0,		17,13, 10,1, "Generate" },
  { SGBOX, 0, 0,		 1,15, 38,7, NULL },
  { SGTEXT, 0, 0,		 2,15, 5,1, "IDE1:" },
  { SGEDITFIELD, 0, 0,	 8,15, sizeof(ide1_name)-1,1, ide1_name},
  { SGBUTTON, 0, 0,		30,15, 8,1, NULL },
  { SGBUTTON, 0, 0,		 2,17, 5,1, "Path:" },
  { SGTEXT, 0, 0,		 7,17, 31,1, NULL },
  { SGCHECKBOX, 0, 0,	28,18, 8,1, "Present" },
  { SGCHECKBOX, 0, 0,	28,19, 8,1, "ReadOnly" },
  { SGCHECKBOX, 0, 0,	28,20, 8,1, "ByteSwap" },
  { SGCHECKBOX, 0, 0,	28,21, 8,1, "CDROM" },
  { SGTEXT, 0, 0,		 2,19, 10,1, "Geo C/H/S:" },
  { SGEDITFIELD, 0, 0,	12,19, 5,1, ide1_cyl},
  { SGEDITFIELD, 0, 0,	19,19, 3,1, ide1_head},
  { SGEDITFIELD, 0, 0,	24,19, 2,1, ide1_spt},
  { SGTEXT, 0, 0,		 2,21, 5,1, "Size:" },
  { SGEDITFIELD, 0, 0,	 8,21, 5,1, ide1_size},
  { SGTEXT, 0, 0,		14,21, 2,1, "MB" },
 // { SGBUTTON, 0, 0,		17,21, 10,1, "Generate" },
  { SGBUTTON, 0, 0,		10,23, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

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

void UpdateFloppyStatus(void)
{
	discdlg[FLOPPY_MOUNT].txt = is_floppy_inserted() ? eject : insert;
}

void UpdateCDROMstatus(int handle)
{
	int index = (handle == 0) ? IDE0_MOUNT : IDE1_MOUNT;
  	if (gui_options.atadevice[0][handle].isCDROM) {
		discdlg[index].txt = bx_hard_drive.get_cd_media_status(handle) ? eject : insert;
		discdlg[index].w = 8;
		discdlg[index].h = 1;
	}
	else {
		// make it invisible, if possible
		discdlg[index].txt = "";
		discdlg[index].w = 0;
		discdlg[index].h = 0;
	}
}

void RemountCDROM(int handle)
{
  	if (gui_options.atadevice[0][handle].isCDROM) {
		bool status = bx_hard_drive.get_cd_media_status(handle);
		// fprintf(stderr, "handle=%d, status=%s\n", handle, status ? "in" : "out");
		bx_hard_drive.set_cd_media_status(handle, ! status);
		UpdateCDROMstatus(handle);
	}
}

/*-----------------------------------------------------------------------*/
/*
  Show and process the disc image dialog.
*/
void Dialog_DiscDlg(void)
{
  int but;
  char tmpname[MAX_FILENAME_LENGTH];
  char floppy_path[80]="";
  char ide0_path[80]="";
  char ide1_path[80]="";

  /* Set up dialog to actual values: */
  // floppy
  File_ShrinkName(floppy_path, gui_options.floppy.path, discdlg[FLP_PATH].w);
  discdlg[FLP_PATH].txt = floppy_path;

  // IDE0
  File_ShrinkName(ide0_path, gui_options.atadevice[0][0].path, discdlg[IDE0_PATH].w);
  discdlg[IDE0_PATH].txt = ide0_path;
  safe_strncpy(ide0_name, gui_options.atadevice[0][0].model, sizeof(ide0_name));
  int cyl = gui_options.atadevice[0][0].cylinders;
  int head = gui_options.atadevice[0][0].heads;
  int spt = gui_options.atadevice[0][0].spt;
  long size = (((long)cyl * head * spt / 1024)+1)/2;	// round up to MB
  sprintf(ide0_size, "%5ld", size);
  sprintf(ide0_cyl, "%5d", cyl);
  sprintf(ide0_head, "%3d", head);
  sprintf(ide0_spt, "%2d", spt);
  setSelected(IDE0_PRESENT, gui_options.atadevice[0][0].present);
  setSelected(IDE0_READONLY, gui_options.atadevice[0][0].readonly);
  setSelected(IDE0_CDROM, gui_options.atadevice[0][0].isCDROM);
  setSelected(IDE0_BYTESWAP, gui_options.atadevice[0][0].byteswap);

  // IDE1
  File_ShrinkName(ide1_path, gui_options.atadevice[0][1].path, discdlg[IDE1_PATH].w);
  discdlg[IDE1_PATH].txt = ide1_path;
  safe_strncpy(ide1_name, gui_options.atadevice[0][1].model, sizeof(ide1_name));
  cyl = gui_options.atadevice[0][1].cylinders;
  head = gui_options.atadevice[0][1].heads;
  spt = gui_options.atadevice[0][1].spt;
  size = (((long)cyl * head * spt / 1024)+1)/2;	// round up to MB
  sprintf(ide1_size, "%5ld", size);
  sprintf(ide1_cyl, "%5d", cyl);
  sprintf(ide1_head, "%3d", head);
  sprintf(ide1_spt, "%2d", spt);
  setSelected(IDE1_PRESENT, gui_options.atadevice[0][1].present);
  setSelected(IDE1_READONLY, gui_options.atadevice[0][1].readonly);
  setSelected(IDE1_CDROM, gui_options.atadevice[0][1].isCDROM);
  setSelected(IDE1_BYTESWAP, gui_options.atadevice[0][1].byteswap);

  UpdateFloppyStatus();
  UpdateCDROMstatus(0);
  UpdateCDROMstatus(1);

  // note that File_Exists() checks were disabled since they didn't work
  // on /dev/fd0 or /dev/cdrom if no media were inserted

  /* Draw and process the dialog */
  do
  {
    but = SDLGui_DoDialog(discdlg);
    switch(but)
    {
      case FLP_BROWSE:   /* Choose a new disc A: */
        strcpy(tmpname, gui_options.floppy.path);
        if( SDLGui_FileSelect(tmpname, false) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname)/*&& File_Exists(tmpname)*/)
          {
            File_ShrinkName(floppy_path, tmpname, discdlg[FLP_PATH].w);
          }
          else
          {
            floppy_path[0] = 0;
          }
        }
        break;

      case IDE0_BROWSE:
        strcpy(tmpname, gui_options.atadevice[0][0].path);
        if( SDLGui_FileSelect(tmpname, false) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname)/*&& File_Exists(tmpname)*/) {
            strcpy(gui_options.atadevice[0][0].path, tmpname);
            File_ShrinkName(ide0_path, tmpname, discdlg[IDE0_PATH].w);
          }
          else {
          	ide0_path[0] = 0;
          }
        }
        break;

      case IDE1_BROWSE:
        strcpy(tmpname, gui_options.atadevice[0][1].path);
        if( SDLGui_FileSelect(tmpname, false) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname)/*&& File_Exists(tmpname)*/) {
            strcpy(gui_options.atadevice[0][1].path, tmpname);
            File_ShrinkName(ide1_path, tmpname, discdlg[IDE1_PATH].w);
          }
          else {
          	ide1_path[0] = 0;
          }
        }
        break;

      case FLOPPY_MOUNT:
		if (is_floppy_inserted()) {
			remove_floppy();
		}
		else {
			if (! insert_floppy()) {
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
    }
  }
  while(but!=EXIT);

  /* Read values from dialog */
  safe_strncpy(gui_options.atadevice[0][0].model, ide0_name, sizeof(gui_options.atadevice[0][0].model));
  sscanf(ide0_cyl, "%d", &cyl);
  sscanf(ide0_head, "%d", &head);
  sscanf(ide0_spt, "%d", &spt);
  gui_options.atadevice[0][0].cylinders = cyl;
  gui_options.atadevice[0][0].heads = head;
  gui_options.atadevice[0][0].spt = spt;
  gui_options.atadevice[0][0].present = getSelected(IDE0_PRESENT);
  gui_options.atadevice[0][0].readonly = getSelected(IDE0_READONLY);
  gui_options.atadevice[0][0].isCDROM = getSelected(IDE0_CDROM);
  gui_options.atadevice[0][0].byteswap = getSelected(IDE0_BYTESWAP);

  safe_strncpy(gui_options.atadevice[0][1].model, ide1_name, sizeof(gui_options.atadevice[0][1].model));
  sscanf(ide1_cyl, "%d", &cyl);
  sscanf(ide1_head, "%d", &head);
  sscanf(ide1_spt, "%d", &spt);
  gui_options.atadevice[0][1].cylinders = cyl;
  gui_options.atadevice[0][1].heads = head;
  gui_options.atadevice[0][1].spt = spt;
  gui_options.atadevice[0][1].present = getSelected(IDE1_PRESENT);
  gui_options.atadevice[0][1].readonly = getSelected(IDE1_READONLY);
  gui_options.atadevice[0][1].isCDROM = getSelected(IDE1_CDROM);
  gui_options.atadevice[0][1].byteswap = getSelected(IDE1_BYTESWAP);
}

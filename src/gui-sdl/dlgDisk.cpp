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

static char *eject = "Eject";
static char *insert = "Insert";

/* The disks dialog: */
enum DISCDLG {
	box_main,
	box_floppy,
	text_floppy,
	FLP_BROWSE,
	FLP_PATH,
	box_ide0,
	text_ide0,
	IDE0_NAME,
	IDE0_ACTIVE,
	IDE0_BROWSE,
	IDE0_PATH,
	IDE0_READONLY,
	IDE0_CDROM,
	IDE0_BYTESWAP,
	box_ide1,
	text_ide1,
	IDE1_NAME,
	IDE1_ACTIVE,
	IDE1_BROWSE,
	IDE1_PATH,
	IDE1_READONLY,
	IDE1_CDROM,
	IDE1_BYTESWAP,
	text_floppymount,
	FLOPPY_MOUNT,
	text_cdrommount,
	CDROM_MOUNT,
	EXIT
};

SGOBJ discdlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGBOX, 0, 0, 1,1, 38,4, NULL },
  { SGTEXT, 0, 0, 2,1, 14,1, "Floppy disk A:" },
  { SGBUTTON, 0, 0, 2,3, 5,1, "Path:" },
  { SGTEXT, 0, 0, 7,3, 31,1, NULL },
  { SGBOX, 0, 0, 1,6, 38,6, NULL },
  { SGTEXT, 0, 0, 2,6, 5,1, "IDE0:" },
  { SGEDITFIELD, 0, 0, 8,6, sizeof(ide0_name)-1,1, ide0_name},
  { SGCHECKBOX, 0, 0, 30,6, 8,1, "Active" },
  { SGBUTTON, 0, 0, 2,8, 5,1, "Path:" },
  { SGTEXT, 0, 0, 7,8, 31,1, NULL },
  { SGCHECKBOX, 0, 0, 2,10, 10,1, "Read Only" },
  { SGCHECKBOX, 0, 0, 17,10, 6,1, "CDROM" },
  { SGCHECKBOX, 0, 0, 28,10, 8,1, "ByteSwap" },
  { SGBOX, 0, 0, 1,13, 38,6, NULL },
  { SGTEXT, 0, 0, 2,13, 5,1, "IDE1:" },
  { SGEDITFIELD, 0, 0, 8,13, sizeof(ide1_name)-1,1, ide1_name},
  { SGCHECKBOX, 0, 0, 30,13, 8,1, "Active" },
  { SGBUTTON, 0, 0, 2,15, 5,1, "Path:" },
  { SGTEXT, 0, 0, 7,15, 31,1, NULL },
  { SGCHECKBOX, 0, 0, 2,17, 10,1, "Read Only" },
  { SGCHECKBOX, 0, 0, 17,17, 6,1, "CDROM" },
  { SGCHECKBOX, 0, 0, 28,17, 8,1, "ByteSwap" },
  { SGTEXT, 0, 0, 2,20, 6,1, "Floppy" },
  { SGBUTTON, 0, 0, 9,20, 8,1, NULL },
  { SGTEXT, 0, 0, 22,20, 6,1, "CD-ROM" },
  { SGBUTTON, 0, 0, 29,20, 8,1, NULL },
  { SGBUTTON, 0, 0, 10,23, 20,1, "Back to main menu" },
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

void UpdateCDROMstatus(void)
{
	int handle = bx_hard_drive.get_first_cd_handle();
	discdlg[CDROM_MOUNT].txt = bx_hard_drive.get_cd_media_status(handle) ? eject : insert;
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
  setSelected(IDE0_ACTIVE, gui_options.atadevice[0][0].present);
  setSelected(IDE0_READONLY, gui_options.atadevice[0][0].readonly);
  setSelected(IDE0_CDROM, gui_options.atadevice[0][0].isCDROM);
  setSelected(IDE0_BYTESWAP, gui_options.atadevice[0][0].byteswap);

  // IDE1
  File_ShrinkName(ide1_path, gui_options.atadevice[0][1].path, discdlg[IDE1_PATH].w);
  discdlg[IDE1_PATH].txt = ide1_path;
  safe_strncpy(ide1_name, gui_options.atadevice[0][1].model, sizeof(ide1_name));
  setSelected(IDE1_ACTIVE, gui_options.atadevice[0][1].present);
  setSelected(IDE1_READONLY, gui_options.atadevice[0][1].readonly);
  setSelected(IDE1_CDROM, gui_options.atadevice[0][1].isCDROM);
  setSelected(IDE1_BYTESWAP, gui_options.atadevice[0][1].byteswap);

  UpdateFloppyStatus();
  UpdateCDROMstatus();

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

      case CDROM_MOUNT:
        {
		int handle = bx_hard_drive.get_first_cd_handle();
		bool status = bx_hard_drive.get_cd_media_status(handle);
		fprintf(stderr, "handle=%d, status=%s\n", handle, status ? "in" : "out");
        bx_hard_drive.set_cd_media_status(handle, ! status);
        UpdateCDROMstatus();
		}

        break;
    }
  }
  while(but!=EXIT);

  /* Read values from dialog */
  safe_strncpy(gui_options.atadevice[0][0].model, ide0_name, sizeof(gui_options.atadevice[0][0].model));
  gui_options.atadevice[0][0].present = getSelected(IDE0_ACTIVE);
  gui_options.atadevice[0][0].readonly = getSelected(IDE0_READONLY);
  gui_options.atadevice[0][0].isCDROM = getSelected(IDE0_CDROM);
  gui_options.atadevice[0][0].byteswap = getSelected(IDE0_BYTESWAP);

  safe_strncpy(gui_options.atadevice[0][1].model, ide1_name, sizeof(gui_options.atadevice[0][1].model));
  gui_options.atadevice[0][1].present = getSelected(IDE1_ACTIVE);
  gui_options.atadevice[0][1].readonly = getSelected(IDE1_READONLY);
  gui_options.atadevice[0][1].isCDROM = getSelected(IDE1_CDROM);
  gui_options.atadevice[0][1].byteswap = getSelected(IDE1_BYTESWAP);
}

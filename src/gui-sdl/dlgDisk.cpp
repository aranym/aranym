#include "sdlgui.h"
#include "file.h"
#include "ata.h"

extern bx_options_t gui_options;

/* The disks dialog: */
enum DISCDLG {
	box_main,
	box_floppy,
	text_floppy,
	FLP_BROWSE,
	FLP_PATH,
	FLP_ACTIVE,
	box_ide0,
	text_ide0,
	IDE0_BROWSE,
	IDE0_PATH,
	text_type0,
	IDE0_HDD,
	IDE0_CDROM,
	IDE0_MOUNT,
	text_state0,
	IDE0_RDONLY,
	IDE0_ACTIVE,
	box_ide1,
	text_ide1,
	IDE1_BROWSE,
	IDE1_PATH,
	text_type1,
	IDE1_HDD,
	IDE1_CDROM,
	text_state1,
	IDE1_RDONLY,
	IDE1_ACTIVE,
	IDE1_MOUNT,
	EXIT
};

SGOBJ discdlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGBOX, 0, 0, 1,1, 38,5, NULL },
  { SGTEXT, 0, 0, 14,1, 14,1, "Floppy disk A:" },
  { SGBUTTON, 0, 0, 2,3, 5,1, "Path:" },
  { SGTEXT, 0, 0, 7,3, 31,1, NULL },
  { SGCHECKBOX, 0, 0, 2,5, 8,1, "Active" },
  { SGBOX, 0, 0, 1,7, 38,7, NULL },
  { SGTEXT, 0, 0, 8,7, 33,1, "Hard disk IDE0 (Master)" },
  { SGBUTTON, 0, 0, 2,9, 5,1, "Path:" },
  { SGTEXT, 0, 0, 7,9, 31,1, NULL },
  { SGTEXT, 0, 0, 2,11, 6,1, "Type:" },
  { SGRADIOBUT, 0, 0, 9,11, 6,1, "HDD" },
  { SGRADIOBUT, 0, 0, 15,11, 6,1, "CD-ROM" },
  { SGBUTTON, 0, 0, 29,11, 8,1, "Mount" },
  { SGTEXT, 0, 0, 2,13, 6,1, "State:" },
  { SGCHECKBOX, 0, 0, 9,13, 10,1, "Read Only" },
  { SGCHECKBOX, 0, 0, 22,13, 8,1, "Active" },
  { SGBOX, 0, 0, 1,15, 38,7, NULL },
  { SGTEXT, 0, 0, 8,15, 33,1, "Hard disk IDE1 (Slave)" },
  { SGBUTTON, 0, 0, 2,17, 5,1, "Path:" },
  { SGTEXT, 0, 0, 7,17, 31,1, NULL },
  { SGTEXT, 0, 0, 2,19, 6,1, "Type:" },
  { SGRADIOBUT, 0, 0, 2,20, 6,1, "HDD" },
  { SGRADIOBUT, 0, 0, 2,21, 6,1, "CD-ROM" },
  { SGTEXT, 0, 0, 15,19, 6,1, "State:" },
  { SGCHECKBOX, 0, 0, 15,20, 10,1, "Read Only" },
  { SGCHECKBOX, 0, 0, 15,21, 8,1, "Active" },
  { SGBUTTON, 0, 0, 29,19, 8,1, "Mount" },
  { SGBUTTON, 0, 0, 10,23, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

/*
void UpdateFloppyStatus(void)
{
	static char *eject = "Eject";
	static char *insert = "Insert";
	discdlg[FLP_MOUNT].txt = gui_options.floppy.inserted ? eject : insert;
}
*/
void UpdateCDROMstatus(void)
{
	static char *eject = "Eject";
	static char *insert = "Insert";
	int handle = bx_hard_drive.get_first_cd_handle();
	// discdlg[CDROMUM].txt = bx_hard_drive.get_cd_media_status(handle) ? eject : insert;
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

  UpdateCDROMstatus();

  /* Set up dialog to actual values: */
  File_ShrinkName(floppy_path, gui_options.floppy.path, discdlg[FLP_PATH].w);
  discdlg[FLP_PATH].txt = floppy_path;

  File_ShrinkName(ide0_path, gui_options.atadevice[0][0].path, discdlg[IDE0_PATH].w);
  discdlg[IDE0_PATH].txt = ide0_path;

  File_ShrinkName(ide1_path, gui_options.atadevice[0][1].path, discdlg[IDE1_PATH].w);
  discdlg[IDE1_PATH].txt = ide1_path;

  /* Draw and process the dialog */
  do
  {
    but = SDLGui_DoDialog(discdlg);
    switch(but)
    {
/*
      case CDROMUM:
        {
		int handle = bx_hard_drive.get_first_cd_handle();
		bool status = bx_hard_drive.get_cd_media_status(handle);
        bx_hard_drive.set_cd_media_status(handle, ! status);
        UpdateCDROMstatus();
		}

        break;
*/
      case FLP_BROWSE:   /* Choose a new disc A: */
        strcpy(tmpname, gui_options.floppy.path);
        if( SDLGui_FileSelect(tmpname, false) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) )
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
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) ) {
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
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) ) {
            strcpy(gui_options.atadevice[0][1].path, tmpname);
            File_ShrinkName(ide1_path, tmpname, discdlg[IDE1_PATH].w);
          }
          else {
          	ide1_path[0] = 0;
          }
        }
        break;
#if 0
      case CREATEIMG:
        fprintf(stderr,"Sorry, creating disc images not yet supported\n");
        break;
      case UNMOUNTGDOS:
        GemDOS_UnInitDrives();   /* FIXME: This shouldn't be done here but it's the only quick solution I could think of */
        strcpy(DialogParams.HardDisc.szHardDiscDirectories[0], ConfigureParams.HardDisc.szHardDiscDirectories[0]);
        dlgnamegdos[0] = 0;
        break;
      case BROWSEGDOS:
        strcpy(tmpname, DialogParams.HardDisc.szHardDiscDirectories[0]);
        if( SDLGui_FileSelect(tmpname) )
        {
          char *ptr;
          ptr = strrchr(tmpname, '/');
          if( ptr!=NULL )  ptr[1]=0;        /* Remove file name from path */
          strcpy(DialogParams.HardDisc.szHardDiscDirectories[0], tmpname);
          File_ShrinkName(dlgnamegdos, DialogParams.HardDisc.szHardDiscDirectories[0], discdlg[DISCGDOS].w);
        }
        break;
      case BROWSEHDIMG:
        strcpy(tmpname, DialogParams.HardDisc.szHardDiscImage);
        if( SDLGui_FileSelect(tmpname) )
        {
          strcpy(DialogParams.HardDisc.szHardDiscImage, tmpname);
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) )
          {
            File_ShrinkName(dlgnamehdimg, tmpname, discdlg[DISCHDIMG].w);
          }
          else
          {
            dlgnamehdimg[0] = 0;
          }
        }
        break;
#endif
    }
  }
  while(but!=EXIT);

  /* Read values from dialog */
  /*
  DialogParams.DiscImage.bAutoInsertDiscB = (discdlg[AUTOB].state & SG_SELECTED);
  DialogParams.HardDisc.bBootFromHardDisc = (discdlg[BOOTHD].state & SG_SELECTED);
  */
}

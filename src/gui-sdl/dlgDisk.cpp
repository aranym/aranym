#include "sdlgui.h"
#include "file.h"
#include "ata.h"

/* The disks dialog: */
#define DISCDLG_FLP_BROWSE	3
#define DISCDLG_FLP_PATH	4
#define DISCDLG_FLP_ACTIVE	5
#define DISCDLG_IDE0_BROWSE	8
#define DISCDLG_IDE0_PATH	9
#define DISCDLG_IDE0_RDONLY	10
#define DISCDLG_IDE0_ACTIVE	11
#define DISCDLG_IDE1_BROWSE	19
#define DISCDLG_IDE1_PATH	20
#define DISCDLG_IDE1_RDONLY	21
#define DISCDLG_IDE1_ACTIVE	23
#define DISCDLG_IDE1_ISCD	24
#define DISCDLG_CDROMUM     27
#define DISCDLG_EXIT        28

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
	discdlg[DISCDLG_FLP_MOUNT].txt = bx_options.floppy.inserted ? eject : insert;
}
*/
void UpdateCDROMstatus(void)
{
	static char *eject = "Eject";
	static char *insert = "Insert";
	int handle = bx_hard_drive.get_first_cd_handle();
	discdlg[DISCDLG_CDROMUM].txt = bx_hard_drive.get_cd_media_status(handle) ? eject : insert;
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
  File_ShrinkName(floppy_path, bx_options.floppy.path, discdlg[DISCDLG_FLP_PATH].w);
  discdlg[DISCDLG_FLP_PATH].txt = floppy_path;

  File_ShrinkName(ide0_path, bx_options.atadevice[0][0].path, discdlg[DISCDLG_IDE0_PATH].w);
  discdlg[DISCDLG_IDE0_PATH].txt = ide0_path;

  File_ShrinkName(ide1_path, bx_options.atadevice[0][1].path, discdlg[DISCDLG_IDE1_PATH].w);
  discdlg[DISCDLG_IDE1_PATH].txt = ide1_path;

  /* Draw and process the dialog */
  do
  {
    but = SDLGui_DoDialog(discdlg);
    switch(but)
    {
      case DISCDLG_CDROMUM:
        {
		int handle = bx_hard_drive.get_first_cd_handle();
		bool status = bx_hard_drive.get_cd_media_status(handle);
        bx_hard_drive.set_cd_media_status(handle, ! status);
        UpdateCDROMstatus();
		}

        break;

      case DISCDLG_FLP_BROWSE:   /* Choose a new disc A: */
        strcpy(tmpname, bx_options.floppy.path);
        if( SDLGui_FileSelect(tmpname, false) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) )
          {
            File_ShrinkName(floppy_path, tmpname, discdlg[DISCDLG_FLP_PATH].w);
          }
          else
          {
            floppy_path[0] = 0;
          }
        }
        break;

      case DISCDLG_IDE0_BROWSE:
        strcpy(tmpname, bx_options.atadevice[0][0].path);
        if( SDLGui_FileSelect(tmpname, false) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) ) {
          // strcpy(bx_options.atadevice[0][0].path, tmpname);
            File_ShrinkName(ide0_path, tmpname, discdlg[DISCDLG_IDE0_PATH].w);
          }
          else {
          	ide0_path[0] = 0;
          }
        }
        break;

      case DISCDLG_IDE1_BROWSE:
        strcpy(tmpname, bx_options.atadevice[0][1].path);
        if( SDLGui_FileSelect(tmpname, false) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) ) {
          // strcpy(bx_options.atadevice[0][1].path, tmpname);
            File_ShrinkName(ide1_path, tmpname, discdlg[DISCDLG_IDE1_PATH].w);
          }
          else {
          	ide1_path[0] = 0;
          }
        }
        break;
#if 0
      case DISCDLG_CREATEIMG:
        fprintf(stderr,"Sorry, creating disc images not yet supported\n");
        break;
      case DISCDLG_UNMOUNTGDOS:
        GemDOS_UnInitDrives();   /* FIXME: This shouldn't be done here but it's the only quick solution I could think of */
        strcpy(DialogParams.HardDisc.szHardDiscDirectories[0], ConfigureParams.HardDisc.szHardDiscDirectories[0]);
        dlgnamegdos[0] = 0;
        break;
      case DISCDLG_BROWSEGDOS:
        strcpy(tmpname, DialogParams.HardDisc.szHardDiscDirectories[0]);
        if( SDLGui_FileSelect(tmpname) )
        {
          char *ptr;
          ptr = strrchr(tmpname, '/');
          if( ptr!=NULL )  ptr[1]=0;        /* Remove file name from path */
          strcpy(DialogParams.HardDisc.szHardDiscDirectories[0], tmpname);
          File_ShrinkName(dlgnamegdos, DialogParams.HardDisc.szHardDiscDirectories[0], discdlg[DISCDLG_DISCGDOS].w);
        }
        break;
      case DISCDLG_BROWSEHDIMG:
        strcpy(tmpname, DialogParams.HardDisc.szHardDiscImage);
        if( SDLGui_FileSelect(tmpname) )
        {
          strcpy(DialogParams.HardDisc.szHardDiscImage, tmpname);
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) )
          {
            File_ShrinkName(dlgnamehdimg, tmpname, discdlg[DISCDLG_DISCHDIMG].w);
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
  while(but!=DISCDLG_EXIT);

  /* Read values from dialog */
  /*
  DialogParams.DiscImage.bAutoInsertDiscB = (discdlg[DISCDLG_AUTOB].state & SG_SELECTED);
  DialogParams.HardDisc.bBootFromHardDisc = (discdlg[DISCDLG_BOOTHD].state & SG_SELECTED);
  */
}

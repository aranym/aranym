/*
  Hatari

  This is normal 'C' code to handle our options dialog. We keep all our configuration details
  in a variable 'ConfigureParams'. When we open our dialog we copy this and then when we 'OK'
  or 'Cancel' the dialog we can compare and makes the necessary changes.
*/

#include "sysdeps.h"
/*
#include "main.h"
#include "configuration.h"
#include "audio.h"
#include "debug.h"
#include "dialog.h"
#include "floppy.h"
#include "gemdos.h"
#include "hdc.h"
#include "reset.h"
#include "joy.h"
#include "keymap.h"
#include "m68000.h"
#include "memAlloc.h"
#include "memorySnapShot.h"
#include "misc.h"
#include "printer.h"
#include "rs232.h"
#include "screen.h"
#include "screenSnapShot.h"
#include "sound.h"
#include "tos.h"
#include "vdi.h"
#include "video.h"
#include "uae-cpu/hatari-glue.h"
*/
#include "file.h"
#include "sdlgui.h"
#include "host.h"
#include "ata.h"

#define Screen_Save()		{ hostScreen.lock(); hostScreen.saveBackground(); hostScreen.unlock(); }
		
#define Screen_SetFullUpdate()
#define Screen_Draw()		{ hostScreen.lock(); hostScreen.restoreBackground(); hostScreen.unlock(); }

static bool bQuitProgram;

/* The main dialog: */
#define MAINDLG_ABOUT    2
#define MAINDLG_DISCS    3
#define MAINDLG_NORESET  4
#define MAINDLG_RESET    5
#define MAINDLG_OK       6
#define MAINDLG_CANCEL   7
#define MAINDLG_QUIT     8
/*
#define MAINDLG_TOSGEM   4
#define MAINDLG_SCREEN   5
#define MAINDLG_SOUND    6
#define MAINDLG_CPU      7
#define MAINDLG_MEMORY   8
#define MAINDLG_JOY      9
#define MAINDLG_KEYBD    10
#define MAINDLG_DEVICES  11
#define MAINDLG_NORESET  12
#define MAINDLG_RESET    13
#define MAINDLG_OK       14
#define MAINDLG_CANCEL   15
#define MAINDLG_QUIT     16
*/
SGOBJ maindlg[] =
{
  { SGBOX, 0, 0, 0,0, 36,20, NULL },
  { SGTEXT, 0, 0, 10,1, 16,1, "ARAnyM main menu" },
  { SGBUTTON, 0, 0, 4,4, 12,1, "About" },
  { SGBUTTON, 0, 0, 4,6, 12,1, "Discs" },
/*
  { SGBUTTON, 0, 0, 4,8, 12,1, "TOS/GEM" },
  { SGBUTTON, 0, 0, 4,10, 12,1, "Screen" },
  { SGBUTTON, 0, 0, 4,12, 12,1, "Sound" },
  { SGBUTTON, 0, 0, 20,4, 12,1, "CPU" },
  { SGBUTTON, 0, 0, 20,6, 12,1, "Memory" },
  { SGBUTTON, 0, 0, 20,8, 12,1, "Joysticks" },
  { SGBUTTON, 0, 0, 20,10, 12,1, "Keyboard" },
  { SGBUTTON, 0, 0, 20,12, 12,1, "Devices" },
*/
  { SGRADIOBUT, 0, 0, 2,16, 10,1, "No Reset" },
  { SGRADIOBUT, 0, 0, 2,18, 10,1, "Reset" },
  { SGBUTTON, 0, 0, 14,16, 8,3, "Okay" },
  { SGBUTTON, 0, 0, 25,18, 8,1, "Cancel" },
  { SGBUTTON, 0, 0, 25,16, 8,1, "Quit" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

/* The "About"-dialog: */
SGOBJ aboutdlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGTEXT, 0, 0, 14,1, 12,1, "ARAnyM" },
  { SGTEXT, 0, 0, 14,2, 12,1, "======" },
  { SGTEXT, 0, 0, 1,4, 38,1, "ARAnyM has been written by:  C. Fertr," },
  { SGTEXT, 0, 0, 1,5, 38,1, "M. Jurik, S. Opichal, P. Stehlik" },
  { SGTEXT, 0, 0, 1,6, 38,1, "and many others." },
  { SGTEXT, 0, 0, 1,7, 38,1, "Please see the docs for more info." },
  { SGTEXT, 0, 0, 1,9, 38,1, "This program is free software; you can" },
  { SGTEXT, 0, 0, 1,10, 38,1, "redistribute it and/or modify it under" },
  { SGTEXT, 0, 0, 1,11, 38,1, "the terms of the GNU General Public" },
  { SGTEXT, 0, 0, 1,12, 38,1, "License as published by the Free Soft-" },
  { SGTEXT, 0, 0, 1,13, 38,1, "ware Foundation; either version 2 of" },
  { SGTEXT, 0, 0, 1,14, 38,1, "the License, or (at your option) any" },
  { SGTEXT, 0, 0, 1,15, 38,1, "later version." },
  { SGTEXT, 0, 0, 1,17, 38,1, "This program is distributed in the" },
  { SGTEXT, 0, 0, 1,18, 38,1, "hope that it will be useful, but" },
  { SGTEXT, 0, 0, 1,19, 38,1, "WITHOUT ANY WARRANTY. See the GNU Ge-" },
  { SGTEXT, 0, 0, 1,20, 38,1, "neral Public License for more details." },
  { SGBUTTON, 0, 0, 16,23, 8,1, "Okay" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

/* The discs dialog: */
#define DISCDLG_CDROMUM     4
#define DISCDLG_EXIT        5
/*
#define DISCDLG_DISCA       4
#define DISCDLG_BROWSEA     5
#define DISCDLG_DISCB       7
#define DISCDLG_BROWSEB     8
#define DISCDLG_IMGDIR      10
#define DISCDLG_BROWSEIMG   11
#define DISCDLG_AUTOB       12
#define DISCDLG_CREATEIMG   13
#define DISCDLG_BROWSEHDIMG 17
#define DISCDLG_DISCHDIMG   18
#define DISCDLG_UNMOUNTGDOS 20
#define DISCDLG_BROWSEGDOS  21
#define DISCDLG_DISCGDOS    22
#define DISCDLG_BOOTHD      23
#define DISCDLG_EXIT        24
*/
SGOBJ discdlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGBOX, 0, 0, 1,1, 38,11, NULL },
#if 0 // MJ
  { SGTEXT, 0, 0, 14,1, 12,1, "Floppy discs" },
  { SGTEXT, 0, 0, 2,3, 2,1, "A:" },
  { SGTEXT, 0, 0, 5,3, 26,1, NULL },
  { SGBUTTON, 0, 0, 32,3, 6,1, "Browse" },
  { SGTEXT, 0, 0, 2,5, 2,1, "B:" },
  { SGTEXT, 0, 0, 5,5, 26,1, NULL },
  { SGBUTTON, 0, 0, 32,5, 6,1, "Browse" },
  { SGTEXT, 0, 0, 2,7, 30,1, "Default disk images directory:" },
  { SGTEXT, 0, 0, 2,8, 28,1, NULL },
  { SGBUTTON, 0, 0, 32,8, 6,1, "Browse" },
  { SGCHECKBOX, 0, 0, 2,10, 18,1, "Auto insert B" },
  { SGTEXT/*SGBUTTON*/, 0, 0, 20,10, 18,1, ""/*"Create blank image"*/ }, /* Not yet supported */
#endif
  { SGBOX, 0, 0, 1,13, 38,9, NULL },
#if 0 // MJ
  { SGTEXT, 0, 0, 15,13, 10,1, "Hard discs" },
  { SGTEXT, 0, 0, 2,14, 9,1, "HD image:" },
  { SGBUTTON, 0, 0, 32,14, 6,1, "Browse" },
  { SGTEXT, 0, 0, 2,15, 36,1, NULL },
  { SGTEXT, 0, 0, 2,17, 13,1, "GEMDOS drive:" },
  { SGBUTTON, 0, 0, 30,17, 1,1, "\x01" },         /* Up-arrow button for unmounting */
  { SGBUTTON, 0, 0, 32,17, 6,1, "Browse" },
  { SGTEXT, 0, 0, 2,18, 36,1, NULL },
  { SGCHECKBOX, 0, 0, 2,20, 14,1, "Boot from HD" },
#endif
  { SGTEXT, 0, 0, 2,17, 13,1, "CD-ROM drive:" },
  { SGBUTTON, 0, 0, 30,17, 1,1, "\x01" },
  { SGBUTTON, 0, 0, 10,23, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};

#if 0 // joy

/* The TOS/GEM dialog: */
#define DLGTOSGEM_ROMNAME    3
#define DLGTOSGEM_ROMBROWSE  4
#define DLGTOSGEM_RESPOPUP   8
#define DLGTOSGEM_EXIT       5/*9*/
SGOBJ tosgemdlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,10/*19*/, NULL },
  { SGTEXT, 0, 0, 14,1, 13,1, "TOS/GEM setup" },
  { SGTEXT, 0, 0, 2,4, 25,1, "ROM image (needs reset!):" },
  { SGTEXT, 0, 0, 2,6, 34,1, NULL },
  { SGBUTTON, 0, 0, 30,4, 8,1, "Browse" },
/*
  { SGTEXT, 0, 0, 2,10, 4,1, "GEM: (sorry, does not work yet)" },
  { SGCHECKBOX, 0, 0, 2,12, 25,1, "Use extended resolution" },
  { SGTEXT, 0, 0, 2,14, 11,1, "Resolution:" },
  { SGPOPUP, 0, 0, 14,14, 10,1, "800x600" },
*/
  { SGBUTTON, 0, 0, 10,8/*17*/, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};


/* The screen dialog: */
#define DLGSCRN_MODE       4
#define DLGSCRN_FULLSCRN   5
#define DLGSCRN_INTERLACE  6
#define DLGSCRN_FRAMESKIP  7
#define DLGSCRN_COLOR      9
#define DLGSCRN_MONO       10
#define DLGSCRN_ONCHANGE   13
#define DLGSCRN_FPSPOPUP   15
#define DLGSCRN_CAPTURE    16
#define DLGSCRN_RECANIM    17
#define DLGSCRN_EXIT       18
SGOBJ screendlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,25, NULL },
  { SGBOX, 0, 0, 1,1, 38,11, NULL },
  { SGTEXT, 0, 0, 13,2, 14,1, "Screen options" },
  { SGTEXT, 0, 0, 5,4, 13,1, ""/*"Display mode:"*/ },
  { SGTEXT/*SGPOPUP*/, 0, 0, 20,4, 18,1, ""/*"Hi-Color, Lo-Res"*/ },
  { SGCHECKBOX, 0, 0, 5,6, 12,1, "Fullscreen" },
  { SGCHECKBOX, 0, 0, 5,7, 23,1, "Interlaced mode (in fullscreen)" },
  { SGCHECKBOX, 0, 0, 5,8, 10,1, "Frame skip" },
  /*{ SGCHECKBOX, 0, 0, 22,8, 13,1, "Use borders" },*/
  { SGTEXT, 0, 0, 5,10, 8,1, "Monitor:" },
  { SGRADIOBUT, 0, 0, 16,10, 7,1, "Color" },
  { SGRADIOBUT, 0, 0, 26,10, 6,1, "Mono" },
  { SGBOX, 0, 0, 1,13, 38,9, NULL },
  { SGTEXT, 0, 0, 13,14, 14,1, "Screen capture" },
  { SGCHECKBOX, 0, 0, 5,16, 27,1, "Only when display changes" },
  { SGTEXT, 0, 0, 5,18, 18,1, "Frames per second:" },
  { SGTEXT/*SGPOPUP*/, 0, 0, 24,18, 3,1, "25" },
  { SGBUTTON, 0, 0, 3,20, 16,1, "Capture screen" },
  { SGBUTTON, 0, 0, 20,20, 18,1, NULL },
  { SGBUTTON, 0, 0, 10,23, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};


/* The sound dialog: */
#define DLGSOUND_ENABLE  3
#define DLGSOUND_LOW     5
#define DLGSOUND_MEDIUM  6
#define DLGSOUND_HIGH    7
#define DLGSOUND_EXIT    8
SGOBJ sounddlg[] =
{
  { SGBOX, 0, 0, 0,0, 38,/*24*/15, NULL },
  { SGBOX, 0, 0, 1,1, 36,11, NULL },
  { SGTEXT, 0, 0, 13,2, 13,1, "Sound options" },
  { SGCHECKBOX, 0, 0, 12,4, 14,1, "Enable sound" },
  { SGTEXT, 0, 0, 11,6, 14,1, "Playback quality:" },
  { SGRADIOBUT, 0, 0, 12,8, 15,1, "Low (11kHz)" },
  { SGRADIOBUT, 0, 0, 12,9, 19,1, "Medium (22kHz)" },
  { SGRADIOBUT, 0, 0, 12,10, 14,1, "High (44kHz)" },
/*
  { SGBOX, 0, 0, 1,13, 36,7, NULL },
  { SGTEXT, 0, 0, 13,14, 14,1, "Capture YM/WAV" },
  { SGBUTTON, 0, 0, 9,18, 8,1, "Record" },
  { SGBUTTON, 0, 0, 23,18, 8,1, "Stop" },
*/
  { SGBUTTON, 0, 0, 10,/*22*/13, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};


/* The cpu dialog: */
#define DLGCPU_68000 3
#define DLGCPU_68010 4
#define DLGCPU_68020 5
#define DLGCPU_68030 6
#define DLGCPU_68040 7
#define DLGCPU_PREFETCH 8
SGOBJ cpudlg[] =
{
  { SGBOX, 0, 0, 0,0, 30,15, NULL },
  { SGTEXT, 0, 0, 10,1, 11,1, "CPU options" },
  { SGTEXT, 0, 0, 3,4, 8,1, "CPU Type:" },
  { SGRADIOBUT, 0, 0, 16,4, 7,1, "68000" },
  { SGRADIOBUT, 0, 0, 16,5, 7,1, "68010" },
  { SGRADIOBUT, 0, 0, 16,6, 7,1, "68020" },
  { SGRADIOBUT, 0, 0, 16,7, 11,1, "68020+FPU" },
  { SGRADIOBUT, 0, 0, 16,8, 7,1, "68040" },
  { SGCHECKBOX, 0, 0, 3,10, 20,1, "Use prefetch mode" },
  { SGBUTTON, 0, 0, 5,13, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};


/* The memory dialog: */
#define DLGMEM_512KB  4
#define DLGMEM_1MB    5
#define DLGMEM_2MB    6
#define DLGMEM_4MB    7
#define DLGMEM_EXIT   8
SGOBJ memorydlg[] =
{
  { SGBOX, 0, 0, 0,0, 40,11/*19*/, NULL },
  { SGBOX, 0, 0, 1,1, 38,7, NULL },
  { SGTEXT, 0, 0, 15,2, 12,1, "Memory setup" },
  { SGTEXT, 0, 0, 4,4, 12,1, "ST-RAM size:" },
  { SGRADIOBUT, 0, 0, 19,4, 8,1, "512 kB" },
  { SGRADIOBUT, 0, 0, 30,4, 6,1, "1 MB" },
  { SGRADIOBUT, 0, 0, 19,6, 6,1, "2 MB" },
  { SGRADIOBUT, 0, 0, 30,6, 6,1, "4 MB" },
/*
  { SGBOX, 0, 0, 1,9, 38,7, NULL },
  { SGTEXT, 0, 0, 12,10, 17,1, "Memory state save" },
  { SGTEXT, 0, 0, 2,12, 28,1, "/Sorry/Not/yet/supported" },
  { SGBUTTON, 0, 0, 32,12, 6,1, "Browse" },
  { SGBUTTON, 0, 0, 8,14, 10,1, "Save" },
  { SGBUTTON, 0, 0, 22,14, 10,1, "Restore" },
*/
  { SGBUTTON, 0, 0, 10,9/*17*/, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};


/* The joysticks dialog: */
#define DLGJOY_J1CURSOR    4
#define DLGJOY_J1AUTOFIRE  5
#define DLGJOY_J0CURSOR    8
#define DLGJOY_J0AUTOFIRE  9
#define DLGJOY_EXIT        10
SGOBJ joystickdlg[] =
{
  { SGBOX, 0, 0, 0,0, 30,19, NULL },
  { SGTEXT, 0, 0, 7,1, 15,1, "Joysticks setup" },
  { SGBOX, 0, 0, 1,3, 28,6, NULL },
  { SGTEXT, 0, 0, 2,4, 11,1, "Joystick 1:" },
  { SGCHECKBOX, 0, 0, 5,6, 22,1, "Use cursor emulation" },
  { SGCHECKBOX, 0, 0, 5,7, 17,1, "Enable autofire" },
  { SGBOX, 0, 0, 1,10, 28,6, NULL },
  { SGTEXT, 0, 0, 2,11, 11,1, "Joystick 0:" },
  { SGCHECKBOX, 0, 0, 5,13, 22,1, "Use cursor emulation" },
  { SGCHECKBOX, 0, 0, 5,14, 17,1, "Enable autofire" },
  { SGBUTTON, 0, 0, 5,17, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};


/* The keyboard dialog: */
SGOBJ keyboarddlg[] =
{
  { SGBOX, 0, 0, 0,0, 30,8, NULL },
  { SGTEXT, 0, 0, 8,2, 14,1, "Keyboard setup" },
  { SGTEXT, 0, 0, 2,4, 25,1, "Sorry, not yet supported." },
  { SGBUTTON, 0, 0, 5,6, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};


/* The devices dialog: */
SGOBJ devicedlg[] =
{
  { SGBOX, 0, 0, 0,0, 30,8, NULL },
  { SGTEXT, 0, 0, 8,2, 13,1, "Devices setup" },
  { SGTEXT, 0, 0, 2,4, 25,1, "Sorry, not yet supported." },
  { SGBUTTON, 0, 0, 5,6, 20,1, "Back to main menu" },
  { -1, 0, 0, 0,0, 0,0, NULL }
};



DLG_PARAMS ConfigureParams, DialogParams;   /* List of configuration for system and dialog (so can choose 'Cancel') */



/*-----------------------------------------------------------------------*/
/*
  Check if need to warn user that changes will take place after reset
  Return true if wants to reset
*/
bool Dialog_DoNeedReset(void)
{
  /* Did we change colour/mono monitor? If so, must reset */
  if (ConfigureParams.Screen.bUseHighRes!=DialogParams.Screen.bUseHighRes)
    return(true);
  /* Did change to GEM VDI display? */
  if (ConfigureParams.TOSGEM.bUseExtGEMResolutions!=DialogParams.TOSGEM.bUseExtGEMResolutions)
    return(true);
  /* Did change GEM resolution or colour depth? */
  if ( (ConfigureParams.TOSGEM.nGEMResolution!=DialogParams.TOSGEM.nGEMResolution)
   || (ConfigureParams.TOSGEM.nGEMColours!=DialogParams.TOSGEM.nGEMColours) )
    return(true);
  /* Did change TOS ROM image? */
  if (strcmp(DialogParams.TOSGEM.szTOSImageFileName, ConfigureParams.TOSGEM.szTOSImageFileName))
    return(true);
  /* Did change HD image? */
  if (strcmp(DialogParams.HardDisc.szHardDiscImage, ConfigureParams.HardDisc.szHardDiscImage))
    return(true);
  /* Did change GEMDOS drive? */
  if (strcmp(DialogParams.HardDisc.szHardDiscDirectories[0], ConfigureParams.HardDisc.szHardDiscDirectories[0]))
    return(true);

  return(false);
}


/*-----------------------------------------------------------------------*/
/*
  Copy details back to configuration and perform reset
*/
void Dialog_CopyDialogParamsToConfiguration(bool bForceReset)
{
  bool NeedReset;
  bool newGemdosDrive;

  /* Do we need to warn user of that changes will only take effect after reset? */
  if (bForceReset)
    NeedReset = bForceReset;
  else
    NeedReset = Dialog_DoNeedReset();

  /* Do need to change resolution? Need if change display/overscan settings */
  /*(if switch between Colour/Mono cause reset later) */
  if(bInFullScreen)
  {
    if( (DialogParams.Screen.ChosenDisplayMode!=ConfigureParams.Screen.ChosenDisplayMode)
       || (DialogParams.Screen.Advanced.bAllowOverscan!=ConfigureParams.Screen.Advanced.bAllowOverscan) )
    {
      Screen_ReturnFromFullScreen();
      ConfigureParams.Screen.ChosenDisplayMode = DialogParams.Screen.ChosenDisplayMode;
      ConfigureParams.Screen.Advanced.bAllowOverscan = DialogParams.Screen.Advanced.bAllowOverscan;
      Screen_EnterFullScreen();
    }
  }

  /* Did set new printer parameters? */
  if( (DialogParams.Printer.bEnablePrinting!=ConfigureParams.Printer.bEnablePrinting)
     || (DialogParams.Printer.bPrintToFile!=ConfigureParams.Printer.bPrintToFile)
     || (strcmp(DialogParams.Printer.szPrintToFileName,ConfigureParams.Printer.szPrintToFileName)) )
    Printer_CloseAllConnections();

  /* Did set new RS232 parameters? */
  if( (DialogParams.RS232.bEnableRS232!=ConfigureParams.RS232.bEnableRS232)
     || (DialogParams.RS232.nCOMPort!=ConfigureParams.RS232.nCOMPort) )
    RS232_CloseCOMPort();

  /* Did stop sound? Or change playback Hz. If so, also stop sound recording */
  if( (!DialogParams.Sound.bEnableSound)
     || (DialogParams.Sound.nPlaybackQuality!=ConfigureParams.Sound.nPlaybackQuality) )
  {
    if(Sound_AreWeRecording())
      Sound_EndRecording(NULL);
  }

  /* Did change GEMDOS drive? */
  if( strcmp(DialogParams.HardDisc.szHardDiscDirectories[0], ConfigureParams.HardDisc.szHardDiscDirectories[0])!=0 )
  {
    GemDOS_UnInitDrives();
    newGemdosDrive = true;
  }
  else
  {
    newGemdosDrive = false;
  }

  /* Did change HD image? */
  if( strcmp(DialogParams.HardDisc.szHardDiscImage, ConfigureParams.HardDisc.szHardDiscImage)!=0
     && ACSI_EMU_ON )
  {
    HDC_UnInit();
  }

  /* Copy details to configuration, so can be saved out or set on reset */
  ConfigureParams = DialogParams;
  /* And write to configuration now, so don't loose */
  Configuration_UnInit();

  /* Copy details to global, if we reset copy them all */
  Dialog_CopyDetailsFromConfiguration(NeedReset);

  /* Set keyboard remap file */
  /*Keymap_LoadRemapFile(ConfigureParams.Keyboard.szMappingFileName);*/

  /* Resize window if need */
  /*if(!ConfigureParams.TOSGEM.bUseExtGEMResolutions)
    View_ResizeWindowToFull();*/

  /* Did the user changed the CPU mode? */
  check_prefs_changed_cpu(DialogParams.Cpu.level, DialogParams.Cpu.compatible);

  /* Mount a new HD image: */
  if( !ACSI_EMU_ON && !File_DoesFileNameEndWithSlash(ConfigureParams.HardDisc.szHardDiscImage)
      && File_Exists(ConfigureParams.HardDisc.szHardDiscImage) )
  {
    HDC_Init(ConfigureParams.HardDisc.szHardDiscImage);
  }

  /* Mount a new GEMDOS drive? */
  if( newGemdosDrive )
  {
    GemDOS_InitDrives();
  }

  /* Do we need to perform reset? */
  if (NeedReset)
  {
    Reset_Cold();
    /*FM  View_ToggleWindowsMouse(MOUSE_ST);*/
  }

  /* Go into/return from full screen if flagged */
  if ( (!bInFullScreen) && (DialogParams.Screen.bFullScreen) )
    Screen_EnterFullScreen();
  else if ( bInFullScreen && (!DialogParams.Screen.bFullScreen) )
    Screen_ReturnFromFullScreen();
}



/*-----------------------------------------------------------------------*/
/*
  Copy details from configuration structure into global variables for system
*/
void Dialog_CopyDetailsFromConfiguration(bool bReset)
{
  /* Set new timer thread */
/*FM  Main_SetSpeedThreadTimer(ConfigureParams.Configure.nMinMaxSpeed);*/
  /* Set resolution change */
  if (bReset) {
    bUseVDIRes = ConfigureParams.TOSGEM.bUseExtGEMResolutions;
    bUseHighRes = ConfigureParams.Screen.bUseHighRes || (bUseVDIRes && (ConfigureParams.TOSGEM.nGEMColours==GEMCOLOUR_2));
/*FM    VDI_SetResolution(VDIModeOptions[ConfigureParams.TOSGEM.nGEMResolution],ConfigureParams.TOSGEM.nGEMColours);*/
  }

  /* Set playback frequency */
  if( ConfigureParams.Sound.bEnableSound )
    Audio_SetOutputAudioFreq(ConfigureParams.Sound.nPlaybackQuality);

  /* Remove back-slashes, etc.. from names */
  File_CleanFileName(ConfigureParams.TOSGEM.szTOSImageFileName);
}

#endif

/*-----------------------------------------------------------------------*/
/*
  Show and process the disc image dialog.
*/
void Dialog_DiscDlg(void)
{
  int but;
// MJ  char tmpname[256/*MAX_FILENAME_LENGTH*/];
// MJ  char dlgnamea[40]="", dlgnameb[40]="", dlgdiscdir[40]="";
// MJ  char dlgnamegdos[40]="", dlgnamehdimg[40]="";

  SDLGui_CenterDlg(discdlg);

  /* Set up dialog to actual values: */

  /* Disc name A: */
/*
  if( EmulationDrives[0].bDiscInserted )
    File_ShrinkName(dlgnamea, EmulationDrives[0].szFileName, discdlg[DISCDLG_DISCA].w);
  else
    dlgnamea[0] = 0;
*/
#if 0 // MJ
  discdlg[DISCDLG_DISCA].txt = dlgnamea;
#endif

  /* Disc name B: */
/*
  if( EmulationDrives[1].bDiscInserted )
    File_ShrinkName(dlgnameb, EmulationDrives[1].szFileName, discdlg[DISCDLG_DISCB].w);
  else
    dlgnameb[0] = 0;
*/
#if 0 // MJ
  discdlg[DISCDLG_DISCB].txt = dlgnameb;
#endif

  /* Default image directory: */
  // File_ShrinkName(dlgdiscdir, DialogParams.DiscImage.szDiscImageDirectory, discdlg[DISCDLG_IMGDIR].w);
#if 0
  discdlg[DISCDLG_IMGDIR].txt = dlgdiscdir;
#endif

  /* Auto insert disc B: */
/*
  if( DialogParams.DiscImage.bAutoInsertDiscB )
    discdlg[DISCDLG_AUTOB].state |= SG_SELECTED;
   else
*/
#if 0 // MJ
    discdlg[DISCDLG_AUTOB].state &= ~SG_SELECTED;
#endif

  /* Boot from harddisk? */
/*
  if( DialogParams.HardDisc.bBootFromHardDisc )
    discdlg[DISCDLG_BOOTHD].state |= SG_SELECTED;
   else
*/
#if 0 //MJ
    discdlg[DISCDLG_BOOTHD].state &= ~SG_SELECTED;
#endif

  /* GEMDOS Hard disc directory: */
  /*
  if( strcmp(DialogParams.HardDisc.szHardDiscDirectories[0], ConfigureParams.HardDisc.szHardDiscDirectories[0])!=0
      || GEMDOS_EMU_ON )
    File_ShrinkName(dlgnamegdos, DialogParams.HardDisc.szHardDiscDirectories[0], discdlg[DISCDLG_DISCGDOS].w);
  else
    dlgnamegdos[0] = 0;
*/
#if 0 // MJ
  discdlg[DISCDLG_DISCGDOS].txt = dlgnamegdos;
#endif

  /* Hard disc image: */
  /*
  if( ACSI_EMU_ON )
    File_ShrinkName(dlgnamehdimg, DialogParams.HardDisc.szHardDiscImage, discdlg[DISCDLG_DISCHDIMG].w);
  else
    dlgnamehdimg[0] = 0;
  */
#if 0 // MJ
  discdlg[DISCDLG_DISCHDIMG].txt = dlgnamehdimg;
#endif

  /* Draw and process the dialog */
  do
  {
    but = SDLGui_DoDialog(discdlg);
    switch(but)
    {
      case DISCDLG_CDROMUM:
        bx_hard_drive.set_cd_media_status(bx_hard_drive.get_first_cd_handle(), bx_hard_drive.get_cd_media_status(bx_hard_drive.get_first_cd_handle()) == 0);
        break;
#if 0 // MJ
      case DISCDLG_BROWSEA:                       /* Choose a new disc A: */
        /*
        if( EmulationDrives[0].bDiscInserted )
          strcpy(tmpname, EmulationDrives[0].szFileName);
         else
          strcpy(tmpname, DialogParams.DiscImage.szDiscImageDirectory);
        */
        strcpy(tmpname, "/home/joy/");
        if( SDLGui_FileSelect(tmpname) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) )
          {
            Floppy_InsertDiscIntoDrive(0, tmpname); /* FIXME: This shouldn't be done here but in Dialog_CopyDialogParamsToConfiguration */
            File_ShrinkName(dlgnamea, tmpname, discdlg[DISCDLG_DISCA].w);
          }
          else
          {
            Floppy_EjectDiscFromDrive(0, false); /* FIXME: This shouldn't be done here but in Dialog_CopyDialogParamsToConfiguration */
            dlgnamea[0] = 0;
          }
        }
        break;
      case DISCDLG_BROWSEB:                       /* Choose a new disc B: */
        if( EmulationDrives[1].bDiscInserted )
          strcpy(tmpname, EmulationDrives[1].szFileName);
         else
          strcpy(tmpname, DialogParams.DiscImage.szDiscImageDirectory);
        if( SDLGui_FileSelect(tmpname) )
        {
          if( !File_DoesFileNameEndWithSlash(tmpname) && File_Exists(tmpname) )
          {
            Floppy_InsertDiscIntoDrive(1, tmpname); /* FIXME: This shouldn't be done here but in Dialog_CopyDialogParamsToConfiguration */
            File_ShrinkName(dlgnameb, tmpname, discdlg[DISCDLG_DISCB].w);
          }
          else
          {
            Floppy_EjectDiscFromDrive(1, false); /* FIXME: This shouldn't be done here but in Dialog_CopyDialogParamsToConfiguration */
            dlgnameb[0] = 0;
          }
        }
        break;
      case DISCDLG_BROWSEIMG:
        strcpy(tmpname, DialogParams.DiscImage.szDiscImageDirectory);
        if( SDLGui_FileSelect(tmpname) )
        {
          char *ptr;
          ptr = strrchr(tmpname, '/');
          if( ptr!=NULL )  ptr[1]=0;
          strcpy(DialogParams.DiscImage.szDiscImageDirectory, tmpname);
          File_ShrinkName(dlgdiscdir, DialogParams.DiscImage.szDiscImageDirectory, discdlg[DISCDLG_IMGDIR].w);
        }
        break;
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
      case -1:
      	bQuitProgram = true;
      	break;
    }
  }
  while(but!=DISCDLG_EXIT && !bQuitProgram);

  /* Read values from dialog */
  /*
  DialogParams.DiscImage.bAutoInsertDiscB = (discdlg[DISCDLG_AUTOB].state & SG_SELECTED);
  DialogParams.HardDisc.bBootFromHardDisc = (discdlg[DISCDLG_BOOTHD].state & SG_SELECTED);
  */
}

#if 0

/*-----------------------------------------------------------------------*/
/*
  Show and process the TOS/GEM dialog.
*/
void Dialog_TosGemDlg(void)
{
  char tmpname[MAX_FILENAME_LENGTH];
  char dlgromname[35];
  int but;

  SDLGui_CenterDlg(tosgemdlg);
  File_ShrinkName(dlgromname, DialogParams.TOSGEM.szTOSImageFileName, 34);
  tosgemdlg[DLGTOSGEM_ROMNAME].txt = dlgromname;

  do
  {
    but = SDLGui_DoDialog(tosgemdlg);
    switch( but )
    {
      case DLGTOSGEM_ROMBROWSE:
        strcpy(tmpname, DialogParams.TOSGEM.szTOSImageFileName);
        if(tmpname[0]=='.' && tmpname[1]=='/')  /* Is it in the actual working directory? */
        {
          getcwd(tmpname, MAX_FILENAME_LENGTH);
          File_AddSlashToEndFileName(tmpname);
          strcat(tmpname, &DialogParams.TOSGEM.szTOSImageFileName[2]);
        }
        if( SDLGui_FileSelect(tmpname) )        /* Show and process the file selection dlg */
        {
          strcpy(DialogParams.TOSGEM.szTOSImageFileName, tmpname);
          File_ShrinkName(dlgromname, DialogParams.TOSGEM.szTOSImageFileName, 34);
        }
        Screen_SetFullUpdate();
        Screen_Draw();
        break;
      case DLGTOSGEM_RESPOPUP:
        tosgemdlg[DLGTOSGEM_RESPOPUP].state &= ~SG_SELECTED;
        break;
      case -1:
      	bQuitProgram = true;
      	break;
    }
  }
  while(but!=DLGTOSGEM_EXIT && !bQuitProgram);

}


/*-----------------------------------------------------------------------*/
/*
  Show and process the screen dialog.
*/
void Dialog_ScreenDlg(void)
{
  int but;

  SDLGui_CenterDlg(screendlg);

  /* Set up dialog from actual values: */

  if( DialogParams.Screen.bFullScreen )
    screendlg[DLGSCRN_FULLSCRN].state |= SG_SELECTED;
  else
    screendlg[DLGSCRN_FULLSCRN].state &= ~SG_SELECTED;

  if( DialogParams.Screen.Advanced.bInterlacedFullScreen )
    screendlg[DLGSCRN_INTERLACE].state |= SG_SELECTED;
  else
    screendlg[DLGSCRN_INTERLACE].state &= ~SG_SELECTED;

  if( DialogParams.Screen.Advanced.bFrameSkip )
    screendlg[DLGSCRN_FRAMESKIP].state |= SG_SELECTED;
  else
    screendlg[DLGSCRN_FRAMESKIP].state &= ~SG_SELECTED;

  if( DialogParams.Screen.bUseHighRes )
  {
    screendlg[DLGSCRN_COLOR].state &= ~SG_SELECTED;
    screendlg[DLGSCRN_MONO].state |= SG_SELECTED;
  }
  else
  {
    screendlg[DLGSCRN_COLOR].state |= SG_SELECTED;
    screendlg[DLGSCRN_MONO].state &= ~SG_SELECTED;
  }

  if( DialogParams.Screen.bCaptureChange )
    screendlg[DLGSCRN_ONCHANGE].state |= SG_SELECTED;
  else
    screendlg[DLGSCRN_ONCHANGE].state &= ~SG_SELECTED;

  if( ScreenSnapShot_AreWeRecording() )
    screendlg[DLGSCRN_RECANIM].txt = "Stop recording";
  else
    screendlg[DLGSCRN_RECANIM].txt = "Record animation";

  /* The screen dialog main loop */
  do
  {
    but = SDLGui_DoDialog(screendlg);
    switch( but )
    {
      case DLGSCRN_MODE:
        fprintf(stderr,"Sorry, popup menus don't work yet\n");
        break;
      case DLGSCRN_FPSPOPUP:
        fprintf(stderr,"Sorry, popup menus don't work yet\n");
        break;
      case DLGSCRN_CAPTURE:
        Screen_SetFullUpdate();
        Screen_Draw();
        ScreenSnapShot_SaveScreen();
        break;
      case DLGSCRN_RECANIM:
        if( ScreenSnapShot_AreWeRecording() )
        {
          screendlg[DLGSCRN_RECANIM].txt = "Record animation";
          ScreenSnapShot_EndRecording();
        }
        else
        {
          screendlg[DLGSCRN_RECANIM].txt = "Stop recording";
          DialogParams.Screen.bCaptureChange = (screendlg[DLGSCRN_ONCHANGE].state & SG_SELECTED);
          ScreenSnapShot_BeginRecording(DialogParams.Screen.bCaptureChange, 25);
        }
        break;
      case -1:
      	bQuitProgram = true;
      	break;
    }
  }
  while( but!=DLGSCRN_EXIT && !bQuitProgram );

  /* Read values from dialog */
  DialogParams.Screen.bFullScreen = (screendlg[DLGSCRN_FULLSCRN].state & SG_SELECTED);
  DialogParams.Screen.Advanced.bInterlacedFullScreen = (screendlg[DLGSCRN_INTERLACE].state & SG_SELECTED);
  DialogParams.Screen.Advanced.bFrameSkip = (screendlg[DLGSCRN_FRAMESKIP].state & SG_SELECTED);
  DialogParams.Screen.bUseHighRes = (screendlg[DLGSCRN_MONO].state & SG_SELECTED);
  DialogParams.Screen.bCaptureChange = (screendlg[DLGSCRN_ONCHANGE].state & SG_SELECTED);
}


/*-----------------------------------------------------------------------*/
/*
  Show and process the sound dialog.
*/
void Dialog_SoundDlg(void)
{
  int but;

  SDLGui_CenterDlg(sounddlg);

  /* Set up dialog from actual values: */

  if( DialogParams.Sound.bEnableSound )
    sounddlg[DLGSOUND_ENABLE].state |= SG_SELECTED;
  else
    sounddlg[DLGSOUND_ENABLE].state &= ~SG_SELECTED;

  sounddlg[DLGSOUND_LOW].state &= ~SG_SELECTED;
  sounddlg[DLGSOUND_MEDIUM].state &= ~SG_SELECTED;
  sounddlg[DLGSOUND_HIGH].state &= ~SG_SELECTED;
  if( DialogParams.Sound.nPlaybackQuality==PLAYBACK_LOW )
    sounddlg[DLGSOUND_LOW].state |= SG_SELECTED;
  else if( DialogParams.Sound.nPlaybackQuality==PLAYBACK_MEDIUM )
    sounddlg[DLGSOUND_MEDIUM].state |= SG_SELECTED;
  else
    sounddlg[DLGSOUND_HIGH].state |= SG_SELECTED;

  /* The sound dialog main loop */
  do
  {
    but = SDLGui_DoDialog(sounddlg);
    if (but == -1)
      bQuitProgram = true;
  }
  while( but!=DLGSOUND_EXIT && !bQuitProgram );

  /* Read values from dialog */
  DialogParams.Sound.bEnableSound = (sounddlg[DLGSOUND_ENABLE].state & SG_SELECTED);
  if( sounddlg[DLGSOUND_LOW].state & SG_SELECTED )
    DialogParams.Sound.nPlaybackQuality = PLAYBACK_LOW;
  else if( sounddlg[DLGSOUND_MEDIUM].state & SG_SELECTED )
    DialogParams.Sound.nPlaybackQuality = PLAYBACK_MEDIUM;
  else
    DialogParams.Sound.nPlaybackQuality = PLAYBACK_HIGH;

}


/*-----------------------------------------------------------------------*/
/*
  Show and process the memory dialog.
*/
void Dialog_MemDlg(void)
{
  int but;

  SDLGui_CenterDlg(memorydlg);

  memorydlg[DLGMEM_512KB].state &= ~SG_SELECTED;
  memorydlg[DLGMEM_1MB].state &= ~SG_SELECTED;
  memorydlg[DLGMEM_2MB].state &= ~SG_SELECTED;
  memorydlg[DLGMEM_4MB].state &= ~SG_SELECTED;
  if( DialogParams.Memory.nMemorySize == MEMORY_SIZE_512Kb )
    memorydlg[DLGMEM_512KB].state |= SG_SELECTED;
  else if( DialogParams.Memory.nMemorySize == MEMORY_SIZE_1Mb )
    memorydlg[DLGMEM_1MB].state |= SG_SELECTED;
  else if( DialogParams.Memory.nMemorySize == MEMORY_SIZE_2Mb )
    memorydlg[DLGMEM_2MB].state |= SG_SELECTED;
  else
    memorydlg[DLGMEM_4MB].state |= SG_SELECTED;

  do
  {
    but = SDLGui_DoDialog(memorydlg);
    if (but == -1)
      bQuitProgram = true;
  }
  while( but!=DLGMEM_EXIT && !bQuitProgram );

  if( memorydlg[DLGMEM_512KB].state & SG_SELECTED )
    DialogParams.Memory.nMemorySize = MEMORY_SIZE_512Kb;
  else if( memorydlg[DLGMEM_1MB].state & SG_SELECTED )
    DialogParams.Memory.nMemorySize = MEMORY_SIZE_1Mb;
  else if( memorydlg[DLGMEM_2MB].state & SG_SELECTED )
    DialogParams.Memory.nMemorySize = MEMORY_SIZE_2Mb;
  else
    DialogParams.Memory.nMemorySize = MEMORY_SIZE_4Mb;
}


/*-----------------------------------------------------------------------*/
/*
  Show and process the joystick dialog.
*/
void Dialog_JoyDlg(void)
{
  int but;

  SDLGui_CenterDlg(joystickdlg);

  /* Set up dialog from actual values: */

  if( DialogParams.Joysticks.Joy[1].bCursorEmulation )
    joystickdlg[DLGJOY_J1CURSOR].state |= SG_SELECTED;
  else
    joystickdlg[DLGJOY_J1CURSOR].state &= ~SG_SELECTED;

  if( DialogParams.Joysticks.Joy[1].bEnableAutoFire )
    joystickdlg[DLGJOY_J1AUTOFIRE].state |= SG_SELECTED;
  else
    joystickdlg[DLGJOY_J1AUTOFIRE].state &= ~SG_SELECTED;

  if( DialogParams.Joysticks.Joy[0].bCursorEmulation )
    joystickdlg[DLGJOY_J0CURSOR].state |= SG_SELECTED;
  else
    joystickdlg[DLGJOY_J0CURSOR].state &= ~SG_SELECTED;

  if( DialogParams.Joysticks.Joy[0].bEnableAutoFire )
    joystickdlg[DLGJOY_J0AUTOFIRE].state |= SG_SELECTED;
  else
    joystickdlg[DLGJOY_J0AUTOFIRE].state &= ~SG_SELECTED;

  do
  {
    but = SDLGui_DoDialog(joystickdlg);
    if (but == -1)
      bQuitProgram = true;
  }
  while( but!=DLGJOY_EXIT && !bQuitProgram );

  /* Read values from dialog */
  DialogParams.Joysticks.Joy[1].bCursorEmulation = (joystickdlg[DLGJOY_J1CURSOR].state & SG_SELECTED);
  DialogParams.Joysticks.Joy[1].bEnableAutoFire = (joystickdlg[DLGJOY_J1AUTOFIRE].state & SG_SELECTED);
  DialogParams.Joysticks.Joy[0].bCursorEmulation = (joystickdlg[DLGJOY_J0CURSOR].state & SG_SELECTED);
  DialogParams.Joysticks.Joy[0].bEnableAutoFire = (joystickdlg[DLGJOY_J0AUTOFIRE].state & SG_SELECTED);
}


/*-----------------------------------------------------------------------*/
/*
  Show and process the CPU dialog.
*/
void Dialog_CpuDlg(void)
{
  int i;

  SDLGui_CenterDlg(cpudlg);

  /* Set up dialog from actual values: */

  for(i=DLGCPU_68000; i<=DLGCPU_68040; i++)
  {
    cpudlg[i].state &= ~SG_SELECTED;
  }

  cpudlg[DLGCPU_68000+DialogParams.Cpu.level].state |= SG_SELECTED;

  if( DialogParams.Cpu.compatible )
    cpudlg[DLGCPU_PREFETCH].state |= SG_SELECTED;
  else
    cpudlg[DLGCPU_PREFETCH].state &= ~SG_SELECTED;

  /* Show the dialog: */
  SDLGui_DoDialog(cpudlg);

  /* Read values from dialog: */

  for(i=DLGCPU_68000; i<=DLGCPU_68040; i++)
  {
    if( cpudlg[i].state&SG_SELECTED )
    {
      DialogParams.Cpu.level = i-DLGCPU_68000;
      break;
    }
  }

  DialogParams.Cpu.compatible = (cpudlg[DLGCPU_PREFETCH].state & SG_SELECTED);
}

#endif
/*-----------------------------------------------------------------------*/
/*
  This functions sets up the actual font and then displays the main dialog.
*/
int Dialog_MainDlg(bool *bReset)
{
  int retbut;

  Screen_Save();

  SDLGui_PrepareFont();
  SDLGui_CenterDlg(maindlg);
  hostScreen.lock();
  SDL_ShowCursor(SDL_ENABLE);
  hostScreen.unlock();

  maindlg[MAINDLG_NORESET].state |= SG_SELECTED;
  maindlg[MAINDLG_RESET].state &= ~SG_SELECTED;

  do
  {
    retbut = SDLGui_DoDialog(maindlg);
    switch(retbut)
    {
      case MAINDLG_ABOUT:
        SDLGui_CenterDlg(aboutdlg);
        SDLGui_DoDialog(aboutdlg);
        break;
      case MAINDLG_DISCS:
        Dialog_DiscDlg();
        break;
/*
      case MAINDLG_TOSGEM:
        Dialog_TosGemDlg();
        break;
      case MAINDLG_SCREEN:
        Dialog_ScreenDlg();
        break;
      case MAINDLG_SOUND:
        Dialog_SoundDlg();
        break;
      case MAINDLG_CPU:
        Dialog_CpuDlg();
        break;
      case MAINDLG_MEMORY:
        Dialog_MemDlg();
        break;
      case MAINDLG_JOY:
        Dialog_JoyDlg();
        break;
      case MAINDLG_KEYBD:
        SDLGui_CenterDlg(keyboarddlg);
        SDLGui_DoDialog(keyboarddlg);
        break;
      case MAINDLG_DEVICES:
        SDLGui_CenterDlg(devicedlg);
        SDLGui_DoDialog(devicedlg);
        break;
*/
      case MAINDLG_QUIT:
        bQuitProgram = true;
        break;
      case -1:
      	bQuitProgram = true;
      	break;
    }
    Screen_SetFullUpdate();
    Screen_Draw();
  }
  while(retbut!=MAINDLG_OK && retbut!=MAINDLG_CANCEL && !bQuitProgram);

  hostScreen.lock();
  SDL_ShowCursor(SDL_DISABLE);
  hostScreen.unlock();

  if( maindlg[MAINDLG_RESET].state & SG_SELECTED )
    *bReset = true;
  else
    *bReset = false;

  return(retbut==MAINDLG_OK);
}


/*-----------------------------------------------------------------------*/
/*
  Open Property sheet Options dialog
  Return true if user choses OK, or false if cancel!
*/
bool Dialog_DoProperty(bool *bForceReset, bool *bForceQuit)
{
  bool bOKDialog;  /* Did user 'OK' dialog? */
  bool bReset;
#if 0
  Main_PauseEmulation();

  /* Copy details to DialogParams (this is so can restore if 'Cancel' dialog) */
  ConfigureParams.Screen.bFullScreen = bInFullScreen;
  DialogParams = ConfigureParams;

  bSaveMemoryState = false;
  bRestoreMemoryState = false;
#endif
  bOKDialog = Dialog_MainDlg(&bReset);
  *bForceQuit = bQuitProgram;
  if (bOKDialog) {
    *bForceReset = bReset;
  }
#if 0
  /* Copy details to configuration, and ask user if wishes to reset */
  if (bOKDialog)
    Dialog_CopyDialogParamsToConfiguration(bForceReset);
  /* Did want to save/restore memory save? If did, need to re-enter emulation mode so can save in 'safe-zone' */
  if (bSaveMemoryState || bRestoreMemoryState) {
    /* Back into emulation mode, when next VBL occurs state will be safed - otherwise registers are unknown */
    /*FM  View_ToggleWindowsMouse(MOUSE_ST);*/
  }

  Main_UnPauseEmulation();
#endif
  return(bOKDialog);
}


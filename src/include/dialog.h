/* Hatari */

#define MAX_FILENAME_LENGTH 260


/*  Dialog TOS/GEM  */
typedef struct {
  char szTOSImageFileName[MAX_FILENAME_LENGTH];
  bool bUseTimeDate;
  bool bAccGEMGraphics;
  bool bUseExtGEMResolutions;
  int nGEMResolution;
  int nGEMColours;
} DLG_TOSGEM;

enum {
  GEMRES_640x480,
  GEMRES_800x600,
  GEMRES_1024x768
};

enum {
  GEMCOLOUR_2,
  GEMCOLOUR_4,
  GEMCOLOUR_16
};


/* Dialog Sound */
typedef struct {
  bool bEnableSound;
  int nPlaybackQuality;
  char szYMCaptureFileName[MAX_FILENAME_LENGTH];
} DLG_SOUND;

enum {
  PLAYBACK_LOW,
  PLAYBACK_MEDIUM,
  PLAYBACK_HIGH
};


/* Dialog RS232 */
typedef struct {
  bool bEnableRS232;
  int nCOMPort;
} DLG_RS232;

enum {
  COM_PORT_1,
  COM_PORT_2,
  COM_PORT_3,
  COM_PORT_4
};


/* Dialog Keyboard */
typedef struct {
  bool bDisableKeyRepeat;
  int ShortCuts[2][3];     /* F11+F12, NORMAL+SHIFT+CTRL */
  char szMappingFileName[MAX_FILENAME_LENGTH];
} DLG_KEYBOARD;

enum {
  SHORT_CUT_F11,
  SHORT_CUT_F12,
  SHORT_CUT_NONE
};

enum {
  SHORT_CUT_KEY,
  SHORT_CUT_SHIFT,
  SHORT_CUT_CTRL
};


/* Dialog Memory */
typedef struct {
  int nMemorySize;
  char szMemoryCaptureFileName[MAX_FILENAME_LENGTH];
} DLG_MEMORY;

enum {
  MEMORY_SIZE_512Kb,
  MEMORY_SIZE_1Mb,
  MEMORY_SIZE_2Mb,
  MEMORY_SIZE_4Mb
};


/* Dialog Joystick */
typedef struct {
  bool bCursorEmulation;
  bool bEnableAutoFire;
} JOYSTICK;

typedef struct {
  JOYSTICK Joy[2];
} DLG_JOYSTICKS;


/* Dialog Discimage */
typedef struct {
  bool bAutoInsertDiscB;
  char szDiscImageDirectory[MAX_FILENAME_LENGTH];
} DLG_DISCIMAGE;


/* Dialog configure */
typedef struct {
  int nMinMaxSpeed;
  int nPrevMinMaxSpeed;
} DLG_CONFIGURE;

enum {
  MINMAXSPEED_MIN,
  MINMAXSPEED_1,
  MINMAXSPEED_2,
  MINMAXSPEED_3,
  MINMAXSPEED_MAX
};


/* Dialog hard discs */
#define MAX_HARDDRIVES  1
#define DRIVELIST_TO_DRIVE_INDEX(DriveList)  (DriveList+1)
typedef struct {
  int nDriveList;
  bool bBootFromHardDisc;
  int nHardDiscDir;
  char szHardDiscDirectories[MAX_HARDDRIVES][MAX_FILENAME_LENGTH];
  char szHardDiscImage[MAX_FILENAME_LENGTH];
} DLG_HARDDISC;

enum {
  DRIVELIST_NONE,
  DRIVELIST_C,
  DRIVELIST_CD,
  DRIVELIST_CDE,
  DRIVELIST_CDEF
};

enum {
  DRIVE_C,
  DRIVE_D,
  DRIVE_E,
  DRIVE_F
};


/* Dialog screen */
typedef struct {
  bool bDoubleSizeWindow;
  bool bAllowOverscan;
  bool bInterlacedFullScreen;
  bool bSyncToRetrace;
  bool bFrameSkip;
} DLG_SCREEN_ADVANCED;

typedef struct {
  bool bFullScreen;
  DLG_SCREEN_ADVANCED Advanced;
  int ChosenDisplayMode;

  bool bCaptureChange;
  int nFramesPerSecond;
  bool bUseHighRes;
} DLG_SCREEN;


/* Dialog Printer */
typedef struct {
  bool bEnablePrinting;
  bool bPrintToFile;
  char szPrintToFileName[MAX_FILENAME_LENGTH];
} DLG_PRINTER;


/* Dialog CPU */
typedef struct {
  int level;
  bool compatible;
  bool address_space_24;
} DLG_CPU;


/* State of system is stored in this structure */
/* On reset, variables are copied into system globals and used. */
typedef struct {
  /* Configure */
  DLG_CONFIGURE Configure;
  DLG_SCREEN Screen;
  DLG_JOYSTICKS Joysticks;
  DLG_KEYBOARD Keyboard;
  DLG_SOUND Sound;
  DLG_MEMORY Memory;
  DLG_DISCIMAGE DiscImage;
  DLG_HARDDISC HardDisc;
  DLG_TOSGEM TOSGEM;
  DLG_RS232 RS232;
  DLG_PRINTER Printer;
  DLG_CPU Cpu;
} DLG_PARAMS;


extern DLG_PARAMS ConfigureParams, DialogParams;

extern void Dialog_DefaultConfigurationDetails(void);
extern void Dialog_CopyDetailsFromConfiguration(bool bReset);
extern bool Dialog_DoProperty(bool *bReset, bool *bQuit);

/*
 * The ARAnyM MetaDOS driver.
 *
 * This is the FreeMiNT configuration file modified for
 * the ARAnyM HOSTFS.DOS MetaDOS driver
 *
 * 2002 STan
 */

#ifndef _mintfake_h_
#define _mintfake_h_

# ifndef ARAnyM_MetaDOS
# define ARAnyM_MetaDOS
# endif

/* MetaDOS function header macros to let the functions create also metados independent */
# define MetaDOSFile  void *devMD, char *pathNameMD, void *fpMD, long retMD, int opcodeMD,
# define MetaDOSDir   void *devMD, char *pathNameMD, DIR *dirMD, long retMD, int opcodeMD,
# define MetaDOSDTA0  void *devMD, char *pathNameMD, void *dtaMD, long retMD, int opcodeMD
# define MetaDOSDTA0pass  devMD, pathNameMD, dtaMD, retMD, opcodeMD
# define MetaDOSDTA   MetaDOSDTA0,

/* disable some defines from the sys/mint/ *.h */
#ifdef O_GLOBAL
# undef O_GLOBAL
#endif


/* rollback the settings from the FreeMiNT CVS's sys/mint/config.h) */
#ifdef CREATE_PIPES
# undef  CREATE_PIPES
#endif

#ifdef SYSUPDATE_DAEMON
# undef  SYSUPDATE_DAEMON
#endif

#ifdef OLDSOCKDEVEMU
# undef  OLDSOCKDEVEMU
#endif

#ifdef WITH_KERNFS
# undef  WITH_KERNFS
#endif
# define WITH_KERNFS 0

#ifdef DEV_RANDOM
# undef  DEV_RANDOM
#endif

#ifdef PATH_MAX
# undef  PATH_MAX
#endif
# define PATH_MAX 1024

#ifdef SPRINTF_MAX
# undef  SPRINTF_MAX
#endif
# define SPRINTF_MAX 128


#endif /* _mintfake_h_ */

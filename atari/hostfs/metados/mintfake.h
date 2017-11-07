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

# include "debug.h"

/* MetaDOS function header macros to let the functions create also metados independent */
# define MetaDOSFile  void *devMD, char *pathNameMD, FILEPTR *fpMD, long retMD, int opcodeMD,
# define MetaDOSDir   void *devMD, char *pathNameMD, DIR *dirMD, long retMD, int opcodeMD,
# define MetaDOSDirpass   devMD, pathNameMD, dirMD, retMD, opcodeMD,
# define MetaDOSDTA0  void *devMD, char *pathNameMD, DTABUF *dtaMD, long retMD, int opcodeMD
# define MetaDOSDTA0pass  devMD, pathNameMD, dtaMD, retMD, opcodeMD
# define MetaDOSDTA   MetaDOSDTA0,

/* disable some defines from the sys/mint/ *.h */
#ifdef O_GLOBAL
# undef O_GLOBAL
#endif

#define get_curproc() curproc

/* rollback the settings from the FreeMiNT CVS's sys/mint/config.h) */
#ifdef CREATE_PIPES
# undef  CREATE_PIPES
#endif

#ifdef PATH_MAX
# undef  PATH_MAX
#endif
# define PATH_MAX 1024


#endif /* _mintfake_h_ */

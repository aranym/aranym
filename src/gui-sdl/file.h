/*
  Hatari
*/

#ifndef HATARI_FILE_H
#define HATARI_FILE_H

#include "sysdeps.h"

#define MAX_FILENAME_LENGTH 260

/* File types */
enum {
  FILEFILTER_DISCFILES,
  FILEFILTER_ALLFILES,
  FILEFILTER_TOSROM,
  FILEFILTER_MAPFILE,
  FILEFILTER_YMFILE,
  FILEFILTER_MEMORYFILE
};

extern void File_CleanFileName(char *pszFileName);
extern void File_AddSlashToEndFileName(char *pszFileName);
extern bool File_DoesFileExtensionMatch(char *pszFileName, char *pszExtension);
extern bool File_IsRootFileName(char *pszFileName);
extern char *File_RemoveFileNameDrive(char *pszFileName);
extern bool File_DoesFileNameEndWithSlash(char *pszFileName);
extern void File_RemoveFileNameTrailingSlashes(char *pszFileName);
extern int File_Length(char *pszFileName);
extern bool File_Exists(const char *pszFileName);
extern bool File_Delete(char *pszFileName);
extern bool File_QueryOverwrite(char *pszFileName);
extern void File_splitpath(char *pSrcFileName, char *pDir, char *pName, char *Ext);
extern void File_makepath(char *pDestFileName, char *pDir, char *pName, char *pExt);
extern void File_ShrinkName(char *pDestFileName, char *pSrcFileName, int maxlen);

#endif /* HATARI_FILE_H */

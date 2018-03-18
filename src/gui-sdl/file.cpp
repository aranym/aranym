/*
  Hatari

  common file access
*/

#include "sysdeps.h"
#include "file.h"

#define DEBUG 0
#include "debug.h"

# include <cstdlib>
# include <cstdio>
# include <cstring>

#ifdef OS_solaris
  #define DIRHANDLE dirp->d_fd
#elif defined(OS_mingw)
  #define DIRHANDLE dirp->dd_handle
#else
  #define DIRHANDLE dirp->fd
#endif

/*-----------------------------------------------------------------------*/
/*
  Remove any '/'s from end of filenames, but keeps / intact
*/
void File_CleanFileName(char *pszFileName)
{
  int len;

  len = strlen(pszFileName);

  /* Security length check: */
  if( len>MAX_FILENAME_LENGTH )
  {
    pszFileName[MAX_FILENAME_LENGTH-1] = 0;
    len = MAX_FILENAME_LENGTH;
  }

  /* Remove end slash from filename! But / remains! Doh! */
  if( len>2 && pszFileName[len-1]=='/' )
    pszFileName[len-1] = 0;
}


/*-----------------------------------------------------------------------*/
/*
  Add '/' to end of filename
*/
void File_AddSlashToEndFileName(char *pszFileName)
{
  /* Check dir/filenames */
  if (strlen(pszFileName)!=0) {
    if (pszFileName[strlen(pszFileName)-1]!='/')
      strcat(pszFileName,"/");  /* Must use end slash */
  }
}


/*-----------------------------------------------------------------------*/
/*
  Does filename extension match? If so, return true
*/
bool File_DoesFileExtensionMatch(char *pszFileName, char *pszExtension)
{
  if ( strlen(pszFileName) < strlen(pszExtension) )
    return(false);
  /* Is matching extension? */
  if ( !strcasecmp(&pszFileName[strlen(pszFileName)-strlen(pszExtension)], pszExtension) )
    return(true);

  /* No */
  return(false);
}


/*-----------------------------------------------------------------------*/
/*
  Check if filename is from root

  Return true if filename is '/', else give false
*/
bool File_IsRootFileName(char *pszFileName)
{
  if (pszFileName[0]=='\0')     /* If NULL string return! */
    return(false);

  if (pszFileName[0]=='/')
    return(true);

  return(false);
}


/*-----------------------------------------------------------------------*/
/*
  Return string, to remove 'C:' part of filename
*/
char *File_RemoveFileNameDrive(char *pszFileName)
{
  if ( (pszFileName[0]!='\0') && (pszFileName[1]==':') )
    return(&pszFileName[2]);
  else
    return(pszFileName);
}


/*-----------------------------------------------------------------------*/
/*
  Check if filename end with a '/'

  Return true if filename ends with '/'
*/
bool File_DoesFileNameEndWithSlash(char *pszFileName)
{
  if (pszFileName[0]=='\0')    /* If NULL string return! */
    return(false);

  /* Does string end in a '/'? */
  if (pszFileName[strlen(pszFileName)-1]=='/')
    return(true);

  return(false);
}


/*-----------------------------------------------------------------------*/
/*
  Remove any double '/'s  from end of filenames. So just the one
*/
void File_RemoveFileNameTrailingSlashes(char *pszFileName)
{
  int Length;

  /* Do have slash at end of filename? */
  Length = strlen(pszFileName);
  if (Length>=3) {
    if (pszFileName[Length-1]=='/') {     /* Yes, have one previous? */
      if (pszFileName[Length-2]=='/')
        pszFileName[Length-1] = '\0';     /* then remove it! */
    }
  }
}

/*-----------------------------------------------------------------------*/
/*
  Return size of file, -1 if error
*/
int File_Length(char *pszFileName)
{
  FILE *DiscFile;
  int FileSize;
  DiscFile = fopen(pszFileName, "rb");
  if (DiscFile!=NULL) {
    fseek(DiscFile, 0, SEEK_END);
    FileSize = ftell(DiscFile);
    fseek(DiscFile, 0, SEEK_SET);
    fclose(DiscFile);
    return(FileSize);
  }

  return(-1);
}


/*-----------------------------------------------------------------------*/
/*
  Return true if file exists
*/
bool File_Exists(const char *pszFileName)
{
  FILE *DiscFile;

  /* Attempt to open file */
  DiscFile = fopen(pszFileName, "rb");
  if (DiscFile!=NULL) {
    fclose(DiscFile);
    return(true);
  }
  return(false);
}


/*-----------------------------------------------------------------------*/
/*
  Delete file, return true if OK
*/
bool File_Delete(char *pszFileName)
{
  /* Delete the file (must be closed first) */
  return( os_remove(pszFileName) );
}


/*-----------------------------------------------------------------------*/
/*
  Find if file exists, and if so ask user if OK to overwrite
*/
bool File_QueryOverwrite(char *pszFileName)
{

  char szString[MAX_FILENAME_LENGTH];

  /* Try and find if file exists */
  if (File_Exists(pszFileName)) {
    /* File does exist, are we OK to overwrite? */
    sprintf(szString,"File '%s' exists, overwrite?",pszFileName);
/* FIXME: */
//    if (MessageBox(hWnd,szString,PROG_NAME,MB_YESNO | MB_DEFBUTTON2 | MB_ICONSTOP)==IDNO)
//      return(false);
  }

  return(true);
}

/*-----------------------------------------------------------------------*/
/*
  Split a complete filename into path, filename and extension.
  If pExt is NULL, don't split the extension from the file name!
*/
void File_splitpath(char *pSrcFileName, char *pDir, char *pName, char *pExt)
{
  char *ptr1, *ptr2;

  /* Build pathname: */
  ptr1 = strrchr(pSrcFileName, '/');
  if( ptr1 )
  {
    strcpy(pDir, pSrcFileName);
    strcpy(pName, ptr1+1);
    pDir[ptr1-pSrcFileName+1] = 0;
  }
  else
  {
    strcpy(pDir, "");
    strcpy(pName, pSrcFileName);
  }

  /* Build the raw filename: */
  if( pExt!=NULL )
  {
    ptr2 = strrchr(pName+1, '.');
    if( ptr2 )
    {
      pName[ptr2-pName] = 0;
      /* Copy the file extension: */
      strcpy(pExt, ptr2+1);
    }
    else
      pExt[0] = 0;
   }
}


/*-----------------------------------------------------------------------*/
/*
  Build a complete filename from path, filename and extension.
  pExt can also be NULL.
*/
void File_makepath(char *pDestFileName, char *pDir, char *pName, char *pExt)
{
  strcpy(pDestFileName, pDir);
  if( strlen(pDestFileName)==0 )
    strcpy(pDestFileName, "");
  if( pDestFileName[strlen(pDestFileName)-1]!='/' )
    strcat(pDestFileName, "/");

  strcat(pDestFileName, pName);

  if( pExt!=NULL )
  {
    if( strlen(pExt)>0 && pExt[0]!='.' )
      strcat(pDestFileName, ".");
    strcat(pDestFileName, pExt);
  }
}


/*-----------------------------------------------------------------------*/
/*
  Shrink a file name to a certain length and insert some dots if we cut
  something away (usefull for showing file names in a dialog).
*/
void File_ShrinkName(char *pDestFileName, char *pSrcFileName, int maxlen)
{
  int srclen = strlen(pSrcFileName);
  if( srclen<maxlen )
    strcpy(pDestFileName, pSrcFileName);  /* It fits! */
  else
  {
    strncpy(pDestFileName, pSrcFileName, maxlen/2);
    if(maxlen&1)  /* even or uneven? */
      pDestFileName[maxlen/2-1] = 0;
    else
      pDestFileName[maxlen/2-2] = 0;
    strcat(pDestFileName, "...");
    strcat(pDestFileName, &pSrcFileName[strlen(pSrcFileName)-maxlen/2+1]);
  }
}


/*<<---------------[		 cfgopts.c		  ]------------------------/
/																	   /
/  Functional														   /
/	  Description: Configuration file I/O							   /
/																	   /
/  Input		 : Configuration file name							   /
/				   Configuration parameters in a structure			   /
/																	   /
/  Process		 : Interpret information by parameter and read or	   /
/				   write back to the configuration file.			   /
/																	   /
/  Ouput		 : updated configuration file or updated structure.	   /
/																	   /
/  Programmer	 : Jeffry J. Brickley								   /
/																	   /
/																	   /
/---------------------------------------------------------------------*/
#define		  Assigned_Revision 950802
/*-------------------------[ Revision History ]------------------------/
/ Revision 1.0.0  :	 Original Code by Jeffry J. Brickley			   /
/		   1.1.0  : added header capability, JJB, 950802
/		   1.2.0  : ANSIzed, added WHITESPACEs, better header file, Joy/
/		   1.3.0  : comments handling, bugfixes						Joy/
/		   1.4.0  : proper updating of config file					Joy/
/		   1.5.0  : added Char tag and one empty line before HEAD	Joy/
/		   1.6.0  : fixed Char tag, allowed empty Strings			Joy/
/		   1.6.1  : empty line between sections						Joy/
/		   1.6.2  : default boolean values are "Yes"/"No" now		Joy/
/		   1.6.3  : fixed Booolean tag								Joy/
/		   1.7.0  : added buf_size to Config_Tag struct,			   /
/					increased max cfg line length up to 32767 chars Joy/
/		   1.7.1  : fcopy error values propagated to update_conf()	   /
/					fcopy checks for incomplete write (disk full)	Joy/
/          1.8.0  : C++ version, Boolean tag replaced with C++ bool,   /
/                   Int_Tag added                                   Joy/
/------------------------------------------------------------------->>*/
/*	Please keep revision number current.							  */
#define		  REVISION_NO "1.8.0"

extern "C" {

#define ERROR	-1

/*---------------------------------------------------------------------/
/
/  Description:	 CfgOpts is based on GETOPTS by Bob Stout.	It will
/				 process a configuration file based one words and
/				 store it in a structure pointing to physical data
/				 area for each storage item.
/  i.e. ???.CFG:
/	 Port=1
/	 work_space=C:\temp
/	 menus=Yes
/	 user=Jeffry Brickley
/  will write to the following structure:
/	 struct Config_Tag configs[] = {
/	 {"port",		Word_Tag,	 &port_number},
/	 {"work_space", String_Tag,	 work_space, sizeof(work_space)},
/	 {"menus",		Bool_Tag, &menu_flag},
/	 {"user",		String_Tag,	 User_name, sizeof(User_name)},
/	 {NULL,			Error_Tag,	 NULL}
/	 };
/  Note that the structure must always be terminated by a NULL row as
/	  was the same with GETOPTS.  This however is slightly more
/	  complicated than scaning the command line (but not by much) for
/	  data as there can be more variety in words than letters and an
/	  number of data items limited only by memory.	Currently CfgOpts
/	  is not case sensitive, but this can be changed by replacing all
/	  "strcasecmp" functions with "strcmp" functions.
/
/  Like the original code from which this was taken, this is released
/  to the Public Domain.  I cannot make any guarentees other than these
/  work for me and I find them usefull.	 Feel free to pass these on to
/  a friend, but please do not charge him....
/
/---------------------------------------------------------------------*/

#include "cfgopts.h"

#ifdef HAVE_NEW_HEADERS
# include <cstdio>
# include <cstdlib>
# include <cstring>
#else
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
#endif

char	*trim(char *buffer)
{
#define	SPACE	' '
#define	TABULA	'\t'

	if (buffer != NULL) {
		int	i, linelen = strlen(buffer);

		for (i = 0; i < linelen; i++)
			if (buffer[i] != SPACE && buffer[i] != TABULA)
				break;

		if (i > 0 && i < linelen) {
			linelen -= i;
			memcpy(buffer, buffer + i, linelen);	/* mezery zleva pryc */
		}

		for (i = linelen; i > 0; i--) {
			int	j = i-1;
			if (buffer[j] != SPACE && buffer[j] != TABULA)
				break;
		}

		buffer[i] = '\0';						/* mezery zprava pryc */
	}

	return buffer;
}


char	*strip_comment(char *line)
{
#define	REM1	'#'
#define REM2	';'
	int	i, j;

	if (line == NULL)
		return NULL;

	j = strlen(line);
	for (i = 0; i < j; i++)
		if (line[i] == REM1 || line[i] == REM2) {
			line[i] = '\0';
			break;
		}

	return line;
}


/*
 * FCOPY.C - copy one file to another.	Returns the (positive)
 *			 number of bytes copied, or -1 if an error occurred.
 * by: Bob Jarvis
 */

#define BUFFER_SIZE 1024

/*---------------------------------------------------------------------/
/	copy one file to another.
/---------------------------------------------------------------------*/
/*>>------[		  fcopy()	   ]-------------[ 08-02-95 14:02PM ]------/
/ return value:
/	  long					  ; Number of bytes copied or -1 on error
/ parameters:
/	  char *dest			  ; Destination file name
/	  char *source			  ; Source file name
/-------------------------------------------------------------------<<*/
long fcopy(const char *dest, const char *source)
{
	FILE * d, *s;
	char	*buffer;
	size_t incount, outcount;
	long	totcount = 0L;

	s = fopen(source, "rb");
	if (s == NULL)
		return - 1L;

	d = fopen(dest, "wb");
	if (d == NULL) {
		fclose(s);
		return - 1L;
	}

	buffer = (char *)malloc(BUFFER_SIZE);
	if (buffer == NULL) {
		fclose(s);
		fclose(d);
		return - 1L;
	}

	incount = fread(buffer, sizeof(char), BUFFER_SIZE, s);
	outcount = 0;

	while (!feof(s)) {
		totcount += (long)incount;
		outcount += fwrite(buffer, sizeof(char), incount, d);
		incount = fread(buffer, sizeof(char), BUFFER_SIZE, s);
	}

	totcount += (long)incount;
	outcount += fwrite(buffer, sizeof(char), incount, d);

	free(buffer);
	fclose(s);
	fclose(d);

	if (outcount < incount)
		return -1L;				/* disk full? */

	return totcount;
}


static char	line[32768];

/*---------------------------------------------------------------------/
/	reads from an input configuration (INI) file.
/---------------------------------------------------------------------*/
/*>>------[	  input_config()   ]-------------[ 08-02-95 14:02PM ]------/
/ return value:
/	  int					  ; number of records read or -1 on error
/ parameters:
/	  char *filename		  ; filename of INI style file
/	  struct Config_Tag configs[]; Configuration structure
/	  char *header			  ; INI header name (i.e. "[TEST]")
/-------------------------------------------------------------------<<*/
int	input_config(const char *filename, struct Config_Tag configs[], char *header)
{
	struct Config_Tag *ptr;
	int	count = 0, lineno = 0, temp;
	FILE * file;
	char	*fptr, *tok, *next;

	file = fopen(filename, "rt");
	if ( file == NULL ) 
		return ERROR;	/* return error designation. */
	if ( header != NULL )
		do {
			fptr = trim(fgets(line, sizeof(line), file));  /* get input line */
		} while ( memcmp(line, header, strlen(header)) && !feof(file));

	if ( !feof(file) ) 
		do {
			fptr = trim(fgets(line, sizeof(line), file));  /* get input line */
			if ( fptr == NULL ) 
				continue;
			lineno++;
			strip_comment(line);

			if ( line[0] == '[' ) 
				continue;	 /* next header is the end of our section */

			tok = trim(strtok(line, "=\n\r"));	 /* get first token */
			if ( tok != NULL ) {
				next = trim(strtok(NULL, "=\n\r")); /* get actual config information */
				for ( ptr = configs; ptr->buf; ++ptr )	 /* scan for token */ {
					if ( !strcasecmp( tok , ptr->code ) )  /* got a match? */ {
						if (next == NULL) {
							if ( ptr->type == String_Tag ) {/* string may be empty */
								*(char *)ptr->buf = 0;
								++count;
							}
							else {
								fprintf(stderr, ">>> Missing value in Config file %s on line %d !!!\n", filename, lineno);
							}
							continue;
						}
						switch ( ptr->type )	 /* check type */ {
						case Bool_Tag:
							*((bool * )(ptr->buf)) = (!strcasecmp(next, "FALSE") || !strcasecmp(next, "No")) ? false : true;
							++count;
							break;

						case Byte_Tag:
							sscanf(next, "%d", &temp);
							*((char *)(ptr->buf)) = (char)temp;
							++count;
							break;

						case Word_Tag:
							sscanf(next, "%hd", (short *)(ptr->buf));
							++count;
							break;

						case Int_Tag:
							sscanf(next, "%d", (int *)(ptr->buf));
							++count;
							break;

						case Long_Tag:
							sscanf(next, "%ld", (long *)(ptr->buf));
							++count;
							break;

						case OctWord_Tag:
							sscanf(next, "%ho", (short *)(ptr->buf));
							++count;
							break;

						case OctLong_Tag:
							sscanf(next, "%lo", (long *)(ptr->buf));
							++count;
							break;

						case HexWord_Tag:
							sscanf(next, "%hx", (short *)(ptr->buf));
							++count;
							break;

						case HexLong_Tag:
							sscanf(next, "%lx", (long *)(ptr->buf));
							++count;
							break;

						case Float_Tag:
							sscanf(next, "%g", (float *)ptr->buf);
							++count;
							break;

						case Double_Tag:
							sscanf(next, "%lg", (double *)ptr->buf);
							++count;
							break;

						case Char_Tag:
							*(char *)ptr->buf = *next;
							++count;
							break;

						case String_Tag:
							if (ptr->buf_size > 0) {
								char *cptr = (char *)ptr->buf;
								int bufsize = ptr->buf_size;
								strncpy(cptr, next, bufsize);
								cptr[bufsize-1] = '\0';	/* EOS */
								++count;
							}
							else {
								fprintf(stderr, ">>> Wrong buf_size in Config_Tag struct: directive %s, buf_size %d !!!\n", ptr->code, ptr->buf_size);
							}
							break;

						case Function_Tag:
						case Error_Tag:
						default:
							printf("Error in Config file %s on line %d\n",
								filename, lineno);
							break;
						}
					}
				}
			}
		} while ( fptr != NULL && line[0] != '[');
	fclose(file);
	return count;
}

bool write_token(FILE *outfile, struct Config_Tag *ptr)
{
	int temp;
	bool ret_flag = true;

	ptr->stat = 1;	/* jiz ulozeno do souboru */

	fprintf(outfile, "%s = ", ptr->code);
	switch ( ptr->type )  /* check type */ {
	case Bool_Tag:
		fprintf(outfile, "%s\n", *((bool *)(ptr->buf)) ? "Yes" : "No");
		break;

	case Byte_Tag:
		temp = (int)*((char *)(ptr->buf));
		fprintf(outfile, "%hd\n", temp);
		break;

	case Word_Tag:
		fprintf(outfile, "%hd\n", *((short *)(ptr->buf)));
		break;

	case Int_Tag:
		fprintf(outfile, "%d\n", *((int *)(ptr->buf)));
		break;

	case Long_Tag:
		fprintf(outfile, "%ld\n", *((long *)(ptr->buf)));
		break;

	case OctWord_Tag:
		fprintf(outfile, "%ho\n", *((short *)(ptr->buf)));
		break;

	case OctLong_Tag:
		fprintf(outfile, "%lo\n", *((long *)(ptr->buf)));
		break;

	case HexWord_Tag:
		fprintf(outfile, "%hx\n", *((short *)(ptr->buf)));
		break;

	case HexLong_Tag:
		fprintf(outfile, "%lx\n", *((long *)(ptr->buf)));
		break;

	case Float_Tag:
		fprintf(outfile, "%g\n", *((float *)ptr->buf));
		break;

	case Double_Tag:
		fprintf(outfile, "%g\n", *((double *)ptr->buf));
		break;

	case Char_Tag:
		fprintf(outfile, "%c\n", *(char *)ptr->buf);
		break;

	case String_Tag:
		fprintf(outfile, "%s\n", (char *)ptr->buf);
		break;

	case Error_Tag:
	case Function_Tag:
	default:
		printf("Error in Config structure (Contact author).\n");
		ret_flag = false;
	}
	return ret_flag;
}

/*---------------------------------------------------------------------/
/	updates an input configuration (INI) file from a structure.
/---------------------------------------------------------------------*/
/*>>------[	  update_config()  ]-------------[ 08-02-95 14:02PM ]------/
/ return value:
/	  int					  ; Number of records read & updated
/ parameters:
/	  char *filename		  ; filename of INI file
/	  struct Config_Tag configs[]; Configuration structure
/	  char *header			  ; INI header name (i.e. "[TEST]")
/-------------------------------------------------------------------<<*/
int	update_config(const char *filename, struct Config_Tag configs[], char *header)
{
	struct Config_Tag *ptr;
	int	count = 0, lineno = 0;
	FILE * infile, *outfile;
	char	*fptr, *tok;
	int result = 0;

	for ( ptr = configs; ptr->buf; ++ptr )
		ptr->stat = 0;	/* jeste neulozeno do souboru */

	infile = fopen(filename, "rt");
	if ( infile == NULL ) {
/* konfiguracni soubor jeste vubec neexistuje */
		outfile = fopen("temp.$$$", "wt");
		if ( outfile == NULL )
			return ERROR;		/* return error designation. */
		if ( header != NULL ) {
			fprintf(outfile, "%s\n", header);
		}
		for ( ptr = configs; ptr->buf; ++ptr )	  /* scan for token */ {
			if ( write_token(outfile, ptr) )
				++count;
		}

		fclose(outfile);
		result = fcopy(filename, "temp.$$$");
		remove("temp.$$$");

		if (result < 0)
			return result;
		return count;
	}
	outfile = fopen("temp.$$$", "wt");
	if ( outfile == NULL ) {
		fclose(infile);
		return ERROR;		   /* return error designation. */
	}
	if ( header != NULL ) {
/* konfiguracni soubor existuje a je otevren - hledame nasi sekci */
		do {
			fptr = trim(fgets(line, sizeof(line), infile));	 /* get input line */
			if (feof(infile))
				break;

			fprintf(outfile, "%s", line);
		} while ( memcmp(line, header, strlen(header)));
	}
	if ( feof(infile) ) {
/* v jiz existujicim konfiguracnim souboru neni sekce, kterou zapisujeme */

		if ( header != NULL ) {
			fprintf(outfile, "\n%s\n", header);
		}
		for ( ptr = configs; ptr->buf; ++ptr )	  /* scan for token */ {
			if ( write_token(outfile, ptr) )
				++count;
		}
	} else {
/* v jiz existujicim souboru byla nalezena sekce, kterou mame updatovat */
		for (;;) {
			fptr = trim(fgets(line, sizeof(line), infile)); /* get input line */
			if ( fptr == NULL ) 
				break;
			lineno++;
			if ( line[0] == REM1 || line[0] == REM2 ) {
				fprintf(outfile, "%s", line);
				continue;  /* skip comments */
			}
			if ( line[0] == '[' || feof(infile) ) {	/* konec nasi sekce */
				break;	/* zbytek konfig. souboru jen opis z puvodniho */
			}

			tok = trim(strtok(line, "=\n\r"));	/* get first token */
			if ( tok != NULL ) {
				for ( ptr = configs; ptr->buf; ++ptr )	/* scan for token */ {
					if ( !strcasecmp( tok , ptr->code ) ) /* got a match? */ {
						if ( write_token(outfile, ptr) ) {
							++count;
						}
					}
				}
			}
		}
/* nasli jsme zacatek dalsi sekce - tedy zkoncila nase sekce. Nyni je nutne 
   zjistit, ktere radky struktury jsme jeste neupdatovali a ty tam v podstate
   zapsat jako by konfiguracni soubor ani neexistoval */

		for ( ptr = configs; ptr->buf; ++ptr ) {
			if (ptr->stat == 0)
				if ( write_token(outfile, ptr) )
					++count;
		}

		if ( !feof(infile) && fptr != NULL)
				fprintf(outfile, "\n%s", line);/* doplnit prvni radku dalsi sekce */

		/* zkopirovat zbytek konfiguracniho souboru (cizi sekce) */
		for(;;) {
			fptr = trim(fgets(line, sizeof(line), infile));	 /* get input line */
			if (feof(infile))
				break;

			fprintf(outfile, "%s", line);
		}
	}
	fclose(infile);
	fclose(outfile);
	result = fcopy(filename, "temp.$$$");
	remove("temp.$$$");

	if (result < 0)
		return result;
	return count;
}


#ifdef TEST

#include <stdlib.h>

bool test1 = true, test2 = false;
short	test3 = -37;
long	test4 = 100000L;
char	test5[80] = "Default string";

struct Config_Tag configs[] = {
	   { "test1", Bool_Tag, &test1  }, /* Valid options		  */
	   { "test2", Bool_Tag, &test2  }, 
	   { "test3", Word_Tag, &test3	   }, 
	   { "test4", Long_Tag, &test4	  }, 
	   { "test5", String_Tag, test5, sizeof(test5)	  }, 
	   { NULL , Error_Tag, NULL		   }/* Terminating record	*/

	 };


#define TFprint(v) ((v) ? "TRUE" : "FALSE")

/*---------------------------------------------------------------------/
/	test main routine, read/write to a sample INI file.
/---------------------------------------------------------------------*/
/*>>------[		  main()	   ]-------------[ 08-02-95 14:02PM ]------/
/ return value:
/	  int					  ; 0
/ parameters:
/	  int argc				  ; number of arguments
/	  char *argv[]			  ; command line arguments
/-------------------------------------------------------------------<<*/
int	main(int argc, char *argv[])
{
	int	i;
	printf("Defaults:\ntest1 = %s\ntest2 = %s\ntest3 = %d\ntest4 = %ld\n"
		"test5 = \"%s\"\n\n", TFprint(test1), TFprint(test2), test3,
		test4, test5);

	printf("input_config() returned %d\n",
		input_config("test.cfg", configs, "[TEST1]"));

	printf("Options are now:\ntest1 = %s\ntest2 = %s\ntest3 = %d\n"
		"test4 = %ld\ntest5 = \"%s\"\n\n", TFprint(test1),
		TFprint(test2), test3, test4, test5);

#ifdef TEST_UPDATE
	test1 = true;
	test2 = false;
	test3 = -37;
	test4 = 100000L;
	strcpy(test5, "Default value");

	printf("update_config() returned %d\n",
		update_config("test.cfg", configs, "[TEST2]"));

	printf("Options are now:\ntest1 = %s\ntest2 = %s\ntest3 = %d\n"
		"test4 = %ld\ntest5 = \"%s\"\n\n", TFprint(test1),
		TFprint(test2), test3, test4, test5);
#endif

	return 0;
}

#endif

} // extern "C"


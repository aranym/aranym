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
/          1.8.1  : trim() - memmove() for overlapping memory. Thanks  /
/                   to Thothy                                       Joy/
/          1.9.0  : compress/expand Path                            Joy/
/------------------------------------------------------------------->>*/

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

#include "sysdeps.h"
#include "cfgopts.h"

#define DEBUG 0
#include "debug.h"

ConfigOptions::ConfigOptions(const char *cfgfile, const char *home, const char *data)
{
	config_file = cfgfile;

	// prepare home_folder with trailing slash
	safe_strncpy(home_folder, home, sizeof(home_folder));
	int hflen = strlen(home_folder);
	if (hflen > 0 && home_folder[hflen-1] != '/' && home_folder[hflen-1] != '\\')
		strcat(home_folder, "/");

	// prepare data_folder with trailing slash
	safe_strncpy(data_folder, data, sizeof(data_folder));
	int dflen = strlen(data_folder);
	if (dflen > 0 && data_folder[dflen-1] != '/' && data_folder[dflen-1] != '\\')
		strcat(data_folder, "/");
	
	// prepare config_folder with trailing slash
	safe_strncpy(config_folder, config_file, sizeof(config_folder));
	char *slash = strrchr(config_folder, '/');
	char *alt_slash = strrchr(config_folder, '\\');
	if (slash != NULL) {
		slash[1] = '\0';
	}
	else if (alt_slash != NULL) {
		alt_slash[1] = '\0';
	}
	else {
		config_folder[0] = '\0';
	}
}

char * ConfigOptions::trim(char *buffer)
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
			memmove(buffer, buffer + i, linelen);	/* trim spaces on left */
		}

		for (i = linelen; i > 0; i--) {
			int	j = i-1;
			if (buffer[j] != SPACE && buffer[j] != TABULA)
				break;
		}

		buffer[i] = '\0';						/* trim spaces on right */
	}

	return buffer;
}


char * ConfigOptions::strip_comment(char *line)
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
long ConfigOptions::fcopy(const char *dest, const char *source)
{
	FILE * d, *s;
	char	*buffer;
	size_t incount, outcount;
	long	totcount = 0L;

	s = fopen(source, "rb");
	if (s == NULL)
		return -1L;

	d = fopen(dest, "wb");
	if (d == NULL) {
		fclose(s);
		return -1L;
	}

	buffer = (char *)malloc(BUFFER_SIZE);
	if (buffer == NULL) {
		fclose(s);
		fclose(d);
		return -1L;
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

void ConfigOptions::expand_path(char *dest, const char *path, unsigned short buf_size)
{
	dest[0] = '\0';

	if ( !strlen(path) )
		return;

	size_t prefixLen = 0;

	if ( path[0] == '~' && (path[1] == '/' || path[1] == '\\') ) {
		safe_strncpy(dest, home_folder, buf_size);
		path+=2;
	} else if (path[0] == '*' && (path[1] == '/' || path[1] == '\\') ) {
		safe_strncpy(dest, data_folder, buf_size);
		path+=2;
	} else if ( path[0] != '/' && path[0] != '\\' && path[1] != ':' ) {
		safe_strncpy(dest, config_folder, buf_size);
	}
	else {
		safe_strncpy(dest, path, buf_size);
		return;
	}

	prefixLen = strlen( dest );

	if ( buf_size >= prefixLen + strlen(path) ) {
		memmove( dest + prefixLen, path, strlen(path)+1 );	
	} else {
		panicbug("Error - config entry size is insufficient");
	}
}

void ConfigOptions::compress_path(char *dest, char *path, unsigned short buf_size)
{
	dest[0] = '\0';

	if ( !strlen(path) ) {
		return;
	}

	size_t prefixLen = 0;
	const char *replacement = NULL;

	safe_strncpy(dest, config_folder, buf_size);
	prefixLen = strlen(dest);
	if (prefixLen && strncmp(path, dest, prefixLen) == 0) {
		replacement = "";
		D(bug("%s matches %.*s", path, prefixLen, dest));
	} 
	else {
		safe_strncpy(dest, data_folder, buf_size);
		prefixLen = strlen(dest);
		if (prefixLen && strncmp(path, dest, prefixLen) == 0) {
			replacement = "*";
			--prefixLen;
			D(bug("%s matches %.*s", path, prefixLen, dest));
		} 
		else {
			safe_strncpy(dest, home_folder, buf_size);
			prefixLen = strlen(dest);
			if (prefixLen && strncmp(path, dest, prefixLen) == 0) {
				replacement = "~";
				--prefixLen;
				D(bug("%s matches %.*s", path, prefixLen, dest));
			}
		}
	}

	if (replacement) {
		int len1 = strlen(replacement);
		strcpy(dest, replacement);
		safe_strncpy(dest+len1, path+prefixLen, buf_size - len1);
	}
	else {
		safe_strncpy(dest, path, buf_size);
	}
}


bool ConfigOptions::set_config_value(struct Config_Tag *ptr, const char *value)
{
	int temp;
	
	if (value == NULL) {
		if ( ptr->type == Path_Tag ||
		     ptr->type == String_Tag ) {/* path or string may be empty */
			*(char *)ptr->buf = 0;
			return true;
		}
		return false;
	}
	switch ( ptr->type )	 /* check type */ {
	case Bool_Tag:
		*((bool * )(ptr->buf)) = (!strcasecmp(value, "FALSE") || !strcasecmp(value, "No")) ? false : true;
		break;

	case Byte_Tag:
		sscanf(value, "%d", &temp);
		*((char *)(ptr->buf)) = (char)temp;
		break;

	case Word_Tag:
		sscanf(value, "%hd", (short *)(ptr->buf));
		break;

	case Int_Tag:
		sscanf(value, "%d", (int *)(ptr->buf));
		break;

	case Long_Tag:
		sscanf(value, "%ld", (long *)(ptr->buf));
		break;

	case OctWord_Tag:
		sscanf(value, "%ho", (short *)(ptr->buf));
		break;

	case OctLong_Tag:
		sscanf(value, "%lo", (long *)(ptr->buf));
		break;

	case HexWord_Tag:
		sscanf(value, "%hx", (short *)(ptr->buf));
		break;

	case HexLong_Tag:
		sscanf(value, "%lx", (long *)(ptr->buf));
		break;

	case Float_Tag:
		sscanf(value, "%g", (float *)ptr->buf);
		break;

	case Double_Tag:
		sscanf(value, "%lg", (double *)ptr->buf);
		break;

	case Char_Tag:
		*(char *)ptr->buf = *value;
		break;

	case Path_Tag:
		if (ptr->buf_size > 0) {
			char tmpbuf[MAX_PATH];
			safe_strncpy(tmpbuf, value, sizeof(tmpbuf));
			expand_path((char *)ptr->buf, tmpbuf, ptr->buf_size);
		}
		else {
			panicbug(">>> Wrong buf_size in Config_Tag struct: directive %s, buf_size %d !!!", ptr->code, ptr->buf_size);
		}
		break;

	case String_Tag:
		if (ptr->buf_size > 0) {
			safe_strncpy((char *)ptr->buf, value, ptr->buf_size);
		}
		else {
			panicbug(">>> Wrong buf_size in Config_Tag struct: directive %s, buf_size %d !!!", ptr->code, ptr->buf_size);
		}
		break;
	case Function_Tag:
	case Error_Tag:
	default:
		return false;
	}
	return true;
}


char *ConfigOptions::get_config_value(const struct Config_Tag *ptr, bool type)
{
	char value[40];
	char *valuep = NULL;
	
	switch ( ptr->type ) {
	case Bool_Tag:
		if (type)
			strcpy(value, "bool");
		else
			strcpy(value, *((bool * )(ptr->buf)) ? "true" : "false");
		break;

	case Byte_Tag:
		if (type)
			strcpy(value, "byte");
		else
			sprintf(value, "%d", *((char *)(ptr->buf)));
		break;

	case Word_Tag:
		if (type)
			strcpy(value, "word");
		else
			sprintf(value, "%d", *((short *)(ptr->buf)));
		break;

	case Int_Tag:
		if (type)
			strcpy(value, "int");
		else
			sprintf(value, "%d", *((int *)(ptr->buf)));
		break;

	case Long_Tag:
		if (type)
			strcpy(value, "long");
		else
			sprintf(value, "%ld", *((long *)(ptr->buf)));
		break;

	case OctWord_Tag:
		if (type)
			strcpy(value, "octword");
		else
			sprintf(value, "%o", *((short *)(ptr->buf)));
		break;

	case OctLong_Tag:
		if (type)
			strcpy(value, "octlong");
		else
			sprintf(value, "%lo", *((long *)(ptr->buf)));
		break;

	case HexWord_Tag:
		if (type)
			strcpy(value, "hexword");
		else
			sprintf(value, "%x", *((short *)(ptr->buf)));
		break;

	case HexLong_Tag:
		if (type)
			strcpy(value, "hexlong");
		else
			sprintf(value, "%lx", *((long *)(ptr->buf)));
		break;

	case Float_Tag:
		if (type)
			strcpy(value, "float");
		else
			sprintf(value, "%g", *((float *)(ptr->buf)));
		break;

	case Double_Tag:
		if (type)
			strcpy(value, "double");
		else
			sprintf(value, "%g", *((double *)(ptr->buf)));
		break;

	case Char_Tag:
		if (type)
			strcpy(value, "char");
		else
			sprintf(value, "%c", *((char *)(ptr->buf)));
		break;

	case Path_Tag:
		if (ptr->buf_size > 0) {
			char tmpbuf[MAX_PATH];
			if (type)
			{
				strcpy(value, "path");
			} else
			{
				compress_path(tmpbuf, (char *)ptr->buf, sizeof(tmpbuf));
				valuep = strdup(tmpbuf);
			}
		}
		else {
			panicbug(">>> Wrong buf_size in Config_Tag struct: directive %s, buf_size %d !!!", ptr->code, ptr->buf_size);
		}
		break;

	case String_Tag:
		if (ptr->buf_size > 0) {
			if (type)
			{
				strcpy(value, "string");
			} else
			{
				valuep = strdup((char *)ptr->buf);
			}
		}
		else {
			panicbug(">>> Wrong buf_size in Config_Tag struct: directive %s, buf_size %d !!!", ptr->code, ptr->buf_size);
		}
		break;
	case Function_Tag:
	case Error_Tag:
	default:
		return NULL;
	}
	if (valuep == NULL)
		valuep = strdup(value);
	return valuep;
}


/*---------------------------------------------------------------------/
/	reads from an input configuration (INI) file.
/---------------------------------------------------------------------*/
/*>>------[	  input_config()   ]-------------[ 08-02-95 14:02PM ]------/
/ return value:
/	  int					  ; number of records read or -1 on error
/ parameters:
/	  struct Config_Tag configs[]; Configuration structure
/	  char *header			  ; INI header name (i.e. "[TEST]")
/-------------------------------------------------------------------<<*/
int	ConfigOptions::input_config(struct Config_Tag configs[], const char *header)
{
	struct Config_Tag *ptr;
	int	count = 0, lineno = 0;
	FILE * file;
	char	*fptr, *tok, *next;

	file = fopen(config_file, "rt");
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
				next = trim(strtok(NULL, "\n\r")); /* get actual config information */
				for ( ptr = configs; ptr->code; ++ptr )	 /* scan for token */ {
					if ( !strcasecmp( tok , ptr->code ) )  /* got a match? */ {
						if (!set_config_value(ptr, next))
						{
							if (next == NULL)
								panicbug(">>> Missing value in Config file %s on line %d !!!", config_file, lineno);
							else
								bug("Error in Config file %s on line %d", config_file, lineno);
						} else
						{
							++count;
						}
					}
				}
			}
		} while ( fptr != NULL && line[0] != '[');
	fclose(file);

	/*
	 * expand_path() (for converting the relative filenames)
	 * will not be called when the entry is not found in the config file,
	 * so go through the list again and do that now.
	 */
	for ( ptr = configs; ptr->code; ++ptr)
	{
		if (ptr->type == Path_Tag)
		{
			char *path = (char *)ptr->buf;
			if (path[0] != '\0' && path[0] != '/' && path[0] != '\\' && path[1] != ':' )
			{
				char tmpbuf[MAX_PATH];
				safe_strncpy(tmpbuf, path, sizeof(tmpbuf));
				expand_path(path, tmpbuf, ptr->buf_size);
			}
		}
	}
	
	return count;
}


int ConfigOptions::process_config(struct Config_Tag *conf, const char *title, bool verbose)
{
	int status = input_config(conf, title);
	if (status >= 0) {
		if (verbose)
			infoprint("%s configuration: found %d valid directives.", title, status);
	}
	else {
		panicbug("Error while reading/processing the '%s' config file.", config_file);
	}
	return status;
}


bool ConfigOptions::write_token(FILE *outfile, struct Config_Tag *ptr)
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
		fprintf(outfile, "%hd\n", (short)temp);
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

	case Path_Tag:
		{
			char tmpbuf[MAX_PATH];
			compress_path(tmpbuf, (char *)ptr->buf, sizeof(tmpbuf));
			fprintf(outfile, "%s\n", tmpbuf);
		}
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
/	  struct Config_Tag configs[]; Configuration structure
/	  char *header			  ; INI header name (i.e. "[TEST]")
/-------------------------------------------------------------------<<*/
int	ConfigOptions::update_config(struct Config_Tag configs[], const char *header)
{
	static const char *const tempfilenames[] = {
		"/tmp/aratemp.$$$",
		"aratemp.$$$",
		NULL
	};
	const char *tempfilename;
	struct Config_Tag *ptr;
	int	count = 0, lineno = 0;
	FILE * infile, *outfile;
	char	*fptr, *tok;
	int result = 0;
	int i;
	
	for ( ptr = configs; ptr->buf; ++ptr )
		ptr->stat = 0;	/* jeste neulozeno do souboru */

	outfile = NULL;
	i = 0;
	while ((tempfilename = tempfilenames[i++]) != NULL)
	{
		outfile = fopen(tempfilename, "w");
		if (outfile != NULL)
			break;
	}
	if ( outfile == NULL ) {
		panicbug("Error: unable to open %s file.", tempfilenames[0]);
		return ERROR;		/* return error designation. */
	}
	infile = fopen(config_file, "r");
	if ( infile == NULL ) {
/* konfiguracni soubor jeste vubec neexistuje */
		if ( header != NULL ) {
			fprintf(outfile, "%s\n", header);
		}
		for ( ptr = configs; ptr->buf; ++ptr )	  /* scan for token */ {
			if ( write_token(outfile, ptr) )
				++count;
		}

		fclose(outfile);
		result = fcopy(config_file, tempfilename);
		os_remove(tempfilename);

		if (result < 0) {
			panicbug("Error %d in fcopy.", result);
			return result;
		}
		return count;
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
	result = fcopy(config_file, tempfilename);
	os_remove(tempfilename);

	if (result < 0) {
		panicbug("Error %d in fcopy(%s,%s).", result, config_file, 
				tempfilename);
		return result;
	}
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
#endif /* TEST_UPDATE */

	return 0;
}

#endif /* TEST */

/*
vim:ts=4:sw=4:
*/

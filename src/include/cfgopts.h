/*
**	CFGOPTS.H
*/

#ifndef TAG_TYPE_defined
#define TAG_TYPE_defined

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "config.h"
#include "tools.h"

typedef enum {
	Error_Tag,
	Byte_Tag,
	Bool_Tag,
	Word_Tag,
	Int_Tag,
	Long_Tag,
	OctWord_Tag,
	OctLong_Tag,
	HexWord_Tag,
	HexLong_Tag,
	Float_Tag,
	Double_Tag,
	Char_Tag,
	String_Tag,
	Path_Tag,
	Function_Tag
} TAG_TYPE;

struct Config_Tag {
	const char	*code;				/* Option switch		*/
	TAG_TYPE	type;				/* Type of option		*/
	void		 *buf;				/* Storage location		*/
	short		buf_size;			/* Storage size for String_Tag - max. 32k */
	char		stat;				/* internal flag for update_config */
};

#ifndef MAX_PATH
#define MAX_PATH	1024
#endif

class ConfigOptions
{
	private:
		char * trim(char *);
		char * strip_comment(char *);
		long fcopy(const char *, const char *);
		void expand_path(char *, const char *, unsigned short);
		void compress_path(char *, char *, unsigned short);
		bool write_token(FILE *, struct Config_Tag *);

		const char *config_file;
		char config_folder[512];
		char home_folder[512];
		char data_folder[512];
		char line[32768];

	public:
		ConfigOptions(const char *, const char *, const char *);
		int input_config(struct Config_Tag *, const char *);
		int update_config(struct Config_Tag *, const char *);
		int process_config(struct Config_Tag *, const char *, bool verbose);
};

#endif

/*
vim:ts=4:sw=4:
*/

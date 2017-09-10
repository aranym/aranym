/*
**	CFGOPTS.H
*/

#ifndef TAG_TYPE_defined
#define TAG_TYPE_defined

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

struct Config_Section {
	const char *name;
	Config_Tag *tags;
	bool skip_if_empty;
	void (*preset)(void);
	void (*postload)(void);
	void (*presave)(void);
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
		bool write_token(FILE *, struct Config_Tag *);

		const char *config_file;
		char config_folder[512];
		char home_folder[512];
		char data_folder[512];
		char line[32768];

	public:
		ConfigOptions(const char *cfgfile, const char *home, const char *data);
		int input_config(struct Config_Tag *configs, const char *section);
		int update_config(struct Config_Tag *configs, const char *section);
		bool set_config_value(struct Config_Tag *tag, const char *value);
		char *get_config_value(const struct Config_Tag *ptr, bool type);
		int process_config(struct Config_Tag *configs, const char *section, bool verbose);

		void compress_path(char *, char *, unsigned short);
};

#endif

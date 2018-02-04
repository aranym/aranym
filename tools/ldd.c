#define _WIN32_WINNT 0x0501

#undef __STRICT_ANSI__

#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#ifdef __CYGWIN__
#include <sys/cygwin.h>
#include <cygwin/version.h>
#define _wcsicmp wcscasecmp
#define _wcsnicmp wcsncasecmp
#define _wcsdup wcsdup
#endif
#include <unistd.h>
#include <libgen.h>


#ifndef _WIN32
#include <sys/param.h>
#define IsBadReadPtr(p, s) 0
#define DWORD unsigned int
#define WORD unsigned short
#define BYTE unsigned char
#define ULONGLONG unsigned long
#define _wcsdup(s) wcsdup(s)
#define _wcsicmp wcscasecmp
#define UnmapViewOfFile(p) free(p)

#define MAX_PATH PATH_MAX

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		DWORD Characteristics;
		DWORD OriginalFirstThunk;
	};
	DWORD TimeDateStamp;
	DWORD ForwarderChain;
	DWORD Name;
	DWORD FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR,*PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_DOS_HEADER {
	WORD e_magic;
	WORD e_cblp;
	WORD e_cp;
	WORD e_crlc;
	WORD e_cparhdr;
	WORD e_minalloc;
	WORD e_maxalloc;
	WORD e_ss;
	WORD e_sp;
	WORD e_csum;
	WORD e_ip;
	WORD e_cs;
	WORD e_lfarlc;
	WORD e_ovno;
	WORD e_res[4];
	WORD e_oemid;
	WORD e_oeminfo;
	WORD e_res2[10];
	DWORD e_lfanew;
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_SIZEOF_SHORT_NAME 8
#define IMAGE_NT_SIGNATURE 0x00004550
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b

typedef struct _IMAGE_DATA_DIRECTORY {
	DWORD VirtualAddress;
	DWORD Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_SECTION_HEADER {
	BYTE Name[IMAGE_SIZEOF_SHORT_NAME];
	union {
		DWORD PhysicalAddress;
		DWORD VirtualSize;
	};
	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD PointerToRawData;
	DWORD PointerToRelocations;
	DWORD PointerToLinenumbers;
	WORD NumberOfRelocations;
	WORD NumberOfLinenumbers;
	DWORD Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_FILE_HEADER {
	WORD Machine;
	WORD NumberOfSections;
	DWORD TimeDateStamp;
	DWORD PointerToSymbolTable;
	DWORD NumberOfSymbols;
	WORD SizeOfOptionalHeader;
	WORD Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_OPTIONAL_HEADER {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	DWORD BaseOfData;
	DWORD ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	DWORD SizeOfStackReserve;
	DWORD SizeOfStackCommit;
	DWORD SizeOfHeapReserve;
	DWORD SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32,*PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
	WORD Magic;
	BYTE MajorLinkerVersion;
	BYTE MinorLinkerVersion;
	DWORD SizeOfCode;
	DWORD SizeOfInitializedData;
	DWORD SizeOfUninitializedData;
	DWORD AddressOfEntryPoint;
	DWORD BaseOfCode;
	ULONGLONG ImageBase;
	DWORD SectionAlignment;
	DWORD FileAlignment;
	WORD MajorOperatingSystemVersion;
	WORD MinorOperatingSystemVersion;
	WORD MajorImageVersion;
	WORD MinorImageVersion;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	DWORD Win32VersionValue;
	DWORD SizeOfImage;
	DWORD SizeOfHeaders;
	DWORD CheckSum;
	WORD Subsystem;
	WORD DllCharacteristics;
	ULONGLONG SizeOfStackReserve;
	ULONGLONG SizeOfStackCommit;
	ULONGLONG SizeOfHeapReserve;
	ULONGLONG SizeOfHeapCommit;
	DWORD LoaderFlags;
	DWORD NumberOfRvaAndSizes;
	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64,*PIMAGE_OPTIONAL_HEADER64;

#define IMAGE_DIRECTORY_ENTRY_IMPORT	1

#endif


static char const program_name[] = "ldd";


#ifdef _WIN32
#include <windows.h>
#include <imagehlp.h>
#include <psapi.h>
#endif
#include <wchar.h>

#ifndef STATUS_DLL_NOT_FOUND
#  define STATUS_DLL_NOT_FOUND (0xC0000135L)
#endif


#ifndef FALSE
# define FALSE 0
# define TRUE  1
#endif


#define VERSION "2.0"


static int shortname;
static int pathname;


static int error(const char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "ldd: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\nTry `ldd --help' for more information.\n");
	exit(1);
}


static void print_usage(void)
{
	printf("Usage: %s [OPTION]... FILE...\n\
\n\
Print shared library dependencies\n\
\n\
  -h, --help              print this help and exit\n\
  -V, --version           print version information and exit\n\
  -r, --function-relocs   process data and function relocations\n\
                          (currently unimplemented)\n\
  -u, --unused            print unused direct dependencies\n\
                          (currently unimplemented)\n\
  -v, --verbose           print all information\n\
                          (currently unimplemented)",
	program_name);
}


static void print_version(void)
{
	printf(
#if defined(__CYGWIN__)
	   "ldd (Cygwin) %d.%d.%d\n"
#elif defined(__MINGW32__)
#ifdef _WIN64
	   "ldd (MingW) (64bit) " VERSION "\n"
#else
	   "ldd (MingW) " VERSION "\n"
#endif
#elif defined(__WINE__)
#ifdef _WIN64
	   "ldd (wine) (64bit) " VERSION "\n"
#else
	   "ldd (wine) " VERSION "\n"
#endif
#elif defined(_MSC_VER)
#ifdef _WIN64
	   "ldd (MSVC) (64bit) " VERSION "\n"
#else
	   "ldd (MSVC) " VERSION "\n"
#endif
#else
	   "ldd " VERSION "\n"
#endif
	   "Print shared library dependencies\n"
	   "Copyright (C) 2009 - %s Chris Faylor\n"
	   "This is free software; see the source for copying conditions.  There is NO\n"
	   "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
#if defined(__CYGWIN__)
	  CYGWIN_VERSION_DLL_MAJOR / 1000,
	  CYGWIN_VERSION_DLL_MAJOR % 1000,
	  CYGWIN_VERSION_DLL_MINOR,
#endif
	   strrchr (__DATE__, ' ') + 1);
}


#define print_errno_error_and_return(__fn) \
  do {\
    fprintf (stderr, "ldd: %s: %s\n", (__fn), strerror (errno));\
    return 1;\
  } while (0)

static struct filelist
{
	struct filelist *next;
	wchar_t *name;
} *head;


#ifdef _WIN32
static char *xstrdup(const char *s)
{
	return strcpy(malloc(strlen(s) + 1), s);
}
#endif


static int saw_file(const wchar_t *name)
{
	struct filelist *p;

	for (p = head; p; p = p->next)
		if (_wcsicmp(name, p->name) == 0)
			return TRUE;

	p = (struct filelist *) malloc(sizeof(struct filelist));

	p->next = head;
	p->name = _wcsdup(name);
	head = p;
	return FALSE;
}


#ifdef _WIN32
#define SLOP strlen (" (?)")
static char *tocyg(wchar_t *win_fn)
{
	char *fn;
#ifdef __CYGWIN__
	ssize_t cwlen = cygwin_conv_path(CCP_WIN_W_TO_POSIX, win_fn, NULL, 0);

	if (cwlen > 0)
	{
		char *fn_cyg = (char *) malloc(cwlen + SLOP + 1);

		if (cygwin_conv_path(CCP_WIN_W_TO_POSIX, win_fn, fn_cyg, cwlen) == 0)
			fn = fn_cyg;
		else
		{
			int len;
			
			free(fn_cyg);
			len = wcstombs(NULL, win_fn, 0);

			fn = (char *) malloc(len + SLOP + 1);
			wcstombs(fn, win_fn, len + SLOP + 1);
		}
	} else
#endif
	{
		int len = wcstombs(NULL, win_fn, 0) + 1;

		if ((fn = (char *) malloc(len)) != NULL)
			wcstombs(fn, win_fn, len);
	}
	return fn;
}
#endif


static int process_file(const wchar_t *filename, const char *internal_fn, const char *print_fn);


#ifdef _WIN32
WINBASEAPI WINBOOL WINAPI Wow64DisableWow64FsRedirection(PVOID *oldValue);
WINBASEAPI WINBOOL WINAPI Wow64RevertWow64FsRedirection(PVOID OldValue);
WINBASEAPI BOOLEAN WINAPI Wow64EnableWow64FsRedirection(BOOLEAN Wow64FsEnableRedirection);

/*
DWORD WINAPI GetFinalPathNameByHandleW(
  HANDLE hFile,
  LPWSTR lpszFilePath,
  DWORD cchFilePath,
  DWORD dwFlags
);

UINT WINAPI GetSystemWow64DirectoryW(
  LPWSTR lpBuffer,
  UINT uSize
);
*/

typedef WINBOOL (WINAPI *PFNWOW64DISABLEWOW64FSREDIRECTION)(PVOID *oldValue);
typedef WINBOOL (WINAPI *PFNWOW64REVERTWOW64FSREDIRECTION)(PVOID OldValue);
typedef BOOL (WINAPI *PFNSETDLLDIRECTORYW)(LPWSTR name);
typedef DWORD (WINAPI *PFNGETFINALPATHNAMEBYHANDLEW)(HANDLE hFile, LPWSTR lpszFilePath, DWORD cchFilePath, DWORD dwFlags);
typedef UINT (WINAPI *PFNGETSYSTEMWOW64DIRECTORYW)(LPWSTR buffer, UINT size);

static PFNWOW64DISABLEWOW64FSREDIRECTION p_Wow64DisableWow64FsRedirection;
static PFNWOW64REVERTWOW64FSREDIRECTION p_Wow64RevertWow64FsRedirection;
static PFNSETDLLDIRECTORYW p_SetDllDirectoryW;
static PFNGETFINALPATHNAMEBYHANDLEW p_GetFinalPathNameByHandleW;
static PFNGETSYSTEMWOW64DIRECTORYW p_GetSystemWow64DirectoryW;

#endif



/* dump of import directory
   section begins at pointer 'section base'
   section RVA is 'section_rva'
   import directory begins at pointer 'imp' */
static int dump_import_directory(wchar_t *search_path, const void *const section_base,
					   const DWORD section_rva,
					   const IMAGE_IMPORT_DESCRIPTOR *imp, int is_64bit)
{
	int ret = 0;
	
	/* get memory address given the RVA */
#define adr(rva) ((const void*) ((char*) section_base+((DWORD) (rva))-section_rva))
	
	/* continue until address inaccessible or there's no DLL name */
	for (; !IsBadReadPtr(imp, sizeof(*imp)) && imp->Name; imp++)
	{
		char *fn = (char *) adr(imp->Name);
		wchar_t *fnw;
		/* output DLL's name */
		int len;
		
		len = mbstowcs(NULL, fn, 0);

		if (len <= 0)
			continue;
		fnw = (wchar_t *)malloc((len + 1) * sizeof(*fnw));

		mbstowcs(fnw, fn, len + 1);

		if (!saw_file(fnw))
		{
#ifdef _WIN32
			PVOID redirect;
			wchar_t *dummy;
			char *print_fn;
			wchar_t full_path[MAX_PATH];

			redirect = NULL;
			
			if (is_64bit && p_Wow64DisableWow64FsRedirection)
				p_Wow64DisableWow64FsRedirection(&redirect);
			
#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0x00000020
#endif
			
			if (!SearchPathW(search_path, fnw, NULL, MAX_PATH, full_path, &dummy))
			{
				print_fn = xstrdup("not found");
				if (shortname || pathname)
					printf("notfound:%s\n", fn);
				else
					printf("\t%s => %s\n", fn, print_fn);
				free(print_fn);
				ret |= 1;
			} else
			{
				if (strncmp(fn, "API-MS-", 7) != 0 && strncmp(fn, "api-ms-", 7) != 0)
				{
					print_fn = tocyg(full_path);
				} else
				{
					print_fn = NULL;
				}
				ret |= process_file(full_path, fn, print_fn);
				free(print_fn);
			}
			if (is_64bit && p_Wow64RevertWow64FsRedirection)
				p_Wow64RevertWow64FsRedirection(redirect);
#else
			printf("\t%s\n", fn);
#endif
		}
		free(fnw);
	}
#undef adr

	return ret;
}


/* load a file in RAM (memory-mapped)
   return pointer to loaded file
   0 if no success  */
static void *map_file(const wchar_t *filename)
{
	void *basepointer;
#ifdef _WIN32
	HANDLE hFile, hMapping;

	if ((hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
	 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0)) == INVALID_HANDLE_VALUE)
	{
		__extension__ fprintf(stderr, "couldn't open %ls\n", filename);
		return 0;
	}
	if (!(hMapping = CreateFileMapping(hFile, 0, PAGE_READONLY | SEC_COMMIT, 0, 0, 0)))
	{
		fprintf(stderr, "CreateFileMapping failed with windows error %u\n", (unsigned int) GetLastError());
		CloseHandle(hFile);
		return 0;
	}
	if (!(basepointer = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0)))
	{
		fprintf(stderr, "MapViewOfFile failed with windows error %u\n", (unsigned int) GetLastError());
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return 0;
	}
	CloseHandle(hMapping);
	CloseHandle(hFile);

#else
	FILE *fp;
	char fname[PATH_MAX];
	size_t size;
	
	wcstombs(fname, filename, sizeof(fname));
	fp = fopen(fname, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "can't open %s: %s\n", fname, strerror(errno));
		return NULL;
	}
	fseek(fp, 0l, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0l, SEEK_SET);
	basepointer = malloc(size);
	if (fread(basepointer, 1, size, fp) != size)
	{
		fprintf(stderr, "fread failed on %s: %s\n", fname, strerror(errno));
		fclose(fp);
		free(basepointer);
		return NULL;
	}
	fclose(fp);
#endif
	return basepointer;
}


/* this will return a pointer immediatly behind the DOS-header
   0 if error */
static void *skip_dos_stub(const wchar_t *filename, const IMAGE_DOS_HEADER *dos_ptr)
{
	/* look there's enough space for a DOS-header */
	if (IsBadReadPtr(dos_ptr, sizeof(*dos_ptr)))
	{
		__extension__ fprintf(stderr, "%ls: not enough space for DOS-header\n", filename);
		return 0;
	}
	/* validate MZ */
	if (dos_ptr->e_magic != IMAGE_DOS_SIGNATURE)
	{
		__extension__ fprintf(stderr, "%ls: not a DOS-stub\n", filename);
		return 0;
	}
	if (dos_ptr->e_lfarlc < 0x40 || dos_ptr->e_lfanew == 0)
	{
		__extension__ fprintf(stderr, "%ls: invalid DOS-stub\n", filename);
		return 0;
	}
	/* ok, then, go get it */
	return (char *) dos_ptr + dos_ptr->e_lfanew;
}


/* find the directory's section index given the RVA
   Returns -1 if impossible */
static int get_directory_index(const unsigned dir_rva,
					 const unsigned dir_length,
					 const int number_of_sections,
					 const IMAGE_SECTION_HEADER * sections)
{
	int sect;

	for (sect = 0; sect < number_of_sections; sect++)
	{
		/* compare directory RVA to section RVA */
		if (sections[sect].VirtualAddress <= dir_rva
			&& dir_rva < sections[sect].VirtualAddress + sections[sect].SizeOfRawData)
			return sect;
	}

	return -1;
}


/* ensure byte-alignment for struct tag_header */
#ifdef _WIN32
#include <pshpack1.h>
#else
#pragma pack(push,1)
#endif

struct tag_header
{
	DWORD signature;
	IMAGE_FILE_HEADER file_head;
	union {
		IMAGE_OPTIONAL_HEADER32 head32;
		IMAGE_OPTIONAL_HEADER64 head64;
	} opt;
};
/* revert to regular alignment */
#ifdef _WIN32
#include <poppack.h>
#else
#pragma pack(pop)
#endif


static char *strccpy(char *s1, const char **s2, char c)
{
	while (**s2 && **s2 != c)
		*s1++ = *((*s2)++);
	*s1 = 0;
	return s1;
}


static wchar_t *combine_path(wchar_t **list)
{
	size_t len = 0;
	wchar_t **p;
	wchar_t *path, *dst;
	
	p = list;
	while (*p != NULL)
	{
		len += wcslen(*p) + 1;
		p++;
	}
	path = malloc(len * sizeof(*path));
	dst = path;
	p = list;
	while (*p != NULL)
	{
		if (dst > path)
			*dst++ = ';';
		wcscpy(dst, *p);
		dst += wcslen(dst);
		p++;
	}
	return path;
}


static wchar_t **split_path(wchar_t *dst, const char *srcpath, size_t size, char delim, int is_64bit)
{
	char *srcbuf;
	wchar_t *d = dst - 1;
	size_t count = 0;
	int saw_current = 0;
#ifdef _WIN32
	int saw_windows = 0;
	int saw_system = 0;
	wchar_t windows_dir[MAX_PATH];
	wchar_t system_dir[MAX_PATH];
	wchar_t syswow64_dir[MAX_PATH];
#endif
	wchar_t **path_list, **listp;
	const char *src = srcpath;
	
#ifdef _WIN32
	GetWindowsDirectoryW(windows_dir, MAX_PATH);
	GetSystemDirectoryW(system_dir, MAX_PATH);
	if (!p_GetSystemWow64DirectoryW || !p_GetSystemWow64DirectoryW(syswow64_dir, MAX_PATH))
		syswow64_dir[0] = 0;
#endif
	
	srcbuf = malloc(size);
	do
	{
		char *srcpath = srcbuf;
		char *s = strccpy(srcpath, &src, delim);
		size_t len = s - srcpath;
		if (len >= MAX_PATH)
		{
			errno = ENAMETOOLONG;
			return NULL;
		}
		/* Paths in Win32 path lists in the environment (%Path%), are often
		   enclosed in quotes (usually paths with spaces).	Trailing backslashes
		   are common, too.Remove them.
		   */
		if (delim == ';' && len)
		{
			if (*srcpath == '"')
			{
				++srcpath;
				*--s = '\0';
				len -= 2;
			}
			while (len && s[-1] == '\\')
			{
				*--s = '\0';
				--len;
			}
		}
		if (strcmp(srcbuf, ".") == 0)
			len = 0;
		if (len)
		{
			++d;
#ifdef __CYGWIN__
			if (cygwin_conv_path(CCP_POSIX_TO_WIN_W, srcpath, d, size - (d - dst)))
				return NULL;
#else
			mbstowcs(d, srcpath, size - (d - dst));
#endif
		} else
		{
			saw_current = 1;
			++d;
#ifdef __CYGWIN__
			if (cygwin_conv_path(CCP_POSIX_TO_WIN_W, ".", d, size - (d - dst)))
				return NULL;
#else
#ifdef _WIN32
			GetCurrentDirectoryW(size - (d - dst), d);
#else
			{
				char dir[PATH_MAX];
				getcwd(dir, sizeof(dir));
				mbstowcs(d, dir, size - (d - dst));
			}
#endif
#endif
		}
		count++;
#ifdef _WIN32
		if (_wcsicmp(d, windows_dir) == 0)
			saw_windows = 1;
		if (_wcsicmp(d, system_dir) == 0)
			saw_system = 1;
#endif
		d = wcschr(d, '\0');
		*d = delim;
	} while (*src++);
	if (d < dst)
		d++;
	*d = '\0';

	path_list = listp = malloc((count + 5) * sizeof(char *));
	
	d = dst;
	
	*listp++ = d;
	wcscpy(d, L"{appdir}");
	d += wcslen(d);
	*d++ = '\0';
	
#ifdef _WIN32
	*listp++ = d;
	if (!is_64bit && syswow64_dir[0])
		wcscpy(d, syswow64_dir);
	else
		wcscpy(d, system_dir);
	d += wcslen(d);
	*d++ = '\0';
	
	*listp++ = d;
	wcscpy(d, windows_dir);
	d += wcslen(d);
	*d = '\0';
#endif
	
	src = srcpath;
	do
	{
		char *srcpath = srcbuf;
		char *s = strccpy(srcpath, &src, delim);
		size_t len = s - srcpath;
		if (len >= MAX_PATH)
		{
			errno = ENAMETOOLONG;
			return NULL;
		}
		/* Paths in Win32 path lists in the environment (%Path%), are often
		   enclosed in quotes (usually paths with spaces).	Trailing backslashes
		   are common, too.Remove them.
		   */
		if (delim == ';' && len)
		{
			if (*srcpath == '"')
			{
				++srcpath;
				*--s = '\0';
				len -= 2;
			}
			while (len && s[-1] == '\\')
			{
				*--s = '\0';
				--len;
			}
		}
		if (strcmp(srcbuf, ".") == 0)
			len = 0;
		if (len)
		{
			++d;
#ifdef __CYGWIN__
			if (cygwin_conv_path(CCP_POSIX_TO_WIN_W, srcpath, d, size - (d - dst)))
				return NULL;
#else
			mbstowcs(d, srcpath, size - (d - dst));
#endif
		} else
		{
			saw_current = 1;
			++d;
#ifdef __CYGWIN__
			if (cygwin_conv_path(CCP_POSIX_TO_WIN_W, ".", d, size - (d - dst)))
				return NULL;
#else
#ifdef _WIN32
			GetCurrentDirectoryW(size - (d - dst), d);
#else
			{
				char dir[PATH_MAX];
				getcwd(dir, sizeof(dir));
				mbstowcs(d, dir, size - (d - dst));
			}
#endif
#endif
		}
#ifdef _WIN32
		if (_wcsicmp(d, system_dir) == 0 && !is_64bit && syswow64_dir[0])
			wcscpy(d, syswow64_dir);
#endif
		*listp++ = d;
		d = wcschr(d, '\0');
		*d = '\0';
	} while (*src++);
	*d = '\0';
	*listp = NULL;
	free(srcbuf);
	(void) saw_current;
#ifdef _WIN32
	(void) saw_system;
	(void) saw_windows;
#endif
	return path_list;
}

	

/* dump imports of a single file
   Returns 0 if successful, !=0 else */
static int process_file(const wchar_t *filename, const char *internal_fn, const char *print_fn)
{
	void *basepointer;					/* Points to loaded PE file */
	int number_of_sections;
	DWORD import_rva;					/* RVA of import directory */
	DWORD import_length;				/* length of import directory */
	const IMAGE_SECTION_HEADER *section_headers;		/* an array of unknown length */
	int import_index;					/* index of section with import directory */
	const void *section_address;
	const struct tag_header *header;
	int is_64bit = 0;
	wchar_t *dir;
	wchar_t *p1, *p2;
	wchar_t *path_w;
	wchar_t **search_path_split;
	wchar_t *search_path;
	ssize_t len;
	char *path;
	ULONGLONG ImageBase;
	
	/* first, load file */
	basepointer = map_file(filename);
	if (!basepointer)
	{
		__extension__ fprintf(stderr, "%ls: cannot load file\n", filename);
		return 1;
	}
	/* get header pointer; validate a little bit */
	header = (struct tag_header *) skip_dos_stub(filename, (IMAGE_DOS_HEADER *) basepointer);
	if (!header)
	{
		__extension__ fprintf(stderr, "%ls: cannot skip DOS stub\n", filename);
		UnmapViewOfFile(basepointer);
		return 2;
	}
	/* look there's enough space for PE headers */
	if (IsBadReadPtr(header, sizeof(*header)))
	{
		__extension__ fprintf(stderr, "%ls: not enough space for PE headers\n", filename);
		UnmapViewOfFile(basepointer);
		return 3;
	}
	/* validate PE signature */
	if (header->signature != IMAGE_NT_SIGNATURE)
	{
		__extension__ fprintf(stderr, "%ls: not a PE file\n", filename);
		UnmapViewOfFile(basepointer);
		return 4;
	}
	if (header->opt.head32.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
		header->file_head.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32))
	{
		/* get number of sections */
		number_of_sections = header->file_head.NumberOfSections;
	
		/* check there are sections... */
		if (number_of_sections < 1)
		{
			UnmapViewOfFile(basepointer);
			return 5;
		}
		/* get RVA and length of import directory */
		import_rva = header->opt.head32.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		import_length = header->opt.head32.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		ImageBase = header->opt.head32.ImageBase;
	} else if (header->opt.head64.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
		header->file_head.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64))
	{
		/* get number of sections */
		number_of_sections = header->file_head.NumberOfSections;
	
		/* check there are sections... */
		if (number_of_sections < 1)
		{
			UnmapViewOfFile(basepointer);
			return 5;
		}
		/* get RVA and length of import directory */
		import_rva = header->opt.head64.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		import_length = header->opt.head64.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		ImageBase = header->opt.head64.ImageBase;
		is_64bit = 1;
	} else
	{
		__extension__ fprintf(stderr, "%ls: unknown PE file\n", filename);
		UnmapViewOfFile(basepointer);
		return 4;
	}
	if (print_fn)
	{
		if (pathname)
		{
			printf("%s\n", print_fn);
		} else if (shortname)
		{
			printf("%s\n", internal_fn);
		} else
		{
#ifdef __MINGW32__
			printf("\t%s => %s (0x%I64x)\n", internal_fn, print_fn, ImageBase);
#else
			printf("\t%s => %s (0x%llx)\n", internal_fn, print_fn, (unsigned long long) ImageBase);
#endif
		}
	}
	
	section_headers = (const IMAGE_SECTION_HEADER *)((char *)header + offsetof(struct tag_header, opt) + header->file_head.SizeOfOptionalHeader);
	/* validate there's enough space for section headers */
	if (IsBadReadPtr(section_headers, number_of_sections * sizeof(IMAGE_SECTION_HEADER)))
	{
		__extension__ fprintf(stderr, "%ls: not enough space for section headers\n", filename);
		UnmapViewOfFile(basepointer);
		return 6;
	}

	/* check there's stuff to care about */
	if (!import_rva || !import_length)
	{
		UnmapViewOfFile(basepointer);
		return 0;						/* success! */
	}
	/* get import directory pointer */
	import_index = get_directory_index(import_rva, import_length, number_of_sections, section_headers);

	/* check directory was found */
	if (import_index < 0)
	{
		__extension__ fprintf(stderr, "%ls: couldn't find import directory in sections\n", filename);
		UnmapViewOfFile(basepointer);
		return 7;
	}
	/* The pointer to the start of the import directory's section */
	section_address = (char *) basepointer + section_headers[import_index].PointerToRawData;

	dir = _wcsdup(filename);
	p1 = wcsrchr(dir, '\\');
	p2 = wcsrchr(dir, '/');
	if (p1 == NULL || p2 > p1)
		p1 = p2;
	if (p1 == NULL)
	{
		free(dir);
		dir = _wcsdup(L".");
	} else
	{
		p1[0] = 0;
	}

	/*
	 * setup a path such that SearchPath uses the same
	 * search order as LoadLibrary.
	 * FIXME: this assumes current behavior of Windows
	 * and may break if the behavior of LoadLibrary() changes
	 * in future version of windows.
	 */
	path = getenv("PATH");
#if defined(__CYGWIN__) || !defined(_WIN32)
	len = strlen(path);
	if (len <= 0)
		print_errno_error_and_return("PATH");
	len = len * 2 + 1 + 5 * MAX_PATH;
	path_w = malloc(len * sizeof(*path_w));
	
	search_path_split = split_path(path_w, path, len, ':', is_64bit);
#else
	len = mbstowcs(NULL, path, 0) + 1;
	if (len <= 0)
		print_errno_error_and_return("PATH");
	len = len + 1 + 5 * MAX_PATH;
	path_w = malloc(len * sizeof(*path_w));
	search_path_split = split_path(path_w, path, len, ';', is_64bit);
#endif
	
	if (search_path_split == NULL)
		print_errno_error_and_return("PATH");

	search_path_split[0] = dir;
	search_path = combine_path(search_path_split);
	free(path_w);
	
	if (dump_import_directory(search_path, section_address,
						  section_headers[import_index].VirtualAddress,
	/* the last parameter is the pointer to the import directory:
	   section address + (import RVA - section RVA)
	   The difference is the offset of the import directory in the section */
			(const IMAGE_IMPORT_DESCRIPTOR *) ((char *) section_address + import_rva - section_headers[import_index].VirtualAddress),
			is_64bit))
	{
		free(dir);
		UnmapViewOfFile(basepointer);
		return 8;
	}
	free(dir);
	UnmapViewOfFile(basepointer);
	return 0;
}


static int report(const char *in_fn)
{
	ssize_t len;
	wchar_t *fn_win;
	struct filelist *p;
	int ret;
	
#ifdef __CYGWIN__
	len = cygwin_conv_path(CCP_POSIX_TO_WIN_W, in_fn, NULL, 0);

	if (len <= 0)
		print_errno_error_and_return(in_fn);
	fn_win = malloc((len + 1) * sizeof(*fn_win));
	
	if (cygwin_conv_path(CCP_POSIX_TO_WIN_W, in_fn, fn_win, len))
		print_errno_error_and_return(in_fn);
#else
	len = mbstowcs(NULL, in_fn, 0) + 1;
	if (len <= 0)
		print_errno_error_and_return(in_fn);
	fn_win = malloc((len + 1) * sizeof(*fn_win));
	mbstowcs(fn_win, in_fn, len);
#endif

	head = NULL;
	ret = process_file(fn_win, NULL, NULL);
	while (head)
	{
		free(head->name);
		p = head->next;
		free(head);
		head = p;
	}
	free(fn_win);
	return ret;
}


static struct option const longopts[] =
{
	{ "help", no_argument, NULL, 'h' },
	{ "verbose", no_argument, NULL, 'v' },
	{ "version", no_argument, NULL, 'V' },
	{ "data-relocs", no_argument, NULL, 'd' },
	{ "function-relocs", no_argument, NULL, 'r' },
	{ "unused", no_argument, NULL, 'u' },
	{ "short", no_argument, NULL, 's' },
	{ "path", no_argument, NULL, 'p' },
	{ "multiple", no_argument, NULL, 'm' },
	{ 0, no_argument, NULL, 0 }
};


int main(int argc, char **argv)
{
	int optch;
	int ret = 0;
	int multiple;
	char *fn;
	
	/* Use locale from environment.  If not set or set to "C", use UTF-8. */
	setlocale(LC_CTYPE, "");
	if (!strcmp(setlocale(LC_CTYPE, NULL), "C"))
		setlocale(LC_CTYPE, "en_US.UTF-8");

	multiple = 0;
	shortname = 0;
	pathname = 0;
	
	while ((optch = getopt_long(argc, argv, "mdhprsuvV", longopts, NULL)) != -1)
	{
		switch (optch)
		{
		case 'd':
		case 'r':
		case 'u':
			error("option not implemented `-%c'", optch);
			exit(1);
			break;
		case 'v':
			break;
		case 'm':
			multiple = 1;
			break;
		case 's':
			shortname = 1;
			break;
		case 'p':
			pathname = 1;
			break;
		case 'h':
			print_usage();
			return 0;
		case 'V':
			print_version();
			return 0;
		default:
			fprintf (stderr, "Try `%s --help' for more information.\n", program_name);
			return 1;
		}
	}
	
	if ((argc - optind) <= 0)
		error("missing file arguments");

	if ((argc - optind) > 1)
		multiple = 1;

#ifdef _WIN32
	p_Wow64DisableWow64FsRedirection = (PFNWOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64DisableWow64FsRedirection");
	p_Wow64RevertWow64FsRedirection = (PFNWOW64REVERTWOW64FSREDIRECTION)GetProcAddress(GetModuleHandleA("kernel32.dll"), "Wow64RevertWow64FsRedirection");
	p_SetDllDirectoryW = (PFNSETDLLDIRECTORYW)GetProcAddress(GetModuleHandleA("kernel32.dll"), "SetDllDirectoryW");
	p_GetFinalPathNameByHandleW = (PFNGETFINALPATHNAMEBYHANDLEW)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetFinalPathNameByHandleW");
	p_GetSystemWow64DirectoryW = (PFNGETSYSTEMWOW64DIRECTORYW)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetSystemWow64DirectoryW");
#endif

	while (optind < argc)
	{
		fn = argv[optind++];
		if (multiple)
			printf("%s:\n", fn);

		if (report(fn) != 0)
		{
			ret = 1;
		}
	}
	exit(ret);
	return ret;
}

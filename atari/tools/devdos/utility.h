#ifndef UTILITY_H
#define UTILITY_H
/*
 * fVDI utility function declarations
 *
 * Copyright 2003, Johan Klockars 
 * This software is licensed under the GNU General Public License.
 * Please, see LICENSE.TXT for further information.
 */

long init_utility(void);

/*
 * Memory access
 */
long get_protected_l(long addr);
void set_protected_l(long addr, long value);
long get_l(long addr);
void set_l(long addr, long value);

/*
 * Cookie and XBRA access
 */
long get_cookie(const unsigned char *cname, long super);
long set_cookie(const unsigned char *name, long value);
long remove_xbra(long vector, const unsigned char *name);
void check_cookies(void);

/*
 * Memory pool allocation (from set of same sized blocks)
 */
long initialize_pool(long size, long n);
char *allocate_block(long size);
void free_block(void *addr);


/*
 * Memory/string operations
 */
void *memchr(const void *s, long c, size_t n);
char *memrchr(const void *s, long c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, long n);
void *memset(void *s, int c, size_t n);

char *strdup(const char *s);
char *strncpy(char *dest, const char *src, size_t n);
long strncmp(const char *s1, const char *s2, size_t n);
char *strcat(char *dest, const char *src);
char *strchr(const char *s, long c);
char *strrchr(const char *s, long c);
char *strupr(char *s);


void qsort(void *base, long nmemb, long size,
           long (*compar)(const void *, const void *));

/*
 * Character numerics
 */
long numeric(long ch);
long check_base(char ch, long base);
long atol(const char *text);
void ltoa(char *buf, long n, unsigned long base);
void ultoa(char *buf, unsigned long un, unsigned long base);
long str2long(const unsigned char *text);

/*
 * General memory allocation
 */
void *fmalloc(long size, long type);
void *malloc(long size);
void *realloc(void *addr, long new_size);
long free(void *addr);
long free_all(void);
void allocate(long amount);

/*
 * ctype.h
 */
int tolower (int c);
int toupper (int c);

/*
 * Text output
 */
long puts(const char *text);
void error(const char *text1, const char *text2);
#define puts_nl(text)	{ puts(text); puts("\x0a\x0d"); }
long sprintf(char *str, const char *format, ...);
long vsprintf(char *str, const char *format, va_list args);

/*
 * Maths
 */
short Isin(short angle);
short Icos(short angle);
#define ABS(x)    (((x) >= 0) ? (x) : -(x))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#define MAX(x,y)  ((x) > (y) ? (x) : (y))

#endif


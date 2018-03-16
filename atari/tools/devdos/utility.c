/*
 * fVDI utility functions
 *
 * Copyright 1997-2003, Johan Klockars 
 * This software is licensed under the GNU General Public License.
 * Please, see LICENSE.TXT for further information.
 */

#include <stdarg.h>
#include <stddef.h>	/* for size_t */
#include <osbind.h>

#include "utility.h"

/* just forwards */
static inline void copymem(const void *s, void *d, long n);
static inline void setmem(void *d, long v, long n);
static inline void copy(const char *src, char *dest);
static inline void cat(const char *src, char *dest);


short debug = 0;
char silentx[10] = "\0\0\0\0";

/*
 * Global variables
 */

typedef struct _Circle {
   struct _Circle *prev;
   struct _Circle *next;
   long size;
} Circle;

short Falcon = 0;
short TT = 0;
long cpu = 0;
long fpu = 0;
long frb = 0;
long video = 0;

long nvdi = 0;
long eddi = 0;
long mint = 0;
long magic = 0;

short mxalloc = 0;


/* BEG: from ctype.c */
#define _ctype _mint_ctype
extern unsigned char _mint_ctype[256];

# define _CTc		0x01	/* control character */
# define _CTd		0x02	/* numeric digit */
# define _CTu		0x04	/* upper case */
# define _CTl		0x08	/* lower case */
# define _CTs		0x10	/* whitespace */
# define _CTp		0x20	/* punctuation */
# define _CTx		0x40	/* hexadecimal */

# define isalnum(c)	(  _ctype[(unsigned char)(c)] & (_CTu|_CTl|_CTd))
# define isalpha(c)	(  _ctype[(unsigned char)(c)] & (_CTu|_CTl))
# define isascii(c)	(!((c) & ~0x7f))
# define iscntrl(c)	(  _ctype[(unsigned char)(c)] &  _CTc)
# define isdigit(c)	(  _ctype[(unsigned char)(c)] &  _CTd)
# define isgraph(c)	(!(_ctype[(unsigned char)(c)] & (_CTc|_CTs)) && (_ctype[(unsigned char)(c)]))
# define islower(c)	(  _ctype[(unsigned char)(c)] &  _CTl)
# define isprint(c)	(!(_ctype[(unsigned char)(c)] &  _CTc)       && (_ctype[(unsigned char)(c)]))
# define ispunct(c)	(  _ctype[(unsigned char)(c)] &  _CTp)
# define isspace(c)	(  _ctype[(unsigned char)(c)] &  _CTs)
# define isupper(c)	(  _ctype[(unsigned char)(c)] &  _CTu)
# define isxdigit(c)	(  _ctype[(unsigned char)(c)] &  _CTx)
# define iswhite(c)	(isspace (c))

# define isodigit(c)	((c) >= '0' && (c) <= '7')
# define iscymf(c)	(isalpha(c) || ((c) == '_'))
# define iscym(c)	(isalnum(c) || ((c) == '_'))

# define _toupper(c)	((c) ^ 0x20)
# define _tolower(c)	((c) ^ 0x20)
# define _toascii(c)	((c) & 0x7f)

int tolower (int c)
{
	return (isupper (c) ? _tolower (c) : c);
}

int toupper (int c)
{
	return (islower (c) ? _toupper (c) : c);
}
/* END: from ctype.c */


char * strupr (char *s)
{
	char c;
	char *old = s;
	
	while ((c = *s) != 0)
	{
		if (islower (c))
			*s = _toupper(c);
		
		s++;
	}
	
	return old;
}


/*
 * Turn string (max four characters) into a long
 */
long str2long(const unsigned char *text)
{
   long v;

   v = 0;
   while (*text) {
      v <<= 8;
      v += *text++;
   }

   return v;
}


long get_l(long addr)
{
   return *(long *)addr;
}


/*
 * Get a long value after switching to supervisor mode
 */
long get_protected_l(long addr)
{
   long oldstack, v;

   oldstack = (long)Super(0L);
   v = *(long *)addr;
   Super((void *)oldstack);

   return v;
}


void set_l(long addr, long value)
{
   *(long *)addr = value;
}

static inline
void copymem(const void *s, void *d, long n)
{
   const char *src  = (const char *)s;
   char *dest = (char *)d;

   for(n = n - 1; n >= 0; n--)
      *dest++ = *src++;
}


static
void copymem_aligned(const void *s, void *d, long n)
{
   const long *src = (const long *)s;
   long *dest = (long *)d;
   long n4;

   for(n4 = (n >> 2) - 1; n4 >= 0; n4--)
      *dest++ = *src++;

   if (n & 3) {
      const char *s1 = (const char *)src;
      char *d1 = (char *)dest;
      switch(n & 3) {
      case 3:
         *d1++ = *s1++;
      case 2:
         *d1++ = *s1++;
      case 1:
         *d1++ = *s1++;
         break;
      }
   }
}


void *memcpy(void *dest, const void *src, size_t n)
{
   if (n > 3) {
     if (!((long)dest & 1)) {
       if (!((long)src & 1)) {
          copymem_aligned(src, dest, n);
          return dest;
       }
     } else if ((long)src & 1) {
       *((char *)dest)++ = *((const char *)src)++;
       copymem_aligned(src, dest, n - 1);
       return (char *)dest - 1;
     }
   }

   copymem(src, dest, n);

   return dest;
}


void *memmove(void *dest, const void *src, long n)
{
   const char *s1;
   char *d1;

   if (((long)dest >= (long)src + n) || ((long)dest + n <= (long)src))
      return memcpy(dest, src, n);

   if ((long)dest < (long)src) {
      s1 = (const char *)src;
      d1 = (char *)dest;
      for(n--; n >= 0; n--)
         *d1++ = *s1++;
   } else {
      s1 = (const char *)src + n;
      d1 = (char *)dest + n;
      for(n--; n >= 0; n--)
         *(--d1) = *(--s1);
   }

   return dest;
}


static inline
void setmem(void *d, long v, long n)
{
   char *dest;

   dest = (char *)d;
   for(n = n - 1; n >= 0; n--)
      *dest++ = (char)v;
}


static
void setmem_aligned(void *d, long v, long n)
{
   long *dest;
   long n4;

   dest = (long *)d;
   for(n4 = (n >> 2) - 1; n4 >= 0; n4--)
      *dest++ = v;

   if (n & 3) {
      char *d1 = (char *)dest;
      switch(n & 3) {
      case 3:
         *d1++ = (char)(v >> 24);
      case 2:
         *d1++ = (char)(v >> 16);
      case 1:
         *d1++ = (char)(v >> 8);
         break;
      }
   }
}


/* This function needs an 'int' parameter
 * to be compatible with gcc's built-in
 * version.
 * For module use, a separate version will
 * be needed since they can't be guaranteed
 * to have the same size for 'int'.
 */
void *memset(void *s, int c, size_t n)
{
   if ((n > 3) && !((long)s & 1)) {
      unsigned long v;
      v = ((unsigned short)c << 8) | (unsigned short)c;
      v = (v << 16) | v;
      setmem_aligned(s, v, n);
   } else
      setmem(s, c, n);

   return s;
}


long strlen(const char *s)
{
   const char *p = s;
   while (*p++)
      ;

   return (long)(p - s) - 1;
}


int strcmp(const char *s1, const char *s2)
{
   char c1;

   do {
       if (!(c1 = *s1++)) {
           s2++;
           break;
       }
   } while (c1 == *s2++);

   return (long)(c1 - s2[-1]);
}


long strncmp(const char *s1, const char *s2, size_t n)
{
   char c1;
   long ns;     /* size_t can't be negative */

   ns = n;
   for(ns--; ns >= 0; ns--) {
      if (!(c1 = *s1++)) {
         s2++;
         break;
      }
      if (c1 != *s2++)
         break;
   }

   if (ns < 0)
      return 0L;

   return (long)(c1 - s2[-1]);
}


int memcmp(const void *s1, const void *s2, size_t n)
{
   const char *s1c, *s2c;
   long ns;     /* size_t can't be negative */

   ns = n;
   s1c = (const char *)s1;
   s2c = (const char *)s2;
   for(ns--; ns >= 0; ns--) {
      if (*s1c++ != *s2c++)
         return (long)(s1c[-1] - s2c[-1]);
   }

   return 0L;
}

static inline
void copy(const char *src, char *dest)
{
   while ( (*dest++ = *src++) )
      ;
}

char *strcpy(char *dest, const char *src)
{
   copy(src, dest);

   return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
   char c1, *d;
   long ns;

   d = dest;
   ns = n;
   for(ns--; ns >= 0; ns--) {
      c1 = *src++;
      *dest++ = c1;
      if (!c1)
         break;
   }
   for(ns--; ns >= 0; ns--)
      *dest++ = 0;

   return d;
}


char *strdup(const char *s)
{
   char *d;

   if ((d = (char *)malloc(strlen(s) + 1)))
      strcpy(d, s);

   return d;
}


static inline
void cat(const char *src, char *dest)
{
   while(*dest++)
      ;
   copy(src, dest - 1);
}


char *strcat(char *dest, const char *src)
{
   cat(src, dest);

   return dest;
}


char *strchr(const char *s, long c)
{
   char ch, c1;

   if (!c)
      return (char *)s + strlen(s);

   c1 = c;
   while(ch = *s++) {
      if (ch == c1)
         return (char *)s - 1;
   }

   return 0;
}


char *strrchr(const char *s, long c)
{
   char *found, ch, c1;

   if (!c)
      return (char *)s + strlen(s);

   c1 = c;
   found = 0;
   while(ch = *s++) {
      if (ch == c1)
         found = (char *)s;
   }

   if (found)
      return found - 1;

   return 0;
}


void *memchr(const void *s, long c, size_t n)
{
   char ch, c1;
   char *m;
   long ns;

   m = (char *)s;
   c1 = c;
   ns = n;
   for(ns--; ns >= 0; ns--) {
      ch = *m++;
      if (ch == c1)
         return (char *)m - 1;
   }

   return 0;
}


char *memrchr(const void *s, long c, size_t n)
{
   char ch, c1;
   const char *m;
   long ns;

   m = (const char *)s + n;
   c1 = c;
   ns = n;
   for(ns--; ns >= 0; ns--) {
      ch = *--m;
      if (ch == c1)
         return (void *)m;
   }

   return 0;
}


long numeric(long ch)
{
   if ((ch >= '0') && (ch <= '9'))
      return 1;
   
   return 0;
}


long check_base(char ch, long base)
{
   if (numeric(ch) && (ch < '0' + base))
      return ch - '0';
   if ((ch >= 'a') && (ch <= 'z'))
      ch -= 'a' - 'A';
   if ((ch >= 'A') && (ch < 'A' + base - 10))
      return ch - 'A' + 10;
   
   return -1;
}


long atol(const char *text)
{
   long n;
   int minus, base, ch;

   while(isspace(*text))
       text++;

   minus = 0;   
   if (*text == '-') {
      minus = 1;
      text++;
   }
   base = 10;
   if (*text == '$') {
      base = 16;
      text++;
   } else if (*text == '%') {
      base = 2;
      text++;
   }

   n = 0;
   while ((ch = check_base(*text++, base)) >= 0)
      n = n * base + ch;

   if (minus)
      n = -n;
   
   return n;
}


void ultoa(char *buf, unsigned long un, unsigned long base)
{
   char *tmp, ch;
   
   tmp = buf;
   do {
      ch = un % base;
      un = un / base;
      if (ch <= 9)
         ch += '0';
      else
         ch += 'a' - 10;
      *tmp++ = ch;
   } while (un);
   *tmp = '\0';
   while (tmp > buf) {
      ch = *buf;
      *buf++ = *--tmp;
      *tmp = ch;
   }
}


void ltoa(char *buf, long n, unsigned long base)
{
   unsigned long un;
   char *tmp, ch;
   
   un = n;
   if ((base == 10) && (n < 0)) {
      *buf++ = '-';
      un = -n;
   }
   
   tmp = buf;
   do {
      ch = un % base;
      un = un / base;
      if (ch <= 9)
         ch += '0';
      else
         ch += 'a' - 10;
      *tmp++ = ch;
   } while (un);
   *tmp = '\0';
   while (tmp > buf) {
      ch = *buf;
      *buf++ = *--tmp;
      *tmp = ch;
   }
}


/* Mostly complete, but only simple things tested */
long sprintf(char *str, const char *format, ...)
{
   long r;
   va_list args;
   va_start(args, format);
   r = vsprintf( str, format, args);
   va_end(args);
   return r;
}


/* Mostly complete, but only simple things tested */
long vsprintf(char *str, const char *format, va_list args)
{
   int mode = 0;
   char *s, *text, ch;
   long val_l = 0;
   int base = 10;
   int field = 0;
   int precision = 0;
   int opts = 0;

   s = str;

   while ( (ch = *format++) ) {
      if (mode) {
         switch(ch) {
         case 's':
            text = va_arg(args, char *);
            while ((*str++ = *text++))
               ;
            str--;
            mode = 0;
            break;

         case 'c':
            *str++ = va_arg(args, int);    /* char promoted to int */
            mode = 0;
            break;

         case 'p':
            opts |= 16;
            mode = 4;
         case 'x':
         case 'X':
            base = 16;
            if (opts & 16) {
               *str++ = '0';
               *str++ = 'x';
            }
         case 'o':
            if (base == 10) {
               base = 8;
               if (opts & 16) {
                  *str++ = '0';
               }
            }
         case 'u':
            opts |= 32;   /* Unsigned conversion */
         case 'd':
         case 'i':
           if (!(opts & 0x0100) && (opts & 1) && !(opts & 8)) {
               precision = field;
               field = 0;
            }
            switch (mode) {
            case 4:
               val_l = va_arg(args, long);
               break;
            case 3:
               if (opts & 32)
                  val_l = va_arg(args, unsigned char);
               else
                  val_l = va_arg(args, signed char);
               break;
            case 2:
               if (opts & 32)
                  val_l = va_arg(args, unsigned short);
               else
                  val_l = va_arg(args, short);
               break;
            case 1:
               if (opts & 32)
                  val_l = va_arg(args, unsigned int);
               else
                  val_l = va_arg(args, int);
               break;
            default:
               break;
            }
            if (!(opts & 32)) {
               if (val_l > 0) {
                  if (opts & 4)
                     *str++ = '+';
                  else if (opts & 2)
                     *str++ = ' ';
               } else if (val_l < 0) {
                  *str++ = '-';
                  val_l = -val_l;
               }
            }
            if (val_l || !(opts & 0x0100) || (precision != 0))
               ultoa(str, val_l, base);
            val_l = strlen(str);
            if (val_l < precision) {
              memmove(str + precision - val_l, str, val_l + 1);
              memset(str, '0', precision - val_l);
            }
            str += strlen(str);
            mode = 0;
            break;

         case '%':
            *str++ = '%';
            mode = 0;
            break;


         case 'h':
            opts |= 0x8000;
            if (mode == 1)
               mode = 2;
            else if (mode == 2)
               mode = 3;
            else
               opts |= 0x4000;
            break;

         case 'z':
         case 't':
         case 'l':
            opts |= 0x8000;
            if (mode == 1)
               mode = 4;
            else
               opts |= 0x4000;
            break;


         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            if (mode == 1) {
               ch -= '0';
               if (!ch && !(opts & 0x8000)) {
                  if (!(opts & 1))
                     opts |= 1;   /* Zero padding */
                  else
                     opts |= 0x4000;
               } else {
                  opts |= 0x8000;
                  if (opts & 0x0100) {
                     if (!(opts & 0x0200)) {
                        opts |= 0x0400;
                        precision = precision * 10 + ch;
                     } else
                        opts |= 0x4000;
                  } else if (!(opts & 0x0800)) {
                     opts |= 0x0400;
                     field = field * 10 + ch;
                  } else
                     opts |= 0x4000;
               }
            } else
               opts |= 0x4000;
            break;

         case '*':
            if (mode == 1) {
               if (opts & 0x0100) {
                  if (!(opts & 0x0600)) {
                     opts |= 0x0200;
                     precision = va_arg(args, int);
                     if (precision < 0)
                        precision = 0;
                  } else
                     opts |= 0x4000;
               } else if (!(opts & 0x8c00)) {
                  opts |= 0x8800;
                  field = va_arg(args, int);
                  if (field < 0) {
                     opts |= 8;
                     field = -field;
                  }
               } else
                  opts |= 0x4000;
            } else
               opts |= 0x4000;
            break;

         case ' ':
            if (!(opts & 0x8002)) {
               opts |= 2;   /* Space in front of positive numbers */
            } else
               opts |= 0x4000;
            break;

         case '+':
            if (!(opts & 0x8004)) {
               opts |= 4;   /* Sign in front of all numbers */
            } else
               opts |= 0x4000;
            break;

         case '-':
            if (!(opts & 0x8008)) {
               opts |= 8;   /* Left justified field */
            } else
               opts |= 0x4000;
            break;

         case '#':
            if (!(opts & 0x8010)) {
               opts |= 16;  /* 0x/0 in front of hexadecimal/octal numbers */
            } else
               opts |= 0x4000;
            break;

         case '.':
            if (!(opts & 0x0100) && (mode == 1)) {
               opts &= ~0x0400;
               opts |= 0x8100;
               precision = 0;
            } else
               opts |= 0x4000;
            break;

         default:
            opts |= 0x4000;
            break;
         }

         if (opts & 0x4000) {
           *str++ = '%';
           *str++ = '?';
           mode = 0;
         }

         if ((mode == 0) && (field > str - s)) {
           val_l = field - (str - s);
           if (opts & 8) {
             memset(str, ' ', val_l);
           } else {
             memmove(s + val_l, s, str - s);
             memset(s, ' ', val_l);
           }
           str += val_l;
         }
      } else if (ch == '%') {
         mode = 1;
         base = 10;
         opts = 0;
         field = 0;
         precision = 0;
         s = str;
      } else {
         *str++ = ch;
      }
   }
   *str = 0;

   return strlen(s);
}


void qsort(void *base, long nmemb, long size,
           long (*compar)(const void *, const void *))
{
    static long incs[16] = { 1391376, 463792, 198768, 86961, 33936, 13776, 
                             4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1 };
    long i, j, k, h, j_size, h_size;
    short n;
    char buf[16], *v, *p1, *p2, *cbase;

    v = buf;
    if (size > sizeof(buf)) {
       v = malloc(size);
       if (!v)       /* Can't sort? */
          return;
    }

   cbase = (char *)base;
   for(k = 0; k < 16; k++) {
      h = incs[k];
      h_size = h * size;
      for(i = h; i < nmemb; i++) {
         j = i;
         j_size = j * size;
         p1 = v;
         p2 = cbase + j_size;
         for(n = size - 1; n >= 0; n--)
            *p1++ = *p2++;
         while ((j >= h) && (compar(v, cbase + j_size - h_size) < 0)) {
            p1 = cbase + j_size;
            p2 = p1 - h_size;
            for(n = size - 1; n >= 0; n--)
               *p1++ = *p2++;
            j -= h;
            j_size -= h_size;
         }
         p1 = cbase + j_size;
         p2 = v;
         for(n = size - 1; n >= 0; n--)
            *p1++ = *p2++;
      }
   } 

   if (size > sizeof(buf))
      free(v);
}

void *fmalloc(long size, long type)
{
   Circle *new;

   if (mint | magic) {
      if (!(type & 0xfff8))  /* Simple type? */
         type |= 0x4030;     /* Keep around, supervisor accessible */
      new = (Circle *)Mxalloc(size + sizeof(Circle), type);
   } else {
      type &= 3;
      if (mxalloc)      /* Alternative if possible */
         new = (Circle *)Mxalloc(size + sizeof(Circle), type);
      else
         new = (Circle *)Malloc(size + sizeof(Circle));
   }
   
   if ((long)new > 0) {
      if ((debug > 2) && !(silentx[0] & 0x01)) {
         char buffer[10];
         ltoa(buffer, (long)new, 16);
         puts("Allocation at $");
         puts(buffer);
         puts(", ");
         ltoa(buffer, size, 10);
         puts(buffer);
         puts_nl(" bytes");
      }
      new->size = size + sizeof(Circle);
      *(long *)&new[1] = size;
      return (void *)&new[1];
   } else
      return new;
}


#if 0
  static short block_space[] = {32, 76, 156, 316, 1008, 2028, 4068, 8148, 16308};
#else
  static short block_space[] = {16, 48, 112, 240, 496, 1008, 2028, 4068, 8148, 16308};
#endif
  static char *block_free[]  = { 0,  0,   0,   0,   0,    0,    0,    0,    0,     0};
#if 0
  static char *block_used[]  = { 0,  0,   0,   0,   0,    0,    0,    0,    0,     0};
#else
#if 0
static Circle block_used[]  = { {0,0,0},  {0,0,0},   {0,0,0},   {0,0,0},   {0,0,0},    {0,0,0},    {0,0,0},    {0,0,0},    {0,0,0},     {0,0,0}};
#else
  static Circle *block_used[]  = { 0,  0,   0,   0,   0,    0,    0,    0,    0,     0};
#endif
#endif
  static short free_blocks[] = { 0,  0,   0,   0,   0,    0,    0,    0,    0,     0};
  static short used_blocks[] = { 0,  0,   0,   0,   0,    0,    0,    0,    0,     0};
  static short allocated = 0;

#define ADDR_NOT_OK 0xfc000003


void allocate(long amount)
{
  const int sizes = sizeof(block_space) / sizeof(block_space[0]);
  char *buf;
  Circle *link, *last;
  int i;

  amount &= ~0x0fL;
  if (!amount)
    return;

  buf = fmalloc(amount * 1024, 3);
  if (!buf)
    return;

  if ((debug > 2) && !(silentx[0] & 0x02)) {
    puts("       Malloc at ");
    ltoa(buf, (long)buf, 16);
    puts(buf);
    puts("\x0a\x0d");
  }

  last = (Circle *)block_free[sizes - 1];
  for(i = 0; i < amount; i += 16) {
    link = (Circle *)&buf[i * 1024L];
    link->next = last;
    link->prev = 0;
    link->size = sizes - 1;
    last = link;
  }

  block_free[sizes - 1] = (char *)last;
  free_blocks[sizes - 1] += amount >> 4;
  allocated += amount >> 4;
}


#define OS_MARGIN 64
#define MIN_BLOCK 32
#define LINK_SIZE (sizeof(Circle))
#define MIN_KB_BLOCK
void *malloc(long size)
{
  int m, n;
  const int sizes = sizeof(block_space) / sizeof(block_space[0]);
  char *block, buf[10];
  Circle *link, *next;

  if ( (size > 16 * 1024 - OS_MARGIN - LINK_SIZE))
    return fmalloc(size, 3);

  /* This will always break eventually thanks to the if-statement above */
  for(n = 0; n < sizes; n++) {
    if (size <= block_space[n])
      break;
  }

  if ((debug > 2) && !(silentx[0] & 0x02)) {
    ltoa(buf, n, 10);
    puts("Alloc: Need block of size ");
    puts(buf);
    puts("/");
    ltoa(buf, size, 10);
    puts(buf);
    puts("\x0a\x0d");
  }

  if (!block_free[n]) {
    for(m = n + 1; m < sizes; m++) {
      if (block_free[m])
        break;
    }
    if (m >= sizes) {
      m = sizes - 1;
      block_free[m] = fmalloc(16 * 1024 - OS_MARGIN, 3);
      if (!block_free[m])
        return 0;
      if ((debug > 2) && !(silentx[0] & 0x02)) {
        puts("       Malloc at ");
        ltoa(buf, (long)block_free[m], 16);
        puts(buf);
        puts("\x0a\x0d");
      }
      link = (Circle *)block_free[m];
      link->next = 0;
      link->prev = 0;
      link->size = m;
      free_blocks[m]++;
      allocated++;
    }
    for(; m > n; m--) {
      if ((debug > 2) && !(silentx[0] & 0x02)) {
        puts("       Splitting\x0a\x0d");
      }
      block_free[m - 1] = block_free[m];
      link = (Circle *)block_free[m];
      block_free[m] = (char *)link->next;
      free_blocks[m]--;
      next = (Circle *)(block_free[m - 1] + block_space[m - 1] + LINK_SIZE);
      link->next = next;
      link->prev = 0;
      link->size = m - 1;
      free_blocks[m - 1]++;
      link = next;
      link->next = 0;
      link->prev = 0;
      link->size = m - 1;
      free_blocks[m - 1]++;
    }
  } else {
    if ((debug > 2) && !(silentx[0] & 0x02)) {
      puts("       Available\x0a\x0d");
    }
  }

  block = block_free[n];
  block_free[n] = (char *)((Circle *)block)->next;

  if ((debug > 2) && !(silentx[0] & 0x02)) {
    puts("       Allocating at ");
    ltoa(buf, (long)block, 16);
    puts(buf);
    puts(" (next at ");
    ltoa(buf, (long)block_free[n], 16);
    puts(buf);
    puts(")\x0a\x0d");
  }

  ((Circle *)block)->size = (((Circle *)block)->size & 0xffff) + (size << 16);

      if (1 ) {
         Circle *new = (Circle *)block;
         if (block_used[n]) {
            new->prev = block_used[n]->prev;
            new->next = block_used[n];
            block_used[n]->prev->next = new;
            block_used[n]->prev = new;
         } else {
            block_used[n] = new;
            new->prev = new;
            new->next = new;
         }
      }

  free_blocks[n]--;
  used_blocks[n]++;

  *(long *)(block + sizeof(Circle)) = block_space[n];
  return block + sizeof(Circle);
}


void *realloc(void *addr, long new_size)
{
   Circle *current;
   long old_size;
   void *new;

   if (!addr)
      return malloc(new_size);
   if (!new_size) {
      free(addr);
      return 0;
   }

   new = malloc(new_size);
   if ((long)new <= 0)
      return 0;
   current = &((Circle *)addr)[-1];
#if 0
   if (current->prev)
#else
   if ((long)current->prev & 1)
#endif
      old_size = current->size - sizeof(Circle);
   else
      old_size = current->size >> 16;

   copymem_aligned(addr, new, old_size < new_size ? old_size : new_size);
   free(addr);

   if ((debug > 2) && !(silentx[0] & 0x01)) {
      char buffer[10];
      puts("Reallocation from size ");
      ltoa(buffer, old_size, 10);
      puts(buffer);
      puts(" at $");
      ltoa(buffer, (long)addr, 16);
      puts(buffer);
      puts(" to ");
      ltoa(buffer, new_size, 10);
      puts_nl(buffer);
   }

   return new;
}


long free(void *addr)
{
   Circle *current;
   long size, ret;

   if (!addr)
       return 0;

   current = &((Circle *)addr)[-1];

   if (!((long)current->prev & 1)) {
     size = current->size & 0xffff;
     if (((debug > 2) && !(silentx[0] & 0x02)) ||
         (unsigned int)size >= sizeof(block_space) / sizeof(block_space[0]) ||
         !(current->size >> 16)) {
       char buf[10];
       puts("Freeing at ");
       ltoa(buf, (long)current, 16);
       puts(buf);
#if 1
       puts(" (");
       ltoa(buf, (long)current->size, 16);
       puts(buf);
       puts(" ,");
       ltoa(buf, (long)current->prev, 16);
       puts(buf);
       puts(" ,");
       ltoa(buf, (long)current->next, 16);
       puts(buf);
       puts(")");
#endif
       puts("\x0a\x0d");
     }

   if (1) {
     if (block_used[size] == current) {
       block_used[size] = current->next;
       if (current->next == current->prev)
         block_used[size] = 0;
     }
     current->prev->next = current->next;
     current->next->prev = current->prev;
   }

     current->next = (Circle *)block_free[size];
     block_free[size] = (char *)current;
     free_blocks[size]++;
     used_blocks[size]--;
     return 0;
   } else {
     if ((debug > 2) && !(silentx[0] & 0x01)) {
       char buf[10];
       puts("Standard free at ");
       ltoa(buf, (long)current, 16);
       puts(buf);
       puts("\x0a\x0d");
     }
   }

   size = current->size;

   if ((debug > 2) && !(silentx[0] & 0x01)) {
      char buffer[10];
      ltoa(buffer, (long)current, 16);
      puts("Freeing at $");
      puts_nl(buffer);
   }
   ret = Mfree(current);
   
   if (ret)
      return ret;
   else
      return size;
}



long puts(const char *text)
{
#if 0
   nfCall((nf_print_id, text));
#endif
   return 1;
}


long init_utility(void)
{
   long tmp;
   
   tmp = (long)Mxalloc(10, 3);      /* Try allocating a little memory */
   if (tmp == -32)
      mxalloc = 0;
   else if (tmp < 0)
      return 0;                     /* Should not happen */
   else {
      mxalloc = 1;
      Mfree((void *)tmp);
   }

   return 1;
}

#if defined(__TOS__) && !defined(__mc68000__)
#  define __mc68000__ 1
#endif

#if defined(__COLDFIRE__) && !defined(__mcoldfire__)
#  define __mcoldfire__ 1
#endif

#if (defined(__68881__) || defined(_M68881) || defined(__M68881__)) && !defined(__HAVE_68881__)
#  define __HAVE_68881__ 1
#endif

/* Note: PureC until version 2.50 does not define any symbol when using -2 or better,
   you will have to do that in the Project/Makefile */
#if (defined(mc68020) || defined(__68020__) || defined(__M68020__)) && !defined(__mc68020__)
#  define __mc68020__ 1
#endif
#if (defined(mc68030) || defined(__68030__) || defined(__M68030__)) && !defined(__mc68030__)
#  define __mc68030__ 1
#endif
#if (defined(mc68040) || defined(__68040__) || defined(__M68040__)) && !defined(__mc68040__)
#  define __mc68040__ 1
#endif
#if (defined(mc68060) || defined(__68060__) || defined(__M68060__)) && !defined(__mc68060__)
#  define __mc68060__ 1
#endif

#if defined(__mcoldfire__)
 #define ASCII_ARCH_TARGET	"coldfire"
#elif defined(__mc68060__)
 #ifdef __mc68020__
  #define ASCII_ARCH_TARGET	"m68020-060"
 #else
  #define ASCII_ARCH_TARGET	"m68060"
 #endif
#elif defined(__mc68040__)
 #define ASCII_ARCH_TARGET	"m68040"
#elif defined(__mc68030__)
#ifdef __HAVE_68881__
 #define ASCII_ARCH_TARGET	"m68030+881"
#else
 #define ASCII_ARCH_TARGET	"m68030"
#endif
#elif defined(__mc68020__)
 #define ASCII_ARCH_TARGET	"m68020"
#elif defined(__mc68010__)
 #define ASCII_ARCH_TARGET	"m68010"
#elif defined(__mc68000__)
 #define ASCII_ARCH_TARGET	"m68000"
#else
 #define ASCII_ARCH_TARGET	"unknown 68k"
#endif

#if defined __GNUC__
#  define ASCII_COMPILER "GNU-C " __STRINGIFY(__GNUC__) "." __STRINGIFY(__GNUC_MINOR__) "." __STRINGIFY(__GNUC_PATCHLEVEL__)
#elif defined (__AHCC__)
#  define ASCII_COMPILER "AHCC " __STRINGIFY(__AHCC__)
#elif defined (__PUREC__)
#  define ASCII_COMPILER "Pure-C " __STRINGIFY(__PUREC__)
#elif defined (__SOZOBONX__)
#  define ASCII_COMPILER "Sozobon " __STRINGIFY(__SOZOBONX__)
#else
#  define ASCII_COMPILER "unknown compiler"
#endif

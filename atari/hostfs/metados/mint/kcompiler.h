#if !defined(__GNUC__)
# error "this driver should be compiled by GNU-C"
#endif

#if !defined(__MSHORT__)
# error "this driver must be compiled with -mshort"
#endif

#define _cdecl
#define EXITING void
#define NORETURN __attribute__((__noreturn__))
#define INLINE static __inline __attribute__ ((__always_inline__))

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef long long llong;
typedef __SIZE_TYPE__ size_t;

#undef NULL
#define NULL ((void *)0)

#define UNUSED(x) ((void)(x))


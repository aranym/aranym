 /*
  * UAE - The Un*x Amiga Emulator
  *
  * MC68000 emulation
  *
  * Copyright 1995 Bernd Schmidt
  */

#ifndef SPCFLAGS_H
#define SPCFLAGS_H

typedef uae_u32 spcflags_t;

enum {
	SPCFLAG_STOP			= 0x01,
	SPCFLAG_INT			= 0x02,
	SPCFLAG_BRK			= 0x04,
	SPCFLAG_TRACE			= 0x08,
	SPCFLAG_DOTRACE			= 0x10,
	SPCFLAG_DOINT			= 0x20,
#if USE_JIT
	SPCFLAG_JIT_END_COMPILE		= 0x40,
	SPCFLAG_JIT_EXEC_RETURN		= 0x80,
#else
	SPCFLAG_JIT_END_COMPILE		= 0,
	SPCFLAG_JIT_EXEC_RETURN		= 0,
#endif
	SPCFLAG_MFP_TIMERC		= 0x100,
	SPCFLAG_MFP_TIMERC2		= 0x200,
	SPCFLAG_MFP_ACIA		= 0x400,
	SPCFLAG_DISK			= 0x800,
	SPCFLAG_VBL			= 0x1000,
	SPCFLAG_MFP			= 0x2000,
	SPCFLAG_MODE_CHANGE		= 0x4000,
	
	SPCFLAG_MFP_ALL			= SPCFLAG_MFP_TIMERC
					| SPCFLAG_MFP_ACIA
					,
	
	SPCFLAG_STOP_EXEC		= SPCFLAG_INT
					| SPCFLAG_DOINT
					| SPCFLAG_MFP_ALL
					,

	SPCFLAG_ALL			= SPCFLAG_STOP
					| SPCFLAG_INT
					| SPCFLAG_BRK
					| SPCFLAG_TRACE
					| SPCFLAG_DOTRACE
					| SPCFLAG_DOINT
					| SPCFLAG_JIT_END_COMPILE
					| SPCFLAG_JIT_EXEC_RETURN
					| SPCFLAG_MFP_TIMERC
					| SPCFLAG_MFP_TIMERC2
					| SPCFLAG_MFP_ACIA
					,
	
	SPCFLAG_ALL_BUT_EXEC_RETURN	= SPCFLAG_ALL & ~SPCFLAG_JIT_EXEC_RETURN

};

#define SPCFLAGS_TEST(m) \
	((regs.spcflags & (m)) != 0)

/* Macro only used in m68k_reset() */
#define SPCFLAGS_INIT(m) do { \
	regs.spcflags = (m); \
} while (0)

#if !(ENABLE_EXCLUSIVE_SPCFLAGS)

#define SPCFLAGS_SET(m) do { \
	regs.spcflags |= (m); \
} while (0)

#define SPCFLAGS_CLEAR(m) do { \
	regs.spcflags &= ~(m); \
} while (0)

#elif defined(CPU_i386) && defined(X86_ASSEMBLY)

#define HAVE_HARDWARE_LOCKS

#define SPCFLAGS_SET(m) do { \
	ASM_VOLATILE("lock\n\torl %1,%0" : "=m" (regs.spcflags) : "i" ((m))); \
} while (0)

#define SPCFLAGS_CLEAR(m) do { \
	ASM_VOLATILE("lock\n\tandl %1,%0" : "=m" (regs.spcflags) : "i" (~(m))); \
} while (0)

#elif defined(HAVE_PTHREADS)

#undef HAVE_HARDWARE_LOCKS

#include <pthread.h>
extern pthread_mutex_t spcflags_lock;

#define SPCFLAGS_SET(m) do { 				\
	pthread_mutex_lock(&spcflags_lock);		\
	regs.spcflags |= (m);					\
	pthread_mutex_unlock(&spcflags_lock);	\
} while (0)

#define SPCFLAGS_CLEAR(m) do {				\
	pthread_mutex_lock(&spcflags_lock);		\
	regs.spcflags &= ~(m);					\
	pthread_mutex_unlock(&spcflags_lock);	\
} while (0)

#else

#error "Can't handle spcflags atomically!"

#endif

#endif /* SPCFLAGS_H */

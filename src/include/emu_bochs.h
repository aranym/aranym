
#ifndef _emu_bochs_h
#define _emu_bochs_h

// moje triky
#define put(a)
#define settype(a)
#define BX_DEBUG(a)	printf a
#define BX_PANIC(a)	printf a
#define BX_INFO(a)	printf a
#define BX_ASSERT(x) do {if (!(x)) BX_PANIC(("failed assertion \"%s\" at %s:%d\n", #x, __FILE__, __LINE__));} while (0)
#define BX_INSERTED	10
#define BX_EJECTED	11
// konec mych triku

#endif

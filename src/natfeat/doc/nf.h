/*
 * nf.h - Implementation interface for Native features
 *
 * Copyright (c) 2002 Laurent Vogel
 *
 * TODO - determine what license should apply (LGPL?)
 */

/*========================== basic types ===================================*/

/* TODO: extern C ..., #IFDEF _NF_H, ... */

#include <sys/types.h>
#include <inttypes.h>
#include <stdarg.h>

/* the type of an address in the 68k side */
typedef uint32_t nf_addr_t;

/* the type of the native function */
typedef void (*nf_func_t)(nf_addr_t);

#if BUS_ERROR_STRUCT
/* this structure contains details that will be used to fill in specific
 * fields in the bus error stack frame. it will be set by whatever functions
 * are used to check for bus error, and pass to nf_buserr() to actually
 * raise the bus error.
 */
struct nf_berr;
typedef struct nf_berr nf_berr_t;
#endif

/*========================== basic emulator glue ===========================*/

/* TODO: document behaviour about bus error */
int8_t   nf_get_b (nf_addr_t addr);
uint8_t  nf_get_ub(nf_addr_t addr);
int16_t  nf_get_w (nf_addr_t addr);
uint16_t nf_get_uw(nf_addr_t addr);
int32_t  nf_get_l (nf_addr_t addr);
uint32_t nf_get_ul(nf_addr_t addr);

void nf_set_b (nf_addr_t addr, int8_t   value);
void nf_set_ub(nf_addr_t addr, uint8_t  value);
void nf_set_w (nf_addr_t addr, int16_t  value);
void nf_set_uw(nf_addr_t addr, uint16_t value);
void nf_set_l (nf_addr_t addr, int32_t  value);
void nf_set_ul(nf_addr_t addr, uint32_t value);

void nf_set_d0l (int32_t  value);
void nf_set_d0ul(uint32_t value);

/* TODO: for native to 68k function call, more control is needed 
 * (push to stack, pop from stack, set pc, ...)
 */

/* returns != 0 if the memory zone (from begin to end inclusive)
 * is readable, else sets bad to any bad address within the zone
 */
int nf_readable (nf_addr_t begin, nf_addr_t end, nf_addr_t *bad);
int nf_writeable(nf_addr_t begin, nf_addr_t end, nf_addr_t *bad);

#if NEEDS_FURTHER_THOUGHT
/* interrupt level 6
 * TODO: needs further thought 
 */
void nf_interrupt(int number); 
#endif

/* raise a bus error for the given address; write != 0 if the bus error
 * occurred when writing, 0 if reading.
 * TODO: the bus error case should be handled differently. when detecting
 * that a bus error occurs, the utility function should save this bus error
 * context, and the native function implementation should simply call
 * bus error and return ??? Or, the nf_addr_t *bad should be replaced 
 * by a type nf_berr_t * pointer to a structure.
 */
void nf_buserr (nf_addr_t bad, int write);

/*========================== context handling ===========================*/

/* functions providing a means to encapsulate native addresses into 
 * a 'token' that can be represented in an int32_t regardless of the
 * native bus address size.
 */

/* returns a token usable to later retrieve the passed ptr */
int32_t nf_to_token(void * ptr);

/* returns the ptr that was earlier associated with this token.
 * the token is not destroyed and the ptr can be retrieved any number of
 * times.
 * TODO: what happens if the token is not a token created by nf_to_token?
 */
void * nf_from_token(int32_t token);

/* frees the token. It is unspecified what happens if further calls to 
 * nf_from_token() on this same token happen.
 */ 
void * nf_free_token(int32_t token);


/*========================== utility functions ===========================*/

/* these can be implemented using the other above, or using more direct
 * access to the emulator internals.
 */

/* TODO: what if bus error? */
void nf_set_zone(nf_addr_t addr, void *buf, size_t size);
void nf_get_zone(void *buf, nf_addr_t addr, size_t size);

/* returns the length of the string in 68k memory, or -1 if a bus error
 * occured. TODO: what is the bad address then?
 */
int nf_strlen(nf_addr_t addr);

/* returns a malloc()-allocated copy of the string found in 68k memory.
 * TODO: what if no memory, what if bus error?
 * TODO: does the use of malloc cause a problem? should a nf_malloc be
 * defined (to allow using whatever allocation scheme is available in
 * the emulator)?
 */
char * nf_get_str(nf_addr_t addr);

/* copies in 68k memory the supplied string.
 * TODO: what if bus error?
 */
void nf_set_str(nf_addr_t addr, char *s);

/* copies up to size chars in native memory, and returns a malloc()-allocated
 * string that is always zero-terminated.
 * TODO: memory, bus error?
 */
char * nf_get_nstr(nf_addr_t addr, int size);

/* copies in 68k memory the supplied string, up to size chars.
 * TODO: is the string zero-terminated?
 * TODO: what if bus error?
 */
void nf_set_nstr(nf_addr_t addr, char *s, int size);

/*========================== management functions ========================*/

/* called either by the nf manager or by implementations to report errors 
 * TODO: check if this is coherent with what emulators provide 
 */
void nf_error(const char *fmt, ...);
void nf_verror(const char *fmt, va_list ap);

/* called by the emulator to initialise the nf manager; non-zero means error.*/
int nf_init(void);

/* called by a native implementation module to register native functions
 * returns the id by which the native function will be callable from 68k side.
 * a zero id is returned in case of error (no memory, or duplicate name).
 * supplying a NULL name registers an unnamed function.
 */
int32_t nf_register_func(const char * name, nf_func_t func);

#if VALUE_ONLY_FEATURES
/* if value-only features are obtained using the ID,
 * called by a native implementation module to register native 
 * value-only features. returns 0 if an error occurs 
 * (no memory, or duplicate name)
 */
int nf_register_value(const char *name, int32_t value);
#endif

/* called by the CPU core */
int32_t nf_get_id(nf_addr_t name);

nf_func_t nf_get_func(int32_t id);


/* TODO: implement the four native opcodes */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Tables for labelling ARAnyM internals. 
  *
  */

#include "sysdeps.h"

struct mem_labels
{
    const char *name;
    memptr adr;
};

struct customData
{
    const char *name;
    memptr adr;
};


extern struct mem_labels mem_labels[];
extern struct mem_labels int_labels[];
extern struct mem_labels trap_labels[];


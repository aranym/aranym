 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Routines for labelling ARAnyM internals. 
  *
  */

#include "identify.h"

struct mem_labels int_labels[] =
{
    { "Reset:SSP",  0x0000 },
    { "EXECBASE",   0x0004 },
    { "BUS ERROR",  0x0008 },
    { "ADR ERROR",  0x000C },
    { "ILLEG OPC",  0x0010 },
    { "DIV BY 0",   0x0014 },
    { "CHK",        0x0018 },
    { "TRAPV",      0x001C },
    { "PRIVIL VIO", 0x0020 },
    { "TRACE",      0x0024 },
    { "LINEA EMU",  0x0028 },
    { "LINEF EMU",  0x002C },
    { "INT Uninit", 0x003C },
    { "INT Unjust", 0x0060 },
    { "Lvl 1 Int",  0x0064 },
    { "Lvl 2 Int",  0x0068 },
    { "Lvl 3 Int",  0x006C },
    { "Lvl 4 Int",  0x0070 },
    { "Lvl 5 Int",  0x0074 },
    { "Lvl 6 Int",  0x0078 },
    { "NMI",        0x007C },
    { 0, 0 }
};

struct mem_labels trap_labels[] =
{
    { "TRAP 01",    0x0080 },
    { "TRAP 02",    0x0084 },
    { "TRAP 03",    0x0088 },
    { "TRAP 04",    0x008C },
    { "TRAP 05",    0x0090 },
    { "TRAP 06",    0x0094 },
    { "TRAP 07",    0x0098 },
    { "TRAP 08",    0x009C },
    { "TRAP 09",    0x00A0 },
    { "TRAP 10",    0x00A4 },
    { "TRAP 11",    0x00A8 },
    { "TRAP 12",    0x00AC },
    { "TRAP 13",    0x00B0 },
    { "TRAP 14",    0x00B4 },
    { "TRAP 15",    0x00B8 },
    { 0, 0 }
};

/*
struct mem_labels mem_labels[] =
{
    { NULL, 0 }
};
*/


/* 2001 MJ */

 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Memory management
  *
  * (c) 1995 Bernd Schmidt
  */

#include <stdio.h>
#include <stdlib.h>

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "m68k.h"
#include "newcpu.h"
#include "memory.h"
#include "readcpu.h"
#include "newcpu.h"
#include "main.h"
#include "exceptions.h"

#if !REAL_ADDRESSING && !DIRECT_ADDRESSING
// This part need rewrite for ARAnyM !!
// It can be taken from hatari.

#error Not prepared for your platform, maybe you need memory banks from UAE 

#endif /* !REAL_ADDRESSING && !DIRECT_ADDRESSING */

#ifdef FULLMMU

uaecptr mmu_decode_addr(uaecptr addr, bool data, bool write)
{
    uae_u32 rp;
    uaecptr mask;
    if (regs.s) rp = regs.srp; else rp = regs.urp;
    if (data) {
      if (regs.dtt0 & 0x8000) {
        if (((regs.dtt0 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.dtt0 & 0x6000))
              || (regs.s && ((regs.dtt0 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.dtt0 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.dtt0) & 0xff0000) << 8;
          if ((addr & mask) == (regs.dtt0 & mask)) return addr;
        }
      }
      if (regs.dtt1 & 0x8000) {
        if (((regs.dtt1 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.dtt1 & 0x6000))
              || (regs.s && ((regs.dtt1 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.dtt1 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.dtt1) & 0xff0000) << 8;
          if ((addr & mask) == (regs.dtt1 & mask)) return addr;
        }
      }
      if (regs.tcp) {
        uaecptr atcindex = ((addr << 11) >> 24);
        if (regs.atcvald[atcindex] && (regs.atcind[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperd[atcindex]) {
            if (!write || !regs.atcwritepd[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifd[atcindex]) {
                regs.atcmodifd[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
			         last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (!(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 3);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 3);
		}
              }
              return regs.atcoutd[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcind[atcindex] = addr & 0xffffe000;
                         regs.atcoutd[atcindex] = pd & 0xffffe000;
                         regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifd[atcindex] = write;
                         regs.atcwritepd[atcindex] = wr ? 1 : 0;
                         regs.atcresidd[atcindex] = 1;
                         regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2d[atcindex] = regs.s; // ??
                         addr = (pd & 0xffffe000) | (addr & 0x00001fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
          }
        }
      } else {
        uaecptr atcindex = ((addr << 12) >> 24);
        if (regs.atcvald[atcindex] && (regs.atcind[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperd[atcindex]) {
            if (!write || !regs.atcwritepd[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifd[atcindex]) {
                regs.atcmodifd[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (write && !(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 2);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 2);
		}
              }
              return regs.atcoutd[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcind[atcindex] = addr & 0xfffff000;
                         regs.atcoutd[atcindex] = pd & 0xfffff000;
                         regs.atcu0d[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1d[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperd[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmd[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifd[atcindex] = write;
                         regs.atcwritepd[atcindex] = wr ? 1 : 0;
                         regs.atcresidd[atcindex] = 1;
                         regs.atcglobald[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2d[atcindex] = regs.s; // ??
                         addr = (pd & 0xfffff000) | (addr & 0x00000fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
	  }
        }    
      }
    } else {
      if (regs.itt0 & 0x8000) {
        if (((regs.itt0 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.itt0 & 0x6000))
              || (regs.s && ((regs.itt0 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.itt0 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.itt0) & 0xff0000) << 8;
          if ((addr & mask) == (regs.itt0 & mask)) return addr;
        }
      }
      if (regs.itt1 & 0x8000) {
        if (((regs.itt1 & 0x6000) > 0x3000)
              || (!regs.s || !(regs.itt1 & 0x6000))
              || (regs.s && ((regs.itt1 & 0x6000) == 0x2000))) {
          if ((write) && ((regs.itt1 & 0x4) != 0)) {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
          mask = ((~regs.itt1) & 0xff0000) << 8;
          if ((addr & mask) == (regs.itt1 & mask)) return addr;
        }
      }
      if (regs.tcp) {
        uaecptr atcindex = ((addr << 11) >> 24);
        if (regs.atcvali[atcindex] && (regs.atcini[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperi[atcindex]) {
            if (!write || !regs.atcwritepi[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifi[atcindex]) {
                regs.atcmodifi[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (!(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 2);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 2);
		}
              }
              return regs.atcouti[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff80) | ((addr & 0x0003e000) >> 11));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcini[atcindex] = addr & 0xffffe000;
                         regs.atcouti[atcindex] = pd & 0xffffe000;
                         regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifi[atcindex] = write;
                         regs.atcwritepi[atcindex] = wr ? 1 : 0;
                         regs.atcresidi[atcindex] = 1;
                         regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2i[atcindex] = regs.s; // ??
                         addr = (pd & 0xffffe000) | (addr & 0x00001fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
	  } 
        }
      } else {
        uaecptr atcindex = ((addr << 12) >> 24);
        if (regs.atcvali[atcindex] && (regs.atcini[atcindex] = addr)) {
          if (!regs.s || !regs.atcsuperi[atcindex]) {
            if (!write || !regs.atcwritepi[atcindex]) {
              // Resident?
              if (write && !regs.atcmodifi[atcindex]) {
                regs.atcmodifi[atcindex] = 1;
                // Najit v pameti a nastavit modif
                uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
                uint16 root = *rootp;
                uint8 *apdt, *apd;
                uint16 pdt, pd;
                flagtype wr;
                if ((root & 0x3) > 1) {
                  wr = (root & 0x4) >> 2;         // write
                  *rootp = root | 0x8;            // used
                  apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
                  pdt = *apdt;
                  if ((pdt & 0x3) > 1) {
                    wr += (pdt & 0x4) >> 2;
                    *apdt = pdt | 0x8;
                    apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
                    pd = *apd;
                    switch (pd & 0x3) {
                      case 0: 
		               last_fault_for_exception_3 = pd & 0xfffffffc;
			       longjmp(excep_env, 2);
                      case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                               pd = *apd;
                               if (((pd & 0x3) % 2) == 0) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 2);
			       }
                      default: wr += (pd & 0x4) >> 2;
                               if (write && !(wr > 1)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               if (!regs.s && ((pd & 0x80) != 0)) {
				 last_fault_for_exception_3 = pd & 0xfffffffc;
				 longjmp(excep_env, 3);
			       }
                               *apd = pd | 0x18;
                    }
                  } else {
		    last_fault_for_exception_3 = pdt & 0xfffffffc;
		    longjmp(excep_env, 2);
		  }
                } else {
		  last_fault_for_exception_3 = root & 0xfffffffc;
		  longjmp(excep_env, 2);
		}
              }
              return regs.atcouti[atcindex];
            } else {
	      last_fault_for_exception_3 = addr;
	      longjmp(excep_env, 3);
	    }
          } else {
	    last_fault_for_exception_3 = addr;
	    longjmp(excep_env, 3);
	  }
        } else {
          uint16 *rootp = (uint16 *)do_get_real_address_direct(rp & ((addr >> 25) << 2));
          uint16 root = *rootp;
          uint8 *apdt, *apd;
          uint16 pdt, pd;
          flagtype wr;
          if ((root & 0x3) > 1) {
            wr = (root & 0x4) >> 2;         // write
            *rootp = root | 0x8;            // used
            apdt = do_get_real_address_direct((root & 0xfffffe00) | ((addr & 0x01fc0000) >> 16));
            pdt = *apdt;
            if ((pdt & 0x3) > 1) {
              wr += (pdt & 0x4) >> 2;
              *apdt = pdt | 0x8;
              apd = do_get_real_address_direct((pdt & 0xffffff00) | ((addr & 0x0003f000) >> 10));
              pd = *apd;
              switch (pd & 0x3) {
                case 0: 
		         last_fault_for_exception_3 = pd & 0xfffffffc;
			 longjmp(excep_env, 2);
                case 2:  apd = do_get_real_address_direct(pd & 0xfffffffc);
                         pd = *apd;
                         if (((pd & 0x3) % 2) == 0) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 2);
			 }
                default: wr += (pd & 0x4) >> 2;
                         if (write && !(wr > 1)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         if (!regs.s && ((pd & 0x80) != 0)) {
			   last_fault_for_exception_3 = pd & 0xfffffffc;
			   longjmp(excep_env, 3);
			 }
                         *apd = pd | 0x8;
                         if (write) *apd = pd | 0x10;
                         regs.atcini[atcindex] = addr & 0xfffff000;
                         regs.atcouti[atcindex] = pd & 0xfffff000;
                         regs.atcu0i[atcindex] = (pd & 0x00000100) >> 8;
                         regs.atcu1i[atcindex] = (pd & 0x00000200) >> 9;
                         regs.atcsuperi[atcindex] = (pd & 0x00000080) >> 7;
                         regs.atccmi[atcindex] = (pd & 0x00000060) >> 5;
                         regs.atcmodifi[atcindex] = write;
                         regs.atcwritepi[atcindex] = wr ? 1 : 0;
                         regs.atcresidi[atcindex] = 1;
                         regs.atcglobali[atcindex] = (pd & 0x00000400) >> 10;
                         regs.atcfc2i[atcindex] = regs.s; // ??
                         addr = (pd & 0xfffff000) | (addr & 0x00000fff);
              }
            } else {
	      last_fault_for_exception_3 = pdt & 0xfffffffc;
	      longjmp(excep_env, 2);
	    }
          } else {
	    last_fault_for_exception_3 = root & 0xfffffffc;
	    longjmp(excep_env, 2);
	  }
        }    
      }
    }
  return addr;
}

#endif /* FULLMMU */

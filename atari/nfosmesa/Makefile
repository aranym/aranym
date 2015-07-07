include ../CONFIGVARS

#
# The NFOSMesa code does nothing but call out to the
# emulator, so there is no need to create extra m68k/20/30/40 versions.
# We create a coldfire binary, though.
# (of course a v4e binary makes only sense when Aranym is emulating a FireBEE ;)
#
CFLAGS  = -O2 -fomit-frame-pointer -Wall -I. -DTGL_ENABLE_CHECKS
ASFLAGS =
LDFLAGS = -lldg -lgem

NOSTARTUP=yes

OBJECTS = nfosmesa.o lib-osmesa.o lib-oldmesa.o lib-misc.o lib-ldg.o lib-gl.o natfeat.o natfeat_asm.o

ifeq ($(NOSTARTUP),yes)
OBJECTS += crtinit.o libc.o
LDFLAGS += -lgcc
CRT0 = crt0.o
STARTUP = -nostartfiles -nostdlib
endif


M68K_OBJECTS = $(OBJECTS:.o=_68k.o)
MV4E_OBJECTS = $(OBJECTS:.o=_v4e.o)

M68K_FLAGS = -m68000
M020_FLAGS = -m68020-60
M030_FLAGS = -m68030 -m68881
M060_FLAGS = -m68060
MV4E_FLAGS = -mcpu=5475

all:	dirs m68k/osmesa.ldg v4e/osmesa.ldg \
	m68k/tiny_gl.ldg v4e/tiny_gl.ldg \
	m68k/tiny_gl.slb m68k/osmesa.slb


crt0.o: crt0.S
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<



m68k/osmesa.ldg:	$(CRT0) $(M68K_OBJECTS)
	$(AM_V_LD)${CC} $(M68K_FLAGS) -o $@ $(STARTUP)  $^ ${LDFLAGS}
	$(AM_V_at)${STRIP} $@

nfosmesa_68k.o: nfosmesa.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

lib-osmesa_68k.o: lib-osmesa.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

lib-oldmesa_68k.o: lib-oldmesa.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

lib-misc_68k.o: lib-misc.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

lib-ldg_68k.o: lib-ldg.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

lib-gl_68k.o: lib-gl.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

natfeat_68k.o: ../natfeat/natfeat.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

natfeat_asm_68k.o: ../natfeat/natfeat_asm.S
	$(AM_V_CC)$(CC) $(ASFLAGS) $(M68K_FLAGS) -c -o $@ $<


v4e/osmesa.ldg:	$(CRT0) $(MV4E_OBJECTS)
	$(AM_V_LD)${CC} $(MV4E_FLAGS) -o $@ $(STARTUP) $^ ${LDFLAGS}
	$(AM_V_at)${STRIP} $@

nfosmesa_v4e.o: nfosmesa.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

lib-osmesa_v4e.o: lib-osmesa.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

lib-oldmesa_v4e.o: lib-oldmesa.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

lib-misc_v4e.o: lib-misc.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

lib-ldg_v4e.o: lib-ldg.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

lib-gl_v4e.o: lib-gl.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

natfeat_v4e.o: ../natfeat/natfeat.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

natfeat_asm_v4e.o: ../natfeat/natfeat_asm.S
	$(AM_V_CC)$(CC) $(ASFLAGS) $(MV4E_FLAGS) -c -o $@ $<


TINY_OBJECTS = nftiny_gl.o lib-osmesa.o lib-oldmesa.o lib-misc.o lib-ldg.o lib-tinygl.o natfeat.o natfeat_asm.o 

ifeq ($(NOSTARTUP),yes)
TINY_OBJECTS += crtinit.o libc.o
endif

TINY_M68K_OBJECTS = $(TINY_OBJECTS:.o=_68k.o)
TINY_MV4E_OBJECTS = $(TINY_OBJECTS:.o=_v4e.o)

m68k/tiny_gl.ldg:	$(CRT0) $(TINY_M68K_OBJECTS)
	$(AM_V_LD)${CC} $(M68K_FLAGS) -o $@ $(STARTUP) $^ ${LDFLAGS}
	$(AM_V_at)${STRIP} $@

nftiny_gl_68k.o: nftiny_gl.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

lib-tinygl_68k.o: lib-tinygl.c lib-gl.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

crtinit_68k.o: crtinit.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<

libc_68k.o: libc.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) -c -o $@ $<



v4e/tiny_gl.ldg:	$(CRT0) $(TINY_MV4E_OBJECTS)
	$(AM_V_LD)${CC} $(MV4E_FLAGS) -o $@ $(STARTUP) $^ ${LDFLAGS}
	$(AM_V_at)${STRIP} $@

nftiny_gl_v4e.o: nftiny_gl.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

lib-tinygl_v4e.o: lib-tinygl.c lib-gl.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

crtinit_v4e.o: crtinit.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<

libc_v4e.o: libc.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(MV4E_FLAGS) -c -o $@ $<


STRIPEX = ../tools/stripex

# -ffreestanding is needed to prevent GNU-C from optimizing fprintf to fputs
# -fno-unit-at-a-time is needed to prevent static functions
# to be moved before the SLB header
SLB_CFLAGS = -fno-unit-at-a-time -ffreestanding

TINYGL_SLB_OBJS=tiny_gl_slb.o lib-osmesa_68k.o lib-oldmesa_68k.o lib-misc_68k.o libc_68k.o natfeat_68k.o natfeat_asm_68k.o
OSMESA_SLB_OBJS=osmesa_slb.o lib-osmesa_68k.o lib-oldmesa_68k.o lib-misc_68k.o libc_68k.o natfeat_68k.o natfeat_asm_68k.o

# -mprg-flags,15 is fastload, load to ttram, mallocs from ttram, and bit3 required for SLBs
m68k/tiny_gl.slb: $(TINYGL_SLB_OBJS) $(STRIPEX)
	$(AM_V_LD)$(CC) $(M68K_FLAGS) -nostartfiles -nostdlib -Wl,--mprg-flags,15 -Wl,-b,a.out-mintprg -s -o $@ $(TINYGL_SLB_OBJS) -lgcc
	$(AM_V_at)$(STRIPEX) $@

m68k/osmesa.slb: $(OSMESA_SLB_OBJS) $(STRIPEX)
	$(AM_V_LD)$(CC) $(M68K_FLAGS) -nostartfiles -nostdlib -Wl,--mprg-flags,15 -Wl,-b,a.out-mintprg -s -o $@ $(OSMESA_SLB_OBJS) -lgcc
	$(AM_V_at)$(STRIPEX) $@

tiny_gl_slb.o: tiny_gl_slb.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) $(SLB_CFLAGS) -c -o $@ $<
osmesa_slb.o: osmesa_slb.c
	$(AM_V_CC)$(CC) $(CFLAGS) $(M68K_FLAGS) $(SLB_CFLAGS) -c -o $@ $<

$(STRIPEX): ../tools/stripex.c
	$(AM_V_CC)$(NATIVECC) $(NATIVECFLAGS) -o $@ $<


dirs:
	$(AM_V_at)$(MKDIR_P) m68k v4e

clean:
	rm -f *.o m68k/*.ldg m68k/*.slb v4e/*.ldg v4e/*.slb $(STRIPEX)
	-rmdir m68k v4e

TESTDIR=/windows/c/atari/src/ldg-code/test
cptest: tinygl_load.c 
	cp nfosmesa_load.c $(TESTDIR)/nfosmesa.c
	cp tinygl_load.c $(TESTDIR)/nftinygl.c
	cp tinygl_loadslb.c $(TESTDIR)/tinyslb.c
	cp ldg/nfosmesa.h $(TESTDIR)/ldg
	cp ldg/tiny_gl.h $(TESTDIR)/ldg
	cp slb/tiny_gl.h $(TESTDIR)/slb
	cd $(TESTDIR) && cr2nl -d -R .

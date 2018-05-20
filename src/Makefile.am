SUBDIRS = uae_cpu

AM_CPPFLAGS = $(DEFINES) \
	-I$(srcdir)/include \
	-I$(srcdir)/Unix \
	-I$(builddir) \
	-I$(srcdir)/natfeat \
	-I$(builddir)/uae_cpu \
	-I$(srcdir)/uae_cpu

if OS_TYPE_beos
AM_CPPFLAGS += -I$(srcdir)/Unix/beos
endif
if SDL_GUI
AM_CPPFLAGS += -I$(srcdir)/gui-sdl
endif

CFLAGS_NOWARN = $(DBGSP) $(ARCHFLAGS) $(SDL_CFLAGS) $(ZLIB_CFLAGS) $(DISASM_CFLAGS)
AM_CFLAGS = $(CFLAGS_NOWARN) $(WFLAGS)
AM_CXXFLAGS = $(CFLAGS_NOWARN) $(WFLAGS)
AM_LDFLAGS = $(ARCHFLAGS)

LIBS = @LIBS@ $(SDL_LIBS) $(JPEG_LIBS) $(ZLIB_LIBS) $(DISASM_LIBS)

BEOS_RESOURCES = $(srcdir)/Unix/beos/aranym.rdef

BUNDLE_RESOURCES = \
	$(srcdir)/Unix/darwin/busybee.icns \
	$(top_srcdir)/data/logo.bmp

BUNDLE_LOCALIZATON_FOLDERS = \
	$(srcdir)/Unix/darwin/*.lproj

BUNDLE_CONTENTS = \
	$(srcdir)/Unix/darwin/Info.plist \
	$(srcdir)/Unix/darwin/PkgInfo 	

BUNDLE_FRAMEWORKS =  /Library/Frameworks/SDL.framework /Library/Frameworks/SDL_image.framework

APP = aranym
APP_ARCH = aranym_$(ARCH)
APP_STATIC = aranym_static
APP_LIB = libaranym.a
APP_BUNDLE = ARAnyM.app

bin_PROGRAMS = aranym
noinst_LIBRARIES = libaranym.a
if NEED_ARATAPIF
bin_PROGRAMS += aratapif
endif
if OS_TYPE_darwin
bin_SCRIPTS = $(APP_BUNDLE)
endif

EXTRA_DIST =

## Files
aranym_SOURCES = \
	Unix/sysdeps.h \
	main.cpp \
	Unix/main_unix.cpp \
	Unix/vm_alloc.cpp Unix/vm_alloc.h \
	Unix/host_clock_unix.cpp \
	Unix/vasprintf.c Unix/ansidecl.h \
	acia.cpp \
	acsifdc.cpp \
	adler32.c \
	aradata.cpp \
	aranym_exception.cpp \
	ata.cpp \
	audio_conv.cpp \
	audio_dma.cpp \
	audio_crossbar.cpp \
	blitter.cpp \
	bootos.cpp \
	bootos_tos.cpp \
	bootos_emutos.cpp \
	bootos_linux.cpp \
	cdrom.cpp \
	cfgopts.cpp \
	dirty_rects.cpp \
	dsp.cpp \
	disasm/disasm-builtin.h \
	disasm/disasm-bfd.h \
	disasm/disasm-glue.cpp \
	disasm/disasm-builtin.cpp \
	disasm/disasm-x86.cpp \
	disasm/disasm-arm.cpp \
	disasm/disasm-arm.h \
	disasm/arm.h \
	disasm/disasm-arm-builtin.c \
	disasm/disasm-x86.h \
	disasm/i386.h \
	disasm/i386-opc.h \
	disasm/i386-dis-evex.h \
	disasm/disasm-x86-builtin.c \
	emul_op.cpp \
	glenums.c \
	natfeats.cpp \
	fakeio.cpp \
	hardware.cpp \
	host.cpp \
	host_audio.cpp \
	host_clock.cpp \
	hostscreen.cpp \
	hostscreen_opengl.cpp \
	host_surface.cpp \
	host_surface_opengl.cpp \
	ide.cpp \
	identify.cpp \
	ikbd.cpp \
	input.cpp \
	joypads.cpp \
	logo.cpp \
	sdl_bmp.cpp \
	md5.cpp \
	mfp.cpp \
	midi.cpp \
	midi_file.cpp \
	mmu.cpp \
	ndebug.cpp \
	ncr5380.cpp \
	parallel.cpp \
	parallel_file.cpp \
	parameters.cpp \
	romdiff.cpp \
	rtc.cpp \
	serial.cpp \
	scc.cpp \
	tools.cpp \
	videl.cpp \
	videl_zoom.cpp \
	yamaha.cpp \
	natfeat/nf_base.cpp natfeat/nf_base.h \
	natfeat/nf_objs.cpp natfeat/nf_objs.h \
	natfeat/xhdi.cpp natfeat/xhdi.h natfeat/atari_rootsec.h \
	natfeat/nfaudio.cpp natfeat/nfaudio.h \
	natfeat/nfbootstrap.cpp natfeat/nfbootstrap.h \
	natfeat/nf_basicset.cpp natfeat/nf_basicset.h \
	natfeat/debugprintf.cpp natfeat/debugprintf.h \
	natfeat/maptab.cpp natfeat/maptab.h \
	natfeat/nf_scsidrv.cpp natfeat/nf_scsidrv.h \
	natfeat/nf_hostexec.cpp natfeat/nf_hostexec.h \
	uae_cpu/cpu_emulation.h \
	uae_cpu/aranym_glue.cpp \
	uae_cpu/memory.cpp uae_cpu/memory-uae.h \
	uae_cpu/newcpu.cpp uae_cpu/newcpu.h uae_cpu/noflags.h uae_cpu/registers.h uae_cpu/spcflags.h \
	uae_cpu/readcpua.cpp uae_cpu/readcpu.h \
	uae_cpu/cpummu.cpp uae_cpu/cpummu.h \
	uae_cpu/fpu/core.h \
	uae_cpu/fpu/fpu.h \
	uae_cpu/fpu/impl.h \
	uae_cpu/fpu/types.h \
	$(empty)

# these are included elsehere and not compiled separately
EXTRA_DIST += \
	uae_cpu/fpu/exceptions.cpp uae_cpu/fpu/exceptions.h \
	uae_cpu/fpu/flags.cpp uae_cpu/fpu/flags.h \
	uae_cpu/fpu/mathlib.cpp uae_cpu/fpu/mathlib.h \
	uae_cpu/fpu/rounding.cpp uae_cpu/fpu/rounding.h \
	$(empty)


#
# put the objects of generated sources in a convenience library,
# so that we can define separate CFLAGS for them
# (we want to omit the warning flags)
#
libaranym_a_SOURCES = \
	uae_cpu/cpudefsa.cpp \
	uae_cpu/cpustbla.cpp \
	uae_cpu/cpufunctbla.cpp \
	uae_cpu/cpuemu1.cpp \
	uae_cpu/cpuemu2.cpp \
	uae_cpu/cpuemu3.cpp \
	uae_cpu/cpuemu4.cpp \
	uae_cpu/cpuemu5.cpp \
	uae_cpu/cpuemu6.cpp \
	uae_cpu/cpuemu7.cpp \
	uae_cpu/cpuemu8.cpp \
	$(empty)
libaranym_a_CFLAGS = $(CFLAGS_NOWARN)
libaranym_a_CXXFLAGS = $(CFLAGS_NOWARN)

aranym_LDADD = libaranym.a

if SDL_GUI
aranym_SOURCES += \
	gui-sdl/sdlgui.cpp gui-sdl/sdlgui.h \
	gui-sdl/sdlgui.rsc gui-sdl/sdlgui.rso gui-sdl/sdlgui.sdl \
	gui-sdl/dialog.cpp gui-sdl/dialog.h \
	gui-sdl/file.cpp gui-sdl/file.h \
	gui-sdl/dlgMain.cpp gui-sdl/dlgMain.h \
	gui-sdl/font.h gui-sdl/aranym.fnt \
	gui-sdl/dlgAlert.cpp gui-sdl/dlgAlert.h \
	gui-sdl/dlgDisk.cpp gui-sdl/dlgDisk.h \
	gui-sdl/dlgFileSelect.cpp gui-sdl/dlgFileSelect.h \
	gui-sdl/dlgHostfs.cpp gui-sdl/dlgHostfs.h \
	gui-sdl/dlgHotkeys.cpp gui-sdl/dlgHotkeys.h \
	gui-sdl/dlgKeyboard.cpp gui-sdl/dlgKeyboard.h \
	gui-sdl/dlgKeypress.cpp gui-sdl/dlgKeypress.h \
	gui-sdl/dlgNetwork.cpp gui-sdl/dlgNetwork.h \
	gui-sdl/dlgOs.cpp gui-sdl/dlgOs.h \
	gui-sdl/dlgPartition.cpp gui-sdl/dlgPartition.h \
	gui-sdl/dlgUsb.cpp gui-sdl/dlgUsb.h \
	gui-sdl/dlgVideo.cpp gui-sdl/dlgVideo.h \
	$(empty)
endif

if NEED_GETOPT
aranym_SOURCES += Unix/getopt_init.c Unix/getopt1.c Unix/getopt.c Unix/getopt.h
endif
if USE_DEBUGGER
aranym_SOURCES += uae_cpu/debug.cpp
endif

if CPU_TYPE_x86
WIN32_RC_OBJS = aranym_rc_i386.o
else
if CPU_TYPE_x86_64
WIN32_RC_OBJS = aranym_rc_x86_64.o
else
endif
endif

if OS_TYPE_linux
aranym_SOURCES += midi_sequencer.cpp Unix/linux/parameters_linux.cpp Unix/linux/ethernet_linux.cpp Unix/linux/ethernet_linux.h Unix/host_filesys_unix.cpp
else
if OS_TYPE_cygwin
aranym_SOURCES += Unix/cygwin/parameters_cygwin.cpp Unix/cygwin/tools_cygwin.cpp Unix/cygwin/ethernet_cygwin.cpp Unix/cygwin/ethernet_cygwin.h Unix/cygwin/win32_supp.cpp Unix/win32_supp.h Unix/windows_ver.h
aranym_LDADD += $(WIN32_RC_OBJS)
else
if OS_TYPE_mingw
aranym_SOURCES += Unix/cygwin/parameters_cygwin.cpp Unix/cygwin/tools_cygwin.cpp Unix/cygwin/ethernet_cygwin.cpp Unix/cygwin/ethernet_cygwin.h Unix/cygwin/win32_supp.cpp Unix/win32_supp.h Unix/windows_ver.h
aranym_LDADD += $(WIN32_RC_OBJS)
else
if OS_TYPE_irix
aranym_SOURCES += Unix/parameters_unix.cpp Unix/irix/unaligned.c Unix/host_filesys_unix.cpp
else
if OS_TYPE_darwin
if WITH_FINK
aranym_SOURCES += \
	Unix/parameters_unix.cpp \
	Unix/host_filesys_unix.cpp \
	Unix/darwin/ethernet_darwin.cpp Unix/darwin/ethernet_darwin.h \
	Unix/darwin/SDLMain.M Unix/darwin/SDLMain.h \
	Unix/darwin/SDLHotkeysRefresh.M \
	$(empty)
else
aranym_SOURCES += \
	gui-osx/SDLMain.m gui-osx/SDLMain.h \
	Unix/MacOSX/parameters_MacOSX.cpp \
	Unix/darwin/ethernet_darwin.cpp Unix/darwin/ethernet_darwin.h \
	Unix/darwin/SDLHotkeysRefresh.M \
	Unix/MacOSX/fd_trans.c Unix/MacOSX/fd_trans.h \
	Unix/MacOSX/ethernet_macosx.cpp Unix/MacOSX/ethernet_macosx.h \
	Unix/darwin/MachServer.c \
	Unix/darwin/MachUser.c \
	$(empty)
endif
else
if OS_TYPE_beos
aranym_SOURCES += Unix/parameters_unix.cpp Unix/host_filesys_unix.cpp Unix/beos/cdrom_beos.cpp Unix/beos/cdrom_beos.h
else
aranym_SOURCES += Unix/parameters_unix.cpp Unix/host_filesys_unix.cpp
endif
endif
endif
endif
endif
endif


if USE_JIT
aranym_SOURCES += \
	uae_cpu/compiler/compemu.h \
	uae_cpu/compiler/flags_arm.h \
	uae_cpu/compiler/flags_x86.h \
	uae_cpu/compiler/compemu_support.cpp \
	$(empty)
libaranym_a_SOURCES += \
	uae_cpu/compiler/compstbla.cpp \
	uae_cpu/cpustbl_nf.cpp \
	uae_cpu/compiler/compemu1.cpp \
	uae_cpu/compiler/compemu2.cpp \
	uae_cpu/compiler/compemu3.cpp \
	uae_cpu/compiler/compemu4.cpp \
	uae_cpu/compiler/compemu5.cpp \
	uae_cpu/compiler/compemu6.cpp \
	uae_cpu/compiler/compemu7.cpp \
	uae_cpu/compiler/compemu8.cpp \
	uae_cpu/cpuemu1_nf.cpp \
	uae_cpu/cpuemu2_nf.cpp \
	uae_cpu/cpuemu3_nf.cpp \
	uae_cpu/cpuemu4_nf.cpp \
	uae_cpu/cpuemu5_nf.cpp \
	uae_cpu/cpuemu6_nf.cpp \
	uae_cpu/cpuemu7_nf.cpp \
	uae_cpu/cpuemu8_nf.cpp \
	$(empty)
if USE_JIT_FPU
aranym_SOURCES += uae_cpu/compiler/compemu_fpp.cpp
endif
endif


if FPU_IEEE
aranym_SOURCES += uae_cpu/fpu/fpu_ieee.cpp uae_cpu/fpu/fpu_ieee.h
endif
if FPU_X86
aranym_SOURCES += uae_cpu/fpu/fpu_x86.cpp uae_cpu/fpu/fpu_x86.h uae_cpu/fpu/fpu_x86_asm.h
endif
if FPU_UAE
aranym_SOURCES += uae_cpu/fpu/fpu_uae.cpp uae_cpu/fpu/fpu_uae.h
endif
if FPU_MPFR
aranym_SOURCES += uae_cpu/fpu/fpu_mpfr.cpp
endif


if NFCDROM_SUPPORT
aranym_SOURCES += natfeat/nfcdrom.cpp natfeat/nfcdrom.h natfeat/nfcdrom_atari.h
aranym_SOURCES += natfeat/nfcdrom_sdl.cpp natfeat/nfcdrom_sdl.h
if OS_TYPE_linux
aranym_SOURCES += natfeat/nfcdrom_linux.cpp natfeat/nfcdrom_linux.h
endif
if OS_TYPE_cygwin
aranym_SOURCES += natfeat/nfcdrom_win32.cpp natfeat/nfcdrom_win32.h
endif
if OS_TYPE_mingw
aranym_SOURCES += natfeat/nfcdrom_win32.cpp natfeat/nfcdrom_win32.h
endif
endif

if WANT_DSP
aranym_SOURCES += dsp_core.c dsp_cpu.c
if WANT_DSP_DISASM
aranym_SOURCES += dsp_disasm.c
endif
endif

if NFPCI_SUPPORT
aranym_SOURCES += natfeat/nfpci.cpp natfeat/nfpci.h
endif
if NFPCI_LINUX_SUPPORT
aranym_SOURCES += natfeat/nfpci_linux.cpp natfeat/nfpci_linux.h
endif

if ETHERNET_SUPPORT
aranym_SOURCES += natfeat/ethernet.cpp natfeat/ethernet.h
endif
if HOSTFS_SUPPORT
aranym_SOURCES += natfeat/hostfs.cpp natfeat/hostfs.h
endif

if NFCLIPBRD_SUPPORT
aranym_SOURCES += natfeat/nfclipbrd.cpp natfeat/nfclipbrd.h natfeat/nfclipbrd_nfapi.h
if OS_TYPE_cygwin
aranym_SOURCES += Unix/cygwin/clipbrd_cygwin.cpp
else
if OS_TYPE_mingw
aranym_SOURCES += Unix/cygwin/clipbrd_cygwin.cpp
else
if OS_TYPE_darwin
aranym_SOURCES += Unix/darwin/clipbrd_darwin.M
else
if OS_TYPE_beos
aranym_SOURCES += Unix/beos/clipbrd_beos.cpp
else
aranym_SOURCES += Unix/clipbrd_x11.cpp Unix/clipbrd.h
endif
endif
endif
endif
endif

if EXTENDED_SIGSEGV
if OS_TYPE_linux
if CPU_TYPE_x86
aranym_SOURCES += Unix/linux/sigsegv_linux_x86.cpp Unix/sigsegv_common_x86.h
endif
if CPU_TYPE_x86_64
aranym_SOURCES += Unix/linux/sigsegv_linux_x86.cpp Unix/sigsegv_common_x86.h
endif
if CPU_TYPE_arm
aranym_SOURCES += Unix/linux/sigsegv_linux_arm.cpp
endif
endif
if OS_TYPE_openbsd
if CPU_TYPE_x86
aranym_SOURCES += Unix/xbsd/sigsegv_xbsd.cpp
endif
if CPU_TYPE_x86_64
aranym_SOURCES += Unix/xbsd/sigsegv_xbsd.cpp
endif
endif
if OS_TYPE_netbsd
if CPU_TYPE_x86
aranym_SOURCES += Unix/xbsd/sigsegv_xbsd.cpp
endif
endif
if OS_TYPE_freebsd
if CPU_TYPE_x86
aranym_SOURCES += Unix/xbsd/sigsegv_xbsd.cpp
endif
if CPU_TYPE_x86_64
aranym_SOURCES += Unix/xbsd/sigsegv_xbsd.cpp
endif
endif
if OS_TYPE_darwin
if CPU_TYPE_x86
aranym_SOURCES += Unix/darwin/sigsegv_darwin_x86.cpp
endif
if CPU_TYPE_x86_64
aranym_SOURCES += Unix/darwin/sigsegv_darwin_x86.cpp
endif
endif
if OS_TYPE_cygwin
if CPU_TYPE_x86
aranym_SOURCES += Unix/cygwin/sigsegv_cygwin.cpp
endif
endif
if OS_TYPE_mingw
if CPU_TYPE_x86
aranym_SOURCES += Unix/cygwin/sigsegv_cygwin.cpp
endif
endif
if OS_TYPE_solaris
if CPU_TYPE_x86
aranym_SOURCES += Unix/solaris/sigsegv_solaris_x86.cpp
endif
if CPU_TYPE_x86_64
aranym_SOURCES += Unix/solaris/sigsegv_solaris_x86.cpp
endif
endif
if OS_TYPE_beos
if CPU_TYPE_x86
aranym_SOURCES += Unix/beos/sigsegv_beos_x86.cpp
endif
if CPU_TYPE_x86_64
aranym_SOURCES += Unix/beos/sigsegv_beos_x86.cpp
endif
endif
endif

if ENABLE_OPENGL
aranym_SOURCES += dyngl.c
endif

if USBHOST_SUPPORT
aranym_SOURCES += natfeat/usbhost.cpp natfeat/usbhost.h
endif

if NFOSMESA_SUPPORT
aranym_SOURCES += \
	natfeat/nfosmesa.cpp natfeat/nfosmesa.h \
	natfeat/osmesa_context.cpp natfeat/osmesa_context.h
endif

if NFJPEG_SUPPORT
aranym_SOURCES += natfeat/nfjpeg.cpp natfeat/nfjpeg.h
endif

if NFVDI_SUPPORT
aranym_SOURCES += natfeat/nfvdi.cpp natfeat/nfvdi.h natfeat/nfvdi_soft.cpp natfeat/nfvdi_soft.h natfeat/fvdidrv_nfapi.h
if ENABLE_OPENGL
aranym_SOURCES += natfeat/nfvdi_opengl.cpp natfeat/nfvdi_opengl.h
endif
endif

if ENABLE_PARALLELX86
if OS_TYPE_linux
if CPU_TYPE_x86
aranym_SOURCES += parallel_x86.cpp
endif
endif
endif

if ENABLE_PARALLELPARPORT
if OS_TYPE_linux
aranym_SOURCES += parallel_parport.cpp
endif
endif

if ENABLE_SERIALUNIX
aranym_SOURCES += serial_port.cpp
endif


aranym_rc_i386.o: $(srcdir)/Unix/cygwin/aranym.rc
	$(WINDRES) -o $@ --output-format coff --target pe-i386 $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $< 

aranym_rc_x86_64.o: $(srcdir)/Unix/cygwin/aranym.rc
	$(WINDRES) -o $@ --output-format coff --target pe-x86-64 $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $< 


aratapif_SOURCES = Unix/aratapif.c


# previously, the executables where built in the top level
# directory, and some external build scripts (e.g. rpm) excpect
# them there. Remove this copy once the scripts have been updated
aranym$(EXEEXT): $(aranym_OBJECTS) $(aranym_DEPENDENCIES) $(EXTRA_aranym_DEPENDENCIES) 
	@rm -f $@
	$(AM_V_CXXLD)$(CXXLINK) $(aranym_OBJECTS) $(aranym_LDADD) $(LIBS)
if OS_TYPE_beos
	$(BEOS_RC) -o $(APP).rsrc $(BEOS_RESOURCES)
	$(BEOS_XRES) -o $(APP) $(APP).rsrc
	versfile="$(srcdir)/include/version.h"; \
	major=`sed -n -e 's/#define[ \\t]*VER_MAJOR[ \\t]*\\([0-9]*\\)/\\1/p' $$versfile`; \
	minor=`sed -n -e 's/#define[ \\t]*VER_MINOR[ \\t]*\\([0-9]*\\)/\\1/p' $$versfile`; \
	micro=`sed -n -e 's/#define[ \\t]*VER_MICRO[ \\t]*\\([0-9]*\\)/\\1/p' $$versfile`; \
	$(BEOS_SETVERSION) $(APP) \
		-app $$major $$minor $$micro d 0 \
		-short "$(APP) $(PACKAGE_VERSION)" \
		-long "$(APP) $(PACKAGE_VERSION)"
endif
	rm -f ../$@
	cp $@ ../$@

# previously, the executables where built in the top level
# directory, and some external build scripts (e.g. rpm) excpect
# them there. Remove this copy once the scripts have been updated
aratapif$(EXEEXT): $(aratapif_OBJECTS) $(aratapif_DEPENDENCIES) $(EXTRA_aratapif_DEPENDENCIES) 
	@rm -f $@
	$(AM_V_CCLD)$(LINK) $(aratapif_OBJECTS) $(aratapif_LDADD) $(LIBS)
	rm -f ../$@
	cp $@ ../$@

install-exec-hook:
if USE_JIT
	cd "$(DESTDIR)$(bindir)" && mv aranym$(EXEEXT) aranym-jit$(EXEEXT)
endif
if FULLMMU
	cd "$(DESTDIR)$(bindir)" && mv aranym$(EXEEXT) aranym-mmu$(EXEEXT)
endif


$(APP_BUNDLE): $(APP)
	rm -rf $(APP_BUNDLE)
	mkdir -p $(APP_BUNDLE)/Contents
	cp -r $(BUNDLE_CONTENTS) $(APP_BUNDLE)/Contents
	mkdir -p $(APP_BUNDLE)/Contents/MacOS
	cp $(APP) $(APP_BUNDLE)/Contents/MacOS
	mkdir -p $(APP_BUNDLE)/Contents/Resources
	mkdir -p $(APP_BUNDLE)/Contents/Frameworks
	for fwFolder in $(BUNDLE_FRAMEWORKS); do \
	    if test -d $$fwFolder; then \
		cp -rp $$fwFolder $(APP_BUNDLE)/Contents/Frameworks/; \
	    fi; \
	done
	cp -r $(BUNDLE_RESOURCES) $(APP_BUNDLE)/Contents/Resources
	for locFolder in $(BUNDLE_LOCALIZATON_FOLDERS); do \
		cp -R $$locFolder $(APP_BUNDLE)/Contents/Resources/; \
	done
	find $(APP_BUNDLE)/Contents/Resources -type d -and \( -name CVS -or -name \*~.nib \) -or -name \*~ -or -name \*.bak | xargs rm -rf




EXTRA_DIST += \
	include \
	natfeat/doc \
	natfeat/atari_example.c \
	natfeat/atari_example_gfa.lst \
	natfeat/nfosmesa/call-gl.c \
	natfeat/nfosmesa/dispatch-gl.c \
	natfeat/nfosmesa/paramcount-gl.c \
	$(BEOS_RESOURCES) \
	Unix/solaris/which_sparc \
	Unix/cygwin/aranym.ico \
	Unix/cygwin/aranym.rc \
	Unix/MacOSX/MacAranym.xcodeproj \
	Unix/MacOSX/scripts \
	Unix/MacOSX/README.rtf \
	Unix/MacOSX/README_Xcode.rtf \
	Unix/MacOSX/README_ethernet.rtf \
	Unix/MacOSX/bpf_helper.c \
	gui-sdl/genmaptab.c \
	gui-sdl/fntconv.c \
	$(srcdir)/gui-osx/*.lproj \
	gui-osx/Info.plist \
	gui-osx/aranym.icns \
	Unix/darwin/aranym.xcodeproj \
	Unix/darwin/aranym.icns \
	Unix/darwin/aranym2.icns \
	Unix/darwin/busybee.icns \
	Unix/darwin/busybee112-glossy.xcf \
	Unix/darwin/aratapif.sh \
	$(srcdir)/Unix/darwin/*.lproj \
	Unix/darwin/Info.plist \
	Unix/darwin/BUILDING_LEOPARD.txt \
	Unix/darwin/README_ethernet.txt \
	disasm-main.cpp \
	cdromtest.cpp \
	$(empty)


# For tests
m68kdisasm: disasm-main.cpp disasm-glue.cpp disasm-builtin.cpp
	$(AM_V_CXXLD)$(CXX) $(LDFLAGS) $(AM_CPPFLAGS) $(CXXFLAGS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(DEFS) $(WFLAGS) $(CFLAGS) $(SDL_CFLAGS) $(ARCHFLAGS) -o $@ $(srcdir)/disasm-main.cpp $(DISASM_LIBS)

cdromtest: $(srcdir)/cdromtest.cpp $(srcdir)/natfeat/nfcdrom.cpp $(srcdir)/natfeat/nfcdrom_sdl.cpp $(srcdir)/natfeat/nfcdrom_linux.cpp $(srcdir)/natfeat/nfcdrom_win32.cpp
	$(AM_V_CC)$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(DEFS) $(WFLAGS) $(CFLAGS) $(ARCHFLAGS) -o $@ $(srcdir)/cdromtest.cpp $(LIBS)

CLEANFILES = cdromtest$(EXEEXT) m68kdisasm$(EXEEXT)


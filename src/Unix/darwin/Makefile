
CONFIGURE_OPTIONS_ppc  = --host=powerpc-apple-darwin8.10.0 --enable-fullmmu --enable-opengl --enable-fpe=uae --enable-nfjpeg --enable-nfosmesa --enable-ethernet=no --enable-vblupdates
CONFIGURE_OPTIONS_i386 = --host=i386-apple-darwin8.10.0 --enable-opengl --enable-jit-compiler --enable-nfjpeg --enable-nfosmesa --enable-ethernet=no --enable-vblupdates
ACLOCAL_FLAGS = "-I ./darwin/"

srcdir = ..

.PHONY: all clean clean_i386 clean_ppc touch


all: $(srcdir)/Makefile_i386 $(srcdir)/Makefile_ppc $(srcdir)/config.h
	$(MAKE) -f $(srcdir)/Makefile_i386 aranym_i386
	$(MAKE) -f $(srcdir)/Makefile_ppc aranym_powerpc
	#	create the universal binary from the ppc and i386 versions
	lipo -create aranym_powerpc aranym_i386 -output aranym
	#	now just build the bundle containing the just build universal binary
	make -f $(srcdir)/Makefile_ppc


clean: clean_i386 clean_ppc


clean_i386:
	if test -f $(srcdir)/Makefile_i386; then \
		$(MAKE) -f $(srcdir)/Makefile_i386 clean; \
		rm -f $(srcdir)/Makefile_i386; \
	fi
	rm -f $(srcdir)/configure_i386
	rm -f $(srcdir)/config_i386.h
	rm -f $(srcdir)/config.h


clean_ppc:
	if test -f $(srcdir)/Makefile_ppc; then \
		$(MAKE) -f $(srcdir)/Makefile_ppc clean; \
		rm -f $(srcdir)/Makefile_ppc; \
	fi
	rm -f $(srcdir)/configure_ppc
	rm -f $(srcdir)/config_ppc.h
	rm -f $(srcdir)/config.h


touch: 
	touch $(srcdir)/Makefile_i386 
	touch $(srcdir)/Makefile_ppc 


$(srcdir)/config.h: $(srcdir)/config_i386.h $(srcdir)/config_ppc.h
	echo "#if CPU_powerpc" > $(srcdir)/config.h
	cat $(srcdir)/config_ppc.h >> $(srcdir)/config.h
	echo "#elif CPU_i386" >> $(srcdir)/config.h
	cat $(srcdir)/config_i386.h >> $(srcdir)/config.h
	echo "#else" >> $(srcdir)/config.h
	echo "    #error Unsupported CPU type" >> $(srcdir)/config.h
	echo "#endif" >> $(srcdir)/config.h


$(srcdir)/config_i386.h: $(srcdir)/configure.ac $(srcdir)/autogen.sh $(srcdir)/acinclude.m4 sdl.m4 $(srcdir)/config.guess
	cd $(srcdir)/; NO_CONFIGURE=yes ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh;
	cd $(srcdir)/; ./configure $(CONFIGURE_OPTIONS_i386)
	make -f $(srcdir)/Makefile depend
	mv $(srcdir)/configure $(srcdir)/configure_i386
	mv $(srcdir)/config.log $(srcdir)/config_i386.log
	mv $(srcdir)/config.h $(srcdir)/config_i386.h


$(srcdir)/config_ppc.h: $(srcdir)/configure.ac $(srcdir)/autogen.sh $(srcdir)/acinclude.m4 sdl.m4 $(srcdir)/config.guess
	cd $(srcdir)/; NO_CONFIGURE=yes ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh;
	cd $(srcdir)/; ./configure $(CONFIGURE_OPTIONS_ppc)
	make -f $(srcdir)/Makefile depend
	mv $(srcdir)/configure $(srcdir)/configure_ppc
	mv $(srcdir)/config.log $(srcdir)/config_ppc.log
	# 10.3.9 compatibility:
	cat $(srcdir)/config.h | sed 's/#define HAVE_SYS_STATVFS_H 1/\/* #undef HAVE_SYS_STATVFS_H *\//' > $(srcdir)/config_ppc.h
	rm $(srcdir)/config.h
#	mv $(srcdir)/config.h $(srcdir)/config_ppc.h


$(srcdir)/Makefile_i386: $(srcdir)/config_i386.h $(srcdir)/Makefile.in
	mv $(srcdir)/Makefile $(srcdir)/Makefile_i386


$(srcdir)/Makefile_ppc: $(srcdir)/config_ppc.h $(srcdir)/Makefile.in
	mv $(srcdir)/Makefile $(srcdir)/Makefile_ppc



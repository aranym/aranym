
#	Define here the relative path from this file to the folder containing the autogen.sh script.
builddir = ../../..

#	Define here the relative path from the the folder containing the autogen.sh script to the folder containing the sdl.m4 file.
sdlm4dir = src/Unix/darwin

#	Configure the aranym ppc binary.
CONFIGURE_OPTIONS_ppc  = --host=powerpc-apple-darwin8.10.0 --enable-fullmmu --enable-opengl --enable-fpe=uae --enable-nfjpeg --enable-nfosmesa --enable-ethernet=yes --enable-vblupdates=yes

#	Configure the aranym x86 binary.
CONFIGURE_OPTIONS_i386 = --host=i386-apple-darwin8.10.0 --enable-opengl --enable-jit-compiler --enable-nfjpeg --enable-nfosmesa --enable-ethernet=yes --enable-vblupdates=yes

#	This flag is needed to by the autogen.sh script.
ACLOCAL_FLAGS = "-I $(sdlm4dir)/"


.PHONY: universal i386 ppc clean clean_i386 clean_ppc touch

	
#	The universal target builds the x86 executable, then the ppc executable and finally merges both into an universal binary which is then put into the application bundle.
universal: $(builddir)/Makefile_i386 $(builddir)/Makefile_ppc $(builddir)/config.h
	$(MAKE) -f $(builddir)/Makefile_i386 aranym_i386
	$(MAKE) -f $(builddir)/Makefile_ppc aranym_powerpc
	#	create the universal binary from the ppc and i386 versions
	lipo -create aranym_powerpc aranym_i386 -output aranym
	#	now just build the bundle containing the just build universal binary
	$(MAKE) -f $(builddir)/Makefile_ppc


#	The i386 target builds the x86 executable and puts it into the application bundle.
i386: $(builddir)/Makefile_i386
	cp $(builddir)/config_i386.h $(builddir)/config.h
	$(MAKE) -f $(builddir)/Makefile_i386 aranym_i386
	mv aranym_i386 aranym
	$(MAKE) -f $(builddir)/Makefile_i386


#	The ppc target builds the x86 executable and puts it into the application bundle.
ppc: $(builddir)/Makefile_ppc
	cp $(builddir)/config_ppc.h $(builddir)/config.h
	$(MAKE) -f $(builddir)/Makefile_ppc aranym_powerpc
	mv aranym_powerpc aranym
	$(MAKE) -f $(builddir)/Makefile_ppc


#	The clean target removes all build files.
clean: clean_i386 clean_ppc
	rm -f aranym


#	The clean_i386 target removes all build files related to the x86 binary and the application bundle.
clean_i386:
	if test -f $(builddir)/Makefile_i386; then \
		$(MAKE) -f $(builddir)/Makefile_i386 clean; \
		rm -f $(builddir)/Makefile_i386; \
	fi
	rm -f $(builddir)/configure_i386
	rm -f $(builddir)/config_i386.h
	rm -f $(builddir)/config.h


#	The clean_ppc target removes all build files related to the ppc binary and the application bundle.
clean_ppc:
	if test -f $(builddir)/Makefile_ppc; then \
		$(MAKE) -f $(builddir)/Makefile_ppc clean; \
		rm -f $(builddir)/Makefile_ppc; \
	fi
	rm -f $(builddir)/configure_ppc
	rm -f $(builddir)/config_ppc.h
	rm -f $(builddir)/config.h


#	This target touches both makefiles to avoid another configure run if something has been changed in the Makefile.in file.
touch: 
	touch $(builddir)/Makefile_i386 
	touch $(builddir)/Makefile_ppc 


#	After configuration for x86 and ppc binaries, this target combines both configurations into the config.h file. 
$(builddir)/config.h: $(builddir)/config_i386.h $(builddir)/config_ppc.h
	echo "#if CPU_powerpc" > $(builddir)/config.h
	cat $(builddir)/config_ppc.h >> $(builddir)/config.h
	echo "#elif CPU_i386" >> $(builddir)/config.h
	cat $(builddir)/config_i386.h >> $(builddir)/config.h
	echo "#else" >> $(builddir)/config.h
	echo "    #error Unsupported CPU type" >> $(builddir)/config.h
	echo "#endif" >> $(builddir)/config.h


#	This target calls autogen.sh, configure and make depend and renames the generic config* files as config*_i386* to avoid overwriting if configuration for the ppc binary is done.
$(builddir)/config_i386.h: $(builddir)/configure.ac $(builddir)/autogen.sh $(builddir)/acinclude.m4 sdl.m4 $(builddir)/config.guess
	cd $(builddir)/; NO_CONFIGURE=yes ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh;
	cd $(builddir)/; ./configure $(CONFIGURE_OPTIONS_i386)
	$(MAKE) -f $(builddir)/Makefile depend
	mv $(builddir)/configure $(builddir)/configure_i386
	mv $(builddir)/config.log $(builddir)/config_i386.log
	mv $(builddir)/config.h $(builddir)/config_i386.h


#	This target calls autogen.sh, configure and make depend and renames the generic config* files as config*_ppc* to avoid overwriting if configuration for the x86 binary is done.
$(builddir)/config_ppc.h: $(builddir)/configure.ac $(builddir)/autogen.sh $(builddir)/acinclude.m4 sdl.m4 $(builddir)/config.guess
	cd $(builddir)/; NO_CONFIGURE=yes ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh;
	cd $(builddir)/; ./configure $(CONFIGURE_OPTIONS_ppc)
	$(MAKE) -f $(builddir)/Makefile depend
	mv $(builddir)/configure $(builddir)/configure_ppc
	mv $(builddir)/config.log $(builddir)/config_ppc.log
	#mv $(builddir)/config.h $(builddir)/config_ppc.h
	# 10.3.9 compatibility:
	cat $(builddir)/config.h | sed 's/#define HAVE_SYS_STATVFS_H 1/\/* #undef HAVE_SYS_STATVFS_H *\//' > $(builddir)/config_ppc.h
	rm $(builddir)/config.h


#	After a configuration for x86 this target renames the created Makefile to Makefile_i386
$(builddir)/Makefile_i386: $(builddir)/config_i386.h $(builddir)/Makefile.in
	mv $(builddir)/Makefile $(builddir)/Makefile_i386
	touch $(builddir)/Makefile_i386


#	After a configuration for ppc this target renames the created Makefile to Makefile_ppc
$(builddir)/Makefile_ppc: $(builddir)/config_ppc.h $(builddir)/Makefile.in
	mv $(builddir)/Makefile $(builddir)/Makefile_ppc
	touch $(builddir)/Makefile_ppc



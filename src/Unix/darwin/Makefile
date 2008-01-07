
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
universal: $(builddir)/depend_i386 $(builddir)/depend_powerpc
	$(MAKE) -C $(builddir) -f Makefile_i386 aranym_i386
	$(MAKE) -C $(builddir) -f Makefile_ppc aranym_powerpc
	#	create the universal binary from the ppc and i386 versions
	lipo -create $(builddir)/aranym_powerpc $(builddir)/aranym_i386 -output $(builddir)/aranym
	#	now just build the bundle containing the just build universal binary
	$(MAKE) -C $(builddir) -f Makefile_ppc
	rm -rf ARAnyM.app
	mv $(builddir)/ARAnyM.app .

#	The i386 target builds the x86 executable and puts it into the application bundle.
i386: $(builddir)/depend_i386
	$(MAKE) -C $(builddir) -f Makefile_i386 aranym_i386
	cp -p $(builddir)/aranym_i386 $(builddir)/aranym
	$(MAKE) -C $(builddir) -f Makefile_i386
	rm -rf ARAnyM.app
	mv $(builddir)/ARAnyM.app .


#	The ppc target builds the powerpc executable and puts it into the application bundle.
ppc: $(builddir)/depend_powerpc
	$(MAKE) -C $(builddir) -f Makefile_ppc aranym_powerpc
	cp -p $(builddir)/aranym_powerpc $(builddir)/aranym
	$(MAKE) -C $(builddir) -f Makefile_ppc
	rm -rf ARAnyM.app
	mv $(builddir)/ARAnyM.app .


#	The depend target builds both dependencies for x86 and powerpc.
depend: depend_i386 depend_ppc


#	The depend_i386 target builds only the dependencies for x86.
depend_i386: $(builddir)/depend_i386


#	The depend_ppc target builds only the dependencies for powerpc.
depend_ppc: $(builddir)/depend_powerpc


#	The clean target removes all build files.
clean: clean_i386 clean_ppc


#	The clean_i386 target removes all build files related to the x86 binary and the application bundle.
clean_i386:
	if test -f $(builddir)/Makefile_i386; then \
		$(MAKE) -C $(builddir) -f Makefile_i386 clean; \
	fi


#	The clean_ppc target removes all build files related to the ppc binary and the application bundle.
clean_ppc:
	if test -f $(builddir)/Makefile_ppc; then \
		$(MAKE) -C $(builddir) -f Makefile_ppc clean; \
	fi


#	The clean target removes all build files.
distclean: distclean_i386 distclean_ppc


#	The clean_i386 target removes all build files related to the x86 binary and the application bundle.
distclean_i386:
	if test -f $(builddir)/Makefile_i386; then \
		$(MAKE) -C $(builddir) -f Makefile_i386 distclean; \
		rm -f $(builddir)/Makefile_i386; \
	fi
	rm -f $(builddir)/configure_i386
	rm -f $(builddir)/config_i386.h
	rm -f $(builddir)/config_tmp.h


#	The clean_ppc target removes all build files related to the ppc binary and the application bundle.
distclean_ppc:
	if test -f $(builddir)/Makefile_ppc; then \
		$(MAKE) -C $(builddir) -f Makefile_ppc distclean; \
		rm -f $(builddir)/Makefile_ppc; \
	fi
	rm -f $(builddir)/configure_ppc
	rm -f $(builddir)/config_ppc.h
	rm -f $(builddir)/config_tmp.h


#	This target touches both makefiles to avoid another configure run if something has been changed in the Makefile.in file.
touch: 
	touch $(builddir)/Makefile_i386 
	touch $(builddir)/Makefile_ppc 


#	This target combines both configurations into one config_tmp.h file. 
$(builddir)/config_tmp.h:
	echo "#ifndef CONFIG_H" > $@
	echo "    #define CONFIG_H" >> $@
	echo "    #if CPU_powerpc" >> $@
	echo "        #include \"config_ppc.h\"" >> $@
	echo "    #elif CPU_i386" >> $@
	echo "        #include \"config_i386.h\"" >> $@
	echo "    #else" >> $@
	echo "        #error Unsupported CPU type" >> $@
	echo "    #endif" >> $@
	echo "#endif" >> $@


#	This target calls autogen.sh, configure and make depend and renames the generic config* files as config*_i386* to avoid overwriting if configuration for the ppc binary is done.
$(builddir)/config_i386.h: $(builddir)/configure.ac $(builddir)/autogen.sh $(builddir)/acinclude.m4 sdl.m4 $(builddir)/config.guess
	cd $(builddir)/; NO_CONFIGURE=yes ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh;
	cd $(builddir)/; ./configure $(CONFIGURE_OPTIONS_i386)
	mv $(builddir)/configure $(builddir)/configure_i386
	mv $(builddir)/config.log $(builddir)/config_i386.log
	mv $(builddir)/config.h $(builddir)/config_i386.h


#	This target calls autogen.sh, configure and make depend and renames the generic config* files as config*_ppc* to avoid overwriting if configuration for the x86 binary is done.
$(builddir)/config_ppc.h: $(builddir)/configure.ac $(builddir)/autogen.sh $(builddir)/acinclude.m4 sdl.m4 $(builddir)/config.guess
	cd $(builddir)/; NO_CONFIGURE=yes ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh;
	cd $(builddir)/; ./configure $(CONFIGURE_OPTIONS_ppc)
	mv $(builddir)/configure $(builddir)/configure_ppc
	mv $(builddir)/config.log $(builddir)/config_ppc.log
	#mv $(builddir)/config.h $(builddir)/config_ppc.h
	# 10.3.9 compatibility:
	cat $(builddir)/config.h | sed 's/#define HAVE_SYS_STATVFS_H 1/\/* #undef HAVE_SYS_STATVFS_H *\//' > $(builddir)/config_ppc.h
	rm -f $(builddir)/config.h


#	This target calls make to buid the dependency file for x86.
$(builddir)/depend_i386: $(builddir)/Makefile_i386 $(builddir)/config_tmp.h 
	cp $(builddir)/config_tmp.h $(builddir)/config.h
	$(MAKE) -C $(builddir) -f Makefile_i386 depend


#	This target calls make to buid the dependency file for powerpc.
$(builddir)/depend_powerpc: $(builddir)/Makefile_ppc $(builddir)/config_tmp.h
	cp $(builddir)/config_tmp.h $(builddir)/config.h
	$(MAKE) -C $(builddir) -f Makefile_ppc depend


#	After a configuration for x86 this target renames the created Makefile to Makefile_i386
$(builddir)/Makefile_i386: $(builddir)/config_i386.h $(builddir)/Makefile.in
	mv $(builddir)/Makefile $(builddir)/Makefile_i386
	touch $(builddir)/Makefile_i386


#	After a configuration for ppc this target renames the created Makefile to Makefile_ppc
$(builddir)/Makefile_ppc: $(builddir)/config_ppc.h $(builddir)/Makefile.in
	mv $(builddir)/Makefile $(builddir)/Makefile_ppc
	touch $(builddir)/Makefile_ppc



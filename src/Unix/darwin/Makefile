
CONFIGURE_OPTIONS_ppc  = --host=powerpc-apple-darwin8.10.0 --enable-fullmmu --enable-opengl --enable-fpe=uae --enable-nfjpeg --enable-nfosmesa --enable-ethernet=no
CONFIGURE_OPTIONS_i386 = --host=i386-apple-darwin8.10.0 --enable-opengl --enable-jit-compiler --enable-nfjpeg --enable-nfosmesa --enable-ethernet=no
ACLOCAL_FLAGS = "-I ./darwin/"

UNIX_PATH = ..

.PHONY: all clean clean_i386 clean_ppc touch


all: ../Makefile_i386 ../Makefile_ppc ../config.h
	cd ..; $(MAKE) -f Makefile_i386 aranym_i386
	cd ..; $(MAKE) -f Makefile_ppc aranym_powerpc
	#	create the universal binary from the ppc and i386 versions
	cd ..; lipo -create aranym_powerpc aranym_i386 -output aranym
	#	now just build the bundle containing the just build universal binary
	cd ..; make -f Makefile_ppc


clean: clean_i386 clean_ppc


clean_i386:
	if test -f ../Makefile_i386; then \
		cd ..; $(MAKE) -f Makefile_i386 clean; \
		rm -f Makefile_i386; \
	fi
	rm -f ../configure_i386
	rm -f ../config_i386.h
	rm -f ../config.h


clean_ppc:
	if test -f ../Makefile_ppc; then \
		cd ..; $(MAKE) -f Makefile_ppc clean; \
		rm -f Makefile_ppc; \
	fi
	rm -f ../configure_ppc
	rm -f ../config_ppc.h
	rm -f ../config.h


touch: 
	touch ../Makefile_i386 
	touch ../Makefile_ppc 


../config.h: ../config_i386.h ../config_ppc.h
	echo "#if CPU_powerpc" > ../config.h
	cat ../config_ppc.h >> ../config.h
	echo "#elif CPU_i386" >> ../config.h
	cat ../config_i386.h >> ../config.h
	echo "#else" >> ../config.h
	echo "    #error Unsupported CPU type" >> ../config.h
	echo "#endif" >> ../config.h


../config_i386.h: ../configure.ac ../autogen.sh ../acinclude.m4 sdl.m4 ../config.guess ../config.h.in
	cd ..; ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh $(CONFIGURE_OPTIONS_i386)
	mv ../configure ../configure_i386
	mv ../config.log ../config_i386.log
	mv ../config.h ../config_i386.h


../config_ppc.h: ../configure.ac ../autogen.sh ../acinclude.m4 sdl.m4 ../config.guess ../config.h.in
	cd ..; ACLOCAL_FLAGS=$(ACLOCAL_FLAGS) ./autogen.sh $(CONFIGURE_OPTIONS_ppc)
	mv ../configure ../configure_ppc
	mv ../config.log ../config_ppc.log
	mv ../config.h ../config_ppc.h


../Makefile_i386: ../config_i386.h ../Makefile.in
	mv ../Makefile ../Makefile_i386


../Makefile_ppc: ../config_ppc.h ../Makefile.in
	mv ../Makefile ../Makefile_ppc



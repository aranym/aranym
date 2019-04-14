ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign

pkgdatadir = @datadir@/aranym
man1dir = $(mandir)/man1

# Unix makefile for ARAnyM
# Inspired in BasiliskII's Makefile

SUBDIRS = src doc

## Rules

.PHONY: depend
SUFFIXES = .c .cpp .s .o .h .m .M

BUILT_SOURCES = \
	version_date.h \
	$(top_srcdir)/src/natfeat/nfosmesa/call-gl.c \
	$(top_srcdir)/src/natfeat/nfosmesa/dispatch-gl.c \
	$(top_srcdir)/src/natfeat/nfosmesa/paramcount-gl.c \
	$(top_srcdir)/atari/nfosmesa/glfuncs.h \
	$(top_srcdir)/atari/nfosmesa/glfuncs-bynum.h \
	$(top_srcdir)/atari/nfosmesa/ldg/osmesa.h \
	$(top_srcdir)/atari/nfosmesa/ldg/tiny_gl.h \
	$(top_srcdir)/atari/nfosmesa/osmesa_load.c \
	$(top_srcdir)/atari/nfosmesa/tinygl_load.c \
	$(top_srcdir)/atari/nfosmesa/link-tinygl.h \
	$(top_srcdir)/atari/nfosmesa/slb/tiny_gl.h \
	$(top_srcdir)/atari/nfosmesa/slb/osmesa.h \
	$(top_srcdir)/atari/nfosmesa/tinygl_loadslb.c \
	$(top_srcdir)/atari/nfosmesa/osmesa_loadslb.c \
	$(empty)

version_date.h: $(top_srcdir)/ChangeLog
	echo '/* generated from ChangeLog */' > $@
	sed -n '/^20[0-9][0-9]\/[0-9]*\/[0-9]/p' < $(top_srcdir)/ChangeLog | sed 's/^\(20[0-9][0-9]\/[0-9]*\/[0-9]*\).*/#define VERSION_DATE "\1"/;q' >> $@

built_sources: $(BUILT_SOURCES)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(bindir) $(DESTDIR)$(man1dir) $(DESTDIR)$(pkgdatadir) $(DESTDIR)$(pkgdatadir)/atari $(DESTDIR)$(pkgdatadir)/atari/hostfs $(DESTDIR)$(pkgdatadir)/atari/nfcdrom $(DESTDIR)$(pkgdatadir)/atari/nfjpeg $(DESTDIR)$(pkgdatadir)/atari/nfosmesa $(DESTDIR)$(pkgdatadir)/atari/network $(DESTDIR)$(pkgdatadir)/atari/tools $(DESTDIR)$(pkgdatadir)/atari/sound $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(top_srcdir)/ChangeLog $(DESTDIR)$(docdir)/changelog
	$(INSTALL_DATA) $(top_srcdir)/TODO $(DESTDIR)$(docdir)/TODO
	$(INSTALL_DATA) $(top_srcdir)/AUTHORS $(DESTDIR)$(docdir)/AUTHORS
	$(INSTALL_DATA) $(top_srcdir)/NEWS $(DESTDIR)$(docdir)/NEWS
	$(INSTALL_DATA) $(top_srcdir)/README.md $(DESTDIR)$(docdir)/README
	$(INSTALL_DATA) $(top_srcdir)/FAQ $(DESTDIR)$(docdir)/FAQ
	$(INSTALL_DATA) $(top_srcdir)/COPYING $(DESTDIR)$(docdir)/COPYING
	$(INSTALL_DATA) $(top_srcdir)/BUGS $(DESTDIR)$(docdir)/BUGS
	$(INSTALL_DATA) $(top_srcdir)/INSTALL $(DESTDIR)$(docdir)/INSTALL
	$(INSTALL_DATA) $(top_srcdir)/doc/config $(DESTDIR)$(docdir)/config
	$(INSTALL_DATA) $(top_srcdir)/atari/newdesk.inf $(DESTDIR)$(pkgdatadir)/atari/newdesk.inf
	$(INSTALL_DATA) $(top_srcdir)/atari/mmusetup.cnf $(DESTDIR)$(pkgdatadir)/atari/mmusetup.cnf
	$(INSTALL_DATA) $(top_srcdir)/atari/hostfs/bdconfig.sys $(DESTDIR)$(pkgdatadir)/atari/hostfs/bdconfig.sys
	$(INSTALL_DATA) $(top_srcdir)/atari/hostfs/README $(DESTDIR)$(pkgdatadir)/atari/hostfs/README
	$(INSTALL_DATA) $(top_srcdir)/atari/nfosmesa/README $(DESTDIR)$(pkgdatadir)/atari/nfosmesa/README
	$(INSTALL_DATA) $(top_srcdir)/atari/network/ethernet/nfeth-config/eth0-config.sh $(DESTDIR)$(pkgdatadir)/atari/network/eth0-config.sh
	$(INSTALL_DATA) $(top_srcdir)/atari/network/ethernet/README $(DESTDIR)$(pkgdatadir)/atari/network/README
	$(INSTALL) $(top_srcdir)/tools/arabridge $(DESTDIR)$(pkgdatadir)/arabridge
	$(INSTALL) $(top_srcdir)/tools/floppy.sh $(DESTDIR)$(pkgdatadir)/floppy.sh
	$(INSTALL_DATA) $(top_srcdir)/atari/tools/README $(DESTDIR)$(pkgdatadir)/atari/tools/README
	$(INSTALL_DATA) $(top_srcdir)/atari/tools/pc101us.kbd.bz2 $(DESTDIR)$(pkgdatadir)/atari/tools/pc101us.kbd.bz2
	$(INSTALL_DATA) $(top_srcdir)/atari/sound/README $(DESTDIR)$(pkgdatadir)/atari/sound/README
	$(INSTALL_DATA) $(top_srcdir)/data/wm_icon.bmp $(DESTDIR)$(pkgdatadir)/wm_icon.bmp
	$(INSTALL_DATA) $(top_srcdir)/data/logo.bmp $(DESTDIR)$(pkgdatadir)/logo.bmp
	for s in 32 48; do \
		$(mkinstalldirs) "${DESTDIR}${datadir}/icons/hicolor/$${s}x$${s}/apps/"; \
		$(INSTALL_DATA) $(top_srcdir)/contrib/icon-$${s}.png "${DESTDIR}${datadir}/icons/hicolor/$${s}x$${s}/apps/aranym.png"; \
		$(INSTALL_DATA) $(top_srcdir)/contrib/icon-$${s}.png "${DESTDIR}${datadir}/icons/hicolor/$${s}x$${s}/apps/aranym-jit.png"; \
		$(INSTALL_DATA) $(top_srcdir)/contrib/icon-$${s}.png "${DESTDIR}${datadir}/icons/hicolor/$${s}x$${s}/apps/aranym-mmu.png"; \
	done
	$(mkinstalldirs) "${DESTDIR}${datadir}/pixmaps"
	rm -f "${DESTDIR}${datadir}/pixmaps/aranym"*.png
	ln -s ../icons/hicolor/32x32/apps/aranym.png "${DESTDIR}${datadir}/pixmaps/aranym.png"
	ln -s ../icons/hicolor/32x32/apps/aranym.png "${DESTDIR}${datadir}/pixmaps/aranym-jit.png"
	ln -s ../icons/hicolor/32x32/apps/aranym.png "${DESTDIR}${datadir}/pixmaps/aranym-mmu.png"
	$(mkinstalldirs) "${DESTDIR}${datadir}/applications"
	for name in aranym aranym-jit aranym-mmu; do \
		$(INSTALL_DATA) "$(top_srcdir)/contrib/$${name}.desktop" "${DESTDIR}${datadir}/applications/$${name}.desktop"; \
	done

uninstall-local:
	rm -f $(DESTDIR)$(docdir)/changelog
	rm -f $(DESTDIR)$(docdir)/TODO
	rm -f $(DESTDIR)$(docdir)/AUTHORS
	rm -f $(DESTDIR)$(docdir)/NEWS
	rm -f $(DESTDIR)$(docdir)/README
	rm -f $(DESTDIR)$(docdir)/FAQ
	rm -f $(DESTDIR)$(docdir)/COPYING
	rm -f $(DESTDIR)$(docdir)/BUGS
	rm -f $(DESTDIR)$(docdir)/INSTALL
	rm -f $(DESTDIR)$(docdir)/config
	rm -f $(DESTDIR)$(pkgdatadir)/atari/newdesk.inf
	rm -f $(DESTDIR)$(pkgdatadir)/atari/mmusetup.cnf
	rm -f $(DESTDIR)$(pkgdatadir)/atari/hostfs/bdconfig.sys
	rm -f $(DESTDIR)$(pkgdatadir)/atari/hostfs/README
	rm -f $(DESTDIR)$(pkgdatadir)/atari/nfosmesa/README
	rm -f $(DESTDIR)$(pkgdatadir)/atari/network/eth0-config.sh
	rm -f $(DESTDIR)$(pkgdatadir)/atari/network/README
	rm -f $(DESTDIR)$(pkgdatadir)/arabridge
	rm -f $(DESTDIR)$(pkgdatadir)/floppy.sh
	rm -f $(DESTDIR)$(pkgdatadir)/atari/tools/README
	rm -f $(DESTDIR)$(pkgdatadir)/atari/tools/pc101us.kbd.bz2
	rm -f $(DESTDIR)$(pkgdatadir)/atari/sound/README
	rm -f $(DESTDIR)$(pkgdatadir)/wm_icon.bmp
	rm -f $(DESTDIR)$(pkgdatadir)/logo.bmp
	rm -f $(DESTDIR)$(pkgdatadir)/logo.png
	rm -f $(DESTDIR)$(pkgdatadir)/logo.png
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/*/apps/aranym*.png
	rm -f $(DESTDIR)$(datadir)/applications/aranym*.desktop
	rm -f $(DESTDIR)$(datadir)/pixmaps/aranym*.png
	-rmdir $(DESTDIR)$(datadir)/icons/hicolor/*/apps
	-rmdir $(DESTDIR)$(datadir)/icons/hicolor/*
	-rmdir $(DESTDIR)$(datadir)/icons/hicolor
	-rmdir $(DESTDIR)$(datadir)/icons
	-rmdir $(DESTDIR)$(datadir)/pixmaps
	rmdir $(DESTDIR)$(pkgdatadir)/atari/hostfs
	rmdir $(DESTDIR)$(pkgdatadir)/atari/nfcdrom
	rmdir $(DESTDIR)$(pkgdatadir)/atari/nfjpeg
	rmdir $(DESTDIR)$(pkgdatadir)/atari/nfosmesa
	rmdir $(DESTDIR)$(pkgdatadir)/atari/network
	rmdir $(DESTDIR)$(pkgdatadir)/atari/tools
	rmdir $(DESTDIR)$(pkgdatadir)/atari/sound
	rmdir $(DESTDIR)$(pkgdatadir)/atari
	-rmdir $(DESTDIR)$(docdir)
	-rmdir $(DESTDIR)$(man1dir)

DISTCLEANFILES = config.cache config.log config.status config.h version_date.h appimage/appimagetool

# remove the excutables copied from the src/Makefile.
# previously, the executables where built in the top level
# directory, and some external build scripts (e.g. rpm) excpect
# them there. Remove this line once the scripts have been updated
CLEANFILES = aranym$(EXEEXT) aratapif$(EXEEXT)

mrproper: distclean
	rm -f $(top_srcdir)/aclocal.m4 $(builddir)/configure $(top_srcdir)/config.h.in
	rm -rf $(top_srcdir)/autom4te.cache

bindist: all
	rm -rf tmpdist
	$(MAKE) DESTDIR=tmpdist install
	chmod -R a+r tmpdist
	cd tmpdist && GZIP=--best tar chozf ../$(PACKAGE_TARNAME).tar.gz *
	rm -rf tmpdist

appimage::
	rm -rf tmpdist
	test "$(srcdir)" = "." || test -d appimage || mkdir appimage
	unset CDPATH && \
	mkdir tmpdist && \
	DESTDIR=`(cd tmpdist; pwd)` && \
	$(MAKE) DESTDIR="$$DESTDIR" install-strip && \
	cd appimage && \
	CPU_TYPE="$(CPU_TYPE)" \
	VERSION="$(VERSION)" \
	top_srcdir="$(abs_top_srcdir)" \
	BUILDROOT="$$DESTDIR" \
	$(SHELL) "$(abs_top_srcdir)/appimage/build.sh"
	rm -rf tmpdist
	
devdist: mrproper 
	cd $(srcdir)/../ && GZIP=--best tar chozf $(PACKAGE_TARNAME).tar.gz aranym

# not really needed anymore, but *.spec files might do a "make depend"
depend:

#
# check/update autogenerated OpenGL source files needed for NFOSMESA_SUPPORT
#
DYNGL_PL = $(top_srcdir)/tools/dyngl.pl
GLTYPES_H = -incfile $(top_srcdir)/atari/nfosmesa/gltypes.h
GLENUMS = -enums $(top_srcdir)/atari/nfosmesa/enum-gl.h

$(top_srcdir)/atari/nfosmesa/glfuncs.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) -macros $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/glfuncs-bynum.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLENUMS) -macros $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/src/natfeat/nfosmesa/call-gl.c: $(top_srcdir)/atari/nfosmesa/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) -calls $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/src/natfeat/nfosmesa/dispatch-gl.c: $(top_srcdir)/atari/nfosmesa/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) -dispatch $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/src/natfeat/nfosmesa/paramcount-gl.c: $(top_srcdir)/atari/nfosmesa/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLENUMS) -paramcount $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/ldg/osmesa.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL) $(top_srcdir)/atari/nfosmesa/gltypes.h
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLTYPES_H) $(GLENUMS) -ldgheader $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/osmesa_load.c: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLTYPES_H) $(GLENUMS) -ldgsource $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/ldg/tiny_gl.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL) $(top_srcdir)/atari/nfosmesa/gltypes.h
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLTYPES_H) -tinyldgheader $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/tinygl_load.c: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) -tinyldgsource $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/link-tinygl.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) -tinyldglink $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/slb/tiny_gl.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL) $(top_srcdir)/atari/nfosmesa/gltypes.h
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLTYPES_H) -tinyslbheader $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/slb/osmesa.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL) $(top_srcdir)/atari/nfosmesa/gltypes.h
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLTYPES_H) $(GLENUMS) -slbheader $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/tinygl_loadslb.c: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLENUMS) -tinyslbsource $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/osmesa_loadslb.c: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLENUMS) -slbsource $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

# For tests
gldiff:
	$(PERL) $(DYNGL_PL) -protos /usr/include/GL/gl.h /usr/include/GL/glext.h /usr/include/GL/osmesa.h | diff $(top_srcdir)/tools/glfuncs.h - || true
	$(PERL) $(DYNGL_PL) -macros $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/glfuncs.h - || true
	$(PERL) $(DYNGL_PL) -calls $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/src/natfeat/nfosmesa/call-gl.c - || true
	$(PERL) $(DYNGL_PL) -dispatch $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/src/natfeat/nfosmesa/dispatch-gl.c - || true
	$(PERL) $(DYNGL_PL) $(GLENUMS) -ldgheader $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/ldg/osmesa.h - || true
	$(PERL) $(DYNGL_PL) $(GLENUMS) -ldgsource $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/osmesa_load.c - || true
	$(PERL) $(DYNGL_PL) -tinyldgheader $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/ldg/tiny_gl.h - || true
	$(PERL) $(DYNGL_PL) -tinyldgsource $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/tinygl_load.c - || true
	$(PERL) $(DYNGL_PL) -tinyldglink $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/link-tinygl.h - || true
	$(PERL) $(DYNGL_PL) -tinyslbheader $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/slb/tiny_gl.h - || true
	$(PERL) $(DYNGL_PL) $(GLENUMS) -tinyslbsource $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/tinygl_loadslb.c - || true

EXTRA_DIST = \
	BUGS FAQ README.md README-cygwin.in \
	aranym.spec autogen.sh \
	android atari contrib data tools \
	appimage/build.sh \
	appimage/desktopintegration \
	appimage/excludelist \
	appimage/functions.sh

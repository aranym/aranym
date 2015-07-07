ACLOCAL_AMFLAGS = -I m4

datadir = @datadir@/aranym

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

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(bindir) $(DESTDIR)$(man1dir) $(DESTDIR)$(datadir) $(DESTDIR)$(datadir)/atari $(DESTDIR)$(datadir)/atari/hostfs $(DESTDIR)$(datadir)/atari/nfcdrom $(DESTDIR)$(datadir)/atari/nfjpeg $(DESTDIR)$(datadir)/atari/nfosmesa $(DESTDIR)$(datadir)/atari/network $(DESTDIR)$(datadir)/atari/tools $(DESTDIR)$(datadir)/atari/sound $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(top_srcdir)/ChangeLog $(DESTDIR)$(docdir)/changelog
	$(INSTALL_DATA) $(top_srcdir)/TODO $(DESTDIR)$(docdir)/TODO
	$(INSTALL_DATA) $(top_srcdir)/AUTHORS $(DESTDIR)$(docdir)/AUTHORS
	$(INSTALL_DATA) $(top_srcdir)/NEWS $(DESTDIR)$(docdir)/NEWS
	$(INSTALL_DATA) $(top_srcdir)/README $(DESTDIR)$(docdir)/README
	$(INSTALL_DATA) $(top_srcdir)/FAQ $(DESTDIR)$(docdir)/FAQ
	$(INSTALL_DATA) $(top_srcdir)/COPYING $(DESTDIR)$(docdir)/COPYING
	$(INSTALL_DATA) $(top_srcdir)/BUGS $(DESTDIR)$(docdir)/BUGS
	$(INSTALL_DATA) $(top_srcdir)/INSTALL $(DESTDIR)$(docdir)/INSTALL
	$(INSTALL_DATA) $(top_srcdir)/doc/config $(DESTDIR)$(docdir)/config
	$(INSTALL_DATA) $(top_srcdir)/atari/newdesk.inf $(DESTDIR)$(datadir)/atari/newdesk.inf
	$(INSTALL_DATA) $(top_srcdir)/atari/mmusetup.cnf $(DESTDIR)$(datadir)/atari/mmusetup.cnf
	$(INSTALL_DATA) $(top_srcdir)/atari/hostfs/bdconfig.sys $(DESTDIR)$(datadir)/atari/hostfs/bdconfig.sys
	$(INSTALL_DATA) $(top_srcdir)/atari/hostfs/README $(DESTDIR)$(datadir)/atari/hostfs/README
	$(INSTALL_DATA) $(top_srcdir)/atari/nfosmesa/README $(DESTDIR)$(datadir)/atari/nfosmesa/README
	$(INSTALL_DATA) $(top_srcdir)/atari/network/ethernet/nfeth-config/eth0-config.sh $(DESTDIR)$(datadir)/atari/network/eth0-config.sh
	$(INSTALL_DATA) $(top_srcdir)/atari/network/ethernet/README $(DESTDIR)$(datadir)/atari/network/README
	$(INSTALL) $(top_srcdir)/tools/arabridge $(DESTDIR)$(datadir)/arabridge
	$(INSTALL) $(top_srcdir)/tools/floppy.sh $(DESTDIR)$(datadir)/floppy.sh
	$(INSTALL_DATA) $(top_srcdir)/atari/tools/README $(DESTDIR)$(datadir)/atari/tools/README
	$(INSTALL_DATA) $(top_srcdir)/atari/tools/pc101us.kbd.bz2 $(DESTDIR)$(datadir)/atari/tools/pc101us.kbd.bz2
	$(INSTALL_DATA) $(top_srcdir)/atari/sound/README $(DESTDIR)$(datadir)/atari/sound/README
	$(INSTALL_DATA) $(top_srcdir)/data/wm_icon.bmp $(DESTDIR)$(datadir)/wm_icon.bmp
	$(INSTALL_DATA) $(top_srcdir)/data/logo.png $(DESTDIR)$(datadir)/logo.png

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
	rm -f $(DESTDIR)$(datadir)/atari/newdesk.inf
	rm -f $(DESTDIR)$(datadir)/atari/mmusetup.cnf
	rm -f $(DESTDIR)$(datadir)/atari/hostfs/bdconfig.sys
	rm -f $(DESTDIR)$(datadir)/atari/hostfs/README
	rm -f $(DESTDIR)$(datadir)/atari/nfosmesa/README
	rm -f $(DESTDIR)$(datadir)/atari/network/eth0-config.sh
	rm -f $(DESTDIR)$(datadir)/atari/network/README
	rm -f $(DESTDIR)$(datadir)/arabridge
	rm -f $(DESTDIR)$(datadir)/floppy.sh
	rm -f $(DESTDIR)$(datadir)/atari/tools/README
	rm -f $(DESTDIR)$(datadir)/atari/tools/pc101us.kbd.bz2
	rm -f $(DESTDIR)$(datadir)/atari/sound/README
	rm -f $(DESTDIR)$(datadir)/wm_icon.bmp
	rm -f $(DESTDIR)$(datadir)/logo.png
	rmdir $(DESTDIR)$(datadir)/atari/hostfs
	rmdir $(DESTDIR)$(datadir)/atari/nfcdrom
	rmdir $(DESTDIR)$(datadir)/atari/nfjpeg
	rmdir $(DESTDIR)$(datadir)/atari/nfosmesa
	rmdir $(DESTDIR)$(datadir)/atari/network
	rmdir $(DESTDIR)$(datadir)/atari/tools
	rmdir $(DESTDIR)$(datadir)/atari/sound
	rmdir $(DESTDIR)$(datadir)/atari
	rmdir $(DESTDIR)$(datadir)
	-rmdir $(DESTDIR)$(docdir)
	-rmdir $(DESTDIR)$(man1dir)

DISTCLEANFILES = config.cache config.log config.status config.h version_date.h

# remove the excutables copied from the src/Makefile.
# previously, the executables where built in the top level
# directory, and some external build scripts (e.g. rpm) excpect
# them there. Remove this line once the scripts have been updated
CLEANFILES = aranym$(EXEEXT) aratapif$(EXEEXT)

mrproper: distclean
	rm -f $(top_srcdir)/aclocal.m4 $(builddir)/configure $(top_srcdir)/config.h.in
	rm -rf $(top_srcdir)/autom4te.cache

bindist: all
	$(MAKE) DESTDIR=tmpdist install
	chmod -R a+r tmpdist
	cd tmpdist && GZIP=--best tar chozf ../$(PACKAGE_TARNAME).tar.gz *
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

$(top_srcdir)/atari/nfosmesa/ldg/osmesa.h: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL) $(top_srcdir)/atari/nfosmesa/gltypes.h
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLTYPES_H) -ldgheader $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
	fi

$(top_srcdir)/atari/nfosmesa/osmesa_load.c: $(top_srcdir)/tools/glfuncs.h $(DYNGL_PL)
	$(AM_V_GEN)if test -z "$(PERL)"; then \
		echo "$@ is out of date, but PERL not available" >&2; \
	else \
		$(PERL) $(DYNGL_PL) $(GLTYPES_H) -ldgsource $(top_srcdir)/tools/glfuncs.h > $$$$.tmp && mv $$$$.tmp $@ || { rm -f $$$$.tmp; false; } \
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
	$(PERL) $(DYNGL_PL) -ldgheader $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/ldg/osmesa.h - || true
	$(PERL) $(DYNGL_PL) -ldgsource $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/osmesa_load.c - || true
	$(PERL) $(DYNGL_PL) -tinyldgheader $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/ldg/tiny_gl.h - || true
	$(PERL) $(DYNGL_PL) -tinyldgsource $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/tinygl_load.c - || true
	$(PERL) $(DYNGL_PL) -tinyldglink $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/link-tinygl.h - || true
	$(PERL) $(DYNGL_PL) -tinyslbheader $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/slb/tiny_gl.h - || true
	$(PERL) $(DYNGL_PL) -tinyslbsource $(top_srcdir)/tools/glfuncs.h | diff $(top_srcdir)/atari/nfosmesa/tinygl_loadslb.c - || true

EXTRA_DIST = \
	BUGS FAQ README-cygwin.in \
	aranym.spec autogen.sh \
	android atari contrib data tools

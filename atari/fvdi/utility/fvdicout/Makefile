#
# Makefile for fvdicout.app
# 
#    - You have to copy all files of this directory into the FreeMiNT source
#      tree at "tools/toswin2/fvdicout". Instead of you may change top_srcdir at this
#      Makefile to "tools" directory of the MiNT source tree.
#      "make" does only work, if you do so!
#
# Note: You need a crosscompiler to make fvdicout.app or a native m68k-atari-mint
#       gcc, make etc.
#       To use crosscompiling change the variables at CONFIGVARS.
#       To crosscompile even FreeMiNT, you may have to change CRLF at 
#       CONFIGVARS (e.g. say "CRLF = echo crlf"). There may be additional 
#       errors, because of using "flags" at some sub-Makefiles. Comment out
#       "flags"-calls at those Makefiles
#       I need the FreeMiNT CVS tree of the version at least 1.16.0.
#
# STan (c) 2002
#
TARGET = fvdicout.app

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = fvdicout

installdir = /opt/GEM/toswin2

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

strip: $(TARGET)
	$(STRIP) $(TARGET)

all-here: $(TARGET)

# default overwrites
INCLUDES += -I.. -I/usr/GEM/include

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -L/usr/GEM/lib -lcflib -lgem 
LIBS += -liio
GENFILES = $(TARGET)
CFLAGS += -O2

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES

install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)
	

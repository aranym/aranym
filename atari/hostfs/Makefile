#
# Makefile for hostfs.xfs
# 
# If you got these sources as part of STonX note:
#    - You have to copy all files of this directory into the FreeMiNT source
#      tree at "sys/xfs/hostfs". Instead of you may change top_srcdir at this
#      Makefile to "sys" directory of the MiNT source tree.
#      "make" does only work, if you do so!
#
# Note: You need a crosscompiler to make hostfs.xfs or a native m68k-atari-mint
#       gcc, make etc.
#       To use crosscompiling change the variables at CONFIGVARS.
#       To crosscompile even FreeMiNT, you may have to change CRLF at 
#       CONFIGVARS (e.g. say "CRLF = echo crlf"). There may be additional 
#       errors, because of using "flags" at some sub-Makefiles. Comment out
#       "flags"-calls at those Makefiles (resulting programms will only work
#       if you're running at least version 1.15.10 of FreeMiNT.
#
TARGET = hostfs.xfs

GENFILES = $(TARGET) .natfeat
SHELL = /bin/sh
SUBDIRS = metados

srcdir = .
top_srcdir = ../..
subdir = 

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

debug:
	$(MAKE) DEBUG_INFO=-DDEBUG_INFO 

# default overwrites
INCLUDES = -I$(top_srcdir) -I.natfeat
DEFINITIONS =
LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 030
CFLAGS += -D__KERNEL_XFS__


# default definitions
OBJS = $(COBJS:.c=.o)

.natfeat: $(srcdir)/../natfeat
	$(MKDIR) -p $@
	$(CP) -r $</* $@
	cd $@; $(MAKE) CPU=$(CPU) DEBUG_INFO=$(DEBUG_INFO) parentdir=..

$(TARGET): .natfeat $(OBJS) $(LIBKERNTARGET) 
	$(LD) $(CFLAGS) -o $@ $(OBJS) .natfeat/*.o $(LIBS)
	$(STRIP) $@

include $(top_srcdir)/DEPENDENCIES

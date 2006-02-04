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
TARGET = hostfs.dos
GENFILES = $(TARGET) .xfso .natfeat

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = metados

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

debug:
	$(MAKE) DEBUG_INFO="-DDEBUG_INFO"

# default overwrites
INCLUDES = -I$(top_srcdir) -I.natfeat
DEFINITIONS =
LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS =  $(LIBKERN) -lgcc
CPU = 000

# ensure that FreeMiNT CVS proc.h is never loaded
MDFLAGS = -D_mint_proc_h -D_libkern_kernel_xfs_xdd_h
# some little code pieces needs to be metados dependent 
MDFLAGS += -D__KERNEL_XFS__
MDFLAGS += -DARAnyM_MetaDOS
MDFLAGS += $(DEBUG_INFO)

CFLAGS += $(MDFLAGS)

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:%.S=%.o)

.xfso:	$(srcdir)/../hostfs_xfs.c $(srcdir)/../hostfs_dev.c
	$(MKDIR) -p $@
	$(CP) -r $(srcdir)/../hostfs_xfs.c $(srcdir)/../hostfs_dev.c $@
	$(CP) -r $(srcdir)/../hostfs_xfs.h $(srcdir)/../hostfs_dev.h $@
	$(CP) -r $(srcdir)/../hostfs_nfapi.h $@
	touch $@/BINFILES
	touch $@/EXTRAFILES
	touch $@/MISCFILES
	echo "COBJS = hostfs_xfs.c hostfs_dev.c" >$@/SRCFILES
	$(CP) Makefile.xfso $@/Makefile
	cd $@; $(MAKE) CPU=$(CPU) MDFLAGS="$(MDFLAGS)"

.natfeat: $(srcdir)/../../natfeat
	$(MKDIR) -p $@
	$(CP) -r $</* $@
	cd $@; $(MAKE) CPU=$(CPU) parentdir=../..

$(TARGET): .natfeat .xfso $(OBJS) $(LIBKERNTARGET) 
	$(LD) $(CFLAGS) -o $@ $(OBJS) .natfeat/*.o .xfso/*.o $(LIBS)
	$(STRIP) $@

include $(top_srcdir)/DEPENDENCIES

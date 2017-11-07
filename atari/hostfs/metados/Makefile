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
GENFILES = $(TARGET)

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = metados

CPU = 000

default: all

include $(top_srcdir)/CONFIGVARS

all: $(TARGET)

debug:
	$(MAKE) DEBUG_INFO="-DDEBUG_INFO"

# default overwrites
INCLUDES = -I. -I.. -I$(top_srcdir)/natfeat
DEFINITIONS =
LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = -lgcc

# some little code pieces needs to be metados dependent 
MDFLAGS += $(DEBUG_INFO)

CFLAGS += $(INCLUDES)
CFLAGS += $(MDFLAGS)
CFLAGS += -g -O2 -fomit-frame-pointer
CFLAGS += -mshort
CFLAGS += $(WARN)

ASFLAGS = $(CFLAGS)

LDFLAGS += -s

CSRCS = \
	hostfs_nfapi.h \
	dosfile.c dosfile.h \
	dosdir.c dosdir.h \
	filesys.c filesys.h \
	k_fds.c k_fds.h \
	debug.c debug.h \
	global.h \
	kerinfo.h \
	main.c mintfake.h \
	mintproc.c mintproc.h \
	$(top_srcdir)/natfeat/natfeat.c $(top_srcdir)/natfeat/nf_ops.h \
	hostfs/hostfs_dev.c hostfs/hostfs_dev.h \
	hostfs/hostfs_xfs.c hostfs/hostfs_xfs.h \
	mint/strcpy.c \
	mint/strncpy.c \
	mint/strlen.c \
	mint/strupr.c \
	mint/bzero.c \
	mint/memcpy.c \
	mint/ctype.c \
	mint/ksprintf.c \
	mint/dostime.c \
	mint/unixtime.c \
	mint/unix2xbios.c \
	$(empty)

SSRCS = \
	mdglue.S \
	$(top_srcdir)/natfeat/natfeat_asm.S \
	$(empty)

# default definitions
OBJS = $(patsubst %.S, %.o, $(filter %.S, $(SSRCS))) $(patsubst %.c, %.o, $(filter %.c, $(CSRCS)))

$(TARGET): $(OBJS)
	$(LD) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean::
	rm -f *.o */*.o $(top_srcdir)/natfeat/*.o $(TARGET)

#
# Makefile for the embeded natfeat files.
# currently used in hostfs and hostfs/metados Makefiles
#
# usage: copy this dir into a local folder and do
#          make parentdir=..  (adjust the .. ?)
#
SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = $(parentdir)/../..
subdir = .natfeat

default: all

all-here: objs

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:%.S=%.o)

objs: $(OBJS)

include $(top_srcdir)/DEPENDENCIES

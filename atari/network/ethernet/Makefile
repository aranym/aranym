#
# Makefile for 
#
TARGET = aranym.xif

GENFILES = $(TARGET)

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = araxif

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: araether

# default overwrites
INCLUDES = -I../.. -I$(top_srcdir)
XIF_DEFINITIONS = 
DEFINITIONS = -D__KERNEL_XDD__ $(XIF_DEFINITIONS)

LD = $(CC) -mshort -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000

# default definitions
OBJS = $(addsuffix .o, $(basename $(notdir $(COBJS))))

COMMON_OBJS = $(addsuffix .o, $(basename $(COMMON_SRCS)))
ARAETHER_OBJS = $(addsuffix .o, $(basename $(notdir $(ARAETHER_SRCS))))


# generic driver target
$(TARGET): $(XOBJS) $(LIBKERNSTMP)
	$(LD) $(CFLAGS) -o $@ $(XOBJS) $(LIBS)


araether:
	$(MAKE) $(TARGET) \
                "TARGET=$(TARGET)" \
                "XOBJS=$(COMMON_OBJS) $(ARAETHER_OBJS)"


include $(top_srcdir)/DEPENDENCIES

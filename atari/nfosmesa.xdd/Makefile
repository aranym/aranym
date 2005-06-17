#
# Makefile for a xdd skeleton
#
FREEMINT_ROOT_DIR = /home/patrice/src/atari/freemint

TARGET = nfosmesa.xdd

include ../CONFIGVARS

CFLAGS = -O2 -fomit-frame-pointer -Wall -mshort

# default overwrites
INCLUDES = -I$(FREEMINT_ROOT_DIR)/sys
DEFINITIONS = -D__KERNEL_XDD__

COBJS = nfosmesa.c ../natfeat/natfeat.c

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

OBJS = $(COBJS:.c=.o)

all: $(TARGET)

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS) $(INCLUDES) $(DEFINITIONS)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

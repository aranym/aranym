
TARGET = aranym.sys

COBJS = ../common/init.c spec.c dispatch.c 8b_pal.c 16b_pal.c 32b_pal.c
SOBJS =

include ../../../CONFIGVARS

CFLAGS  = -O2 -m68020 -fomit-frame-pointer -Wall -I../include -I../../include
LDFLAGS =

COBJECTS = $(COBJS:.c=.o)
SOBJECTS = $(SOBJS:.S=.o)

all:	$(TARGET)

$(TARGET):	$(COBJECTS) $(SOBJECTS)	
	${CC} -o $@ $(COBJECTS) $(SOBJECTS) ${LDFLAGS}
	${STRIP} $(TARGET)

clean:
	rm $(OBJECTS) $(TARGET)


TARGET = dev.dos

COBJS = bosmeta.c dosdir.c dosfile.c dosfsfn.c main.c utility.c ctype.c
SOBJS = mdglue.S

include ../../CONFIGVARS

CFLAGS  = -mshort $(OPTS) $(WARN) $(GENERAL)
LDFLAGS = -nostdlib -Wl,--entry -Wl,_init -lgcc

COBJECTS = $(COBJS:.c=.o)
SOBJECTS = $(SOBJS:.S=.o)

all:	$(TARGET)


$(TARGET):	$(COBJECTS) $(SOBJECTS)	
	${CC} -o $@ $(COBJECTS) $(SOBJECTS) $(GENERAL) ${CFLAGS} ${LDFLAGS}
	${STRIP} $(TARGET)

clean:
	rm -f $(SOBJECTS) $(COBJECTS) $(TARGET)


TARGET = nfpci.prg

COBJS = nfpci.c nfpci_cookie.c ../natfeat/natfeat.c
SOBJS = entry.S nfpci_bios.S nfpci_xbios.S

include ../CONFIGVARS

CFLAGS  = -O2 -fomit-frame-pointer -Wall
ASFLAGS = -m68020
LDFLAGS = -nostdlib -Wl,--entry -Wl,_init_driver -lgcc -lc

COBJECTS = $(COBJS:.c=.o)
SOBJECTS = $(SOBJS:.S=.o)

all:	$(TARGET)

$(TARGET):	$(COBJECTS) $(SOBJECTS)	
	${CC} -o $@ $(COBJECTS) $(SOBJECTS) ${LDFLAGS}
	${STRIP} $(TARGET)

clean:
	rm $(COBJECTS) $(SOBJECTS) $(TARGET)

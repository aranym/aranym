
TARGET = nfjpeg.prg

COBJS = nfjpeg.c ../natfeat/natfeat.c ../nfpci/nfpci_cookie.c
SOBJS = entry.S nfjpeg_functions.S ../natfeat/natfeat_asm.S

include ../CONFIGVARS

CFLAGS  = -O2 -fomit-frame-pointer -Wall -W -Wstrict-prototypes
LDFLAGS = -nostdlib -Wl,--entry -Wl,_init_driver -lgcc -lc

COBJECTS = $(COBJS:.c=.o)
SOBJECTS = $(SOBJS:.S=.o)

all:	$(TARGET)

$(TARGET):	$(COBJECTS) $(SOBJECTS)	
	${CC} -o $@ $(COBJECTS) $(SOBJECTS) ${LDFLAGS}
	${STRIP} $(TARGET)

clean:
	rm -f $(COBJECTS) $(SOBJECTS) $(TARGET)

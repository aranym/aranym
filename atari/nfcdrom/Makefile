
TARGET = nfcdrom.bos

COBJS = nfcdrom.c ../natfeat/natfeat.c ../nfpci/nfpci_cookie.c
SOBJS = entry.S

include ../CONFIGVARS

CFLAGS  = $(OPTS) $(WARN) $(GENERAL)
LDFLAGS = -nostdlib -Wl,--entry -Wl,_init_driver -lgcc -lc

COBJECTS = $(COBJS:.c=.o)
SOBJECTS = $(SOBJS:.S=.o)

all:	$(TARGET)

$(TARGET):	$(COBJECTS) $(SOBJECTS)	
	${CC} -o $@ $(COBJECTS) $(SOBJECTS) ${LDFLAGS}
	${STRIP} $(TARGET)

clean:
	rm $(OBJECTS) $(TARGET)

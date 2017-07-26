
TARGET = nfstderr.bos
TARGET1 = print.prg

COBJS = nfstderrbos.c ../natfeat/natfeat.c ../nfpci/nfpci_cookie.c
SOBJS = entry.S ../natfeat/natfeat_asm.S

include ../CONFIGVARS

CFLAGS  = $(OPTS) $(WARN) $(GENERAL)
LDFLAGS = -nostdlib -Wl,--entry -Wl,_init_driver -lc -lgcc

COBJECTS = $(COBJS:.c=.o)
SOBJECTS = $(SOBJS:.S=.o)

all:	$(TARGET)

$(TARGET):	$(COBJECTS) $(SOBJECTS)	
	${CC} -o $@ $(COBJECTS) $(SOBJECTS) $(GENERAL) ${CFLAGS} ${LDFLAGS}
	${STRIP} $(TARGET)

$(TARGET1):	print.o
	${CC} -o $@ print.o $(GENERAL) ${CFLAGS}
	${STRIP} $(TARGET1)

clean:
	rm -f $(SOBJECTS) $(COBJECTS) $(TARGET)

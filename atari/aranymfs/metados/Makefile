
TARGET = aranymfs.dos

COBJS = dosmain.c
SOBJS = dinter.S

include ../../CONFIGVARS

ASFLAGS = --register-prefix-optional -m68030 $(ASINC)
CFLAGS  = -O -Wall -mshort -m68030 $(LOCALCONF) $(INC)
LDFLAGS = -nostdlib -Wl,--entry -Wl,_init -lgcc

OBJECTS = $(COBJS:.c=.o)
OBJECTS += $(SOBJS:.S=.o)

all:	$(TARGET)

$(TARGET):	$(OBJECTS)	
	${CC} -o $@ $(OBJECTS) ${LDFLAGS}
	${STRIP} $(TARGET)

clean:
	rm $(OBJECTS) $(TARGET)

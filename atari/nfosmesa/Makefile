
TARGET = osmesa.ldg

COBJS = nfosmesa.c lib-osmesa.c lib-oldmesa.c lib-misc.c lib-gl.c lib-glext.c \
	../natfeat/natfeat.c
SOBJS = ../natfeat/natfeat_asm.S

include ../CONFIGVARS

CFLAGS  = -O2 -fomit-frame-pointer -Wall
ASFLAGS = -m68020
LDFLAGS = -lldg -lgem

COBJECTS = $(COBJS:.c=.o)
SOBJECTS = $(SOBJS:.S=.o)

all:	$(TARGET)

$(TARGET):	$(COBJECTS) $(SOBJECTS)
	${CC} -o $@ $? ${LDFLAGS}
	${STRIP} $@

clean:
	rm -f $(COBJECTS) $(TARGET)

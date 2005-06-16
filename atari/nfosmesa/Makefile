
TARGET = osmesa.ldg

COBJS = nfosmesa.c lib-osmesa.c lib-oldmesa.c lib-misc.c lib-gl.c lib-glext.c \
	../natfeat/natfeat.c ../nfpci/nfpci_cookie.c

include ../CONFIGVARS

CFLAGS  = -O2 -fomit-frame-pointer -Wall
LDFLAGS = -lldg -lgem

COBJECTS = $(COBJS:.c=.o)

all:	$(TARGET)

$(TARGET):	$(COBJECTS)
	${CC} -o $@ $? ${LDFLAGS}
	${STRIP} $@

clean:
	rm $(COBJECTS) $(TARGET)

# GNU makefile for ARAnyM's WWW

## System specific configuration

.SUFFIXES: .xml .html

SHELL = /bin/sh
OUT   = out
STYLE = presentation.xslt
SAXON = build/saxon
SERVER = build/saxonserver
HTML  := $(wildcard *.html)
XML   := $(wildcard *.xml)

all:	$(OUT) $(XML:.xml=.html)

$(OUT)::
	@[ -d $(OUT) ] || mkdir $(OUT) > /dev/null 2>&1

saxon:
	@[ $(SAXON) config.xml config.xml

clean:
	rm -rf $(OUT)
	rm -rf build/saxon.jar

%.html:	%.xml
	@echo $@ ; $(SAXON) $< $(STYLE) >$(OUT)/$@

#-------------------------------------------------------------------------
# DO NOT DELETE THIS LINE -- make depend depends on it.

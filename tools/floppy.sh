#!/bin/sh

bzcat aratools.tar.bz2 | tar xf -

export MTOOLSRC="./mtoolsrc"

echo "drive t: file=\"floppy\"" > ./mtoolsrc

mformat -C -f 1440 -v ARAFLOPPY t:

mmd t:auto
mmd t:gemsys

mcopy aratools/conxboot.prg t:auto/
mcopy aratools/cecile.prg t:auto/
mcopy aratools/fvdi.prg t:auto/
mcopy aratools/betados.prg t:auto/
mcopy aratools/fastram.prg t:auto/
mcopy aratools/clocky.prg t:auto/
mcopy aratools/bdconfig.sys t:auto/
mcopy aratools/aranymfs.dos t:auto/

mcopy aratools/aranym.sys t:gemsys/

mcopy aratools/newdesk.inf t:

rm -rf ./aratools
rm -f ./mtoolsrc

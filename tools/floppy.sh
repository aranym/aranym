#!/bin/sh

bzcat aratools.tar.bz2 | tar xf -

export MTOOLSRC="./mtoolsrc"

echo "drive t: file=\"floppy\"" > ./mtoolsrc

mformat -C -f 1440 -v ARAFLOPPY t:

mmd t:auto
mmd t:gemsys
mmd t:tools

mcopy aratools/conxboot.prg t:auto/
mcopy aratools/cecile.prg t:auto/
mcopy aratools/fvdi.prg t:auto/
mcopy aratools/betados.prg t:auto/
mcopy aratools/fastram.prg t:auto/
mcopy aratools/clocky.prg t:auto/
mcopy aratools/zmagxsnd.prg t:auto/
mcopy aratools/bdconfig.sys t:auto/
mcopy aratools/aranymfs.dos t:auto/

mcopy aratools/aranym.sys t:gemsys/

mcopy aratools/newdesk.inf t:

mcopy aratools/fvdicout.app t:tools/
mcopy aratools/pcpatch.prg t:tools/

rm -rf ./aratools
rm -f ./mtoolsrc

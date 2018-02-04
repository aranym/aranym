#!/bin/sh

/cygdrive/c/cygwin/setup-x86.exe -q -P gettext-devel,libjpeg-devel,libjpeg8,cmake,flex,gmp-devel,libfontconfig-common,libfontconfig1,libfreetype6,libiconv-devel,libmpfr-devel,libncurses-devel,libpcre-devel,libpng-devel,libpng16,libpng16-devel,libusb1.0,libusb1.0-devel,mingw64-i686-SDL,mingw64-i686-SDL2,mingw64-i686-SDL_image,mingw64-i686-SDL2_image,mpfr,zip,unzip

wget -O "nsis-2.51-setup.exe" "https://downloads.sourceforge.net/project/nsis/NSIS%202/2.51/nsis-2.51-setup.exe?r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Fnsis%2Ffiles%2FNSIS%25202%2F2.51%2Fnsis-2.51-setup.exe%2Fdownload&ts=1517716774&use_mirror=svwh"
chmod 755 nsis-2.51-setup.exe
./nsis-2.51-setup.exe /S

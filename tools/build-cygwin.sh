#!/bin/bash
#
# Easy ARAnyM compilation under Cygwin
# (c) 2003-2009 Xavier Joubert for the ARAnyM team
#
#
# This script allows to easily build ARAnyM under Cygwin. The problem
# is that SDL needs to be slightly modified under Cygwin to allow to use
# it to build real Cygwin applications (instead of MinGW32 applications).
# This modification is a dirty hack (see comments below)...
#
# To use this, simply create a directory where everything will be
# contained. This script is designed to avoid polluting your Cygwin
# install. The hacked SDL build will be contained in this directory, so
# you can even keep a "normal" SDL install on your system an continue
# working on other normal SDL applications.
#
# Copy this script in this directory (please check it has the executable
# flag set - if not, "chmod +x build-cygwin.sh").
#
# Untar SDL sources in this directory (They are avaible at :
# http://www.libsdl.org/download-1.2.php ). I used SDL 1.2.15, but it should
# work with newer releases as they will be available (CVS won't work !).
# Please edit the SDL_SOURCES variable below to reflect the path where
# SDL sources lie.
#
# If you want to compile ARAnyM with --enable-nfjpeg (NatFeat jpeg
# decoder), untar SDL_image sources next to SDL sources (They are available
# at : http://www.libsdl.org/projects/SDL_image/release-1.2.html ).
# I used SDL_image 1.2.12,
# but it should work with newer releases as they will be available (CVS
# won't work !). Please edit SDL_IMAGE_SOURCES variable to reflect the path
# where SDL_image sources lie. Set this variable to blank if you don't want
# SDL_image.
#
# Untar ARAnyM sources at the same place. You can also pull sources from
# GIT (see http://aranym.sourceforge.net/development.html for details on
# how to do this). Please edit ARANYM_SOURCES variable below to reflect
# the path where ARAnyM sources lie.
#
# Now you can run "./build-cygwin.sh" to run the build ! You'll get your
# binary after a few minutes (depending on your system). If you get errors
# about "\r", please run "dos2unix build-cygwin.sh" to convert line endings
# from DOS style to unix style.
#
# Any parameter given to build-cygwin.sh will be forwarded to ARAnyM's
# configure script, so you can run "./build-cygwin.sh --enable-fullmmu"
# if you want to build with MMU support. Run
# "aranym-build/configure --help" after first build for a list
# of configure options for ARAnyM.
#
# If you happen to modify some sources, you can run "./build-cygwin.sh"
# again which will simply run "make" in the correct directory (aranym-build).
# Please note that configure options will be ignored in this case !
#
# If you want to re-configure, simply "rm aranym-build/Makefile" and run
# build-cygwin.sh again.
#
# If you modify aranym-build/configure.ac, build-cygwin.sh will automaticaly
# re-configure sources. Don't forget to give any configure options you need !
#
# If you want a clean rebuild from original sources, simply "rm -r
# aranym-build/" and run build-cygwin.sh again.
#
# Cygwin packages that should be installed :
# - gcc-core
# - gcc-g++
# - mingw-gcc-core
# - make
# - makedepend
# - git, autoconf and automake (if you want to use GIT version, or if
#   you modify configure.ac)
# - cygutils (for the dos2unix tool)
#
# Questions about building ARAnyM should go to the developper mailing
# list. See http://aranym.sourceforge.net/development.html for
# informations on how to subscribe to this list.
#

#----------------------------------------------
# Edit this to suit your configuration !
SDL_SOURCES=${PWD}/SDL-1.2.15
SDL_IMAGE_SOURCES=${PWD}/SDL_image-1.2.12
ARANYM_SOURCES=${PWD}/aranym-0.9.15
#----------------------------------------------

# A small function used to report errors
function check_return() {
  if [ $? -ne 0 ] ; then
    if [ -z "$*" ] ; then
      echo "$0: ERROR" 1>&2
    else
      echo "$0: ERROR: $*" 1>&2
    fi
    exit 1
  fi
}

if [ "x"`uname -o` != "xCygwin" ] ; then
  echo "$0: ERROR: This script is intended for Cygwin only."
  exit 1
fi

# Go to the build directory...
# This is done for the path below to be correctly generated.
cd `dirname $0`

SDL_BUILD=${PWD}/SDL-build
SDL_IMAGE_BUILD=${PWD}/SDL_image-build
SDL_PREFIX=${PWD}/SDL-prefix

ARANYM_BUILD=${PWD}/aranym-build

LINE="-------------------------------------------------------------------------------"
LINE2="==============================================================================="

# Firstly, SDL
if [ ! -x ${SDL_PREFIX}/bin/sdl-config -o \
     ! -f ${SDL_PREFIX}/bin/sdl-config.bak ] ; then
  # If not compiled and hacked, restart from the beginning...
  # This could be optimized a lot, but compiling SDL is not the
  # main purpose here.
  echo ${LINE2}
  echo "Building SDL... "
  if [ -d ${SDL_BUILD} ] ; then
    rm -r ${SDL_BUILD}
    check_return "Unable to remove ${SDL_BUILD}."
  fi
  mkdir -p ${SDL_BUILD}
  check_return "Unable to create ${SDL_BUILD}."

  if [ -d ${SDL_PREFIX} ] ; then
    rm -r ${SDL_PREFIX}
    check_return "Unable to remove ${SDL_PREFIX}."
  fi
  mkdir -p ${SDL_PREFIX}
  check_return "Unable to create ${SDL_PREFIX}."

  cd ${SDL_BUILD}
  ${SDL_SOURCES}/configure --prefix=${SDL_PREFIX} CC=i686-pc-mingw32-gcc && \
  make && \
  make install
  check_return "Unable to compile SDL from ${SDL_SOURCES}."

  # To save disk space
  cd ${SDL_PREFIX}
  rm -r ${SDL_BUILD}
  check_return "Unable to remove ${SDL_BUILD}."

  echo "done."

  echo ${LINE}
  echo -n "Modifying sdl-config script to allow building ARAnyM... "
  cd ${SDL_PREFIX}/bin
  check_return "Unable to change directory to ${SDL_PREFIX}/bin."

  # Here is the hack. We remove some flags which make GCC compile for
  # MinGW32 instead of Cygwin. We remove SDLmain from the link since
  # it's a MinGW32 object... So SDL can not parse args given to ARAnyM
  # under Cygwin.
  sed -f- -i.bak sdl-config << EOF
s/-Dmain=SDL_main//
s=-I/usr/include/mingw==
s/-DWIN32//
s/-Uunix//
s/-mno-cygwin//
s/-lmingw32//
s/-lSDLmain//
s/-mwindows//
EOF
  check_return "Unable to edit ${SDL_PREFIX}/bin/sdl-config."

  # Just in case. GNU sed keeps exec flag, but who knows...
  chmod u+x sdl-config
  check_return "Unable to make ${SDL_PREFIX}/bin/sdl-config executable."

  echo "done."
fi

# Now, SDL_image
if [ ! -z ${SDL_IMAGE_SOURCES} -a \
     ! -f ${SDL_PREFIX}/include/SDL/SDL_image.h ] ; then
  # Let SDL_IMAGE_SOURCES blank if you don't want to compile SDL_image
  # library. SDL_image is needed to use --enable-nfjpeg.
  echo ${LINE2}
  echo "Building SDL_image... "
  if [ -d ${SDL_IMAGE_BUILD} ] ; then
    rm -r ${SDL_IMAGE_BUILD}
    check_return "Unable to remove ${SDL_IMAGE_BUILD}."
  fi
  mkdir -p ${SDL_IMAGE_BUILD}
  check_return "Unable to create ${SDL_IMAGE_BUILD}."

  cd ${SDL_IMAGE_BUILD}
  ${SDL_IMAGE_SOURCES}/configure --prefix=${SDL_PREFIX} && \
  make && \
  make install
  check_return "Unable to compile SDL_image from ${SDL_IMAGE_SOURCES}."

  # To save disk space
  cd ${SDL_PREFIX}
  rm -r ${SDL_IMAGE_BUILD}
  check_return "Unable to remove ${SDL_IMAGE_BUILD}."

  echo "done."
fi

echo ${LINE2}
cd ${ARANYM_SOURCES}

if [ configure.ac -nt configure ] ; then
  # This is needed for GIT sources only
  # or if you modified configure.ac

  echo -n "Running aclocal... "
  aclocal -I ${SDL_PREFIX}/share/aclocal
  check_return "aclocal failed. Check your automake installation."
  echo "done."
  echo ${LINE}

  echo -n "Running autoheader... "
  autoheader
  check_return "autoheader failed. Check your autoconf installation."
  echo "done."
  echo ${LINE}

  echo -n "Running autoconf... "
  autoconf
  check_return "autoconf failed."
  echo "done."
  echo ${LINE}
fi

if [ ! -d ${ARANYM_BUILD} ] ; then
  mkdir -p ${ARANYM_BUILD}
  check_return "Unable to create ${ARANYM_BUILD}."
fi

cd ${ARANYM_BUILD}
check_return "Unable to access ${ARANYM_BUILD}."

if [ ${ARANYM_SOURCES}/configure.ac -nt Makefile ] ; then
  echo "Running \"configure $@\"..."
  PATH=${SDL_PREFIX}/bin:${PATH} \
  ${ARANYM_SOURCES}/configure "$@"
  check_return "Unable to configure ARAnyM."
  echo "done."
  echo ${LINE}

  echo "Running make depend..."
  make depend
  check_return "Unable to create dependencies. Check your XFree86-bin installation."
  echo "done."
  echo ${LINE}
else
  if [ ! -z "$*" ] ; then
    echo "$0: WARNING: configure options ignored. if you want to re-configure, please run :" 1>&2
    echo "rm ${ARANYM_BUILD}/Makefile && $0 $@" 1>&2
    echo ${LINE}
  fi
fi

echo "Running make... "
make
check_return "Compilation failed."
echo "done."

echo ${LINE2}

echo "Congratulations ! Your home made ARAnyM binary is available in \"${ARANYM_BUILD}/\"."

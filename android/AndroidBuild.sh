#!/bin/sh

LOCAL_PATH=`dirname $0`
LOCAL_PATH=`cd $LOCAL_PATH && pwd`

ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/armeabi-v7a/libSDL.so
ln -sf libsdl-1.2.so $LOCAL_PATH/../../../obj/local/armeabi-v7a/libpthread.so
ln -sf libsdl_image.so $LOCAL_PATH/../../../obj/local/armeabi-v7a/libSDL_image.so

if [ "$1" = armeabi-v7a ]; then
if [ \! -f aranym/configure ] ; then
	export NO_CONFIGURE=1
	sh -c "cd aranym && ./autogen.sh"
fi

if [ \! -f aranym/Makefile ] ; then
  $LOCAL_PATH/../setEnvironment-armeabi-v7a.sh sh -c "cd aranym && ./configure --build=x86_64-unknown-linux-gnu --host=arm-linux-androideabi --disable-opengl --disable-nfclipbrd --disable-cxx-exceptions --disable-ethernet --disable-jit-compiler --disable-ata-cdrom --disable-parport --disable-sdl2 --prefix=/mnt/sdcard/Android/data/org.aranym.sdl/files"
fi

make -C aranym && cp aranym/src/aranym libapplication-armeabi-v7a.so
fi

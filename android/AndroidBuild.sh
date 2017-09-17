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

configure_options="--host=arm-linux-androideabi \
	--prefix=/storage/sdcard/Android/data/org.aranym.sdl/files \
	--disable-opengl \
	--disable-nfclipbrd \
	--disable-cxx-exceptions \
	--disable-ethernet \
	--disable-ata-cdrom \
	--disable-parport \
	--disable-sdl2 \
	--enable-disasm=builtin \
	"

if [ \! -f aranym/Makefile ] ; then
  $LOCAL_PATH/../setEnvironment-armeabi-v7a.sh sh -c "cd aranym && ./configure $configure_options"
fi

make -C aranym && cp aranym/src/aranym libapplication-armeabi-v7a.so
fi

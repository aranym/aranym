#!/bin/sh

echo rvm_autoupdate_flag=0 >> ~/.rvmrc

case "$TRAVIS_OS_NAME" in
linux)
	sudo apt-get update
	sudo apt-get install -y \
		curl \
		libosmesa6-dev \
		libgl1-mesa-dev \
		libglu1-mesa-dev \
		libsdl1.2-dev \
		libsdl-image1.2-dev \
		libusb-dev \
		libusb-1.0-0-dev \
		libudev-dev \
		zsync \
		libjson-perl \
		libwww-perl \
		
	;;

osx)
	curl --get https://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.12.dmg --output SDL_image.dmg
	curl --get https://www.libsdl.org/release/SDL-1.2.15.dmg --output SDL.dmg
	curl --get https://www.libsdl.org/release/SDL2-2.0.7.dmg --output SDL2.dmg
	curl --get https://www.libsdl.org/projects/SDL_image/release/SDL2_image-2.0.2.dmg --output SDL2_image.dmg
	mkdir -p ~/Library/Frameworks
	
	mountpoint=~/sdltmp
	mkdir "$mountpoint"

	hdiutil attach -mountpoint "$mountpoint" -readonly SDL.dmg
	cp -R "$mountpoint/SDL.framework" ~/Library/Frameworks
	hdiutil detach "$mountpoint"

	hdiutil attach -mountpoint "$mountpoint" -readonly SDL_image.dmg
	cp -R "$mountpoint/SDL_image.framework" ~/Library/Frameworks
	hdiutil detach "$mountpoint"

	hdiutil attach -mountpoint "$mountpoint" -readonly SDL2.dmg
	cp -R "$mountpoint/SDL2.framework" ~/Library/Frameworks
	hdiutil detach "$mountpoint"

	hdiutil attach -mountpoint "$mountpoint" -readonly SDL2_image.dmg
	cp -R "$mountpoint/SDL2_image.framework" ~/Library/Frameworks
	hdiutil detach "$mountpoint"

	rmdir "$mountpoint"
	
	rm -rf src/Unix/MacOSX/SDL.Framework
	rm -rf src/Unix/MacOSX/SDL_image.Framework
	rm -rf src/Unix/MacOSX/SDL2.Framework
	rm -rf src/Unix/MacOSX/SDL2_image.Framework
	ln -s ~/Library/Frameworks/SDL.Framework src/Unix/MacOSX/SDL.Framework
	ln -s ~/Library/Frameworks/SDL_image.Framework src/Unix/MacOSX/SDL_image.Framework
	ln -s ~/Library/Frameworks/SDL2.Framework src/Unix/MacOSX/SDL2.Framework
	ln -s ~/Library/Frameworks/SDL2_image.Framework src/Unix/MacOSX/SDL2_image.Framework
	
	# we must install the macports version of the dependencies,
	# because the brew packages are not universal
	mkdir src/Unix/MacOSX/external
	for i in gmp/gmp-6.1.2_0+universal.darwin_16.i386-x86_64.tbz2 \
		mpfr/mpfr-3.1.5_0+universal.darwin_16.i386-x86_64.tbz2 \
		jpeg/jpeg-9b_0+universal.darwin_16.i386-x86_64.tbz2; do
		f=`basename $i`
		curl --get https://packages.macports.org/$i --output $f
		tar -C src/Unix/MacOSX/external --include="./opt/local" --strip-components=3 -xf $f
	done
	;;

*)
	exit 1
	;;
esac

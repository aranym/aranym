#!/bin/sh

datefile=version_date.h
versfile=src/include/version.h
log=build-cygwin.log

if ! test -f $datefile; then
	echo "$0: $datefile missing" >&2
	exit 1
fi
if ! test -f $versfile; then
	echo "$0: $versfile missing" >&2
	exit 1
fi

#
# configure args for all configurations
#
CONFIGURE_ARGS="--prefix=/usr --enable-usbhost"

major=`sed -n -e 's/#define[ \t]*VER_MAJOR[ \t]*\([0-9]*\)/\1/p' $versfile`
minor=`sed -n -e 's/#define[ \t]*VER_MINOR[ \t]*\([0-9]*\)/\1/p' $versfile`
micro=`sed -n -e 's/#define[ \t]*VER_MICRO[ \t]*\([0-9]*\)/\1/p' $versfile`

version=$major.$minor.$micro

date=`sed -n -e 's/#define[ \t]*VERSION_DATE[ \t]*"\([^"]*\)"/\1/p' $datefile`

echo version=$version
echo date=$date
rm -f $log

trap 'rm -f $log' 1 2 3 15

function build() {
	local sdloption

	case $SDL in 
	1) sdloption=--disable-sdl2 ;;
	2) sdloption=--enable-sdl2 ;;
	*) exit 1 ;;
	esac
	
	echo "configuring aranym-mmu..."
	./configure $CONFIGURE_ARGS $sdloption --enable-fullmmu >> $log 2>&1 || {
		echo "configuring aranym-mmu failed; see $log for details" >&2
		exit 1
	}
	echo "building aranym-mmu..."
	make clean
	make >> $log 2>&1 || {
		echo "building aranym-mmu failed; see $log for details" >&2
		exit 1
	}
	strip aranym.exe || exit 1
	mv aranym.exe aranym-mmu.exe
	
	
	echo "configuring aranym-jit..."
	./configure $CONFIGURE_ARGS $sdloption --enable-jit-compiler --enable-jit-fpu >> $log 2>&1 || {
		echo "configuring aranym-mmu failed; see $log for details" >&2
		exit 1
	}
	echo "building aranym-jit..."
	make clean
	make >> $log 2>&1 || {
		echo "building aranym-jit failed; see $log for details" >&2
		exit 1
	}
	strip aranym.exe || exit 1
	mv aranym.exe aranym-jit.exe
	
	
	echo "configuring aranym..."
	./configure $CONFIGURE_ARGS $sdloption >> $log 2>&1 || {
		echo "configuring aranym-mmu failed; see $log for details" >&2
		exit 1
	}
	echo "building aranym-jit..."
	make clean
	make >> $log 2>&1 || {
		echo "building aranym failed; see $log for details" >&2
		exit 1
	}
	strip aranym.exe || exit 1
	mv aranym.exe aranym-dflt.exe
	
	make clean
	
	mv aranym-dflt.exe aranym.exe
}


function mkdist() {
	local sdlname
	local sdllibs
	local sdldef
	
	case $SDL in 
	1) sdlname=-sdl1
	   sdllibs="SDL.dll SDL_image.dll"
	   ;;
	2) sdlname=-sdl2
	   sdllibs="SDL2.dll SDL2_image.dll"
	   ;;
	*) exit 1 ;;
	esac
	sdldef=-DSDL=${sdlname}
	
	tmpdir="${TMPDIR:-/tmp}"
	distdir="$tmpdir/aranym-$version${sdlname}"
	
	mkdir -p "$distdir/doc" "$distdir/aranym" || exit 1
	
	cp -a COPYING "$distdir/COPYING.txt" || exit 1
	sed -e "s|@VERSION@|$version|g" -e "s|@DATE@|$date|g" README-cygwin.in > "$distdir/README.txt" || exit 1
	for f in AUTHORS BUGS README ChangeLog FAQ NEWS README TODO; do
		cp -a $f "$distdir/doc/$f.txt" || exit 1
	done
	for f in config; do
		cp -a doc/$f "$distdir/doc/$f.txt" || exit 1
	done
	for f in logo.png wm_icon.bmp; do
		cp -a data/$f "$distdir/aranym/$f" || exit 1
	done
	
	(cd "$distdir"
	  for f in doc/* *.txt; do
	     cat $f | tr -d '\r' | sed -e 's/$/\r/' > $$.tmp
	     touch -r $f $$.tmp
	     mv $$.tmp $f
	  done
	)
	
	for f in aranym.exe aranym-jit.exe aranym-mmu.exe; do
		cp -a $f "$distdir" || exit 1
	done
	
	for f in cyggcc_s-1.dll cygstdc++-6.dll cygwin1.dll; do
		cp -a /bin/$f "$distdir" || exit 1
	done
	
	for f in $sdllibs libjpeg-8.dll libpng14-14.dll libtiff-3.dll zlib1.dll; do
		found=false
		for dir in /usr/mingw/bin /usr/bin/mingw /mingw/bin; do
			if test -f "$dir/$f"; then
				cp -a "$dir/$f" "$distdir" || exit 1
				found=true
				break
			fi
		done
		if ! $found; then
			echo "cannot find $f" >&2
			exit 1
		fi
	done
	
	( cd "$tmpdir"
	  zip -r aranym-$version${sdlname}-cygwin.zip aranym-$version${sdlname}
	) || exit 1
	
	echo "$tmpdir/aranym-$version${sdlname}-cygwin.zip ready for release"
	
	nsis=`cygpath "$PROGRAMFILES"`/NSIS/makensis.exe
	if test -x "$nsis"; then
	    cwd=`pwd`
	    cd "$distdir"
	    echo "creating Windows installer"
	    find . -type d | sed -e 's|^./\(.*\)$|${CreateDirectory} "$INSTDIR\\\1"|' -e '/^\.$/d' | tr '/' '\\' > ../aranym.files
	    find . -type f | sed -e 's|^./\(.*\)$|${File} "\1"|' | tr '/' '\\' >> ../aranym.files
	    "$nsis" -V2 -NOCD -DVER_MAJOR=$major -DVER_MINOR=$minor -DVER_MICRO=$micro $sdldef `cygpath -m $cwd/tools/aranym.nsi` || exit 1
	    rm -f ../aranym.files
	    cd $cwd
	    echo "$tmpdir/aranym-$version${sdlname}-setup.exe ready for release"
	fi
	
}

SDL=1
build
mkdist

SDL=2
build
mkdist


rm -f $log

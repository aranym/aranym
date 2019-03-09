
top_srcdir=${top_srcdir:-`cd ..; pwd`}

GLIBC_NEEDED=2.11
STATIC_FILES=${top_srcdir}/appimage

BINTRAY_USER="${BINTRAY_USER:-aranym}"
BINTRAY_REPO_OWNER="${BINTRAY_REPO_OWNER:-$BINTRAY_USER}" # owner and user not always the same
BINTRAY_REPO_NAME="${BINTRAY_REPO_NAME:-aranym-files}"
BINTRAY_REPO="${BINTRAY_REPO_OWNER}/${BINTRAY_REPO_NAME}"

case $CPU_TYPE in
	x86_64) ARCH=x86_64 ;;
	i[3456]86) ARCH=i686 ;;
	arm*) ARCH=armhf ;;
	*) echo "unsupported architecture $CPU_TYPE for AppImage" >&2; exit 1 ;;
esac

. ${top_srcdir}/appimage/functions.sh


for EXE in aranym aranym-mmu aranym-jit
do
	case $EXE in
		aranym-mmu) APP="ARAnyM-MMU" ;;
		aranym-jit) APP="ARAnyM-JIT" ;;
		*) APP="ARAnyM" ;;
	esac
	LOWERAPP="$EXE"
	
	test -x "$BUILDROOT/usr/bin/$EXE" || continue
	
	rm -rf "$APP.AppDir"
	mkdir -p "$APP.AppDir/usr"

	cd "./$APP.AppDir/"

	cp -a "$BUILDROOT/." .
	rm -f usr/bin/aranym*
	cp -a "$BUILDROOT/usr/bin/$EXE" usr/bin
	
	get_desktop
	get_icon
	get_apprun
	test -x ./AppRun || exit 1
	copy_deps
	delete_blacklisted
	move_lib
	
	patch_usr
	
	if test -f usr/bin/aratapif; then
		sudo chown root "usr/bin/aratapif"
		sudo chgrp root "usr/bin/aratapif"
		sudo chmod ug+s "usr/bin/aratapif"
	fi
	
	get_desktopintegration "$LOWERAPP"

# Go out of AppImage
	cd ..

	APPIMAGE=`echo $APP | tr ' ' '_'`
	if test -n "$TRAVIS"; then
		# invoked from travis build; depends on some variables set by .travis/build.sh
		if $isrelease; then
			APPIMAGE=${APPIMAGE}-${VERSION}-${CPU_TYPE}.AppImage
		else
			APPIMAGE=${APPIMAGE}-${VERSION}-${CPU_TYPE}-${SHORT_ID}.AppImage
		fi
	else
		# invoked from toplevel "make appimage"
		APPIMAGE=${APPIMAGE}-${VERSION}-${CPU_TYPE}.AppImage
	fi
	generate_type2_appimage
done	

# Fix up dynamic libraries

cd "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH"
ls -al
test -x "$EXECUTABLE_NAME" || exit 1

# Fixup libmpfr
if test -f libmpfr.dylib -a -f libgmp.dylib; then
	echo "Fixup libmpfr: libgmp"
	for ARCH in $ARCHS ; do
		ORIG_PATH=`otool -arch $ARCH -l "libmpfr.dylib" | grep libgmp | awk '{print $2}'`
		echo "  $ARCH: $ORIG_PATH"
		install_name_tool -change "$ORIG_PATH" @executable_path/libgmp.dylib libmpfr.dylib || exit 1
	done
fi

# Fixup executable
if test -f libgmp.dylib; then
	echo "Fixup $EXECUTABLE_NAME: libgmp"
	for ARCH in $ARCHS ; do
		ORIG_PATH=`otool -arch $ARCH -l "$EXECUTABLE_NAME" | grep libgmp | awk '{print $2}'`
		echo "  $ARCH: $ORIG_PATH"
		install_name_tool -change "$ORIG_PATH" @executable_path/libgmp.dylib "$EXECUTABLE_NAME" || exit 1
	done
fi
if test -f libmpfr.dylib; then
	echo "Fixup $EXECUTABLE_NAME: libmpfr"
	for ARCH in $ARCHS ; do
		ORIG_PATH=`otool -arch $ARCH -l "$EXECUTABLE_NAME" | grep libmpfr | awk '{print $2}'`
		echo "  $ARCH: $ORIG_PATH"
		install_name_tool -change "$ORIG_PATH" @executable_path/libmpfr.dylib "$EXECUTABLE_NAME" || exit 1
	done
fi
if test -f libjpeg.dylib; then
	echo "Fixup $EXECUTABLE_NAME: libjpeg"
	for ARCH in $ARCHS ; do
		ORIG_PATH=`otool -arch $ARCH -l "$EXECUTABLE_NAME" | grep libjpeg | awk '{print $2}'`
		echo "  $ARCH: $ORIG_PATH"
		install_name_tool -change "$ORIG_PATH" @executable_path/libjpeg.dylib "$EXECUTABLE_NAME" || exit 1
	done
fi

exit 0

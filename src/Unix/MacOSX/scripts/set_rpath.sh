# Fix up dynamic libraries

cd "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH"
ls -al
test -x "$EXECUTABLE_NAME" || exit 1

# Fixup libmpfr
if test -f libmpfr.dylib -a -f libgmp.dylib; then
	for i in "" .10; do
		install_name_tool -change /opt/local/lib/libgmp${i}.dylib @executable_path/libgmp.dylib libmpfr.dylib || exit 1
	done
fi

# Fixup executable
if test -f libgmp.dylib; then
	for i in "" .10; do
		install_name_tool -change /opt/local/lib/libgmp${i}.dylib @executable_path/libgmp.dylib "$EXECUTABLE_NAME" || exit 1
	done
fi
if test -f libmpfr.dylib; then
	for i in "" .4; do
		install_name_tool -change /opt/local/lib/libmpfr${i}.dylib @executable_path/libmpfr.dylib "$EXECUTABLE_NAME" || exit 1
	done
fi
if test -f libjpeg.dylib; then
	for i in "" .7 .8 .9; do
		install_name_tool -change /opt/local/lib/libjpeg${i}.dylib @executable_path/libjpeg.dylib "$EXECUTABLE_NAME" || exit 1
	done
fi

exit 0

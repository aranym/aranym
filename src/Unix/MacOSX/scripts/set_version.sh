#!/bin/sh

if [ ! -f "$DERIVED_FILE_DIR/aranym_version" ]; then
  mkdir -p "$DERIVED_FILE_DIR"  || exit 1
  cd "$DERIVED_FILE_DIR"  || exit 1

  cat > aranym_version.c <<EOF
#include <stdio.h>
#include "version_date.h"
#include "version.h"

#define str(x)		_stringify (x)
#define _stringify(x)	#x

#define VERSION_STRING	NAME_STRING " " str (VER_MAJOR) "." str (VER_MINOR) "." str (VER_MICRO) VER_STATUS

int main() {
  puts(VERSION_STRING);
  return 0;
}
EOF
  gcc -o aranym_version aranym_version.c "-I$SOURCE_DIR/include" "-I$DERIVED_FILES_DIR" || exit 1
fi

"$DERIVED_FILE_DIR/aranym_version" || exit 1

VERSION=`"$DERIVED_FILE_DIR/aranym_version" | awk '{FS=" "}{print $2}'`



echo "Setting Aranym version in info.plist to $VERSION"
PLIST=$TARGET_BUILD_DIR/$INFOPLIST_PATH
PLIST_NEW=$PLIST.new
PLIST_OLD=$PLIST.old

echo "Modifying $PLIST"

if [ -f "$PLIST" ]; then
  cat "$PLIST" | sed "s/\#\#VERSION\#\#/$VERSION/" > "$PLIST_NEW" || exit 1
  if [ -f "$PLIST_NEW" ]; then 
    #mv "$PLIST" "$PLIST_OLD"
    mv "$PLIST_NEW" "$PLIST"
  else
    echo "Error: Update failed"
    echo "  PLIST file: $PLIST"
    echo "  VERSION: $VERSION"
    exit 1
  fi
else
  echo "Error: unable to update version description in PLIST: "
  echo "  missing $PLIST"
  exit 1
fi

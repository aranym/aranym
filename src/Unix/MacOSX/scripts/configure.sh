#!/bin/sh

#################### CONFIGURE SYSTEM ################
# Check if configure script has to be run
echo "Checking configure options: $CONFIGURE_OPTIONS"
OPTIONS_FILE=$DERIVED_FILES_DIR/configure_options
FILE_CONTENT=`cat "$OPTIONS_FILE" 2>/dev/null`
if [ \( -f "$OPTIONS_FILE" \) -a \( "$FILE_CONTENT" == "$CONFIGURE_OPTIONS" \) ]; then
  echo "Configuration still up-to-date. Skipping reconfiguration."	
  rsync -pogt "$DERIVED_FILES_DIR/"config*.h "$BUILD_DIR/" || exit 1
  exit 0
fi

echo "Running configure script for the following architectures $ARCHS"

# Check if PROJECT_DIR variable is set (Xcode 2.x)
if [ -z "$PROJECT_DIR" ]; then
  echo '$PROJECT_DIR WAS NOT DECLARED. SETTING VARIABLE:'
  old_dir=$PWD
  cd "$BUILD_DIR/.."
  export PROJECT_DIR="$PWD"
  cd $old_dir
  echo "	\$PROJECT_DIR = $PROJECT_DIR"
  echo
fi

# Make sure makedepend can be found
export PATH="$PATH:/usr/X11R6/bin"

# Make sure SDL.m4 can be found
export ACLOCAL_FLAGS="-I $PROJECT_DIR/../darwin -I /usr/X11/share/aclocal"

# Make sure SDL.framework can be found
export LDFLAGS=-F$PROJECT_DIR
export DYLD_FRAMEWORK_PATH=$PROJECT_DIR

if [ ! -d "$DERIVED_FILES_DIR" ]; then
  echo "Creating $DERIVED_FILES_DIR"
  mkdir -p "$DERIVED_FILES_DIR"
fi

cd "$SOURCE_DIR/.."

# Remove old files
rm -rf Makefile autom4te.cache aclocal.m4 config.h config.log 2>/dev/null

# Generate configure script
if [ -f autogen.sh ]; then
  export NO_CONFIGURE=yes
  echo "Calling autogen.sh to prepare configure script"
  ./autogen.sh
fi

# Configure system for all build architectures
echo "" > "$DERIVED_FILES_DIR/config.h"
UREL=`uname -r`
for ARCH in $ARCHS ; do
  CPU_TYPE=$(eval echo $(echo \$CPU_TYPE_$ARCH))
  echo ; echo "Running configure for architecture $ARCH / $CPU_TYPE / $UREL"
  echo $PWD
  ./configure $CONFIGURE_OPTIONS --host=$ARCH-apple-darwin$UREL || exit 1

  if [ "$ARCH" = "ppc" -a "$SDK_NAME" = "macosx10.3.9" ]; then
    # 10.3.9 compatibility:
	echo "Special handling of PPC 10.3.9 build"
    cat config.h | sed 's/#define HAVE_SYS_STATVFS_H 1/\/* #undef HAVE_SYS_STATVFS_H *\//' > config_$ARCH.h
    rm config.h
  else
    mv config.h config_$ARCH.h
  fi
  cat  >> "$DERIVED_FILES_DIR/config.h" << EOF
#ifdef $CPU_TYPE
#include "config_$ARCH.h"
#endif

EOF
  mv config_$ARCH.h "$DERIVED_FILES_DIR"
  mv Makefile "$DERIVED_FILES_DIR/Makefile_$ARCH"
done

# Remember configure options for next script execution
echo "$CONFIGURE_OPTIONS" > "$OPTIONS_FILE"

echo "Configuration generated:"
cp "$DERIVED_FILES_DIR/"config*.h "$BUILD_DIR/" || exit 1

exit 0
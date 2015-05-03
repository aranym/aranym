#!/bin/sh

########################## BUILD CPU EMULATION CORE ##########################

mkdir -p "$DERIVED_FILES_DIR" && cd "$DERIVED_FILES_DIR" || exit 1
MISSING_FILE_CNT=`ls cpuemu.cpp cpudefs.cpp cputbl.h cpustbl.cpp 2>&1 1>/dev/null | wc -l`
rsync -pogt --stats cpuemu*.cpp cpudefs.cpp cputbl.h cpustbl*.cpp config*.h "$BUILD_DIR" > rsync_output.txt 2>/dev/null
RET=$?
#cat rsync_output.txt
TRANSFERRED=`cat rsync_output.txt | grep "transferred:" | awk '{print $5}'`
echo "$TRANSFERRED files transferred. RSYNC return code=$RET $MISSING_FILE_CNT file(s) are missing."
if [ "$RET" -eq 0 -a "$TRANSFERRED" -eq 0 -a "$MISSING_FILE_CNT" -eq 0 ]; then
  echo "Sources up-to-date. Skipping generation of CPU emulation core"
  exit 0
fi


# remove old files
rm -f "$DERIVED_FILES_DIR"/cpu*.*

if [ "x$TARGET_NAME" = "xMacAranym JIT" ]; then
  IS_JIT_COMPILE="+"
  JIT_ADDITIONAL_FILES="compemu.cpp compstbl.cpp comptbl.h"
  JIT_TARGET=./compemu.cpp
  echo "JIT CPU generation: $JIT_ADDITIONAL_FILES"
else
  echo "no JIT CPU generation"
  IS_JIT_COMPILE="-"
  JIT_ADDITIONAL_FILES=
fi

for ARCH in $ARCHS ; do
  CPU_TYPE=$(eval echo $(echo \$CPU_TYPE_$ARCH))
  echo ; echo "Building CPU emulation core for $ARCH"
  mkdir -p "$TEMP_FILES_DIR/$ARCH"
  cd "$TEMP_FILES_DIR/$ARCH"
  cp "$DERIVED_FILES_DIR"/config*.h .
  make -f "$DERIVED_FILES_DIR/Makefile_$ARCH" top_srcdir="$SOURCE_DIR/.." GEN_DIR=. version_date.h ./cpuemu.cpp $JIT_TARGET || exit 1
  mv cpuemu.cpp "$DERIVED_FILES_DIR/cpuemu_$ARCH.cpp"
  cat  >> "$DERIVED_FILES_DIR/cpuemu.cpp" << EOF
#ifdef $CPU_TYPE
#include "cpuemu_$ARCH.cpp"
#endif

EOF
  mv cpufunctbl.cpp "$DERIVED_FILES_DIR/cpufunctbl_$ARCH.cpp"
  cat  >> "$DERIVED_FILES_DIR/cpufunctbl.cpp" << EOF
#ifdef $CPU_TYPE
#include "cpufunctbl_$ARCH.cpp"
#endif

EOF
done

if [ x$IS_JIT_COMPILE = 'x+' ]; then
  echo "Building cpustbl_nf.cpp"
  cat << EOF > "$DERIVED_FILES_DIR/cpustbl_nf.cpp"
#define NOFLAGS
#include "cpustbl.cpp"
EOF

  echo "Building cpuemu_nf.cpp"
  cat << EOF > "$DERIVED_FILES_DIR/cpuemu_nf.cpp"
#define NOFLAGS
#include "cpuemu.cpp"
EOF

fi

rsync -pogt version_date.h cpudefs.cpp cputbl.h cpustbl*.cpp $JIT_ADDITIONAL_FILES "$DERIVED_FILES_DIR"
cd "$DERIVED_FILES_DIR"
rsync -pogt cpuemu*.cpp cpudefs.cpp cputbl.h cpustbl*.cpp cpufunctbl*.cpp $JIT_ADDITIONAL_FILES version_date.h "$BUILD_DIR" || exit 1

exit 0
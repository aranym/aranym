#!/bin/sh

########################## BUILD CPU EMULATION CORE ##########################

mkdir -p "$DERIVED_FILES_DIR" && cd "$DERIVED_FILES_DIR" || exit 1

case $TARGET_NAME in
*JIT*)
  IS_JIT_COMPILE="+"
  JIT_ADDITIONAL_FILES="compemu.cpp compstbl.cpp comptbl.h"
  JIT_TARGET=./compemu.cpp
  echo "JIT CPU generation: $JIT_ADDITIONAL_FILES"
  ;;
*)
  echo "no JIT CPU generation"
  IS_JIT_COMPILE="-"
  JIT_ADDITIONAL_FILES=
  ;;
esac

rm -f cpuemu.cpp cpufunctbl.cpp cpustbl_nf.cpp cpuemu_nf.cpp

for ARCH in $ARCHS ; do
  CPU_TYPE=$(eval echo $(echo \$CPU_TYPE_$ARCH))
  echo ; echo "Building CPU emulation core for $ARCH"
  cd "$DERIVED_FILES_DIR"
  make -f "Makefile_$ARCH" version_date.h || exit 1
  cd src/uae_cpu
  test -d compiler || mkdir compiler
  make cpuemu.cpp $JIT_TARGET || exit 1
  mv cpuemu.cpp "$DERIVED_FILES_DIR/cpuemu_$ARCH.cpp"
  mv cpudefs.cpp cputbl.h cpustbl*.cpp $JIT_ADDITIONAL_FILES "$DERIVED_FILES_DIR"
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

exit 0

#!/bin/sh

########################## GENERATE MACH FILES FROM SYSTEM DEFS ##########################
set -x
if [[ $ARCHS =~ (^| )x86_64($| ) && ! -f $BUILD_DIR/mach_excServer.c ]]; then
	pushd "$BUILD_DIR"

	DEFS_FILE=/usr/include/mach/mach_exc.defs
	if [[ ! -f $DEFS_FILE ]]; then
		#
		# In XCode 5 SDK is located inside the XCode Application package, and if
		# the command line tools are not installed. To install command line tools
		# execute xcode-select --install
		#
		DEFS_FILE=$SDK_DIR/usr/include/mach/mach_exc.defs
	fi

	mig -arch x86_64 -v $DEFS_FILE
	popd
fi

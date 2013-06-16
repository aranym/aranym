#!/bin/sh

########################## GENERATE MACH FILES FROM SYSTEM DEFS ##########################

if [[ $ARCHS =~ (^| )x86_64($| ) && ! -f $BUILD_DIR/mach_excServer.c ]]; then
	pushd "$BUILD_DIR"
	mig -arch x86_64 -v /usr/include/mach/mach_exc.defs
	popd
fi
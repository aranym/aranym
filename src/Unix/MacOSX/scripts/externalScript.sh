#! /bin/sh

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# ACTIONS
# These are implemented as functions, and just called by the
# short MAIN section below

buildAction () {
    echo "Building..."

    # Now do build steps.

}

cleanAction () {
    echo "Cleaning..."

    # Do your clean steps here.
	cd "$BUILD_DIR" || exit 1
	rm -f cpuemu*.cpp cpudefs.cpp cputbl.h cpustbl*.cpp cpufunctbl*.cpp config*.h compemu.cpp compstbl.cpp comptbl.h
}

# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# MAIN

echo "Running with ACTION=${ACTION}"

case $ACTION in
    # NOTE: for some reason, it gets set to "" rather than "build" when
    # doing a build.
    "")
        buildAction
        ;;

    "clean")
        cleanAction
        ;;
esac

exit 0

if [ x$FREEMINT_SRC_HOME == x ]; then 
	FREEMINT_SRC_HOME=/opt/home/stop/projects/freemint
fi

if [ ! -e $FREEMINT_SRC_HOME/COPYING.MiNT ]; then 
	echo "The $FREEMINT_SRC_HOME seems not to be the FreeMiNT CVS root";
	exit 1;
fi

ETHERNET_BUILD_HOME=$FREEMINT_SRC_HOME/sys/sockets/xif

# copy the sources into the FreeMiNT cvs tree
cp -r ../ethernet $ETHERNET_BUILD_HOME

# build them (all or debug targets are available)
(cd $ETHERNET_BUILD_HOME/ethernet; make distclean; make $@)

# move the binaries into the .bin folder
mkdir -p .bin
cp $ETHERNET_BUILD_HOME/ethernet/*.xif .bin

# cleanup the FreeMiNT cvs tree
rm -rf $ETHERNET_BUILD_HOME/ethernet

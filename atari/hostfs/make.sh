if [ x$FREEMINT_SRC_HOME == x ]; then 
	FREEMINT_SRC_HOME=/opt/home/stop/projects/freemint
fi

if [ ! -e $FREEMINT_SRC_HOME/COPYING.MiNT ]; then 
	echo "The $FREEMINT_SRC_HOME seems not to be the FreeMiNT CVS root";
	exit 1;
fi

HOSTFS_BUILD_HOME=$FREEMINT_SRC_HOME/sys/xfs

# copy the sources into the FreeMiNT cvs tree
cp -r ../hostfs $HOSTFS_BUILD_HOME
cp -r ../natfeat $HOSTFS_BUILD_HOME

# build them (all or debug targets are available)
(cd $HOSTFS_BUILD_HOME/hostfs; make distclean; make $@)

# move the binaries into the bin folder
mkdir -p bin
cp $HOSTFS_BUILD_HOME/hostfs/metados/*.dos bin
cp $HOSTFS_BUILD_HOME/hostfs/*.xfs bin

# cleanup the FreeMiNT cvs tree
rm -rf $HOSTFS_BUILD_HOME/hostfs
rm -rf $HOSTFS_BUILD_HOME/natfeat

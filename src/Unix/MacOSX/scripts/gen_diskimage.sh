#!/bin/sh

VERSION=`"$CONFIGURATION_TEMP_DIR/MacAranym.build/DerivedSources/aranym_version" | awk '{FS=" "}{print $2}'`

cd "$TARGET_BUILD_DIR/aranym-xxx-macosx/DOC"
echo "Processing text files..."
DATE=`date '+%Y\/%m\/%d'`
mv COPYING ..
cat README.rtf \
	| sed "s/(\*VERSION\*)/$VERSION/g" \
	| sed "s/(\*DATE\*)/$DATE/g" \
	> ../README.rtf
rm README.rtf

cd "$TARGET_BUILD_DIR/aranym-xxx-macosx"
VERSION=`echo "-$VERSION" | sed 's/ //g;s/-//g'`
DMGNAME=$TARGET_BUILD_DIR/aranym-$VERSION-macosx-`echo $ARCHS | tr " " "_"`.dmg
echo "Building disk image $DMGNAME..."
hdiutil create "$DMGNAME" -ov -volname "MacAranym $VERSION" -srcfolder . -format UDZO
codesign -s "$CODE_SIGN_IDENTITY" --timestamp -i org.aranym.MacAranym.diskimage "$DMGNAME"
open -R $DMGNAME

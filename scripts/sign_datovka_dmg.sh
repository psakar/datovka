#!/usr/bin/env sh

# Instruction how to use the script:
# 1. Create dir "source-packages" in the root of script location if not exists.
# 2. Copy all dmg packages to be sign into "source-packages" dir.
# 3. Run the script.
# 4. Signed dmg packages are generated in the "dmg-signed" dir.

SIGN_CERT_ID="CZ.NIC, z.s.p.o."

SRC_ROOT=$(pwd)

APP_NAME="datovka"
DMG_TITLE="${APP_NAME} installer"
VOLUME_DIR="/Volumes/${DMG_TITLE}"
APPROOT="${APP_NAME}.app"

DMG_SOURCE_DIR="source-packages"
DMG_SIGN_DIR="dmg-signed"

rm -r $DMG_SIGN_DIR
mkdir $DMG_SIGN_DIR

dmgFiles=$(find $DMG_SOURCE_DIR -name "*.dmg")

for dmg in $dmgFiles; do

	echo
	echo "Open dmg package..."
	DEVICE=$(hdiutil attach -owners on ${dmg} -shadow | egrep '^/dev/' | sed 1q | awk '{print $1}')
	echo "Dmg package has been opened as device $DEVICE"

	cd "$VOLUME_DIR"
	# First sign all application libraries
	echo
	echo ---Sign all Datovka dylib libraries---
	dylibFiles=$(find "${APPROOT}" -name "*.dylib")
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" ${dylibFiles}

	# First sign all application binaries
	echo
	echo ---Sign all Datovka framework binaries---
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "$APPROOT/Contents/Frameworks/QtCore.framework/Versions/5/QtCore"
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "$APPROOT/Contents/Frameworks/QtGui.framework/Versions/5/QtGui"
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "$APPROOT/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork"
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "$APPROOT/Contents/Frameworks/QtPrintSupport.framework/Versions/5/QtPrintSupport"
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "$APPROOT/Contents/Frameworks/QtSql.framework/Versions/5/QtSql"
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "$APPROOT/Contents/Frameworks/QtSvg.framework/Versions/5/QtSvg"
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "$APPROOT/Contents/Frameworks/QtWidgets.framework/Versions/5/QtWidgets"

	# Sign whole application
	echo
	echo ---Sign Datovka application---
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" "${APPROOT}"

	# Verify application signature
	echo
	echo ---Verify application signature---
	codesign -dv --verbose=4 "${APPROOT}"
	echo

	cd $SRC_ROOT

	# Close dmg package
	hdiutil detach $DEVICE
	signDmg=${dmg//${DMG_SOURCE_DIR}/${DMG_SIGN_DIR}}
	hdiutil convert -format UDZO -o $signDmg $dmg -shadow
	rm -r "$dmg.shadow"

	# Sign Datovka dmg package
	echo
	echo ---Sign Datovka dmg package---
	codesign --force --verify --verbose --sign "${SIGN_CERT_ID}" $signDmg

	# Verify Datovka dmg signature
	echo
	echo ---Verify dmg signature---
	codesign -dv --verbose=4 $signDmg
	echo
done

#!/usr/bin/env sh

APP_DIR="../datovka.app"
TITLE="datovka installer"
SIZE="50M"
DMG_TMP_FILE="pack.temp.dmg"
finalDMGName="datovka-installer.dmg"

if [ -f "${DMG_TMP_FILE}" ]; then
	rm -rf "${DMG_TMP_FILE}"
fi

if [ -f "${finalDMGName}" ]; then
	rm -rf "${finalDMGName}"
fi

hdiutil create \
	-srcfolder "${APP_DIR}" \
	-volname "${TITLE}" \
	-fs HFS+ \
	-fsargs "-c c=64,a=16,e=16" \
	-format UDRW \
	-size ${SIZE} \
	"${DMG_TMP_FILE}"

DEVICE=$(hdiutil attach -readwrite -noverify -noautoopen "${DMG_TMP_FILE}" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

sleep 5

echo $DEVICE
VOLUME_DIR="/Volumes/${TITLE}"

if [ ! -d "${VOLUME_DIR}" ]; then
	echo "Could not find '${VOLUME_DIR}'"
	exit 1
fi

mkdir "${VOLUME_DIR}/.background"

backgroundPictureName="dmg_background.png"

BG_SRC="../res/${backgroundPictureName}"
if [ ! -f "${BG_SRC}" ]; then
	echo "Cannot find '${BG_SRC}'"
	exit 1
fi
echo cp "${BG_SRC}" "${VOLUME_DIR}/.background/${backgroundPictureName}"
cp "${BG_SRC}" "${VOLUME_DIR}/.background/${backgroundPictureName}"

applicationName="datovka"

echo '
   tell application "Finder"
     tell disk "'${TITLE}'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 100, 885, 430}
           set theViewOptions to the icon view options of container window
           set arrangement of theViewOptions to not arranged
           set icon size of theViewOptions to 72
           set background picture of theViewOptions to file ".background:'${backgroundPictureName}'"
           make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
           set position of item "'${applicationName}'" of container window to {100, 100}
           set position of item "Applications" of container window to {375, 100}
           update without registering applications
           delay 5
           close
     end tell
   end tell
' | osascript


chmod -Rf go-w /Volumes/"${title}"
sync
sync
hdiutil detach "${DEVICE}"
hdiutil convert "${DMG_TMP_FILE}" -format UDZO -imagekey zlib-level=9 -o "${finalDMGName}"
rm -f "${DMG_TMP_FILE}"

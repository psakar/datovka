#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
SRC_ROOT="${SCRIPT_LOCATION}/.."

APP_NAME="datovka"

APP_DIR="${SRC_ROOT}/${APP_NAME}.app"
DMG_TITLE="${APP_NAME} installer"
SIZE="50M"
DMG_TMP_FILE="${SRC_ROOT}/pack.temp.dmg"
DMG_FINAL_NAME="${SRC_ROOT}/${APP_NAME}-installer.dmg"


if [ ! -d "${APP_DIR}" ]; then
	echo "Cannot find '${APP_DIR}'." >&2
	exit 1
fi


# Clear files potentially created by previous run.
if [ -f "${DMG_TMP_FILE}" ]; then
	rm -rf "${DMG_TMP_FILE}"
fi

if [ -f "${DMG_FINAL_NAME}" ]; then
	rm -rf "${DMG_FINAL_NAME}"
fi


# Create the image.
hdiutil create \
	-srcfolder "${APP_DIR}" \
	-volname "${DMG_TITLE}" \
	-fs HFS+ \
	-fsargs "-c c=64,a=16,e=16" \
	-format UDRW \
	-size ${SIZE} \
	"${DMG_TMP_FILE}"

# Mount the image.
DEVICE=$( \
	hdiutil attach \
		-readwrite \
		-noverify \
		-noautoopen \
		"${DMG_TMP_FILE}" | \
	egrep '^/dev/' | sed 1q | awk '{print $1}')


# Let things settle down.
sleep 10


VOLUME_DIR="/Volumes/${DMG_TITLE}"

if [ ! -d "${VOLUME_DIR}" ]; then
	echo "Could not find '${VOLUME_DIR}'." >&2
	hdiutil detach "${DEVICE}"
	exit 1
fi


BG_DIR="${VOLUME_DIR}/.background"
mkdir "${BG_DIR}"
if [ ! -d "${BG_DIR}" ]; then
	echo "Could not create '${BG_DIR}'." >&2
	hdiutil detach "${DEVICE}"
	exit 1
fi

BG_PIC_NAME="dmg_background.png"

BG_SRC="${SRC_ROOT}/res/${BG_PIC_NAME}"
if [ ! -f "${BG_SRC}" ]; then
	echo "Cannot find '${BG_SRC}'" >&2
	hdiutil detach "${DEVICE}"
	exit 1
fi
echo cp "${BG_SRC}" "${BG_DIR}/${BG_PIC_NAME}"
cp "${BG_SRC}" "${BG_DIR}/${BG_PIC_NAME}"


# Check the background image DPI and fix it if it is not 72x72
DMG_BACKGROUND_IMG="${BG_DIR}/${BG_PIC_NAME}"
_BACKGROUND_IMAGE_DPI_H=`sips -g dpiHeight "${DMG_BACKGROUND_IMG}" | grep -Eo '[0-9]+\.[0-9]+'`
_BACKGROUND_IMAGE_DPI_W=`sips -g dpiWidth "${DMG_BACKGROUND_IMG}" | grep -Eo '[0-9]+\.[0-9]+'`

if [ $(echo " $_BACKGROUND_IMAGE_DPI_H != 72.0 " | bc) -eq 1 -o $(echo " $_BACKGROUND_IMAGE_DPI_W != 72.0 " | bc) -eq 1 ]; then
	echo "WARNING: The background image's DPI is not 72. This will result in distorted backgrounds on Mac OS X 10.7+. I will convert it to 72 DPI for you."

	_DMG_BACKGROUND_TMP="${DMG_BACKGROUND_IMG%.*}"_dpifix."${DMG_BACKGROUND_IMG##*.}"

	sips -s dpiWidth 72 -s dpiHeight 72 "${DMG_BACKGROUND_IMG}" --out "${_DMG_BACKGROUND_TMP}"

	#DMG_BACKGROUND_IMG="${_DMG_BACKGROUND_TMP}"
	mv "${_DMG_BACKGROUND_TMP}" "${DMG_BACKGROUND_IMG}"
fi


echo '
   tell application "Finder"
     tell disk "'${DMG_TITLE}'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 100, 885, 430}
           set theViewOptions to the icon view options of container window
           set arrangement of theViewOptions to not arranged
           set icon size of theViewOptions to 72
           set background picture of theViewOptions to file ".background:'${BG_PIC_NAME}'"
           make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
           set position of item "'${APP_NAME}'" of container window to {100, 100}
           set position of item "Applications" of container window to {375, 100}
           update without registering applications
           delay 5
           close
     end tell
   end tell
' | osascript


chmod -Rf go-w "/Volumes/${DMG_TITLE}"
sync
sync
hdiutil detach "${DEVICE}"
hdiutil convert "${DMG_TMP_FILE}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FINAL_NAME}"
rm -f "${DMG_TMP_FILE}"

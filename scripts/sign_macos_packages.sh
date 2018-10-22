#!/usr/bin/env sh

# Identifier of the signing certificate.
SIGN_CERT_ID="Developer ID Application: CZ.NIC, z.s.p.o."

APP_NAME="datovka"
SIG_PREF="signed_"

DMG_TITLE="${APP_NAME} installer"
VOLUME_DIR="/Volumes/${DMG_TITLE}"

ESCAPED_ECHO_CMD="echo"
if [ ! $(uname -s) = "xDarwin" ]; then
	ESCAPED_ECHO_CMD="echo -e"
fi

USAGE=""
USAGE="${USAGE}Usage:\n\t$0 file1 [file2 ...]\n\n"
USAGE="${USAGE}\tScript for signing ${APP_NAME} software packages.\n"
USAGE="${USAGE}\tIt fill create signed counterparts in same location as the original file.\n"
USAGE="${USAGE}\t\t(E.g. for '/tmp/datovka-4.11.0-64bit-osx10.7.dmg' it will create\n"
USAGE="${USAGE}\t\t '/tmp/${SIG_PREF}datovka-4.11.0-64bit-osx10.7.dmg'.)\n"
USAGE="${USAGE}\tIt accepts these file names:\n"
USAGE="${USAGE}\t\t*.dmg - macOS software package\n"
#USAGE="${USAGE}\t\t*.exe - NSIS installer for Windows\n"
#USAGE="${USAGE}\t\t*.msi - MSI installer for Windows\n"
#USAGE="${USAGE}\t\t*.zip - archive containing Windows package\n"

CODESIGN_CMD="codesign"
GATEKEEPR_CMD="spctl"

# Sign command.
SIGN_CMD="${CODESIGN_CMD} --force --verify --deep --verbose --sign"

# Sign verify command.
SIGN_VERIFY_CMD="${CODESIGN_CMD} --verify --deep --strict --verbose=4"

# Sign verify command for gatekeepr.
SIGN_VERIFY_GATEKEEPR="${GATEKEEPR_CMD} -a -t open --context context:primary-signature -v"

# Check presence of codesign command.
codesign_present () {
	local FOUND=$(which "${CODESIGN_CMD}" 2>/dev/null)
	if [ "x${FOUND}" = "x" ]; then
		echo "Cannot find '${CODESIGN_CMD}' executable." >&2
		return 1
	fi
	return 0
}

# Check whether all files are of supported type.
# Returns 0 on success.
have_supported_suffixes () {
	local FILE_LIST="$@"
	if [ "x${FILE_LIST}" = "x" ]; then
		echo "File list is empty." >&2
		return 1
	fi

	local RETVAL="0"

	for FILE in ${FILE_LIST}; do
		local LOWER_CASE_FILE=$(echo "${FILE}" | tr '[:upper:]' '[:lower:]')
		local NO_MATCH=$(echo "${LOWER_CASE_FILE}" | grep -v -e '\.dmg$')
		if [ "x${NO_MATCH}" != "x" ]; then
			echo "File '${FILE}' cannot be signed." >&2
			RETVAL="1"
		fi
	done

	return ${RETVAL}
}

# Check whether files really exist.
files_exist () {
	local FILE_LIST="$@"
	if [ "x${FILE_LIST}" = "x" ]; then
		echo "File list is empty." >&2
		return 1
	fi

	local RETVAL="0"
	for FILE in ${FILE_LIST}; do
		if [ ! -e "${FILE}" ]; then
			echo "'${FILE}' does not exist." >&2
			RETVAL="1"
		elif [ ! -f "${FILE}" ]; then
			echo "'${FILE}' is not a file." >&2
			RETVAL="1"
		fi
	done

	return ${RETVAL}
}

# If path does not start with '/' the it prepends "${PWD}".
realpath () {
	[[ "$1" = /* ]] && echo "$1" || echo "${PWD}/${1#./}"
}

# Return directory of file (may return '.') via stdout.
# Returns empty string on error.
absolute_file_dir () {
	local FILE_PATH="$1"
	if [ "x${FILE_PATH}" = "x" ]; then
		echo ""
		echo "File path is empty." >&2
		return 1
	fi

	dirname $(realpath "${FILE_PATH}")
	return $?
}

# Return file name via stdout.
# Returns empty string on error.
file_name () {
	local FILE_PATH="$1"
	if [ "x${FILE_PATH}" = "x" ]; then
		echo ""
		echo "File path is empty." >&2
		return 1
	fi

	basename "${FILE_PATH}"
	return $?
}

# Return expected signed file path.
signed_absolute_file_path () {
	local FILE_PATH="$1"
	if [ "x${FILE_PATH}" = "x" ]; then
		echo "File path is empty." >&2
		return 1
	fi

	local DIR_NAME=$(absolute_file_dir "${FILE_PATH}")
	local FILE_NAME=$(file_name "${FILE_PATH}")
	echo "${DIR_NAME}/${SIG_PREF}${FILE_NAME}"
}

# Check whether signed counterparts exist in the given location.
# Return 0 if no signed counterpart exist.
signed_already_exists () {
	local FILE_PATH="$1"
	if [ "x${FILE_PATH}" = "x" ]; then
		echo "File path is empty." >&2
		return 1
	fi

	local SIGNED_NAME=$(signed_absolute_file_path "${FILE_PATH}")

	if [ -e "${SIGNED_NAME}" ]; then
		echo "File '${SIGNED_NAME}' already exists." >&2
		return 1
	fi

	return 0
}

# Check whether signed counterparts already exist.
signed_files_dont_exist () {
	local FILE_LIST="$@"
	if [ "x${FILE_LIST}" = "x" ]; then
		echo "File list is empty." >&2
		return 1
	fi

	local RETVAL="0"

	for FILE in ${FILE_LIST}; do
		signed_already_exists "${FILE}"
		if [ "$?" -ne "0" ]; then
			RETVAL="1"
		fi
	done

	return ${RETVAL}
}

# Return all files that match the supplied suffix (case insensitive).
# Files are written to stdout.
insensitive_match_suffix () {
	local SUFFIX="$1"
	local FILE_LIST="$@"

	if [ "x${SUFFIX}" = "x" ]; then
		echo "Expected non-empty suffix." >&2
		return 1
	fi

	local LOWER_CASE_SUFFIX=$(echo "${SUFFIX}" | tr '[:upper:]' '[:lower:]')

	for FILE in ${FILE_LIST}; do
		if [ -f "${FILE}" ]; then
			local LOWER_CASE_FILE=$(echo "${FILE}" | tr '[:upper:]' '[:lower:]')
			local FOUND=$(echo ${LOWER_CASE_FILE} | grep -e "${LOWER_CASE_SUFFIX}$")
			if [ "x${FOUND}" != "x" ]; then
				echo "${FILE}"
			fi
		fi
	done

	return 0
}

# Sign zip archive.
sign_zip_content () {
	local UNSIGNED_ZIP="$1"
	if [ "x${UNSIGNED_ZIP}0" = "x" ]; then
		echo "Expected non-empty zip file name." >&2
		return 1
	fi

	UNSIGNED_ZIP=$(realpath "${UNSIGNED_ZIP}")
	local SIGNED_ZIP=$(signed_absolute_file_path "${UNSIGNED_ZIP}")

	echo "'${UNSIGNED_ZIP}' -> '${SIGNED_ZIP}'"

	local TMP_DIR=$(mktemp -d /tmp/sign_XXXX)
	pushd "${TMP_DIR}" > /dev/null

	unzip ${UNSIGNED_ZIP} -d "./" > /dev/null
	local DIR_NUM=$(ls -d ./*/ | wc -l)
	if [ "${DIR_NUM}" -ne "1" ]; then
		echo "Expected only a single directory within the archive '${UNSIGNED_ZIP}'." >&2
		return 1
	fi

	#local DLL_FILES=$(find ./ -name "*.dll")
	local DLL_FILES=$(insensitive_match_suffix "dll" $(find ./))
	local EXE_FILES=$(insensitive_match_suffix "exe" $(find ./))

	# Sign all libraries and executables.
	${SIGN_CMD} "${SIGN_CERT_ID}" ${DLL_FILES} || return 1
	${SIGN_CMD} "${SIGN_CERT_ID}" ${EXE_FILES} || return 1

	${SIGN_VERIFY_CMD} ${EXE_FILES} || return 1

	zip -9 -r -X "${SIGNED_ZIP}" .

	popd > /dev/null
	rm -r "${TMP_DIR}"

	return 0
}

# Return all files that are not symlinks.
exclude_symlinks () {
	local FILE_LIST="$@"
	for FILE in ${FILE_LIST}; do
		if [ -e "${FILE}" -a ! -L "${FILE}" ]; then
			echo "${FILE}"
		fi
	done
}

# Sign dmg package content that has already been mounter into a directory.
sign_dmg_content () {
	local VOLUME_DIR="$1"
	if [ "x${VOLUME_DIR}" = "x" ]; then
		echo "Expected non-empty volume directory." >&2
		return 1
	fi

	if [ ! -d "${VOLUME_DIR}" ]; then
		echo "'${VOLUME_DIR}' is not a directory." >&2
		return 1
	fi

	pushd "${VOLUME_DIR}" > /dev/null

	local APP_ROOT="${APP_NAME}.app"

	# Sign the whole application.
	${SIGN_CMD} "${SIGN_CERT_ID}" "${APP_ROOT}" || return 1

	# Verify app signature and sign for gatekeepr.
	${SIGN_VERIFY_CMD} "${APP_ROOT}" || return 1
	${SIGN_VERIFY_GATEKEEPR} "${APP_ROOT}" || return 1

	popd > /dev/null

	return 0
}

# Sign content of dmg package and the package itself.
sign_dmg () {
	local UNSIGNED_DMG="$1"
	if [ "x${UNSIGNED_DMG}" = "x" ]; then
		echo "Expected non-empty dmg file name." >&2
		return 1
	fi

	UNSIGNED_DMG=$(realpath "${UNSIGNED_DMG}")
	local SIGNED_DMG=$(signed_absolute_file_path "${UNSIGNED_DMG}")

	local TMP_DIR=$(mktemp -d /tmp/sign_XXXX)
	local TMP_DMG=$(file_name "${UNSIGNED_DMG}")
	TMP_DMG="${TMP_DIR}/${TMP_DMG}"
	cp "${UNSIGNED_DMG}" "${TMP_DMG}" || exit 1 # Temporary copy.

	echo "'${UNSIGNED_DMG}' -> '${SIGNED_DMG}'"

	echo "Opening dmg package '${TMP_DMG}'"
	local DEVICE=$(hdiutil attach -owners on "${TMP_DMG}" -shadow | egrep '^/dev/' | sed 1q | awk '{print $1}')
	if [ "x${DEVICE}" = "x" ]; then
		echo "Error opening dmg package '${TMP_DMG}'." >&2
		return 1
	fi
	echo "Dmg package opened as device '${DEVICE}'."

	sign_dmg_content "${VOLUME_DIR}"

	# The volume must be detached even on errors.
	hdiutil detach "${DEVICE}"
	hdiutil convert -format UDZO -o "${SIGNED_DMG}" "${TMP_DMG}" -shadow || return 1
	rm "${TMP_DMG}" "${TMP_DMG}.shadow"
	rm -r "${TMP_DIR}"

	# Sign whole package.
	${SIGN_CMD} "${SIGN_CERT_ID}" "${SIGNED_DMG}" || return 1

	# Verify package sign and sign for gatekeepr.
	${SIGN_VERIFY_CMD} "${SIGNED_DMG}" || return 1
	${SIGN_VERIFY_GATEKEEPR} "${SIGNED_DMG}" || return 1

	return 0
}

# Sign windows installer file.
sign_win_installer () {
	local UNSIGNED_INST="$1"
	if [ "x${UNSIGNED_INST}" = "x" ]; then
		echo "Expected non-empty dmg file name." >&2
		return 1
	fi

	UNSIGNED_INST=$(realpath "${UNSIGNED_INST}")
	local SIGNED_INST=$(signed_absolute_file_path "${UNSIGNED_INST}")

	local TMP_DIR=$(mktemp -d /tmp/sign_XXXX)
	local TMP_INST=$(file_name "${UNSIGNED_INST}")
	TMP_INST="${TMP_DIR}/${TMP_INST}"
	cp "${UNSIGNED_INST}" "${TMP_INST}" || exit 1 # Temporary copy.

	echo "'${UNSIGNED_INST}' -> '${SIGNED_INST}'"

	${SIGN_CMD} "${SIGN_CERT_ID}" "${TMP_INST}" || return 1
	mv "${TMP_INST}" "${SIGNED_INST}"
	${SIGN_VERIFY_CMD} "${SIGNED_INST}" || return 1

	rm -r "${TMP_DIR}"

	return 0
}

# Sign provided files.
sign_files () {
	local FILE_LIST="$@"
	if [ "x${FILE_LIST}" = "x" ]; then
		echo "File list is empty." >&2
		return 1
	fi

	for FILE in ${FILE_LIST}; do
		local LOWER_CASE_FILE=$(echo "${FILE}" | tr '[:upper:]' '[:lower:]')
		case "${LOWER_CASE_FILE}" in
		*.dmg)
			sign_dmg "${FILE}"
			if [ "$?" -ne "0" ]; then
				echo "Error while signing dmg file '${FILE}'." >&2
				return 1
			fi
			;;
		*.exe|*.msi)
			sign_win_installer "${FILE}"
			if [ "$?" -ne "0" ]; then
				echo "Error while signing Windows installer file '${FILE}'." >&2
				return 1
			fi
			;;
		*.zip)
			sign_zip_content "${FILE}"
			if [ "$?" -ne "0" ]; then
				echo "Error while signing zip file '${FILE}'." >&2
				return 1
			fi
			;;
		*)
			echo "Unsupported file '${FILE}'." >&2
			return 1
			;;
		esac
	done

	return 0
}

# Must provide some arguments.
if [ "$#" -lt "1" ]; then
	${ESCAPED_ECHO_CMD} "${USAGE}" >&2
	exit 1
fi

FILES="$@"

codesign_present || exit 1
have_supported_suffixes ${FILES} || exit 1
files_exist ${FILES} || exit 1
signed_files_dont_exist ${FILES} || exit 1
sign_files ${FILES} || exit 1

echo OK

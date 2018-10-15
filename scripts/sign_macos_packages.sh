#!/usr/bin/env sh

# Identifier of the signing certificate.
SIGN_CERT_ID="CZ.NIC, z.s.p.o."

APP_NAME="datovka"
SIG_PREF="signed_"

ESCAPED_ECHO_CMD="echo"
if [ ! $(uname -s) = "xDarwin" ]; then
	ESCAPED_ECHO_CMD="echo -e"
fi

USAGE=""
USAGE="${USAGE}Usage:\n\t$0 file1 [file2 ...]\n\n"
USAGE="${USAGE}\tScript for signing ${APP_NAME} software packages.\n"
USAGE="${USAGE}\tIt fill create signed counterparts in same location as the original file.\n"
USAGE="${USAGE}\t\t(E.g. for '/tmp/datovka-4.11.0-windows.zip' it will create\n"
USAGE="${USAGE}\t\t '/tmp/${SIG_PREF}datovka-4.11.0-windows.zip'.)\n"
USAGE="${USAGE}\tIt accepts these file names:\n"
USAGE="${USAGE}\t\t*.dmg - macOS software package\n"
USAGE="${USAGE}\t\t*.exe - NSIS installer for Windows\n"
USAGE="${USAGE}\t\t*.zip - archive containing Windows package\n"

CODESIGN_CMD="codesign"

# Sign command.
SIGN_CMD="${CODESIGN_CMD} --force --verify --verbose --sign"

# Sign verify command.
SIGN_VERIFY_CMD="${CODESIGN_CMD} -dv --verbose=4"

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
		local NO_MATCH=$(echo "${LOWER_CASE_FILE}" | grep -v -e '\.dmg$' -e '\.exe$' -e '\.zip$')
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
	SIGNED_ZIP=$(signed_absolute_file_path "${UNSIGNED_ZIP}")

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
	${SIGN_CMD} "${SIGN_CERT_ID}" ${DLL_FILES}
	${SIGN_CMD} "${SIGN_CERT_ID}" ${EXE_FILES}

	${SIGN_VERIFY_CMD} "${SIGN_CERT_ID}" ${EXE_FILES}

	zip -9 -r -X "${SIGNED_ZIP}" .

	popd > /dev/null
	#rm -r "${TMP_DIR}"
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
		*.zip)
			sign_zip_content "${FILE}"
			if [ "$?" -ne "0" ]; then
				echo "Error while sizning zip file '${FILE}'." >&2
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

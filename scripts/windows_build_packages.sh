#!/usr/bin/env sh

# Obtain location of source root.
src_root () {
	local SCRIPT_LOCATION=""
	local SYSTEM=$(uname -s)
	if [ ! "x${SYSTEM}" = "xDarwin" ]; then
		local SCRIPT=$(readlink -f "$0")
		SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))
	else
		SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
	fi

	echo $(cd "$(dirname "${SCRIPT_LOCATION}")"; pwd)
}

SRC_ROOT=$(src_root)

if [ "x${GETOPT}" = "x" ]; then
	GETOPT="getopt"
fi

APP=""
BUNDLE=""
PKG_VERSION=""
DFLT_PKG_VERSION="0.0.0"
VARIANT=""

USAGE="Usage:\n\t$0 [options]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t-a NAME, --app NAME\n\t\tSupply app executable name (without trailing .exe).\n"
USAGE="${USAGE}\t-b NAME, --bundle NAME\n\t\tSupply bundle name.\n"
USAGE="${USAGE}\t-h, --help\n\t\tPrints help message.\n"
USAGE="${USAGE}\t-p VERSION, --pkg-version VERSION\n\t\tApplication version. Default is '${DFLT_PKG_VERSION}'.\n"
USAGE="${USAGE}\t-v VARIANT, --variant VARIANT\n\t\tBuild variant (home-dir-data or portable-data).\n"

# Return 0 if param is a directory.
directory_exists () {
	local DIR="$1"
	if [ "x${DIR}" = "x" ]; then
		echo "Missing parameter." >&2
		return 1
	fi
	if [ ! -e "${DIR}" ]; then
		echo "'${DIR}' does not exist." >&2
		return 1
	fi
	if [ ! -d "${DIR}" ]; then
		echo "'${DIR}' is not a directory." >&2
		return 1
	fi
	return 0
}

# Try to locate the executable in supplied arguments.
get_exe_path () {
	local EXE_NAME="$1" # First arduments is the excutable name.
	# Remaining arguments may be full paths or directories.
	shift

	for P in "$@"; do
		if [ -e "${P}" ]; then
			local FOUND=$(echo "${P}" | grep "${EXE_NAME}")
			if [ "x${FOUND}" != "x" ]; then
				# The executable may be found.
				echo "${P}"
				return 0
			fi
		elif [ -d "${P}" -a -e "${P}/${EXE_NAME}" ]; then
			echo "${P}/${EXE_NAME}"
			return 0
		fi
	done

	# Search in PATH.
	local FOUND_IN_PATH=$(which "${EXE_NAME}")
	if [ "x${FOUND_IN_PATH}" != "x" ]; then
		echo "${FOUND_IN_PATH}"
		return 0
	fi

	# Nothing found.
	echo ""
	return 1
}

# Return path to 7z.exe.
# The user may specify the variable LOC_7Z variable to provide the path to
# the executable.
exe_7z_path () {
	local DFLT_LOC_7Z="c:/Program Files/7-Zip/7z.exe"
	local DFLT_LOC_7Z_32="c:/Program Files (x86)/7-Zip/7z.exe"

	local EXE_7Z=""
	EXE_7Z=$(get_exe_path "7z.exe" "${LOC_7Z}" "${DFLT_LOC_7Z}" "${DFLT_LOC_7Z_32}")

	if [ "x${EXE_7Z}" = "x" ]; then
		echo ""
		echo "Cannot locate 7z.exe. Please set the 'LOC_7Z' variable to the proper executable." >&2
		return 1
	fi
	echo "${EXE_7Z}"
	return 1
}

# Create zip package.
create_zip_package () {
	local TGT_ZIP_NAME="$1"
	local TGT_ZIP_ROOT_NAME="$2"
	local SRC_DIR_ROOT="$3"

	if [ "x${TGT_ZIP_NAME}" = "x" -o "x${TGT_ZIP_ROOT_NAME}" = "x" -o "x${SRC_DIR_ROOT}" = "x" ]; then
		echo "Missing parameter." >&2
		return 1
	fi

	if [ ! -d "${SRC_DIR_ROOT}" ]; then
		echo "'${SRC_DIR_ROOT}' is not a directory." >&2
		return 1
	fi

	local EXE_7Z=$(exe_7z_path)

	rm -r "${TGT_ZIP_ROOT_NAME}"
	rm "${TGT_ZIP_ROOT_NAME}.zip"
	cp -r "${SRC_DIR_ROOT}" "${TGT_ZIP_ROOT_NAME}"

	"${EXE_7Z}" a -tzip "${TGT_ZIP_NAME}" "${TGT_ZIP_ROOT_NAME}"
	rm -r "${TGT_ZIP_ROOT_NAME}"
}

if ! "${GETOPT}" -l test: -u -o t: -- --test test > /dev/null; then
	echo "The default getopt does not support long options." >&2
	echo "You may provide such getopt version via the GETOPT variable e.g.:" >&2
	echo "GETOPT=/opt/local/bin/getopt $0" >&2
	exit 1
fi

# Parse rest of command line
set -- $("${GETOPT}" -l app:,bundle:,help,variant:,pkg-version: -u -o a:b:hp:v: -- "$@")
if [ $# -lt 1 ]; then
	echo ${USAGE} >&2
	exit 1
fi
while [ $# -gt 0 ]; do
	PARAM="$1"
	case "${PARAM}" in
	-a|--app)
		if [ "x${APP}" = "x" ]; then
			APP="$2"
		else
			echo "App is already set." >&2
			exit 1
		fi
		shift
		;;
	-b|--bundle)
		if [ "x${BUNDLE}" = "x" ]; then
			BUNDLE="$2"
		else
			echo "Bundle is already set." >&2
			exit 1
		fi
		shift
		;;
	-h|--help)
		echo -e ${USAGE}
		exit 0
		;;
	-p|--pkg-version)
		if [ "x${PKG_VERSION}" = "x" ]; then
			PKG_VERSION="$2"
		else
			echo "Version already specified." >&2
			exit
		fi
		shift
		;;
	-v|--variant)
		if [ "x${VARIANT}" = "x" ]; then
			VARIANT_PARAM="$2"
			case "${VARIANT_PARAM}" in
			home-dir-data|portable-data)
				VARIANT="${VARIANT_PARAM}"
				;;
			*)
				echo "Unknown variant '${VARIANT_PARAM}'." >&2
				exit 1
				;;
			esac
			unset VARIANT_PARAM
		else
			echo "Variant is already set." >&2
			exit 1
		fi
		shift
		;;
	--)
		shift
		break
		;;
	-*|*)
		echo "Unknown option '${PARAM}'." >&2
		echo -e ${USAGE} >&2
		exit 1
		;;
	esac
	unset PARAM
	shift
done
if [ $# -gt 0 ]; then
	echo "Unknown options: $@" >&2
	echo -e ${USAGE} >&2
	exit 1
fi

# App name must be set.
if [ "x${APP}" = "x" ]; then
	echo "Missing app name." >&2
	exit 1
fi

# Bundle name must be set.
if [ "x${BUNDLE}" = "x" ]; then
	echo "Missing bundle name." >&2
	exit 1
fi

# Variant must be set.
if [ "x${VARIANT}" = "x" ]; then
	echo "Missing variant identifier." >&2
	exit 1
fi

# Using default app version.
if [ "x${PKG_VERSION}" = "x" ]; then
	PKG_VERSION="${DFLT_PKG_VERSION}"
fi

directory_exists "${BUNDLE}" || exit 1

case "${VARIANT}" in
home-dir-data)
	echo "Variant '${VARIANT}' not supported yet." >&2
	exit 1
	;;
portable-data)
	APP=$(echo "${APP}" | sed -e 's/-portable//g') # Remove the '-portable' from the name.
	create_zip_package "${APP}-portable-${PKG_VERSION}-windows.zip" "${APP}-${PKG_VERSION}-portable" "${BUNDLE}" || exit 1
	;;
*)
	echo "Unknown variant '${VARIANT}'." >&2
	exit 1
esac

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

MAKE_OPTS="-j 4"

USAGE="Usage:\n\t$0 -s SDK_VERSION_NUMBER [options]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t-D\n\t\tDon't compile, just build the package. This implies -d.\n"
USAGE="${USAGE}\t-h, --help\n\t\tPrints help message.\n"
USAGE="${USAGE}\t--shared (default)\n\t\tCompile with shared libraries.\n"
USAGE="${USAGE}\t--i386 (default)\n\t\tCompile for i386 architecture.\n"
USAGE="${USAGE}\t--debug\n\t\tCompile in debug mode.\n"
USAGE="${USAGE}\t--release (default)\n\t\tCompile in release mode.\n"

cd "${SRC_ROOT}"

COMPILE_SRC="yes"
BUILD_PACKAGE="no"

BUILD_TYPE=""
BUILD_SHARED="shared"
BUILD_STATIC="static"

ARCH_NAME=""
ARCH_I386="i386"
ARCH_X86_64="x86_64"

MODE=""
MODE_DEBUG="debug"
MODE_RELEASE="release"

if ! "${GETOPT}" -l test: -u -o t: -- --test test > /dev/null; then
	echo "The default getopt does not support long options." >&2
	echo "You may provide such getopt version via the GETOPT variable e.g.:" >&2
	echo "GETOPT=/opt/local/bin/getopt $0" >&2
	exit 1
fi

# Parse rest of command line
set -- $("${GETOPT}" -l help -l shared -l i386 -l debug -l release -u -o dDh -- "$@")
if [ $# -lt 1 ]; then
	echo -e ${USAGE} >&2
	exit 1
fi

while [ $# -gt 0 ]; do
	case "$1" in
	-D)
		if [ "x${COMPILE_SRC}" = "xyes" ]; then
			COMPILE_SRC=no
			BUILD_PACKAGE=yes
		else
			echo "Option -n already set." >&2
			exit 1
		fi
		;;
	-h|--help)
		echo -e ${USAGE}
		exit 0
		;;
	--shared)
		if [ "x${BUILD_TYPE}" = "x" ]; then
			BUILD_TYPE="${BUILD_SHARED}"
		else
			echo "Build type already specified or in conflict." >&2
			exit 1
		fi
		;;
	--i386)
		if [ "x${ARCH_NAME}" = "x" ]; then
			ARCH_NAME="${ARCH_I386}"
		else
			echo "Architecture already specified or in conflict." >&2
			exit 1
		fi
		;;
	--debug)
		if [ "x${MODE}" = "x" ]; then
			MODE="${MODE_DEBUG}"
		else
			echo "Build mode already specified or in conflict." >&2
			exit 1
		fi
		;;
	--release)
		if [ "x${MODE}" = "x" ]; then
			MODE="${MODE_RELEASE}"
		else
			echo "Build mode already specified or in conflict." >&2
			exit 1
		fi
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
	shift
done
if [ $# -gt 0 ]; then
	echo -e "Unknown options: $@" >&2
	echo -en ${USAGE} >&2
	exit 1
fi

# Use shared libraries by default.
if [ "x${BUILD_TYPE}" = "x" ]; then
	BUILD_TYPE="${BUILD_SHARED}"
fi

# Use i386 by default.
if [ "x${ARCH_NAME}" = "x" ]; then
	ARCH_NAME="${ARCH_I386}"
fi

# Use release by default.
if [ "x${MODE}" = "x" ]; then
	MODE="${MODE_RELEASE}"
fi

APP_NAME="datovka"
PKG_VER=$(cat pri/version.pri | grep '^VERSION\ =\ ' | sed -e 's/VERSION\ =\ //g')
APP_DIR="${APP_NAME}.built"

PROJECT_FILE="${APP_NAME}.pro"

CLI_APP_NAME="datovka-cli"
CLI_PROJECT_FILE="${CLI_APP_NAME}.pro.noauto"

if [ "x${COMPILE_SRC}" = "xyes" ]; then
	QMAKE="qmake.exe"
	LRELEASE="lrelease.exe"
	WINDEPLOYQT="windeployqt.exe"
	MAKE="mingw32-make.exe"

	# Test command presence.
	for CMD in "${LRELEASE}" "${QMAKE}"; do
		LOCATION=$(which "${CMD}")
		if [ "x${LOCATION}" = "x" ]; then
			echo "Cannot find executable '${CMD}'." >&2
			exit 1
		fi

		unset LOCATION
	done

	LIBS_DIR="shared_built"
	LIBS_PATH="libs/${LIBS_DIR}"
	# Test the presence of build libraries.
	if [ ! -d "${SRC_ROOT}/$LIBS_PATH" ]; then
		echo "Directory '${SRC_ROOT}/$LIBS_PATH' does not exist." >&2
		exit 1
	fi

	STATIC="0"
	if [ "x${BUILD_TYPE}" = "x${BUILD_STATIC}" ]; then
		STATIC="1"
		echo "This script does not support static builds." >&2
		exit 1
	fi

	rm -rf debug release

	${LRELEASE} "${PROJECT_FILE}"
	DEBUG_INFO_OPT=""
	#if [ "x${BUILD_TYPE}" = "x${BUILD_SHARED}" ]; then
	#	# https://stackoverflow.com/a/35704181
	#	DEBUG_INFO_OPT="CONFIG+=force_debug_info"
	#fi
	echo ${QMAKE} CONFIG+="${MODE}" WITH_BUILT_LIBS=1 STATIC="${STATIC}" ${DEBUG_INFO_OPT} "${PROJECT_FILE}" -r -spec win32-g++
	${QMAKE} CONFIG+="${MODE}" WITH_BUILT_LIBS=1 STATIC="${STATIC}" ${DEBUG_INFO_OPT} "${PROJECT_FILE}" -r -spec win32-g++
	${MAKE} clean
	${MAKE} ${MAKE_OPTS} || exit 1

	# ${MODE} contains 'debug' or 'release' which are both directories.
	rm -rf "${APP_DIR}"
	mkdir -p "${APP_DIR}"
	cp "${MODE}/${APP_NAME}.exe" "${APP_DIR}/" || exit 1

	# Build CLI version.
	${QMAKE} CONFIG+="${MODE}" WITH_BUILT_LIBS=1 STATIC="${STATIC}" ${DEBUG_INFO_OPT} "${CLI_PROJECT_FILE}" -r -spec win32-g++
	${MAKE} clean
	${MAKE} ${MAKE_OPTS} || exit 1
	cp "${MODE}/${CLI_APP_NAME}.exe" "${APP_DIR}/" || exit 1

	if [ "x${BUILD_TYPE}" = "x${BUILD_SHARED}" ]; then
		"${SRC_ROOT}"/scripts/windows_bundle_shared_libs.sh -a "${APP_NAME}" -b "${APP_DIR}" -m ${MODE} --deployqt "${WINDEPLOYQT}" || exit 1
	fi
fi

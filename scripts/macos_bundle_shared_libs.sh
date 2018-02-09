#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
SRC_ROOT=$(cd "${SCRIPT_LOCATION}"/..; pwd)

APP="datovka"
BUNDLE=""
DFLT_BUNDLE="${APP}.app"

USAGE="Usage:\n\t$0\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t-b NAME, --bundle NAME\n\t\tSupply bundle name. Default is '${DFLT_BUNDLE}'.\n"
USAGE="${USAGE}\t-h, --help\n\t\tPrints help message.\n"

cd "${SRC_ROOT}"

# Return 0 if param is a directory.
directory_exists () {
	DIR=$1
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

# Return 0 if param is an executable file.
executable_exists () {
	EXE=$1
	if [ "x${EXE}" = "x" ]; then
		echo "Missing parameter." >&2
		return 1
	fi
	if [ ! -e "${EXE}" ]; then
		echo "'${EXE}' does not exist." >&2
		return 1
	fi
	if [ ! -f "${EXE}" ]; then
		echo "'${EXE}' is not a file." >&2
		return 1
	fi
	if [ ! -x "${EXE}" ]; then
		echo "'${EXE}' is not executable." >&2
		return 1
	fi
	return 0
}

# Return list of dynamic libraries.
dylibs () {
	APP="$1"
	if [ "x${APP}" = "x" ]; then
		echo ""
		return 1
	fi

	# First sed expression contains a '\t' character.
	otool -L "${APP}" | grep '^[ 	]' | grep "${DIR_LIBS}" | sed -e 's/^[ 	]*//g' -e 's/\(^.*dylib\)\(.*\)$/\1/g'
}

# Return list of Qt frameworks.
qt_frameworks () {
	APP="$1"
	if [ "x${APP}" = "x" ]; then
		echo ""
		return 1
	fi

	# First sed expression contains a '\t' character.
	otool -L "${APP}" | grep '^[ 	]' | grep 'Qt[^.]*.framework' | sed -e 's/^[ 	]*//g' -e 's/[ ](compat.*$//g'
}

# Return list of frameworks without leading @rpath.
qt_frameworks_strip () {
	FRAMEWORKS="$1"
	echo "${FRAMEWORKS}" | sed -e 's/@rpath[/]//g'
}

# Return non-empty location of the Qt frameworks.
qt_frameworks_location () {
	EXE="$1"
	FRAMEWORKS="$2"
	if [ "x${EXE}" = "x" -o "x${FRAMEWORKS}" = "x" ]; then
		echo ""
		return 1
	fi
	if [ ! -x "${EXE}" ]; then
		echo ""
		return 1
	fi

	PATHS=$(otool -l "${EXE}" | grep path | grep -v rpath | sed -e 's/\(^[^/]*path[^/]*\)//g' -e 's/[ ](.*$//g')

	for P in ${PATHS}; do
		for F in ${FRAMEWORKS}; do
			if [ ! -e "${P}/${F}" ]; then
				P=""
				break
			fi
		done
		if [ "x${P}" != "x" ]; then
			# Found location where all frameworks reside.
			echo "${P}"
			return 0
		fi
	done

	# Nothing found.
	echo ""
	return 1
}

# Get all Qt frameworks including those that are dependencies of other frameworks.
qt_frameworks_all () {
	LOC="$1"
	FRAMEWORKS="$2"
	if [ "x${LOC}" = "x" -o "x${FRAMEWORKS}" = "x" ]; then
		echo ""
		return 1
	fi

	FRAMEWORKS=$(echo "${FRAMEWORKS}" | sort -u)
	OUT_FRAMEWORKS=""

	while [ "x${FRAMEWORKS}" != "x${OUT_FRAMEWORKS}" ]; do
		if [ "x${OUT_FRAMEWORKS}" != "x" ]; then
			FRAMEWORKS=$(echo "${OUT_FRAMEWORKS}" | sed -u)
		fi
		OUT_FRAMEWORKS=""

		for F in ${FRAMEWORKS}; do
			FOUND_FWS=$(qt_frameworks "$LOC/${F}")
			if [ "x${FOUND_FWS}" = "x" ]; then
				# Something must be resent.
				echo ""
				return 1
			fi
			STRIPPED_FWS=$(qt_frameworks_strip "${FOUND_FWS}")
			if [ "x${STRIPPED_FWS}" = "x" ]; then
				echo ""
				return 1
			fi
			if [ "x${OUT_FRAMEWORKS}" = "x" ]; then
				OUT_FRAMEWORKS="${STRIPPED_FWS}"
			else
				OUT_FRAMEWORKS="${OUT_FRAMEWORKS}\n${STRIPPED_FWS}"
			fi
		done

		OUT_FRAMEWORKS=$(echo "${OUT_FRAMEWORKS}" | sort -u)
	done

	echo "${FRAMEWORKS}"
	return 0
}

# Parse rest of command line
set -- `getopt -l bundle:,help -u -o b:h -- "$@"`
if [ $# -lt 1 ]; then
	echo ${USAGE} >&2
	exit 1
fi
while [ $# -gt 0 ]; do
	PARAM="$1"
	case "${PARAM}" in
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
		echo ${USAGE}
		exit 0
		;;
	--)
		shift
		break
		;;
	-*|*)
		echo "Unknown option '${PARAM}'." >&2
		echo ${USAGE} >&2
		exit 1
		;;
	esac
	unset PARAM
	shift
done
if [ $# -gt 0 ]; then
	echo "Unknown options: $@" >&2
	echo ${USAGE} >&2
	exit 1
fi

# Use default bundle name if not specified.
if [ "x${BUNDLE}" = "x" ]; then
	BUNDLE="${DFLT_BUNDLE}"
fi

DIR_LIBS="${SRC_ROOT}/libs"
DIR_BUNDLE="${SRC_ROOT}/${BUNDLE}"
DIR_CONTENTS="${DIR_BUNDLE}/Contents"
DIR_MACOS="${DIR_CONTENTS}/MacOs"
FILE_APP="${DIR_MACOS}/${APP}"

directory_exists "${DIR_LIBS}" || exit 1
directory_exists "${DIR_BUNDLE}" || exit 1
directory_exists "${DIR_CONTENTS}" || exit 1
directory_exists "${DIR_MACOS}" || exit 1
executable_exists "${FILE_APP}" || exit 1

# See Qt for macOS - Deployment document.
# https://stackoverflow.com/a/38291080
# http://thecourtsofchaos.com/2013/09/16/how-to-copy-and-relink-binaries-on-osx/

DYLIBS=$(dylibs "${FILE_APP}")
QT_FRAMEWORKS=$(qt_frameworks "${FILE_APP}")
QT_FRAMEWORKS_WITH_RPATH=$(echo "${QT_FRAMEWORKS}" | grep @rpath)

# Test whether path to qt framework is set using an @rpath.
TEST_RPATH="yes"
if [ "x${TEST_RPATH}" = "xyes" ]; then
	FW_NUM=$(echo "${QT_FRAMEWORKS}" | wc -l)
	RPATH_FW_NUM=$(echo "${QT_FRAMEWORKS_WITH_RPATH}" | wc -l)
	if [ "x${FW_NUM}" != "x${RPATH_FW_NUM}" ]; then
		echo "Path to Qt framework is not set using @rpath."
		exit 1
	fi
fi

QT_FRAMEWORKS=$(qt_frameworks_strip "${QT_FRAMEWORKS}")

QT_FRAMEWORK_LOC=$(qt_frameworks_location "${FILE_APP}" "${QT_FRAMEWORKS}")
if [ "x${QT_FRAMEWORK_LOC}" = "x" ]; then
	echo "Could not locate Qt framework."
fi

qt_frameworks_all "${QT_FRAMEWORK_LOC}" "${QT_FRAMEWORKS}"

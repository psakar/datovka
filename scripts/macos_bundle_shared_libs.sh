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

# Returns 0 if number of held lines is equal in both variables.
line_number_match () {
	VAR1="$1"
	VAR2="$2"
	if [ "x${VAR1}" = "x" -o "x${VAR2}" = "x" ]; then
		echo "Received empty paramater." >&2
		return 1
	fi
	LINENUM1=$(echo "${VAR1}" | wc -l)
	LINENUM2=$(echo "${VAR2}" | wc -l)
	if [ "x${LINENUM1}" != "x${LINENUM2}" ]; then
		echo "Line number does not match." >&2
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

# Return non-empty location of dynamic libraries.
dylibs_location () {
	DYLIBS="$1"
	if [ "x${DYLIBS}" = "x" ]; then
		echo ""
		echo "No shared libraries passed." >&2
		return 1
	fi

	# Only one line must remain.
	LOCATION=$(echo "${DYLIBS}" | sed -e 's/[/][^/]*\.dylib//g' | sort -u)
	LOCATION_NUM=$(echo "${LOCATION}" | wc -l)
	if [ "${LOCATION_NUM}" -ne "1" -o "x${LOCATION}" = "x" ]; then
		echo ""
		echo "Could not determine unique location of locally built libraries." >&2
		return 1
	fi

	echo "${LOCATION}"
	return 0
}

# Return list of libraries without leading path.
dylibs_strip () {
	DYLIBS="$1"
	echo "${DYLIBS}" | sed -e 's/^.*[/]//g'
}

# Add libssl to the list of libraries.
dylib_add_ssl () {
	LOC="$1"
	DYLIBS="$2"
	if [ "x${LOC}" = "x" -o "x${DYLIBS}" = "x" ]; then
		echo ""
		return 1
	fi

	SSL_LIBS=$(ls "${LOC}/" | grep libssl)
	FOUND=""
	for S in ${SSL_LIBS}; do
		# Ignore symbolic links.
		if [ ! -L "${LOC}/${S}" ]; then
			if [ "x${FOUND}" = "x" ]; then
				FOUND="${S}"
			else
				FOUND="${FOUND}\n${S}"
			fi
		fi
	done

	if [ "x${FOUND}" = "x" ]; then
		echo ""
		return 1
	fi

	echo "${FOUND}\n${DYLIBS}"
	return 0
}

# Get all Qt frameworks including those that are dependencies of other frameworks.
dylibs_all () {
	LOC="$1"
	DYLIBS="$2"
	if [ "x${LOC}" = "x" -o "x${DYLIBS}" = "x" ]; then
		echo ""
		return 1
	fi

	DYLIBS=$(echo "${DYLIBS}" | sort -u)
	OUT_DYLIBS=""

	while [ "x${DYLIBS}" != "x${OUT_DYLIBS}" ]; do
		if [ "x${OUT_DYLIBS}" != "x" ]; then
			DYLIBS=$(echo "${OUT_DYLIBS}" | sort -u)
		fi
		OUT_DYLIBS=""

		for D in ${DYLIBS}; do
			FOUND_DLS=$(dylibs "${LOC}/${D}")
			if [ "x${FOUND_DLS}" = "x" ]; then
				# Something must be present.
				echo ""
				return 1
			fi
			STRIPPED_DLS=$(dylibs_strip "${FOUND_DLS}")
			if [ "x${STRIPPED_DLS}" = "x" ]; then
				echo ""
				return 1
			fi
			if [ "x${OUT_DYLIBS}" = "x" ]; then
				OUT_DYLIBS="${STRIPPED_DLS}"
			else
				OUT_DYLIBS="${OUT_DYLIBS}\n${STRIPPED_DLS}"
			fi
		done

		OUT_DYLIBS=$(echo "${OUT_DYLIBS}" | sort -u)
	done

	echo "${DYLIBS}"
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
			FRAMEWORKS=$(echo "${OUT_FRAMEWORKS}" | sort -u)
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

# Get Qt frameworks which the application directly depened on.
DYLIBS=$(dylibs "${FILE_APP}")
DYLIBS_BUILT=$(echo "${DYLIBS}" | grep "${SRC_ROOT}")
QT_FRAMEWORKS=$(qt_frameworks "${FILE_APP}")
QT_FRAMEWORKS_WITH_RPATH=$(echo "${QT_FRAMEWORKS}" | grep @rpath)


# Double check whether local libraries are present.
if ! line_number_match "${DYLIBS}" "${DYLIBS_BUILT}"; then
	echo "Something is wrong with locally built libraries."
	exit 1
fi

DYLIBS_LOC=$(dylibs_location "${DYLIBS}")
if [ "x${DYLIBS_LOC}" = "x" ]; then
	echo "Could not locate built shared libraries." >&2
	exit 1
fi

DYLIBS=$(dylibs_strip "${DYLIBS}")

DYLIBS=$(dylib_add_ssl "${DYLIBS_LOC}" "${DYLIBS}")

# Get all shared libraries including those which other depend on.
DYLIBS=$(dylibs_all "${DYLIBS_LOC}" "${DYLIBS}")


# Test whether path to qt framework is set using an @rpath.
TEST_RPATH="yes"
if [ "x${TEST_RPATH}" = "xyes" ]; then
	if ! line_number_match "${QT_FRAMEWORKS}" "${QT_FRAMEWORKS_WITH_RPATH}"; then
		echo "Path to Qt framework is not set using @rpath."
		exit 1
	fi
fi

QT_FRAMEWORKS=$(qt_frameworks_strip "${QT_FRAMEWORKS}")

QT_FRAMEWORK_LOC=$(qt_frameworks_location "${FILE_APP}" "${QT_FRAMEWORKS}")
if [ "x${QT_FRAMEWORK_LOC}" = "x" ]; then
	echo "Could not locate Qt framework." >&2
	exit 1
fi

# Get all Qt frameworks including those which other depend on.
QT_FRAMEWORKS=$(qt_frameworks_all "${QT_FRAMEWORK_LOC}" "${QT_FRAMEWORKS}")

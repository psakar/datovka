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
DFLT_BUNDLE="app.built"
MODE=""
DFLT_MODE="release"
DEPLOYQT_EXE=""
DFLT_DEPLOYQT_EXE="windeployqt.exe"

USAGE="Usage:\n\t$0\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t-a NAME, --app NAME\n\t\tSupply app executable name (without trailing .exe).\n"
USAGE="${USAGE}\t-b NAME, --bundle NAME\n\t\tSupply bundle name. Default is '${DFLT_BUNDLE}'.\n"
USAGE="${USAGE}\t-h, --help\n\t\tPrints help message.\n"
USAGE="${USAGE}\t-m MODE, --mode MODE\n\t\tBuild mode (debug or release). Default is '${DFLT_MODE}'.\n"
USAGE="${USAGE}\t--deployqt EXECUTABLE\n\t\tName (can be full path) of the Qt deployment executable. Default is '${DFLT_DEPLOYQT_EXE}'.\n"

cd "${SRC_ROOT}"

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

# Return 0 if param is an executable file.
executable_exists () {
	local EXE="$1"
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

# Copy Qt configuration file.
qt_conf_copy () {
	local RES_DIR="$1"
	local CONF_SRC="$2"
	if [ "x${RES_DIR}" = "x" -o "x${CONF_SRC}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	if [ ! -f "${CONF_SRC}" ]; then
		echo "'${CONF_SRC}' is not a file." >&2
		return 1
	fi

	if [ ! -d "${RES_DIR}" ]; then
		echo "'${RES_DIR}' is not a directory." >&2
		return 1
	fi

	cp "${CONF_SRC}" "${RES_DIR}/qt.conf"
	if [ "$?" != "0" ]; then
		return 1
	fi

	return 0
}

# Copy localisation files.
locale_copy () {
	local LOCALE_DIR="$1"
	local LOCALE_SRC="$2"
	if [ "x${LOCALE_DIR}" = "x" -o "x${LOCALE_SRC}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	if [ ! -d "${LOCALE_SRC}" ]; then
		echo "'${LOCALE_SRC}' is not a directory." >&2
		return 1
	fi

	if [ ! -d "${LOCALE_DIR}" ]; then
		echo "'${LOCALE_DIR}' is not a directory." >&2
		return 1
	fi

	cp "${LOCALE_SRC}"/*.qm "${LOCALE_DIR}/"
	if [ "$?" != "0" ]; then
		return 1
	fi

	return 0
}

# Return just the name of the library.
dylibs_name () {
	local DYLIBS="$1"
	for DYLIB in ${DYLIBS}; do
		echo "${DYLIB}" | sed -e 's/[.].*dll//g'
	done
}

# Copy shared libraries into bundle.
dylibs_copy () {
	local TGT_LOC="$1"
	local SRC_LOC="$2"
	local DLS="$3"
	if [ "x${TGT_LOC}" = "x" -o "x${SRC_LOC}" = "x" -o "x${DLS}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	# Copy shared libraries files and links.
	local DL_NAMES=$(dylibs_name "${DLS}")
	for D in ${DL_NAMES}; do
		local FILES=$(ls "${SRC_LOC}/${D}"*.dll)
		for F in ${FILES}; do
			cp -R "${F}" "${TGT_LOC}/"
			if [ "$?" != "0" ]; then
				rm -rf "${TGT_LOC}/"
				return 1
			fi
		done
	done

	return 0
}

# Check presence of subdirectories in the application bundle.
have_only_directories () {
	local BASE_DIR="$1"
	shift
	local PASSED_DIRS="$@"
	# Gen only directory names.
	local FOUND_DIRS=$(ls -d "${BASE_DIR}"/*/ | sed -e 's/[/]*$//g' -e 's/.*[/]//g')

	for PASSED_DIR in ${PASSED_DIRS}; do
		HAVE=$(printf "${FOUND_DIRS}" | grep "^${PASSED_DIR}$") # Must use printf here.
		if [ "x${HAVE}" = "x" ]; then
			echo "Directory '${PASSED_DIR}' was not found." >&2
			return 1
		fi
		unset HAVE
	done

	local HAVE_LIST=$(for DIR in ${PASSED_DIRS}; do echo ${DIR}; done) # Separate every entry on a single line.
	for FOUND_DIR in ${FOUND_DIRS}; do
		HAVE=$(printf "${HAVE_LIST}" | grep "^${FOUND_DIR}$") # Must use printf here.
		if [ "x${HAVE}" = "x" ]; then
			echo "Directory '${FOUND_DIR}' is not expected." >&2
			return 1
		fi
		unset HAVE
	done

	return 0
}

if ! "${GETOPT}" -l test: -u -o t: -- --test test > /dev/null; then
	echo "The default getopt does not support long options." >&2
	echo "You may provide such getopt version via the GETOPT variable e.g.:" >&2
	echo "GETOPT=/opt/local/bin/getopt $0" >&2
	exit 1
fi

# Parse rest of command line
set -- $("${GETOPT}" -l app:,bundle:,help,mode:,deployqt: -u -o a:b:hm: -- "$@")
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
	-m|--mode)
		if [ "x${MODE}" = "x" ]; then
			MODE_PARAM="$2"
			case "${MODE_PARAM}" in
			debug|release)
				MODE="${MODE_PARAM}"
				;;
			*)
				echo "Unknown mode '${MODE_PARAM}'." >&2
				exit 1
				;;
			esac
		else
			echo "Mode is already set." >&2
			exit 1
		fi
		shift
		;;
	--deployqt)
		if [ "x${DEPLOYQT_EXE}" = "x" ]; then
			DEPLOYQT_EXE="$2"
		else
			echo "Qt deployment tool already specified." >&2
			exit
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

# Use default bundle name if not specified.
if [ "x${BUNDLE}" = "x" ]; then
	BUNDLE="${DFLT_BUNDLE}"
fi

# Use default mode if not specified.
if [ "x${MODE}" = "x" ]; then
	MODE="${DFLT_MODE}"
fi

# Use default deployment tool if not specified.
if [ "x${DEPLOYQT_EXE}" = "x" ]; then
	DEPLOYQT_EXE="${DFLT_DEPLOYQT_EXE}"
fi

DIR_LIBS="${SRC_ROOT}/libs"
DIR_BUNDLE="${SRC_ROOT}/${BUNDLE}"
FILE_APP="${DIR_BUNDLE}/${APP}.exe"

DLL_LOC="${DIR_LIBS}/shared_built/bin"

DYLIBS=""
DYLIBS="${DYLIBS} libasprintf.dll"
DYLIBS="${DYLIBS} libcurl.dll"
DYLIBS="${DYLIBS} libeay32.dll"
DYLIBS="${DYLIBS} libexpat.dll"
DYLIBS="${DYLIBS} libgettextlib.dll"
DYLIBS="${DYLIBS} libgettextpo.dll"
DYLIBS="${DYLIBS} libgettextsrc.dll"
DYLIBS="${DYLIBS} libcharset.dll"
DYLIBS="${DYLIBS} libiconv.dll"
DYLIBS="${DYLIBS} libintl.dll"
DYLIBS="${DYLIBS} libisds.dll"
DYLIBS="${DYLIBS} libltdl.dll"
DYLIBS="${DYLIBS} libxml2.dll"
DYLIBS="${DYLIBS} ssleay32.dll"

directory_exists "${DIR_LIBS}" || exit 1
directory_exists "${DLL_LOC}" || exit 1
directory_exists "${DIR_BUNDLE}" || exit 1
executable_exists "${FILE_APP}" || exit 1

dylibs_copy "${DIR_BUNDLE}" "${DLL_LOC}" "${DYLIBS}" || exit 1
qt_conf_copy "${DIR_BUNDLE}" "${SRC_ROOT}/res/qt.conf_windows" || exit 1
mkdir -p "${DIR_BUNDLE}/locale"
locale_copy "${DIR_BUNDLE}/locale" "${SRC_ROOT}/locale" || exit 1

# This library must be located (manually added) in the libs/ directory:
dylibs_copy "${DIR_BUNDLE}" "${DIR_LIBS}" "libgcc_s_sjlj-1.dll" || exit 1


cd "${DIR_BUNDLE}"
${DEPLOYQT_EXE} --${MODE} --libdir ./ --plugindir plugins/ "${APP}.exe" || exit 1

# Remove all Qt translations except Czech and English.
rm $(find translations/ | grep qt_ | grep -v 'qt_cs\|qt_en')

# Ensure that there are only the specified subdirectories.
have_only_directories "." "locale" "plugins" "translations" || exit 1

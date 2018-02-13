#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
SRC_ROOT=$(cd "${SCRIPT_LOCATION}"/..; pwd)

APP="datovka"
BUNDLE=""
DFLT_BUNDLE="${APP}.app"
NO_DEBUG="no"

USAGE="Usage:\n\t$0\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t-b NAME, --bundle NAME\n\t\tSupply bundle name. Default is '${DFLT_BUNDLE}'.\n"
USAGE="${USAGE}\t-h, --help\n\t\tPrints help message.\n"
USAGE="${USAGE}\t-n, --no-debug\n\t\tRemoves Qt debug libraries from the bundle.\n"

# otool -L
# otool -l

cd "${SRC_ROOT}"

# Return 0 if param is a directory.
directory_exists () {
	local DIR=$1
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
	local EXE=$1
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
	local VAR1="$1"
	local VAR2="$2"
	if [ "x${VAR1}" = "x" -o "x${VAR2}" = "x" ]; then
		echo "Received empty paramater." >&2
		return 1
	fi
	local LINENUM1=$(echo "${VAR1}" | wc -l)
	local LINENUM2=$(echo "${VAR2}" | wc -l)
	if [ "x${LINENUM1}" != "x${LINENUM2}" ]; then
		echo "Line number does not match." >&2
		return 1
	fi
	return 0
}

# Return list of dynamic libraries.
dylibs () {
	local APP="$1"
	if [ "x${APP}" = "x" ]; then
		echo ""
		return 1
	fi

	# First sed expression contains a '\t' character.
	otool -L "${APP}" | grep '^[ 	]' | grep "${DIR_LIBS}" | sed -e 's/^[ 	]*//g' -e 's/\(^.*dylib\)\(.*\)$/\1/g'
}

# Return non-empty location of dynamic libraries.
dylibs_location () {
	local DYLIBS="$1"
	if [ "x${DYLIBS}" = "x" ]; then
		echo ""
		echo "No shared libraries passed." >&2
		return 1
	fi

	# Only one line must remain.
	local LOCATION=$(echo "${DYLIBS}" | sed -e 's/[/][^/]*\.dylib//g' | sort -u)
	local LOCATION_NUM=$(echo "${LOCATION}" | wc -l)
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
	local DYLIBS="$1"
	echo "${DYLIBS}" | sed -e 's/^.*[/]//g'
}

# Add libssl to the list of libraries.
dylib_add_ssl () {
	local LOC="$1"
	local DYLIBS="$2"
	if [ "x${LOC}" = "x" -o "x${DYLIBS}" = "x" ]; then
		echo ""
		return 1
	fi

	local SSL_LIBS=$(ls "${LOC}/" | grep libssl)
	local FOUND=""
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
	local LOC="$1"
	local DYLIBS="$2"
	if [ "x${LOC}" = "x" -o "x${DYLIBS}" = "x" ]; then
		echo ""
		return 1
	fi

	DYLIBS=$(echo "${DYLIBS}" | sort -u)
	local OUT_DYLIBS=""

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

# Return just the name of the library.
dylibs_name () {
	local DYLIBS="$1"
	echo "${DYLIBS}" | sed -e 's/[.].*dylib//g'
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
		local FILES=$(ls "${SRC_LOC}/${D}"*.dylib)
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

# Return list of Qt frameworks.
qt_frameworks () {
	local APP="$1"
	if [ "x${APP}" = "x" ]; then
		echo ""
		return 1
	fi

	# First sed expression contains a '\t' character.
	otool -L "${APP}" | grep '^[ 	]' | grep 'Qt[^.]*.framework' | sed -e 's/^[ 	]*//g' -e 's/[ ](compat.*$//g'
}

# Return list of frameworks without leading @rpath.
qt_frameworks_strip () {
	local FRAMEWORKS="$1"
	echo "${FRAMEWORKS}" | sed -e 's/@rpath[/]//g'
}

# Return non-empty location of the Qt frameworks.
qt_frameworks_location () {
	local EXE="$1"
	local FRAMEWORKS="$2"
	if [ "x${EXE}" = "x" -o "x${FRAMEWORKS}" = "x" ]; then
		echo ""
		return 1
	fi
	if [ ! -x "${EXE}" ]; then
		echo ""
		return 1
	fi

	local PATHS=$(otool -l "${EXE}" | grep path | grep -v rpath | sed -e 's/\(^[^/]*path[^/]*\)//g' -e 's/[ ](.*$//g')

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
	local LOC="$1"
	local FRAMEWORKS="$2"
	if [ "x${LOC}" = "x" -o "x${FRAMEWORKS}" = "x" ]; then
		echo ""
		return 1
	fi

	FRAMEWORKS=$(echo "${FRAMEWORKS}" | sort -u)
	local OUT_FRAMEWORKS=""

	while [ "x${FRAMEWORKS}" != "x${OUT_FRAMEWORKS}" ]; do
		if [ "x${OUT_FRAMEWORKS}" != "x" ]; then
			FRAMEWORKS=$(echo "${OUT_FRAMEWORKS}" | sort -u)
		fi
		OUT_FRAMEWORKS=""

		for F in ${FRAMEWORKS}; do
			local FOUND_FWS=$(qt_frameworks "$LOC/${F}")
			if [ "x${FOUND_FWS}" = "x" ]; then
				# Something must be resent.
				echo ""
				return 1
			fi
			local STRIPPED_FWS=$(qt_frameworks_strip "${FOUND_FWS}")
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

# Return just the name of the QT framework.
qt_frameworks_name () {
	local FRAMEWORKS="$1"
	echo "${FRAMEWORKS}" | sed -e 's/[/].*$//g'
}

# Copy frameworks into bundle.
qt_frameworks_copy () {
	local TGT_LOC="$1"
	local SRC_LOC="$2"
	local FWS="$3"
	if [ "x${TGT_LOC}" = "x" -o "x${SRC_LOC}" = "x" -o "x${FWS}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	# Copy frameworks.
	for F in ${FWS}; do
		F=$(qt_frameworks_name "${F}")
		local SRC="${SRC_LOC}/${F}"
		if [ ! -d "${SRC}" ]; then
			continue
		fi
		cp -R "${SRC}" "${TGT_LOC}/"
		if [ "$?" != "0" ]; then
			rm -rf "${TGT_LOC}/"
			return 1
		fi
	done

	return 0
}

# Get location of Qt plugins.
qt_plugins_loc () {
	local FW_LOC="$1"
	if [ "x${FW_LOC}" = "x" ]; then
		echo ""
		echo "Missing parameter." >&2
		return 1
	fi

	if [ ! -d "${FW_LOC}" ]; then
		echo ""
		echo "Supplied parameter if not a directory." >&2
		return 1
	fi

	local PLUG_LOC=$(echo "$FW_LOC" | sed -e 's/[/][^/]*[/]*$//g')
	PLUG_LOC="${PLUG_LOC}/plugins"
	if [ ! -d "$PLUG_LOC" ]; then
		echo ""
		echo "Could not determine Qt plugins directory." >&2
		return 1
	fi

	echo "$PLUG_LOC"
	return 0
}

# Copy matching content.
qt_plugins_copy_matching () {
	local TGT_LOC="$1"
	local SRC_LOC="$2"
	local PLUG_SUBDIR="$3"
	if [ "x${TGT_LOC}" = "x" -o "x${SRC_LOC}" = "x" -o "x${PLUG_SUBDIR}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	if [ ! -d "${TGT_LOC}" -o ! -d "${SRC_LOC}" ]; then
		echo "Supplied parameters are not directories." >&2
		return 1
	fi

	shift
	shift
	shift

	SRC_LOC="${SRC_LOC}/${PLUG_SUBDIR}"
	if [ ! -d "${SRC_LOC}" ]; then
		echo "'${SRC_LOC}' is not a directory." >&2
		return 1
	fi

	if [ "$#" -le "0" ]; then
		echo "Missing at least one parameter." >&2
		return 1
	fi

	TGT_LOC="${TGT_LOC}/${PLUG_SUBDIR}"
	if [ ! -d "${TGT_LOC}" ]; then
		mkdir -p "${TGT_LOC}"
	fi

	while [ "$#" -gt "0" ]; do
		local NAME_STUB="$1"
		# Find files matching the stub and copy them into target.
		local MATCHES=$(ls "${SRC_LOC}/" | grep "${NAME_STUB}" | grep "dylib$")
		if [ "x${MATCHES}" = "x" ]; then
			echo "No mathes for '${NAME_STUB}' found in '${SRC_LOC}'." >&2
			return 1
		fi
		for M in ${MATCHES}; do
			local FNAME="${SRC_LOC}/${M}"
			if [ ! -f "${FNAME}" -o -L "${FNAME}" ]; then
				echo "'${FNAME}' is not a file." >&2
				return 1
			fi
			cp "${FNAME}" "${TGT_LOC}/"
			if [ "$?" != "0" ]; then
				return 1
			fi
		done

		shift
	done
}

# Copy Qt plugins into bundle.
qt_plugins_copy () {
	local TGT_LOC="$1"
	local SRC_LOC="$2"
	if [ "x${TGT_LOC}" = "x" -o "x${SRC_LOC}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	if [ ! -d "${TGT_LOC}" -o ! -d "${SRC_LOC}" ]; then
		echo "Supplied parameters are not directories." >&2
		return 1
	fi

	# See content of eg. Wireshark package.

	# Whole directories:
	# accessible ?
	cp -R "${SRC_LOC}/bearer" "${TGT_LOC}/" && \
	cp -R "${SRC_LOC}/iconengines" "${TGT_LOC}/" && \
	cp -R "${SRC_LOC}/printsupport" "${TGT_LOC}/"
	if [ "$?" != "0" ]; then
		rm -rf "${TGT_LOC}/"
		return 1
	fi

	# Only partial content:
	qt_plugins_copy_matching "${TGT_LOC}" "${SRC_LOC}" "imageformats" "gif" "icns" "ico" "jpeg" "svg" "tiff" && \
	qt_plugins_copy_matching "${TGT_LOC}" "${SRC_LOC}" "platforms" "cocoa" && \
	qt_plugins_copy_matching "${TGT_LOC}" "${SRC_LOC}" "sqldrivers" "sqlite"
	if [ "$?" != "0" ]; then
		rm -rf "${TGT_LOC}/"
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

# Update linker reference to shared library.
update_dylib () {
	local DYLIB="$1"
	local DL_NAMES="$2"
	local OLD_PATH="$3"
	local NEW_PATH="$4"
	if [ "x${DYLIB}" = "x" -o "x${DL_NAMES}" = "x" -o "x${OLD_PATH}" = "x" -o "x${NEW_PATH}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	for D in ${DL_NAMES}; do
		# Sed expression contains '\t'.
		local OLD=$(otool -L "${DYLIB}" | grep "${OLD_PATH}" | grep "${D}" | sed -e 's/^[ 	]*//g' -e 's/[ ](.*$//g')
		if [ "x${OLD}" = "x" ]; then
			continue
		fi
		local NEW=$(echo "${OLD}" | sed -e 's/^.*[/]//g') # Strip path.
		NEW="${NEW_PATH}/${NEW}"
		local MATCH_NAME=$(echo ${DYLIB} | grep "${D}")
		if [ "x${MATCH_NAME}" = "x" ]; then
			install_name_tool -change "${OLD}" "${NEW}" "${DYLIB}"
		else
			install_name_tool -id "${NEW}" "${DYLIB}"
		fi
		if [ "$?" != "0" ]; then
			return 1
		fi
	done

	return 0
}

# Update linker references to shared libraries.
recursive_update_dylibs () {
	local FW_DIR="$1"
	local DLS_STRIPPED="$2"
	local OLD_PATH="$3"
	local NEW_PATH="$4"
	if [ "x${FW_DIR}" = "x" -o "x${DLS_STRIPPED}" = "x" -o "x${OLD_PATH}" = "x" -o "x${NEW_PATH}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	local DL_NAMES=$(dylibs_name "${DLS_STRIPPED}")
	for D in ${DL_NAMES}; do
		local FILES=$(ls "${FW_DIR}/${D}"*.dylib)
		for F in ${FILES}; do
			if [ -f "${F}" -a ! -L "${F}" ]; then
				update_dylib "${F}" "${DL_NAMES}" "${OLD_PATH}" "${NEW_PATH}" || return 1
			fi
		done
	done

	return 0
}

# Update linker paths to shared libraries in application.
app_update_dylibs () {
	local APP="$1"
	local DLS_STRIPPED="$2"
	local OLD_PATH="$3"
	local NEW_PATH="$4"
	if [ "x${APP}" = "x" -o "x${DLS_STRIPPED}" = "x" -o "x${OLD_PATH}" = "x" -o "x${NEW_PATH}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	local DL_NAMES=$(dylibs_name "${DLS_STRIPPED}")
	for D in ${DL_NAMES}; do
		# Sed expression contains '\t'.
		local OLD=$(otool -L "${APP}" | grep "${OLD_PATH}" | grep "${D}" | sed -e 's/^[ 	]*//g' -e 's/[ ](.*$//g')
		if [ "x${OLD}" = "x" ]; then
			continue
		fi
		local NEW=$(echo "${OLD}" | sed -e 's/^.*[/]//g') # Strip path.
		NEW="${NEW_PATH}/${NEW}"
		install_name_tool -change "${OLD}" "${NEW}" "${APP}"
		if [ "$?" != "0" ]; then
			return 1
		fi
	done

	return 0
}

# Update linker paths to frameworks in application.
app_update_frameworks () {
	local APP="$1"
	local FWS_STRIPPED="$2"
	local NEW_PATH="$3"
	if [ "x${APP}" = "x" -o "x${FWS_STRIPPED}" = "x" -o "x${NEW_PATH}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	for F in ${FWS_STRIPPED}; do
		# Sed expression contains '\t'.
		local OLD=$(otool -L "${APP}" | grep "${F}" | sed -e 's/^[ 	]*//g' -e 's/[ ](.*$//g')
		local NEW="${NEW_PATH}/${F}"
		install_name_tool -change "${OLD}" "${NEW}" "${APP}"
		if [ "$?" != "0" ]; then
			return 1
		fi
	done

	return 0
}

# Update linker rpath.
app_update_rpath () {
	local APP="$1"
	local OLD_PATH="$2"
	local NEW_PATH="$3"
	if [ "x${APP}" = "x" -o "x${OLD_PATH}" = "x" -o "x${NEW_PATH}" = "x" ]; then
		echo "Invalid input." >&2
		return 1
	fi

	install_name_tool -rpath "${OLD_PATH}" "${NEW_PATH}" "${APP}"
	if [ "$?" != "0" ]; then
		return 1
	fi

	return 0
}

# Remove all debug libraries from given location.
remove_debug () {
	local LOC="$1"
	if [ "x${LOC}" = "x" ]; then
		echo "No paramater given." >&2
		return 1
	fi

	if [ ! -d "${LOC}" ]; then
		echo "'${LOC}' is not a directory." >&2
		return 1
	fi

	local FOUND=$(find "${LOC}/" | grep "_debug")
	if [ "x${FOUND}" != "x" ]; then
		rm ${FOUND} || return 1
	fi

	return 0
}

# Parse rest of command line
set -- `getopt -l bundle:,help,no-debug -u -o b:hn -- "$@"`
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
	-n|--no-debug)
		if [ "x${NO_DEBUG}" != "xyes" ]; then
			NO_DEBUG="yes"
		else
			echo "No debug is already set." >&2
			exit 1
		fi
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
DIR_FRAMEWORKS="${DIR_CONTENTS}/Frameworks"
DIR_MACOS="${DIR_CONTENTS}/MacOs"
DIR_PLUGINS="${DIR_CONTENTS}/PlugIns"
DIR_RESOURCES="${DIR_CONTENTS}/Resources"
FILE_APP="${DIR_MACOS}/${APP}"

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

directory_exists "${DIR_LIBS}" || exit 1
directory_exists "${DIR_BUNDLE}" || exit 1
directory_exists "${DIR_CONTENTS}" || exit 1
if directory_exists "${DIR_FRAMEWORKS}"; then
	rm -rf "${DIR_FRAMEWORKS}"
fi
mkdir -p "${DIR_FRAMEWORKS}"
directory_exists "${DIR_FRAMEWORKS}" || exit 1
directory_exists "${DIR_MACOS}" || exit 1
if directory_exists "${DIR_PLUGINS}"; then
	rm -rf "${DIR_PLUGINS}"
fi
mkdir -p "${DIR_PLUGINS}"
directory_exists "${DIR_PLUGINS}" || exit 1
directory_exists "${DIR_RESOURCES}" || exit 1
executable_exists "${FILE_APP}" || exit 1

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

QT_PLUGINS_LOC=$(qt_plugins_loc "${QT_FRAMEWORK_LOC}")

dylibs_copy "${DIR_FRAMEWORKS}" "${DYLIBS_LOC}" "${DYLIBS}" || exit 0
qt_frameworks_copy "${DIR_FRAMEWORKS}" "${QT_FRAMEWORK_LOC}" "${QT_FRAMEWORKS}" || exit 1
qt_plugins_copy "${DIR_PLUGINS}" "${QT_PLUGINS_LOC}"
qt_conf_copy "${DIR_RESOURCES}" "${SRC_ROOT}/res/qt.conf_macos"

#NEW_SEARCH_PATH="@executable_path/../Frameworks"
NEW_SEARCH_PATH="@rpath"

# Update references leading from the shared libraries.
recursive_update_dylibs "${DIR_FRAMEWORKS}" "${DYLIBS}" "${DYLIBS_LOC}" "${NEW_SEARCH_PATH}" || exit 1

# Update references leading from main application.
app_update_dylibs "${FILE_APP}" "${DYLIBS}" "${DYLIBS_LOC}" "${NEW_SEARCH_PATH}" || exit 1
app_update_frameworks "${FILE_APP}" "${QT_FRAMEWORKS}" "${NEW_SEARCH_PATH}" || exit 1

NEW_RPATH="@executable_path/../Frameworks"
app_update_rpath "${FILE_APP}" "${QT_FRAMEWORK_LOC}" "${NEW_RPATH}" || exit 1

if [ "x${NO_DEBUG}" = "xyes" ]; then
	remove_debug "${DIR_FRAMEWORKS}" || exit 1
	remove_debug "${DIR_PLUGINS}" || exit 1
fi

echo "Bundling OK"

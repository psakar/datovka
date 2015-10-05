#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
SRC_ROOT="${SCRIPT_LOCATION}/.."

DFLT_QT_VER="5.4.2"

if [ "x${QT_VER}" = "x" ]; then
	QT_VER=${DFLT_QT_VER}
fi

USAGE="Usage:\n\t$0 -s SDK_VERSION_NUMBER [options]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t-d\n\t\tAlso builds DMG file.\n"
USAGE="${USAGE}\t-D\n\t\tDon't compile, just build the package. This implies -d.\n"
USAGE="${USAGE}\t-h\n\t\tPrints help message.\n"
USAGE="${USAGE}\t-s SDK_VERSION_NUMBER\n\t\t Supply SDK version name (e.g. 10.7).\n"
USAGE="${USAGE}\n"
USAGE="${USAGE}Default Qt version to link with is '${DFLT_QT_VER}'. To change the version\n"
USAGE="${USAGE}specify the desired version via the QT_VER environment variable.\n"

cd "${SRC_ROOT}"

SDK_VER=""
COMPILE_SRC="yes"
BUILD_DMG="no"

# Parse rest of command line
set -- `getopt dDhs: "$@"`
if [ $# -lt 1 ]; then
	echo ${USAGE} >&2
	exit 1
fi
while [ $# -gt 0 ]; do
	case "$1" in
	-d)
		if [ "x${BUILD_DMG}" = "xno" ]; then
			BUILD_DMG=yes
		else
			echo "Option -d already set." >&2
			exit 1
		fi
		;;
	-D)
		if [ "x${COMPILE_SRC}" = "xyes" ]; then
			COMPILE_SRC=no
			BUILD_DMG=yes
		else
			echo "Option -n already set." >&2
			exit 1
		fi
		;;
	-h)
		echo ${USAGE}
		exit 0
		;;
	-s)
		if [ "x${SDK_VER}" = "x" ]; then
			SDK_VER=$(echo $2 | grep '^[0-9][0-9]*\.[0-9][0-9]*$')
			if [ "x${SDK_VER}" = "x" ]; then
				echo "Wrong -s option parameter '$2'." >&2
				exit 1
			fi
		else
			echo "Option -s already set." >&2
			exit 1
		fi
		shift
		;;
	--)
		shift
		break
		;;
	-*)
		echo ${USAGE} >&2
		exit 1
		;;
	*)
		echo ${USAGE} >&2
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

# Test whether SDK version is really set.
if [ "x${SDK_VER}" = "x" ]; then
	echo "Option -s was not set." >&2
	exit 1
fi

PKG_VER=$(cat datovka.pro | grep '^VERSION\ =\ ' | sed -e 's/VERSION\ =\ //g')
APP="datovka.app"


if [ "x${COMPILE_SRC}" = "xyes" ]; then
	QMAKE_PATH="/usr/local/Qt-${QT_VER}-macx-clang-32-macosx${SDK_VER}-static/bin/qmake"

	# Test qmake presence.
	if [ ! -x ${QMAKE_PATH} ]; then
		echo "Cannot find executable '${QMAKE_PATH}'." >&2
		exit 1
	fi

	LIBS_DIR="built_osx${SDK_VER}"
	LIBS_PATH="libs_static/${LIBS_DIR}"
	LIBS_LINK="libs_static/built"
	# Test the presence of build libraries.
	if [ ! -d "${SRC_ROOT}/$LIBS_PATH" ]; then
		echo "Directory '${SRC_ROOT}/$LIBS_PATH' does not exist." >&2
		exit 1
	else
		# Prepare link.
		rm "${LIBS_LINK}"
		ln -s "${LIBS_DIR}" "${LIBS_LINK}"
	fi

	${QMAKE_PATH} SDK_VER="${SDK_VER}" STATIC=1 datovka.pro
	rm -rf "${APP}"
	make clean
	make
fi

if [ "x${BUILD_DMG}" != "xno" ]; then
	if [ ! -d "${APP}" ]; then
		echo "Cannot find '${APP}'. Run the compilation." >&2
		exit 1
	fi

	# You must be logged in into a desktop session in order to create
	# dmg files.
	TGT="datovka-${PKG_VER}-osx${SDK_VER}.dmg"
	rm -rf "${TGT}"
	${SCRIPT_LOCATION}/build_dmg.sh
	mv datovka-installer.dmg "${TGT}"
fi

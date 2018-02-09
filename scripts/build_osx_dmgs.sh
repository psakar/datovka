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
USAGE="${USAGE}\t-s SDK_VERSION_NUMBER\n\t\tSupply SDK version name (e.g. 10.7).\n"
USAGE="${USAGE}\t--shared\n\t\tCompile with shared libraries.\n"
USAGE="${USAGE}\t--static (default)\n\t\tCompile with static libraries.\n"
USAGE="${USAGE}\t--i386 (default)\n\t\tCompile for i386 architecture.\n"
UASGE="${USAGE}\t--x86_64\n\t\tCompile for x86_64 architecture.\n"
USAGE="${USAGE}\n"
USAGE="${USAGE}Default Qt version to link with is '${DFLT_QT_VER}'. To change the version\n"
USAGE="${USAGE}specify the desired version via the QT_VER environment variable.\n"

cd "${SRC_ROOT}"

SDK_VER=""
COMPILE_SRC="yes"
BUILD_DMG="no"

BUILD_TYPE=""
BUILD_SHARED="shared"
BUILD_STATIC="static"

ARCH_NAME=""
ARCH_I386="i386"
ARCH_X86_64="x86_64"

# Parse rest of command line
set -- `getopt -l shared -l static -l i386 -l x86_64 -u -o dDhs: -- "$@"`
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
	--shared)
		if [ "x${BUILD_TYPE}" = "x" ]; then
			BUILD_TYPE="${BUILD_SHARED}"
		else
			echo "Build type already specified or in conflict." >&2
			exit 1
		fi
		;;
	--static)
		if [ "x${BUILD_TYPE}" = "x" ]; then
			BUILD_TYPE="${BUILD_STATIC}"
		else
			echo "Build type already specified or in conflict." >&2
			exit 1
		fi
		;;
	--i386)
		if [ "x${ARCH_NAME}" = "x" ]; then
			ARCH_NAME="${ARCH_I386}"
		else
			echo "Architecture already specified of in conflict." >&2
			exit 1
		fi
		;;
	--x86_64)
		if [ "x${ARCH_NAME}" = "x" ]; then
			ARCH_NAME="${ARCH_X86_64}"
		else
			echo "Architecture already specified of in conflict." >&2
			exit 1
		fi
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

# Use static libraries by default.
if [ "x${BUILD_TYPE}" = "x" ]; then
	BUILD_TYPE="${BUILD_STATIC}"
fi

# Use i386 by default.
if [ "x${ARCH_NAME}" = "x" ]; then
	ARCH_NAME="${ARCH_I386}"
fi

CLANG_BITS=""
if [ "x${ARCH_NAME}" = "x${ARCH_I386}" ]; then
	CLANG_BITS="32"
elif [ "x${ARCH_NAME}" = "x${ARCH_X86_64}" ]; then
	CLANG_BITS="64"
else
	echo "Unknown architecture." >&2
	exit 1
fi

PKG_VER=$(cat pri/version.pri | grep '^VERSION\ =\ ' | sed -e 's/VERSION\ =\ //g')
APP="datovka.app"


if [ "x${COMPILE_SRC}" = "xyes" ]; then
	QT_PATH="/usr/local/Qt-${QT_VER}-macx-clang-${CLANG_BITS}-macosx${SDK_VER}-${BUILD_TYPE}"
	QMAKE="${QT_PATH}/bin/qmake"
	LRELEASE="${QT_PATH}/bin/lrelease"

	# Test command presence.
	for CMD in "${LRELEASE}" "${QMAKE}"; do
		if [ ! -x "${CMD}" ]; then
			echo "Cannot find executable '${CMD}'." >&2
			exit 1
		fi
	done

	LIBS_DIR="built_macos_sdk${SDK_VER}_${ARCH_NAME}_${BUILD_TYPE}"
	LIBS_PATH="libs/${LIBS_DIR}"
	LIBS_LINK="libs/${BUILD_TYPE}_built_${ARCH_NAME}"
	# Test the presence of build libraries.
	if [ ! -d "${SRC_ROOT}/$LIBS_PATH" ]; then
		echo "Directory '${SRC_ROOT}/$LIBS_PATH' does not exist." >&2
		exit 1
	else
		# Prepare link.
		rm "${LIBS_LINK}"
		ln -s "${LIBS_DIR}" "${LIBS_LINK}"
	fi

	STATIC="0"
	if [ "x${BUILD_TYPE}" = "x${BUILD_STATIC}" ]; then
		STATIC="1"
	fi

	${LRELEASE} datovka.pro
	${QMAKE} SDK_VER="${SDK_VER}" WITH_BUILT_LIBS=1 STATIC="${STATIC}" datovka.pro
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

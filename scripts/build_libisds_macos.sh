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
cd "${SRC_ROOT}"

. "${SRC_ROOT}"/scripts/helper_dependency_sources.sh

adjust_sources "macos"

OSX_MIN_VER=10.7
MAKEOPTS="-j 2"

SDK_VER="$1"
if [ "x${SDK_VER}" = "x" ]; then
	echo "No sdk version supplied. Run '$0 SDK_VER' again." >&2
	exit 1
fi

SDKROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs"
ISYSROOT="${SDKROOT}/MacOSX${SDK_VER}.sdk"

if [ ! -d "${ISYSROOT}" ]; then
	echo "Directory '${ISYSROOT}' does not exist." >&2
	echo "Cannot find SDK root for version '${SDK_VER}'." >&2
	exit 1
fi

LIB_ROOT="${SRC_ROOT}"/libs
mkdir -p "${LIB_ROOT}"
if [ ! -d "${LIB_ROOT}" ]; then
	echo "Cannot find directory '${LIB_ROOT}'" >&2
	exit 1
fi
cd "${LIB_ROOT}"

SRCDIR="${LIB_ROOT}/srcs"
PATCHDIR="${SRC_ROOT}/scripts/patches"
WORKDIR_PREFIX="${LIB_ROOT}/work_macos_sdk${SDK_VER}"
BUILTDIR_PREFIX="${LIB_ROOT}/built_macos_sdk${SDK_VER}"

if [ ! -d "${SRCDIR}" ]; then
	mkdir "${SRCDIR}"
fi

download_all_sources "${SRCDIR}" || exit 1
sleep 3

# Specifies which targets to build.
TARGETS=""
TARGETS="${TARGETS} i386_static"
TARGETS="${TARGETS} i386_shared"
TARGETS="${TARGETS} x86_64_static"
TARGETS="${TARGETS} x86_64_shared"

# Return 0 if targets are OK.
check_params () {
	local ARCH="$1"
	local TYPE="$2"
	if [ "x${ARCH}" != "xi386" -a "x${ARCH}" != "xx86_64" ]; then
		echo "Unknown architecture '${ARCH}'." >&2
		return 1
	fi
	if [ "x${TYPE}" != "xstatic" -a "x${TYPE}" != "xshared" ]; then
		echo "Unknown type '${TYPE}'." >&2
		return 1
	fi
	return 0
}

# Return 0 if target is scheduled for build.
target_scheduled () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || return 1
	local RES=""
	RES=$(echo "${TARGETS}" | grep "${ARCH}" | grep "${TYPE}")
	if [ "x${RES}" != "x" ]; then
		return 0
	else
		return 1
	fi
}

workdir_name () {
	local ARCH="$1"
	local TYPE="$2"
	echo "${WORKDIR_PREFIX}_${ARCH}_${TYPE}"
}

builtdir_name () {
	local ARCH="$1"
	local TYPE="$2"
	echo "${BUILTDIR_PREFIX}_${ARCH}_${TYPE}"
}

ensure_dir_presence () {
	mkdir -p "$1"
}

# Create missing directories.
target_scheduled i386 static && ensure_dir_presence $(workdir_name i386 static)
target_scheduled i386 static && ensure_dir_presence $(builtdir_name i386 static)
target_scheduled i386 shared && ensure_dir_presence $(workdir_name i386 shared)
target_scheduled i386 shared && ensure_dir_presence $(builtdir_name i386 shared)
target_scheduled x86_64 static && ensure_dir_presence $(workdir_name x86_64 static)
target_scheduled x86_64 static && ensure_dir_presence $(builtdir_name x86_64 static)
target_scheduled x86_64 shared && ensure_dir_presence $(workdir_name x86_64 shared)
target_scheduled x86_64 shared && ensure_dir_presence $(builtdir_name x86_64 shared)


ZLIB_ARCHIVE="${_ZLIB_ARCHIVE}"
EXPAT_ARCHIVE="${_EXPAT_ARCHIVE}"
LIBTOOL_ARCHIVE="${_LIBTOOL_ARCHIVE}"

LIBICONV_ARCHIVE="${_LIBICONV_ARCHIVE}"
LIBXML2_ARCHIVE="${_LIBXML2_ARCHIVE}"
GETTEXT_ARCHIVE="${_GETTEXT_ARCHIVE}" # Enable NLS.

# Libcurl is already available in the system.
USE_SYSTEM_CURL="yes"
LIBCURL_ARCHIVE="${_LIBCURL_ARCHIVE}"
OPENSSL_ARCHIVE="${_OPENSSL_ARCHIVE}"

LIBISDS_ARCHIVE="${_LIBISDS_ARCHIVE}"
LIBISDS_ARCHIVE_PATCHES="${_LIBISDS_ARCHIVE_PATCHES}"
#LIBISDS_GIT="https://gitlab.labs.nic.cz/kslany/libisds.git"
#LIBISDS_BRANCH="feature-openssl" # Use master.

# lipo -info
# otool -L

build_zlib () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${ZLIB_ARCHIVE}" "${WORKDIR}" zlib
	cd "${WORKDIR}"/zlib*

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --static"
	fi

	CFLAGS="-mmacosx-version-min=${OSX_MIN_VER} -arch ${ARCH}" LDFLAGS="-mmacosx-version-min=${OSX_MIN_VER}" ./configure ${CONFOPTS} --archs="-arch ${ARCH}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	if [ "x${TYPE}" = "xshared" ]; then
		rm -rf "${BUILTDIR}"/lib/libz.a
	fi

	return 0
}

if [ ! -z "${ZLIB_ARCHIVE}" ]; then
	echo "Building zlib."
	if target_scheduled i386 static; then build_zlib i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_zlib i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_zlib x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_zlib x86_64 shared || exit 1; fi
fi


build_expat () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${EXPAT_ARCHIVE}" "${WORKDIR}" expat
	cd "${WORKDIR}"/expat*

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi

	./configure ${CONFOPTS} \
	    CFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${EXPAT_ARCHIVE}" ]; then
	echo "Building expat."
	if target_scheduled i386 static; then build_expat i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_expat i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_expat x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_expat x86_64 shared || exit 1; fi
fi


build_libtool () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBTOOL_ARCHIVE}" "${WORKDIR}" libtool
	cd "${WORKDIR}"/libtool*

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi

	./configure ${CONFOPTS} \
	    CFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${LIBTOOL_ARCHIVE}" ]; then
	echo "Building libtool."
	if target_scheduled i386 static; then build_libtool i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_libtool i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_libtool x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_libtool x86_64 shared || exit 1; fi
fi


build_libiconv () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBICONV_ARCHIVE}" "${WORKDIR}" libiconv
	cd "${WORKDIR}"/libiconv*

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi

	./configure ${CONFOPTS} \
	    CFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${LIBICONV_ARCHIVE}" ]; then
	echo "Building libiconv."
	if target_scheduled i386 static; then build_libiconv i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_libiconv i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_libiconv x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_libiconv x86_64 shared || exit 1; fi
fi


build_libxml2 () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBXML2_ARCHIVE}" "${WORKDIR}" libxml2
	cd "${WORKDIR}"/libxml2*

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --without-lzma"
	CONFOPTS="${CONFOPTS} --without-zlib"
	CONFOPTS="${CONFOPTS} --without-python"
	CONFOPTS="${CONFOPTS} --with-iconv=${BUILTDIR}"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${LIBXML2_ARCHIVE}" ]; then
	echo "Bulding libxml2."
	if target_scheduled i386 static; then build_libxml2 i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_libxml2 i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_libxml2 x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_libxml2 x86_64 shared || exit 1; fi
fi


build_gettext () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${GETTEXT_ARCHIVE}" "${WORKDIR}" gettext
	cd "${WORKDIR}"/gettext*

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --with-libxml2-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --enable-relocatable"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}" \
	    CPPFLAGS="-I${BUILTDIR}/include" \
	    LDFLAGS="-L${BUILTDIR}/lib -arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
	echo "Building gettext."
	if target_scheduled i386 static; then build_gettext i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_gettext i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_gettext x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_gettext x86_64 shared || exit 1; fi
fi


build_openssl () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${OPENSSL_ARCHIVE}" "${WORKDIR}" openssl
	cd "${WORKDIR}"/openssl*

	local CONFOPTS=""
	#CONFOPTS="${CONFOPTS} no-asm"
	if [ "x${ARCH}" = "xi386" ]; then
		CONFOPTS="${CONFOPTS} darwin-i386-cc"
	fi
	if [ "x${ARCH}" = "xx86_64" ]; then
		CONFOPTS="${CONFOPTS} darwin64-x86_64-cc"
	fi
	CONFOPTS="${CONFOPTS} enable-static-engine"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} no-shared"
	fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} shared"
	fi
	CONFOPTS="${CONFOPTS} no-krb5"

	./Configure ${CONFOPTS} --prefix="${BUILTDIR}"
	# Patch Makefile
	sed -ie "s/^CFLAG= -/CFLAG=  -mmacosx-version-min=${OSX_MIN_VER} -/" Makefile
	make depend || exit 1
	make ${MAKEOPTS} && make install_sw || exit 1

	if [ "x${TYPE}" = "xshared" ]; then
		rm -rf "${BUILTDIR}"/lib/libcrypto.a
		rm -rf "${BUILTDIR}"/lib/libssl.a
	fi

	unset CONFOPTS

	return 0
}

if [ ! -z "${OPENSSL_ARCHIVE}" ]; then
	echo "Building openssl."
	if target_scheduled i386 static; then build_openssl i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_openssl i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_openssl x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_openssl x86_64 shared || exit 1; fi
fi


build_libcurl () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBCURL_ARCHIVE}" "${WORKDIR}" curl
	cd "${WORKDIR}"/curl*

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	# Leave shared version as this is required to configure libisds.
	#if [ "x${TYPE}" = "xstatic" ]; then
	#	CONFOPTS="${CONFOPTS} --disable-shared"
	#fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --enable-http"
	CONFOPTS="${CONFOPTS} --enable-ipv6"
	CONFOPTS="${CONFOPTS} --enable-proxy"
	CONFOPTS="${CONFOPTS} --disable-file"
	CONFOPTS="${CONFOPTS} --disable-ftp"
	CONFOPTS="${CONFOPTS} --disable-gopher"
	CONFOPTS="${CONFOPTS} --disable-imap"
	CONFOPTS="${CONFOPTS} --disable-ldap"
	CONFOPTS="${CONFOPTS} --disable-ldaps"
	CONFOPTS="${CONFOPTS} --disable-manual"
	CONFOPTS="${CONFOPTS} --disable-pop3"
	CONFOPTS="${CONFOPTS} --disable-rtsp"
	CONFOPTS="${CONFOPTS} --disable-smb"
	CONFOPTS="${CONFOPTS} --disable-smtp"
	#CONFOPTS="${CONFOPTS} --disable-sspi"
	CONFOPTS="${CONFOPTS} --disable-telnet"
	CONFOPTS="${CONFOPTS} --disable-tftp"
	CONFOPTS="${CONFOPTS} --without-axtls"
	CONFOPTS="${CONFOPTS} --without-zsh-functions-dir"
	CONFOPTS="${CONFOPTS} --with-darwinssl"

	# Recent libcurl may require OSX_MIN_VER 10.8 or later.

	./configure ${CONFOPTS} \
	    CFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CXXFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    LDFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ "x${USE_SYSTEM_CURL}" != "xyes" ] && [ ! -z "${LIBCURL_ARCHIVE}" ]; then
	echo "Building libcurl."
	if target_scheduled i386 static; then build_libcurl i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_libcurl i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_libcurl x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_libcurl x86_64 shared || exit 1; fi
fi


build_libisds () {
	local ARCH="$1"
	local TYPE="$2"
	check_params "${ARCH}" "${TYPE}" || exit 1
	local WORKDIR=$(workdir_name "${ARCH}" "${TYPE}")
	local BUILTDIR=$(builtdir_name "${ARCH}" "${TYPE}")

	if [ ! -z "${LIBISDS_ARCHIVE}" ]; then
		erase_and_decompress "${SRCDIR}" "${LIBISDS_ARCHIVE}" "${WORKDIR}" libisds
		cd "${WORKDIR}"/libisds*

		if [ "x${LIBISDS_ARCHIVE_PATCHES}" != "x" ]; then
			# Apply patches.
			for f in ${LIBISDS_ARCHIVE_PATCHES}; do
				local PATCHFILE="${PATCHDIR}/${f}"
				if [ ! -f "${PATCHFILE}" ]; then
					echo "Missing ${PATCHFILE}" >&2
					exit 1
				fi
				cp "${PATCHFILE}" ./
				echo "Applying ${f}"
				patch -p1 < ${f}
				unset PATCHFILE
			done
		fi
	elif [ ! -z "${LIBISDS_GIT}" ]; then
		# libisds with OpenSSL back-end
		rm -rf "${WORKDIR}"/libisds*
		cd "${WORKDIR}"
		git clone "${LIBISDS_GIT}" libisds-git
		cd "${WORKDIR}"/libisds*
		if [ ! -z "${LIBISDS_BRANCH}" ]; then
			git checkout "${LIBISDS_BRANCH}"
		fi
	else
		echo "Cannot prepare libisds sources." >&2
		exit 1
	fi

	local CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xshared" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --enable-debug"
	CONFOPTS="${CONFOPTS} --enable-openssl-backend"
	CONFOPTS="${CONFOPTS} --disable-fatalwarnings"
	CONFOPTS="${CONFOPTS} --with-xml-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libcurl=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR}"

	local NLS="--disable-nls"
	if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
		NLS=""
	fi
	CONFOPTS="${CONFOPTS} ${NLS}"

	if [ -z "${LIBISDS_ARCHIVE}" -a ! -z "${LIBISDS_GIT}" ]; then
		autoheader && glibtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo "configure build ok"
	fi
	./configure ${CONFOPTS} \
	    CFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CXXFLAGS="-arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" \
	    LDFLAGS="-L${BUILTDIR}/lib -arch ${ARCH} -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	if [ "x${TYPE}" = "xstatic" -a -f "${BUILTDIR}/lib/libcurl.dylib" ]; then
		mv "${BUILTDIR}/lib/libcurl.dylib" "${BUILTDIR}/lib/libcurl.dylib_x"
	fi

	return 0
}

if [ ! -z "${LIBISDS_ARCHIVE}" -a ! -z "${LIBISDS_GIT}" ]; then
	echo "Select libisds archive or git repository." >&2
	exit 1
elif [ ! -z "${LIBISDS_ARCHIVE}" -o ! -z "${LIBISDS_GIT}" ]; then
	echo "Building libisds."
	if target_scheduled i386 static; then build_libisds i386 static || exit 1; fi
	if target_scheduled i386 shared; then build_libisds i386 shared || exit 1; fi
	if target_scheduled x86_64 static; then build_libisds x86_64 static || exit 1; fi
	if target_scheduled x86_64 shared; then build_libisds x86_64 shared || exit 1; fi
fi

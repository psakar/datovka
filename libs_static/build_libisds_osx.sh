#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
SRC_ROOT=$(cd "${SCRIPT_LOCATION}"/..; pwd)

cd "${SRC_ROOT}"

. "${SRC_ROOT}"/scripts/dependency_sources.sh

adjust_sources "osx"

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
if [ ! -d "${LIB_ROOT}" ]; then
	echo "Cannot find directory '${LIB_ROOT}'" >&2
	exit 1
fi
cd "${LIB_ROOT}"

SRCDIR="${LIB_ROOT}/srcs"
PATCHDIR="${SRC_ROOT}/scripts/patches"
WORKDIR_i386_STATIC="${LIB_ROOT}/work_macos_sdk${SDK_VER}_i386_static"
BUILTDIR_i386_STATIC="${LIB_ROOT}/built_macos_sdk${SDK_VER}_i386_static"

if [ ! -d "${SRCDIR}" ]; then
	mkdir "${SRCDIR}"
fi

if [ ! -d "${WORKDIR_i386_STATIC}" ]; then
	mkdir "${WORKDIR_i386_STATIC}"
fi

if [ ! -d "${BUILTDIR_i386_STATIC}" ]; then
	mkdir "${BUILTDIR_i386_STATIC}"
fi

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


if [ ! -z "${ZLIB_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${ZLIB_ARCHIVE}" "${WORKDIR_i386_STATIC}" zlib
	cd "${WORKDIR_i386_STATIC}"/zlib*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --static"

	CFLAGS="-mmacosx-version-min=${OSX_MIN_VER}" LDFLAGS="-mmacosx-version-min=${OSX_MIN_VER}" ./configure ${CONFOPTS} --archs="-arch i386"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${EXPAT_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${EXPAT_ARCHIVE}" "${WORKDIR_i386_STATIC}" expat
	cd "${WORKDIR_i386_STATIC}"/expat*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --disable-shared"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${LIBTOOL_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${LIBTOOL_ARCHIVE}" "${WORKDIR_i386_STATIC}" libtool
	cd "${WORKDIR_i386_STATIC}"/libtool*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --disable-shared"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${LIBICONV_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${LIBICONV_ARCHIVE}" "${WORKDIR_i386_STATIC}" libiconv
	cd "${WORKDIR_i386_STATIC}"/libiconv*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --disable-shared"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${LIBXML2_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${LIBXML2_ARCHIVE}" "${WORKDIR_i386_STATIC}" libxml2
	cd "${WORKDIR_i386_STATIC}"/libxml2*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --without-lzma"
	CONFOPTS="${CONFOPTS} --without-python"
	CONFOPTS="${CONFOPTS} --with-iconv=${BUILTDIR_i386_STATIC}"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${GETTEXT_ARCHIVE}" "${WORKDIR_i386_STATIC}" gettext
	cd "${WORKDIR_i386_STATIC}"/gettext*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --with-libxml2-prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR_i386_STATIC}"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CPPFLAGS="-I${BUILTDIR_i386_STATIC}/include" \
	    LDFLAGS="-L${BUILTDIR_i386_STATIC}/lib -arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ "x${USE_SYSTEM_CURL}" != "xyes" ] && [ ! -z "${LIBCURL_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${LIBCURL_ARCHIVE}" "${WORKDIR_i386_STATIC}" curl
	cd "${WORKDIR_i386_STATIC}"/curl*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	#CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --enable-ipv6"
	CONFOPTS="${CONFOPTS} --with-darwinssl"
	CONFOPTS="${CONFOPTS} --without-axtls"
	CONFOPTS="${CONFOPTS} --disable-ldap"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${OPENSSL_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${OPENSSL_ARCHIVE}" "${WORKDIR_i386_STATIC}" openssl
	cd "${WORKDIR_i386_STATIC}"/openssl*

	# no-asm
	# darwin-i386-cc
	./Configure darwin-i386-cc enable-static-engine no-shared no-krb5 \
	    --prefix="${BUILTDIR_i386_STATIC}"
	# Patch Makefile
	sed -ie "s/^CFLAG= -/CFLAG=  -mmacosx-version-min=${OSX_MIN_VER} -/" Makefile
	make ${MAKEOPTS} && make install_sw || exit 1
fi


if [ ! -z "${LIBISDS_ARCHIVE}" -a ! -z "${LIBISDS_GIT}" ]; then
	echo "Select libisds archive or git repository." >&2
	exit 1
elif [ ! -z "${LIBISDS_ARCHIVE}" ]; then
	erase_and_decompress "${SRCDIR}" "${LIBISDS_ARCHIVE}" "${WORKDIR_i386_STATIC}" libisds
	cd "${WORKDIR_i386_STATIC}"/libisds*

	if [ "x${LIBISDS_ARCHIVE_PATCHES}" != "x" ]; then
		# Apply patches.
		for f in ${LIBISDS_ARCHIVE_PATCHES}; do
			PATCHFILE="${PATCHDIR}/${f}"
			if [ ! -f "${PATCHFILE}" ]; then
				echo "Missing ${PATCHFILE}" >&2
				exit 1
			fi
			cp "${PATCHFILE}" ./
			patch -p1 < ${f}
		done
	fi

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --enable-debug"
	CONFOPTS="${CONFOPTS} --enable-openssl-backend"
	CONFOPTS="${CONFOPTS} --disable-fatalwarnings"
	CONFOPTS="${CONFOPTS} --with-xml-prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --with-libcurl=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR_i386_STATIC}"

	NLS="--disable-nls"
	if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
		NLS=""
	fi
	CONFOPTS="${CONFOPTS} ${NLS}"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CPPFLAGS="-I${BUILTDIR_i386_STATIC}/include -I${BUILTDIR_i386_STATIC}/include/libxml2" \
	    LDFLAGS="-L${BUILTDIR_i386_STATIC}/lib -arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	if [ -f "${BUILTDIR_i386_STATIC}/lib/libcurl.dylib" ]; then
		mv "${BUILTDIR_i386_STATIC}/lib/libcurl.dylib" "${BUILTDIR_i386_STATIC}/lib/libcurl.dylib_x"
	fi
elif [ ! -z "${LIBISDS_GIT}" ]; then
	# libisds with OpenSSL back-end
	rm -rf "${WORKDIR_i386_STATIC}"/libisds*
	cd "${WORKDIR_i386_STATIC}"
	git clone "${LIBISDS_GIT}" libisds-git
	cd "${WORKDIR_i386_STATIC}"/libisds*
	if [ ! -z "${LIBISDS_BRANCH}" ]; then
		git checkout "${LIBISDS_BRANCH}"
	fi

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --enable-debug"
	CONFOPTS="${CONFOPTS} --enable-openssl-backend"
	CONFOPTS="${CONFOPTS} --disable-fatalwarnings"
	CONFOPTS="${CONFOPTS} --with-xml-prefix=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --with-libcurl=${BUILTDIR_i386_STATIC}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR_i386_STATIC}"

	NLS="--disable-nls"
	if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
		NLS=""
	fi
	CONFOPTS="${CONFOPTS} ${NLS}"

	autoheader && glibtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo configure build ok
	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CPPFLAGS="-I${BUILTDIR_i386_STATIC}/include -I${BUILTDIR_i386_STATIC}/include/libxml2" \
	    LDFLAGS="-L${BUILTDIR_i386_STATIC}/lib -arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	if [ -f "${BUILTDIR_i386_STATIC}/lib/libcurl.dylib" ]; then
		mv "${BUILTDIR_i386_STATIC}/lib/libcurl.dylib" "${BUILTDIR_i386_STATIC}/lib/libcurl.dylib_x"
	fi
fi

#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)

. "${SCRIPT_LOCATION}"/../scripts/dependency_sources.sh

OSX_MIN_VER=10.6
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


SRCDIR="${SCRIPT_LOCATION}/srcs"
WORKDIR="${SCRIPT_LOCATION}/work"
BUILTDIR="${SCRIPT_LOCATION}/built_osx${SDK_VER}"

if [ ! -d "${SRCDIR}" ]; then
	mkdir "${SRCDIR}"
fi

if [ ! -d "${WORKDIR}" ]; then
	mkdir "${WORKDIR}"
fi

if [ ! -d "${BUILTDIR}" ]; then
	mkdir "${BUILTDIR}"
fi

ZLIB_ARCHIVE="${_ZLIB_ARCHIVE}"
EXPAT_ARCHIVE="${_EXPAT_ARCHIVE}"
LIBTOOL_ARCHIVE="${_LIBTOOL_ARCHIVE}"

LIBICONV_ARCHIVE="${_LIBICONV_ARCHIVE}"
LIBXML2_ARCHIVE="${_LIBXML2_ARCHIVE}"
GETTEXT_ARCHIVE="${_GETTEXT_ARCHIVE}"

# Libcurl is already available in the system.
USE_SYSTEM_CURL="yes"
LIBCURL_ARCHIVE="${_LIBCURL_ARCHIVE}"
OPENSSL_ARCHIVE="${_OPENSSL_ARCHIVE}"

LIBISDS_ARCHIVE="${_LIBISDS_ARCHIVE}"
#LIBISDS_GIT="https://gitlab.labs.nic.cz/kslany/libisds.git"
#LIBISDS_BRANCH="feature-openssl" # Use master.


if [ ! -z "${ZLIB_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${ZLIB_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# zlib
	rm -rf "${WORKDIR}"/zlib*
	cd "${WORKDIR}"
	tar -xJf "${ARCHIVE}"
	cd "${WORKDIR}"/zlib*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --static"

	CFLAGS="-mmacosx-version-min=${OSX_MIN_VER}" LDFLAGS="-mmacosx-version-min=${OSX_MIN_VER}" ./configure ${CONFOPTS} --archs="-arch i386"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${EXPAT_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${EXPAT_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# expat
	rm -rf "${WORKDIR}"/expat*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/expat*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --disable-shared"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${LIBTOOL_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBTOOL_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# libtool
	rm -rf "${WORKDIR}"/libtool*
	cd "${WORKDIR}"
	tar -xJf "${ARCHIVE}"
	cd "${WORKDIR}"/libtool*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --disable-shared"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${LIBICONV_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBICONV_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# libiconv
	rm -rf "${WORKDIR}"/libiconv*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/libiconv*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --disable-shared"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${LIBXML2_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBXML2_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# libxml2
	rm -rf "${WORKDIR}"/libxml2*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/libxml2*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --without-python"
	CONFOPTS="${CONFOPTS} --with-iconv=${BUILTDIR}"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    LDFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${GETTEXT_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# gettext
	rm -rf "${WORKDIR}"/gettext*
	cd "${WORKDIR}"
	tar -xJf "${ARCHIVE}"
	cd "${WORKDIR}"/gettext*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --with-libxml2-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR}"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER}" \
	    CPPFLAGS="-I${BUILTDIR}/include" \
	    LDFLAGS="-L${BUILTDIR}/lib -arch i386 -mmacosx-version-min=${OSX_MIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
fi


if [ "x${USE_SYSTEM_CURL}" != "xyes" ] && [ ! -z "${LIBCURL_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBCURL_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# libcurl
	rm -rf "${WORKDIR}"/curl*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/curl*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
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
	ARCHIVE="${SRCDIR}/${OPENSSL_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# OpenSSL
	rm -rf "${WORKDIR}"/openssl*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/openssl*

	# no-asm
	# darwin-i386-cc
	./Configure darwin-i386-cc enable-static-engine no-shared no-krb5 --prefix="${BUILTDIR}"
	# Patch Makefile
	sed -ie "s/^CFLAG= -/CFLAG=  -mmacosx-version-min=${OSX_MIN_VER} -/" Makefile
	make ${MAKEOPTS} && make install_sw || exit 1
fi


if [ ! -z "${LIBISDS_ARCHIVE}" -a ! -z "${LIBISDS_GIT}" ]; then
	echo "Select libisds archive or git repository." >&2
	exit 1
elif [ ! -z "${LIBISDS_ARCHIVE}" ]; then
	# libisds with OpenSSL back-end
	ARCHIVE="${SRCDIR}/${LIBISDS_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}" >&2
		exit 1
	fi
	# libisds
	rm -rf "${WORKDIR}"/libisds*
	cd "${WORKDIR}"
	tar -xJf "${ARCHIVE}"
	cd "${WORKDIR}"/libisds*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --enable-debug"
	CONFOPTS="${CONFOPTS} --enable-openssl-backend"
	CONFOPTS="${CONFOPTS} --disable-fatalwarnings"
	CONFOPTS="${CONFOPTS} --with-xml-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libcurl=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR}"

	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" \
	    LDFLAGS="-L${BUILTDIR}/lib -arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	if [ -f "${BUILTDIR}/lib/libcurl.dylib" ]; then
		mv "${BUILTDIR}/lib/libcurl.dylib" "${BUILTDIR}/lib/libcurl.dylib_x"
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

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --disable-shared"
	CONFOPTS="${CONFOPTS} --enable-debug"
	CONFOPTS="${CONFOPTS} --enable-openssl-backend"
	CONFOPTS="${CONFOPTS} --disable-fatalwarnings"
	CONFOPTS="${CONFOPTS} --with-xml-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libcurl=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR}"

	autoheader && glibtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo configure build ok
	./configure ${CONFOPTS} \
	    CFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CXXFLAGS="-arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}" \
	    CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" \
	    LDFLAGS="-L${BUILTDIR}/lib -arch i386 -mmacosx-version-min=${OSX_MIN_VER} -isysroot ${ISYSROOT}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	if [ -f "${BUILTDIR}/lib/libcurl.dylib" ]; then
		mv "${BUILTDIR}/lib/libcurl.dylib" "${BUILTDIR}/lib/libcurl.dylib_x"
	fi
fi

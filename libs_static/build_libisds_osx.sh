#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)

SRCDIR="${SCRIPT_LOCATION}/srcs"
WORKDIR="${SCRIPT_LOCATION}/work"
BUILTDIR="${SCRIPT_LOCATION}/built"

if [ ! -d "${SRCDIR}" ]; then
	mkdir "${SRCDIR}"
fi

if [ ! -d "${WORKDIR}" ]; then
	mkdir "${WORKDIR}"
fi

if [ ! -d "${BUILTDIR}" ]; then
	mkdir "${BUILTDIR}"
fi

USE_SYSTEM_CURL="yes"


SDKROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs"
ISYSROOT="${SDKROOT}/MacOSX10.7.sdk"

ZLIB_ARCHIVE=zlib-1.2.8.tar.xz
EXPAT_ARCHIVE=expat-2.1.0.tar.gz
LIBTOOL_ARCHIVE=libtool-2.4.3.tar.xz

LIBICONV_ARCHIVE=libiconv-1.14.tar.gz
LIBXML2_ARCHIVE=libxml2-2.9.2.tar.gz
GETTEXT_ARCHIVE=gettext-0.19.3.tar.xz

# Libcurl is already available in the system.
LIBCURL_ARCHIVE=curl-7.39.0.tar.gz
OPENSSL_ARCHIVE=openssl-1.0.1j.tar.gz

LIBISDS_GIT="https://gitlab.labs.nic.cz/kslany/libisds.git"
LIBISDS_BRANCH="feature-openssl"


if [ ! -z "${ZLIB_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${ZLIB_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
		exit 1
	fi
	# zlib
	rm -rf "${WORKDIR}"/zlib*
	cd "${WORKDIR}"
	tar -xJf "${ARCHIVE}"
	cd "${WORKDIR}"/zlib*
	./configure --prefix=${BUILTDIR} --static --archs="-arch i386"
	make && make install || exit 1
fi


if [ ! -z "${EXPAT_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${EXPAT_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
		exit 1
	fi
	# expat
	rm -rf "${WORKDIR}"/expat*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/expat*
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386"
	make && make install || exit 1
fi


if [ ! -z "${LIBTOOL_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBTOOL_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
		exit 1
	fi
	# libtool
	rm -rf "${WORKDIR}"/libtool*
	cd "${WORKDIR}"
	tar -xJf "${ARCHIVE}"
	cd "${WORKDIR}"/libtool*
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386"
	make && make install || exit 1
fi


if [ ! -z "${LIBICONV_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBICONV_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
		exit 1
	fi
	# libiconv
	rm -rf "${WORKDIR}"/libiconv*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/libiconv*
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386"
	make && make install || exit 1
fi


if [ ! -z "${LIBXML2_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBXML2_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
		exit 1
	fi
	# libxml2
	rm -rf "${WORKDIR}"/libxml2*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/libxml2*
	./configure --without-python --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386" --with-iconv="${BUILTDIR}"
	make && make install || exit 1
fi


if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${GETTEXT_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
		exit 1
	fi
	# gettext
	rm -rf "${WORKDIR}"/gettext*
	cd "${WORKDIR}"
	tar -xJf "${ARCHIVE}"
	cd "${WORKDIR}"/gettext*
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386" --with-libxml2-prefix="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include" LDFLAGS="-L${BUILTDIR}/lib"
	make && make install || exit 1
fi


if [ "x${USE_SYSTEM_CURL}" != "xyes" ] && [ ! -z "${LIBCURL_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${LIBCURL_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
		exit 1
	fi
	# libcurl
	rm -rf "${WORKDIR}"/curl*
	cd "${WORKDIR}"
	tar -xzf "${ARCHIVE}"
	cd "${WORKDIR}"/curl*
	# --disable-shared
	./configure --enable-ipv6 --with-darwinssl --without-axtls --disable-ldap --prefix="${BUILTDIR}" CFLAGS="-arch i386 -isysroot ${ISYSROOT}" CXXFLAGS="-arch i386 -isysroot ${ISYSROOT}" LDFLAGS="-arch i386 -isysroot ${ISYSROOT}"
	#make ##&& make install || exit 1
fi


if [ ! -z "${OPENSSL_ARCHIVE}" ]; then
	ARCHIVE="${SRCDIR}/${OPENSSL_ARCHIVE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing ${ARCHIVE}"
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
	make && make install_sw || exit 1
fi


if [ ! -z "${LIBISDS_GIT}" ]; then
	# libisds with OpenSSL back-end
	rm -rf "${WORKDIR}"/libisds*
	cd "${WORKDIR}"
	git clone "${LIBISDS_GIT}" libisds-git
	cd "${WORKDIR}"/libisds*
	git checkout "${LIBISDS_BRANCH}"
	autoheader && glibtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo configure build ok
	./configure --enable-debug --enable-openssl-backend --disable-fatalwarnings --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386 -isysroot ${ISYSROOT}" CXXFLAGS="-arch i386 -isysroot ${ISYSROOT}" --with-xml-prefix="${BUILTDIR}" --with-libcurl="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" LDFLAGS="-arch i386 -isysroot ${ISYSROOT} -L${BUILTDIR}/lib"
	make && make install || exit 1
	if [ -f "${BUILTDIR}/lib/libcurl.dylib" ]; then
		mv "${BUILTDIR}/lib/libcurl.dylib" "${BUILTDIR}/lib/libcurl.dylib_x"
	fi
fi

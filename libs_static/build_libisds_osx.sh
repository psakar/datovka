#!/usr/bin/env sh

SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)

. "${SCRIPT_LOCATION}"/../scripts/dependency_sources.sh

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

SDKROOT="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs"
ISYSROOT="${SDKROOT}/MacOSX10.7.sdk"

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
	./configure --prefix=${BUILTDIR} --static --archs="-arch i386"
	make && make install || exit 1
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
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386"
	make && make install || exit 1
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
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386"
	make && make install || exit 1
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
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386"
	make && make install || exit 1
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
	./configure --without-python --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386" --with-iconv="${BUILTDIR}"
	make && make install || exit 1
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
	./configure --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386" --with-libxml2-prefix="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include" LDFLAGS="-L${BUILTDIR}/lib"
	make && make install || exit 1
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
	# --disable-shared
	./configure --enable-ipv6 --with-darwinssl --without-axtls --disable-ldap --prefix="${BUILTDIR}" CFLAGS="-arch i386 -isysroot ${ISYSROOT}" CXXFLAGS="-arch i386 -isysroot ${ISYSROOT}" LDFLAGS="-arch i386 -isysroot ${ISYSROOT}"
	#make ##&& make install || exit 1
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
	make && make install_sw || exit 1
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

	./configure --enable-debug --enable-openssl-backend --disable-fatalwarnings --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386 -isysroot ${ISYSROOT}" CXXFLAGS="-arch i386 -isysroot ${ISYSROOT}" --with-xml-prefix="${BUILTDIR}" --with-libcurl="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" LDFLAGS="-arch i386 -isysroot ${ISYSROOT} -L${BUILTDIR}/lib"
	make && make install || exit 1

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
	autoheader && glibtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo configure build ok
	./configure --enable-debug --enable-openssl-backend --disable-fatalwarnings --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386 -isysroot ${ISYSROOT}" CXXFLAGS="-arch i386 -isysroot ${ISYSROOT}" --with-xml-prefix="${BUILTDIR}" --with-libcurl="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" LDFLAGS="-arch i386 -isysroot ${ISYSROOT} -L${BUILTDIR}/lib"
	make && make install || exit 1

	if [ -f "${BUILTDIR}/lib/libcurl.dylib" ]; then
		mv "${BUILTDIR}/lib/libcurl.dylib" "${BUILTDIR}/lib/libcurl.dylib_x"
	fi
fi

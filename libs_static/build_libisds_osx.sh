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

ZLIB_ARCHIVE=zlib-1.2.8.tar.xz
EXPAT_ARCHIVE=expat-2.1.0.tar.gz
LIBTOOL_ARCHIVE=libtool-2.4.3.tar.xz

LIBICONV_ARCHIVE=libiconv-1.14.tar.gz
LIBXML2_ARCHIVE=libxml2-2.9.2.tar.gz
GETTEXT_ARCHIVE=gettext-0.19.3.tar.xz

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

if [ ! -z "${LIBCURL_ARCHIVE}" ]; then
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
	#--disable-shared
	./configure --enable-ipv6 --with-darwinssl --without-axtls --disable-ldap --prefix="${BUILTDIR}" CFLAGS="-arch i386" CXXFLAGS="-arch i386"
	make && make install || exit 1
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
	make && make install_sw exit 1
fi


if [ ! -z "${LIBISDS_GIT}" ]; then
	# libisds with OpenSSL back-end
	rm -rf "${WORKDIR}"/libisds*
	cd "${WORKDIR}"
	git clone "${LIBISDS_GIT}" libisds-git
	cd "${WORKDIR}"/libisds*
	git checkout "${LIBISDS_BRANCH}"
	autoheader && glibtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo configure build ok
	./configure --enable-debug --enable-openssl-backend --disable-fatalwarnings --prefix="${BUILTDIR}" --disable-shared CFLAGS="-arch i386" CXXFLAGS="-arch i386" --with-xml-prefix="${BUILTDIR}" --with-libcurl="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" LDFLAGS="-arch i386 -L${BUILTDIR}/lib"
	make && make install || exit 1
	mv "${BUILTDIR}/lib/libcurl.dylib" "${BUILTDIR}/lib/libcurl.dylib_x"
fi

#../libtool --tag=CC --mode=link i686-pc-mingw32-gcc -g -O2 -g -std=c99 -Wall -version-info 8:0:3 -R${BUILTDIR}/lib -L${BUILTDIR}/lib -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo  libisds_la-soap.lo libisds_la-win32.lo -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto

#../libtool -v --tag=CC --mode=link i686-pc-mingw32-gcc -O2 -g -std=c99 -Wall -version-info 8:0:3 -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo  libisds_la-soap.lo libisds_la-win32.lo -R${BUILTDIR}/lib -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto

#i686-pc-mingw32-gcc -shared -O2 -g -std=c99 -Wall -o libisds.so libisds_la-cdecode.o libisds_la-cencode.o libisds_la-isds.o libisds_la-physxml.o libisds_la-utils.o libisds_la-validator.o libisds_la-crypto_openssl.o libisds_la-soap.o libisds_la-win32.o -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto

# -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto

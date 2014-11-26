#!/usr/bin/env sh

SCRIPT=$(readlink -f "$0")
SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))

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

X86_MINGV_HOST=i586-mingw32msvc
#X86_MINGV_HOST=i686-pc-mingw32
X86_MINGW_PREFIX=${X86_MINGV_HOST}-
X86_MINGW_CC=${X86_MINGV_HOST}-gcc
X86_MINGW_LD=${X86_MINGV_HOST}-ld
X86_MINGW_STRIP=${X86_MINGV_HOST}-strip
X86_MINGW_RANLIB=${X86_MINGV_HOST}-ranlib

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
	make -f win32/Makefile.gcc SHARED_MODE=1 PREFIX=${X86_MINGW_PREFIX} BINARY_PATH=${BUILTDIR}/bin INCLUDE_PATH=${BUILTDIR}/include LIBRARY_PATH=${BUILTDIR}/lib install || exit 1
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
	./configure --prefix="${BUILTDIR}" --host="${X86_MINGV_HOST}"
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
	./configure --prefix="${BUILTDIR}" --host="${X86_MINGV_HOST}"
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
	# --disable-static
	./configure --prefix="${BUILTDIR}" --host="${X86_MINGV_HOST}"
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
	# --disable-static
	./configure --without-python --prefix="${BUILTDIR}" --host="${X86_MINGV_HOST}" --with-iconv="${BUILTDIR}"
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
	# --disable-static
	./configure --prefix="${BUILTDIR}" --host="${X86_MINGV_HOST}" --with-libxml2-prefix="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include" LDFLAGS="-L${BUILTDIR}/lib"
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
	# --disable-static
	./configure --enable-ipv6 --with-winssl --without-axtls --prefix="${BUILTDIR}" --host="${X86_MINGV_HOST}"
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
	./Configure mingw enable-static-engine shared no-krb5 --prefix="${BUILTDIR}" --cross-compile-prefix="${X86_MINGW_PREFIX}"
	make && make install_sw || exit 1
	cp libeay32.dll "${BUILTDIR}/bin/"
	cp ssleay32.dll "${BUILTDIR}/bin/"
fi


if [ ! -z "${LIBISDS_GIT}" ]; then
	# libisds with OpenSSL back-end
	rm -rf "${WORKDIR}"/libisds*
	cd "${WORKDIR}"
	git clone "${LIBISDS_GIT}" libisds-git
	cd "${WORKDIR}"/libisds*
	git checkout "${LIBISDS_BRANCH}"
	cat configure.ac | sed -e 's/AC_FUNC_MALLOC//g' > nomalloc_configure.ac
	mv nomalloc_configure.ac configure.ac
	autoheader && libtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo configure build ok
	./configure --enable-debug --enable-openssl-backend --disable-fatalwarnings --prefix="${BUILTDIR}" --host="${X86_MINGV_HOST}" --with-xml-prefix="${BUILTDIR}" --with-libcurl="${BUILTDIR}" --with-libiconv-prefix="${BUILTDIR}" CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2" LDFLAGS="-L${BUILTDIR}/lib"
	make
	cd src
	#i686-pc-mingw32-gcc -shared -O2 -g -std=c99 -Wall -o .libs/libisds.dll libisds_la-cdecode.o libisds_la-cencode.o libisds_la-isds.o libisds_la-physxml.o libisds_la-utils.o libisds_la-validator.o libisds_la-crypto_openssl.o libisds_la-soap.o libisds_la-win32.o -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto
	#../libtool -v --tag=CC --mode=link i686-pc-mingw32-gcc  -g -O2 -g -std=c99 -Wall -version-info 8:0:3 -L${BUILTDIR}/lib -lxml2 -lz -L${BUILTDIR}/lib -liconv -L${BUILTDIR}/lib -lcurl -lwldap32 -lz -lws2_32 -lexpat -L${BUILTDIR}/lib -lintl -L${BUILTDIR}/lib -liconv -R${BUILTDIR}/lib -L${BUILTDIR}/lib -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo  libisds_la-soap.lo libisds_la-win32.lo -L${BUILTDIR}/bin -leay32 -no-undefined
	../libtool -v --tag=CC --mode=link ${X86_MINGW_CC}  -g -O2 -g -std=c99 -Wall -version-info 8:0:3 -L${BUILTDIR}/lib -lxml2 -lz -L${BUILTDIR}/lib -liconv -L${BUILTDIR}/lib -lcurl -lwldap32 -lz -lws2_32 -lexpat -L${BUILTDIR}/lib -lintl -L${BUILTDIR}/lib -liconv -R${BUILTDIR}/lib -L${BUILTDIR}/lib -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo  libisds_la-soap.lo libisds_la-win32.lo -L${BUILTDIR}/bin -leay32 -no-undefined
	cd ..
	#make install
fi

#../libtool --tag=CC --mode=link i686-pc-mingw32-gcc -g -O2 -g -std=c99 -Wall -version-info 8:0:3 -R${BUILTDIR}/lib -L${BUILTDIR}/lib -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo  libisds_la-soap.lo libisds_la-win32.lo -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto

#../libtool -v --tag=CC --mode=link i686-pc-mingw32-gcc -O2 -g -std=c99 -Wall -version-info 8:0:3 -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo  libisds_la-soap.lo libisds_la-win32.lo -R${BUILTDIR}/lib -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto

#i686-pc-mingw32-gcc -shared -O2 -g -std=c99 -Wall -o libisds.so libisds_la-cdecode.o libisds_la-cencode.o libisds_la-isds.o libisds_la-physxml.o libisds_la-utils.o libisds_la-validator.o libisds_la-crypto_openssl.o libisds_la-soap.o libisds_la-win32.o -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto

# -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto
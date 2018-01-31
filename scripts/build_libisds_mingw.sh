#!/usr/bin/env sh

SCRIPT=$(readlink -f "$0")
SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))
SRC_ROOT=$(cd "${SCRIPT_LOCATION}"/..; pwd)

. "${SRC_ROOT}"/scripts/dependency_sources.sh

adjust_sources "mingw"

WIN_VER="0x0501" #https://msdn.microsoft.com/en-us/library/windows/desktop/aa383745%28v=vs.85%29.aspx
MAKEOPTS="-j 4"

LIB_ROOT="${SRC_ROOT}"/libs
if [ ! -d "${LIB_ROOT}" ]; then
	echo "Cannot find directory '${LIB_ROOT}'" >&2
	exit 1
fi
cd "${LIB_ROOT}"

SRCDIR="${LIB_ROOT}/srcs"
PATCHDIR="${SRC_ROOT}/scripts/patches"
WORKDIR_PREFIX="${LIB_ROOT}/work"
BUILTDIR_PREFIX="${LIB_ROOT}/built"


#X86_MINGV_HOST=i586-mingw32msvc # This old compiler chain isn't available on newer distros.
X86_MINGV_HOST=i686-w64-mingw32
#X86_MINGV_HOST=i686-pc-mingw32
X86_MINGW_PREFIX=${X86_MINGV_HOST}-
X86_MINGW_CC=${X86_MINGV_HOST}-gcc
X86_MINGW_LD=${X86_MINGV_HOST}-ld
X86_MINGW_STRIP=${X86_MINGV_HOST}-strip
X86_MINGW_RANLIB=${X86_MINGV_HOST}-ranlib

WORKDIR_PREFIX="${WORKDIR_PREFIX}_${X86_MINGV_HOST}"
BUILTDIR_PREFIX="${BUILTDIR_PREFIX}_${X86_MINGV_HOST}"

if [ ! -d "${SRCDIR}" ]; then
	mkdir "${SRCDIR}"
fi

# Specifies which targets to build.
TARGETS=""
#TARGETS="${TARGETS} static"
TARGETS="${TARGETS} dynamic"

# Return 0 if targets are OK.
check_params () {
	TYPE=$1
	if [ "x${TYPE}" != "xstatic" -a "x${TYPE}" != "xdynamic" ]; then
		echo "Unknown type '${TYPE}'." >&2
		return 1
	fi
	return 0
}

# Return 0 if target is scheduled for build.
target_scheduled () {
	TYPE=$1
	check_params "${TYPE}" || return 1
	RES=$(echo "${TARGETS}" | grep "${TYPE}")
	if [ "x${RES}" != "x" ]; then
		return 0
	else
		return 1
	fi
}

workdir_name () {
	TYPE=$1
	echo "${WORKDIR_PREFIX}_${TYPE}"
}

builtdir_name () {
	TYPE=$1
	echo "${BUILTDIR_PREFIX}_${TYPE}"
}

ensure_dir_presence () {
	mkdir -p "$1"
}

# Create missing directories.
target_scheduled static && ensure_dir_presence $(workdir_name static)
target_scheduled static && ensure_dir_presence $(builtdir_name static)
target_scheduled dynamic && ensure_dir_presence $(workdir_name dynamic)
target_scheduled dynamic && ensure_dir_presence $(builtdir_name dynamic)

# Store information about build.
store_build_info () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	BUILTDIR=$(builtdir_name "${TYPE}")

	${X86_MINGW_PREFIX}gcc -v 2> "${BUILTDIR}/gcc-version.txt"
	${X86_MINGW_PREFIX}g++ -v 2> "${BUILTDIR}/g++-version.txt"
	# Curly brackets don't work in plain Bourne shell.
	#cat /etc/*{release,version} > "${BUILTDIR}/linux-version.txt"
	cat /etc/*release /etc/*version > "${BUILTDIR}/linux-version.txt"
	uname -a > "${BUILTDIR}/uname.txt"
	git log -n 1 --pretty=format:"%h - %ad : %s" > "${BUILTDIR}/git.txt"
}

target_scheduled static && store_build_info static
target_scheduled dynamic && store_build_info dynamic


ZLIB_ARCHIVE="${_ZLIB_ARCHIVE}"
EXPAT_ARCHIVE="${_EXPAT_ARCHIVE}"
LIBTOOL_ARCHIVE="${_LIBTOOL_ARCHIVE}"

LIBICONV_ARCHIVE="${_LIBICONV_ARCHIVE}"
LIBXML2_ARCHIVE="${_LIBXML2_ARCHIVE}"
GETTEXT_ARCHIVE="${_GETTEXT_ARCHIVE}" # Enable NLS.

LIBCURL_ARCHIVE="${_LIBCURL_ARCHIVE}"
OPENSSL_ARCHIVE="${_OPENSSL_ARCHIVE}"

LIBISDS_ARCHIVE="${_LIBISDS_ARCHIVE}"
LIBISDS_ARCHIVE_PATCHES="${_LIBISDS_ARCHIVE_PATCHES}"
#LIBISDS_GIT="https://gitlab.labs.nic.cz/kslany/libisds.git"
#LIBISDS_BRANCH="feature-openssl" # Use master.


build_zlib () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${ZLIB_ARCHIVE}" "${WORKDIR}" zlib
	cd "${WORKDIR}"/zlib*

	SHARED="1"
	if [ "x${TYPE}" = "xstatic" ]; then
		SHARED="0"
	fi

	make ${MAKEOPTS} -f win32/Makefile.gcc \
	    SHARED_MODE=${SHARED} \
	    PREFIX=${X86_MINGW_PREFIX} \
	    BINARY_PATH=${BUILTDIR}/bin \
	    INCLUDE_PATH=${BUILTDIR}/include \
	    LIBRARY_PATH=${BUILTDIR}/lib \
	    install || exit 1

	if [ "x${TYPE}" = "xdynamic" ]; then
		rm -rf "${BUILTDIR}"/lib/libz.a
	fi

	return 0
}

if [ ! -z "${ZLIB_ARCHIVE}" ]; then
	echo "Building zlib."
	if target_scheduled static; then build_zlib static || exit 1; fi
	if target_scheduled dynamic; then build_zlib dynamic || exit 1; fi
fi


build_expat () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${EXPAT_ARCHIVE}" "${WORKDIR}" expat
	cd "${WORKDIR}"/expat*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi

	./configure ${CONFOPTS} --host="${X86_MINGV_HOST}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${EXPAT_ARCHIVE}" ]; then
	echo "Building expat."
	if target_scheduled static; then build_expat static || exit 1; fi
	if target_scheduled dynamic; then build_expat dynamic || exit 1; fi
fi


build_libtool () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBTOOL_ARCHIVE}" "${WORKDIR}" libtool
	cd "${WORKDIR}"/libtool*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi

	./configure ${CONFOPTS} --host="${X86_MINGV_HOST}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${LIBTOOL_ARCHIVE}" ]; then
	echo "Building libtool."
	if target_scheduled static; then build_libtool static || exit 1; fi
	if target_scheduled dynamic; then build_libtool dynamic || exit 1; fi
fi


build_libiconv () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBICONV_ARCHIVE}" "${WORKDIR}" libiconv
	cd "${WORKDIR}"/libiconv*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi

	./configure ${CONFOPTS} --host="${X86_MINGV_HOST}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${LIBICONV_ARCHIVE}" ]; then
	echo "Building libiconv."
	if target_scheduled static; then build_libiconv static || exit 1; fi
	if target_scheduled dynamic; then build_libiconv dynamic || exit 1; fi
fi


build_libxml2 () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBXML2_ARCHIVE}" "${WORKDIR}" libxml2
	cd "${WORKDIR}"/libxml2*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --without-python"
	CONFOPTS="${CONFOPTS} --with-iconv=${BUILTDIR}"

	./configure ${CONFOPTS} --host="${X86_MINGV_HOST}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${LIBXML2_ARCHIVE}" ]; then
	echo "Bulding libxml2."
	if target_scheduled static; then build_libxml2 static || exit 1; fi
	if target_scheduled dynamic; then build_libxml2 dynamic || exit 1; fi
fi


build_gettext () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${GETTEXT_ARCHIVE}" "${WORKDIR}" gettext
	cd "${WORKDIR}"/gettext*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --with-libxml2-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --enable-threads=win32"
	CONFOPTS="${CONFOPTS} --enable-relocatable"

	DEFINES=""
	if [ "x${TYPE}" = "xstatic" ]; then
		DEFINES="${DEFINES} -DLIBXML_STATIC"
	fi

	./configure ${CONFOPTS} --host="${X86_MINGV_HOST}" \
	    CPPFLAGS="-I${BUILTDIR}/include ${DEFINES}" \
	    LDFLAGS="-L${BUILTDIR}/lib"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS
	unset DEFINES

	return 0
}

if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
	echo "Building gettext."
	if target_scheduled static; then build_gettext static || exit 1; fi
	if target_scheduled dynamic; then build_gettext dynamic || exit 1; fi
fi


build_libcurl () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${LIBCURL_ARCHIVE}" "${WORKDIR}" curl
	cd "${WORKDIR}"/curl*

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --enable-ipv6"
	CONFOPTS="${CONFOPTS} --with-winssl"
	CONFOPTS="${CONFOPTS} --without-axtls"
	CONFOPTS="${CONFOPTS} --without-zsh-functions-dir"
	CONFOPTS="${CONFOPTS} --disable-ldap"
	CONFOPTS="${CONFOPTS} --disable-ldaps"
	CONFOPTS="${CONFOPTS} --disable-rtsp"
	#CONFOPTS="${CONFOPTS} --disable-sspi"

	./configure ${CONFOPTS} --host="${X86_MINGV_HOST}" \
	    CPPFLAGS="-DWINVER=${WIN_VER}"
	make ${MAKEOPTS} && make install || exit 1

	unset CONFOPTS

	return 0
}

if [ ! -z "${LIBCURL_ARCHIVE}" ]; then
	echo "Building libcurl."
	if target_scheduled static; then build_libcurl static || exit 1; fi
	if target_scheduled dynamic; then build_libcurl dynamic || exit 1; fi
fi


build_openssl () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	erase_and_decompress "${SRCDIR}" "${OPENSSL_ARCHIVE}" "${WORKDIR}" openssl
	cd "${WORKDIR}"/openssl*

	CONFOPTS=""
	#CONFOPTS="${CONFOPTS} no-asm"
	CONFOPTS="${CONFOPTS} mingw"
	CONFOPTS="${CONFOPTS} enable-static-engine"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} no-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} shared"
	fi
	CONFOPTS="${CONFOPTS} no-krb5"

	./Configure ${CONFOPTS} --prefix="${BUILTDIR}" --cross-compile-prefix="${X86_MINGW_PREFIX}"
	make depend || exit 1
	make ${MAKEOPTS} && make install_sw || exit 1

	if [ "x${TYPE}" = "xdynamic" ]; then
		cp libeay32.dll "${BUILTDIR}/bin/"
		cp ssleay32.dll "${BUILTDIR}/bin/"
	fi

	unset CONFOPTS

	return 0
}

if [ ! -z "${OPENSSL_ARCHIVE}" ]; then
	echo "Building openssl."
	if target_scheduled static; then build_openssl static || exit 1; fi
	if target_scheduled dynamic; then build_openssl dynamic || exit 1; fi
fi


build_libisds () {
	TYPE=$1
	check_params "${TYPE}" || exit 1
	WORKDIR=$(workdir_name "${TYPE}")
	BUILTDIR=$(builtdir_name "${TYPE}")

	if [ ! -z "${LIBISDS_ARCHIVE}" ]; then
		erase_and_decompress "${SRCDIR}" "${LIBISDS_ARCHIVE}" "${WORKDIR}" libisds
		cd "${WORKDIR}"/libisds*

		if [ "x${LIBISDS_ARCHIVE_PATCHES}" != "x" ]; then
			# Apply patches.
			for f in ${LIBISDS_ARCHIVE_PATCHES}; do
				PATCHFILE="${PATCHDIR}/${f}"
				if [ ! -f "${PATCHFILE}" ]; then
					echo "Missing ${PATCHFILE}" >&2
					exit 1
				fi
				cp "${PATCHFILE}" ./
				echo "Applying ${f}"
				patch -p1 < ${f}
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

	CONFOPTS=""
	CONFOPTS="${CONFOPTS} --prefix=${BUILTDIR}"
	if [ "x${TYPE}" = "xstatic" ]; then
		CONFOPTS="${CONFOPTS} --disable-shared"
	fi
	if [ "x${TYPE}" = "xdynamic" ]; then
		CONFOPTS="${CONFOPTS} --disable-static"
	fi
	CONFOPTS="${CONFOPTS} --enable-debug"
	CONFOPTS="${CONFOPTS} --enable-openssl-backend"
	CONFOPTS="${CONFOPTS} --disable-fatalwarnings"
	CONFOPTS="${CONFOPTS} --with-xml-prefix=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libcurl=${BUILTDIR}"
	CONFOPTS="${CONFOPTS} --with-libiconv-prefix=${BUILTDIR}"

	LINTL=""
	NLS="--disable-nls"
	if [ ! -z "${GETTEXT_ARCHIVE}" ]; then
		LINTL="-lintl"
		NLS=""
	fi
	CONFOPTS="${CONFOPTS} ${NLS}"

	DEFINES=""
	LINKER=""
	if [ "x${TYPE}" = "xstatic" ]; then
		DEFINES="${DEFINES} -DLIBXML_STATIC"
		LINKER="${LINKER} -mwindows"
	fi

	if [ -z "${LIBISDS_ARCHIVE}" -a ! -z "${LIBISDS_GIT}" ]; then
		cat configure.ac | sed -e 's/AC_FUNC_MALLOC//g' > nomalloc_configure.ac
		mv nomalloc_configure.ac configure.ac
		autoheader && libtoolize -c --install && aclocal -I m4 && automake --add-missing --copy && autoconf && echo "configure build ok"
	fi
	# Receiving undefined reference to `rpl_malloc' with i686-w64-mingw32.
	# The following variable forces the malloc check to pass.
	ac_cv_func_malloc_0_nonnull=yes \
	./configure ${CONFOPTS} --host="${X86_MINGV_HOST}" \
	    CPPFLAGS="-I${BUILTDIR}/include -I${BUILTDIR}/include/libxml2 ${DEFINES}" \
	    LDFLAGS="-L${BUILTDIR}/lib ${LINKER}"
	make ${MAKEOPTS}
	if [ "x${TYPE}" = "xdynamic" ]; then
		cd src
		#i686-pc-mingw32-gcc -shared -O2 -g -std=c99 -Wall -o .libs/libisds.dll libisds_la-cdecode.o libisds_la-cencode.o libisds_la-isds.o libisds_la-physxml.o libisds_la-utils.o libisds_la-validator.o libisds_la-crypto_openssl.o libisds_la-soap.o libisds_la-win32.o -L${BUILTDIR}/lib -lxml2 -liconv -lcurl -lexpat -lintl -lcrypto
		#../libtool -v --tag=CC --mode=link i686-pc-mingw32-gcc  -g -O2 -g -std=c99 -Wall -version-info 8:0:3 -L${BUILTDIR}/lib -lxml2 -lz -L${BUILTDIR}/lib -liconv -L${BUILTDIR}/lib -lcurl -lwldap32 -lz -lws2_32 -lexpat -L${BUILTDIR}/lib -lintl -L${BUILTDIR}/lib -liconv -R${BUILTDIR}/lib -L${BUILTDIR}/lib -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo libisds_la-soap.lo libisds_la-win32.lo -L${BUILTDIR}/bin -leay32 -no-undefined
		../libtool -v --tag=CC --mode=link ${X86_MINGW_CC} -g -O2 -g -std=c99 -Wall -version-info 8:0:3 -L${BUILTDIR}/lib -lxml2 -lz -L${BUILTDIR}/lib -liconv -L${BUILTDIR}/lib -lcurl -lwldap32 -lz -lws2_32 -lexpat -L${BUILTDIR}/lib ${LINTL} -L${BUILTDIR}/lib -liconv -R${BUILTDIR}/lib -L${BUILTDIR}/lib -o libisds.la -rpath ${BUILTDIR}/lib libisds_la-cdecode.lo libisds_la-cencode.lo libisds_la-isds.lo libisds_la-physxml.lo libisds_la-utils.lo libisds_la-validator.lo libisds_la-crypto_openssl.lo libisds_la-soap.lo libisds_la-win32.lo -L${BUILTDIR}/bin -leay32 -no-undefined
		cd ..
	fi
	make install || exit 1

	unset CONFOPTS
	unset DEFINES
	unset LINKER

	return 0
}

if [ ! -z "${LIBISDS_ARCHIVE}" -a ! -z "${LIBISDS_GIT}" ]; then
	echo "Select libisds archive or git repository." >&2
	exit 1
elif [ ! -z "${LIBISDS_ARCHIVE}" -o ! -z "${LIBISDS_GIT}" ]; then
	echo "Building libisds."
	if target_scheduled static; then build_libisds static || exit 1; fi
	if target_scheduled dynamic; then build_libisds dynamic || exit 1; fi
fi

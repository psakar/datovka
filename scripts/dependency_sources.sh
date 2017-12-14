#!/usr/bin/env sh

# Latest libraries.
_ZLIB_ARCHIVE="zlib-1.2.11.tar.xz"
_EXPAT_ARCHIVE="expat-2.2.5.tar.bz2"
_LIBTOOL_ARCHIVE="libtool-2.4.6.tar.xz"

_LIBICONV_ARCHIVE="libiconv-1.15.tar.gz"
_LIBXML2_ARCHIVE="libxml2-2.9.7.tar.gz"
_GETTEXT_ARCHIVE="gettext-0.19.8.1.tar.xz"

_LIBCURL_ARCHIVE="curl-7.57.0.tar.xz"
_OPENSSL_ARCHIVE="openssl-1.0.2n.tar.gz"

_LIBISDS_ARCHIVE="libisds-0.10.7.tar.xz"
_LIBISDS_ARCHIVE_PATCHES=" \
	"
_LIBISDS_GIT="https://gitlab.labs.nic.cz/kslany/libisds.git"
#_LIBISDS_BRANCH="feature-openssl" # Use master.

# Adjusts sources according to the environment it is being compilled in.
adjust_sources () {
	PARAM="$1"
	if [ "x${PARAM}" = "x" ]; then
		echo "Use parameter for 'adjust_sources'" >&2
		exit 1
	fi

	case "${PARAM}" in
	mingw)
		# Latest gettext fails to compile with Mingw.
		_GETTEXT_ARCHIVE="gettext-0.19.7.tar.xz"
		echo "Using ${_GETTEXT_ARCHIVE}."
		;;
	osx)
		# libxml2 past version 2.9.2, which does compile, fail
		# to compile on OS X with the error:
		# xmlIO.c:1357:52: error: use of undeclared identifier 'LZMA_OK'
		#     ret =  (__libxml2_xzclose((xzFile) context) == LZMA_OK ) ? 0 : -1;
		# Possible solution of to disable lzma support.
		# See also https://github.com/sparklemotion/nokogiri/issues/1445
		_LIBXML2_ARCHIVE="libxml2-2.9.7.tar.gz"
		echo "Using ${_LIBXML2_ARCHIVE}"
		;;
	*)
		echo "Using defaults."
		;;
	esac

	# Make the warning more distinct.
	sleep 3
}

# Decompress compressed archive.
decompress_archive () {
	ARCHIVE="$1"
	if [ "x${ARCHIVE}" = "x" ]; then
		echo "Use parameter for '$0'" >&2
		exit 1
	fi

	DECOMPRESS_CMD=""
	case "${ARCHIVE}" in
	*.tar.gz)
		DECOMPRESS_CMD="tar -xzf"
		;;
	.tar.bz2)
		DECOMPRESS_CMD="tar -xjf"
		;;
	.tar.xz)
		DECOMPRESS_CMD="tar -xJf"
		;;
	*)
		;;
	esac

	if [ "x${DECOMPRESS_CMD}" = "x" ]; then
		echo "Don't know how to decompress '${ARCHIVE}'." >&2
		exit 1
	fi

	${DECOMPRESS_CMD} "${ARCHIVE}" || exit 1
}

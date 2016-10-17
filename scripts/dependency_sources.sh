#!/usr/bin/env sh

# Latest libraries.
_ZLIB_ARCHIVE="zlib-1.2.8.tar.xz"
_EXPAT_ARCHIVE="expat-2.2.0.tar.bz2"
_LIBTOOL_ARCHIVE="libtool-2.4.6.tar.xz"

_LIBICONV_ARCHIVE="libiconv-1.14.tar.gz"
_LIBXML2_ARCHIVE="libxml2-2.9.4.tar.gz"
_GETTEXT_ARCHIVE="gettext-0.19.8.1.tar.xz"

_LIBCURL_ARCHIVE="curl-7.50.3.tar.bz2"
_OPENSSL_ARCHIVE="openssl-1.0.2j.tar.gz"

_LIBISDS_ARCHIVE="libisds-0.10.6.tar.xz"
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

	case "$1" in
	mingw)
		# Latest gettext fails to compile with Mingw.
		_GETTEXT_ARCHIVE="gettext-0.19.7.tar.xz"
		echo "Using ${_GETTEXT_ARCHIVE}."
		;;
	osx)
		# Latest libxml2 fails to compile on OS X.
		_LIBXML2_ARCHIVE="libxml2-2.9.2.tar.gz"
		echo "Using ${_LIBXML2_ARCHIVE}"
		;;
	*)
		echo "Using defaults."
		;;
	esac

	# Make the warning more distinct.
	sleep 3
}

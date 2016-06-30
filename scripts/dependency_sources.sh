
_ZLIB_ARCHIVE="zlib-1.2.8.tar.xz"
_EXPAT_ARCHIVE="expat-2.2.0.tar.bz2"
_LIBTOOL_ARCHIVE="libtool-2.4.6.tar.xz"

_LIBICONV_ARCHIVE="libiconv-1.14.tar.gz"
_LIBXML2_ARCHIVE="libxml2-2.9.4.tar.gz"
_GETTEXT_ARCHIVE="gettext-0.19.8.tar.xz"

_LIBCURL_ARCHIVE="curl-7.49.1.tar.bz2"
_OPENSSL_ARCHIVE="openssl-1.0.2h.tar.gz"

_LIBISDS_ARCHIVE="libisds-0.10.1.tar.xz" # 10.4 is available but we have not tested it
_LIBISDS_ARCHIVE_PATCHES=" \
	001-0.10.1-string-precision-type-fix.patch \
	002-0.10.1-replace-positional-arguments.patch \
	003-0.10.1-publish-ovmid-fix.patch \
	"
_LIBISDS_GIT="https://gitlab.labs.nic.cz/kslany/libisds.git"
#_LIBISDS_BRANCH="feature-openssl" # Use master.

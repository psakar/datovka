#!/usr/bin/env sh

# Latest libraries.
_ZLIB_ARCHIVE="zlib-1.2.11.tar.xz"
_ZLIB_URL_PREFIX="http://zlib.net/"
_ZLIB_SHA256="4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066"
_ZLIB_SIG_SUFF=".asc"
_ZLIB_KEY_FP="5ED46A6721D365587791E2AA783FCD8E58BCAFBA" # "Mark Adler <madler@alumni.caltech.edu>"
_EXPAT_ARCHIVE="expat-2.2.5.tar.bz2"
_EXPAT_URL_PREFIX="https://github.com/libexpat/libexpat/releases/download/R_2_2_5/"
_EXPAT_SHA256="d9dc32efba7e74f788fcc4f212a43216fc37cf5f23f4c2339664d473353aedf6" # Locally computed hash.
_EXPAT_SIG_SUFF=""
_EXPAT_KEY_FP="" #
_LIBTOOL_ARCHIVE="libtool-2.4.6.tar.xz"
_LIBTOOL_URL_PREFIX="http://ftpmirror.gnu.org/libtool/"
_LIBTOOL_SHA256=""
_LIBTOOL_SIG_SUFF=".sig"
_LIBTOOL_KEY_FP="CFE2BE707B538E8B26757D84151308092983D606" # "Gary Vaughan (Free Software Developer) <gary@vaughan.pe>"

_LIBICONV_ARCHIVE="libiconv-1.15.tar.gz"
_LIBICONV_URL_PREFIX="https://ftp.gnu.org/pub/gnu/libiconv/"
_LIBICONV_SHA256=""
_LIBICONV_SIG_SUFF=".sig"
_LIBICONV_KEY_FP="68D94D8AAEEAD48AE7DC5B904F494A942E4616C2" # "Bruno Haible (Open Source Development) <bruno@clisp.org>"
_LIBXML2_ARCHIVE="libxml2-2.9.8.tar.gz"
_LIBXML2_URL_PREFIX="ftp://xmlsoft.org/libxml2/"
_LIBXML2_SHA256=""
_LIBXML2_SIG_SUFF=".asc"
_LIBXML2_KEY_FP="DB46681BB91ADCEA170FA2D415588B26596BEA5D" # "Daniel Veillard (Red Hat work email) <veillard@redhat.com>"
_GETTEXT_ARCHIVE="gettext-0.19.8.1.tar.xz"
_GETTEXT_URL_PREFIX="http://ftp.gnu.org/pub/gnu/gettext/"
_GETTEXT_SHA256=""
_GETTEXT_SIG_SUFF=".sig"
_GETTEXT_KEY_FP="462225C3B46F34879FC8496CD605848ED7E69871" # "Daiki Ueno <ueno@unixuser.org>"

_LIBCURL_ARCHIVE="curl-7.59.0.tar.xz"
_LIBCURL_URL_PREFIX="https://curl.haxx.se/download/"
_LIBCURL_SHA256=""
_LIBCURL_SIG_SUFF=".asc"
_LIBCURL_KEY_FP="27EDEAF22F3ABCEB50DB9A125CC908FDB71E12C2" # "Daniel Stenberg <daniel@haxx.se>"
_OPENSSL_ARCHIVE="openssl-1.0.2n.tar.gz"
_OPENSSL_URL_PREFIX="https://www.openssl.org/source/"
_OPENSSL_SHA256="370babb75f278c39e0c50e8c4e7493bc0f18db6867478341a832a982fd15a8fe"
_OPENSSL_SIG_SUFF=".asc"
_OPENSSL_KEY_FP="8657ABB260F056B1E5190839D9C4D26D0E604491" # "Matt Caswell <matt@openssl.org>"

_LIBISDS_ARCHIVE="libisds-0.10.7.tar.xz"
_LIBISDS_URL_PREFIX="http://xpisar.wz.cz/libisds/dist/"
_LIBISDS_SHA256=""
_LIBISDS_SIG_SUFF=".asc"
_LIBISDS_KEY_FP="4B528393E6A3B0DFB2EF3A6412C9C5C767C6FAA2" # "Petr Pisar <petr.pisar@atlas.cz>"
_LIBISDS_ARCHIVE_PATCHES=" \
	"
_LIBISDS_GIT="https://gitlab.labs.nic.cz/kslany/libisds.git"
#_LIBISDS_BRANCH="feature-openssl" # Use master.

# Compute SHA256 checksum.
check_sha256 () {
	local FILE_NAME="$1"
	local SHA256_HASH="$2"

	local CMD_OPENSSL=openssl
	local CMD_SED=sed
	if [ $(uname) = "Darwin" ]; then
		# OS X version of sed does not recognise \s as white space
		# identifier.
		CMD_SED=gsed
	fi

	if [ "x${FILE_NAME}" = "x" ]; then
		echo "No file name supplied." >&2
		return 1
	fi
	if [ "x${SHA256_HASH}" = "x" ]; then
		echo "No sha256 checksum for '${FILE_NAME}' supplied." >&2
		return 1
	fi

	if [ -z $(command -v "${CMD_OPENSSL}") ]; then
		echo "Install '${CMD_OPENSSL}' to be able to compute file checksum." >&2
		return 1
	fi
	if [ -z $(command -v "${CMD_SED}") ]; then
		echo "Install '${CMD_SED}' to be able to compute file checksum." >&2
		return 1
	fi

	COMPUTED_SHA256=$("${CMD_OPENSSL}" sha256 "${FILE_NAME}" | "${CMD_SED}" -e 's/^.*\s//g')

	if [ "x${COMPUTED_SHA256}" != "x${SHA256_HASH}" ]; then
		echo "'${FILE_NAME}' sha256 checksum mismatch." >&2
		echo "'${FILE_NAME}' computed sha256: '${COMPUTED_SHA256}'" >&2
		echo "'${FILE_NAME}' expected sha256: '${SHA256_HASH}'" >&2
		return 1
	fi

	echo "'${FILE_NAME}' sha256 checksum OK."
	return 0
}

# Check signature.
check_sig () {
	local FILE_NAME="$1"
	local SIG_FILE_NAME="$2"
	local KEY_FP="$3"

	local CMD_GPG=gpg

	if [ "x${FILE_NAME}" = "x" ]; then
		echo "No file name supplied." >&2
		return 1
	fi
	if [ "x${SIG_FILE_NAME}" = "x" ]; then
		echo "No signature file name supplied." >&2
		return 1
	fi
	if [ "x${KEY_FP}" = "x" ]; then
		echo "Missing key fingerprint to signature of '${ARCHIVE_FILE}'." >&2
		return 1
	fi

	if [ -z $(command -v "${CMD_GPG}") ]; then
		echo "Install '${CMD_GPG}' to be able to compute file checksum." >&2
		return 1
	fi

	local GPG_DIR="./.gnupg_work"
	rm -rf "${GPG_DIR}"
	mkdir -p "${GPG_DIR}"

	local GPG_PARAMS="--homedir ${GPG_DIR} --no-default-keyring --keyring keys.gpg --no-permission-warning"
	local KEY_SRVR="sks.labs.nic.cz"
	"${CMD_GPG}" ${GPG_PARAMS} --keyserver "${KEY_SRVR}" --recv "${KEY_FP}"
	"${CMD_GPG}" ${GPG_PARAMS} --verify "${SIG_FILE_NAME}" "${FILE_NAME}"
	local GPG_RET="$?"
	rm -rf "${GPG_DIR}"
	if [ "${GPG_RET}" != "0" ]; then
		echo "'${FILE_NAME}' signature check failed." >&2
		return 1
	fi

	echo "'${FILE_NAME}' signature OK."
	return 0
}

# Download file.
download_file () {
	local ARCHIVE_FILE="$1"
	local URL_PREFIX="$2"

	local CMD_CURL=curl

	if [ "x${ARCHIVE_FILE}" = "x" ]; then
		echo "Missing archive file name." >&2
		return 1
	fi
	if [ "x${URL_PREFIX}" = "x" ]; then
		echo "Missing URL prefix for archive '${ARCHIVE_FILE}'." >&2
		return 1
	fi

	if [ -z $(command -v "${CMD_CURL}") ]; then
		echo ""
		echo "Install '${CMD_CURL}' to be able to compute file checksum." >&2
		return 1
	fi

	"${CMD_CURL}" -L -O "${URL_PREFIX}${ARCHIVE_FILE}"
}

# Download sources.
ensure_source_presence () {
	local SRC_DIR="$1"
	local ARCHIVE_FILE="$2"
	local URL_PREFIX="$3"
	local SHA256_HASH="$4"
	local SIG_SUF="$5"
	local KEY_FP="$6"

	local ORIG_DIR=$(pwd)
	local ARCHIVE_SIG_FILE=""

	if [ "x${SRC_DIR}" = "x" -o ! -d "${SRC_DIR}" ]; then
		echo "'${SRC_DIR}' is not a directory." >&2
		return 1
	fi
	if [ "x${ARCHIVE_FILE}" = "x" -o -e "${SRC_DIR}/${ARCHIVE_FILE}" -a ! -f "${SRC_DIR}/${ARCHIVE_FILE}" ]; then
		echo "Missing archive file '${ARCHIVE_FILE}' argument or '${SRC_DIR}/${ARCHIVE_FILE}' is not a file." >&2
		return 1
	fi
	if [ "x${URL_PREFIX}" = "x" ]; then
		echo "Missing URL prefix for archive '${ARCHIVE_FILE}'." >&2
		return 1
	fi
	if [ "x${SHA256_HASH}" = "x" -a "x${SIG_SUF}" = "x" ]; then
		echo "Cannot check '${ARCHIVE_FILE}' because no hash or signature is supplied." >&2
		return 1
	fi
	if [ "x${SIG_SUF}" != "x" ]; then
		ARCHIVE_SIG_FILE="${ARCHIVE_FILE}${SIG_SUF}"
		if [ -e "${SRC_DIR}/${ARCHIVE_SIG_FILE}" -a ! -f "${SRC_DIR}/${ARCHIVE_SIG_FILE}" ]; then
			echo "'${SRC_DIR}/${ARCHIVE_SIG_FILE}' is not a file." >&2
			return 1
		fi
		if [ "x${KEY_FP}" = "x" ]; then
			echo "Missing key fingerprint to signature of '${ARCHIVE_FILE}'." >&2
			return 1
		fi
	fi

	cd "${SRC_DIR}"

	# Download signature file.
	if [ "x${ARCHIVE_SIG_FILE}" != "x" -a ! -e "${ARCHIVE_SIG_FILE}" ]; then
		if ! download_file "${ARCHIVE_SIG_FILE}" "${URL_PREFIX}"; then
			echo "Cannot download '${ARCHIVE_SIG_FILE}'." >&2
			return 1
		fi
		if [ ! -f "${ARCHIVE_SIG_FILE}" ]; then
			echo "'${ARCHIVE_SIG_FILE}' is missing." >&2
			return 1
		fi
	fi

	if [ -e "${ARCHIVE_FILE}" ]; then
		CHECK_SHA256_RET="0"
		if [ "x${SHA256_HASH}" != "x" ]; then
			check_sha256 "${ARCHIVE_FILE}" "${SHA256_HASH}"
			CHECK_SHA256_RET="$?"
		fi

		if [ "${CHECK_SHA256_RET}" = "0" ]; then
			if [ "x${ARCHIVE_SIG_FILE}" != "x" ]; then
				check_sig "${ARCHIVE_FILE}" "${ARCHIVE_SIG_FILE}" "${KEY_FP}" && return 0
			else
				return 0
			fi
		fi

		# Checksum mismatch.
		echo "Trying to download '${ARCHIVE_FILE}'." >&2
		mv "${ARCHIVE_FILE}" "bad_${ARCHIVE_FILE}"
		if [ "x${ARCHIVE_SIG_FILE}" != "x" ]; then
			echo "Trying to download '${ARCHIVE_SIG_FILE}'." >&2
			mv "${ARCHIVE_SIG_FILE}" "bad_${ARCHIVE_SIG_FILE}"
		fi
	fi

	# Download source file.
	if [ ! -e "${ARCHIVE_FILE}" ]; then
		if ! download_file "${ARCHIVE_FILE}" "${URL_PREFIX}"; then
			echo "Cannot download '${ARCHIVE_FILE}'." >&2
			return 1
		fi
	fi
	if [ ! -f "${ARCHIVE_FILE}" ]; then
		echo "'${ARCHIVE_FILE}' is missing." >&2
		return 1
	fi

	# Download signature file.
	if [ "x${ARCHIVE_SIG_FILE}" != "x" -a ! -e "${ARCHIVE_SIG_FILE}" ]; then
		if ! download_file "${ARCHIVE_SIG_FILE}" "${URL_PREFIX}"; then
			echo "Cannot download '${ARCHIVE_SIG_FILE}'." >&2
			return 1
		fi
		if [ ! -f "${ARCHIVE_SIG_FILE}" ]; then
			echo "'${ARCHIVE_SIG_FILE}' is missing." >&2
			return 1
		fi
	fi

	if [ "x${SHA256_HASH}" != "x" ]; then
		check_sha256 "${ARCHIVE_FILE}" "${SHA256_HASH}" || return 1
	fi
	if [ "x${ARCHIVE_SIG_FILE}" != "x" ]; then
		check_sig "${ARCHIVE_FILE}" "${ARCHIVE_SIG_FILE}" "${KEY_FP}" || return 1
	fi

	cd "${ORIG_DIR}"
	return 0
}

# Download all required sources.
download_all_sources () {
	local SRC_DIR="$1"

	if [ "x${SRC_DIR}" = "x" -o ! -d "${SRC_DIR}" ]; then
		echo "'${SRC_DIR}' is not a directory." >&2
		return 1
	fi

	ensure_source_presence "${SRC_DIR}" "${_ZLIB_ARCHIVE}" "${_ZLIB_URL_PREFIX}" \
	    "${_ZLIB_SHA256}" "${_ZLIB_SIG_SUFF}" "${_ZLIB_KEY_FP}" || return 1
	ensure_source_presence "${SRC_DIR}" "${_EXPAT_ARCHIVE}" "${_EXPAT_URL_PREFIX}" \
	    "${_EXPAT_SHA256}" "${_EXPAT_SIG_SUFF}" "${_EXPAT_KEY_FP}" || return 1
	ensure_source_presence "${SRC_DIR}" "${_LIBTOOL_ARCHIVE}" "${_LIBTOOL_URL_PREFIX}" \
	    "${_LIBTOOL_SHA256}" "${_LIBTOOL_SIG_SUFF}" "${_LIBTOOL_KEY_FP}" || return 1

	ensure_source_presence "${SRC_DIR}" "${_LIBICONV_ARCHIVE}" "${_LIBICONV_URL_PREFIX}" \
	    "${_LIBICONV_SHA256}" "${_LIBICONV_SIG_SUFF}" "${_LIBICONV_KEY_FP}" || return 1
	ensure_source_presence "${SRC_DIR}" "${_LIBXML2_ARCHIVE}" "${_LIBXML2_URL_PREFIX}" \
	    "${_LIBXML2_SHA256}" "${_LIBXML2_SIG_SUFF}" "${_LIBXML2_KEY_FP}" || return 1
	ensure_source_presence "${SRC_DIR}" "${_GETTEXT_ARCHIVE}" "${_GETTEXT_URL_PREFIX}" \
	    "${_GETTEXT_SHA256}" "${_GETTEXT_SIG_SUFF}" "${_GETTEXT_KEY_FP}" || return 1

	ensure_source_presence "${SRC_DIR}" "${_LIBCURL_ARCHIVE}" "${_LIBCURL_URL_PREFIX}" \
	    "${_LIBCURL_SHA256}" "${_LIBCURL_SIG_SUFF}" "${_LIBCURL_KEY_FP}" || return 1
	ensure_source_presence "${SRC_DIR}" "${_OPENSSL_ARCHIVE}" "${_OPENSSL_URL_PREFIX}" \
	    "${_OPENSSL_SHA256}" "${_OPENSSL_SIG_SUFF}" "${_OPENSSL_KEY_FP}" || return 1

	ensure_source_presence "${SRC_DIR}" "${_LIBISDS_ARCHIVE}" "${_LIBISDS_URL_PREFIX}" \
	    "${_LIBISDS_SHA256}" "${_LIBISDS_SIG_SUFF}" "${_LIBISDS_KEY_FP}" || return 1

	return 0
}

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
		_GETTEXT_SHA256="378fa86a091cec3acdece3c961bb8d8c0689906287809a8daa79dc0c6398d934"
		echo "Using '${_GETTEXT_ARCHIVE}'."
		;;
	osx)
		# libxml2 past version 2.9.2, which does compile, fail
		# to compile on OS X with the error:
		# xmlIO.c:1357:52: error: use of undeclared identifier 'LZMA_OK'
		#     ret =  (__libxml2_xzclose((xzFile) context) == LZMA_OK ) ? 0 : -1;
		# Possible solution of to disable lzma support.
		# See also https://github.com/sparklemotion/nokogiri/issues/1445

		#_LIBXML2_ARCHIVE="libxml2-2.9.8.tar.gz"
		#_LIBXML2_SHA256="0b74e51595654f958148759cfef0993114ddccccbb6f31aee018f3558e8e2732"
		echo "Using '${_LIBXML2_ARCHIVE}'."
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
	*.tar.bz2)
		DECOMPRESS_CMD="tar -xjf"
		;;
	*.tar.xz)
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

# First erase any potential targets, then decompress.
erase_and_decompress () {
	SRC_DIR="$1"
	ARCHIVE_FILE="$2"
	WORK_DIR="$3"
	ARCHIVE_NAME="$4"

	if [ "x${SRC_DIR}" = "x" ]; then
		echo "No source directory specified." >&2
		exit 1
	fi
	if [ "x${ARCHIVE_FILE}" = "x" ]; then
		echo "No archive file specified." >&2
		exit 1
	fi
	if [ "x${WORK_DIR}" = "x" ]; then
		echo "No working directory specified." >&2
		exit 1
	fi
	if [ "x${ARCHIVE_NAME}" = "x" ]; then
		echo "No archive name specified." >&2
		exit 1
	fi

	ARCHIVE="${SRC_DIR}/${ARCHIVE_FILE}"
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Missing file '${ARCHIVE}'." >&2
		exit 1
	fi
	rm -rf "${WORK_DIR}"/"${ARCHIVE_NAME}"*
	cd "${WORK_DIR}"
	decompress_archive "${ARCHIVE}"
}

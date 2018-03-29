#!/usr/bin/env sh

PACKAGE="libisds"
VERSION="0.10.7"
RELEASE="1"

SCRIPT_LOCATION=""
SYSTEM=$(uname -s)
if [ ! "x${SYSTEM}" = "xDarwin" ]; then
	SCRIPT=$(readlink -f "$0")
	SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))
else
	SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
fi

SRC_ROOT="${SCRIPT_LOCATION}/.."
cd "${SRC_ROOT}"

PACKAGE_SRC="${SRC_ROOT}/libs/srcs/${PACKAGE}-${VERSION}.tar.xz"
if [ ! -f "${PACKAGE_SRC}" ]; then
	echo "Cannot find '${PACKAGE_SRC}'." >&2
	exit 1
fi

DISTRO_WORK_DIR="_distrofiles/${PACKAGE}"
rm -rf "${DISTRO_WORK_DIR}"
if [ ! -d "${DISTRO_WORK_DIR}" ]; then
	rm -rf "${DISTRO_WORK_DIR}"
	mkdir -p "${DISTRO_WORK_DIR}"
fi

# Remove *.swp files.
rm_swp_files () {
	local DIR="$1"

	if [ ! -d "${DIR}" ]; then
		echo "'${DIR}' is not a directory." >&2
		return 1
	fi
	for f in $(find "${DISTRO_WORK_DIR}/"); do
		if [ -f "${f}" ]; then
			SWP_FILE=$(echo "${f}" | grep '[.]swp$')
			if [ "x${SWP_FILE}" != "x" ]; then
				echo "Removing '${SWP_FILE}'."
				rm "${SWP_FILE}"
			fi
		fi
	done
	return 0
}

cp -r "${SRC_ROOT}/distro/${PACKAGE}/"* "${DISTRO_WORK_DIR}"
rm_swp_files "${DISTRO_WORK_DIR}"

# Fill VERSION field in distribution specific files.
SUBST_FILES=$(find "${DISTRO_WORK_DIR}" | grep '[.]in$')
for f in ${SUBST_FILES}; do
	OUT_NAME=$(echo "${f}" | sed 's/[.]in$//g')
	echo "'${f}' -> '${OUT_NAME}'"
	cat "${f}" | sed -e "s/__VERSION__/${VERSION}/g" -e "s/__RELEASE__/${RELEASE}/g" > "${OUT_NAME}"
	rm "${f}"
done

# Rename archive to Debian format.
cp "${PACKAGE_SRC}" "${DISTRO_WORK_DIR}/${PACKAGE}_${VERSION}.orig.tar.xz" || exit 1

# Compute MD5 checksum.
compute_md5_checksum () {
	local FILE_NAME="$1"

	local CMD_SHA256SUM=md5sum
	local CMD_OPENSSL=openssl

	if [ -z $(command -v "${CMD_OPENSSL}") ]; then
		echo "Install '${CMD_OPENSSL}' to be able to check checksum file." >&2
		CMD_OPENSSL=""
	fi

	if [ "x${CMD_OPENSSL}" = "x" ]; then
		echo ""
		return 1
	fi

	"${CMD_OPENSSL}" md5 "${FILE_NAME}" | sed -e 's/^.*\s//g'
	return 0
}

# Create Debian archive and complete dsc.
pushd "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb"
ARCHIVE="${PACKAGE}_${VERSION}-${RELEASE}.debian.tar.xz"
tar -chJf "${ARCHIVE}" debian
echo " $(compute_md5_checksum ${ARCHIVE}) $(wc -c ${ARCHIVE})" >> "${PACKAGE}.dsc"
cd ..
ARCHIVE="${PACKAGE}_${VERSION}.orig.tar.xz"
echo " $(compute_md5_checksum ${ARCHIVE}) $(wc -c ${ARCHIVE})" >> "deb/${PACKAGE}.dsc"
popd

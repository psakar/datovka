#!/usr/bin/env sh

# Obtain location of source root.
src_root () {
	local SCRIPT_LOCATION=""
	local SYSTEM=$(uname -s)
	if [ ! "x${SYSTEM}" = "xDarwin" ]; then
		local SCRIPT=$(readlink -f "$0")
		SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))
	else
		SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
	fi

	echo $(cd "$(dirname "${SCRIPT_LOCATION}")"; pwd)
}

PACKAGE="libisds"
VERSION="0.10.7"
RELEASE="1"

SRC_ROOT=$(src_root)
cd "${SRC_ROOT}"

. "${SRC_ROOT}"/scripts/helper_packaging.sh

PACKAGE_SRC="${SRC_ROOT}/libs/srcs/${PACKAGE}-${VERSION}.tar.xz"
file_present "${PACKAGE_SRC}" || exit 1

DISTRO_WORK_DIR="_distrofiles/${PACKAGE}"
rm_and_create_dir "${DISTRO_WORK_DIR}" || exit 1

cp -ra "${SRC_ROOT}/distro/${PACKAGE}/"* "${DISTRO_WORK_DIR}"
rm_swp_files "${DISTRO_WORK_DIR}"

fill_in_files "${DISTRO_WORK_DIR}" || exit 1

# Rename archive to Debian format.
cp -p "${PACKAGE_SRC}" "${DISTRO_WORK_DIR}/${PACKAGE}_${VERSION}.orig.tar.xz" || exit 1

# Create Debian archive and complete dsc.
pushd "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb"
ARCHIVE="${PACKAGE}_${VERSION}-${RELEASE}.debian.tar.xz"
tar -chJf "${ARCHIVE}" debian
echo " $(compute_md5_checksum ${ARCHIVE}) $(wc -c ${ARCHIVE})" >> "${PACKAGE}.dsc"
cd ..
ARCHIVE="${PACKAGE}_${VERSION}.orig.tar.xz"
echo " $(compute_md5_checksum ${ARCHIVE}) $(wc -c ${ARCHIVE})" >> "deb/${PACKAGE}.dsc"
popd

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

SRC_ROOT=$(src_root)
cd "${SRC_ROOT}"

. "${SRC_ROOT}"/scripts/helper_packaging.sh

PROJECT="${HP_OBS_PROJECT}:${HP_REPO_DEVEL}"
PACKAGE="libisds"
VERSION="0.10.7"
RELEASE="1"

CMD_OSC="osc"
if [ -z $(command -v "${CMD_OSC}") ]; then
	echo "Install '${CMD_OSC}' to be able to use the openSUSE Build Service." >&2
	exit 1
fi

DISTRO_WORK_DIR="_distrofiles/${PACKAGE}"
dir_present "${DISTRO_WORK_DIR}" || exit 1

PACKAGE_ORIG_SRC="${DISTRO_WORK_DIR}/${PACKAGE}_${VERSION}.orig.tar.xz"
file_present "${PACKAGE_ORIG_SRC}" || exit 1

${CMD_OSC} co "${PROJECT}" "${PACKAGE}"
pushd "${PROJECT}/${PACKAGE}"
${CMD_OSC} del *
cp "${SRC_ROOT}/${PACKAGE_ORIG_SRC}" ./ || exit 1
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/rpm/${PACKAGE}.spec" ./
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}.dsc" ./ || exit 1
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}_${VERSION}-${RELEASE}.debian.tar.xz" ./ || exit 1
osc addremove
osc ci -n
popd

echo "OK"

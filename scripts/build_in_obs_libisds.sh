#!/usr/bin/env sh

PROJECT="home:CZ-NIC:datovka-devel"
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

CMD_OSC="osc"
if [ -z $(command -v "${CMD_OSC}") ]; then
	echo "Install '${CMD_OSC}' to be able to use the openSUSE Build Service." >&2
	return 1
fi

DISTRO_WORK_DIR="_distrofiles/${PACKAGE}"
if [ ! -d "${DISTRO_WORK_DIR}" ]; then
	echo "Cannot find '${DISTRO_WORK_DIR}'." >&2
	return 1
fi

PACKAGE_ORIG_SRC="${DISTRO_WORK_DIR}/${PACKAGE}_${VERSION}.orig.tar.xz"
if [ ! -f "${PACKAGE_ORIG_SRC}" ]; then
	echo "Cannot find '${PACKAGE_ORIG_SRC}'." >&2
	return 1
fi

${CMD_OSC} co "${PROJECT}" "${PACKAGE}"
pushd "${PROJECT}/${PACKAGE}"
${CMD_OSC} del *
cp "${SRC_ROOT}/${PACKAGE_ORIG_SRC}" ./ || exit 1
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}.dsc" ./ || exit 1
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}_${VERSION}-${RELEASE}.debian.tar.xz" ./ || exit 1
osc addremove
osc ci -n
popd

echo "OK"

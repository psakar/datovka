#!/usr/bin/env sh

PROJECT="home:CZ-NIC:datovka-devel"
PACKAGE="datovka"
VERSION="4.10.2"

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

LIBISDS_ORIG_SRC="${DISTRO_WORK_DIR}/${PACKAGE}_${VERSION}.orig.tar.xz"
if [ ! -f "${LIBISDS_ORIG_SRC}" ]; then
	echo "Cannot find '${LIBISDS_ORIG_SRC}'." >&2
	return 1
fi

${CMD_OSC} co "${PROJECT}" "${PACKAGE}"

pushd "${PROJECT}/${PACKAGE}"
${CMD_OSC} del *
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}.dsc" ./ || exit 1
cp "${SRC_ROOT}/${LIBISDS_ORIG_SRC}" ./ || exit 1
cp "${SRC_ROOT}/${DISTRO_WORK_DIR}/deb/${PACKAGE}_${VERSION}-1.debian.tar.xz" ./ || exit 1
osc addremove
osc ci -n
popd

echo "OK"

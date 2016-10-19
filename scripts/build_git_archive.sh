#!/usr/bin/env sh

DESIRED_VERSION="$1"

SYSTEM=$(uname -s)
if [ ! "x${SYSTEM}" = "xDarwin" ]; then
	SCRIPT=$(readlink -f "$0")
	SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))
else
	SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
fi

SRC_ROOT="${SCRIPT_LOCATION}/.."

cd "${SRC_ROOT}"

ALL_TAGS=$(git tag | sort -fr)
LATEST_TAG=$(echo "${ALL_TAGS}" | head -n 1)
LATEST_VERSION=$(echo "${LATEST_TAG}" | sed -e 's/^v//g')

if [ "x${DESIRED_VERSION}" = "x" ]; then
	DESIRED_VERSION="${LATEST_VERSION}"
fi
DESIRED_TAG="v${DESIRED_VERSION}"

FOUND="no"
for TAG in ${ALL_TAGS}; do
	if [ "${TAG}" = ${DESIRED_TAG} ]; then
		FOUND="yes"
	fi
done

if [ "${FOUND}" = "no" ]; then
	echo "Tag '${DESIRED_TAG}' of desired version '${DESIRED_VERSION}' not found." >&2
	exit 1
fi

echo "Building archive version '${DESIRED_VERSION}' from tag '${DESIRED_TAG}'."

TARGET_FILE="datovka-${DESIRED_VERSION}.tar.xz"
rm -f "${TARGET_FILE}"
git archive --format=tar --prefix=datovka-${DESIRED_VERSION}/ "${DESIRED_TAG}" | xz -9 > "${TARGET_FILE}"

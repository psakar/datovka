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

PACKAGE_NAME="datovka-${DESIRED_VERSION}"
TARGTET_TAR="${PACKAGE_NAME}.tar"
TARGET_COMPRESSED="${PACKAGE_NAME}.tar.xz"

archive_add_qm_files() {
	ARCHIVE="$1"

	LRELEASE=lrelease
	PROJECT_FILE=datovka.pro
	GENERATED_QM_FILES="datovka_cs.qm datovka_en.qm"

	if [ -z $(command -v "${LRELEASE}") ]; then
		echo "Missing command '${LRELEASE}'" >&2
		return 1
	fi

	if [ "x${ARCHIVE}" = "x" ]; then
		echo "No archive specified." >&2
		return 1
	fi
	if [ ! -f "${ARCHIVE}" ]; then
		echo "Supplied archive name '${ARCHIVE}' is not a file." >&2
		return 1
	fi

	TEMPDIR=$(mktemp -d tmp.XXXXXXXX)
	cd "${TEMPDIR}"

	# Extract .tar archive content into a separate copy and generate .qm files.
	tar -xf "../${ARCHIVE}"
	cd "${PACKAGE_NAME}"
	"${LRELEASE}" "${PROJECT_FILE}"
	cd ..

	# Check presence of .qm files.
	ABORT="no"
	for file in ${GENERATED_QM_FILES}; do
		ADDED_FILE_NAME="${PACKAGE_NAME}/locale/${file}"
		if [ ! -f "${ADDED_FILE_NAME}" ]; then
			echo "File '${ADDED_FILE_NAME}' does not exist in '${TEMPDIR}'." >&2
			ABORT="yes"
		fi
	done
	if [ "x${ABORT}" != "xno" ]; then
		return 1
	fi

	# Add .qm files into .tar archive.
	for file in ${GENERATED_QM_FILES}; do
		ADDED_FILE_NAME="${PACKAGE_NAME}/locale/${file}"
		tar --append -f "../${ARCHIVE}" "${ADDED_FILE_NAME}"
	done

	cd ..
	rm -r "${TEMPDIR}"
}

compute_checksum() {
	FILE_NAME="$1"

	CHECKSUM=sha256sum
	SUMSUFF=sha256

	if [ -z $(command -v "${CHECKSUM}") ]; then
		echo "Install '${CHECKSUM}' to be able to create checksum file." >&2
		return 1
	fi

	if [ "x${FILE_NAME}" = "x" ]; then
		echo "Supplied empty file name." >&2
		return 1
	fi

	"${CHECKSUM}" "${FILE_NAME}" | sed -e 's/\s.*$//g' > "${FILE_NAME}.${SUMSUFF}"
}

rm -f "${TARGTET_TAR}" "${TARGET_COMPRESSED}"

# Generate .tar archive from git repository.
git archive --format=tar --prefix="${PACKAGE_NAME}/" "${DESIRED_TAG}" > "${TARGTET_TAR}"

archive_add_qm_files "${TARGTET_TAR}"

cat "${TARGTET_TAR}" | xz -9 > "${TARGET_COMPRESSED}"
rm "${TARGTET_TAR}"

compute_checksum "${TARGET_COMPRESSED}"

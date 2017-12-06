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

ALL_TAGS=$(git tag -l --sort=-version:refname)
LATEST_TAG=$(echo "${ALL_TAGS}" | head -n 1)
LATEST_VERSION=$(echo "${LATEST_TAG}" | sed -e 's/^v//g')

if [ "x${LATEST_VERSION}" = "x" ]; then
	echo "Cannot determine latest version tag." >&2
	exit 1
fi

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

		# Modify timestamps of generated files in order to be able
		# to generate reproducible checksums.
		touch -r "${PACKAGE_NAME}/datovka.pro" "${ADDED_FILE_NAME}"

		tar --append -f "../${ARCHIVE}" "${ADDED_FILE_NAME}"
	done

	cd ..
	rm -r "${TEMPDIR}"
}

compute_checksum() {
	FILE_NAME="$1"

	CMD_SHA256SUM=sha256sum
	CMD_OPENSSL=openssl
	SUMSUFF=sha256

	if [ -z $(command -v "${CMD_SHA256SUM}") ]; then
		echo "Install '${CMD_SHA256SUM}' to be able to check checksum file." >&2
		CMD_SHA256SUM=""
	fi

	if [ -z $(command -v "${CMD_OPENSSL}") ]; then
		echo "Install '${CMD_OPENSSL}' to be able to check checksum file." >&2
		CMD_OPENSSL=""
	fi

	if [ "x${FILE_NAME}" = "x" ]; then
		echo "Supplied empty file name." >&2
		return 1
	fi

	SED=sed
	if [ $(uname) = "Darwin" ]; then
		# OS X version of sed does not recognise \s as white space
		# identifier .
		SED=gsed
	fi

	if [ "x${CMD_SHA256SUM}" != "x" -a "${CMD_OPENSSL}" != "x" ]; then
		SHA256SUM_SUM=$("${CMD_SHA256SUM}" "${FILE_NAME}" | "${SED}" -e 's/\s.*$//g')
		OPENSSL_SUM=$("${CMD_OPENSSL}" sha256 "${FILE_NAME}" | "${SED}" -e 's/^.*\s//g')

		# Compare checksums.
		if [ "x${SHA256SUM_SUM}" = "x${OPENSSL_SUM}" -a "x${SHA256SUM_SUM}" != "x" ]; then
			echo "Checksum comparison OK."
			echo "${SHA256SUM_SUM}" > "${FILE_NAME}.${SUMSUFF}"
		else
			echo "Checksums differ or are empty." >&2
			return 1
		fi
	elif [ "x${CMD_SHA256SUM}" != "x" ]; then
		"${CMD_SHA256SUM}" "${FILE_NAME}" | "${SED}" -e 's/\s.*$//g' > "${FILE_NAME}.${SUMSUFF}"
	elif [ "x${CMD_OPENSSL}" != "x" ]; then
		"${CMD_OPENSSL}" sha256 "${FILE_NAME}" | "${SED}" -e 's/^.*\s//g' > "${FILE_NAME}.${SUMSUFF}"
	else
		echo "No command to compute sha256 checksum." >&2
		return 1
	fi

	SUM_FILE_SIZE=""
	EXPECTED_SUM_FILE_SIZE="65"
	if [ $(uname) != "Darwin" ]; then
		SUM_FILE_SIZE=$(stat -c '%s' "${FILE_NAME}.${SUMSUFF}")
	else
		SUM_FILE_SIZE=$(stat -f '%z' "${FILE_NAME}.${SUMSUFF}")
	fi
	if [ "x${SUM_FILE_SIZE}" = "x${EXPECTED_SUM_FILE_SIZE}" ]; then
		echo "File size of '${FILE_NAME}.${SUMSUFF}' OK."
	else
		echo "Unexpected size '${SUM_FILE_SIZE}' of file '${FILE_NAME}.${SUMSUFF}'."
		rm "${FILE_NAME}.${SUMSUFF}"
		return 1
	fi
}

rm -f "${TARGTET_TAR}" "${TARGET_COMPRESSED}"

# Generate .tar archive from git repository.
git archive --format=tar --prefix="${PACKAGE_NAME}/" "${DESIRED_TAG}" > "${TARGTET_TAR}"

archive_add_qm_files "${TARGTET_TAR}"

cat "${TARGTET_TAR}" | xz -9 > "${TARGET_COMPRESSED}"
rm "${TARGTET_TAR}"

compute_checksum "${TARGET_COMPRESSED}"

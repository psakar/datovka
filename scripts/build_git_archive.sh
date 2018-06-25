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

DESIRED_BRANCH=""
DESIRED_VERSION=""

B_SHORT="-b"
B_LONG="--branch"

H_SHORT="-h"
H_LONG="--help"

V_SHORT="-v"
V_LONG="--version-tag"

USAGE="Usage:\n\t$0 [options] [version]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t${B_SHORT}, ${B_LONG} <branch>\n\t\tSpecify branch to take sources from.\n"
USAGE="${USAGE}\t${H_SHORT}, ${H_LONG}\n\t\tPrints help message.\n"
USAGE="${USAGE}\t${V_SHORT}, ${V_LONG} <version>\n\t\tSpecify version (i.e. tag without leading 'v').\n"

# Parse rest of command line
while [ $# -gt 0 ]; do
	KEY="$1"
	VAL="$2"
	case "${KEY}" in
	${B_SHORT}|${B_LONG})
		if [ "x${VAL}" = "x" ]; then
			echo "Argument '${KEY}' requires an argument." >&2
			exit 1
		fi
		if [ "x${DESIRED_BRANCH}" = "x" ]; then
			DESIRED_BRANCH="${VAL}"
			shift
		else
			echo "Version already specified or in conflict." >&2
			exit 1
		fi
		;;
	${H_SHORT}|${H_LONG})
		echo -e ${USAGE}
		exit 0
		;;
	${V_SHORT}|${V_LONG})
		if [ "x${VAL}" = "x" ]; then
			echo "Argument '${KEY}' requires an argument." >&2
			exit 1
		fi
		if [ "x${DESIRED_VERSION}" = "x" ]; then
			DESIRED_VERSION="${VAL}"
			shift
		else
			echo "Version already specified or in conflict." >&2
			exit 1
		fi
		;;
	--)
		shift
		break
		;;
	-*|*)
		# Unknown option.
		break
		;;
	esac
	shift
done
if [ $# -eq 1 ]; then
	if [ "x${DESIRED_VERSION}" != "x" ]; then
		echo -e "Version tag is already set to '${DESIRED_VERSION}'." >&2
		echo -en ${USAGE} >&2
		exit 1
	fi
	DESIRED_VERSION="$1"
elif [ $# -gt 1 ]; then
	echo -e "Unknown options: $@" >&2
	echo -en ${USAGE} >&2
	exit 1
fi

if [ "x${DESIRED_BRANCH}" != "x" -a "x${DESIRED_VERSION}" != "x" ]; then
	echo "You cannot specify both branch and tag." >&2
	exit 1
fi

LATEST_AVAILABLE_VERSION=""
ALL_AVAILABLE_TAGS=""
#DESIRED_BRANCH=""
DESIRED_TAG=""
#DESIRED_VERSION""

if [ "x${DESIRED_BRANCH}" != "x" ]; then
	# Use latest commit in specified branch.
	ALL_BRANCHES=$(git branch -a)

	FOUND="no"
	for B in ${ALL_BRANCHES}; do
		if [ "${B}" = "${DESIRED_BRANCH}" ]; then
			FOUND="yes"
		fi
	done
	if [ "${FOUND}" = "no" ]; then
		echo "Branch '${DESIRED_BRANCH}' not found." >&2
		exit 1
	fi

	ALL_AVAILABLE_TAGS=$(git tag -l --sort=-version:refname --merged "${DESIRED_BRANCH}")
	LATEST_TAG=$(echo "${ALL_AVAILABLE_TAGS}" | head -n 1)
	LATEST_VERSION=$(echo "${LATEST_TAG}" | sed -e 's/^v//g')
	if [ "x${LATEST_VERSION}" = "x" ]; then
		echo "Cannot determine latest version tag." >&2
		exit 1
	fi

	DESIRED_VERSION="${LATEST_VERSION}"

	echo "Building archive version '${DESIRED_VERSION}' from branch '${DESIRED_BRANCH}'."
else
	# Use latest or specified version.
	ALL_AVAILABLE_TAGS=$(git tag -l --sort=-version:refname)
	LATEST_TAG=$(echo "${ALL_AVAILABLE_TAGS}" | head -n 1)
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
	for T in ${ALL_AVAILABLE_TAGS}; do
		if [ "${T}" = ${DESIRED_TAG} ]; then
			FOUND="yes"
		fi
	done
	if [ "${FOUND}" = "no" ]; then
		echo "Tag '${DESIRED_TAG}' of desired version '${DESIRED_VERSION}' not found." >&2
		exit 1
	fi

	echo "Building archive version '${DESIRED_VERSION}' from tag '${DESIRED_TAG}'."
fi

archive_add_qm_files() {
	local ARCHIVE="$1"
	local PACKAGE_NAME="$2"

	local LRELEASE=lrelease
	local PROJECT_FILE=datovka.pro
	local GENERATED_QM_FILES="datovka_cs.qm datovka_en.qm"

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

	if [ "x${PACKAGE_NAME}" = "x" ]; then
		echo "No package name specified." >&2
		return 1
	fi

	local TEMPDIR=$(mktemp -d tmp.XXXXXXXX)
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
	local FILE_NAME="$1"

	local CMD_SHA256SUM=sha256sum
	local CMD_OPENSSL=openssl
	local SUMSUFF=sha256

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
		echo "Unexpected size '${SUM_FILE_SIZE}' of file '${FILE_NAME}.${SUMSUFF}'." >&2
		rm "${FILE_NAME}.${SUMSUFF}"
		return 1
	fi
}

COMMIT_ID=""
PACKAGE_NAME=""
TARGTET_TAR=""
TARGET_COMPRESSED=""

if [ "x${DESIRED_BRANCH}" != "x" ]; then
	COMMIT_ID=$(git rev-parse "${DESIRED_BRANCH}")
	SHORT_COMMIT_ID=$(git rev-parse --short=16 "${DESIRED_BRANCH}")
	UTC_TIME=$(date -u +%Y%m%d.%H%M%S)
	PACKAGE_NAME="datovka-${DESIRED_VERSION}.9999.${UTC_TIME}.${SHORT_COMMIT_ID}"
	TARGTET_TAR="${PACKAGE_NAME}.tar"
	TARGET_COMPRESSED="${PACKAGE_NAME}.tar.xz"
elif [ "x${DESIRED_TAG}" != "x" ]; then
	COMMIT_ID=$(git rev-list -n 1 "${DESIRED_TAG}")
	PACKAGE_NAME="datovka-${DESIRED_VERSION}"
	TARGTET_TAR="${PACKAGE_NAME}.tar"
	TARGET_COMPRESSED="${PACKAGE_NAME}.tar.xz"
else
	echo "Cannot build git archive." >&2
	exit 1
fi

if [ "x${COMMIT_ID}" = "x" ]; then
	echo "Could not determine commit identifier." >&2
	exit 1
fi

rm -f "${TARGTET_TAR}" "${TARGET_COMPRESSED}"

# Generate .tar archive from git repository.
if [ "x${DESIRED_BRANCH}" != "x" ]; then
	# Build from tag.
	git archive --format=tar --prefix="${PACKAGE_NAME}/" "${DESIRED_BRANCH}" > "${TARGTET_TAR}"
elif [ "x${DESIRED_TAG}" != "x" ]; then
	# Build from tag.
	git archive --format=tar --prefix="${PACKAGE_NAME}/" "${DESIRED_TAG}" > "${TARGTET_TAR}"
fi

TAR_COMMIT_ID=$(git get-tar-commit-id < "${TARGTET_TAR}")
if [ "x${COMMIT_ID}" != "x${TAR_COMMIT_ID}" ]; then
	echo "Commit identifiers '${COMMIT_ID}' and '${TAR_COMMIT_ID}' are different." >&2
	exit 1
fi

archive_add_qm_files "${TARGTET_TAR}" "${PACKAGE_NAME}"

cat "${TARGTET_TAR}" | xz -9 > "${TARGET_COMPRESSED}"
rm "${TARGTET_TAR}"

compute_checksum "${TARGET_COMPRESSED}"

echo "${TARGET_COMPRESSED}"

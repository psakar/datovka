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

DB_SHORT="-D"
DB_LONG="--desktop-branch"
DB="develop"
D_REPO="https://gitlab.labs.nic.cz/datovka/datovka.git"

MB_SHORT="-M"
MB_LONG="--mobile-branch"
MB="develop"
M_REPO="https://gitlab.labs.nic.cz/datovka/mobile-datovka.git"

WD_SHORT="-d"
WD_LONG="--work-dir"
WD=""

H_SHORT="-h"
H_LONG="--help"

USAGE=""
USAGE="${USAGE}Usage:\n"
USAGE="${USAGE}\t$0 [options]\n\n"
USAGE="${USAGE}Supported options:\n"
USAGE="${USAGE}\t${DB_SHORT}, ${DB_LONG} <branch-name>\n\t\tSpecify desktop branch for code comparison.\n"
USAGE="${USAGE}\t${MB_SHORT}, ${MB_LONG} <branch-name>\n\t\tSpecify mobile branch for code comparison.\n"
USAGE="${USAGE}\t${WD_SHORT}, ${WD_LONG} <directory>\n\t\tSpecify working directory.\n"
USAGE="${USAGE}\t${H_SHORT}, ${H_LONG}\n\t\tPrint help message.\n"
USAGE="${USAGE}\n"
USAGE="${USAGE}Usage example:\n"
USAGE="${USAGE}\t$0 ${WD_SHORT} /tmp\n"

#cd "${SRC_ROOT}"

ECHO_CMD="echo -en"

# Parse rest of command line
POSITIONAL_ARGS=""
#set -- $(getopt D:M: "$@")
#if [ $# -lt 1 ]; then
#	${ECHO_CMD} "${USAGE}" >&2
#	exit 1
#fi
while [ $# -gt 0 ]; do
	KEY="$1"
	VAL="$2"
	case ${KEY} in
	${DB_SHORT}|${DB_LONG})
		if [ "x${VAL}" = "x" ]; then
			${ECHO_CMD} "Value needed for desktop branch.\n" >&2
			${ECHO_CMD} "${USAGE}" >&2
			exit 1
		fi
		DB="${VAL}"
		shift
		;;
	${MB_SHORT}|${MB_LONG})
		if [ "x${VAL}" = "x" ]; then
			${ECHO_CMD} "Value needed for mobile branch.\n" >&2
			${ECHO_CMD} "${USAGE}" >&2
			exit 1
		fi
		MB="${VAL}"
		shift
		;;
	${WD_SHORT}|${WD_LONG})
		if [ "x${VAL}" = "x" ]; then
			${ECHO_CMD} "Value needed for directory name.\n" >&2
			${ECHO_CMD} "${USAGE}" >&2
			exit 1
		fi
		WD="${VAL}"
		shift
		;;
	${H_SHORT}|${H_LONG})
		${ECHO_CMD} "${USAGE}"
		exit 0
		;;
	--)
		shift
		break
		;;
	-*)
		${ECHO_CMD} "${USAGE}" >&2
		exit 1
		;;
	*)
		${ECHO_CMD} "${USAGE}" >&2
		exit 1
		;;
	esac
	shift
done
if [ $# -gt 0 ]; then
        ${ECHO_CMD} "Unknown options: $@" >&2
        ${ECHO_CMD} ${USAGE} >&2
        exit 1
fi

if [ "x${WD}" = "x" ]; then
	WD=$(mktemp -p /tmp -d datovka_repo_check_XXXXXXXX)
else
	if [ ! -e "${WD}" ]; then
		mkdir -p "${WD}"
	fi
	if [ ! -d "${WD}" -o ! -w "${WD}" ]; then
		${ECHO_CMD} "'${WD}' is not a writeable directory.\n" >&2
		exit 1
	fi
fi

cd "${WD}"

DESK="datovka"
MOBI="mobile-datovka"

SHARED_DIR_NAME="datovka_shared"

# Download specified branch.
download_repo () {
	REPO="$1"
	BRANCH="$2"
	DIR="$3"
	DIR_PATH=$(pwd)"/${DIR}"

	if [ "x${REPO}" = "x" -o "x${BRANCH}" = "x" -o "x${DIR}" = "x" ]; then
		echo "Missing parameters in download_repo()." >&2
		exit 1
	fi

	if [ ! -e "${DIR}" ]; then
		git clone "${REPO}" "${DIR}" || exit 1
	fi

	if [ -d "${DIR}" -a -x "${DIR}" -a -w "${DIR}" ]; then
		DIT_DIR="${DIR}/.git"
		if [ ! -d "${DIT_DIR}" ]; then
			echo echo "'${DIR_PATH}' is not a git repository." >&2
		fi
	fi

	if [ ! -d "${DIR}" -o ! -x "${DIR}" -o ! -w "${DIR}" ]; then
		echo "'${DIR_PATH}' is not a writeable directory." >&2
		exit 1
	fi

	cd "${DIR}"

	git clean -fd
	git clean -fx
	git checkout -- . || exit 1
	git checkout "${BRANCH}" || exit 1
	git pull || exit 1

	cd ..
}

# Compare directory content.
compare_dir_content () {
	D_SHARED="$1"
	M_SHARED="$2"

	if [ "x${D_SHARED}" = "x" -o "x${M_SHARED}" = "x" ]; then
		echo "Missing parameters in compare_dir_content()." >&2
		exit 1
	fi

	if [ ! -d "${D_SHARED}" -o ! -x "${D_SHARED}" ]; then
		echo "'${D_SHARED}' is not a directory." >&2
		exit 1
	fi
	if [ ! -d "${M_SHARED}" -o ! -x "${M_SHARED}" ]; then
		echo "'${M_SHARED}' is not a directory." >&2
		exit 1
	fi

	RESULT=$(diff -rq "${D_SHARED}" "${M_SHARED}")
	if [ "x${RESULT}" = "x" ]; then
		RESULT="Shared code equal."
	fi

	echo "${RESULT}"
}

${ECHO_CMD} "------------------------\n"
download_repo "${D_REPO}" "${DB}" "${DESK}"
download_repo "${M_REPO}" "${MB}" "${MOBI}"
${ECHO_CMD} "------------------------\n"
compare_dir_content "${DESK}/src/datovka_shared" "${MOBI}/src/datovka_shared"
${ECHO_CMD} "------------------------\n"

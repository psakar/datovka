#!/usr/bin/env bash

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

	echo $(cd "$(dirname "${SCRIPT_LOCATION}")"; cd ..; pwd)
}

SRC_ROOT=$(src_root)
cd "${SRC_ROOT}"

SCRIPT_PATH="${SRC_ROOT}/tests/cli"

. "${SCRIPT_PATH}/helper.sh" # Contains HAVE_ERROR variable.

# Determine tested executable path.
APP_BINARY_NAME="datovka"
BINARY_LOCATION="${SRC_ROOT}"
OS_NAME="unknown"
ATTACH_LOAD_PATH="${SCRIPT_PATH}/attachment"
TMP_DIR=$(mktemp -d ${SRC_ROOT}/tmp-XXXX) # Create temporary directory.

case "${OSTYPE}" in
linux-gnu)
	APP_BINARY_NAME="datovka"
	BINARY_LOCATION="${SRC_ROOT}"
	OS_NAME="Linux"
	ATTACH_LOAD_PATH="${SCRIPT_PATH}/attachment"
	ATTACH_TMP_DIR="${TMP_DIR}"
	;;
darwin*)
	APP_BINARY_NAME="datovka"
	BINARY_LOCATION="${SRC_ROOT}/datovka.app/Contents/MacOS"
	OS_NAME="macOS"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	ATTACH_TMP_DIR="${TMP_DIR}"
	;;
msys|win32)
	APP_BINARY_NAME="datovka-cli.exe"
	BINARY_LOCATION="C:/Program Files (x86)/CZ.NIC/Datovka"
	OS_NAME="Windows"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:0:1}:${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_TMP_DIR="${TMP_DIR}"
	ATTACH_TMP_DIR="${ATTACH_TMP_DIR:1:${#ATTACH_TMP_DIR}}"
	ATTACH_TMP_DIR="${ATTACH_TMP_DIR:0:1}:${ATTACH_TMP_DIR:1:${#ATTACH_TMP_DIR}}"
	;;
*)
	echo_error "ERROR: Unknown platform"
	exit 1
	;;
esac

APP_BINARY_PATH="${BINARY_LOCATION}/${APP_BINARY_NAME}"
if [ ! -e "${APP_BINARY_PATH}" ]; then
	echo_error "ERROR: Cannot locate tested binary."
	exit 1
fi
if [ ! -d "${ATTACH_LOAD_PATH}" ]; then
	echo_error "ERROR: Cannot locate directory where to load attachments from."
	exit 1
fi
if [ ! -d "${TMP_DIR}" ]; then
	echo_error "ERROR: cannot access temporary directory."
	exit 1
fi


# Run Login tests
"${SCRIPT_PATH}/login.sh" "${APP_BINARY_PATH}" || HAVE_ERROR="true"

# Run Send message tests
"${SCRIPT_PATH}/sendmsg.sh" "${APP_BINARY_PATH}" "${OS_NAME}" "${ATTACH_LOAD_PATH}" || HAVE_ERROR="true"

# Run Get message list and download message tests
"${SCRIPT_PATH}/getmsg.sh" "${APP_BINARY_PATH}" "${OS_NAME}" "${ATTACH_LOAD_PATH}" "${ATTACH_TMP_DIR}" || HAVE_ERROR="true"

# Run Find data box tests
"${SCRIPT_PATH}/find.sh" "${APP_BINARY_PATH}" || HAVE_ERROR="true"

rm -r "${TMP_DIR}" # delete temporary directory.

if [ "x${HAVE_ERROR}" = "xfalse" ]; then
	echo ""
	echo_success "======================================================================="
	echo_success "SUCCESS: All CLI tests have finished successfully."
	echo_success "======================================================================="
	echo ""
	exit 0
else
	echo ""
	echo_error "======================================================================="
	echo_error "FAILURE: Some CLI tests have failed!"
	echo_error "======================================================================="
	echo ""
	exit 1
fi

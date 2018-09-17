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


. "${SRC_ROOT}/untracked/logins.sh"

APP_BINARY_PATH="$1"
if [ ! -e "${APP_BINARY_PATH}" ]; then
	echo_error "ERROR: Cannot locate tested binary."
	exit 1
fi

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"


echo ""
echo "***********************************************************************"
echo "FIND DATA BOX TEST: Find data boxes by ic, firmname"
echo "***********************************************************************"
# this request must finish with success
"${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX1'" \
	--find-databox "dbType='OVM',ic='12345678'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo_error "FindDatabox: $USERNAME_FIND_DATABOX1 - ERROR"
else
	echo_success "FindDatabox: $USERNAME_FIND_DATABOX1 - OK"
fi
echo ""

# this request must finish with success
"${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX1'" \
	--find-databox "dbType='OVM',ic='00000001'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo_error "FindDatabox: $USERNAME_FIND_DATABOX1 - ERROR"
else
	echo_success "FindDatabox: $USERNAME_FIND_DATABOX1 - OK"
fi
echo ""

# this request must finish with success
"${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX2'" \
	--find-databox "dbType='OVM',firmName='Ministerstvo dopravy'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo_error "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR"
else
	echo_success "FindDatabox: $USERNAME_FIND_DATABOX2 - OK"
fi
echo ""

# this request must finish with error
"${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX2'" \
	--find-databox "dbType='FO',firmName='Ministerstvo dopravy'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo_success "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR: it is OK"
else
	echo_error "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR"
fi

if [ "x${HAVE_ERROR}" = "xfalse" ]; then
	echo ""
	echo_success "-----------------------------------------------------------------------"
	echo_success "SUCCESS: All find data box tests have finished as expected."
	echo_success "-----------------------------------------------------------------------"
	echo ""
	exit 0
else 
	echo ""
	echo_error "-----------------------------------------------------------------------"
	echo_error "FAILURE: Some find data box tests have failed!"
	echo_error "-----------------------------------------------------------------------"
	echo ""
	exit 1
fi

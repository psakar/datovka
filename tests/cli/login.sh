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

CMDARGS=""
CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"


echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Log in to non-existent accounts and usernames."
echo "***********************************************************************"
for username in $NOEXIST_USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$username'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_success "Login: '$username' - username doesn't exist in datovka - OK"
	else
		echo_error "Login: '$username' - ERROR: username shouldn't exist in datovka!"
	fi
done

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Log in to data box for all existing accounts where"
echo "            username and password is used and remembered."
echo "***********************************************************************"
for username in $USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$username'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_error "Login: '$username' - ERROR: username doesn't exist or required data missing!"
	else
		echo_success "Login: '$username' - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Log in to data box using a certificate."
echo "            Note: certificate password is required."
echo "***********************************************************************"
"${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_CERT',otpcode='$USERNAME_CERT_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo_error "Login: '$USERNAME_CERT' - ERROR: username doesn't exist or certificate is wrong or missing!"
else
	echo_success "Login: $USERNAME_CERT - OK"
fi

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Log in to data box for all existing accounts where"
echo "            user has restricted privileges."
echo "***********************************************************************"
for username in $RESTRICT_USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$username'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_error "Login: '$username' - ERROR: username doesn't exist or required data missing!"
	else
		echo_success "Login: '$username' - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Log in to data box where password"
echo "            is not stored in the dsgui.conf."
echo "***********************************************************************"
"${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_NOPWD',password='$USERNAME_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo_error "Login: '$USERNAME_NOPWD' - ERROR: account doesn't exist or required password is wrong or missing!"
else
	echo_success "Login: $USERNAME_NOPWD - external password was used - OK"
fi

echo ""
echo "***********************************************************************"
echo "USER INFO TEST: Obtaining user info for exist accounts."
echo "***********************************************************************"
#---Get user info for account with username and pwd---
for username in $USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$username'" \
		--get-user-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_error "Get user info: $username - ERROR"
	else
		echo_success "Get user info: $username - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "OWNER INFO TEST: Obtaining owner info for exist accounts."
echo "***********************************************************************"
#---Get owner info for account with username and pwd---
for username in $USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$username'" \
		--get-owner-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_error "Get owner info: $username - ERROR"
	else
		echo_success "Get owner info: $username - OK"
	fi
done

if [ "x${HAVE_ERROR}" = "xfalse" ]; then
	echo ""
	echo_success "-----------------------------------------------------------------------"
	echo_success "SUCCESS: All login tests have finished as expected."
	echo_success "-----------------------------------------------------------------------"
	echo ""
	exit 0
else
	echo ""
	echo_error "-----------------------------------------------------------------------"
	echo_error "FAILURE: Some login tests have failed!"
	echo_error "-----------------------------------------------------------------------"
	echo ""
	exit 1
fi

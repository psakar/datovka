#!/usr/bin/env bash

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"

. "${SCRIPTPATH}/../../untracked/logins.sh"

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	APP_BINARY_NAME="datovka"
	APP_PATH="${SCRIPTPATH}/../.."
elif [[ "$OSTYPE" == "darwin"* ]]; then
	APP_BINARY_NAME="datovka"
	APP_PATH="${SCRIPTPATH}/../../datovka.app/Contents/MacOS"
elif [[ "$OSTYPE" == "msys" ]]; then
	APP_BINARY_NAME="datovka-cli.exe"
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"
elif [[ "$OSTYPE" == "win32" ]]; then
	APP_BINARY_NAME="datovka-cli.exe"
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"
else
	echo "ERROR: Unknown platform"
	exit
fi

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Login into non exist accoutns and usernames."
echo "***********************************************************************"
for username in $NOEXIST_USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$username'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Login: '$username' - username not exists in datovka - OK"
	else
		echo "Login: '$username' - ERROR: username shouldnt exists in datovka!"
		exit
	fi
done

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Login into databox for all existing accounts where"
echo "            username and password is used and remembered." 
echo "***********************************************************************"
for username in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$username'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Login: '$username' - ERROR: username not exists or required data missing!"
		exit
	else
		echo "Login: '$username' - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Login into databox with certificate."
echo "            Note: certificate password is requried."
echo "***********************************************************************"
"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_CERT',otpcode='$USERNAME_CERT_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: '$USERNAME_CERT' - ERROR: username not exists or required data missing!"
	exit
else
	echo "Login: $USERNAME_CERT - OK"
fi

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Login into databox for all existing accounts where"
echo "            user has restricted privilegies." 
echo "***********************************************************************"
for username in $RESTRICT_USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$username'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Login: '$username' - ERROR: username not exists or required data missing!"
		exit
	else
		echo "Login: '$username' - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "LOGIN TEST: Login into databox where password"
echo "            is not stored in the dsgui.conf."
echo "***********************************************************************"
"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_NOPWD',password='$USERNAME_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: '$USERNAME_NOPWD' - ERROR: account not exists or required data missing!"
	exit
else
	echo "Login: $USERNAME_NOPWD - external password was used - OK"
fi

echo ""
echo "***********************************************************************"
echo "USER INFO TEST: Obtaining user info for exist accounts."
echo "***********************************************************************"
#---Get user info for account with username and pwd---
for username in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$username'" \
		--get-user-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "User info: $username - ERROR"
		exit
	else
		echo "User info: $username - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "OWNER INFO TEST: Obtaining owner info for exist accounts."
echo "***********************************************************************"
#---Get owner info for account with username and pwd---
for username in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$username'" \
		--get-owner-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Owner info: $username - ERROR"
		exit
	else
		echo "Owner info: $username - OK"
	fi
done

echo ""
echo ""
echo "------------------------------------------------------------------------"
echo "CONGRATULATION: All login tests were done with success."
echo "------------------------------------------------------------------------"
echo ""
